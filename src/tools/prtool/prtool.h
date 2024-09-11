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
#include <vector>

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

#include "../../common/pragma_files.h"


//
// smd.cpp
//

#include "smd.h"

#endif /*_CONVERTER_H_*/