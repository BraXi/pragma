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


static vec_t _DotProduct(vec3_t v1, vec3_t v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

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

#if 0
void R_SetEntityAmbientLight(rentity_t* ent)
{
	float	scale;
	float	min;
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

	// set ambient light dir
	// TODO: read sun settings from world
	VectorSet(ent->ambient_dir, 0, 1, -1);
	VectorNormalize(ent->ambient_dir);
}
#endif

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

	if (!r_world || !R_UsingProgram())
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
=================
R_SetupEntityLightingGrid
=================
*/
static void R_SetupEntityLightingGrid(rentity_t* ent) 
{
	vec3_t	lightOrigin;
	int		pos[3];
	int		i, j;
	byte* gridData;
	float	frac[3];
	int		gridStep[3];
	vec3_t	direction;
	float	totalFactor;

	if (ent->renderfx & RF_LIGHTING_ORIGIN) 
	{
		// seperate lightOrigins are needed so an object that is sinking into the 
		// ground can still be lit, and so multi-part models can be lit identically
		VectorCopy(ent->lightingOrigin, lightOrigin);
	}
	else 
	{
		VectorCopy(ent->origin, lightOrigin);
	}

	VectorSubtract(lightOrigin, r_world->lightGridOrigin, lightOrigin);

	for (i = 0; i < 3; i++) 
	{
		float	v;

		v = lightOrigin[i] * r_world->lightGridInverseSize[i];
		pos[i] = floor(v);
		frac[i] = v - pos[i];

		if (pos[i] < 0) 
		{
			pos[i] = 0;
		}
		else if (pos[i] >= r_world->lightGridBounds[i] - 1) 
		{
			pos[i] = r_world->lightGridBounds[i] - 1;
		}
	}


	VectorClear(ent->ambientLight);
	VectorClear(ent->directedLight);
	VectorClear(direction);

	assert(r_world->lightGridData); // NULL when a BSP doesn't have lightmaps

	// trilerp the light value
	gridStep[0] = 8;
	gridStep[1] = 8 * r_world->lightGridBounds[0];
	gridStep[2] = 8 * r_world->lightGridBounds[0] * r_world->lightGridBounds[1];
	gridData = r_world->lightGridData + pos[0] * gridStep[0] + pos[1] * gridStep[1] + pos[2] * gridStep[2];

	totalFactor = 0;
	for (i = 0; i < 8; i++) 
	{
		float	factor;
		byte* data;
		int		lat, lng;
		vec3_t	normal;

		factor = 1.0;
		data = gridData;

		for (j = 0; j < 3; j++) 
		{
			if (i & (1 << j)) 
			{
				factor *= frac[j];
				data += gridStep[j];
			}
			else 
			{
				factor *= (1.0f - frac[j]);
			}
		}

		if (!(data[0] + data[1] + data[2])) 
		{
			continue;	// ignore samples in walls
		}

		totalFactor += factor;

		ent->ambientLight[0] += factor * data[0];
		ent->ambientLight[1] += factor * data[1];
		ent->ambientLight[2] += factor * data[2];

		ent->directedLight[0] += factor * data[3];
		ent->directedLight[1] += factor * data[4];
		ent->directedLight[2] += factor * data[5];

		lat = data[7];
		lng = data[6];
		lat *= (FUNCTABLE_SIZE / 256);
		lng *= (FUNCTABLE_SIZE / 256);

		// decode X as cos( lat ) * sin( long )
		normal[0] = r_sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * r_sinTable[lng];
		// decode Y as sin( lat ) * sin( long )
		normal[1] = r_sinTable[lat] * r_sinTable[lng];
		// decode Z as cos( long )
		normal[2] = r_sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

		VectorMA(direction, factor, normal, direction);
	}

	if (totalFactor > 0 && totalFactor < 0.99) 
	{
		totalFactor = 1.0f / totalFactor;
		VectorScale(ent->ambientLight, totalFactor, ent->ambientLight);
		VectorScale(ent->directedLight, totalFactor, ent->directedLight);
	}

	VectorScale(ent->ambientLight, r_ambientLightScale->value, ent->ambientLight);
	VectorScale(ent->directedLight, r_directedLightScale->value, ent->directedLight);

	VectorNormalize2(direction, ent->lightDir);
}


/*
===============
LogLight
===============
*/
static void LogLight(rentity_t* ent) 
{
	float	max1, max2;

	if (!(ent->renderfx & RF_VIEW_MODEL)) 
	{
		return;
	}

	max1 = ent->ambientLight[0];
	if (ent->ambientLight[1] > max1) 
	{
		max1 = ent->ambientLight[1];
	}
	else if (ent->ambientLight[2] > max1) 
	{
		max1 = ent->ambientLight[2];
	}

	max2 = ent->directedLight[0];
	if (ent->directedLight[1] > max2) 
	{
		max2 = ent->directedLight[1];
	}
	else if (ent->directedLight[2] > max2) 
	{
		max2 = ent->directedLight[2];
	}

	ri.Printf(PRINT_ALL, "amb: %.2f %.2f %.2f (%i) dir: %.2f %.2f %.2f (%i)\n", ent->ambientLight[0], ent->ambientLight[1], ent->ambientLight[2], max1, ent->directedLight[0], ent->directedLight[1], ent->directedLight[2], max2);
}

void R_SetEntityAmbientLight(rentity_t* ent)
{
	vec3_t lightDir;
	vec3_t lightOrigin;
	vec3_t temp;
	int	i;

	// lighting calculations 
	if (ent->lightingCalculated) 
	{
		return;
	}
	ent->lightingCalculated = true;


	if (ent->renderfx & RF_LIGHTING_ORIGIN) 
	{
		// seperate lightOrigins are needed so an object that is
		// sinking into the ground can still be lit, and so
		// multi-part models can be lit identically
		VectorCopy(ent->lightingOrigin, lightOrigin);
	}
	else
	{
		VectorCopy(ent->origin, lightOrigin);
	}

	// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
	if (!(r_newrefdef.view.flags & RDF_NOWORLDMODEL) && r_world->lightGridData) 
	{
		R_SetupEntityLightingGrid(ent);
	}
	else 
	{
		VectorSet(ent->ambientLight, 255, 255, 255);
		VectorSet(ent->directedLight, 255, 255, 255);
		VectorCopy(r_world->sunDirection, ent->lightDir);

		VectorNormalize(ent->lightDir);
	}

	// give some entities (such as view models) a bit of light so they are never completly dark
	if (ent->renderfx & RF_MINLIGHT ) 
	{
		ent->ambientLight[0] += 32;
		ent->ambientLight[1] += 32;
		ent->ambientLight[2] += 32;
	}


	// clamp ambient to a maximum of 150% brightness
	for (i = 0; i < 3; i++) 
	{
		if (ent->ambientLight[i] > 255 * 1.5) 
		{
			ent->ambientLight[i] = 255 * 1.5;
		}
	}


#if 0 // test: Set dir to UP & RIGHT
	VectorCopy(ent->lightDir, temp);
	VectorSet(ent->lightDir, 0, -1, -1); 
#else

	// Negate the direction for GLSL
	for (i = 0; i < 3; i++)
		ent->lightDir[i] = -ent->lightDir[i];
	VectorNormalize(ent->lightDir);
#endif

	// Convert color from bytes to floats for GLSL
	for (i = 0; i < 3; i++)
	{
		ent->ambientLight[i] = ent->ambientLight[i] * (1.0f / 255.0f);
		ent->directedLight[i] = ent->directedLight[i] * (1.0f / 255.0f);
	}

	if (r_debugLight->value) 
	{
		LogLight(ent);

	}
}

