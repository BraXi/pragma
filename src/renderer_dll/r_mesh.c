/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_mesh.c: triangle model functions

#include "r_local.h"

void R_DrawNewModel(const rentity_t* ent, qboolean isAnimated);
void R_DrawAliasModel(const rentity_t* ent); // r_aliasmod.c

qboolean r_pendingflip = false;

void R_DrawString3D(char* string, vec3_t pos, float fontSize, int alignx, vec3_t color);

/*
=================
R_EntityCountVisibleParts
Returns number of visible parts.
=================
*/
static int R_EntityCountVisibleParts(const rentity_t* ent)
{
	int surf, numSurfaces, numVis;

	if (!ent->model)
		return 0;

	if (ent->model->type == MOD_ALIAS)
	{
		numSurfaces = ent->model->alias->numSurfaces;
	}
	else if (ent->model->type == MOD_NEWFORMAT)
	{
		numSurfaces = ent->model->newmod->numSurfaces;
	}
	else
	{
#ifdef _DEBUG
		ri.Printf(PRINT_HIGH, "%s: unknown model type\n", __FUNCTION__);
#endif
		return 0;
	}

#ifdef _DEBUG
	if(numSurfaces > 8)
		ri.Error(ERR_FATAL, "%s: numSurfaces > 8\n", __FUNCTION__);
#endif

	for (surf = 0, numVis = 0; surf < numSurfaces; surf++)
	{
		if ((ent->hiddenPartsBits & (1 << surf)))
		{
			continue;
		}
		numVis++;
	}

	return numVis;
}

/*
=================
R_ValidateEntityAnimation
Validate frame and oldframe, takes care of RF_NOANIMLERP effect.
=================
*/
static void R_ValidateEntityAnimation(rentity_t* ent)
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

	if (R_EntityCountVisibleParts(ent) == 0)
	{
		rperf.ent_cull_nosurfs++;
		return; // all parts are hidden so there's nothing to draw, reject
	}

	if (ent->alpha <= 0.05f && (ent->renderfx & RF_TRANSLUCENT))
	{
		rperf.ent_cull_alpha++;
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
		{
			rperf.ent_cull_distance++;
			return; // entity is too far, reject
		}
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

		for (i = 0; i < 3; i++)
			ent->center_origin[i] = ent->origin[i] + ((ent->model->mins[i] + ent->model->maxs[i]) / 2.0f);

		if (R_CullBox(mins, maxs))
		{
			rperf.ent_cull_frustum++;
			return; // not in view frustum, reject
		}
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
	qboolean bAnimated = false;

	if (ent->visibleFrame != r_framecount)
		return; // not visible in this frame

	// move, rotate and scale
#ifndef FIX_SQB
	ent->angles[PITCH] = -ent->angles[PITCH]; // stupid quake bug
#endif
	R_RotateForEntity(ent);
#ifndef FIX_SQB
	ent->angles[PITCH] = -ent->angles[PITCH]; // stupid quake bug
#endif

	// decide what program to use
	if (ent->model->type == MOD_ALIAS)
	{
		R_BindProgram(GLPROG_ALIAS);
		R_ProgUniform1f(LOC_LERPFRAC, (1.0f - ent->animbacklerp));
	}
	else if (ent->model->type == MOD_NEWFORMAT)
	{
		if(bAnimated)
			R_BindProgram(GLPROG_SMDL_ANIMATED);
		else
			R_BindProgram(GLPROG_SMDL_RIGID);
	}
	else
	{
		ri.Error(ERR_DROP, "R_DrawModelEntity: wrong model type %i", ent->model->type);
		return;
	}

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

	// enable transparency
	if (ent->renderfx & RF_TRANSLUCENT)
		R_Blend(true);

	// hack the depth range to prevent view model from poking into walls
	if (ent->renderfx & RF_DEPTHHACK)
		glDepthRange(gldepthmin, gldepthmin + 0.3f * (gldepthmax - gldepthmin));

	// 
	// setup common uniforms
	//
	R_ProgUniformVec3(LOC_AMBIENT_COLOR, ent->ambient_color);
	R_ProgUniformVec3(LOC_AMBIENT_DIR, ent->ambient_dir);

	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);
	if (r_pendingflip)
	{
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	R_ProgUniform1f(LOC_PARM0, (r_fullbright->value || ent->renderfx & RF_FULLBRIGHT) ? 1.0f : 0.0f);
	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, (ent->renderfx & RF_TRANSLUCENT) ? ent->alpha : 1.0f);
	R_SendDynamicLightsToCurrentProgram((ent->renderfx & RF_VIEW_MODEL));

	//
	// render the model
	//
	switch (ent->model->type)
	{
	case MOD_ALIAS:
		glDisable(GL_CULL_FACE);
		R_DrawAliasModel(ent);
		glEnable(GL_CULL_FACE);
		break;
	case MOD_NEWFORMAT:
		R_DrawNewModel(ent, bAnimated);
		break;
	}

	// disable transparency
	if (ent->renderfx & RF_TRANSLUCENT)
		R_Blend(false);

	// remove depth hack
	if (ent->renderfx & RF_DEPTHHACK)
		glDepthRange(gldepthmin, gldepthmax);

	if (r_pendingflip)
	{
		Mat4Scale(r_projection_matrix, -1, 1, 1);
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (pCurrentRefEnt->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(false);

	if ((ent->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{ 
		//scale unset by whatever consumed pendingflip. 
		r_pendingflip = false;
		R_SetCullFace(GL_FRONT);
	}

	R_UnbindProgram();
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
R_DrawDebugLines
=============
*/
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



