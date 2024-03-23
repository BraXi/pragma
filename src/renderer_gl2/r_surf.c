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
void DrawGLPoly (poly_t *p)
{
	int		i;
	polyvert_t	*v;

	glBegin (GL_POLYGON);
	v = &p->verts[0];
	for (i = 0; i < p->numverts; i++, v++)
	{
		glTexCoord2f (v->texCoord[0], v->texCoord[1]);
		glVertex3fv (v->pos);
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
	polyvert_t *v;
	poly_t *p;
	float	scroll;

	p = fa->polys;

	scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
	if(scroll == 0.0)
		scroll = -64.0;

	glBegin (GL_POLYGON);
	v = &p->verts[0];

	for (i = 0; i < p->numverts; i++, v++)
	{
		glTexCoord2f ((v->texCoord[0] + scroll), v->texCoord[1]);
		glVertex3fv (v->pos);
	}
	glEnd ();
}


void R_DrawTriangleOutlines (void)
{

}

/*
** DrawGLPolyChain
*/
void DrawGLPolyChain( poly_t *p, float soffset, float toffset )
{
	polyvert_t* v;
	int j;
	for (; p != 0; p = p->chain)
	{
		glBegin(GL_POLYGON);
		v = &p->verts[0];
		for (j = 0; j < p->numverts; j++, v++)
		{
			glTexCoord2f(v->lmTexCoord[0] - soffset, v->lmTexCoord[1] - toffset);
			glVertex3fv(v->pos);
		}
		glEnd();
	}
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa)
{
	image_t		*image;

	rperf.brush_polys++;

	image = R_TextureAnimation (fa->texinfo);

	R_MultiTextureBind(TMU_DIFFUSE, image->texnum);
	R_MultiTextureBind(TMU_LIGHTMAP_0, r_texture_white->texnum);

	if (fa->flags & SURF_DRAWTURB)
	{
		EmitWaterPolys(fa);
		return;
	}

	if(fa->texinfo->flags & SURF_FLOWING)
		DrawGLFlowingPoly (fa);
	else
		DrawGLPoly (fa->polys);
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

	R_MultiTextureBind(TMU_LIGHTMAP_0, r_texture_white->texnum); // no lightmap
	R_Blend(true);

	R_BindProgram(GLPROG_WORLD);
	for (s=r_alpha_surfaces ; s ; s=s->texturechain)
	{
		R_MultiTextureBind(TMU_DIFFUSE, s->texinfo->image->texnum);
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
	polyvert_t		*v;
	float		scroll;
	poly_t	*poly;

	numVerts = surf->polys->numverts;
	poly = surf->polys;

	scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
	if (scroll == 0.0)
		scroll = -64.0;

	glBegin(GL_POLYGON);

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
}

/*
================
DrawLightMappedGenericSurf
================
*/
inline static void DrawLightMappedGenericSurf(msurface_t* surf)
{
	int			i, numVerts;
	polyvert_t		*v;
	poly_t	*poly;

	numVerts = surf->polys->numverts;
	poly = surf->polys;

	glBegin(GL_POLYGON);
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
		R_MultiTextureBind(TMU_DIFFUSE, colorMapTexId);
		R_MultiTextureBind(TMU_LIGHTMAP_0, r_texture_white->texnum);
	}
	else
	{
		R_MultiTextureBind(TMU_DIFFUSE, (r_lightmap->value > 0.0f) ? r_texture_white->texnum : colorMapTexId);
		R_MultiTextureBind(TMU_LIGHTMAP_0, (r_fullbright->value > 0.0f) ? r_texture_white->texnum : lightMapTexId);
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
	image_t* image;
	
	image = R_TextureAnimation(surf->texinfo);
	if (!image)
		image = r_texture_missing;

	if ((surf->flags & SURF_DRAWTURB))
	{
		R_MultiTextureBind(TMU_DIFFUSE, image->texnum);
		R_MultiTextureBind(TMU_LIGHTMAP_0, r_texture_white->texnum);
		EmitWaterPolys(surf);
		return;
	}

	rperf.brush_polys++;
	DrawLightMappedSurf(surf, image->texnum, gl_state.lightmap_textures + surf->lightMapTextureNum);
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
				R_RenderBrushPoly( psurf );
			}
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
	gl_state.current_texture[0] = gl_state.current_texture[1] = -1;

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

	gl_state.current_texture[0] = gl_state.current_texture[1] = -1;

	R_ClearSkyBox ();

	if (r_singlepass->value)
	{
		// draw world in a single pass
		R_RecursiveWorldNode(r_worldmodel->nodes);

		DrawTextureChains();
		//R_MultiTextureBind(GL_TEXTURE1, r_texture_white->texnum);
	}
	else
	{
		R_MultiTextureBind(TMU_LIGHTMAP_0, r_texture_white->texnum);
		R_RecursiveWorldNode(r_worldmodel->nodes);	
		DrawTextureChains();
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

