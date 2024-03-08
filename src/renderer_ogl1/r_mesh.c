/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_mesh.c: triangle model functions

#include "r_local.h"

// globals for model lighting
vec3_t	model_shadevector;
float	model_shadelight[3];

void R_DrawMD3Model(rentity_t* ent, lod_t lod, float animlerp); // r_md3.c
void R_DrawSprite(rentity_t* ent); // r_sprite.c

/*
=================
R_EntityShouldRender

Decides whenever entity is visible and could be drawn
=================
*/
static qboolean R_EntityShouldRender(rentity_t* ent)
{
	int i;
	vec3_t mins, maxs, v;

	if (!ent->model) // this shouldn't really happen at this point!
		return false;

	if (ent->model->type == MOD_SPRITE)
		return true;

	if (ent->alpha <= 0.01f && (ent->renderfx & RF_TRANSLUCENT))
	{
		ri.Printf(PRINT_LOW, "%s: %f alpha!\n", __FUNCTION__, ent->alpha);
		return false;
	}
	if (ent->renderfx & RF_VIEW_MODEL)
	{
		// don't cull viwmodels unless its centered
		return r_lefthand->value == 2 ? false : true;
	}
	else if(pCurrentModel->cullDist > 0)
	{
		// cull objects based on distance TODO: account for scale or na?
		VectorSubtract(r_newrefdef.view.origin, ent->origin, v); // FIXME: doesn't account for FOV
		if (VectorLength(v) > pCurrentModel->cullDist)
			return false;
	}



	if (ent->model->type == MOD_MD3) 
	{
		// technicaly this could be used for sprites, but it takes 
		// more cycles culling than actually rendering them lol

		if (ent->angles[0] || ent->angles[1] || ent->angles[2])
		{
			for (i = 0; i < 3; i++)
			{
				mins[i] = ent->origin[i] - pCurrentModel->radius;
				maxs[i] = ent->origin[i] + pCurrentModel->radius;
			}
		}
		else
		{
			VectorAdd(ent->origin, ent->model->mins, mins);
			VectorAdd(ent->origin, pCurrentModel->maxs, maxs);
		}

		if ((ent->renderfx & RF_SCALE) && ent->scale != 1.0f && ent->scale > 0.0f)
		{
			// adjust bbox for scale
			for (i = 0; i < 3; i++)
			{
				mins[i] = mins[i] * ent->scale;
				maxs[i] = maxs[i] * ent->scale;
			}
		}

		if (R_CullBox(mins, maxs))
			return false;
	}

	return true;
}

/*
=================
R_SetEntityShadeLight

get lighting information for centity
=================
*/
void R_SetEntityShadeLight(rentity_t* ent)
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
	else if (pCurrentRefEnt->renderfx & RF_FULLBRIGHT || r_fullbright->value)
	{
		for (i = 0; i < 3; i++)
			model_shadelight[i] = 1.0;
	}
	else 
	{
		if (ent->inheritLight > 0)
		{
			VectorCopy(r_newrefdef.entities[ent->inheritLight].shadelightpoint, model_shadelight);
		}
		else
		{
			R_LightPoint(pCurrentRefEnt->origin, model_shadelight);
			VectorCopy(model_shadelight, ent->shadelightpoint);
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
		scale = 0.1 * sin(r_newrefdef.time * 7.0);
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
	model_shadevector[2] = 0;
	VectorNormalize(model_shadevector);
}

static void R_EntityAnim(rentity_t* ent, char* func)
{
	// check if animation is correct
	if ((ent->frame >= ent->model->numframes) || (ent->frame < 0))
	{
		ri.Printf(PRINT_DEVELOPER, "%s: no such frame %d in '%s'\n", func, ent->frame, ent->model->name);
		ent->frame = 0;
		ent->oldframe = 0;
	}

	if ((ent->oldframe >= ent->model->numframes) || (ent->oldframe < 0))
	{
		ri.Printf(PRINT_DEVELOPER, "%s: no such oldframe %d in '%s'\n", func, ent->frame, ent->model->name);
		ent->frame = 0;
		ent->oldframe = 0;
	}

	// decide if we should lerp
	if (!r_lerpmodels->value || ent->renderfx & RF_NOANIMLERP)
		ent->animbacklerp = 0.0f;
}

void R_DrawEntityModel(rentity_t* ent)
{
	float		lerp;
	lod_t		lod;

	// don't bother if we're not visible
	if (!R_EntityShouldRender(ent))
		return;

	// check if the animation is correct and set lerp
	R_EntityAnim(ent, __FUNCTION__);
//	lerp = 1.0 - ent->backlerp;
	lerp = 1.0 - ent->animbacklerp;

	// setup lighting
	R_SetEntityShadeLight(ent);
	R_SetTexEnv(GL_MODULATE);

	// 1. transparency
	if (ent->renderfx & RF_TRANSLUCENT)
		R_Blend(true);

	// hack the depth range to prevent view model from poking into walls
	if (ent->renderfx & RF_DEPTHHACK)
		glDepthRange(gldepthmin, gldepthmin + 0.3 * (gldepthmax - gldepthmin));

	// if its a view model and we chose to keep it in left hand 
	if ((ent->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{
		extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glScalef(-1, 1, 1);
		MYgluPerspective(r_newrefdef.view.fov_y, (float)r_newrefdef.width / r_newrefdef.height, 4, 4096);
		glMatrixMode(GL_MODELVIEW);
		R_SetCullFace(GL_BACK);
	}

	glPushMatrix();
	{
		// move, rotate and scale
		ent->angles[PITCH] = -ent->angles[PITCH];
		R_RotateForEntity(ent);
		ent->angles[PITCH] = -ent->angles[PITCH];

		if (ent->renderfx & RF_SCALE && ent->scale > 0.0f)
			glScalef(ent->scale, ent->scale, ent->scale);

		// render model
		switch (ent->model->type)
		{
		case MOD_MD3:
			lod = LOD_HIGH;
			R_DrawMD3Model(ent, lod, lerp);
			break;

		case MOD_SPRITE:
			R_UnbindProgram();
			R_DrawSprite(ent);
			break;
		default:
			ri.Error(ERR_DROP, "R_DrawEntityModel: wrong model type: %s", ent->model->type);
		}

		// restore scale
		if (ent->renderfx & RF_SCALE)
			glScalef(1.0f, 1.0f, 1.0f);
	}
	glPopMatrix();

	// restore shade model
	R_SetTexEnv(GL_REPLACE);

	// restore transparency
	if (pCurrentRefEnt->renderfx & RF_TRANSLUCENT)
		R_Blend(false);

	// remove depth hack
	if (pCurrentRefEnt->renderfx & RF_DEPTHHACK)
		glDepthRange(gldepthmin, gldepthmax);

	if ((pCurrentRefEnt->renderfx & RF_VIEW_MODEL) && (r_lefthand->value == 1.0F))
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
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

	glPushMatrix();

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
	R_UnbindProgram();
	R_EndLinesRendering();

#if 1
	R_SetTexEnv(GL_MODULATE);
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
	R_SetTexEnv(GL_REPLACE);
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
	R_SetTexEnv(GL_MODULATE);
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
	R_SetTexEnv(GL_REPLACE);
	R_AlphaTest(false);
	R_DepthTest(true);
	R_WriteToDepthBuffer(GL_TRUE);
#endif

	glColor3f(1,1,1);
	glLineWidth(1.0f);
	glPopMatrix();
}



void R_DrawBBox(vec3_t mins, vec3_t maxs)
{
	R_BeginLinesRendering(true);

//	R_EmitWireBox(mins, maxs);

	R_EndLinesRendering();
}


