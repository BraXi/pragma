/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_smdl.c -- skeletal models and animations rendering


#include "r_local.h"

extern vec3_t	model_shadevector;
extern float	model_shadelight[3];
extern qboolean r_pendingflip;

/*
=================
R_UploadSkelModel
=================
*/
void R_UploadSkelModel(model_t* mod)
{
	smdl_data_t* smdl = mod->smdl;
	smdl_vert_t* v;
	int i;

	mod->vb[0] = R_AllocVertexBuffer((V_UV | V_NORMAL), smdl->hdr.numverts, 0);

	for (i = 0; i < smdl->hdr.numverts; i++)
	{
		v = &smdl->verts[i];

		VectorCopy(v->xyz, mod->vb[0]->verts[i].xyz);
		VectorCopy(v->normal, mod->vb[0]->verts[i].normal);
		Vector2Copy(v->uv, mod->vb[0]->verts[i].st);
	}

	R_UpdateVertexBuffer(mod->vb[0], mod->vb[0]->verts, smdl->hdr.numverts, (V_UV | V_NORMAL));
}

/*
=================
R_TempDrawSkelModel
=================
*/
void R_DrawSkelModel(rentity_t* ent)
{
	smdl_data_t	*mod;
	smdl_surf_t	*surf;
	int i;

	mod = ent->model->smdl;
	mod = ent->model->smdl;
	if (ent == NULL || mod == NULL || !ent->model->vb[0])
	{
		ri.Printf(PRINT_LOW, "R_DrawSkelModel: ent has no model\n");
		return;
	}

#if 0
	qboolean anythingToDraw = false;
	for (i = 0; i < mod->hdr.numsurfaces; i++)
	{
		if (!(ent->hiddenPartsBits & (1 << i)))
		{
			anythingToDraw = true;
			break;
		}
	}
	if (!anythingToDraw)
		return;
#endif

	R_BindProgram(GLPROG_SMDL);

	R_ProgUniformVec3(LOC_SHADECOLOR, model_shadelight);
	R_ProgUniformVec3(LOC_SHADEVECTOR, model_shadevector);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);

	R_ProgUniform1f(LOC_PARM0, (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT) ? 1.0f : 0.0f);
	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, (ent->renderfx & RF_TRANSLUCENT) ? ent->alpha : 1.0f);

	if (r_pendingflip)
	{
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}


	glDisable(GL_CULL_FACE); // cull order is BACK for smds
	for (i = 0; i < mod->hdr.numsurfaces; i++)
	{
		if ((ent->hiddenPartsBits & (1 << i)))
			continue; // surface is hidden

		surf = mod->surfaces[i];

		R_MultiTextureBind(TMU_DIFFUSE, surf->texnum);
		R_DrawVertexBuffer(ent->model->vb[0], surf->firstvert, surf->numverts);

		if (r_speeds->value)
			rperf.alias_tris += surf->numverts / 3;
	}
	glEnable(GL_CULL_FACE);

	if (r_pendingflip)
	{
		Mat4Scale(r_projection_matrix, -1, 1, 1);
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (r_speeds->value)
		rperf.alias_drawcalls++;

}