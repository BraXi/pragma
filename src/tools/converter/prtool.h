/*
prtool, part of pragma
Copyright (C) 2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once
#ifndef _CONVERTER_H_
#define _CONVERTER_H_

#include "../tools_shared.h"
#include <vector> //ugly mothafucka for which i compile as cpp

#define PRTOOL_VERSION "0.1"

#define DEFAULT_GAME_DIR "main"
#define DEFAULT_DEV_DIR "devraw"

#define PAKLIST_DIR "pak_src"

extern char g_gamedir[MAXPATH];
extern char g_devdir[MAXPATH];
extern char g_controlFileName[MAXPATH];
extern bool g_verbose;


typedef void (*xcommand_t) ();
typedef struct
{
	const char* name;
	xcommand_t pFunc;
	int numargs;
} command_t;

//
// models.c
//

typedef enum { MOD_BAD = 0, MOD_BRUSH, MOD_MD3, MOD_SKEL } modtype_t;

typedef struct model_s
{
	char		name[MAX_QPATH];
	modtype_t	type;

	int		numTextures;
	char	textureNames[32][MAX_QPATH];

	void	*pData;
	size_t	dataSize;
} model_t;

extern model_t* LoadModel(const char* modelname, const char* filename);

#include "../../qcommon/qfiles.h"
#define MAX_FILENAME_IN_PACK 56

//
// smd.cpp
//

typedef enum { SMD_MODEL, SMD_ANIMATION } smdtype_t; // What was loaded from file and what we expect to load

typedef struct smd_bone_s
{
	int index;
	int parent;		// parent bone index index, -1 == root bone
	char name[MAX_QPATH];
} smd_bone_t;

typedef struct smd_vert_s
{
	vec3_t	xyz;
	vec3_t	normal;
	vec2_t	uv;
	int	bone;
} smd_vert_t;

typedef struct smd_bonetransform_s
{
	int bone;
	vec3_t position;
	vec3_t rotation;
} smd_bonetransform_t;

typedef struct smd_surface_s
{
	int firstvert;
	int numverts;			// firstvert + numverts = triangles
	char texture[MAX_QPATH];
} smd_surface_t;

typedef struct smddata_s
{
	char name[MAX_QPATH];
	smdtype_t type;

	// data loaded or derived from .smd file
	int numbones;
	int numframes;
	int numverts;
	int numsurfaces;

	vec3_t mins, maxs;
	int radius;

	smd_bone_t* vBones[SMDL_MAX_BONES]; //SMD_NODES
	smd_bonetransform_t* vBoneTransforms[SMDL_MAX_BONES*SMDL_MAX_FRAMES]; //SMD_SKELETON
	smd_vert_t* vVerts[SMDL_MAX_VERTS]; //SMD_TRIANGLES
	smd_surface_t* vSurfaces[SMDL_MAX_SURFACES]; // built for each texture
} smddata_t;

#endif /*_CONVERTER_H_*/