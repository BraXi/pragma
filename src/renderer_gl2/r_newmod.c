/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_newmod.c -- skeletal model and animation loading

#include "r_local.h"

#define SMDL_TEST 1 // DIRTY EXPERIMENTAL STUFF

extern int modelFileLength;
extern model_t* pLoadModel;

extern vec3_t	model_shadevector;
extern float	model_shadelight[3];
extern qboolean r_pendingflip;

#if defined(SMDL_TEST)
#define MAX_BONES 128 // Must match GLSL
static mat4_t bones_mat[MAX_BONES];

smdl_data_t* anim = NULL;
GLuint skel_vbo = 0;
#endif

/*
=================
R_UploadSkelModel
=================
*/
static void R_UploadSkelModel(model_t* mod)
{
	smdl_data_t* smdl = mod->smdl;

#if defined(SMDL_TEST)
	int bufsize = sizeof(smdl_vert_t) * smdl->hdr.numverts;

	glGenBuffers(1, &skel_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, skel_vbo);
	glBufferData(GL_ARRAY_BUFFER, bufsize, &smdl->verts[0].xyz[0], GL_STATIC_DRAW);

	// check if its all right
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufsize);
	if (bufsize != bufsize)
	{
		ri.Error(ERR_FATAL, "failed to allocate vertex buffer for smdl\n");
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
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

#endif
}

/*
=================
R_TempDrawSkelModel
=================
*/
#if defined(SMDL_TEST)

static void R_BeginSkelRendering()
{
	int attrib;

	R_BindProgram(GLPROG_SMDL);

	glBindBuffer(GL_ARRAY_BUFFER, skel_vbo);

	attrib = R_GetProgAttribLoc(VALOC_POS);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(smdl_vert_t), NULL);
	}

	attrib = R_GetProgAttribLoc(VALOC_NORMAL);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(smdl_vert_t), (void*)offsetof(smdl_vert_t, normal));
	}

	attrib = R_GetProgAttribLoc(VALOC_TEXCOORD);
	if (attrib != -1)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_FALSE, sizeof(smdl_vert_t), (void*)offsetof(smdl_vert_t, uv));
	}

	attrib = R_GetProgAttribLoc(VALOC_BONEID);
	if (attrib != -1)
	{
		// THIS IS ONLY A FLOAT BECAUSE SILLY OPENGL 2.1 CANNOT HAVE INT ATTRIBUTES
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 1, GL_FLOAT, GL_FALSE, sizeof(smdl_vert_t), (void*)offsetof(smdl_vert_t, bone));
	}
	else
		ri.Error(ERR_FATAL, "!VALOC_BONEID\n");


	R_ProgUniformVec3(LOC_AMBIENT_COLOR, model_shadelight);
	R_ProgUniformVec3(LOC_AMBIENT_DIR, model_shadevector);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);

	R_ProgUniform1f(LOC_PARM0, (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT) ? 1.0f : 0.0f);
	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, (pCurrentRefEnt->renderfx & RF_TRANSLUCENT) ? pCurrentRefEnt->alpha : 1.0f);

	if (r_pendingflip)
	{
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (pCurrentRefEnt->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(true);
}


static void R_EndSkelRendering()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (r_pendingflip)
	{
		Mat4Scale(r_projection_matrix, -1, 1, 1);
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (pCurrentRefEnt->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(false);

	R_UnbindProgram(GLPROG_SMDL);
}

void R_DrawSkelModel(rentity_t* ent)
{
	smdl_data_t* mod;
	smdl_surf_t* surf;
	int i;

	if (ent == NULL || ent->model == NULL || ent->model->smdl == NULL)
	{
		ri.Printf(PRINT_LOW, "R_DrawSkelModel: ent has no model\n");
		return;
	}

	mod = ent->model->smdl;

	R_BeginSkelRendering();

	for (i = 0; i < MAX_BONES; i++)
		Mat4MakeIdentity(bones_mat[i]);

	R_ProgUniformMatrix4fv(LOC_BONES, MAX_BONES, bones_mat[0]);

	glDisable(GL_CULL_FACE); // FIXME: cull order is BACK for smds
	for (i = 0; i < mod->hdr.numsurfaces; i++)
	{
		//if ((ent->hiddenPartsBits & (1 << i)))
		//	continue; // surface is hidden

		surf = mod->surfaces[i];

		R_MultiTextureBind(TMU_DIFFUSE, surf->texnum);
		glDrawArrays(GL_TRIANGLES, surf->firstvert, surf->numverts);

		if (r_speeds->value)
			rperf.alias_tris += surf->numverts / 3;
	}
	glEnable(GL_CULL_FACE);

	R_EndSkelRendering();

	if (r_speeds->value)
		rperf.alias_drawcalls++;
}

#else
void R_DrawSkelModel(rentity_t* ent)
{
	smdl_data_t* mod;
	smdl_surf_t* surf;
	int i;


	if (ent == NULL || ent->model == NULL || ent->model->smdl == NULL || !ent->model->vb[0])
	{
		ri.Printf(PRINT_LOW, "R_DrawSkelModel: ent has no model\n");
		return;
	}

	mod = ent->model->smdl;

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

	R_ProgUniformVec3(LOC_AMBIENT_COLOR, model_shadelight);
	R_ProgUniformVec3(LOC_AMBIENT_DIR, model_shadevector);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);

	R_ProgUniform1f(LOC_PARM0, (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT) ? 1.0f : 0.0f);
	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, (ent->renderfx & RF_TRANSLUCENT) ? ent->alpha : 1.0f);

	if (r_pendingflip)
	{
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (ent->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(true);

	glDisable(GL_CULL_FACE); // FIXME: cull order is BACK for smds
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

	if (ent->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(false);

	if (r_speeds->value)
		rperf.alias_drawcalls++;
}
#endif



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

	mod->extradata = ri.Glob_HunkBegin(1024*1024, "Skeletal Model (Renderer/EXE)");
	mod->smdl = ri.Glob_HunkAlloc(sizeof(smdl_data_t));

	loaded = ri.LoadAnimOrModel(SMDL_MODEL, mod->smdl, pLoadModel->name, modelFileLength, buffer);

	if (!loaded)
	{
		ri.Glob_HunkFree(mod->extradata);
		mod->extradata = NULL;
		//ri.Printf(PRINT_LOW, "Warning: failed to load model '%s'.\n", mod->name);
		return;
	}

	surf = mod->smdl->surfaces[0];
	for (int i = 0; i < mod->smdl->hdr.numsurfaces; i++)
	{
		if (surf->texture[0] == '$') // CODE TEXTURE
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

	mod->extradatasize = ri.Glob_HunkEnd();
	mod->type = MOD_SKEL;
	mod->numframes = 1;

	VectorCopy(mod->smdl->hdr.mins, mod->mins);
	VectorCopy(mod->smdl->hdr.maxs, mod->maxs);
	mod->radius = mod->smdl->hdr.boundingradius;

	R_UploadSkelModel(mod);

#if defined(SMDL_TEST)
	// load up animation
	unsigned* buf;
	int flen = ri.LoadFile("modelanims/infected_idle.smd", &buf);

	if (!buf)
	{
		ri.Error(ERR_FATAL, "!buf\n");
	}

	void *a = ri.Glob_HunkBegin(1024 * 1024, "anim");
	anim = ri.Glob_HunkAlloc(sizeof(smdl_data_t));
	loaded = ri.LoadAnimOrModel(SMDL_ANIMATION, anim, "infected_idle", flen, buf);

	if (!loaded)
	{
		ri.Glob_HunkFree(a);
		ri.Error(ERR_FATAL, "!loaded\n");
		return;
	}

	mod->numframes = anim->hdr.numframes;
	ri.Printf(PRINT_LOW, "mod %s with %i frames\n", mod->name, mod->numframes);
#endif
}







