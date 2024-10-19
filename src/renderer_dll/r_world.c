/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_world.c -- NEW WORLD RENDERING CODE

#include <assert.h>
#include "r_local.h"

static vec3_t		modelorg; // relative to viewpoint

void R_BeginLinesRendering(qboolean dt);
void R_EndLinesRendering();

qboolean R_DrawQ3World();

extern int registration_sequence; // experimental nature heh

/*
=================
R_DrawBrushModel

If bTransparentPass is true, only transparent bits will be drawn
=================
*/
void R_DrawBrushModel(rentity_t *ent)
{

}


/*
=================
R_PreprocessBrushModelEntity
Checks if entity is visible this frame and marks which dynamic lights impact this entity
=================
*/
void R_PreprocessBrushModelEntity(rentity_t * ent)
{
	vec3_t		mins, maxs;
	qboolean	rotated;
	//dlight_t	*light;
	//vec3_t		lightorg;
	int			i;

#ifdef _DEBUG
	if (!ent->model || ent->model->type != MOD_BRUSH)
		return;
#endif

	if ((ent->renderfx & RF_TRANSLUCENT) && ent->alpha <= 0.05f)
	{
		return; // completly transparent, reject
	}

	if (ent->angles[0] || ent->angles[1] || ent->angles[2])
	{
		rotated = true;
		for (i = 0; i < 3; i++)
		{
			mins[i] = ent->origin[i] - ent->model->radius;
			maxs[i] = ent->origin[i] + ent->model->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(ent->origin, ent->model->mins, mins);
		VectorAdd(ent->origin, ent->model->maxs, maxs);
	}

	for (i = 0; i < 3; i++)
		ent->center_origin[i] = ent->origin[i] + ((ent->model->mins[i] + ent->model->maxs[i]) / 2.0f);

	if (R_CullBox(mins, maxs))
	{
		return; // not in frustum, reject
	}

	// mark entitiy as visible this frame
	ent->visibleFrame = r_framecount;

	R_MatrixForEntity(ent);

	// mark which dynamic lights impact this model (not the best)
	if (!r_fullbright->value)
	{
	}

}
/*
================
R_TraverseWorldBSP

Traverse BSP tree and build texture chains for efficient rendering.
!!ASSUMES WE HAVE ONLY ONE SHADOW CASTER AT MAX!!
================
*/

void R_TraverseWorldBSP()
{
	if (!r_drawworld->value)
		return;

	if (r_newrefdef.view.flags & RDF_NOWORLDMODEL)
		return;

	if (gl_state.bTraversedBSP)
		return;

	// moved here from fix vis problems 
	VectorCopy(r_newrefdef.view.origin, modelorg); 

	// clear skybox
	R_ClearSkyBox();


	gl_state.bTraversedBSP = true;
}

/*
================
R_InitWorldEntity
================
*/
static void R_InitWorldEntity()
{
	memset(&r_worldent, 0, sizeof(r_worldent));
	memcpy(r_worldent.modelMatrix, mat4_identity, sizeof(mat4_t)); // no transform needed for world.
	
	r_worldent.model = r_worldmodel;
	r_worldent.frame = (int)(r_newrefdef.time * 2);
	r_worldent.alpha = 1.0f;
	
	VectorSet(r_worldent.renderColor, 1.0f, 1.0f, 1.0f);
}

/*
================
R_DrawWorld

Draws world and skybox
================
*/
void R_DrawWorld()
{
	if (!r_drawworld->value)
		return;

	if (r_newrefdef.view.flags & RDF_NOWORLDMODEL)
		return;

	if (!gl_state.bTraversedBSP)
	{
		ri.Error(ERR_FATAL, "R_DrawWorld without traversal of BSP");
	}

	R_InitWorldEntity();
	r_pCurrentEntity = &r_worldent;
	r_pCurrentModel = r_worldmodel;

	VectorCopy(r_newrefdef.view.origin, modelorg);

	R_DrawQ3World();

	// 
	// DRAW SKYBOX but only in final pass
	//
	if (gl_state.bShadowMapPass == false)
	{
		R_DrawSkyBox();
	}
}
