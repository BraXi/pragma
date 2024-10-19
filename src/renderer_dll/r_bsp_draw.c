/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

extern renderWorld_t world;

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

extern material_t* r_materials;
/*
=================
R_DrawQ3World
=================
*/
qboolean R_DrawQ3World()
{
	worldSurface_t* surf;
	worldSurf_Face_t* face;
	worldSurf_Mesh_t* mesh;
	GLuint* pIndexes;
	unsigned int  numIndexes;
	int i, lightmap;
	int material;

	if (!world.bLoaded)
		return false;

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

		glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, pIndexes);
	}

	R_UnbindProgram();
	R_UnbindWorldDrawVertsBuffer();
	return true;
}