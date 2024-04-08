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

extern vec3_t		modelorg;		// relative to viewpoint
extern msurface_t	*r_alpha_surfaces;
extern image_t		*R_World_TextureAnimation(mtexinfo_t* tex);

extern int registration_sequence; // experimental nature heh

#define MAX_INDICES 16384 // build indices untill this

// gfx world stores the current state for batching drawcalls, if any 
// of them change we draw what we have so far and begin building indices again
typedef struct gfx_world_s
{
	int			tex_diffuse;
	int			tex_lm;

	vec3_t		styles[MAX_LIGHTMAPS_PER_SURFACE];
	vec3_t		new_styles[MAX_LIGHTMAPS_PER_SURFACE];

	vec4_t		color;
	 
	unsigned int uniformflags; //Flags that require a uniform change. 
							   //Currently SURF_DRAWTURB, SURF_FLOWING, SURF_SCROLLX, SURF_SCROLLY, SURF_SCROLLFLIP, and probably sky

	GLuint		vbo;
	int			totalVertCount;

	int			numIndices;
	GLuint		indicesBuf[MAX_INDICES];

	qboolean	isRenderingWorld;
	int			registration_sequence; // hackery
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

	ri.Printf(PRINT_ALL, "Freed world surface cache.\n");
	memset(&gfx_world, 0, sizeof(gfx_world));
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

	ri.Printf(PRINT_LOW, "World surface cache is %i kb (%i verts in %i surfaces)\n", bufsize / 1024, gfx_world.totalVertCount, r_worldmodel->numsurfaces);
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

	attrib = R_GetProgAttribLoc(VALOC_ALPHA);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 1, GL_FLOAT, GL_FALSE, sizeof(polyvert_t), (void*)offsetof(polyvert_t, alpha));
	}

	R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, 1.0f);
	R_ProgUniform1f(LOC_WARPSTRENGTH, 0.f);
	R_ProgUniform2f(LOC_FLOWSTRENGTH, 0.f, 0.f);

	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);

	gfx_world.uniformflags = 0; //Always force a uniform update if needed
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
R_World_DrawAndFlushBufferedGeo

This should be also called at the end of rendering to make sure we drawn everything
=======================
*/
static void R_World_DrawAndFlushBufferedGeo()
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


// ==============================================================================
// BATCHING
// ==============================================================================

/*
=======================
R_World_GrabSurfaceTextures

Grabs the diffuse and lightmap textures for given surface
=======================
*/
static void R_World_GrabSurfaceTextures(const msurface_t* surf, int *outDiffuse, int *outLM)
{
	image_t* image;
	qboolean lightmapped = true;

	image = R_World_TextureAnimation(surf->texinfo);
	if (!image)
		image = r_texture_missing;

	if (surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
		lightmapped = false;

	*outDiffuse = r_lightmap->value ? r_texture_white->texnum : image->texnum;
	*outLM = (r_fullbright->value || !lightmapped) ? r_texture_white->texnum : (gl_state.lightmap_textures + surf->lightMapTextureNum);

	if (*outDiffuse == r_texture_white->texnum && *outLM == r_texture_white->texnum)
		*outDiffuse = image->texnum; // never let the world render completly white
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

	if (!r_dynamic->value) // r_dynamic 0 disables lightstyles and dlights
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
R_World_NewDrawSurface
=======================
*/
static void R_World_NewDrawSurface(msurface_t* surf, qboolean lightmapped)
{
	int			tex_diffuse, tex_lightmap;
	int			numTris, numIndices, i;
	unsigned int* dst_indices;
	qboolean	stylechanged;

	R_World_GrabSurfaceTextures(surf, &tex_diffuse, &tex_lightmap);

	// fullbright surfaces (water, translucent) have no styles
	stylechanged = R_World_UpdateLightStylesForSurf((r_fullbright->value > 0.0f || !lightmapped) ? NULL : surf);

	numTris = surf->numedges - 2;
	numIndices = numTris * 3;

	//These flags are ones that are relevant to uniforms.
	unsigned int uniformflags = surf->texinfo->flags & (SURF_WARP | SURF_FLOWING | SURF_SCROLLX | SURF_SCROLLY | SURF_SCROLLFLIP);

	//
	// draw everything we have buffered so far and begin building again 
	// when any of the textures change or surface lightstyles change
	//
	if (gfx_world.tex_diffuse != tex_diffuse || gfx_world.tex_lm != tex_lightmap || gfx_world.uniformflags != uniformflags || gfx_world.numIndices + numIndices >= MAX_INDICES  || stylechanged )
	{
		R_World_DrawAndFlushBufferedGeo();
		//Only when starting a new batch should uniforms be updated, since they will persist across all of the batch's draws
		
		if (surf->texinfo->flags & SURF_WARP)
			R_ProgUniform1f(LOC_WARPSTRENGTH, 1.f);
		else
			R_ProgUniform1f(LOC_WARPSTRENGTH, 0.f);
		
		float scrollx = 0, scrolly = 0;
		if (surf->texinfo->flags & SURF_FLOWING)
			scrollx = -64;
		else if (surf->texinfo->flags & SURF_SCROLLX)
			scrollx = -32;
		if (surf->texinfo->flags & SURF_SCROLLY)
			scrolly = -32;
		if (surf->texinfo->flags & SURF_SCROLLFLIP)
		{
			scrollx = -scrollx; scrolly = -scrolly;
		}

		R_ProgUniform2f(LOC_FLOWSTRENGTH, scrollx, scrolly);
	}

	if (stylechanged)
	{
		for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
			VectorCopy(gfx_world.new_styles[i], gfx_world.styles[i]);
	}


	dst_indices = gfx_world.indicesBuf + gfx_world.numIndices;
	for (i = 0; i < numTris; i++) // q2pro code
	{
		dst_indices[0] = surf->firstvert;
		dst_indices[1] = surf->firstvert + (i + 1);
		dst_indices[2] = surf->firstvert + (i + 2);
		dst_indices += 3;
	}

	rperf.brush_tris += numTris;
	rperf.brush_polys++;

	gfx_world.numIndices += numIndices;
	gfx_world.tex_diffuse = tex_diffuse;
	gfx_world.tex_lm = tex_lightmap;
	gfx_world.uniformflags = uniformflags;
}

/*
=================
R_World_LightInlineModel

calculate dynamic lighting for brushmodel, adopts Spike's fix from QuakeSpasm
note: assumes pCurrentRefEnt & pCurrentModel are properly set
=================
*/
void R_World_LightInlineModel()
{
	dlight_t	*light;
	vec3_t		lightorg;
	int			i;

//	if (!pCurrentModel || pCurrentModel->type != MOD_BRUSH)
//		return; // this can never happen

	light = r_newrefdef.dlights;
	for (i = 0; i < r_newrefdef.num_dlights; i++, light++)
	{
		VectorSubtract(light->origin, pCurrentRefEnt->origin, lightorg);
		R_MarkLights(light, lightorg, (1 << i), (pCurrentModel->nodes + pCurrentModel->firstnode));
	}
}

/*
=================
R_DrawInlineBModel_NEW

This is a copy of R_DrawInlineBModel adapted for batching
=================
*/
void R_DrawInlineBModel_NEW()
{
	int			i;
	cplane_t	*pplane;
	msurface_t* psurf;
	vec3_t		color;
	float		dot, alpha = 1.0f;

	// setup RF_COLOR and RF_TRANSLUCENT
	if (pCurrentRefEnt->renderfx & RF_COLOR)
		VectorCopy(pCurrentRefEnt->renderColor, color);
	else
		VectorSet(color, 1.0f, 1.0f, 1.0f);

	if ((pCurrentRefEnt->renderfx & RF_TRANSLUCENT) && pCurrentRefEnt->alpha != 1.0f)
	{
		if (pCurrentRefEnt->alpha <= 0.0f)
			return; // completly gone

		R_Blend(true);
		alpha = pCurrentRefEnt->alpha;
	}

	// light bmodel
	R_World_LightInlineModel();

	//
	// draw brushmodel
	//
	R_World_BeginRendering();
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], alpha);

	psurf = &pCurrentModel->surfaces[pCurrentModel->firstmodelsurface];
	for (i = 0; i < pCurrentModel->nummodelsurfaces; i++, psurf++)
	{
		// find which side of the node we are on
		pplane = psurf->plane;
		dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
			{
				// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else
				R_World_NewDrawSurface(psurf, true);
		}
	}
	R_World_DrawAndFlushBufferedGeo();

	if (alpha != 1.0f)
		R_Blend(false);

	R_World_EndRendering();
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
	// surfaces that aren't transparent or warped.
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
				R_World_NewDrawSurface(s, true);
				// draw all lightmapped surfs
			}
		}
	}
	R_World_DrawAndFlushBufferedGeo();

	//
	// non-transparent warp surfaces. 
	// [ISB] To avoid a context change by changing shaders, and the overhead of updating uniforms,
	// the world shader always performs warps but the strength is zeroed to cancel it out. 
	//
	R_ProgUniform1f(LOC_WARPSTRENGTH, 1.f);
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
				R_World_NewDrawSurface(s, false);
			}
		}

		if (!r_showtris->value) // texture chains are used in r_showtris
			image->texturechain = NULL;
	}
	R_World_DrawAndFlushBufferedGeo();
	R_ProgUniform1f(LOC_WARPSTRENGTH, 0.f);
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

	// we are already at world matrix
	//glLoadMatrixf(r_world_matrix);

	R_World_BeginRendering();
	R_Blend(true);

	for (surf = r_alpha_surfaces; surf; surf = surf->texturechain)
	{
		//All surfaces can go through the same pipe now.
		R_World_NewDrawSurface(surf, false);
	}
	R_World_DrawAndFlushBufferedGeo();

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
	// we already have texturechains

	// should move this to Mod_LoadFaces or Mod_LoadBSP
	if (registration_sequence != gfx_world.registration_sequence)
	{
		R_DestroyWorldVertexBuffer();
		gfx_world.registration_sequence = registration_sequence;
	}

	if (!gfx_world.vbo) 
	{
		R_BuildVertexBufferForWorld();
	}


	R_DrawWorld_TextureChains_NEW();

	R_World_EndRendering(); // to make sure we're out of rendering world
}
