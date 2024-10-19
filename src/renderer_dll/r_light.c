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
===============
R_LightForPoint

Returns the lightmap pixel color beneath point
===============
*/
void R_LightForPoint(const vec3_t point, vec3_t outAmbient)
{
	VectorSet(outAmbient, 1.0f, 1.0f, 1.0f); 
	VectorScale (outAmbient, r_ambientlightscale->value, outAmbient);
}
