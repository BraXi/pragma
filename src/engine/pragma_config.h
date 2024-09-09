/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

//
// MAIN CONFIGURATION FILE FOR ENGINE
//


#pragma once

#ifndef PRAGMA_CONFIG_INCLUDED 
#define PRAGMA_CONFIG_INCLUDED 1

// what renderer DLL to use by default
#define DEFAULT_RENDERER "gl2"


// define this to dissalow any data but the pak0.pak matching checksum below
//#define	NO_ADDONS 

// if a packfile directory differs from this, it is assumed to be hacked
#define	PAK0_CHECKSUM	0x40e614e0


// Enable "stupid quake bug" fix
#define FIX_SQB 1 

// Include new gui system
#define NEW_GUI 1

// net protocol will use floats for coordinates instead of shorts, this applies to pmove too
// this also fixes some of pmove issues like dragging players motion towards 0,0,0 and higher jumps
// all entities can move beyond +/- 4096qu boundary, 
#define PROTOCOL_FLOAT_COORDS 1

// protocol can use shorts when modelindex or soundindex exceed byte
#define PROTOCOL_EXTENDED_ASSETS 1

#define	MAX_CLIENTS			32		// absolute limit of maxclients
#define	MAX_GENTITIES		2048	// must change protocol to increase more
#define	MAX_LIGHTSTYLES		256

#ifdef PROTOCOL_EXTENDED_ASSETS
#	define	MAX_MODELS			1024	// these can be sent over the net as shorts
#	define	MAX_SOUNDS			1024	// so theoretical limit is 32768 for each
#else
#	define	MAX_MODELS			256		// these are sent over the net as bytes
#	define	MAX_SOUNDS			256		// so they cannot be higher than 255
#endif
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings


// main engine directory to load assets from (the default 'game' directory)
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
#define PRAGMA_VERSION "0.29" 
#define PRAGMA_TIMESTAMP (__DATE__ " " __TIME__)

// version history:
// 0.29 - xx.xx.2024 -- lighting overhaul
// 0.28 - 22.03.2024 -- BSPX, QBISM, DECOUPLEDLM, LMSHIFT
// 0.27 - 02.03.2024 -- experimental renderer
// 0.26 - 21.02.2024
// 0.25 - 18.01.2024 -- linux dedicated server
// 0.24 - 17.01.2024 -- win32 dedicated server
// 0.23 - 04.01.2024

//#define DEDICATED_ONLY 1

#endif /*PRAGMA_CONFIG_INCLUDED*/