/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_newmod.c -- skeletal model and animation loading

#include "r_local.h"


extern int modelFileLength;
extern model_t* pLoadModel;

extern vec3_t	model_shadevector;
extern float	model_shadelight[3];
extern qboolean r_pendingflip;


/*
=================
R_UploadSkelModel
=================
*/
static void R_UploadSkelModel(model_t* mod)
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
	smdl_data_t* mod;
	smdl_surf_t* surf;
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


/*
=================
Mod_LoadSkelModel
=================
*/
void Mod_LoadSkelModel(model_t* mod, void* buffer, lod_t lod)
{
	qboolean loaded = false;
	smdl_surf_t* surf;
	char texturename[MAX_QPATH];

	mod->extradata = Hunk_Begin(1024*1024, "skeletal model");
	mod->smdl = Hunk_Alloc(sizeof(smdl_data_t));

	loaded = ri.LoadAnimOrModel(SMDL_MODEL, mod->smdl, pLoadModel->name, modelFileLength, buffer);

	if (!loaded)
	{
		Hunk_Free(mod->extradata);
		mod->extradata = NULL;
		ri.Printf(PRINT_LOW, "Loading model '%s' failed.\n", mod->name);
		return;
	}

	surf = mod->smdl->surfaces[0];
	for (int i = 0; i < mod->smdl->hdr.numsurfaces; i++)
	{
		if (surf->texture[0] == '$')
		{
			mod->images[i] = R_FindTexture(surf->texture, it_model, true);
		}
		else
		{
			Com_sprintf(texturename, sizeof(texturename), "modelskins/%s", surf->texture);
			mod->images[i] = R_FindTexture(texturename, it_model, true);
		}

		if (!mod->images[i])
			mod->images[i] = r_texture_missing;

		surf->texnum = mod->images[i]->texnum;
	}

	mod->type = MOD_SKEL;
	mod->numframes = 1;

	VectorCopy(mod->smdl->hdr.mins, mod->mins);
	VectorCopy(mod->smdl->hdr.maxs, mod->maxs);
	mod->radius = mod->smdl->hdr.boundingradius;

	R_UploadSkelModel(mod);

}

#define MAX_ANIMATIONS 128
static smdl_anim_t animsArray[MAX_ANIMATIONS];
static unsigned int animsCount = 0;

/*
=================
AnimationForName
=================
*/
smdl_anim_t* AnimationForName(char *name, qboolean crash)
{
	smdl_anim_t	*anim;
	unsigned	*buf;
	int			i, fileLen;
	qboolean	loaded;
	char		filename[MAX_QPATH];

	if (!name[0])
		ri.Error(ERR_DROP, "%s: NULL name", __FUNCTION__);

	for (i = 0, anim = animsArray; i < animsCount; i++, anim++)
	{
		if (!anim->name[0])
			continue;

		if (!strcmp(anim->name, name))
			return anim;
	}

	for (i = 0, anim = animsArray; i < animsCount; i++, anim++)
	{
		if (!anim->name[0])
			break;
	}

	if (i == animsCount)
	{
		if (animsCount == MAX_ANIMATIONS)
			ri.Error(ERR_DROP, "%s: hit limit of %d animations", __FUNCTION__, MAX_ANIMATIONS);
		animsCount++;
	}

	Com_sprintf(filename, sizeof(filename), "modelanims/%s.smd", name);

	fileLen = ri.LoadFile(filename, &buf);
	if (!buf)
	{
		if (crash)
			ri.Error(ERR_DROP, "%s: %s not found", __FUNCTION__, anim->name);

		memset(anim->name, 0, sizeof(anim->name));
		return NULL;
	}

	anim->extradata = Hunk_Begin(1024 * 256, "animation"); // 256kb should be more than plenty?

	loaded = ri.LoadAnimOrModel(SMDL_MODEL, &anim->data, name, modelFileLength, buf);

	if (!loaded)
	{
		ri.Printf(PRINT_LOW, "Loading animation '%s' failed.\n", anim->name);
		Hunk_Free(anim->extradata);
		ri.FreeFile(buf);
		memset(&anim, 0, sizeof(anim));
		return NULL;
	}

	ri.FreeFile(buf);

	anim->extradatasize = Hunk_End();
	strncpy(anim->name, name, MAX_QPATH);

	return anim;
}




