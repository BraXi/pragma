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

// main engine directory to load assets from (the default 'game' directory)
#define	BASEDIRNAME	"main" 

// define this to dissalow any data but the pak0.pak matching checksum below
//#define NO_ADDONS 

// if a packfile directory differs from this, it is assumed to be hacked
#define	PAK0_CHECKSUM	0x40e614e0

// Enable fix for "stupid quake bug"
#define FIX_SQB 1 

// Include new GUI system
#define NEW_GUI 1


// See protocol.h for MORE!

// protocol can use shorts when modelindex or soundindex exceed byte

#define	MAX_CLIENTS			12		// maximum number of client slots
#define	MAX_GENTITIES		2048	// number of game entities
#define	MAX_LIGHTSTYLES		256		// number of light styles

#define	MAX_ITEMS			256 // general config strings
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings


#define	MAX_MODELS			512		// number of models (excluding inline models)
#define	MAX_SOUNDS			512
#define	MAX_IMAGES			256

#if (MAX_SOUNDS > 256 || MAX_IMAGES > 256)
#define PROTOCOL_EXTENDED_ASSETS 1
#endif




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
#define PRAGMA_VERSION "0.34" 
#define PRAGMA_TIMESTAMP (__DATE__ " " __TIME__)

// version history:
// 0.34 - 24.09.2024 -- inline models separated from external models
// 0.33 - 22.09.2024 -- lots of fixes from over time
// 0.32 - 08.09.2024 -- source tree cleanup
// 0.31 - 03.09.2024 -- prtool and pragma's own model/anim formats
// 0.30 - xx.xx.2024 -- skeletal models
// 0.29 - xx.xx.2024 -- lighting overhaul
// 0.28 - 22.03.2024 -- BSPX, QBISM, DECOUPLEDLM, LMSHIFT
// 0.27 - 02.03.2024 -- experimental renderer
// 0.26 - 21.02.2024
// 0.25 - 18.01.2024 -- linux dedicated server
// 0.24 - 17.01.2024 -- win32 dedicated server
// 0.23 - 04.01.2024

//#define DEDICATED_ONLY 1

#endif /*PRAGMA_CONFIG_INCLUDED*/
