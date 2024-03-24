/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_lightmap.c - code loosely from yamagi's gl3 renderer
#include "r_local.h"

#define	LM_BLOCK_WIDTH	256
#define	LM_BLOCK_HEIGHT	256
#define	MAX_LIGHTMAPS	128
#define	TEXNUM_LIGHTMAPS	1024
#define LM_BYTES 4

#define LIGHTMAP_GL_FORMAT GL_RGBA

typedef struct
{
	int	current_lightmap_texture;

	int			allocated[LM_BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in main memory so texsubimage can update properly
	byte		lightmap_buffers[MAX_LIGHTMAPS_PER_SURFACE][LM_BYTES * LM_BLOCK_WIDTH * LM_BLOCK_HEIGHT];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;


#define DoubleDotProduct(x,y) (long double)((long double)x[0]*(long double)y[0]+(long double)x[1]*(long double)y[1]+(long double)x[2]*(long double)y[2])
/*
===============
NewLM_BuildPolygonFromSurface

Does also calculate proper lightmap coordinates for poly
===============
*/
void NewLM_BuildPolygonFromSurface(model_t* mod, msurface_t* surf)
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
		// the plane, invert it so it's usable for the shader
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

		//
		// lightmap texture coordinates
		//
#if DECOUPLED_LM
		s = DoubleDotProduct(vec, surf->lmvecs[0]) + (long double)surf->lmvecs[0][3];
#else
		s = DotProduct(vec, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
#endif
		s -= surf->texturemins[0];
		s += surf->light_s * (1 << surf->lmshift);
		s += (1 << surf->lmshift) * 0.5;
		s /= LM_BLOCK_WIDTH * (1 << surf->lmshift);

#if DECOUPLED_LM
		t = DoubleDotProduct(vec, surf->lmvecs[1]) + (long double)surf->lmvecs[1][3];
#else
		t = DotProduct(vec, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3];
#endif

		t -= surf->texturemins[1];
		t += surf->light_t * (1 << surf->lmshift);
		t += (1 << surf->lmshift) * 0.5;
		t /= LM_BLOCK_HEIGHT * (1 << surf->lmshift);

		Vector2Set(vert->lmTexCoord, s, t);
		VectorCopy(normal, vert->normal);

		vert->lightFlags = 0;
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/


/*
================
R_LightMap_InitBlock
================
*/
static void R_LightMap_InitBlock(void)
{
	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));
}

/*
================
R_LightMap_UploadBlock

Pack 4 light maps into one and upload to GPU
================
*/
static void R_LightMap_UploadBlock()
{
	R_BindTexture(gl_state.lightmap_textures + gl_lms.current_lightmap_texture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, LIGHTMAP_GL_FORMAT, LM_BLOCK_WIDTH*2, LM_BLOCK_HEIGHT*2, 0, LIGHTMAP_GL_FORMAT, GL_UNSIGNED_BYTE, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, LIGHTMAP_GL_FORMAT, GL_UNSIGNED_BYTE, gl_lms.lightmap_buffers[0]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, LM_BLOCK_WIDTH, 0, LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, LIGHTMAP_GL_FORMAT, GL_UNSIGNED_BYTE, gl_lms.lightmap_buffers[1]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, LM_BLOCK_HEIGHT, LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, LIGHTMAP_GL_FORMAT, GL_UNSIGNED_BYTE, gl_lms.lightmap_buffers[2]);
	glTexSubImage2D(GL_TEXTURE_2D, 0, LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, LIGHTMAP_GL_FORMAT, GL_UNSIGNED_BYTE, gl_lms.lightmap_buffers[3]);

	if (++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS)
		ri.Error(ERR_DROP, "%s - MAX_LIGHTMAPS exceeded\n", __FUNCTION__);
}


/*
===============
R_LightMap_BuildLightMaps

Combine and scale multiple lightmaps into the floating format in blocklights

Credits to yamagiquake2
===============
*/

void R_LightMap_BuildLightMaps(msurface_t* surf, int stride, int offsetInLMbuf)
{
	int smax, tmax;
	int r, g, b, a, max;
	int i, j, size, map, nummaps;
	byte* lightmap;

	if (surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
		ri.Error(ERR_DROP, "%s called for non-lit surface", __FUNCTION__);

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;
	size = smax * tmax;

	stride -= (smax << 2);

	if (size > LM_BLOCK_WIDTH * LM_BLOCK_HEIGHT * 3)
	{
		ri.Error(ERR_DROP, "Bad lightmap size");
	}

	// count number of lightmaps surf actually has
	for (nummaps = 0; nummaps < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[nummaps] != 255;)
	{
		nummaps ++;
	}

	if (!surf->samples)
	{
		// no lightmap samples? set at least one lightmap to fullbright, rest to 0 as normal

		if (nummaps == 0)  nummaps = 1; // make sure at least one lightmap is set to fullbright

		for (map = 0; map < MAX_LIGHTMAPS_PER_SURFACE; ++map)
		{
			// we always create 4 (MAX_LIGHTMAPS_PER_SURFACE) lightmaps.
			// if surf has less (nummaps < 4), the remaining ones are zeroed out.
			// this makes sure that all 4 lightmap textures in gl3state.lightmap_textureIDs[i] have the same layout
			// and the shader can use the same texture coordinates for all of them

			int c = (map < nummaps) ? 255 : 0;
			byte* dest = gl_lms.lightmap_buffers[map] + offsetInLMbuf;

			for (i = 0; i < tmax; i++, dest += stride)
			{
				memset(dest, c, 4 * smax);
				dest += 4 * smax;
			}
		}

		return;
	}

	/* add all the lightmaps */

	// Note: dynamic lights aren't handled here anymore, they're handled in the shader

	// as we don't apply scale here anymore, nor blend the nummaps lightmaps together,
	// the code has gotten a lot easier and we can copy directly from surf->samples to dest
	// without converting to float first etc

	lightmap = surf->samples;

	for (map = 0; map < nummaps; ++map)
	{
		byte* dest = gl_lms.lightmap_buffers[map] + offsetInLMbuf;
		int idxInLightmap = 0;
		for (i = 0; i < tmax; i++, dest += stride)
		{
			for (j = 0; j < smax; j++)
			{
				r = lightmap[idxInLightmap * 3 + 0];
				g = lightmap[idxInLightmap * 3 + 1];
				b = lightmap[idxInLightmap * 3 + 2];

				/* determine the brightest of the three color components */
				if (r > g)  max = r;
				else  max = g;

				if (b > max)  max = b;

				/* alpha is ONLY used for the mono lightmap case. For this
				   reason we set it to the brightest of the color components
				   so that things don't get too dim. */
				a = max;

				dest[0] = r;
				dest[1] = g;
				dest[2] = b;
				dest[3] = a;

				dest += 4;
				++idxInLightmap;
			}
		}

		lightmap += size * 3; /* skip to next lightmap */
	}

	for (; map < MAX_LIGHTMAPS_PER_SURFACE; ++map)
	{
		// like above, fill up remaining lightmaps with 0

		byte* dest = gl_lms.lightmap_buffers[map] + offsetInLMbuf;

		for (i = 0; i < tmax; i++, dest += stride)
		{
			memset(dest, 0, 4 * smax);
			dest += 4 * smax;
		}
	}
}
/*
================
R_LightMap_AllocBlock

returns a texture number and the position inside it
================
*/
static qboolean R_LightMap_AllocBlock(int w, int h, int* x, int* y)
{
	int		i, j;
	int		best, best2;

	best = LM_BLOCK_HEIGHT;

	for (i = 0; i < LM_BLOCK_WIDTH - w; i++)
	{
		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (gl_lms.allocated[i + j] >= best)
				break;
			if (gl_lms.allocated[i + j] > best2)
				best2 = gl_lms.allocated[i + j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > LM_BLOCK_HEIGHT)
		return false;

	for (i = 0; i < w; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}


/*
========================
R_CreateLightMapForSurface
========================
*/
void R_CreateLightMapForSurface(msurface_t* surf)
{
	int		smax, tmax;
	int		offset, stride;

	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
		return;

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;

	if (!R_LightMap_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
	{
		R_LightMap_UploadBlock();
		R_LightMap_InitBlock();

		if (!R_LightMap_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
		{
			ri.Error(ERR_FATAL, "Consecutive calls to %s(%d,%d) failed\n", __FUNCTION__, smax, tmax);
		}
	}

	surf->lightMapTextureNum = gl_lms.current_lightmap_texture;

	stride = LM_BLOCK_WIDTH * LM_BYTES;
	offset = (surf->light_t * LM_BLOCK_WIDTH + surf->light_s) * LM_BYTES;

	R_LightMap_BuildLightMaps(surf, stride, offset);
}


/*
==================
R_LightMap_BeginBuilding
==================
*/
void R_LightMap_BeginBuilding(model_t* m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;

	memset(gl_lms.allocated, 0, sizeof(gl_lms.allocated));

	r_framecount = 1; // no dlightcache

	//R_SelectTextureUnit(1);

	//
	// setup the base lightstyles so the lightmaps won't 
	// have to be regenerated the first time they're seen
	//
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		VectorSet(lightstyles[i].rgb, 1.0f, 1.0f, 1.0f);
		lightstyles[i].white = 3; // braxi -- why is is this out of scale? shouldn't it be 2 at max (aka lightstyle string "Z")?
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
		gl_state.lightmap_textures = TEXNUM_LIGHTMAPS; // this is the first texnum for lightmap being created

	gl_lms.current_lightmap_texture = 0;
}

/*
=======================
R_LightMap_EndBuilding
=======================
*/
void R_LightMap_EndBuilding()
{
	R_LightMap_UploadBlock();
}
