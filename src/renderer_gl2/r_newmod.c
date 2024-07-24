/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_newmod.c -- skeletal model and animation loading

#include "r_local.h"

/*
notes
-----
sequence (smdl_seq_t) has all the bones and their translation/rotation for a keyframe

each sequence[] is an array of sizeof(smdl_seq_t) * smdl->hdr->numbones

model will have only one sequence which is a reference pose!
reference pose is: mod->smdl->seqences[0]

an anim will have anim->hdr.numframes sequences
each anim key frame is: anim->smdl->seqences[]

in short - sequence contains all the bones for a given frame
*/

#define SMDL_TEST 1 // DIRTY EXPERIMENTAL STUFF

extern int modelFileLength;
extern model_t* pLoadModel;

extern vec3_t	model_shadevector;
extern float	model_shadelight[3];
extern qboolean r_pendingflip;

#if defined(SMDL_TEST)
#define MAX_BONES 128 // Must match GLSL

//typedef vec_t mat4x4_t[4][4];
static mat4_t FinalBonesMat[MAX_BONES];

static mat4_t InvBindPoseMat[MAX_BONES];

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
R_DrawSkelModel
=================
*/
#if defined(SMDL_TEST)


void multiplyMatrices(float result[16], const float a[16], const float b[16]) 
{
	int i, j;
	for (i = 0; i < 4; ++i) 
	{
		for (j = 0; j < 4; ++j) 
		{
			result[i + j * 4] = a[i] * b[j * 4] + a[i + 4] * b[j * 4 + 1] + a[i + 8] * b[j * 4 + 2] + a[i + 12] * b[j * 4 + 3];
		}
	}
}



void UpdateBoneMatrix(smdl_seq_t* bone, qboolean bNormalizeQuat) 
{
	smdl_bone_t* boneinfo;
	int i, j, k;
	int parent, self;
	mat4_t tempMatrix;

	self = bone->bone;
	boneinfo = pCurrentModel->smdl->bones[self];
	parent = boneinfo->parent;

	if (bNormalizeQuat)
	{
		// quaternion must be normalized before conversion to matrix
		Quat_Normalize(&bone->q);
	}

	//Mat4MakeIdentity(tempMatrix);

	// now create matrix from quaternion
	Quat_ToMat4(bone->q, tempMatrix);

	tempMatrix[12] = bone->pos[0];
	tempMatrix[13] = bone->pos[1];
	tempMatrix[14] = bone->pos[2];

	if (parent == -1)
	{
		for (i = 0; i < 16; i++)
		{
			FinalBonesMat[self][i] = tempMatrix[i];
		}
	}
	else
	{
		float result[16] = { 0 };

		for (i = 0; i < 4; i++) 
		{
			for (j = 0; j < 4; j++) 
			{
				for (k = 0; k < 4; k++) 
				{
					result[i + j * 4] += FinalBonesMat[parent][i + k * 4] * tempMatrix[k + j * 4];
				}
			}
		}
		for (i = 0; i < 16; i++) 
		{
			FinalBonesMat[self][i] = result[i];
		}
	}

}

static void R_SkeletonForFrame(int frame)
{
	smdl_data_t* model;
	smdl_seq_t* ref; // reference skeleton
	smdl_seq_t* seq; // bones pos/rot in current frame
	smdl_bone_t* bone; // seq->bone

	int i, j;

	cvar_t* fr = ri.Cvar_Get("frame", "0", 0, "");
	int framee = fr->value;

	if (framee >= pCurrentModel->smdl->hdr.numframes)
		frame = 0;

	model = pCurrentRefEnt->model->smdl;
	ref = model->seqences[0];
	seq = anim->seqences[framee];

	static qboolean awkay = false;

	//if (awkay)
	//	return;

	//awkay = true;
	// 
	// identity bones for this model
	for (i = 0; i < model->hdr.numbones; i++)
	{
		Mat4MakeIdentity(FinalBonesMat[i]);
	}


	for (i = 0; i < model->hdr.numbones; i++)
	{
		UpdateBoneMatrix(seq, false);
		seq++;
	}

	for (i = 0; i < model->hdr.numbones; i++)
		Mat4Multiply(FinalBonesMat[i], InvBindPoseMat[i]);
}

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
		glVertexAttribPointer(attrib, 1, GL_INT, GL_FALSE, sizeof(smdl_vert_t), (void*)offsetof(smdl_vert_t, bone));
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

#if 0
	static qboolean ident = false;

	if (!ident)
	{
		for (i = 0; i < MAX_BONES; i++)
			Mat4MakeIdentity(bones_mat[i]);
		ident = true;
		ri.Printf(PRINT_LOW, "ident!\n");
	}

	float x = r_newrefdef.time * 0.1;

	for (i = 10; i < MAX_BONES; i++)
	{
		if (i > 20)
		{
			Mat4RotateAroundX(bones_mat[i], x);
			Mat4RotateAroundY(bones_mat[i],x);
			Mat4RotateAroundZ(bones_mat[i], -x);
		}
		else
		{
			Mat4RotateAroundZ(bones_mat[i], x);
		}
	}
#endif

	R_SkeletonForFrame(1);

	R_ProgUniformMatrix4fv(LOC_BONES, MAX_BONES, FinalBonesMat[0]);

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


void CreateInverseBindPoseMatrix(smdl_data_t* model, smdl_seq_t* bone, qboolean bNormalizeQuat)
{
	smdl_bone_t* boneinfo;
	int i, j, k;
	int parent, self;
	mat4_t tempMatrix;

	self = bone->bone;
	boneinfo = model->bones[self];
	parent = boneinfo->parent;

	if (bNormalizeQuat)
	{
		// quaternion must be normalized before conversion to matrix
		Quat_Normalize(&bone->q);
	}

	Mat4MakeIdentity(tempMatrix);

	// now create matrix from quaternion
	Quat_ToMat4(bone->q, tempMatrix);

	tempMatrix[12] = bone->pos[0];
	tempMatrix[13] = bone->pos[1];
	tempMatrix[14] = bone->pos[2];

	if (parent == -1)
	{
		for (i = 0; i < 16; i++)
		{
			InvBindPoseMat[self][i] = tempMatrix[i];
		}
	}
	else
	{
		float result[16] = { 0 };

		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				for (k = 0; k < 4; k++)
				{
					result[i + j * 4] += InvBindPoseMat[parent][i + k * 4] * tempMatrix[k + j * 4];
				}
			}
		}
		for (i = 0; i < 16; i++)
		{
			InvBindPoseMat[self][i] = result[i];
		}
	}
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


	smdl_data_t* model;
	smdl_seq_t* ref; // reference skeleton
	smdl_seq_t* seq; // bones pos/rot in current frame
	smdl_bone_t* bone; // seq->bone

	int j, frame = 0;

	model = mod->smdl;
	ref = model->seqences[0];

	//seq = ref; // anim->seqences[frame];

	for (frame = -1; frame < anim->hdr.numframes; frame++)
	{
		if (frame == -1)
			seq = model->seqences[0]; // reference pose
		else
			seq = anim->seqences[frame]; // animation frame

		ri.Printf(PRINT_LOW, "---- frame %i ----\n", frame);
		for (j = 0; j < mod->smdl->hdr.numbones; j++)
		{
			bone = model->bones[j];

			Quat_FromAngles(seq->rot, &seq->q);
			Quat_Normalize(&seq->q);

			if (frame == -1)
			{
				CreateInverseBindPoseMatrix(model, seq, false);
				//Mat4Invert(InvBindPoseMat[j], InvBindPoseMat[j]);
			}
			
			ri.Printf(PRINT_LOW, "%i '%s' %i: [%.2f %.2f %.2f] [%.3f %.3f %.3f]\n", seq->bone, bone->name, bone->parent, seq->pos[0], seq->pos[1], seq->pos[2], seq->rot[0], seq->rot[1], seq->rot[2]);
			seq++;
		}
	}

	for (j = 0; j < mod->smdl->hdr.numbones; j++)
		Mat4Invert(InvBindPoseMat[j], InvBindPoseMat[j]);

#endif
}







