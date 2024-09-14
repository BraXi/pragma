/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// pragma.h -- definitions common between client and server, but not game.dll
#pragma once

#include "../common/shared.h"

//============================================================================

extern qboolean print_time;

#ifdef _WIN32
	#ifdef DEDICATED_ONLY
		#ifndef _DEBUG
			#define BUILDSTRING "Win Release DEDSV"
		#else
			#define BUILDSTRING "Win Debug DEDSV"
		#endif
	#else
		#ifndef _DEBUG
			#define BUILDSTRING "Win Release"
		#else
			#define BUILDSTRING "Win Debug"
		#endif
	#endif
#elif defined(__linux__)

	#ifdef DEDICATED_ONLY
		#ifndef _DEBUG
			#define BUILDSTRING "LINUX RELEASE DEDSV"
		#else
			#define BUILDSTRING "LINUX DEBUG DEDSV"
		#endif
	#else
		#ifndef _DEBUG
			#define BUILDSTRING "LINUX RELEASE"
		#else
			#define BUILDSTRING "LINUX DEBUG"
		#endif
	#endif

#else
	#define BUILDSTRING "NON-WIN32"
	#define	CPUSTRING	"NON-WIN32"
#endif

#if defined(__x86_64__) || defined(_M_X64)
        #define	CPUSTRING	"x64"
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
        #define	CPUSTRING	"x86"
#else
	#define	CPUSTRING	"NON-x86/x64"
#endif

//============================================================================

#include "../common/crc.h"
#include "../common/pragma_files.h"
#include "script/scriptvm.h"
#include "protocol.h"
#include "cmd.h"
#include "cvar.h"
#include "sizebuf.h"
#include "network.h"
#include "net_chan.h"
#include "cmodel.h"
#include "filesystem.h"
#include "message.h"
#include "usercmd.h"

//============================================================================
typedef enum memtag_s
{
	TAG_NONE,
	TAG_FX,
	TAG_NAV_NODES,

	TAG_SERVER_GAME,
	TAG_CLIENT_GAME,
	TAG_GUI,

	TAG_QCVM_MEMORY, // dummy, used to offset
	
	TAG_QCVM1,
	TAG_QCVM2,
	TAG_QCVM3,

	NUM_MEMORY_TAGS
} memtag_t;

/*
// script virtual machines
typedef enum
{
	VM_NONE,	// no program execution at all
	VM_SVGAME,	// server game
	VM_CLGAME,	// client game
	VM_GUI,		// gui
	NUM_SCRIPT_VMS
} vmType_t;
*/

//============================================================================

//#define	FOFS(type,x) (int)&(((type *)0)->x)
typedef void (*parsecommand_t) (char* value, byte* basePtr);

typedef enum
{
	F_INT,
	F_FLOAT,
	F_STRING,
	F_STRINGCOPY,
	F_BOOLEAN,
	F_VECTOR2,
	F_VECTOR3,
	F_VECTOR4,
	F_QCFUNC,
	F_HACK,
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char* name;
	fieldtype_t	type;
	int		ofs;
	parsecommand_t function;
} parsefield_t;

//============================================================================

typedef enum { MOD_BAD, MOD_BRUSH, MOD_ALIAS, MOD_NEWFORMAT } modtype_t;

//============================================================================
typedef enum { DPRIMITIVE_LINE, DPRIMITIVE_POINT, DPRIMITIVE_BOX, DPRIMITIVE_TEXT } dprimitive_t;

typedef struct debugline_s
{
	dprimitive_t type;
	vec3_t p1, p2;			// absmin & absmax if DPRIMITIVE_BOX
	vec3_t color;
	float thickness;			// font size if DPRIMITIVE_TEXT
	qboolean depthTest;
	float drawtime;
	char text[MAX_QPATH];				// if DPRIMITIVE_TEXT
} debugprimitive_t;

//============================================================================



extern	qboolean		bigendien;

extern	short	BigShort (short l);
extern	short	LittleShort (short l);
extern	int		BigLong (int l);
extern	int		LittleLong (int l);
extern	float	BigFloat (float l);
extern	float	LittleFloat (float l);

//============================================================================


int	COM_Argc (void);
char *COM_Argv (int arg);	// range and null checked
void COM_ClearArgv (int arg);
int COM_CheckParm (char *parm);
void COM_AddParm (char *parm);

void COM_Init (void);
void COM_InitArgv (int argc, char **argv);

char *CopyString (const char *in);


void Info_Print (char *s);




/*
==============================================================

MISC

==============================================================
*/


#define	ERR_FATAL	0		// exit the entire game with a popup window
#define	ERR_DROP	1		// print to console and disconnect from game
#define	ERR_QUIT	2		// not an error, just a normal exit

#define	EXEC_NOW	0		// don't return until completed
#define	EXEC_INSERT	1		// insert at current position, but don't run yet
#define	EXEC_APPEND	2		// add to end of the command buffer

// renderer begin
#define	PRINT_ALL			0
#define PRINT_DEVELOPER		1		// only print when "developer 1"
#define PRINT_ALERT			2
// renderer end

void		Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush));
void		Com_EndRedirect (void);
void 		Com_Printf (char *fmt, ...);
void 		Com_DPrintf (dprintLevel_t chan, char *fmt, ...);
void 		Com_Error (int code, char *fmt, ...);
void 		Com_Quit (void);

int			Com_ServerState (void);		// this should have just been a cvar...
void		Com_SetServerState (int state);

unsigned	Com_BlockChecksum (void *buffer, int length);
byte		COM_BlockSequenceCRCByte (byte *base, int length, int sequence);

float	frand(void);	// 0 to 1
float	crand(void);	// -1 to 1

extern	cvar_t	*developer;
extern	cvar_t	*dedicated;

#ifndef DEDICATED_ONLY
extern	cvar_t	*host_speeds;
extern	cvar_t	*log_stats;
extern	FILE *log_stats_file;
#endif

// host_speeds times
extern	int		time_before_game;
extern	int		time_after_game;
extern	int		time_before_ref;
extern	int		time_after_ref;

void Z_Free (void *ptr);
void *Z_Malloc (int size);			// returns 0 filled memory
void *Z_TagMalloc (int size, memtag_t tag);
void Z_FreeTags (memtag_t tag);

char* COM_NewString(char* string, memtag_t memtag);
qboolean COM_ParseField(char* key, char* value, byte* basePtr, parsefield_t* f);

void Qcommon_Init (int argc, char **argv);
void Qcommon_Frame (int msec);
void Qcommon_Shutdown (void);

#define NUM_VERTEX_NORMALS	162
extern	vec3_t	bytedirs[NUM_VERTEX_NORMALS];

#ifndef DEDICATED_ONLY
// this is in the client code, but can be used for debugging from server
void CL_DebugGraph (float value, vec3_t color);
#endif /*DEDICATED_ONLY*/

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

void	Sys_Init (void);

#ifndef DEDICATED_ONLY
void	Sys_AppActivate (void);
#endif

char	*Sys_ConsoleInput (void);
void	Sys_ConsoleOutput (char *string);
void	Sys_SendKeyEvents (void);
void	Sys_Error (char *error, ...);
void	Sys_Quit (void);
char	*Sys_GetClipboardData( void );

/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

#ifndef DEDICATED_ONLY
void CL_Init (void);
void CL_Drop (void);
void CL_Shutdown (void);
void CL_Frame (int msec);
void Con_Print (char *text);
void SCR_BeginLoadingPlaque (void);
#endif

void SV_Init (void);
void SV_Shutdown (char *finalmsg, qboolean reconnect);
void SV_Frame (int msec);

qboolean Com_IsServerActive();

#ifndef DEDICATED_ONLY
qboolean Con_IsClientActive();
#endif

