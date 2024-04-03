/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_world.c -- NEW WORLD RENDERING CODE

#include <assert.h>
#include "r_local.h"

extern cvar_t		*r_fastworld;

extern vec3_t		modelorg;		// relative to viewpoint
extern msurface_t	*r_alpha_surfaces;
extern image_t		*R_World_TextureAnimation(mtexinfo_t* tex);


#define MAX_INDICES 16384 // build indices untill this

// gfx world stores the current state for batching drawcalls, if any 
// of them change we draw what we have and begin building indices again
typedef struct gfx_world_s
{
	int			tex_diffuse;
	int			tex_lm;

	vec3_t		styles[MAX_LIGHTMAPS_PER_SURFACE];
	vec3_t		new_styles[MAX_LIGHTMAPS_PER_SURFACE];

	qboolean	isWarp;

	GLuint		vbo;
	int			totalVertCount;

	int			numIndices;
	GLuint		indicesBuf[MAX_INDICES];

	qboolean	isRenderingWorld;

} gfx_world_t;

static gfx_world_t gfx_world = { 0 };


// ==============================================================================
// VERTEX BUFFER MANAGMENT
// ==============================================================================

/*
=================
R_DestroyWorldVertexBuffer

Delete world's vbo
=================
*/
static void R_DestroyWorldVertexBuffer()
{
	if (!gfx_world.vbo)
		return;

	glDeleteBuffers(1, &gfx_world.vbo);
	gfx_world.vbo = 0;
}

/*
=================
R_BuildVertexBufferForWorld

upload all world surfaces as one vertex buffer
=================
*/
static void R_BuildVertexBufferForWorld()
{
	msurface_t	*surf;
	poly_t		*p, *bp;
	int			i, j, v, bufsize;
	GLint		ongpusize = 0;

	polyvert_t* tempVerts;

	if (!r_worldmodel)
		return;

	// walk all surfaces, set their firstvert and count the vertices to build a temp buffer
	gfx_world.totalVertCount = 0;
	for (surf = r_worldmodel->surfaces, i = 0; i < r_worldmodel->numsurfaces; i++, surf++)
	{
		surf->firstvert = gfx_world.totalVertCount;
		surf->numverts = 0;

		// warps have polygon chains
		for (bp = surf->polys; bp; bp = bp->next)
		{
			p = bp;
			surf->numverts += p->numverts;
		}
		gfx_world.totalVertCount += surf->numverts;

//		ri.Printf(PRINT_ALL, "%i verts in %i surf\n", surf->numverts, i);
	}

	bufsize = sizeof(polyvert_t) * gfx_world.totalVertCount; // this must match the vbo size on gpu, otherwise we probably failed to upload due to vram limit

	// create temp buffer
	tempVerts = malloc(bufsize);
	if (!tempVerts)
		ri.Error(ERR_FATAL, "failed to allocate %i kb for gfx_world", bufsize / 1024);

	for (surf = r_worldmodel->surfaces, v = 0, i = 0; i < r_worldmodel->numsurfaces; i++, surf++)
	{
		// copy verts to temp buffer
		for (bp = surf->polys; bp; bp = bp->next)
		{
			p = bp;
			for (j = 0; j < p->numverts; j++)
			{
				memcpy(&tempVerts[v], &p->verts[j], sizeof(polyvert_t));
				v++;
			}
		}

	}

	// upload vertices to gpu
	glGenBuffers(1, &gfx_world.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glBufferData(GL_ARRAY_BUFFER, bufsize, &tempVerts[0].pos[0], GL_STATIC_DRAW);

	free(tempVerts);

	// check if its all right
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &ongpusize);
	if (ongpusize != bufsize)
	{
		R_DestroyWorldVertexBuffer();
		ri.Error(ERR_FATAL, "failed to allocate vertex buffer for gfx_world (probably out of VRAM)\n");
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ri.Printf(PRINT_ALL, "World surface cache is %i kb (%i surfaces, %i verts)\n", bufsize / 1024, r_worldmodel->numsurfaces, gfx_world.totalVertCount);
}

// ==============================================================================
// RENDERING
// ==============================================================================

/*
===============
R_World_BeginRendering

Binds the world's VBO, shader and assign attrib locations for rendering the bsp
No UBOs in GL2.1 so we have to bind attrib locations each time we start rendering world, binding diferent VBO invalidates locations
===============
*/
static void R_World_BeginRendering()
{
	int attrib;

	// bind shader and buffer and assign attrib locations

	R_BindProgram(GLPROG_WORLD_NEW);
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);

	attrib = R_GetProgAttribLoc(VALOC_POS);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), NULL);
	}

	attrib = R_GetProgAttribLoc(VALOC_TEXCOORD);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, texCoord));
	}

	attrib = R_GetProgAttribLoc(VALOC_LMTEXCOORD);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, lmTexCoord));
	}

	attrib = R_GetProgAttribLoc(VALOC_NORMAL);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, normal));
	}

	attrib = R_GetProgAttribLoc(VALOC_LIGHTFLAGS);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, lightFlags));
	}

	gfx_world.isRenderingWorld = true;
}

/*
===============
R_World_EndRendering
===============
*/
static void R_World_EndRendering()
{
	if (!gfx_world.isRenderingWorld)
		return;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	R_UnbindProgram();

	gfx_world.isRenderingWorld = false;
}

/*
=======================
R_World_UpdateLightStylesForSurf

Update lightstyles for surface, pass NULL to disable lightstyles, this is slightly
diferent from R_LightMap_UpdateLightStylesForSurf and returns true when style has changed
=======================
*/
static qboolean R_World_UpdateLightStylesForSurf(msurface_t* surf)
{
	qboolean hasChanged = false;
	int i;

	if (surf == NULL)
	{
		VectorSet(gfx_world.new_styles[0], 1.0f, 1.0f, 1.0f);
		for (i = 1; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
			VectorClear(gfx_world.new_styles[i]);
		goto change;
	}

	if (!r_dynamic->value) // r_dynamic off disables lightstyles
	{
		for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
			VectorSet(gfx_world.new_styles[i], 1.0f, 1.0f, 1.0f);
	}
	else
	{
		// add light styles
		for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[i] != (MAX_LIGHTSTYLES - 1); i++)
		{
			gfx_world.new_styles[i][0] = r_newrefdef.lightstyles[surf->styles[i]].rgb[0];
			gfx_world.new_styles[i][1] = r_newrefdef.lightstyles[surf->styles[i]].rgb[1];
			gfx_world.new_styles[i][2] = r_newrefdef.lightstyles[surf->styles[i]].rgb[2];
		}
	}

change:
	for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
	{
		if (!VectorCompare(gfx_world.new_styles[i], gfx_world.styles[i]))
		{
			hasChanged = true;
		}
	}

	return hasChanged;
}


/*
=======================
World_DrawAndFlushBufferedGeo


=======================
*/
static void World_DrawAndFlushBufferedGeo()
{
	if (!gfx_world.numIndices || !gfx_world.isRenderingWorld)
		return;

	rperf.brush_drawcalls++;

	R_MultiTextureBind(TMU_DIFFUSE, gfx_world.tex_diffuse);
	R_MultiTextureBind(TMU_LIGHTMAP, gfx_world.tex_lm);

	R_ProgUniform3fv(LOC_LIGHTSTYLES, MAX_LIGHTMAPS_PER_SURFACE, gfx_world.styles[0]);

//	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glDrawElements(GL_TRIANGLES, gfx_world.numIndices, GL_UNSIGNED_INT, &gfx_world.indicesBuf[0]);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);

	gfx_world.numIndices = 0;
}



/*
=================
R_NEW_DrawInlineBModel
=================
*/
void R_DrawInlineBModel_NEW()
{
}


/*
================
R_DrawWorld_TextureChains_NEW
================
*/
static void R_DrawWorld_TextureChains_NEW()
{
	int		i;
	msurface_t* s;
	image_t* image;

	rperf.brush_textures = 0;

	R_World_BeginRendering();

	//
	// lightmapped surfaces
	//
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence || !image->texturechain)
			continue;

		rperf.brush_textures++;

		for (s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
			{ 
				// draw all lightmapped surfs
			}
		}
	}

	//
	// unlit and/or fullbright surfaces
	//
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		s = image->texturechain;
		if (!s)
			continue;

		for (; s; s = s->texturechain)
		{
			if (s->flags & SURF_DRAWTURB)
			{
				// draw surf
			}
		}

		if (!r_showtris->value) // texture chains are used in r_showtris
			image->texturechain = NULL;
	}

	R_World_EndRendering();
}

/*
================
R_World_DrawAlphaSurfaces_NEW
================
*/
void R_World_DrawAlphaSurfaces_NEW()
{
	msurface_t* surf;

	glLoadMatrixf(r_world_matrix);

	R_World_BeginRendering();

	R_Blend(true);
	for (surf = r_alpha_surfaces; surf; surf = surf->texturechain)
	{
		if (surf->flags & SURF_DRAWTURB)
		{
			// draw unlit water surf chain
		}
		else
		{
			// draw unlit surf
		}
	}
	R_Blend(false);

	R_World_EndRendering();

	r_alpha_surfaces = NULL;
}


/*
================
R_DrawWorld_NEW

================
*/
void R_DrawWorld_NEW()
{
	// we already have texturechains from R_DrawWorld

	if (!gfx_world.vbo) // should move this to Mod_LoadFaces or Mod_LoadBSP
	{
		R_BuildVertexBufferForWorld();
	}

	R_World_BeginRendering(); // at this stage, we have VBO and shader bound and can render world
	{
		R_DrawWorld_TextureChains_NEW();
	}
	R_World_EndRendering();
}





/*
=================
R_DynamicLightsToProg
=================
*/
void R_DynamicLightsToProg()
{
	dlight_t* dlight;
	int			i, j;
	vec4_t		dl_pos_and_rad[MAX_DLIGHTS]; // holds xyz + radius for each light
	vec3_t		dl_colors[MAX_DLIGHTS];
	int			numDynLights;

	if (!r_worldmodel)
		return;

	numDynLights = !r_dynamic->value ? 0 : r_newrefdef.num_dlights; // no dlights when r_dynamic is off

	dlight = r_newrefdef.dlights;
	for (i = 0; i < numDynLights; i++, dlight++)
	{
		for (j = 0; j < 3; j++)
			dl_pos_and_rad[i][j] = dlight->origin[j];
		dl_pos_and_rad[i][3] = dlight->intensity;
		VectorCopy(dlight->color, dl_colors[i]);
	}

	R_BindProgram(r_fastworld->value ? GLPROG_WORLD_NEW : GLPROG_WORLD);
	R_ProgUniform1i(LOC_DLIGHT_COUNT, numDynLights);
	if (numDynLights > 0)
	{
		R_ProgUniform3fv(LOC_DLIGHT_COLORS, numDynLights, &dl_colors[0][0]);
		R_ProgUniform4fv(LOC_DLIGHT_POS_AND_RAD, numDynLights, &dl_pos_and_rad[0][0]);
	}
	R_UnbindProgram();
}