/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

#ifndef _MODEL_CACHE_H_
#define _MODEL_CACHE_H_

// CACHE_CLIENT will load additional data for renderer
// CACHE_SERVER will only load the bare minimum
// CACHE_NONE means we no longer need the model
typedef enum { CACHE_NONE, CACHE_SERVER, CACHE_CLIENT} cacheuser_t;

typedef struct cached_model_s // !!! IF THIS IS CHANGED RENDERER MUST BE RECOMPILED !!!
{
	char name[MAX_QPATH];

	modtype_t type;
	cacheuser_t users[2]; // client & server

	int numFrames;
	int numParts;
	int numBones;

	vec3_t mins, maxs;

	cmodel_t* brush;
	pmodel_header_t* skel;
	md3Header_t* alias;

	// inversed skeleton matrix is created for bind pose
	mat4_t* pInverseSkeleton;

	// shared with everyone, accessible via: brush, skel or alias ptrs
	void* pData;
	int dataSize;

	// pRenderData is private to renderer
	void* pRenderData;
	int renderDataSize;
} cached_model_t;


const int Mod_NumCachedModels();

qboolean Mod_HasRenderData(cached_model_t* pModel);

cached_model_t* Mod_Register(const cacheuser_t user, const char* name, const qboolean bMustLoad);
//void Mod_Free(cached_model_t* pModel);
void Mod_FreeUnused(const cacheuser_t user);
void Mod_FreeAll();

mat4_t* Mod_GetInverseSkeletonMatrix(const cached_model_t* pModel);

pmodel_bone_t* Mod_GetBonesPtr(const cached_model_t* pModel);
panim_bonetrans_t* Mod_GetSkeletonPtr(const cached_model_t* pModel);
pmodel_vertex_t* Mod_GetVertexesPtr(const cached_model_t* pModel);
void* Mod_GetSurfacesPtr(const cached_model_t* pModel);
void* Mod_GetPartsPtr(const cached_model_t* pModel);

modtype_t Mod_GetType(const cached_model_t* pModel);
int Mod_NumVerts(const cached_model_t* pModel);
int Mod_NumFrames(const cached_model_t* pModel);
int Mod_NumParts(const cached_model_t* pModel);
int Mod_NumBones(const cached_model_t* pModel);

int Mod_PartIndexForName(const cached_model_t* pModel, const char* partName);
int Mod_BoneIndexForName(const cached_model_t* pModel, const char* boneName);

// float Mod_GetCullRadius(const cached_model_t* pModel);
//vec3_t Mod_GetCullMins(const cached_model_t* pModel);
//vec3_t Mod_GetCullMaxs(const cached_model_t* pModel);

#endif /*_MODEL_CACHE_H_*/
