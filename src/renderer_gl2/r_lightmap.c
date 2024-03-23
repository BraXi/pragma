/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_lightmap.c - code loosely from yamagi's gl3 renderer
#include "r_local.h"

#define LM_BYTES 4

#define	LM_BLOCK_WIDTH		256
#define	LM_BLOCK_HEIGHT		256
#define	LM_MAX_LIGHTMAPS	128

#define GL_LIGHTMAP_FORMAT GL_RGBA

typedef struct
{
	int current_lightmap_texture; // index into gl3state.lightmap_textureIDs[]

	int allocated[LM_BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in main memory so texsubimage can update properly
	byte lightmap_buffers[MAX_LIGHTMAPS_PER_SURFACE][4 * LM_BLOCK_WIDTH * LM_BLOCK_HEIGHT];
} newlightmapstate_t;


void _SelectTmu(int tmu) {} // stub
void _BindLightMap(int tmu) {} // stub

#define TEXNUM_LIGHTMAPS 1024

newlightmapstate_t lm_state;

/*
===============
NewLM_SurfHasLightMap
===============
*/
qboolean NewLM_SurfaceHasLightMap(msurface_t *surf)
{
	if (surf->flags & (SURF_DRAWSKY | SURF_DRAWTURB))
	{
		return false;
	}
	return true;
}


/*
===============
NewLM_InitBlock
===============
*/
void NewLM_InitBlock()
{
	memset(lm_state.allocated, 0, sizeof(lm_state.allocated));
}

/*
===============
NewLM_UploadBlock
===============
*/
void NewLM_UploadBlock()
{
	int map;

	// NOTE: we don't use the dynamic lightmap anymore - all lightmaps are loaded at level load
	//       and not changed after that. they're blended dynamically depending on light styles
	//       though, and dynamic lights are (will be) applied in shader, hopefully per fragment.

	_BindLightMap(lm_state.current_lightmap_texture);

	// upload all 4 lightmaps
	for (map = 0; map < MAX_LIGHTMAPS_PER_SURFACE; ++map)
	{
		_SelectTmu(GL_TEXTURE1 + map); // this relies on GL_TEXTURE2 being GL_TEXTURE1+1 etc

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_LIGHTMAP_FORMAT, LM_BLOCK_WIDTH, LM_BLOCK_HEIGHT, 0, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE, lm_state.lightmap_buffers[map]);
	}

	if (++lm_state.current_lightmap_texture == LM_MAX_LIGHTMAPS)
	{
		ri.Error(ERR_DROP, "%s: LM_MAX_LIGHTMAPS exceeded\n", __func__);
	}
}


/*
===============
NewLM_AllocBlock
returns a texture number and the position inside it
===============
*/
qboolean NewLM_AllocBlock(int w, int h, int* x, int* y)
{
	int i, best;

	best = LM_BLOCK_HEIGHT;

	for (i = 0; i < LM_BLOCK_WIDTH - w; i++)
	{
		int		j, best2;

		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (lm_state.allocated[i + j] >= best)
			{
				break;
			}

			if (lm_state.allocated[i + j] > best2)
			{
				best2 = lm_state.allocated[i + j];
			}
		}

		if (j == w)
		{
			/* this is a valid spot */
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > LM_BLOCK_HEIGHT)
	{
		return false;
	}

	for (i = 0; i < w; i++)
	{
		lm_state.allocated[*x + i] = best + h;
	}

	return true;
}

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
		s = DotProduct(vec, surf->lmvecs[0]) + surf->lmvecs[0][3];
#else
		s = DotProduct(vec, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
#endif
		s -= surf->texturemins[0];
		s += surf->light_s * (1 << surf->lmshift);
		s += (1 << surf->lmshift) * 0.5;
		s /= LM_BLOCK_WIDTH * (1 << surf->lmshift);

#if DECOUPLED_LM
		t = DotProduct(vec, surf->lmvecs[1]) + surf->lmvecs[1][3];
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
===============
NewLM_BuildPolygonFromSurface

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/

void NewLM_BuildLightMap(msurface_t* surf, int offsetInLMbuf, int stride)
{
	int smax, tmax;
	int r, g, b, a, max;
	int i, j, size, map, nummaps;
	byte* lightmap;

	static const MAX_LM_SIZE = (LM_BLOCK_WIDTH * LM_BLOCK_HEIGHT * 3);


	if (surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP))
	{
		ri.Error(ERR_DROP, "%s called for non-lit surface", __func__);
	}

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;
	size = smax * tmax;

	stride -= (smax << 2);

	if (size > MAX_LM_SIZE)
	{
		ri.Error(ERR_DROP, "%s: Bad lightmap size (%i", __func__, MAX_LM_SIZE);
	}

	// count number of lightmaps surf actually has
	for (nummaps = 0; nummaps < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[nummaps] != 255; )
	{
		nummaps++;
	}

	if (!surf->samples)
	{
		// no lightmap samples? set at least one lightmap to fullbright, rest to 0 as normal

		if (nummaps == 0)
			nummaps = 1; // make sure at least one lightmap is set to fullbright

		for (map = 0; map < MAX_LIGHTMAPS_PER_SURFACE; map++)
		{
			// we always create 4 (MAX_LIGHTMAPS_PER_SURFACE) lightmaps.
			// if surf has less (nummaps < 4), the remaining ones are zeroed out.
			// this makes sure that all 4 lightmap textures in gl3state.lightmap_textureIDs[i] have the same layout
			// and the shader can use the same texture coordinates for all of them

			int c = (map < nummaps) ? 255 : 0;

			byte* dest = lm_state.lightmap_buffers[map] + offsetInLMbuf;

			// build full white [rgba 1,1,1,1] lightmap
			for (i = 0; i < tmax; i++, dest += stride)
			{
				memset(dest, c, 4 * smax);
				dest += 4 * smax;
			}
		}

		return;
	}

	//
	// add all the lightmaps
	//

	// Note: dynamic lights aren't handled here anymore, they're handled in the shader

	// as we don't apply scale here anymore, nor blend the nummaps lightmaps together,
	// the code has gotten a lot easier and we can copy directly from surf->samples to dest
	// without converting to float first etc

	lightmap = surf->samples;

	for (map = 0; map < nummaps; ++map)
	{
		byte* dest = lm_state.lightmap_buffers[map] + offsetInLMbuf;
		int idxInLightmap = 0;
		for (i = 0; i < tmax; i++, dest += stride)
		{
			for (j = 0; j < smax; j++)
			{
				r = lightmap[idxInLightmap * 3 + 0];
				g = lightmap[idxInLightmap * 3 + 1];
				b = lightmap[idxInLightmap * 3 + 2];

				// determine the brightest of the three color components 
				if (r > g)
					max = r;
				else
					max = g;

				if (b > max)
					max = b;

				// alpha is ONLY used for the mono lightmap case. For this reason we set it 
				// to the brightest of the color components so that things don't get too dim.
				a = max;

				dest[0] = r;
				dest[1] = g;
				dest[2] = b;
				dest[3] = a;

				dest += 4;
				idxInLightmap ++;
			}
		}

		lightmap += size * 3; // skip to the next lightmap
	}

	for (; map < MAX_LIGHTMAPS_PER_SURFACE; map++)
	{
		// like above, fill up remaining lightmaps with 0

		byte* dest = lm_state.lightmap_buffers[map] + offsetInLMbuf;

		for (i = 0; i < tmax; i++, dest += stride)
		{
			memset(dest, 0, 4 * smax);
			dest += 4 * smax;
		}
	}
}

/*
===============
NewLM_CreateSurfaceLightmap
===============
*/
void NewLM_CreateSurfaceLightmap(msurface_t* surf)
{
	int smax, tmax;

	if (NewLM_SurfaceHasLightMap(surf) == false)
		return;

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;

	if (!NewLM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
	{
		NewLM_UploadBlock();
		NewLM_InitBlock();

		if (!NewLM_AllocBlock(smax, tmax, &surf->light_s, &surf->light_t))
		{
			ri.Error(ERR_FATAL, "%s: Consecutive calls to LM_AllocBlock(%d,%d) failed\n", __func__, smax, tmax);
		}
	}

	surf->lightmaptexturenum = lm_state.current_lightmap_texture;

	NewLM_BuildLightMap(surf, (surf->light_t * LM_BLOCK_WIDTH + surf->light_s) * LM_BYTES, (LM_BLOCK_WIDTH * LM_BYTES));
}