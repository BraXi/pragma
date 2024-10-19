/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

extern renderWorld_t world;
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

	glBindBuffer(GL_ARRAY_BUFFER, world.vbo_verts);

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

	r_worldent.model = r_worldmodel;
	r_worldent.frame = (int)(r_newrefdef.time * 2);
	r_worldent.alpha = 1.0f;

	VectorSet(r_worldent.renderColor, 1.0f, 1.0f, 1.0f);
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
R_DrawWorld
Draw world and skybox
================
*/
void R_DrawWorld()
{
	worldSurface_t* surf;
	worldSurf_Face_t* face;
	worldSurf_Mesh_t* mesh;
	GLuint* pIndexes;
	unsigned int  numIndexes;
	int i, lightmap;
	int material;

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.view.flags & RDF_NOWORLDMODEL)
		return;

	if (!gl_state.bTraversedBSP)
	{
		ri.Error(ERR_FATAL, "R_DrawWorld without traversal of BSP");
	}

	if (!world.bLoaded)
	{
		return;
	}

	R_InitWorldEntity();
	r_pCurrentEntity = &r_worldent;
	r_pCurrentModel = r_worldmodel;

	VectorCopy(r_newrefdef.view.origin, modelorg);

	R_BindProgram(GLPROG_Q3WORLD);
	R_BindWorldDrawVertsBuffer();
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_worldent.modelMatrix);
	R_ProgUniformMatrix4fv(LOC_MODELVIEW, 1, r_world_matrix);

	R_MultiTextureBind(TMU_DIFFUSE, r_texture_white->texnum);

	pIndexes = NULL;
	numIndexes = 0;
	lightmap = 0;

	//glDisable(GL_CULL_FACE);
	surf = world.surfaces;
	for (i = 0; i < world.numSurfaces; i++, surf++)
	{
		pIndexes = NULL;
		numIndexes = 0;
		material = surf->material;
		if (surf->surfaceType == WORLDSURF_FACE)
		{
			face = (worldSurf_Face_t*)surf->data;

			lightmap = face->lightmap;
			numIndexes = face->numIndexes;
			pIndexes = face->indices;
		}
		else if (surf->surfaceType == WORLDSURF_MESH)
		{
			mesh = (worldSurf_Mesh_t*)surf->data;
			lightmap = mesh->lightmap;
			numIndexes = mesh->numIndexes;
			pIndexes = mesh->indices;
		}
		else if (surf->surfaceType == WORLDSURF_PATCH)
		{
		}

		if (numIndexes == 0)
			continue; // nothing to draw

		R_MultiTextureBind(TMU_DIFFUSE, r_materials[material].diffuse_id);


		if (lightmap >= 0)
		{
			R_MultiTextureBind(TMU_LIGHTMAP, world.lightmaps[lightmap]->texnum);
			R_ProgUniform1f(LOC_PARM0, 0.0f);
		}
		else
		{
			R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);
			R_ProgUniform1f(LOC_PARM0, 1.0f);
		}

		rperf.brush_drawcalls++;
		rperf.brush_tris += numIndexes / 3;
		glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, pIndexes);
	}

	R_UnbindProgram();
	R_UnbindWorldDrawVertsBuffer();

	// 
	// DRAW SKYBOX but only in final pass
	//
	if (gl_state.bShadowMapPass == false)
	{
		R_DrawSkyBox();
	}
}