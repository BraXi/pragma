/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_mesh.c: triangle model functions

#include "r_local.h"

void R_DrawNewModel(rentity_t* ent);

void R_DrawSkelModel(rentity_t* ent); // r_smdl.c
void R_DrawAliasModel(rentity_t* ent, float animlerp); // r_aliasmod.c

qboolean r_pendingflip = false;


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
		VectorCopy(ent->renderColor, ent->ambient_color);
	}
	else if (ent->renderfx & RF_FULLBRIGHT || r_fullbright->value)
	{
		VectorSet(ent->ambient_color, 1.0f, 1.0f, 1.0f);
	}
	else 
	{
		R_LightPoint(ent->origin, ent->ambient_color);
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

	yaw = ent->angles[1] / 180 * M_PI;
	ent->ambient_dir[0] = cos(-yaw);
	ent->ambient_dir[1] = sin(-yaw);
	ent->ambient_dir[2] = -1;
	VectorNormalize(ent->ambient_dir);
}

/*
=================
R_ValidateEntityAnimation
Validate frame and oldframe, takes care of RF_NOANIMLERP effect.
=================
*/
void R_ValidateEntityAnimation(rentity_t* ent)
{
	if (ent->model && ent->model->type != MOD_NEWFORMAT)
	{
		if ((ent->frame >= ent->model->numframes) || (ent->frame < 0))
		{
			ri.Printf(PRINT_DEVELOPER, "No such frame %d in '%s'\n", ent->frame, ent->model->name);
			ent->frame = 0;
			ent->oldframe = 0;
		}

		if ((ent->oldframe >= ent->model->numframes) || (ent->oldframe < 0))
		{
			ri.Printf(PRINT_DEVELOPER, "No such oldframe %d in '%s'\n", ent->frame, ent->model->name);
			ent->frame = 0;
			ent->oldframe = 0;
		}
	}
	// decide if we should lerp
	if (!r_lerpmodels->value || ent->renderfx & RF_NOANIMLERP)
		ent->animbacklerp = 0.0f;
}

/*
=================
R_PreProcessModelEntity

Figure out if the entity should be rendered this frame,
if so, set ambient light values, effects and validate animation.
Correct only for entities that have MOD_ALIAS or MOD_NEWFORMAT models.

Notes: 
RF_VIEW_MODEL entities are never culled.
RF_TRANSLUCENT entities with too low alpha are skipped.
RF_SCALE with too small scale are skipped.
Entities that are too far from camera are skipped.

FIXME: Culling based on model's draw distance doesn't account for FOV !!
=================
*/
void R_PreProcessModelEntity(rentity_t* ent)
{
	int i;
	vec3_t mins, maxs, v;
	float scale = 1.0f;

	if (ent->alpha <= 0.05f && (ent->renderfx & RF_TRANSLUCENT))
	{
		return; // completly transparent, reject
	}

	if (ent->renderfx & RF_VIEW_MODEL && r_lefthand->value >= 2)
	{
		return; // player desires to not draw view model
	}
	else if (ent->model->cullDist > 0.0f)
	{
		// cull objects based on distance, but only if they're not a view model
		VectorSubtract(r_newrefdef.view.origin, ent->origin, v);
		if (VectorLength(v) > ent->model->cullDist)
			return; // entity is too far, reject
	}

	if (ent->renderfx & RF_BEAM)
	{
		// assume beams are always visible
		ent->visibleFrame = r_framecount;
		return;
	}

	if (ent->model->type == MOD_ALIAS || ent->model->type == MOD_NEWFORMAT)
	{
		if (ent->renderfx & RF_SCALE)
		{
			if (ent->scale <= 0.1f)
				return; // model is too tiny, reject

			scale = ent->scale; // adjust radius if the model was scaled
		}

		if (!VectorCompare(ent->angles, vec3_origin) || scale != 1.0)
		{
			for (i = 0; i < 3; i++)
			{
				mins[i] = ent->origin[i] - (ent->model->radius * scale);
				maxs[i] = ent->origin[i] + (ent->model->radius * scale);
			}
		}
		else
		{
			VectorAdd(ent->origin, ent->model->mins, mins);
			VectorAdd(ent->origin, ent->model->maxs, maxs);
		}

		if (R_CullBox(mins, maxs))
			return; // not in view frustum, reject
	}

	// entity is visible this frame!
	ent->visibleFrame = r_framecount;

	R_SetEntityAmbientLight(ent);
	R_ValidateEntityAnimation(ent);
}


/*
=================
R_DrawModelEntity
Draws an entity which has a regular model (not inline model).
=================
*/
void R_DrawModelEntity(rentity_t* ent)
{
	if (ent->visibleFrame != r_framecount)
		return; // not visible in this frame

	// 1. transparency
	if (ent->renderfx & RF_TRANSLUCENT)
		R_Blend(true);

	// hack the depth range to prevent view model from poking into walls
	if (ent->renderfx & RF_DEPTHHACK)
		glDepthRange(gldepthmin, gldepthmin + 0.3f * (gldepthmax - gldepthmin));

	// move, rotate and scale
#ifndef FIX_SQB
	ent->angles[PITCH] = -ent->angles[PITCH]; // stupid quake bug
#endif
	R_RotateForEntity(ent);
#ifndef FIX_SQB
	ent->angles[PITCH] = -ent->angles[PITCH]; // stupid quake bug
#endif

	if (ent->renderfx & RF_SCALE && ent->scale > 0.0f)
		Mat4Scale(r_local_matrix, ent->scale, ent->scale, ent->scale);

	// if its a view model and we chose to keep it in left hand 
	if ((ent->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{
		//Mat4Scale(r_local_matrix, 1, -1, 1); //I wish I could, but the muzzle flash is manually placed. Have to flip the projection matrix
		Mat4Scale(r_projection_matrix, -1, 1, 1);
		r_pendingflip = true;
		R_SetCullFace(GL_BACK);
	}

	// render model
	switch (ent->model->type)
	{
	case MOD_ALIAS:
		R_DrawAliasModel(ent, (1.0f - ent->animbacklerp));
		break;

	case MOD_NEWFORMAT:
		R_DrawNewModel(ent);
		break;

	default:
		ri.Error(ERR_DROP, "R_DrawModelEntity: wrong model type %i", ent->model->type);
	}

	// restore transparency
	if (ent->renderfx & RF_TRANSLUCENT)
		R_Blend(false);

	// remove depth hack
	if (ent->renderfx & RF_DEPTHHACK)
		glDepthRange(gldepthmin, gldepthmax);

	if ((ent->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{ 
		//scale unset by whatever consumed pendingflip. 
		r_pendingflip = false;
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
	glDisable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	R_CullFace(false);
	R_DepthTest(dt);
	R_BindProgram(GLPROG_DEBUGLINE);
}

/*
=============
R_EndLinesRendering
=============
*/
void R_EndLinesRendering()
{
	glEnable(GL_TEXTURE_2D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	R_CullFace(true);
	R_DepthTest(true);
	R_UnbindProgram();
}


/*
=============
R_DrawDebugLine
=============
*/
static void R_DrawDebugLine(debugprimitive_t *line)
{
	glBegin(GL_LINES);
		glVertex3fv(line->p1);
		glVertex3fv(line->p2);
	glEnd();
}

/*
=============
R_DrawWirePoint
=============
*/
static void R_DrawWirePoint(vec3_t origin)
{
	int size = 8;
	glBegin(GL_LINES);
		glVertex3f(origin[0] - size, origin[1], origin[2]);
		glVertex3f(origin[0] + size, origin[1], origin[2]);
		glVertex3f(origin[0], origin[1] - size, origin[2]);
		glVertex3f(origin[0], origin[1] + size, origin[2]);
		glVertex3f(origin[0], origin[1], origin[2] - size);
		glVertex3f(origin[0], origin[1], origin[2] + size);
	glEnd();
}

static void R_DrawWireBoundingBox(vec3_t mins, vec3_t maxs)
{
	glBegin(GL_LINES);
		glVertex3f(mins[0], mins[1], mins[2]);
		glVertex3f(maxs[0], mins[1], mins[2]);

		glVertex3f(mins[0], maxs[1], mins[2]);
		glVertex3f(maxs[0], maxs[1], mins[2]);

		glVertex3f(mins[0], mins[1], maxs[2]);
		glVertex3f(maxs[0], mins[1], maxs[2]);

		glVertex3f(mins[0], maxs[1], maxs[2]);
		glVertex3f(maxs[0], maxs[1], maxs[2]);

		glVertex3f(mins[0], mins[1], mins[2]);
		glVertex3f(mins[0], maxs[1], mins[2]);

		glVertex3f(maxs[0], mins[1], mins[2]);
		glVertex3f(maxs[0], maxs[1], mins[2]);

		glVertex3f(mins[0], mins[1], maxs[2]);
		glVertex3f(mins[0], maxs[1], maxs[2]);

		glVertex3f(maxs[0], mins[1], maxs[2]);
		glVertex3f(maxs[0], maxs[1], maxs[2]);

		glVertex3f(mins[0], mins[1], mins[2]);
		glVertex3f(mins[0], mins[1], maxs[2]);

		glVertex3f(mins[0], maxs[1], mins[2]);
		glVertex3f(mins[0], maxs[1], maxs[2]);

		glVertex3f(maxs[0], mins[1], mins[2]);
		glVertex3f(maxs[0], mins[1], maxs[2]);

		glVertex3f(maxs[0], maxs[1], mins[2]);
		glVertex3f(maxs[0], maxs[1], maxs[2]);
	glEnd();
}
/*
=============
R_DrawWireBox
=============
*/
static void R_DrawWireBox(vec3_t mins, vec3_t maxs)
{
	glBegin(GL_QUAD_STRIP);
	glVertex3f(mins[0], mins[1], mins[2]);
	glVertex3f(mins[0], mins[1], maxs[2]);
	glVertex3f(maxs[0], mins[1], mins[2]);
	glVertex3f(maxs[0], mins[1], maxs[2]);
	glVertex3f(maxs[0], maxs[1], mins[2]);
	glVertex3f(maxs[0], maxs[1], maxs[2]);
	glVertex3f(mins[0], maxs[1], mins[2]);
	glVertex3f(mins[0], maxs[1], maxs[2]);
	glVertex3f(mins[0], mins[1], mins[2]);
	glVertex3f(mins[0], mins[1], maxs[2]);
	glEnd();
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

	R_BeginLinesRendering(true);
	for (i = 0; i < r_newrefdef.num_debugprimitives; i++)
	{
		line = &r_newrefdef.debugprimitives[i];
		if (!line->depthTest)
			continue;

		if (line->type == DPRIMITIVE_TEXT)
			continue;

		R_ProgUniform4f(LOC_COLOR4, line->color[0], line->color[1], line->color[2], 1.0f);
		glLineWidth(line->thickness);
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

		R_ProgUniform4f(LOC_COLOR4, line->color[0], line->color[1], line->color[2], 1.0f);
		glLineWidth(line->thickness);
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
	R_AlphaTest(false);
	R_DepthTest(true);
	R_WriteToDepthBuffer(GL_TRUE);
#endif
	glLineWidth(1.0f);
}



