/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_mesh.c: triangle model functions

#include "r_local.h"

// globals for model lighting
vec3_t	model_shadevector;
float	model_shadelight[3];


void R_DrawMD3Model(centity_t* ent, lod_t lod, float lerp); // r_md3.c
void R_DrawSprite(centity_t* ent); // r_sprite.c

/*
=================
R_EntityShouldRender

decides whenever entity is visible and should be drawn
=================
*/
static qboolean R_EntityShouldRender(centity_t* ent)
{
	vec3_t	bbox[8];

	if (ent->model->type == MOD_SPRITE)
		return true;

	if (ent->alpha <= 0.0f && (ent->renderfx & RF_TRANSLUCENT))
	{
		ri.Con_Printf(PRINT_LOW, "%s:  %f alpha!\n", __FUNCTION__, ent->alpha);
		return false;
	}
	//return false; // completly transparent


	// we usually dont cull viwmodels, unless its centered
	if (ent->renderfx & RF_VIEW_MODEL)
		return r_lefthand->value == 2 ? false : true;

	if (!ent->model) // this shouldn't really happen at this point!
		return false;


	if (ent->model->type == MOD_MD3)
		return true; //todo

	return true;
}

/*
=================
R_SetEntityShadeLight

get lighting information for centity
=================
*/
/*static*/ void R_SetEntityShadeLight(centity_t* ent) // uncomment when md2 is gone
{
	float	scale;
	float	min;
	float	an;
	int		i;

	if ((ent->renderfx & RF_COLOR))
	{
		for (i = 0; i < 3; i++)
			model_shadelight[i] = ent->renderColor[i];
	}
	else if (currententity->renderfx & RF_FULLBRIGHT || r_fullbright->value)
	{
		for (i = 0; i < 3; i++)
			model_shadelight[i] = 1.0;
	}
	else
	{
		R_LightPoint(currententity->origin, model_shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if (ent->renderfx & RF_VIEW_MODEL)
		{
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (model_shadelight[0] > model_shadelight[1])
			{
				if (model_shadelight[0] > model_shadelight[2])
					r_lightlevel->value = 150 * model_shadelight[0];
				else
					r_lightlevel->value = 150 * model_shadelight[2];
			}
			else
			{
				if (model_shadelight[1] > model_shadelight[2])
					r_lightlevel->value = 150 * model_shadelight[1];
				else
					r_lightlevel->value = 150 * model_shadelight[2];
			}
		}
	}

	if (ent->renderfx & RF_MINLIGHT)
	{
		for (i = 0; i < 3; i++)
			if (model_shadelight[i] > 0.1)
				break;

		if (i == 3)
		{
			model_shadelight[0] = 0.1;
			model_shadelight[1] = 0.1;
			model_shadelight[2] = 0.1;
		}
	}

	if (ent->renderfx & RF_GLOW)
	{
		scale = 0.1 * sin(r_newrefdef.time * 7);
		for (i = 0; i < 3; i++)
		{
			min = model_shadelight[i] * 0.8;
			model_shadelight[i] += scale;
			if (model_shadelight[i] < min)
				model_shadelight[i] = min;
		}
	}

	// this is uttery shit
	an = ent->angles[1] / 180 * M_PI;
	model_shadevector[0] = cos(-an);
	model_shadevector[1] = sin(-an);
	model_shadevector[2] = 2;
	VectorNormalize(model_shadevector);
}

static void R_EntityAnim(centity_t* ent, const char* func)
{
	// check if animation is correct
	if ((ent->frame >= ent->model->numframes) || (ent->frame < 0))
	{
		ri.Con_Printf(PRINT_DEVELOPER, "%s: no such frame %d in '%s'\n", func, ent->frame, ent->model->name);
		ent->frame = 0;
		ent->oldframe = 0;
	}

	if ((ent->oldframe >= ent->model->numframes) || (ent->oldframe < 0))
	{
		ri.Con_Printf(PRINT_DEVELOPER, "%s: no such oldframe %d in '%s'\n", func, ent->frame, ent->model->name);
		ent->frame = 0;
		ent->oldframe = 0;
	}

	// decide if we should lerp
	if (!r_lerpmodels->value || ent->renderfx & RF_NOANIMLERP)
		ent->backlerp = 0;
}

void R_DrawEntityModel(centity_t* ent)
{
	float		lerp;
	lod_t		lod;

	// don't bother if we're not visible
	if (!R_EntityShouldRender(ent))
		return;

	// check if the animation is correct and set lerp
	R_EntityAnim(ent, __FUNCTIONW__);
	lerp = 1.0 - ent->backlerp;

	// setup lighting
	R_SetEntityShadeLight(ent);

	qglShadeModel(GL_SMOOTH);
	GL_TexEnv(GL_MODULATE);

	// 1. transparency
	if (ent->renderfx & RF_TRANSLUCENT)
		qglEnable(GL_BLEND);

	// hack the depth range to prevent view model from poking into walls
	if (ent->renderfx & RF_DEPTHHACK)
		qglDepthRange(gldepthmin, gldepthmin + 0.3 * (gldepthmax - gldepthmin));

	// if its a view model and we chose to keep it in left hand 
	if ((ent->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{
		extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglScalef(-1, 1, 1);
		MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / r_newrefdef.height, 4, 4096);
		qglMatrixMode(GL_MODELVIEW);
		qglCullFace(GL_BACK);
	}

	qglPushMatrix();
	{
		// move, rotate and scale
		ent->angles[PITCH] = -ent->angles[PITCH];
		R_RotateForEntity(ent);
		ent->angles[PITCH] = -ent->angles[PITCH];

		if (ent->renderfx & RF_SCALE && ent->scale > 0.0f)
			qglScalef(ent->scale, ent->scale, ent->scale);

		// render model
		switch (ent->model->type)
		{
		case MOD_MD3:
			lod = LOD_HIGH;
			R_DrawMD3Model(ent, lod, lerp);
			break;

		case MOD_SPRITE:
			R_DrawSprite(ent);
			break;
		default:
			ri.Sys_Error(ERR_DROP, "R_DrawEntityModel: wrong model type: %s", ent->model->type);
		}

		// restore scale
		if (ent->renderfx & RF_SCALE)
			qglScalef(1.0f, 1.0f, 1.0f);
	}
	qglPopMatrix();

	// restore shade model
	qglShadeModel(GL_FLAT);
	GL_TexEnv(GL_REPLACE);

	// restore transparency
	if (currententity->renderfx & RF_TRANSLUCENT)
		qglDisable(GL_BLEND);

	// remove depth hack
	if (currententity->renderfx & RF_DEPTHHACK)
		qglDepthRange(gldepthmin, gldepthmax);

	if ((currententity->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		qglCullFace(GL_FRONT);
	}
}


void R_EmitWireBox(vec3_t mins, vec3_t maxs)
{
	qglBegin(GL_QUAD_STRIP);
	qglVertex3f(mins[0], mins[1], mins[2]);
	qglVertex3f(mins[0], mins[1], maxs[2]);
	qglVertex3f(maxs[0], mins[1], mins[2]);
	qglVertex3f(maxs[0], mins[1], maxs[2]);
	qglVertex3f(maxs[0], maxs[1], mins[2]);
	qglVertex3f(maxs[0], maxs[1], maxs[2]);
	qglVertex3f(mins[0], maxs[1], mins[2]);
	qglVertex3f(mins[0], maxs[1], maxs[2]);
	qglVertex3f(mins[0], mins[1], mins[2]);
	qglVertex3f(mins[0], mins[1], maxs[2]);
	qglEnd();
}

void R_EmitWirePoint(vec3_t origin)
{
	int size = 8;

	qglBegin(GL_LINES);
	qglVertex3f(origin[0] - size, origin[1], origin[2]);
	qglVertex3f(origin[0] + size, origin[1], origin[2]);
	qglVertex3f(origin[0], origin[1] - size, origin[2]);
	qglVertex3f(origin[0], origin[1] + size, origin[2]);
	qglVertex3f(origin[0], origin[1], origin[2] - size);
	qglVertex3f(origin[0], origin[1], origin[2] + size);
	qglEnd();
}


void R_DrawBBox(vec3_t mins, vec3_t maxs)
{
	qglDisable(GL_DEPTH_TEST);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_CULL_FACE);
	qglColor3f(1, 1, 1);

	R_EmitWireBox(mins, maxs);

	qglColor3f(1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
	qglEnable(GL_CULL_FACE);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglEnable(GL_DEPTH_TEST);
}


