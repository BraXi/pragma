/*
prtool, part of pragma
Copyright (C) 2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

#ifdef _WIN32
	#define _CRT_SECURE_NO_DEPRECATE
	#define _WINDOWS_LEAN_AND_MEAN
#endif

typedef unsigned char byte;

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
//#include <vector>
//#include <iostream>

#ifdef _WIN32
	#include <windows.h>
	//extern HANDLE hConsole;
	#define CON_COL_WHITE	""
	#define CON_COL_GREEN	""
	#define CON_COL_RED		""
	#define CON_COL_YELLOW	""
#else
	#define CON_COL_WHITE	"\x1B[37m"
	#define CON_COL_GREEN	"\x1B[32m"
	#define CON_COL_RED		"\x1B[31m"
	#define CON_COL_YELLOW	"\x1B[33m"
#endif

#ifndef M_PI
	#define M_PI	3.14159265358979323846
#endif

#define MAX_QPATH 64		// maximum path length in quake filesystem (for includes from qcommon mostly)
#define	MAXPRINTMSG	4096	// maximum length of a print
#define	MAXLINELEN	512		// maximum length of a single line
#define MAXPATH 512			// maximum path length in prtool

#include "../common/mathlib.h"

extern void* Com_SafeMalloc(const size_t size, const char* sWhere);

extern void Com_Exit(const int code);

extern int GetNumWarnings();

extern void Com_HappyPrintf(const char* text, ...);
extern void Com_Printf(const char* text, ...);
extern void Com_Warning(const char* text, ...);
extern void Com_Error2(const char* text, ...);
extern void Com_Error(const char* text, ...);

extern FILE* Com_OpenReadFile(const char* filename, const int crash);
extern FILE* Com_OpenWriteFile(const char* filename, const int crash);
extern void Com_SafeRead(FILE* f, void* buffer, const unsigned int count);
extern void Com_SafeWrite(FILE* f, void* buffer, const unsigned int count);
extern long Com_FileLength(FILE* f);
extern int Com_LoadFile(const char* filename, void** buffer, int crash);
extern int Com_FileExists(const char* filename, ...);

extern int Com_NumArgs();
extern char* Com_GetArg(int arg);
extern char* Com_Args();
extern char* Com_Parse(char* data);
extern void Com_Tokenize(char* text);

extern int Com_EndianLong(int val);
extern short Com_EndianShort(short val);
extern float Com_EndianFloat(float val);

extern int Com_stricmp(const char* s1, const char* s2);


extern void ClearBounds(vec3_t mins, vec3_t maxs);
extern void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs);
extern float RadiusFromBounds(vec3_t mins, vec3_t maxs);;