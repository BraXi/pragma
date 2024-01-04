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
//	vec3_t	bbox[8];

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

	// --- begin yquake2 ---
	// Apply r_overbrightbits to the mesh. If we don't do this they will appear slightly dimmer relative to walls.
	if (r_overbrightbits->value)
	{
		for (i = 0; i < 3; ++i)
		{
			model_shadelight[i] *= r_overbrightbits->value;
		}
	}
	// --- end yquake2 ---

	// this is uttery shit
	an = ent->angles[1] / 180 * M_PI;
	model_shadevector[0] = cos(-an);
	model_shadevector[1] = sin(-an);
	model_shadevector[2] = 100;
	VectorNormalize(model_shadevector);
}

static void R_EntityAnim(centity_t* ent, char* func)
{
	// check if animation is correct
	if ((ent->frame >= ent->model->numframes) || (ent->frame < 0))
	{
//		ri.Con_Printf(PRINT_DEVELOPER, "%s: no such frame %d in '%s'\n", func, ent->frame, ent->model->name);
		ent->frame = 0;
		ent->oldframe = 0;
	}

	if ((ent->oldframe >= ent->model->numframes) || (ent->oldframe < 0))
	{
//		ri.Con_Printf(PRINT_DEVELOPER, "%s: no such oldframe %d in '%s'\n", func, ent->frame, ent->model->name);
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
	R_EntityAnim(ent, __FUNCTION__);
	lerp = 1.0 - ent->backlerp;

	// setup lighting
	R_SetEntityShadeLight(ent);

	qglShadeModel(GL_SMOOTH);
	GL_TexEnv(GL_MODULATE);

	// 1. transparency
	if (ent->renderfx & RF_TRANSLUCENT)
		R_Blend(true);

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
		R_SetCullFace(GL_BACK);
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
		R_Blend(false);

	// remove depth hack
	if (currententity->renderfx & RF_DEPTHHACK)
		qglDepthRange(gldepthmin, gldepthmax);

	if ((currententity->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		R_SetCullFace(GL_FRONT);
	}
}


/*
=============
R_BeginLinesRendering
=============
*/
void R_BeginLinesRendering(qboolean dt)
{
	R_DepthTest(dt);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	qglDisable(GL_TEXTURE_2D);
	R_CullFace(false);
	qglColor4f(1, 1, 1, 1);
}

/*
=============
R_EndLinesRendering
=============
*/
void R_EndLinesRendering()
{
	qglColor4f(1, 1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
	R_CullFace(true);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	R_DepthTest(true);
}


/*
=============
R_DrawDebugLine
=============
*/
static void R_DrawDebugLine(debugprimitive_t *line)
{
	qglBegin(GL_LINES);
		qglVertex3fv(line->p1);
		qglVertex3fv(line->p2);
	qglEnd();
}

/*
=============
R_DrawWirePoint
=============
*/
static void R_DrawWirePoint(vec3_t origin)
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

static void R_DrawWireBoundingBox(vec3_t mins, vec3_t maxs)
{
	qglBegin(GL_LINES);
		qglVertex3f(mins[0], mins[1], mins[2]);
		qglVertex3f(maxs[0], mins[1], mins[2]);

		qglVertex3f(mins[0], maxs[1], mins[2]);
		qglVertex3f(maxs[0], maxs[1], mins[2]);

		qglVertex3f(mins[0], mins[1], maxs[2]);
		qglVertex3f(maxs[0], mins[1], maxs[2]);

		qglVertex3f(mins[0], maxs[1], maxs[2]);
		qglVertex3f(maxs[0], maxs[1], maxs[2]);

		qglVertex3f(mins[0], mins[1], mins[2]);
		qglVertex3f(mins[0], maxs[1], mins[2]);

		qglVertex3f(maxs[0], mins[1], mins[2]);
		qglVertex3f(maxs[0], maxs[1], mins[2]);

		qglVertex3f(mins[0], mins[1], maxs[2]);
		qglVertex3f(mins[0], maxs[1], maxs[2]);

		qglVertex3f(maxs[0], mins[1], maxs[2]);
		qglVertex3f(maxs[0], maxs[1], maxs[2]);

		qglVertex3f(mins[0], mins[1], mins[2]);
		qglVertex3f(mins[0], mins[1], maxs[2]);

		qglVertex3f(mins[0], maxs[1], mins[2]);
		qglVertex3f(mins[0], maxs[1], maxs[2]);

		qglVertex3f(maxs[0], mins[1], mins[2]);
		qglVertex3f(maxs[0], mins[1], maxs[2]);

		qglVertex3f(maxs[0], maxs[1], mins[2]);
		qglVertex3f(maxs[0], maxs[1], maxs[2]);
	qglEnd();
}
/*
=============
R_DrawWireBox
=============
*/
static void R_DrawWireBox(vec3_t mins, vec3_t maxs)
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

/*
=============
R_DrawDebugLines
=============
*/
extern void R_DrawString3D(char* string, vec3_t pos, float fontSize, int alignx, vec3_t color);
void R_DrawDebugLines(void)
{
	int		i;
	debugprimitive_t* line;

	qglPushMatrix();
	R_BeginLinesRendering(true);
	for (i = 0; i < r_newrefdef.num_debugprimitives; i++)
	{
		line = &r_newrefdef.debugprimitives[i];
		if (!line->depthTest)
			continue;

		if (line->type == DPRIMITIVE_TEXT)
			continue;

		qglColor3fv(line->color);
		qglLineWidth(line->thickness);
		switch (line->type)
		{
		case DPRIMITIVE_LINE:
			R_DrawDebugLine(line);
			break;
		case DPRIMITIVE_POINT:
			R_DrawWirePoint(line->p1);
			break;
		case DPRIMITIVE_BOX:
			R_DrawWireBoundingBox(line->p1, line->p2);
			break;
		}
	}
	R_EndLinesRendering();

#if 1
	GL_TexEnv(GL_MODULATE);
	R_AlphaTest(true);
	R_DepthTest(true);
	for (i = 0; i < r_newrefdef.num_debugprimitives; i++)
	{
		line = &r_newrefdef.debugprimitives[i];
		if (!line->depthTest)
			continue;

		switch (line->type)
		{
		case DPRIMITIVE_TEXT:

			R_DrawString3D(line->text, line->p1, line->thickness, 1, line->color);
			break;
		}
	}
	GL_TexEnv(GL_REPLACE);
	R_AlphaTest(false);
#endif

#if 1
	R_WriteToDepthBuffer(GL_FALSE);
	R_BeginLinesRendering(false);
	for (i = 0; i < r_newrefdef.num_debugprimitives; i++)
	{
		line = &r_newrefdef.debugprimitives[i];
		if (line->depthTest)
			continue;
		if (line->type == DPRIMITIVE_TEXT)
			continue;

		qglColor3fv(line->color);
		qglLineWidth(line->thickness);
		switch (line->type)
		{
		case DPRIMITIVE_LINE:
			R_DrawDebugLine(line);
			break;
		case DPRIMITIVE_POINT:
			R_DrawWirePoint(line->p1);
			break;
		case DPRIMITIVE_BOX:
			R_DrawWireBoundingBox(line->p1, line->p2);
			break;
		}
	}
	R_EndLinesRendering();


//	R_CullFace(false);
	GL_TexEnv(GL_MODULATE);
	R_AlphaTest(true);
	R_DepthTest(false);
	for (i = 0; i < r_newrefdef.num_debugprimitives; i++)
	{
		line = &r_newrefdef.debugprimitives[i];
		if (line->depthTest)
			continue;

		switch (line->type)
		{
		case DPRIMITIVE_TEXT:

			R_DrawString3D(line->text, line->p1, line->thickness, 1, line->color);
			break;
		}
	}
	GL_TexEnv(GL_REPLACE);
	R_AlphaTest(false);
	R_DepthTest(true);
	R_WriteToDepthBuffer(GL_TRUE);
#endif

	qglColor3f(1,1,1);
	qglLineWidth(1.0f);
	qglPopMatrix();
}



void R_DrawBBox(vec3_t mins, vec3_t maxs)
{
	R_BeginLinesRendering(true);

//	R_EmitWireBox(mins, maxs);

	R_EndLinesRendering();
}


