/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
notes
-----
SMDL_MAX_BONES must match MAX_BONES in glsl

sequence (smdl_seq_t) has all the bones and their translation/rotation for a keyframe

each sequence[] is an array of sizeof(smdl_seq_t) * smdl->hdr->numbones

model will have only one sequence which is a reference pose!
reference pose is: mod->smdl->seqences[0]

an anim will have anim->hdr.numframes sequences
each anim key frame is: anim->smdl->seqences[]

in short - sequence contains all the bones for a given frame
*/

#include "r_local.h"

// common to all models
extern int modelFileLength;
extern model_t* pLoadModel;

extern vec3_t	model_shadevector;
extern float	model_shadelight[3];
extern qboolean r_pendingflip;

static mat4_t FinalBonesMat[SMDL_MAX_BONES];

smdl_data_t* anim = NULL;



// MOVE THESE TWO TO COM_MODEL.C

/*
=================
CalcBoneMatrix
Calculate matrix for a given bone frame and store result in bones matrix array
=================
*/
void CalcBoneMatrix(const smdl_data_t* pModelData, smdl_seq_t* pBone, qboolean bNormalizeQuat, mat4_t* pMatrix)
{
	smdl_bone_t* boneinfo;
	int i, j, k;
	int parent, self;
	mat4_t tempMatrix;

	self = pBone->bone;
	boneinfo = pModelData->bones[self];
	parent = boneinfo->parent;

	if (bNormalizeQuat)
	{
		// quaternion must be normalized before conversion to matrix its 
		// normalized for every exact frame, but not for "inbetweens" during slerp
		Quat_Normalize(&pBone->q);
	}

	//Mat4MakeIdentity(tempMatrix);

	// create matrix from quaternion
	Quat_ToMat4(pBone->q, tempMatrix);

	tempMatrix[12] = pBone->pos[0];
	tempMatrix[13] = pBone->pos[1];
	tempMatrix[14] = pBone->pos[2];

	if (parent == -1)
	{
		memcpy(pMatrix[self], tempMatrix, sizeof(mat4_t));
	}
	else
	{
		memset(pMatrix[self], 0, sizeof(mat4_t));
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				for (k = 0; k < 4; k++)
				{
					pMatrix[self][i + j * 4] += pMatrix[parent][i + k * 4] * tempMatrix[k + j * 4];
				}
			}
		}
	}
}

/*
=================
CalcInverseMatrixForModel
Calculate and invert matrices for bind pose skeleton
=================
*/
static void CalcInverseMatrixForModel(smdl_data_t* pModelData)
{
	smdl_seq_t* pSeq;
	mat4_t* pInvBindBonePatrix;
	int i;

	pInvBindBonePatrix = pModelData->invBindPoseMat;

	// make identity matrix for MAX_BONES in case
	// animation has more bones than the model
	for (i = 0; i < SMDL_MAX_BONES; i++)
		Mat4MakeIdentity(pInvBindBonePatrix[i]);

	pSeq = pModelData->seqences[0]; // reference skeleton
	for (i = 0; i < pModelData->hdr.numbones; i++, pSeq++)
	{
		Quat_FromAngles(pSeq->rot, &pSeq->q);
		CalcBoneMatrix(pModelData, pSeq, true, pInvBindBonePatrix);
	}

	for (i = 0; i < pModelData->hdr.numbones; i++)
		Mat4Invert(pInvBindBonePatrix[i], pInvBindBonePatrix[i]);
}

/*
=================
R_UploadSkelModelTris
=================
*/
static void R_UploadSkelModelTris(model_t* mod)
{
	int bufsize, ongpusize;

	if (!mod->smdl)
	{
		ri.Error(ERR_FATAL, "%s: !mod->smdl\n", __FUNCTION__);
		return;
	}

	if (mod->smdl->hdr.numverts <= 0)
	{
		ri.Error(ERR_FATAL, "%s: no verts!\n", __FUNCTION__);
		return;
	}

	mod->vb[0] = R_AllocVertexBuffer(0, 0, 0); // HAACK

	bufsize = sizeof(smdl_vert_t) * mod->smdl->hdr.numverts;

	glGenBuffers(1, &mod->vb[0]->vboBuf);
	glBindBuffer(GL_ARRAY_BUFFER, mod->vb[0]->vboBuf);
	glBufferData(GL_ARRAY_BUFFER, bufsize, &mod->smdl->verts[0].xyz[0], GL_STATIC_DRAW);

	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &ongpusize);
	if (bufsize != ongpusize)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		ri.Error(ERR_FATAL, "%s: VBO upload failed for %s\n", __FUNCTION__, mod->name);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
=================
R_SkeletonForFrame
=================
*/
static void R_SkeletonForFrame(int frame)
{
	smdl_data_t* pModelData;
	smdl_seq_t* seq; // bones pos/rot in current frame
	int i;

	cvar_t* fr = ri.Cvar_Get("frame", "0", 0, "");
	int framee = fr->value;

	pModelData = pCurrentRefEnt->model->smdl;
	seq = anim->seqences[framee];

	for (i = 0; i < pModelData->hdr.numbones; i++)
	{
		CalcBoneMatrix(pCurrentRefEnt->model->smdl, seq, false, FinalBonesMat);
		seq++;
	}

	for (i = 0; i < pModelData->hdr.numbones; i++)
		Mat4Multiply(FinalBonesMat[i], pModelData->invBindPoseMat[i]);
}

/*
=================
R_DrawSkelModel
=================
*/
void R_DrawSkelModel(rentity_t* ent)
{
	smdl_data_t* mod;
	smdl_surf_t* surf;
	int i;
	
	// i know these two are ugly :P
	// TODO: revisit this when materials can change programs
	static int va_attrib[4];
	static qboolean vaAtrribsChecked = false;

	if (ent == NULL || ent->model == NULL || ent->model->smdl == NULL)
	{
		ri.Printf(PRINT_LOW, "%s: entity has no model\n", __FUNCTION__);
		return;
	}

	mod = ent->model->smdl;

	if (ent->model->vb[0] == NULL || ent->model->vb[0]->vboBuf == 0)
	{
		ri.Printf(PRINT_LOW, "%s: mod has no vertex data\n", __FUNCTION__);
		return;
	}

	R_BindProgram(GLPROG_SMDL_ANIMATED);
	glBindBuffer(GL_ARRAY_BUFFER, ent->model->vb[0]->vboBuf);


	// FIXME: this should be done only once at startup and when program changes from GLPROG_SMDL_ANIMATED
	if (!vaAtrribsChecked)
	{
		va_attrib[0] = R_GetProgAttribLoc(VALOC_POS);
		va_attrib[1] = R_GetProgAttribLoc(VALOC_NORMAL);
		va_attrib[2] = R_GetProgAttribLoc(VALOC_TEXCOORD);
		va_attrib[3] = R_GetProgAttribLoc(VALOC_BONEID);
	
		for (i = 0; i < 4; i++)
		{
			if (va_attrib[i] == -1)
			{
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				R_UnbindProgram();
				ri.Error(ERR_FATAL, "%s: not drawing model %s - missing attribute %i in program %s\n", __FUNCTION__, ent->model->name, i, R_GetCurrentProgramName());
				return;
			}
		}
	}

	// 
	// setup common uniforms
	//
	R_ProgUniformVec3(LOC_AMBIENT_COLOR, model_shadelight);
	R_ProgUniformVec3(LOC_AMBIENT_DIR, model_shadevector);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);

	R_ProgUniform1f(LOC_PARM0, (r_fullbright->value || ent->renderfx & RF_FULLBRIGHT) ? 1.0f : 0.0f);
	R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, (ent->renderfx & RF_TRANSLUCENT) ? ent->alpha : 1.0f);

	if (r_pendingflip)
	{
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (ent->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(true);

	//
	// setup skeleton
	//
	R_SkeletonForFrame(ent->frame);
	R_ProgUniformMatrix4fv(LOC_BONES, SMDL_MAX_BONES, FinalBonesMat[0]);


	//
	// draw the model
	//
	glEnableVertexAttribArray(va_attrib[0]);
	glVertexAttribPointer(va_attrib[0], 3, GL_FLOAT, GL_FALSE, sizeof(smdl_vert_t), NULL);

	glEnableVertexAttribArray(va_attrib[1]);
	glVertexAttribPointer(va_attrib[1], 3, GL_FLOAT, GL_FALSE, sizeof(smdl_vert_t), (void*)offsetof(smdl_vert_t, normal));

	glEnableVertexAttribArray(va_attrib[2]);
	glVertexAttribPointer(va_attrib[2], 2, GL_FLOAT, GL_FALSE, sizeof(smdl_vert_t), (void*)offsetof(smdl_vert_t, uv));

	glEnableVertexAttribArray(va_attrib[3]);
	glVertexAttribPointer(va_attrib[3], 1, GL_INT, GL_FALSE, sizeof(smdl_vert_t), (void*)offsetof(smdl_vert_t, bone));

	glDisable(GL_CULL_FACE); // FIXME: cull order is BACK for skel models
	for (i = 0; i < mod->hdr.numsurfaces; i++)
	{
		if ((ent->hiddenPartsBits & (1 << i)))
			continue; // surface is hidden

		surf = mod->surfaces[i];

		R_MultiTextureBind(TMU_DIFFUSE, surf->texnum);
		glDrawArrays(GL_TRIANGLES, surf->firstvert, surf->numverts);

		if (r_speeds->value)
			rperf.alias_tris += surf->numverts / 3;
	}

	glDisableVertexAttribArray(va_attrib[0]);
	glDisableVertexAttribArray(va_attrib[1]);
	glDisableVertexAttribArray(va_attrib[2]);
	glDisableVertexAttribArray(va_attrib[3]);

	if (r_pendingflip)
	{
		Mat4Scale(r_projection_matrix, -1, 1, 1);
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (pCurrentRefEnt->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(false);

	glEnable(GL_CULL_FACE);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	R_UnbindProgram(GLPROG_SMDL_ANIMATED);

	if (r_speeds->value)
		rperf.alias_drawcalls++;
}

/*
=================
Mod_LoadSkelModel
=================
*/
void Mod_LoadSkelModel(model_t* mod, void* buffer)
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
		mod->smdl = NULL;
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

	R_UploadSkelModelTris(mod);

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


	smdl_data_t* model;
	smdl_seq_t* ref; // reference skeleton
	smdl_seq_t* seq; // bones pos/rot in current frame
	smdl_bone_t* bone; // seq->bone

	int j, frame = 0;

	model = mod->smdl;
	ref = model->seqences[0];

	for (frame = 0; frame < anim->hdr.numframes; frame++)
	{
		seq = anim->seqences[frame]; // animation frame

		ri.Printf(PRINT_LOW, "---- frame %i ----\n", frame);
		for (j = 0; j < mod->smdl->hdr.numbones; j++)
		{
			bone = model->bones[j];

			Quat_FromAngles(seq->rot, &seq->q);
			Quat_Normalize(&seq->q);

			ri.Printf(PRINT_LOW, "%i '%s' %i: [%.2f %.2f %.2f] [%.3f %.3f %.3f]\n", seq->bone, bone->name, bone->parent, seq->pos[0], seq->pos[1], seq->pos[2], seq->rot[0], seq->rot[1], seq->rot[2]);
			seq++;
		}
	}

	CalcInverseMatrixForModel(model);
}







