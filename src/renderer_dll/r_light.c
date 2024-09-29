/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_light.c - dynamic lights and light sampling from lightmap

#include "r_local.h"

unsigned int r_dlightframecount;

#define	DLIGHT_CUTOFF	64

/*
=================
R_SetEntityAmbientLight

Calculates ambient lighting color and direction for a given entity
by adding effects and probing lightmap color beneath entity.

Takes care of RF_COLOR, RF_FULLBRIGHT, RF_GLOW, RF_MINLIGHT effects.

FIXME: this is uttery shit and deserves a rework !!!

It should calculate ambient lighting by probing for nearby
light sources and find the one thats closest & strongest!
=================
*/
void R_SetEntityAmbientLight(rentity_t* ent)
{
	float	scale;
	float	min;
	float	yaw;
	int		i;

	if ((ent->renderfx & RF_COLOR))
	{
		// entity has RF_COLOR, but fullbright is off
		VectorCopy(ent->renderColor, ent->ambient_color);
	}
	else if (ent->renderfx & RF_FULLBRIGHT || r_fullbright->value)
	{
		// entity is fullbright, but has no RF_COLOR
		VectorSet(ent->ambient_color, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		// entity has no fullbright and RF_COLOR
		R_LightForPoint(ent->origin, ent->ambient_color);
	}

	if (ent->renderfx & RF_GLOW)
	{
		scale = 1.0f * sin(r_newrefdef.time * 7.0);
		for (i = 0; i < 3; i++)
		{
			min = ent->ambient_color[i] * 0.8;
			ent->ambient_color[i] += scale;
			if (ent->ambient_color[i] < min)
				ent->ambient_color[i] = min;
		}
	}

	// RF_MINLIGHT - don't let the model go completly dark
	if (ent->renderfx & RF_MINLIGHT)
	{
		for (i = 0; i < 3; i++)
			if (ent->ambient_color[i] > 0.1)
				break;

		if (i == 3)
		{
			VectorSet(ent->ambient_color, 0.1f, 0.1f, 0.1f);
		}
	}

	// calculate ambient light direction.. this is meeeeh
	yaw = ent->angles[1] / 180 * M_PI;
	ent->ambient_dir[0] = cos(-yaw);
	ent->ambient_dir[1] = sin(-yaw);
	ent->ambient_dir[2] = -1;


	VectorNormalize(ent->ambient_dir);
}

/*
=============
R_MarkLights
=============
*/
void R_MarkLights(dlight_t* light, vec3_t lightorg, int bit, mnode_t* node)
{
	cplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;
	
	if (node->contents != -1)
		return;

	splitplane = node->plane;

	dist = DotProduct(lightorg, splitplane->normal) - splitplane->dist;
	
	if (dist > light->intensity-DLIGHT_CUTOFF)
	{
		R_MarkLights(light, lightorg, bit, node->children[0]);
		return;
	}
	if (dist < -light->intensity+DLIGHT_CUTOFF)
	{
		R_MarkLights(light, lightorg, bit, node->children[1]);	
		return;
	}
		
// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces ; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}
	R_MarkLights(light, lightorg, bit, node->children[0]);
	R_MarkLights(light, lightorg, bit, node->children[1]);
}

/*
=============
R_MarkDynamicLights
=============
*/
void R_MarkDynamicLights()
{
	int			i;
	dlight_t	*l;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't advanced yet for this frame
	
	l = r_newrefdef.dlights;
	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		R_MarkLights(l, l->origin, 1 << i, r_worldmodel->nodes);
	}

	// braxi -- moved dlight shader updates to R_UpdateCommonProgUniforms()
}
/*
=================
R_SendDynamicLightsToCurrentProgram
=================
*/
void R_SendDynamicLightsToCurrentProgram(qboolean bNoViewFlashLight)
{
	dlight_t* dlight;
	int			i, j, ln;
	vec4_t		dl_pos_and_rad[MAX_DLIGHTS]; // holds xyz + radius for each light
	vec4_t		dl_dir_and_cutoff[MAX_DLIGHTS]; // holds xyz direction + spot cutoff for each light
	vec3_t		dl_colors[MAX_DLIGHTS];
	int			numDynLights;

	if (!r_worldmodel || !R_UsingProgram())
		return;

	numDynLights = !r_dynamic->value ? 0 : r_newrefdef.num_dlights; // no dlights when r_dynamic is off

	dlight = r_newrefdef.dlights;
	for (ln = 0, i = 0; i < numDynLights; i++, dlight++)
	{
		if (dlight->type == DL_VIEW_FLASHLIGHT && bNoViewFlashLight)
		{
			int y = i;
			continue;
		}

//		if (dlight->intensity <= DLIGHT_CUTOFF)
//			continue;

		for (j = 0; j < 3; j++)
			dl_pos_and_rad[ln][j] = dlight->origin[j];
		dl_pos_and_rad[ln][3] = dlight->intensity;

		VectorCopy(dlight->color, dl_colors[ln]);

		//Notes about spotlights:
		//xyz must be normalized. (this could be done in shader if really needed)
		//xyz should probably be valid, even if spotlights aren't being used.
		//spot cutoff is in the range -1 (infinitely small cone) to 1 (disable spotlight entirely)

#if 0 // test
		srand(i);
		dlight->type = DL_SPOTLIGHT;
		for (j = 0; j < 3; j++)
		{
			dl_dir_and_cutoff[i][j] = rand() / (float)RAND_MAX * 2 - 1;
			VectorNormalize(dl_dir_and_cutoff[i]);
		}
		float coff = -rand() / (float)RAND_MAX;
		dl_dir_and_cutoff[i][3] = -0.95;
#else
		if (dlight->type == DL_SPOTLIGHT || dlight->type == DL_VIEW_FLASHLIGHT)
		{
			for (j = 0; j < 3; j++)
			{
				dl_dir_and_cutoff[ln][j] = dlight->dir[j];
				//VectorNormalize(dl_dir_and_cutoff[i]);
			}

			dl_dir_and_cutoff[ln][3] = dlight->cutoff;
		}
		else
		{
			//In the absence of spotlights, set these to values that disable spotlights
			dl_dir_and_cutoff[ln][0] = dl_dir_and_cutoff[ln][3] = 1.f;
			dl_dir_and_cutoff[ln][1] = dl_dir_and_cutoff[ln][2] = 0.f;
		}
#endif
		ln++;

	}

	R_ProgUniform1i(LOC_DLIGHT_COUNT, ln);
	if (ln > 0)
	{
		R_ProgUniform3fv(LOC_DLIGHT_COLORS, ln, &dl_colors[0][0]);
		R_ProgUniform4fv(LOC_DLIGHT_POS_AND_RAD, ln, &dl_pos_and_rad[0][0]);
		R_ProgUniform4fv(LOC_DLIGHT_DIR_AND_CUTOFF, ln, &dl_dir_and_cutoff[0][0]);
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

static vec3_t		pointcolor;
static cplane_t		*lightplane;	// used as shadow plane
static vec3_t		lightspot;

static int R_RecursiveLightPoint(mnode_t *node, const vec3_t start, const vec3_t end)
{
	float		front, back, frac;
	int			side, maps, r, i;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	mtexinfo_t	*tex;
	byte		*lightmap;

	if (node->contents != -1)
		return -1; // didn't hit anything
	
	//
	// calculate mid point
	//
	
	// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;
	
	if ( (back < 0) == side)
		return R_RecursiveLightPoint (node->children[side], start, end);
	
	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0]) * frac;
	mid[1] = start[1] + (end[1] - start[1]) * frac;
	mid[2] = start[2] + (end[2] - start[2]) * frac;
	
	//
	// go down front side	
	//
	r = R_RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r; // hit something
		
	if ( (back < 0) == side )
		return -1; // didn't hit anything
		
	//
	// check for impact on this node
	//
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags & (SURF_DRAWTURB|SURF_DRAWSKY)) 
			continue;	// no lightmaps

		tex = surf->texinfo;
		lightmap = surf->samples;

#ifdef DECOUPLED_LM
		s = DotProduct(mid, surf->lmvecs[0]) + surf->lmvecs[0][3];
		t = DotProduct(mid, surf->lmvecs[1]) + surf->lmvecs[1][3];
#else
		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];;
#endif

		if (s < surf->texturemins[0] || t < surf->texturemins[1])
			continue;

//#ifdef DECOUPLED_LM
//		if (s >= surf->lm_width || t >= surf->lm_height)
//			continue; // this code will bust maps with no decoupledlm (in case you compile engine without standard bsp support)
//#endif
		
		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];
		
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if (!surf->samples)
			return 0;

		ds >>= surf->lmshift;
		dt >>= surf->lmshift;
		
		VectorCopy (vec3_origin, pointcolor);
		if (lightmap)
		{
			vec3_t scale;

			lightmap += 3 * (dt * ((surf->extents[0] >> surf->lmshift) + 1) + ds);

			for (maps = 0 ; maps < MAX_LIGHTMAPS_PER_SURFACE && surf->styles[maps] != 255 ; maps++)
			{
				if (r_dynamic->value)
				{
					for (i = 0; i < 3; i++)
						scale[i] = r_ambientlightscale->value * r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

					pointcolor[0] += lightmap[0] * scale[0] * (1.0 / 255);
					pointcolor[1] += lightmap[1] * scale[1] * (1.0 / 255);
					pointcolor[2] += lightmap[2] * scale[2] * (1.0 / 255);
				}
				else
				{
					pointcolor[0] += lightmap[0] * r_ambientlightscale->value * (1.0 / 255);
					pointcolor[1] += lightmap[1] * r_ambientlightscale->value * (1.0 / 255);
					pointcolor[2] += lightmap[2] * r_ambientlightscale->value * (1.0 / 255);
				}

				lightmap += 3 * ((surf->extents[0] >> surf->lmshift) + 1) * ((surf->extents[1] >> surf->lmshift) + 1);
			}
		}	
		return 1;
	}

// go down back side
	return R_RecursiveLightPoint (node->children[!side], mid, end);
}

/*
===============
R_LightForPoint

Returns the lightmap pixel color beneath point
===============
*/
void R_LightForPoint(const vec3_t point, vec3_t outAmbient)
{
	vec3_t		end;
	float		r;
	
	if ( (r_newrefdef.view.flags & RDF_NOWORLDMODEL) || !r_worldmodel->lightdata || r_worldmodel->lightdatasize <= 0)
	{
		VectorSet(outAmbient, 1.0f, 1.0f, 1.0f); // fullbright
		return;
	}
	
	end[0] = point[0];
	end[1] = point[1];
	end[2] = point[2] - 2048.0f; // go this far down

	//
	// find lightmap pixel color underneath p
	//
	r = R_RecursiveLightPoint (r_worldmodel->nodes, point, end);
	
	if (r == -1)
	{
		// nothing was found, return white
		VectorSet(outAmbient, 1.0f, 1.0f, 1.0f); 
		//VectorCopy(vec3_origin, outAmbient);
	}
	else
	{
		VectorCopy(pointcolor, outAmbient);
	}

	// scale the light color with r_ambientlightscale cvar
	VectorScale (outAmbient, r_ambientlightscale->value, outAmbient);
}
