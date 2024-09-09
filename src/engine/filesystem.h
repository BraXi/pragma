/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/


/*
==============================================================
FILESYSTEM
==============================================================
*/

#pragma once

#ifndef _PRAGMA_FILESYSTEM_H_
#define _PRAGMA_FILESYSTEM_H_

void FS_InitFilesystem(void);
void FS_SetGamedir(char* dir);
char *FS_Gamedir(void);
char *FS_NextPath(char* prevpath);
void FS_ExecAutoexec(void);

int FS_FOpenFile(char* filename, FILE** file);
void FS_FCloseFile(FILE* f); // note: this can't be called from another DLL, due to MS libc issues


int FS_LoadFile(char* path, void** buffer);
// a null buffer will just return the file length without loading
// a -1 length is not present

void FS_Read(void* buffer, int len, FILE* f);
// properly handles partial reads

void FS_FreeFile(void* buffer);

int FS_LoadTextFile(char* filename, char** buffer);

void FS_CreatePath(char* path);

#endif /*_PRAGMA_FILESYSTEM_H_*/
