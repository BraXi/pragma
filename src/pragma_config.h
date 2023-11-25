#pragma once

// version string
#define PRAGMA_VERSION "0.21"
#define PRAGMA_TIMESTAMP (__DATE__ " " __TIME__)

// net protocol will use floats for coordinates instead of shorts, this applies to pmove too
// this also fixes some of pmove issues like dragging players motion towards 0,0,0 and higher jumps
// all entities can move beyond +/- 4096qu boundary, 
#define PROTOCOL_FLOAT_COORDS 1

// player view angles will use floats instead shorts
#define PROTOCOL_FLOAT_PLAYERANGLES 0

// protocol can use shorts when modelindex or soundindex exceed byte
#define PROTOCOL_EXTENDED_ASSETS 1

// main engine directory to load assets from, the default 'game'
#define	BASEDIRNAME	"main" 

// experimental -- use GLFW for windows and input instead of windows api
#define USE_GLFW 0

// experimental -- variable server fps
#define SERVER_FPS 10
//#define SERVER_FPS 20
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
#pragma error "wrong server fps"
#endif