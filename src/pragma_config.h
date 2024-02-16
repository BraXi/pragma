/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

#ifndef PRAGMA_CONFIG_INCLUDED 
#define PRAGMA_CONFIG_INCLUDED 1

#define NEW_GUI 1 //new gui

// Pragma will use pmove in shared cg/sv qc progs instead of C version
// Enabling USE_PMOVE_IN_PROGS allows mods to have full control over pmove
#define USE_PMOVE_IN_PROGS 1

// net protocol will use floats for coordinates instead of shorts, this applies to pmove too
// this also fixes some of pmove issues like dragging players motion towards 0,0,0 and higher jumps
// all entities can move beyond +/- 4096qu boundary, 
#define PROTOCOL_FLOAT_COORDS 1

// protocol can use shorts when modelindex or soundindex exceed byte
#define PROTOCOL_EXTENDED_ASSETS 1

// main engine directory to load assets from, the default 'game'
#define	BASEDIRNAME	"main" 

// experimental -- use GLFW for windows and input instead of windows api [not implemented yet]
#define USE_GLFW 0

// experimental -- variable server fps
#define SERVER_FPS 10		// quake 2
//#define SERVER_FPS 20		// default
//#define SERVER_FPS 40

#if SERVER_FPS == 10
#define SV_FRAMETIME 0.1
#define SV_FRAMETIME_MSEC 100
#elif SERVER_FPS == 20
#define SV_FRAMETIME 0.05
#define SV_FRAMETIME_MSEC 50
#elif SERVER_FPS == 40
#define SV_FRAMETIME 0.025
#define SV_FRAMETIME_MSEC 25
#else
#pragma error "Wrong SERVER_FPS"
#endif

// version string
#define PRAGMA_VERSION "0.25" 
#define PRAGMA_TIMESTAMP (__DATE__ " " __TIME__)

// version history:
// 0.25 - 18.01.2024 -- linux dedicated server
// 0.24 - 17.01.2024 -- win32 dedicated server
// 0.23 - 04.01.2024

//#define DEDICATED_ONLY 1

#endif /*PRAGMA_CONFIG_INCLUDED*/
