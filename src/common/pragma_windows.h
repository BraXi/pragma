/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// pragma_windows.h: Win32-specific pragmaheader file
// 
// NOTE: this file is not included in DEDICATED_ONLY binary !

#ifndef _PRAGMA_WINDOWS_H_
#define _PRAGMA_WINDOWS_H_

#pragma once

#pragma warning( disable : 4229 )  // mgraph gets this

#include <windows.h>
#include <dsound.h>

#define	WINDOW_STYLE	(WS_OVERLAPPED|WS_BORDER|WS_CAPTION|WS_VISIBLE)

extern	HINSTANCE	global_hInstance;

extern LPDIRECTSOUND pDS;
extern LPDIRECTSOUNDBUFFER pDSBuf;

extern DWORD gSndBufSize;

extern HWND			cl_hwnd;
extern qboolean		ActiveApp, Minimized;

void IN_Activate (qboolean active);
void IN_MouseEvent (int mstate);

extern int		window_center_x, window_center_y;
extern RECT		window_rect;

#endif /*_PRAGMA_WINDOWS_H_*/