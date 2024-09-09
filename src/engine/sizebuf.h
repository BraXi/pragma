/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================

SIZEBUF


==============================================================
*/

#pragma once

#ifndef _PRAGMA_SIZEBUF_H_
#define _PRAGMA_SIZEBUF_H_

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Com_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte* data;
	int		maxsize;
	int		cursize;
	int		readcount;
} sizebuf_t;

void SZ_Init(sizebuf_t* buf, byte* data, const int length);
void SZ_Clear(sizebuf_t* buf);
void* SZ_GetSpace(sizebuf_t* buf, const int length);
void SZ_Write(sizebuf_t* buf, void* data, const int length);
void SZ_Print(sizebuf_t* buf, char* data);	// strcats onto the sizebuf

#endif /*_PRAGMA_SIZEBUF_H_*/
