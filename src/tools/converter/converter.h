#pragma once
#ifndef _CONVERTER_H_
#define _CONVERTER_H_

#include "../tools_shared.h"

#define PRTOOL_VERSION "0.1"

#define defaultgamedir "main"
#define defaultdevdir "devraw"

#define PAKLIST_DIR "pak_src"

extern char g_gamedir[MAXPATH];
extern char g_devdir[MAXPATH];
extern char g_controlFileName[MAXPATH];
extern bool g_verbose;

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

extern model_t* Mod_LoadModel(const char* modelname, const char* filename);

#include "../../qcommon/qfiles.h"

#define MAX_FILENAME_IN_PACK 56

#endif /*_CONVERTER_H_*/