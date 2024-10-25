/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

extern material_t* r_materials;
static vec3_t		modelorg; // relative to viewpoint

/*
=================
R_BindWorldDrawVertsBuffer
=================
*/
static void R_BindWorldDrawVertsBuffer()
{
	static const dv_size = sizeof(worldDrawVert_t);
	int attrib_xyz, attrib_normal, attrib_tc, attrib_lmtc, attrib_color;

	glBindBuffer(GL_ARRAY_BUFFER, r_world->vbo_verts);

	// diferent program may have diferent atrributes and their locations
	attrib_xyz = R_GetProgAttribLoc(VALOC_POS);
	attrib_normal = R_GetProgAttribLoc(VALOC_NORMAL);
	attrib_tc = R_GetProgAttribLoc(VALOC_TEXCOORD);
	attrib_lmtc = R_GetProgAttribLoc(VALOC_LMTEXCOORD);
	attrib_color = R_GetProgAttribLoc(VALOC_COLOR);

	if (attrib_xyz == -1)
	{
		ri.Error(ERR_DROP, "no vertex position attribute in renderprogram.");
	}

	// vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(attrib_xyz, 3, GL_FLOAT, GL_FALSE, dv_size, (void*)0);

	// diffuse coords
	if (attrib_tc != -1)
	{
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(attrib_tc, 2, GL_FLOAT, GL_FALSE, dv_size, (void*)offsetof(worldDrawVert_t, st));
	}

	// lightmap coords
	if (attrib_lmtc != -1)
	{
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(attrib_lmtc, 2, GL_FLOAT, GL_FALSE, dv_size, (void*)offsetof(worldDrawVert_t, lightmap));
	}

	// vertex normals
	if (attrib_normal != -1)
	{
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(attrib_normal, 3, GL_FLOAT, GL_FALSE, dv_size, (void*)offsetof(worldDrawVert_t, normal));
	}

	// vertex colors for vertex lighting
	if (attrib_color != -1)
	{
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(attrib_color, 4, GL_FLOAT, GL_FALSE, dv_size, (void*)offsetof(worldDrawVert_t, color));
	}
}

/*
=================
R_UnbindWorldDrawVertsBuffer
=================
*/
static void R_UnbindWorldDrawVertsBuffer()
{
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
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

	r_worldent.model = NULL;
	r_worldent.frame = (int)(r_newrefdef.time * 2);
	r_worldent.alpha = 1.0f;

	VectorSet(r_worldent.renderColor, 1.0f, 1.0f, 1.0f);

	r_pCurrentEntity = &r_worldent;
	r_pCurrentModel = NULL;
}


/*
=================
R_DrawBrushModel

If bTransparentPass is true, only transparent bits will be drawn
=================
*/
void R_DrawBrushModel(rentity_t* ent)
{

}


/*
=================
R_PreprocessBrushModelEntity
Checks if entity is visible this frame and marks which dynamic lights impact this entity
=================
*/
void R_PreprocessBrushModelEntity(rentity_t* ent)
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
R_SetLightMap
================
*/
static void R_SetLightMap(const worldLightMap_t lightmap_index)
{
	qboolean useVertexColors;
	worldLightMap_t lightmap;

	useVertexColors = false;

	if (r_fullbright->value)
	{
		lightmap = LIGHTMAP_WHITEIMAGE;
		goto change_lightmap; // Sneak in GOTO so I can be called evil
	}

	lightmap = lightmap_index;

	if (lightmap >= LIGHTMAP_LIGHTMAP)
	{
		// use lightmap image	
		//useVertexColors = false; // implicit
	}
	else
	{
		switch (lightmap)
		{
		case LIGHTMAP_NONE:
		case LIGHTMAP_WHITEIMAGE:
			// fullbright
			useVertexColors = false;
			break;

		case LIGHTMAP_BY_VERTEX:
			// vertex colors
			useVertexColors = true;
			break;
		}
	}

change_lightmap:
	if (lightmap >= LIGHTMAP_LIGHTMAP)
	{
		if (lightmap >= r_world->numLightmaps)
		{
			ri.Error(ERR_DROP, __FUNCTION__": Wrong lightmap %i.\n", lightmap);
			return;
		}

		R_MultiTextureBind(TMU_LIGHTMAP, r_world->lightmaps[lightmap]->texnum);		
	}
	else
	{
		R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);
	}

	if (useVertexColors)
		R_ProgUniform1f(LOC_PARM0, 1.0f);
	else
		R_ProgUniform1f(LOC_PARM0, 0.0f);	
}

/*
================
R_BeginRenderingWorld
================
*/
static void R_BeginRenderingWorld()
{
	if (gl_state.bRenderingWorldModel)
		return;

	gl_state.bRenderingWorldModel = true;
	R_BindProgram(GLPROG_Q3WORLD);
	R_BindWorldDrawVertsBuffer();
}

/*
================
R_EndRenderingWorld
================
*/
static void R_EndRenderingWorld()
{
	if (!gl_state.bRenderingWorldModel)
		return;

	gl_state.bRenderingWorldModel = false;
	R_UnbindProgram();
	R_UnbindWorldDrawVertsBuffer();
}

/*
================
R_IsWorldSurfaceDrawable
================
*/
qboolean R_IsWorldSurfaceDrawable(const worldSurface_t* surf)
{
	int surfaceFlags;

	surfaceFlags = r_world->materials[surf->material_id].surfaceFlags;

	if (surfaceFlags & SURF_SKIP || surfaceFlags & SURF_NODRAW)
	{
		rperf.brush_nodraw++;
		return false;
	}

	return true;
}


/*
================
R_DrawBModel
Draw brush model
================
*/
void R_DrawBModel(const int bmodel_index)
{
	worldSurface_t* surf;
	GLuint* pDrawIndexes;
	unsigned int numIndexes, numSurfaces;
	int material_id, i;
	worldLightMap_t lightmap_id;

#ifdef _DEBUG
	if (!r_world)
	{
		ri.Error(ERR_DROP, __FUNCTION__": no world.\n");
		return;
	}

	if (!gl_state.bRenderingWorldModel)
	{
		ri.Error(ERR_DROP, __FUNCTION__": not in rendering world stage.\n");
		return;
	}
#endif

	if (bmodel_index < 0 || bmodel_index >= r_world->numInlineModels)
	{
		ri.Error(ERR_DROP, __FUNCTION__": bad index %i.\n", bmodel_index);
		return;
	}

	surf = &r_world->surfaces[r_world->inlineModels[bmodel_index].firstSurface];
	numSurfaces = r_world->inlineModels[bmodel_index].numSurfaces;

	for (i = 0; i < numSurfaces; i++, surf++)
	{
		if (surf->surfaceType != WORLDSURF_FACE && surf->surfaceType != WORLDSURF_MESH)
			continue;

		if (!R_IsWorldSurfaceDrawable(surf))
			continue;

		material_id = surf->material_id;
		lightmap_id = surf->lightmap_id;
		numIndexes = surf->numIndexes;
		pDrawIndexes = surf->drawIndexes;

		//ri.Printf(PRINT_ALL, "%4i: %iM, %iL\n", bmodel_index, material_id, lightmap_id);
		if (numIndexes == 0)
			continue;

		R_MultiTextureBind(TMU_DIFFUSE, r_materials[material_id].diffuse_id);
		R_SetLightMap(lightmap_id);

		glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, pDrawIndexes);

		rperf.brush_drawcalls++;
		rperf.brush_tris += numIndexes / 3;
	}
}

/*
================
R_DrawWorld
Draw world and skybox
================
*/
void R_DrawWorld()
{
	int i;

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.view.flags & RDF_NOWORLDMODEL)
		return;

	if (!gl_state.bTraversedBSP)
	{
		ri.Error(ERR_FATAL, __FUNCTION__": BSP tree not traversed");
	}

	if (!r_world)
	{
		return;
	}

	R_InitWorldEntity();

	VectorCopy(r_newrefdef.view.origin, modelorg);

	R_BeginRenderingWorld();

	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_worldent.modelMatrix);
	R_ProgUniformMatrix4fv(LOC_MODELVIEW, 1, r_world_matrix);


	for(i = 0; i < r_world->numInlineModels; i++)
		R_DrawBModel(i);

	R_EndRenderingWorld();

	// 
	// DRAW SKYBOX but only in final pass
	//
	if (gl_state.bShadowMapPass == false)
	{
		R_DrawSkyBox();
	}
}