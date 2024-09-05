/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

// common to all models
extern int modelFileLength;
extern model_t* pLoadModel;

// shoud be part of rentity cache
mat4_t* finalBonesMat;

// should be part of com_model_t
mat4_t* invertedBonesMat;

static void CalcInverseMatrixForModel(model_t* pModel);

/*
=================
R_GetModelBonesPtr
Returns pointer to bones array of a model.
=================
*/
pmodel_bone_t* R_GetModelBonesPtr(const model_t* mod)
{
	if (!mod || mod->type != MOD_NEWFORMAT)
		return NULL;
	return (pmodel_bone_t*)((byte*)mod->newmod + mod->newmod->ofs_bones);
}

/*
=================
R_GetModelSkeletonPtr
Returns pointer to bone transformations for reference skeleton
=================
*/
panim_bonetrans_t* R_GetModelSkeletonPtr(const model_t* mod)
{
	if (!mod || mod->type != MOD_NEWFORMAT)
		return NULL;
	return (panim_bonetrans_t*)((byte*)mod->newmod + mod->newmod->ofs_skeleton);
}

/*
=================
R_GetModelVertexesPtr
Returns pointer to vertex array.
=================
*/
pmodel_vertex_t* R_GetModelVertexesPtr(const model_t* mod)
{
	if (!mod || mod->type != MOD_NEWFORMAT)
		return NULL;
	return (pmodel_vertex_t*)((byte*)mod->newmod + mod->newmod->ofs_vertexes);
}

/*
=================
R_GetModelSurfacesPtr
Returns pointer to surfaces
=================
*/
pmodel_surface_t* R_GetModelSurfacesPtr(const model_t* mod)
{
	if (!mod || mod->type != MOD_NEWFORMAT)
		return NULL;
	return (pmodel_surface_t*)((byte*)mod->newmod + mod->newmod->ofs_surfaces);
}

/*
=================
R_GetModelPartsPtr
Returns pointer to parts
=================
*/
pmodel_part_t* R_GetModelPartsPtr(const model_t* mod)
{
	if (!mod || mod->type != MOD_NEWFORMAT)
		return NULL;
	return (pmodel_part_t*)((byte*)mod->newmod + mod->newmod->ofs_parts);
}

int Mod_GetNumVerts(const model_t* mod)
{
	if (!mod || mod->type != MOD_NEWFORMAT)
		return 0;
	return (int)mod->newmod->numVertexes;
}


int Mod_GetNumBones(const model_t* mod)
{
	if (!mod || mod->type != MOD_NEWFORMAT)
		return 0;
	return (int)mod->newmod->numBones;
}



void Mod_LoadNewModelTextures(model_t* mod)
{
	int i;
	pmodel_surface_t* surf;
	image_t* tex;
	char texname[MAX_QPATH];

	if (!mod || mod->type != MOD_NEWFORMAT || mod->newmod == NULL)
		return;

	surf = R_GetModelSurfacesPtr(mod);
	for (i = 0; i < mod->newmod->numSurfaces; i++, surf++)
	{
		Com_sprintf(texname, sizeof(texname), "modelskins/%s", surf->material);
		tex = R_FindTexture(texname, it_model, true);
		if (!tex || tex && tex == r_texture_missing)
		{
			tex = r_texture_missing;
			ri.Printf(PRINT_ALL, "%s: missing texture '%s' for model '%s'\n", __FUNCTION__, texname, mod->name);
		}
		surf->materialIndex = tex->texnum;
	}
}

static void R_UploadNewModelTris(model_t* mod)
{
	int bufsize, ongpusize;
	pmodel_vertex_t *pverts = R_GetModelVertexesPtr(mod);

	if (!mod->newmod)
	{
		ri.Error(ERR_FATAL, "%s: !mod->newmod\n", __FUNCTION__);
		return;
	}

	if (mod->newmod->numVertexes < 3)
	{
		ri.Error(ERR_FATAL, "%s: no verts!\n", __FUNCTION__);
		return;
	}

	mod->vb[0] = R_AllocVertexBuffer(0, 0, 0); // HAACK

	bufsize = sizeof(pmodel_vertex_t) * mod->newmod->numVertexes;

	glGenBuffers(1, &mod->vb[0]->vboBuf);
	glBindBuffer(GL_ARRAY_BUFFER, mod->vb[0]->vboBuf);
	glBufferData(GL_ARRAY_BUFFER, bufsize, &pverts->xyz[0], GL_STATIC_DRAW);

	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &ongpusize);
	if (bufsize != ongpusize)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		ri.Error(ERR_FATAL, "%s: upload size %i != %i for %s\n", __FUNCTION__, ongpusize, bufsize, mod->name);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void R_LoadNewModel(model_t* mod, void* buffer)
{
	pmodel_header_t* pHdr;
	pmodel_bone_t *bone;
	panim_bonetrans_t *trans;
	pmodel_vertex_t *vert;
	pmodel_surface_t *surf;
	pmodel_part_t *part;
	int  i, j;

	mod->type = MOD_BAD;

	if (modelFileLength <= sizeof(pmodel_header_t))
	{
		ri.Error(ERR_DROP, "%s: '%s' has weird size.\n", __FUNCTION__, mod->name);
		return;
	}

	// swap header
	pHdr = (pmodel_header_t*)buffer;
	pHdr->version = LittleLong(pHdr->version);
	pHdr->flags = LittleLong(pHdr->flags);
	pHdr->numBones = LittleLong(pHdr->numBones);
	pHdr->numVertexes = LittleLong(pHdr->numVertexes);
	pHdr->numSurfaces = LittleLong(pHdr->numSurfaces);
	pHdr->numParts = LittleLong(pHdr->numParts);

	for (i = 0; i < 3; i++)
	{
		pHdr->mins[i] = LittleFloat(pHdr->mins[i]);
		pHdr->maxs[i] = LittleFloat(pHdr->maxs[i]);
	}

	// do a basic validation
	if (pHdr->ident != PMODEL_IDENT)
	{
		ri.Error(ERR_DROP, "%s: '%s' is not a model.\n", __FUNCTION__, mod->name);
		return;
	}

	if (pHdr->version != PMODEL_VERSION)
	{
		ri.Error(ERR_DROP, "%s: Model '%s' has wrong version (%i should be %i).\n", __FUNCTION__, mod->name, pHdr->version, PMODEL_VERSION);
		return;
	}

	if (pHdr->numBones < 1)
	{
		ri.Error(ERR_DROP, "%s: Model '%s' has no bones.\n", __FUNCTION__, mod->name);
		return;
	}

	if (pHdr->numVertexes < 3)
	{
		ri.Error(ERR_DROP, "%s: Model '%s' has too few vertexs.\n", __FUNCTION__, mod->name);
		return;
	}

	if (pHdr->numSurfaces < 1)
	{
		ri.Error(ERR_DROP, "%s: Model '%s' has no surfaces.\n", __FUNCTION__, mod->name);
		return;
	}

	if (pHdr->numParts < 1)
	{
		ri.Error(ERR_DROP, "%s: Model '%s' has no parts.\n", __FUNCTION__, mod->name);
		return;
	}

	// calculate offsets 
	// TODO: move it to converter ?
	pHdr->ofs_bones = sizeof(pmodel_header_t);
	pHdr->ofs_skeleton = (pHdr->ofs_bones + (sizeof(pmodel_bone_t) * pHdr->numBones));
	pHdr->ofs_vertexes = (pHdr->ofs_skeleton + (sizeof(panim_bonetrans_t) * pHdr->numBones));
	pHdr->ofs_surfaces = (pHdr->ofs_vertexes + (sizeof(pmodel_vertex_t) * pHdr->numVertexes));
	pHdr->ofs_parts = (pHdr->ofs_surfaces + (sizeof(pmodel_surface_t) * pHdr->numSurfaces));
	pHdr->ofs_end = (pHdr->ofs_parts + (sizeof(pmodel_part_t) * pHdr->numParts));

	if (pHdr->ofs_end > modelFileLength)
	{
		ri.Error(ERR_DROP, "%s: model '%s' is corrupt.\n", __FUNCTION__, mod->name);
		return;
	}

	// allocate model
	mod->extradatasize = modelFileLength+1;
	mod->newmod = Hunk_Alloc(modelFileLength+1);
	memcpy(mod->newmod, buffer, modelFileLength);
	mod->type = MOD_NEWFORMAT;

	// bones
	
	//assert(bone);
	bone = (pmodel_bone_t*)((byte*)mod->newmod + pHdr->ofs_bones);
	for (i = 0; i < pHdr->numBones; i++, bone++)
	{	
		bone->number = LittleLong(bone->number);
		bone->parentIndex = LittleLong(bone->parentIndex);

		for (j = 0; j < 3; j++)
		{
			bone->mins[j] = LittleLong(bone->mins[j]);
			bone->maxs[j] = LittleLong(bone->maxs[j]);
		}
	}

	// skeleton
	trans = R_GetModelSkeletonPtr(mod);
	//assert(trans);
	for (i = 0; i < pHdr->numBones; i++, trans++)
	{
		trans->bone = LittleLong(trans->bone);
		for (j = 0; j < 3; j++)
		{
			trans->origin[j] = LittleLong(trans->origin[j]);
			trans->rotation[j] = LittleLong(trans->rotation[j]);
		}
	}

	// vertexes
	vert = R_GetModelVertexesPtr(mod);
	//assert(vert);
	for (i = 0; i < pHdr->numVertexes; i++, vert++)
	{
		vert->boneId = LittleLong(vert->boneId);
		for (j = 0; j < 3; j++)
		{
			vert->xyz[j] = LittleFloat(vert->xyz[j]);
			vert->normal[j] = LittleFloat(vert->normal[j]);
		}
	
		vert->uv[0] = LittleFloat(vert->uv[0]);
		vert->uv[1] = -LittleFloat(vert->uv[1]);
	}

	// surfaces
	surf = R_GetModelSurfacesPtr(mod);
	//assert(surf);
	for (i = 0; i < pHdr->numSurfaces; i++, surf++)
	{
		surf->firstVert = LittleLong(surf->firstVert);
		surf->numVerts = LittleLong(surf->numVerts);
	}

	// parts
	part = R_GetModelPartsPtr(mod);
	//assert(part);
	for (i = 0; i < pHdr->numParts; i++, part++)
	{
		part->firstSurf = LittleLong(part->firstSurf);
		part->numSurfs = LittleLong(part->numSurfs);
	}

//#ifdef PRAGMA_RENDERER
	mod->numframes = 1;

	CalcInverseMatrixForModel(mod);

	// load textures
	Mod_LoadNewModelTextures(mod);

	// upload verts to gpu
	R_UploadNewModelTris(mod);
//#endif
}

/*
=================
CalcBoneMatrix
Calculate matrix for a given bone frame and store result in bones matrix array
=================
*/
static void CalcBoneMatrix(const model_t* pModel, panim_bonetrans_t* pTrans, const qboolean bNormalizeQuat, mat4_t* pMatrix)
{
	pmodel_bone_t* boneinfo;
	int i, j, k;
	int selfIdx, parentIdx;
	mat4_t tempMatrix;

	selfIdx = pTrans->bone; // index to transitioning bone
	boneinfo = R_GetModelBonesPtr(pModel) + pTrans->bone;
	parentIdx = boneinfo->parentIndex;

	if (bNormalizeQuat)
	{
		// quaternion must be normalized before conversion to matrix its 
		// normalized for every exact frame, but not for "inbetweens" during slerp
		Quat_Normalize(&pTrans->quat);
	}

	//Mat4MakeIdentity(tempMatrix);

	// create matrix from quaternion
	Quat_ToMat4(pTrans->quat, tempMatrix);

	tempMatrix[12] = pTrans->origin[0];
	tempMatrix[13] = pTrans->origin[1];
	tempMatrix[14] = pTrans->origin[2];

	if (parentIdx == -1)
	{
		// root bone
		memcpy(pMatrix[selfIdx], tempMatrix, sizeof(mat4_t));
	}
	else
	{
		// child bone
		memset(pMatrix[selfIdx], 0, sizeof(mat4_t));
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 4; j++)
			{
				for (k = 0; k < 4; k++)
				{
					pMatrix[selfIdx][i + j * 4] += pMatrix[parentIdx][i + k * 4] * tempMatrix[k + j * 4];
				}
			}
		}
	}
}

/*
=================
CalcInverseMatrixForModel
Calculate and invert matrix for skeleton
=================
*/
static void CalcInverseMatrixForModel(model_t* pModel)
{
	panim_bonetrans_t* bonetrans;
	mat4_t* invBoneMatrix;
	int i, numbones;

	if (pModel->inverseBoneMatrix)
	{
		return; // inverse bone matrix already created
	}

	numbones = Mod_GetNumBones(pModel);
	pModel->inverseBoneMatrix = invBoneMatrix = ri.MemAlloc(sizeof(mat4_t) * numbones);

	for (i = 0; i < numbones; i++)
		Mat4MakeIdentity(invBoneMatrix[i]);

	bonetrans = R_GetModelSkeletonPtr(pModel);
	for (i = 0; i < numbones; i++, bonetrans++)
	{
		Quat_FromAngles(bonetrans->rotation, &bonetrans->quat);
		CalcBoneMatrix(pModel, bonetrans, true, invBoneMatrix);
	}

	for (i = 0; i < numbones; i++)
		Mat4Invert(invBoneMatrix[i], invBoneMatrix[i]);
}



extern vec3_t	model_shadevector;
extern float	model_shadelight[3];
extern qboolean r_pendingflip;

void R_DrawNewModel(rentity_t* ent)
{
	pmodel_header_t* hdr;
	pmodel_part_t* part;
	pmodel_surface_t* surf;
	int i;
	qboolean isAnimated = false;
	int numAttribs;

	static int va_attrib[4];
	static qboolean vaAtrribsChecked = false;

	if (ent == NULL || ent->model == NULL || ent->model->newmod == NULL)
	{
		ri.Printf(PRINT_LOW, "%s: rentity has no model.\n", __FUNCTION__);
		return;
	}

	hdr = ent->model->newmod;

	if (ent->model->vb[0] == NULL || ent->model->vb[0]->vboBuf == 0)
	{
		ri.Printf(PRINT_LOW, "%s: mod has no vertex data.\n", __FUNCTION__);
		return;
	}


	R_BindProgram(isAnimated == true ? GLPROG_SMDL_ANIMATED : GLPROG_SMDL_RIGID);

	glBindBuffer(GL_ARRAY_BUFFER, ent->model->vb[0]->vboBuf);

	// FIXME: this should be done only once at startup and when program changes from GLPROG_SMDL_ANIMATED
	//if (!vaAtrribsChecked)
	{
		vaAtrribsChecked = true;

		va_attrib[0] = R_GetProgAttribLoc(VALOC_POS);
		va_attrib[1] = R_GetProgAttribLoc(VALOC_NORMAL);
		va_attrib[2] = R_GetProgAttribLoc(VALOC_TEXCOORD);

		if (isAnimated)
		{
			va_attrib[3] = R_GetProgAttribLoc(VALOC_BONEID);
			numAttribs = 4;
		}
		else
		{
			numAttribs = 3;
		}

		for (i = 0; i < numAttribs; i++)
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
	
	if (isAnimated && finalBonesMat != NULL)
	{	
		//R_SkeletonForFrame(ent->frame);
		R_ProgUniformMatrix4fv(LOC_BONES, hdr->numBones /*SMDL_MAX_BONES*/, finalBonesMat[0]);
	}

	//
	// draw the model
	//
	glEnableVertexAttribArray(va_attrib[0]);
	glVertexAttribPointer(va_attrib[0], 3, GL_FLOAT, GL_FALSE, sizeof(pmodel_vertex_t), NULL);

	glEnableVertexAttribArray(va_attrib[1]);
	glVertexAttribPointer(va_attrib[1], 3, GL_FLOAT, GL_FALSE, sizeof(pmodel_vertex_t), (void*)offsetof(pmodel_vertex_t, normal));

	glEnableVertexAttribArray(va_attrib[2]);
	glVertexAttribPointer(va_attrib[2], 2, GL_FLOAT, GL_FALSE, sizeof(pmodel_vertex_t), (void*)offsetof(pmodel_vertex_t, uv));

	if (isAnimated)
	{
		glEnableVertexAttribArray(va_attrib[3]);
		glVertexAttribPointer(va_attrib[3], 1, GL_INT, GL_FALSE, sizeof(pmodel_vertex_t), (void*)offsetof(pmodel_vertex_t, boneId));
	}

	glDisable(GL_CULL_FACE); // FIXME: cull order is BACK for skel models
	
	part = R_GetModelPartsPtr(ent->model);
	for (i = 0; i < hdr->numParts; i++, part++)
	{
		if ((ent->hiddenPartsBits & (1 << i)))
			continue; // part of the model is hidden

		surf = R_GetModelSurfacesPtr(ent->model) + part->firstSurf;
		for (int j = 0; j < part->numSurfs; j++, surf++)
		{
			R_MultiTextureBind(TMU_DIFFUSE, surf->materialIndex);
			glDrawArrays(GL_TRIANGLES, surf->firstVert, surf->numVerts);

			if (r_speeds->value)
				rperf.alias_tris += surf->numVerts / 3;
		}
	}
	glEnable(GL_CULL_FACE); // FIXME: cull order is BACK for skel models

	for (i = 0; i < numAttribs; i++)
		glDisableVertexAttribArray(va_attrib[i]);

	if (r_pendingflip)
	{
		Mat4Scale(r_projection_matrix, -1, 1, 1);
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (pCurrentRefEnt->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(false);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	R_UnbindProgram(GLPROG_SMDL_ANIMATED);

	if (r_speeds->value)
		rperf.alias_drawcalls++;
}