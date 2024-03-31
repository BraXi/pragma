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

vec3_t		modelorg;		// relative to viewpoint
msurface_t	*r_alpha_surfaces = NULL;
static byte fatvis[MAX_MAP_LEAFS_QBSP / 8]; // markleaves

extern void R_LightMap_TexCoordsForSurf(msurface_t* surf, polyvert_t* vert, vec3_t pos);
extern void R_LightMap_UpdateLightStylesForSurf(msurface_t* surf);
static void R_World_DrawSurface(msurface_t* surf);
extern void R_BeginLinesRendering(qboolean dt);
extern void R_EndLinesRendering();

extern cvar_t* r_fastworld;

extern void R_DrawInlineBModel_NEW();
extern void R_DrawWorld_TextureChains_NEW();
extern void R_World_DrawAlphaSurfaces_NEW();
extern void R_DrawWorld_NEW();

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
image_t *R_World_TextureAnimation(mtexinfo_t *tex)
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
R_World_DrawUnlitGeom

draws unlit geometry
================
*/
void R_World_DrawUnlitGeom(msurface_t* surf)
{
	int		i;
	polyvert_t	*v;

	v = &surf->polys->verts[0];

	glBegin (GL_TRIANGLE_FAN);
	for (i = 0; i < surf->polys->numverts; i++, v++)
	{
		glTexCoord2f (v->texCoord[0], v->texCoord[1]);
		glVertex3fv (v->pos);
	}
	glEnd ();
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

	if (r_fastworld->value)
	{
		R_World_DrawAlphaSurfaces_NEW();
		return;
	}

	//
	// go back to the world matrix
	//
    glLoadMatrixf (r_world_matrix);

	R_BindProgram(GLPROG_WORLD);
	R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum); // no lightmap

	R_LightMap_UpdateLightStylesForSurf(NULL); // hack for trans

	R_Blend(true);
	for (surf=r_alpha_surfaces ; surf ; surf=surf->texturechain)
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
R_DrawWorld_TextureChains

Draws world geometry sorted by diffuse map textures
================
*/
static void R_DrawWorld_TextureChains()
{
	int		i;
	msurface_t	*s;
	image_t		*image;


	if (r_fastworld->value)
	{
		R_DrawWorld_TextureChains_NEW();
		return;
	}

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
	int			i, numVerts;
	polyvert_t		*v;
	float		scroll;
	poly_t	*poly;

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
}

/*
================
R_World_DrawGenericSurfLM
================
*/
inline static void R_World_DrawGenericSurfLM(msurface_t* surf)
{
	int			i, numVerts;
	polyvert_t		*v;
	poly_t	*poly;

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
	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);

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

	if (r_fastworld->value)
	{
		R_DrawInlineBModel_NEW();
	}
	else
	{
		R_DrawInlineBModel();
		R_SelectTextureUnit(0);
	}

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

	// build texture chains
	R_World_RecursiveNode(r_worldmodel->nodes);

	if (r_fastworld->value)
	{
		R_DrawWorld_NEW();
	}
	else
	{
		R_BindProgram(GLPROG_WORLD);
		{
			R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);
			R_DrawWorld_TextureChains(); // draw opaque surfaces now, we draw translucent surfs later
		}
		R_UnbindProgram();
	}

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

