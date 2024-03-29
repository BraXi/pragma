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

msurface_t	*r_alpha_surfaces;

#define DYNAMIC_LIGHT_WIDTH  256
#define DYNAMIC_LIGHT_HEIGHT 256

#define LIGHTMAP_BYTES 4

#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256
#define	MAX_LIGHTMAPS	128

#define GL_LIGHTMAP_FORMAT GL_RGBA

typedef struct
{
	int internal_format;
	int	current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;


static void		R_LightMap_InitBlock( void );
static void		R_LightMap_UploadBlock( qboolean dynamic );
static qboolean	R_LightMap_AllocBlock (int w, int h, int *x, int *y);

extern void R_LightMap_SetCacheStateForSurf( msurface_t *surf );
extern void R_LightMap_Build (msurface_t *surf, byte *dest, int stride);

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *R_TextureAnimation (mtexinfo_t *tex)
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
================
DrawGLPoly
================
*/
void DrawGLPoly (glpoly_t *p)
{
	int		i;
	float	*v;

	glBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f (v[3], v[4]);
		glVertex3fv (v);
	}
	glEnd ();
}

/*
================
DrawGLFlowingPoly -- version of DrawGLPoly that handles scrolling texture
================
*/
void DrawGLFlowingPoly (msurface_t *fa)
{
	int		i;
	float	*v;
	glpoly_t *p;
	float	scroll;

	p = fa->polys;

	scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
	if(scroll == 0.0)
		scroll = -64.0;

	glBegin (GL_POLYGON);
	v = p->verts[0];
	for (i=0 ; i<p->numverts ; i++, v+= VERTEXSIZE)
	{
		glTexCoord2f ((v[3] + scroll), v[4]);
		glVertex3fv (v);
	}
	glEnd ();
}

/*
** R_DrawTriangleOutlines
*/
extern void R_BeginLinesRendering(qboolean dt);
extern void R_EndLinesRendering();
void R_DrawTriangleOutlines (void)
{
	int			i, j;
	glpoly_t	*p;

	if (!r_showtris->value)
		return;
;
	R_WriteToDepthBuffer(true);
	R_BeginLinesRendering(r_showtris->value >= 2 ? true : false);
	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		msurface_t *surf;

		for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
		{
			p = surf->polys;
			for ( ; p ; p=p->chain)
			{
				for (j = 2; j < p->numverts; j++)
				{
					glBegin (GL_LINE_STRIP);
					glVertex3fv (p->verts[0]);
					glVertex3fv (p->verts[j-1]);
					glVertex3fv (p->verts[j]);
					glVertex3fv (p->verts[0]);
					glEnd ();
				}
			}
		}
	}
	R_EndLinesRendering();
	R_WriteToDepthBuffer(true);
}

/*
** DrawGLPolyChain
*/
void DrawGLPolyChain( glpoly_t *p, float soffset, float toffset )
{
	if ( soffset == 0 && toffset == 0 )
	{
		for ( ; p != 0; p = p->chain )
		{
			float *v;
			int j;

			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[5], v[6] );
				glVertex3fv (v);
			}
			glEnd ();
		}
	}
	else
	{
		for ( ; p != 0; p = p->chain )
		{
			float *v;
			int j;

			glBegin (GL_POLYGON);
			v = p->verts[0];
			for (j=0 ; j<p->numverts ; j++, v+= VERTEXSIZE)
			{
				glTexCoord2f (v[5] - soffset, v[6] - toffset );
				glVertex3fv (v);
			}
			glEnd ();
		}
	}
}

/*
** R_BlendLightMaps
**
** This routine takes all the given light mapped surfaces in the world and
** blends them into the framebuffer.
*/
void R_BlendLightmaps (void)
{
	int			i;
	msurface_t	*surf, *newdrawsurf = 0;


	// don't bother if we're set to fullbright
	if (r_fullbright->value)
		return;

	if (!r_worldmodel->lightdata)
		return;

	// don't bother writing Z
	R_WriteToDepthBuffer(GL_FALSE);

	/*
	** set the appropriate blending mode unless we're only looking at the lightmaps.
	*/
	if (!r_lightmap->value)
	{
		R_Blend(true);

		if ( r_saturatelighting->value )
		{
			R_BlendFunc( GL_ONE, GL_ONE );
		}
		else
		{
			if ( r_monolightmap->string[0] != '0' )
			{
				switch ( toupper( r_monolightmap->string[0] ) )
				{
				case 'I':
					R_BlendFunc(GL_ZERO, GL_SRC_COLOR );
					break;
				case 'L':
					R_BlendFunc(GL_ZERO, GL_SRC_COLOR );
					break;
				case 'A':
				default:
					R_BlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
					break;
				}
			}
			else
			{
				R_BlendFunc(GL_ZERO, GL_SRC_COLOR );
			}
		}
	}

	if ( pCurrentModel == r_worldmodel )
		rperf.visible_lightmaps = 0;

	/*
	** render static lightmaps first
	*/
	for ( i = 1; i < MAX_LIGHTMAPS; i++ )
	{
		if ( gl_lms.lightmap_surfaces[i] )
		{
			if (pCurrentModel == r_worldmodel)
				rperf.visible_lightmaps++;

			R_BindTexture( gl_state.lightmap_textures + i);

			for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
			{
				if (surf->polys)
				{
					DrawGLPolyChain(surf->polys, 0, 0);
				}
			}
		}
	}

	/*
	** render dynamic lightmaps
	*/
	if ( r_dynamic->value )
	{
		R_LightMap_InitBlock();

		R_BindTexture( gl_state.lightmap_textures+0 );

		if (pCurrentModel == r_worldmodel)
			rperf.visible_lightmaps++;

		newdrawsurf = gl_lms.lightmap_surfaces[0];

		for ( surf = gl_lms.lightmap_surfaces[0]; surf != 0; surf = surf->lightmapchain )
		{
			int		smax, tmax;
			byte	*base;

			smax = (surf->extents[0] >> surf->lmshift) + 1;
			tmax = (surf->extents[1] >> surf->lmshift) + 1;
			//smax = (surf->extents[0]>>4)+1;
			//tmax = (surf->extents[1]>>4)+1;

			if ( R_LightMap_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
			{
				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_LightMap_Build (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
			else
			{
				msurface_t *drawsurf;

				// upload what we have so far
				R_LightMap_UploadBlock( true );

				// draw all surfaces that use this lightmap
				for ( drawsurf = newdrawsurf; drawsurf != surf; drawsurf = drawsurf->lightmapchain )
				{
					if (drawsurf->polys)
					{
						DrawGLPolyChain(drawsurf->polys,
							(drawsurf->light_s - drawsurf->dlight_s) * (1.0 / BLOCK_WIDTH),
							(drawsurf->light_t - drawsurf->dlight_t) * (1.0 / BLOCK_HEIGHT)); //LMCHANGE

						//DrawGLPolyChain(drawsurf->polys,
						//	(drawsurf->light_s - drawsurf->dlight_s)* (1.0 / 128.0),
						//	(drawsurf->light_t - drawsurf->dlight_t)* (1.0 / 128.0));
					}
				}

				newdrawsurf = drawsurf;

				// clear the block
				R_LightMap_InitBlock();

				// try uploading the block now
				if ( !R_LightMap_AllocBlock( smax, tmax, &surf->dlight_s, &surf->dlight_t ) )
				{
					ri.Error( ERR_FATAL, "Consecutive calls to R_LightMap_AllocBlock(%d,%d) failed (dynamic)\n", smax, tmax );
				}

				base = gl_lms.lightmap_buffer;
				base += ( surf->dlight_t * BLOCK_WIDTH + surf->dlight_s ) * LIGHTMAP_BYTES;

				R_LightMap_Build (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
			}
		}

		/*
		** draw remainder of dynamic lightmaps that haven't been uploaded yet
		*/
		if ( newdrawsurf )
			R_LightMap_UploadBlock( true );

		for ( surf = newdrawsurf; surf != 0; surf = surf->lightmapchain )
		{
			if ( surf->polys )
				DrawGLPolyChain( surf->polys, ( surf->light_s - surf->dlight_s ) * ( 1.0 / BLOCK_WIDTH ), ( surf->light_t - surf->dlight_t ) * ( 1.0 / BLOCK_HEIGHT ) ); //LMCHANGE
			//DrawGLPolyChain(surf->polys, (surf->light_s - surf->dlight_s) * (1.0 / 128.0), (surf->light_t - surf->dlight_t) * (1.0 / 128.0));
		}
	}

	/*
	** restore state
	*/
	R_Blend(false);
	R_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	R_WriteToDepthBuffer(GL_TRUE);
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa)
{
	int			maps;
	image_t		*image;
	qboolean is_dynamic = false;

	rperf.brush_polys++;

	image = R_TextureAnimation (fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		R_MultiTextureBind(GL_TEXTURE0, image->texnum);
		EmitWaterPolys(fa);
		return;
	}
	else
	{
		R_MultiTextureBind(GL_TEXTURE0, image->texnum);
	}


	if(fa->texinfo->flags & SURF_FLOWING)
		DrawGLFlowingPoly (fa);
	else
		DrawGLPoly (fa->polys);

	/*
	** check for lightmap modification
	*/
	for ( maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++ )
	{
		if ( r_newrefdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps] )
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ( ( fa->dlightframe == r_framecount ) )
	{
dynamic:
		if ( r_dynamic->value )
		{
			if (!( fa->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP ) ) )
			{
				is_dynamic = true;
			}
		}
	}

	if ( is_dynamic )
	{
		if ( ( fa->styles[maps] >= 32 || fa->styles[maps] == 0 ) && ( fa->dlightframe != r_framecount ) )
		{
			unsigned	temp[256*256]; //BLOCKLIGHT_DIMS
			int			smax, tmax;
			
			smax = (fa->extents[0] >> fa->lmshift) + 1;
			tmax = (fa->extents[1] >> fa->lmshift) + 1;

			R_LightMap_Build( fa, (void *)temp, smax*4 );
			R_LightMap_SetCacheStateForSurf( fa );

			R_MultiTextureBind(GL_TEXTURE1, gl_state.lightmap_textures + fa->lightmaptexturenum );

			glTexSubImage2D( GL_TEXTURE_2D, 0,
							  fa->light_s, fa->light_t, 
							  smax, tmax, 
							  GL_LIGHTMAP_FORMAT, 
							  GL_UNSIGNED_BYTE, temp );

			fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
			gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = gl_lms.lightmap_surfaces[0];
			gl_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = gl_lms.lightmap_surfaces[fa->lightmaptexturenum];
		gl_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}


/*
================
R_DrawWorldAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawWorldAlphaSurfaces()
{
	msurface_t	*s;
	float		alpha;

	//
	// go back to the world matrix
	//
    glLoadMatrixf (r_world_matrix);

	R_EnableMultitexture(true);
	R_MultiTextureBind(GL_TEXTURE1, r_texture_white->texnum); // no lightmap
	R_Blend(true);

	R_BindProgram(GLPROG_WORLD);
	for (s=r_alpha_surfaces ; s ; s=s->texturechain)
	{
		R_MultiTextureBind(GL_TEXTURE0, s->texinfo->image->texnum);
		rperf.brush_polys++;

		if (s->texinfo->flags & SURF_TRANS33)
			alpha = 0.33f;
		else if (s->texinfo->flags & SURF_TRANS66)
			alpha = 0.66f;
		else
			alpha = 1.0f;

		R_ProgUniform4f(LOC_COLOR4, 1.0f, 1.0f, 1.0f, alpha);

		if (s->flags & SURF_DRAWTURB)
			EmitWaterPolys (s);
		else
			DrawGLPoly (s->polys);
	}

	R_UnbindProgram();

	R_EnableMultitexture(false);
	R_Blend(false);

	r_alpha_surfaces = NULL;
}

/*
================
DrawTextureChains
================
*/
static void R_LightMappedWorldSurf(msurface_t* surf);
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	image_t		*image;

//	R_MultiTextureBind(GL_TEXTURE1, r_texture_white->texnum);
//	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);

	rperf.visible_textures = 0;
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence || !image->texturechain)
			continue;

		rperf.visible_textures++;

		for ( s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
				R_LightMappedWorldSurf(s);
				//R_RenderBrushPoly(s); // lightmapped
		}
	}

	for ( i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		s = image->texturechain;
		if (!s)
			continue;

		for ( ; s ; s = s->texturechain)
		{
			if ( s->flags & SURF_DRAWTURB )
				R_LightMappedWorldSurf(s);
				//R_RenderBrushPoly(s); // unlit and fullbright
		}
		image->texturechain = NULL;
	}
}

/*
================
DrawLightMappedFlowingSurf
================
*/
inline static void DrawLightMappedFlowingSurf(msurface_t* surf)
{
	int			i, numVerts;
	float		*v, scroll;
	glpoly_t	*poly;

	numVerts = surf->polys->numverts;
	poly = surf->polys;

	scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
	if (scroll == 0.0)
		scroll = -64.0;

	glBegin(GL_POLYGON);
	while (poly != NULL)
	{
		v = poly->verts[0];
		for (i = 0; i < numVerts /*poly->numverts*/; i++, v += VERTEXSIZE)
		{
			glMultiTexCoord2f(GL_TEXTURE0, (v[3] + scroll), v[4]);
			glMultiTexCoord2f(GL_TEXTURE1, v[5], v[6]);
			glVertex3fv(v);
		}
		poly = poly->chain;
	}
	glEnd();
}

/*
================
DrawLightMappedGenericSurf
================
*/
inline static void DrawLightMappedGenericSurf(msurface_t* surf)
{
	int			i, numVerts;
	float		*v;
	glpoly_t	*poly;

	numVerts = surf->polys->numverts;
	poly = surf->polys;

	glBegin(GL_POLYGON);
	while(poly != NULL)
	{
		v = poly->verts[0];
		for (i = 0; i < numVerts /*poly->numverts*/; i++, v += VERTEXSIZE)
		{
			glMultiTexCoord2f(GL_TEXTURE0, v[3], v[4]);
			glMultiTexCoord2f(GL_TEXTURE1, v[5], v[6]);
			glVertex3fv(v);
		}
		poly = poly->chain;
	}
	glEnd();
}

/*
================
DrawLightMappedSurf
================
*/
inline static void DrawLightMappedSurf(msurface_t* surf, int colorMapTexId, int lightMapTexId)
{
	// not ideal, but not the worst
	if (r_lightmap->value && r_fullbright->value)
	{
		R_MultiTextureBind(GL_TEXTURE0, colorMapTexId);
		R_MultiTextureBind(GL_TEXTURE1, r_texture_white->texnum);
	}
	else
	{
		R_MultiTextureBind(GL_TEXTURE0, (r_lightmap->value > 0.0f) ? r_texture_white->texnum : colorMapTexId);
		R_MultiTextureBind(GL_TEXTURE1, (r_fullbright->value > 0.0f) ? r_texture_white->texnum : lightMapTexId);
	}

	if (surf->texinfo->flags & SURF_FLOWING)
	{
		DrawLightMappedFlowingSurf(surf);
	}
	else
	{
		DrawLightMappedGenericSurf(surf);
	}
}

/*
================
R_LightMappedWorldSurf
================
*/
static void R_LightMappedWorldSurf( msurface_t *surf )
{
	int		nv = surf->polys->numverts;
	int		map;
	image_t	*image = R_TextureAnimation(surf->texinfo);

	qboolean is_dynamic = false;
	unsigned lightMapTexId = surf->lightmaptexturenum;


	if ((surf->flags & SURF_DRAWTURB))
	{
		if (!image)
			image = r_texture_missing;

		R_MultiTextureBind(GL_TEXTURE0, image->texnum);
		R_MultiTextureBind(GL_TEXTURE1, r_texture_white->texnum);
		EmitWaterPolys(surf);
		return;
	}

	// check if the lightmap has changed
	for ( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++ )
	{
		if ( r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map] )
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ( ( surf->dlightframe == r_framecount ) )
	{
dynamic:
		if ( r_dynamic->value )
		{
			if ( !(surf->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP ) ) )
			{
				is_dynamic = true;
			}
		}
	}

	if ( is_dynamic )
	{
		//unsigned	temp[128*128]; 
		unsigned	temp[BLOCK_WIDTH * BLOCK_HEIGHT]; //LMCHANGE
		int			smax, tmax;

		if ( ( surf->styles[map] >= 32 || surf->styles[map] == 0 ) && ( surf->dlightframe != r_framecount ) )
		{
			smax = (surf->extents[0] >> surf->lmshift) + 1;
			tmax = (surf->extents[1] >> surf->lmshift) + 1;
			//smax = (surf->extents[0]>>4)+1;
			//tmax = (surf->extents[1]>>4)+1;

			R_LightMap_Build( surf, (void *)temp, smax*4 );
			R_LightMap_SetCacheStateForSurf( surf );

			lightMapTexId = surf->lightmaptexturenum;

			R_MultiTextureBind(GL_TEXTURE1, gl_state.lightmap_textures + lightMapTexId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, surf->light_s, surf->light_t, smax, tmax,  GL_LIGHTMAP_FORMAT,  GL_UNSIGNED_BYTE, temp);
		}
		else
		{
			smax = (surf->extents[0] >> surf->lmshift) + 1;
			tmax = (surf->extents[1] >> surf->lmshift) + 1;
			//smax = (surf->extents[0]>>4)+1;
			//tmax = (surf->extents[1]>>4)+1;

			R_LightMap_Build( surf, (void *)temp, smax*4 );

			lightMapTexId = 0;
			R_MultiTextureBind(GL_TEXTURE1, gl_state.lightmap_textures + lightMapTexId);
			glTexSubImage2D(GL_TEXTURE_2D, 0, surf->light_s, surf->light_t, smax, tmax, GL_LIGHTMAP_FORMAT, GL_UNSIGNED_BYTE, temp );
		}

		rperf.brush_polys++;
		DrawLightMappedSurf(surf, image->texnum, gl_state.lightmap_textures+lightMapTexId);
	}
	else
	{
		rperf.brush_polys++;
		DrawLightMappedSurf(surf, image->texnum, gl_state.lightmap_textures + lightMapTexId);
	}
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
		 // Spike's fix from QS
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

			if (r_singlepass->value)
			{
				R_LightMappedWorldSurf(psurf);
			}
			else
			{
				//R_EnableMultitexture( false );
				R_RenderBrushPoly( psurf );
				//R_EnableMultitexture( true );
			}
		}
	}

	if ( !(pCurrentRefEnt->renderfx & RF_TRANSLUCENT) )
	{
		if (!r_singlepass->value || !glMultiTexCoord2f )
			R_BlendLightmaps ();
	}
	else
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
	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

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

	memset (gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

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

	R_EnableMultitexture( true );
	R_DrawInlineBModel ();
	R_EnableMultitexture( false );

	glPopMatrix ();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node)
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
				return;		// not visible
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

// node is just a decision point, so go down the apropriate sides

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
	R_RecursiveWorldNode (node->children[side]);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{	
			// just adds to visible sky bounds
			R_AddSkySurface (surf);
		}
		else if (surf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66))
		{	
			// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			//if( r_singlepass->value )
			//{
			//	R_LightMappedWorldSurf(surf);
			//}
			//else
			{
				// the polygon is visible, so add it to the texture sorted chain
				// FIXME: this is a hack for animation
				image = R_TextureAnimation (surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
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

	R_BindProgram(GLPROG_WORLD);
	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);

	pCurrentRefEnt = &ent;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;
	memset (gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	R_ClearSkyBox ();

	if (r_singlepass->value)
	{
		// draw world in a single pass

		R_EnableMultitexture(true);
		R_RecursiveWorldNode(r_worldmodel->nodes);

		DrawTextureChains();
		//R_MultiTextureBind(GL_TEXTURE1, r_texture_white->texnum);

		R_EnableMultitexture(false);
	}
	else
	{
		R_EnableMultitexture(true);
		R_MultiTextureBind(GL_TEXTURE1, r_texture_white->texnum);
		R_RecursiveWorldNode(r_worldmodel->nodes);	
		DrawTextureChains();
		R_EnableMultitexture(false);
		R_BlendLightmaps();
	}

	R_UnbindProgram();

	R_DrawSkyBox ();

	R_DrawTriangleOutlines ();
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current cluster
Note: a camera may be in two PVS areas hence there ate two clusters
===============
*/
void R_MarkLeaves(void)
{
	byte	*vis;
	byte	fatvis[MAX_MAP_LEAFS_QBSP/8];
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
static void R_LightMap_InitBlock( void )
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

/*
================
R_LightMap_UploadBlock
================
*/
static void R_LightMap_UploadBlock( qboolean dynamic )
{
	int texture;
	int height = 0;

	if ( dynamic )
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	R_BindTexture( gl_state.lightmap_textures + texture );
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ( dynamic )
	{
		int i;

		for ( i = 0; i < BLOCK_WIDTH; i++ )
		{
			if ( gl_lms.allocated[i] > height )
				height = gl_lms.allocated[i];
		}

		glTexSubImage2D( GL_TEXTURE_2D, 
						  0,
						  0, 0,
						  BLOCK_WIDTH, height,
						  GL_LIGHTMAP_FORMAT,
						  GL_UNSIGNED_BYTE,
						  gl_lms.lightmap_buffer );
	}
	else
	{
		glTexImage2D( GL_TEXTURE_2D, 
					   0, 
					   gl_lms.internal_format,
					   BLOCK_WIDTH, BLOCK_HEIGHT, 
					   0, 
					   GL_LIGHTMAP_FORMAT, 
					   GL_UNSIGNED_BYTE, 
					   gl_lms.lightmap_buffer );
		if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
			ri.Error( ERR_DROP, "R_LightMap_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
	}
}

/*
================
R_LightMap_AllocBlock

returns a texture number and the position inside it
================
*/
static qboolean R_LightMap_AllocBlock(int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for (i=0 ; i<BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

/*
================
R_BuildPolygonFromSurface

Does also calculate proper lightmap coordinates for poly
================
*/
void R_BuildPolygonFromSurface(msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	int			vertpage;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;
	vec3_t		total;

	// reconstruct the polygon
	pedges = pCurrentModel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	VectorClear (total);
	//
	// draw texture
	//
	poly = Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i = 0; i < lnumverts; i++)
	{
		lindex = pCurrentModel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = pCurrentModel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = pCurrentModel->vertexes[r_pedge->v[1]].position;
		}

		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorAdd (total, vec, total);
		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
#if DECOUPLED_LM
		s = DotProduct(vec, fa->lmvecs[0]) + fa->lmvecs[0][3];
#else
		s = DotProduct(vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
#endif
		s -= fa->texturemins[0];
		s += fa->light_s * (1 << fa->lmshift);
		s += (1 << fa->lmshift) * 0.5;
		s /= BLOCK_WIDTH * (1 << fa->lmshift);

#if DECOUPLED_LM
		t = DotProduct(vec, fa->lmvecs[1]) + fa->lmvecs[1][3];
#else
		t = DotProduct(vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
#endif
		
		t -= fa->texturemins[1];
		t += fa->light_t * (1 << fa->lmshift);
		t += (1 << fa->lmshift) * 0.5;
		t /= BLOCK_HEIGHT * (1 << fa->lmshift);

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;

}

/*
========================
R_CreateLightMapForSurface
========================
*/
void R_CreateLightMapForSurface(msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0] >> surf->lmshift) + 1;
	tmax = (surf->extents[1] >> surf->lmshift) + 1;

	if ( !R_LightMap_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		R_LightMap_UploadBlock( false );
		R_LightMap_InitBlock();
		if ( !R_LightMap_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			ri.Error( ERR_FATAL, "Consecutive calls to R_LightMap_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_LightMap_SetCacheStateForSurf( surf );
	R_LightMap_Build(surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}


/*
==================
R_LightMap_BeginBuilding


Note: This does set current texture mapping unit to 1, we keep all lightmaps at unit 1
==================
*/
void R_LightMap_BeginBuilding(model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;
	unsigned		dummy[BLOCK_WIDTH*BLOCK_HEIGHT]; //LMCHANGE

	memset( gl_lms.allocated, 0, sizeof(gl_lms.allocated) );

	r_framecount = 1; // no dlightcache

	R_EnableMultitexture( true );
	R_SelectTextureUnit( GL_TEXTURE1 );

	//
	// setup the base lightstyles so the lightmaps won't have to be regenerated
	// the first time they're seen
	//
	for (i = 0; i < MAX_LIGHTSTYLES ; i++)
	{
		VectorSet(lightstyles[i].rgb, 1.0f, 1.0f, 1.0f);
		lightstyles[i].white = 3; // braxi -- why is is this out of scale? shouldn't it be 2 at max (aka lightstyle string "Z")?
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
	{
		// this is the first texnum for lightmap being created
		gl_state.lightmap_textures = TEXNUM_LIGHTMAPS; 
	}

	gl_lms.current_lightmap_texture = 1;

	/*
	** if mono lightmaps are enabled and we want to use alpha
	** blending (a,1-a) then we're likely running on a 3DLabs
	** Permedia2.  In a perfect world we'd use a GL_ALPHA lightmap
	** in order to conserve space and maximize bandwidth, however 
	** this isn't a perfect world.
	**
	** So we have to use alpha lightmaps, but stored in GL_RGBA format,
	** which means we only get 1/16th the color resolution we should when
	** using alpha lightmaps.  If we find another board that supports
	** only alpha lightmaps but that can at least support the GL_ALPHA
	** format then we should change this code to use real alpha maps.
	*/
	if ( toupper( r_monolightmap->string[0] ) == 'A' )
	{
		gl_lms.internal_format = gl_tex_alpha_format;
	}
	/*
	** try to do hacked colored lighting with a blended texture
	*/
	else if ( toupper( r_monolightmap->string[0] ) == 'C' )
	{
		gl_lms.internal_format = gl_tex_alpha_format;
	}
	else if ( toupper( r_monolightmap->string[0] ) == 'I' )
	{
		gl_lms.internal_format = GL_INTENSITY8;
	}
	else if ( toupper( r_monolightmap->string[0] ) == 'L' ) 
	{
		gl_lms.internal_format = GL_LUMINANCE8;
	}
	else
	{
		gl_lms.internal_format = gl_tex_solid_format;
	}

	//
	// initialize the dynamic lightmap texture
	//
	R_BindTexture( gl_state.lightmap_textures + 0 );

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D( GL_TEXTURE_2D, 
				   0, 
				   gl_lms.internal_format,
				   BLOCK_WIDTH, BLOCK_HEIGHT, 
				   0, 
				   GL_LIGHTMAP_FORMAT, 
				   GL_UNSIGNED_BYTE, 
				   dummy );
}

/*
=======================
R_LightMap_EndBuilding
=======================
*/
void R_LightMap_EndBuilding()
{
	R_LightMap_UploadBlock( false );
	R_EnableMultitexture( false );
}

