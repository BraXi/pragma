/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_surf.c: surface-related refresh code

#include <assert.h>
#include "r_local.h"

static vec3_t	modelorg;		// relative to viewpoint
static msurface_t	*r_alpha_surfaces = NULL;
static byte fatvis[MAX_MAP_LEAFS_QBSP / 8]; // markleaves

static image_t* R_World_TextureAnimation(mtexinfo_t* tex);
static void R_World_DrawSurface(msurface_t* surf);

extern void R_LightMap_TexCoordsForSurf(msurface_t* surf, polyvert_t* vert, vec3_t pos);
extern qboolean R_LightMap_UpdateLightStylesForSurf(msurface_t* surf);
extern void R_BeginLinesRendering(qboolean dt);
extern void R_EndLinesRendering();

extern cvar_t* r_test;

extern int registration_sequence;
static int seq;
static qboolean world_vbo_ready = false;

#define MAX_INDICES 16384
typedef struct gfx_world_s
{
	int			tex_diffuse;
	int			tex_lm;

	vec3_t		styles[MAX_LIGHTMAPS_PER_SURFACE];
	vec3_t		new_styles[MAX_LIGHTMAPS_PER_SURFACE];

	unsigned int vbo;

	int numIndices;
	GLuint indicesBuf[MAX_INDICES];

	polyvert_t* staticVerts;
	int numStaticVerts;

} gfx_world_t;

gfx_world_t gfx_world;

float	r_turbsin[] =
{
	#include "warpsin.h"
};
#define TURBSCALE (256.0 / (2 * M_PI))

void R_DestroyWorldVertexBuffer()
{
	if (!gfx_world.vbo)
		return;

	glDeleteBuffers(1, &gfx_world.vbo);	

	gfx_world.vbo = 0;
	world_vbo_ready = false;
}

/*
===============
R_BuildWorldVertexBuffer
===============
*/
void R_BuildWorldVertexBuffer()
{
	msurface_t	*surf;
	poly_t		*poly;
	int			i, j, v, bufsize;
	GLint		ongpusize = 0;

	if (world_vbo_ready)
		return;

	gfx_world.numStaticVerts = 0;
	for (surf = r_worldmodel->surfaces, i = 0; i < r_worldmodel->numsurfaces; i++, surf++)
	{
		// count all verts to build temp buf, set vert indexes for surfs
		surf->firstvert = gfx_world.numStaticVerts;
		surf->numverts = 0;

		poly = surf->polys;
		while (poly != NULL) // account for warps
		{
			surf->numverts += poly->numverts;
			poly = poly->chain;
		}
		gfx_world.numStaticVerts += surf->numverts;

//		ri.Printf(PRINT_ALL, "%i verts in %i surf\n", surf->numverts, i);
	}

	bufsize = sizeof(polyvert_t) * gfx_world.numStaticVerts;

	gfx_world.staticVerts = malloc(bufsize);
	if(!gfx_world.staticVerts)
		ri.Error(ERR_FATAL, "failed to allocate %i kb for gfx_world.staticVerts", bufsize/1024);

	v = 0;
	for (surf = r_worldmodel->surfaces, i = 0; i < r_worldmodel->numsurfaces; i++, surf++)
	{
		// copy verts to temp buffer
		poly = surf->polys;
		while (poly != NULL)
		{
			for (j = 0; j < poly->numverts; j++)
			{
				memcpy(&gfx_world.staticVerts[v], &poly->verts[j], sizeof(polyvert_t));
				//Vector4Set(gfx_world.staticVerts[v].styles, 1, 1, 1, 1);
				v++;
			}
			poly = poly->chain;
		}

	}

	glGenBuffers(1, &gfx_world.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glBufferData(GL_ARRAY_BUFFER, bufsize, &gfx_world.staticVerts[0].pos[0], GL_STATIC_DRAW);

	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &ongpusize);
	if (ongpusize != bufsize)
	{
		R_DestroyWorldVertexBuffer();
		ri.Error(ERR_FATAL, "failed to allocate vertex buffer for gfx_world (probably out of VRAM)\n");
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	ri.Printf(PRINT_ALL, "World surface cache is %i kb (%i surfaces, %i verts)\n", bufsize / 1024, r_worldmodel->numsurfaces, gfx_world.numStaticVerts);

	free(gfx_world.staticVerts);
	gfx_world.staticVerts = NULL;

	world_vbo_ready = true;	
}

/*
===============
R_World_BeginRendering
===============
*/
static void R_World_BeginRendering()
{
	int attrib;

	if (registration_sequence != seq)
	{
		R_DestroyWorldVertexBuffer();
		seq = registration_sequence;
		world_vbo_ready = false;
	}

	if (!world_vbo_ready)
	{
		R_BuildWorldVertexBuffer(); // once
	}

	R_BindProgram(GLPROG_WORLD);

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

	attrib = R_GetProgAttribLoc(VALOC_LMCOORD);
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

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	R_UnbindProgram();
}

/*
=======================
R_World_UpdateLightStylesForSurf

Update lightstyles for surface, pass NULL to disable lightstyles
=======================
*/
qboolean R_World_UpdateLightStylesForSurf(msurface_t* surf)
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

	if (!r_dynamic->value)
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
			//lm_colors[i][3] = 1.0f;
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
	if (!gfx_world.numIndices || !world_vbo_ready)
		return;

	rperf.brush_drawcalls++;

	R_MultiTextureBind(TMU_DIFFUSE, gfx_world.tex_diffuse);
	R_MultiTextureBind(TMU_LIGHTMAP, gfx_world.tex_lm);

	R_ProgUniform3fv(LOC_LIGHTSTYLES, MAX_LIGHTMAPS_PER_SURFACE, gfx_world.styles[0]);

	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glDrawElements(GL_TRIANGLES, gfx_world.numIndices, GL_UNSIGNED_INT, &gfx_world.indicesBuf[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	gfx_world.numIndices = 0;
}

/*
=======================
R_World_NewDrawSurface
=======================
*/
static __inline void R_World_NewDrawSurface(const msurface_t* surf, qboolean lightmapped)
{
	image_t		*image;
	int			tex_diffuse, tex_lightmap;
	int			numTris, numIndices, i;
	unsigned int* dst_indices;
	qboolean stylechanged;

	//
	// update textures and styles
	//
	image = R_World_TextureAnimation(surf->texinfo);
	if (!image)
		image = r_texture_missing;

	tex_diffuse = r_lightmap->value ? r_texture_white->texnum : image->texnum;
	tex_lightmap = (r_fullbright->value || !lightmapped) ? r_texture_white->texnum : (gl_state.lightmap_textures + surf->lightMapTextureNum);

	if (tex_diffuse == r_texture_white->texnum && tex_lightmap == r_texture_white->texnum)
		tex_diffuse = image->texnum; // never let the world render completly white

	// fullbright surfaces (water, translucent) have no styles
	stylechanged = R_World_UpdateLightStylesForSurf((r_fullbright->value > 0.0f || !lightmapped) ? NULL : surf);

	numTris = surf->numedges - 2;
	numIndices = numTris * 3;

	//
	// draw everything we have buffered so far and begin building again 
	// when any of the textures change or surface lightstyle changes
	//
	if (gfx_world.tex_diffuse != tex_diffuse || gfx_world.tex_lm != tex_lightmap || gfx_world.numIndices + numIndices >= MAX_INDICES || stylechanged)
	{
		World_DrawAndFlushBufferedGeo();
	}

	if (stylechanged)
	{
		for (i = 0; i < MAX_LIGHTMAPS_PER_SURFACE; i++)
			VectorCopy(gfx_world.new_styles[i], gfx_world.styles[i]);
	}

	dst_indices = gfx_world.indicesBuf + gfx_world.numIndices;
	for (i = 0; i < numTris; i++)
	{
		dst_indices[0] = surf->firstvert;
		dst_indices[1] = surf->firstvert + (i + 1);
		dst_indices[2] = surf->firstvert + (i + 2);
		dst_indices += 3;
	}

	rperf.brush_tris += numTris;
	rperf.brush_polys ++;

	gfx_world.numIndices += numIndices;
	gfx_world.tex_diffuse = tex_diffuse;
	gfx_world.tex_lm = tex_lightmap;
}

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_World_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
static image_t *R_World_TextureAnimation(mtexinfo_t *tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = pCurrentRefEnt->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}


/*
=============
R_World_DrawUnlitWaterSurf

Does a water warp on the pre-fragmented glpoly_t chain, also handles unlit flowing geometry
=============
*/
void R_World_DrawUnlitWaterSurf(msurface_t* surf)
{
#if 1
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glDrawArrays(GL_TRIANGLE_FAN, surf->firstvert, surf->numverts);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	rperf.brush_drawcalls++;
#else

	poly_t* p, * bp;
	polyvert_t* v;
	int			i;
	float		s, t, os, ot;
	float		scroll;
	float		rdt = r_newrefdef.time;

	if (surf->texinfo->flags & SURF_FLOWING)
		scroll = -64 * ((r_newrefdef.time * 0.5) - (int)(r_newrefdef.time * 0.5));
	else
		scroll = 0;

	for (bp = surf->polys; bp; bp = bp->next)
	{
		p = bp;

		glBegin(GL_TRIANGLE_FAN);
		for (i = 0, v = &p->verts[0]; i < p->numverts; i++, v++)
		{
			os = v->texCoord[0];
			ot = v->texCoord[1];

			s = os + r_turbsin[(int)((ot * 0.125 + r_newrefdef.time) * TURBSCALE) & 255];
			s += scroll;
			s *= (1.0 / 64);

			t = ot + r_turbsin[(int)((os * 0.125 + rdt) * TURBSCALE) & 255];
			t *= (1.0 / 64);

			glTexCoord2f(s, t);
			glVertex3fv(v->pos);
		}
		glEnd();
	}
#endif
}

/*
================
R_World_DrawUnlitGeom

draws unlit geometry
================
*/
void R_World_DrawUnlitGeom(msurface_t* surf)
{
#if 1
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glDrawArrays(GL_TRIANGLE_FAN, surf->firstvert, surf->numverts);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	rperf.brush_drawcalls++;
#else
	int		i;
	polyvert_t	*v;

	R_World_NewDrawSurface(surf, false);
	return;

	v = &surf->polys->verts[0];

	glBegin (GL_TRIANGLE_FAN);
	for (i = 0; i < surf->polys->numverts; i++, v++)
	{
		glTexCoord2f (v->texCoord[0], v->texCoord[1]);
		glVertex3fv (v->pos);
	}
	glEnd ();
#endif
}

static void R_DrawTriangleOutlines()
{
	int		i, j;
	msurface_t* surf;
	poly_t* p;
	image_t* image;

	if (!r_showtris->value)
		return;

	R_WriteToDepthBuffer(false);
	R_BeginLinesRendering(r_showtris->value >= 2 ? true : false);

	if (r_showtris->value > 1)
	{
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1, -1);
		glLineWidth(2);
	}

	R_BindProgram(GLPROG_DEBUGLINE);
	R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, 1.0f);

	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		surf = image->texturechain;
		if (!surf)
			continue;

		for (; surf; surf = surf->texturechain)
		{
			if (r_showtris->value > 2)
			{
				if (surf->flags & SURF_DRAWTURB)
					R_ProgUniform4f(LOC_COLOR4, 0.0f, 0.2f, 1.0f, 1.0f);
				else if (surf->flags & SURF_UNDERWATER)
					R_ProgUniform4f(LOC_COLOR4, 0.0f, 1.0f, 0.0f, 1.0f);
				else
					R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, 1.0f);
			}

			p = surf->polys;
			for (; p; p = p->chain)
			{
				for (j = 2; j < p->numverts; j++)
				{
					glBegin(GL_LINE_STRIP);
					glVertex3fv(p->verts[0].pos);
					glVertex3fv(p->verts[j - 1].pos);
					glVertex3fv(p->verts[j].pos);
					glVertex3fv(p->verts[0].pos);
					glEnd();
				}
			}
		}
		image->texturechain = NULL;
	}

	// draw transparent
	R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 0.0f, 1.0f);
	for (surf = r_alpha_surfaces; surf; surf = surf->texturechain)
	{
		p = surf->polys;
		for (; p; p = p->chain)
		{
			for (j = 2; j < p->numverts; j++)
			{
				glBegin(GL_LINE_STRIP);
				glVertex3fv(p->verts[0].pos);
				glVertex3fv(p->verts[j - 1].pos);
				glVertex3fv(p->verts[j].pos);
				glVertex3fv(p->verts[0].pos);
				glEnd();
			}
		}
	}

	R_UnbindProgram();

	if (r_showtris->value > 1)
	{
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(0.0f, 0.0f);
	}

	R_EndLinesRendering();
	R_WriteToDepthBuffer(true);
}


/*
================
R_World_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_World_DrawAlphaSurfaces()
{
	msurface_t	*surf;
	float		oldalpha = -1.0f, alpha;

	// go back to the world matrix
    glLoadMatrixf (r_world_matrix);

	R_BindProgram(GLPROG_WORLD);

	// transparent surfaces have no lightmap and no lightstyles
	R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);
	R_LightMap_UpdateLightStylesForSurf(NULL); 

	R_Blend(true);

	for (surf = r_alpha_surfaces; surf ; surf=surf->texturechain)
	{
		R_MultiTextureBind(TMU_DIFFUSE, surf->texinfo->image->texnum);
		rperf.brush_polys++;

		if (surf->texinfo->flags & SURF_TRANS33)
			alpha = 0.33f;
		else if (surf->texinfo->flags & SURF_TRANS66)
			alpha = 0.66f;
		else
			alpha = 1.0f;

		if (alpha != oldalpha)
		{
			R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, alpha);
			oldalpha = alpha;
		}

		if (surf->flags & SURF_DRAWTURB)
			R_World_DrawUnlitWaterSurf(surf);
		else
			R_World_DrawUnlitGeom(surf);
	}
	R_Blend(false);
	R_UnbindProgram();

	r_alpha_surfaces = NULL;
}
/*
================
R_DrawWorld_New
================
*/
static void R_DrawWorld_New()
{
	int		i;
	msurface_t* surf;
	image_t* image;


	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence || !image->texturechain)
			continue;

		for (surf = image->texturechain; surf; surf = surf->texturechain)
		{
			R_World_UpdateLightStylesForSurf(surf);
		}
	}
	
	rperf.visible_textures = 0;
	// lightmapped surfaces
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence || !image->texturechain)
			continue;

		rperf.visible_textures++;

		for (surf = image->texturechain; surf; surf = surf->texturechain)
		{
			if (!(surf->flags & SURF_DRAWTURB))
				R_World_NewDrawSurface(surf, true);
		}
	}
	World_DrawAndFlushBufferedGeo();
	
	// unlit and/or fullbright surfaces
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		surf = image->texturechain;
		if (!surf)
			continue;

		for (; surf; surf = surf->texturechain)
		{
			if (surf->flags & SURF_DRAWTURB)
				R_World_NewDrawSurface(surf, false);
		}

		if (!r_showtris->value)
			image->texturechain = NULL;
	}

	World_DrawAndFlushBufferedGeo();
}


/*
================
R_DrawWorld_TextureChains

Draws world geometry sorted by diffuse map textures
================
*/
static void R_DrawWorld_TextureChains()
{
	int		i;
	msurface_t	*s;
	image_t		*image;

	rperf.visible_textures = 0;

	// lightmapped surfaces
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence || !image->texturechain)
			continue;

		rperf.visible_textures++;

		for ( s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
				R_World_DrawSurface(s);
		}
	}

	// unlit and/or fullbright surfaces
	for ( i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		s = image->texturechain;
		if (!s)
			continue;

		for ( ; s; s = s->texturechain)
		{	
			if ( s->flags & SURF_DRAWTURB )
				R_World_DrawSurface(s);  
		}

		if(!r_showtris->value)
			image->texturechain = NULL;
	}
}

/*
================
R_World_DrawFlowingSurfLM
================
*/
inline static void R_World_DrawFlowingSurfLM(msurface_t* surf)
{
#if 1
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glDrawArrays(GL_TRIANGLE_FAN, surf->firstvert, surf->numverts);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	rperf.brush_drawcalls++;
#else

	int			i, numVerts;
	polyvert_t* v;
	float		scroll;
	poly_t* poly;

	numVerts = surf->polys->numverts;
	poly = surf->polys;

	scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
	if (scroll == 0.0)
		scroll = -64.0;

	glBegin(GL_TRIANGLE_FAN);

	while (poly != NULL)
	{
		v = &poly->verts[0];
		for (i = 0; i < numVerts /*poly->numverts*/; i++, v++)
		{
			glMultiTexCoord2f(GL_TEXTURE0, (v->texCoord[0] + scroll), v->texCoord[1]);
			glMultiTexCoord2f(GL_TEXTURE1, v->lmTexCoord[0], v->lmTexCoord[1]);
			glVertex3fv(v->pos);
		}
		poly = poly->chain;
	}
	glEnd();
#endif
}

/*
================
R_World_DrawGenericSurfLM
================
*/
inline static void R_World_DrawGenericSurfLM(msurface_t* surf)
{
#if 1
	glBindBuffer(GL_ARRAY_BUFFER, gfx_world.vbo);
	glDrawArrays(GL_TRIANGLE_FAN, surf->firstvert, surf->numverts);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	rperf.brush_drawcalls++;

#else

	int			i, numVerts;
	polyvert_t* v;
	poly_t* poly;

	numVerts = surf->polys->numverts;
	poly = surf->polys;

	glBegin(GL_TRIANGLE_FAN);
	while(poly != NULL)
	{
		v = &poly->verts[0];
		for (i = 0; i < poly->numverts; i++, v++)
		{
			glMultiTexCoord2f(GL_TEXTURE0, v->texCoord[0], v->texCoord[1]);
			glMultiTexCoord2f(GL_TEXTURE1, v->lmTexCoord[0], v->lmTexCoord[1]);
			glVertex3fv(v->pos);
		}
		poly = poly->chain;
	}
	glEnd();
#endif
}


/*
================
R_World_DrawSurface
================
*/
static void R_World_DrawSurface( msurface_t *surf )
{
	image_t* image;
	int tex_diffuse, tex_lightmap;
	
	image = R_World_TextureAnimation(surf->texinfo);
	if (!image)
		image = r_texture_missing;


	if ((surf->flags & SURF_DRAWTURB))
	{
		R_LightMap_UpdateLightStylesForSurf(NULL); // disable lightstyles
		R_MultiTextureBind(TMU_DIFFUSE, image->texnum);
		R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);
		R_World_DrawUnlitWaterSurf(surf);
		return;
	}

	if (r_lightmap->value && r_fullbright->value)
	{
		// never let the world to be rendered completly white
		tex_diffuse = image->texnum;
		tex_lightmap = r_texture_white->texnum;
	}
	else
	{
		tex_diffuse = r_lightmap->value ? r_texture_white->texnum : image->texnum;
		tex_lightmap = r_fullbright->value ? r_texture_white->texnum : (gl_state.lightmap_textures + surf->lightMapTextureNum);
	}

	R_LightMap_UpdateLightStylesForSurf(r_fullbright->value ? NULL : surf); // disable lightstyles when fullbright

	R_MultiTextureBind(TMU_DIFFUSE, tex_diffuse);
	R_MultiTextureBind(TMU_LIGHTMAP, tex_lightmap);

	if (surf->texinfo->flags & SURF_FLOWING)
		R_World_DrawFlowingSurfLM(surf);
	else
		R_World_DrawGenericSurfLM(surf);

	rperf.brush_polys++;
}

/*
=================
R_DrawInlineBModel
=================
*/

void R_DrawInlineBModel (void)
{
	int			i, k;
	cplane_t	*pplane;
	float		dot;
	msurface_t	*psurf;
	dlight_t	*light;
	vec3_t		lightorg;

	// calculate dynamic lighting for bmodel
	light = r_newrefdef.dlights;
	for (k = 0; k < r_newrefdef.num_dlights; k++, light++)
	{
		 // Spike'surf fix from QS
		VectorSubtract(light->origin, pCurrentRefEnt->origin, lightorg);
		R_MarkLights(light, lightorg, (1 << k), (pCurrentModel->nodes + pCurrentModel->firstnode));
	}

	psurf = &pCurrentModel->surfaces[pCurrentModel->firstmodelsurface];

	R_BindProgram(GLPROG_WORLD);

	if ( pCurrentRefEnt->renderfx & RF_TRANSLUCENT )
	{
		R_Blend(true);
		R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, pCurrentRefEnt->alpha);
	}

	//
	// draw texture
	//
	for (i=0 ; i<pCurrentModel->nummodelsurfaces ; i++, psurf++)
	{
	// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

	// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) || (!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66) )
			{	
				// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}

			R_World_DrawSurface(psurf);
		}
	}

	if ( (pCurrentRefEnt->renderfx & RF_TRANSLUCENT) )
	{
		R_Blend(false);
		R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1.0);
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (rentity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (pCurrentModel->nummodelsurfaces == 0)
		return;

	pCurrentRefEnt = e;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - pCurrentModel->radius;
			maxs[i] = e->origin[i] + pCurrentModel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, pCurrentModel->mins, mins);
		VectorAdd (e->origin, pCurrentModel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs))
		return;

	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);

	VectorSubtract (r_newrefdef.view.origin, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

    glPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug
	R_RotateForEntity (e);
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug

	R_DrawInlineBModel ();
	R_SelectTextureUnit(0);

	glPopMatrix ();
}


/*
===============
R_BuildPolygonFromSurface

Does also calculate proper lightmap coordinates for poly
===============
*/
void R_BuildPolygonFromSurface(model_t* mod, msurface_t* surf)
{
	int i, lnumverts;
	medge_t* pedges, * r_pedge;
	float* vec;
	poly_t* poly;
	vec3_t total;
	vec3_t normal;

	// reconstruct the polygon
	pedges = mod->edges;
	lnumverts = surf->numedges;

	VectorClear(total);

	/* draw texture */
	poly = Hunk_Alloc(sizeof(poly_t) + (lnumverts - 4) * sizeof(polyvert_t));
	poly->next = surf->polys;
	poly->flags = surf->flags;
	surf->polys = poly;
	poly->numverts = lnumverts;

	VectorCopy(surf->plane->normal, normal);

	if (surf->flags & SURF_PLANEBACK)
	{
		// if for some reason the normal sticks to the back of 
		// the plane, invert it so it'surf usable for the shader
		for (i = 0; i < 3; ++i)
			normal[i] = -normal[i];
	}

	for (i = 0; i < lnumverts; i++)
	{
		polyvert_t* vert;
		float s, t;
		int lindex;

		vert = &poly->verts[i];

		lindex = mod->surfedges[surf->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = mod->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = mod->vertexes[r_pedge->v[1]].position;
		}

		//
		// diffuse texture coordinates
		//
		s = DotProduct(vec, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
		s /= surf->texinfo->image->width;

		t = DotProduct(vec, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3];
		t /= surf->texinfo->image->height;

		VectorAdd(total, vec, total);
		VectorCopy(vec, vert->pos);
		Vector2Set(vert->texCoord, s, t);

		VectorCopy(normal, vert->normal);

		R_LightMap_TexCoordsForSurf(surf, vert, vec);

		vert->lightFlags = 0;
	}
}


/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_World_RecursiveNode

Builts two chains for solid and transparent geometry which we later use 
to draw world, discards nodes which are not in PVS or frustum
================
*/
void R_World_RecursiveNode(mnode_t *node)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;
	image_t		*image;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (R_CullBox (node->mins, node->maxs))
		return;
	
	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
				return;	// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		return;
	}

	//
	// node is just a decision point, so go down the apropriate sides
	//
	
	// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

// recurse down the children, front side first
	R_World_RecursiveNode (node->children[side]);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{	
			//
			// add to visible sky bounds
			//
			R_AddSkySurface (surf);
		}
		else if (surf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66))
		{	
			//
			// add surface to the translucent chain
			//
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			//
			// add surface to the texture chain
			//
			// FIXME: this is a hack for animation
			image = R_World_TextureAnimation (surf->texinfo);
			surf->texturechain = image->texturechain;
			image->texturechain = surf;
		}
	}

	// recurse down the back side
	R_World_RecursiveNode (node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld()
{
	rentity_t	ent;

	if (!r_drawworld->value)
		return;

	if ( r_newrefdef.view.flags & RDF_NOWORLDMODEL )
		return;

	pCurrentModel = r_worldmodel;

	VectorCopy (r_newrefdef.view.origin, modelorg);

	// auto cycle the world frame for texture animation
	memset (&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time*2);

	pCurrentRefEnt = &ent;

	R_ClearSkyBox ();

	//if (r_test->value)
	{
		R_World_BeginRendering();
		gfx_world.numIndices = 0;
	}

	// build texture chains
	R_World_RecursiveNode(r_worldmodel->nodes);

	R_BindProgram(GLPROG_WORLD);
	{
		R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);

		if (r_test->value)
			R_DrawWorld_New();
		else
			R_DrawWorld_TextureChains(); // draw opaque surfaces now, we draw translucent surfs later
	}
	R_UnbindProgram();

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	R_DrawSkyBox ();

	R_DrawTriangleOutlines ();
}


/*
===============
R_World_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current cluster
Note: a camera may be in two PVS areas hence there ate two clusters
===============
*/
void R_World_MarkLeaves()
{
	byte	*vis;
	mnode_t	*node;
	int		i, c;
	mleaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where the pvs ends
	if (r_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_BSP_ClusterPVS (r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy (fatvis, vis, (r_worldmodel->numleafs+7)/8);
		vis = Mod_BSP_ClusterPVS (r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}
	
	for (i=0,leaf=r_worldmodel->leafs ; i<r_worldmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}

#if 0
	for (i=0 ; i<r_worldmodel->vis->XX_numclusters ; i++)
	{
		if (vis[i>>3] & (1<<(i&7)))
		{
			node = (mnode_t *)&r_worldmodel->leafs[i];	// FIXME: cluster
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
#endif
}

