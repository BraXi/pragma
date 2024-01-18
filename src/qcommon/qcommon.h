/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// qcommon.h -- definitions common between client and server, but not game.dll
#pragma once

#include "shared.h"
#include "../script/scriptvm.h"

//============================================================================

extern qboolean print_time;

#ifdef _WIN32
	#ifdef DEDICATED_ONLY
		#ifndef _DEBUG
			#define BUILDSTRING "WIN RELEASE DEDSV"
		#else
			#define BUILDSTRING "WIN DEBUG DEDSV"
		#endif
	#else
		#ifndef _DEBUG
			#define BUILDSTRING "WIN RELEASE"
		#else
			#define BUILDSTRING "WIN DEBUG"
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

#define TAG_SERVER_MODELDATA 20231
#define TAG_SERVER_GAME 20232

//============================================================================

#define	FOFS(type,x) (int)&(((type *)0)->x)
typedef void (*parsecommand_t) (char* value, byte* basePtr);

typedef enum
{
	F_INT,
	F_FLOAT,
	F_STRING,
	F_BOOLEAN,
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

typedef enum { MOD_BAD, MOD_BRUSH, MOD_SPRITE, MOD_MD3 } modtype_t;

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

#define NET_RATE_MIN 5000 // was absurdly low (100) in Q2
#define NET_RATE_MAX 25000
#define NET_RATE_DEFAULT 15000 // was 5000 in Q2
//============================================================================

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Com_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
	int		readcount;
} sizebuf_t;

void SZ_Init (sizebuf_t *buf, byte *data, int length);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, void *data, int length);
void SZ_Print (sizebuf_t *buf, char *data);	// strcats onto the sizebuf

//============================================================================

struct usercmd_s;
struct entity_state_s;

#if PROTOCOL_FLOAT_COORDS == 1
static inline int MSG_PackSolid32(const vec3_t mins, const vec3_t maxs)
{
	// Q2PRO code
	int x = maxs[0];
	int y = maxs[1];
	int zd = -mins[2];
	int zu = maxs[2] + 32;

	clamp(x, 1, 255);
	clamp(y, 1, 255);
	clamp(zd, 0, 255);
	clamp(zu, 0, 255);

	return MakeLittleLong(x, y, zd, zu);
}

static inline void MSG_UnpackSolid32(int packedsolid, vec3_t mins, vec3_t maxs)
{
	// Q2PRO code
	int x = packedsolid & 255;
	int y = (packedsolid >> 8) & 255;
	int zd = (packedsolid >> 16) & 255;
	int zu = ((packedsolid >> 24) & 255) - 32;

	VectorSet(mins, -x, -y, -zd);
	VectorSet(maxs, x, y, zu);
}
#else
static inline void MSG_UnpackSolid16(int packedsolid, vec3_t bmins, vec3_t bmaxs)
{
	int		x, zd, zu;
	// encoded bbox
	x = 8 * ((int)packedsolid & 31);
	zd = 8 * (((int)packedsolid >> 5) & 31);
	zu = 8 * (((int)packedsolid >> 10) & 63) - 32;

	bmins[0] = bmins[1] = -x;
	bmaxs[0] = bmaxs[1] = x;
	bmins[2] = -zd;
	bmaxs[2] = zu;
}
#endif

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WritePos (sizebuf_t *sb, vec3_t pos);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f);
void MSG_WriteDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);
void MSG_WriteDeltaEntity (struct entity_state_s *from, struct entity_state_s *to, sizebuf_t *msg, qboolean force, qboolean newentity);
void MSG_WriteDir (sizebuf_t *sb, vec3_t vector);



void	MSG_BeginReading (sizebuf_t *sb);

int		MSG_ReadChar (sizebuf_t *sb);
int		MSG_ReadByte (sizebuf_t *sb);
int		MSG_ReadShort (sizebuf_t *sb);
int		MSG_ReadLong (sizebuf_t *sb);
float	MSG_ReadFloat (sizebuf_t *sb);
char	*MSG_ReadString (sizebuf_t *sb);
char	*MSG_ReadStringLine (sizebuf_t *sb);

float	MSG_ReadCoord (sizebuf_t *sb);
void	MSG_ReadPos (sizebuf_t *sb, vec3_t pos);
float	MSG_ReadAngle (sizebuf_t *sb);
float	MSG_ReadAngle16 (sizebuf_t *sb);
void	MSG_ReadDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);

void	MSG_ReadDir (sizebuf_t *sb, vec3_t vector);

void	MSG_ReadData (sizebuf_t *sb, void *buffer, int size);

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

char *CopyString (char *in);

//============================================================================

void Info_Print (char *s);


/* crc.h */

void CRC_Init(unsigned short *crcvalue);
void CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);
unsigned short CRC_Block (byte *start, int count);
unsigned short CRC_ChecksumFile(char* name, qboolean fatal);



/*
==============================================================

PROTOCOL

==============================================================
*/

// protocol.h -- communications protocols

#define PROTOCOL_REVISION 4
#ifdef PROTOCOL_EXTENDED_ASSETS
	#define	PROTOCOL_VERSION	('B'+'X'+PROTOCOL_REVISION)
#else
	#define	PROTOCOL_VERSION	('B'+'X'+PROTOCOL_REVISION+'X')
#endif


//=========================================

#define	PORT_MASTER	27900
#define	PORT_CLIENT	27901
#define	PORT_SERVER	27910

//=========================================

#define	UPDATE_BACKUP	16	// copies of entity_state_t to keep buffered
							// must be power of two
#define	UPDATE_MASK		(UPDATE_BACKUP-1)



//==================
// the svc_strings[] array in cl_parse.c should mirror this
//==================

//
// server to client
//
enum svc_ops_e
{
	SVC_BAD,

	SVC_MUZZLEFLASH,
	SVC_TEMP_ENTITY,
	SVC_LAYOUT,
	SVC_INVENTORY,
	SVC_CGCMD,					// handled in cgame progs: [byte cmd] ....

	SVC_NOP,

	SVC_DISCONNECT,
	SVC_RECONNECT,

	SVC_SOUND,					// <see code>
	SVC_STOPSOUND,				// [byte] entity index

	SVC_PRINT,					// [byte] id [string] null terminated string

	SVC_STUFFTEXT,				// [string] stuffed into client's console buffer, should be \n terminated

	SVC_SERVERDATA,				// [long] protocol ...
	SVC_CONFIGSTRING,			// [short] [string]
	SVC_SPAWNBASELINE,

	SVC_CENTERPRINT,			// [string] to put in center of the screen

	SVC_DOWNLOAD,				// [short] size [size bytes]

	SVC_PLAYERINFO,				// variable

	SVC_PACKET_ENTITIES,		// [...]
	SVC_DELTA_PACKET_ENTITIES,	// [...]

	SVC_FRAME
};

//==============================================

//
// client to server
//
enum clc_ops_e
{
	clc_bad,
	clc_nop, 		
	clc_move,				// [[usercmd_t]
	clc_userinfo,			// [[userinfo string]
	clc_stringcmd			// [string] message
};

//==============================================

// plyer_state_t communication

#define	PS_M_TYPE			(1<<0)
#define	PS_M_ORIGIN			(1<<1)
#define	PS_M_VELOCITY		(1<<2)
#define	PS_M_TIME			(1<<3)
#define	PS_M_FLAGS			(1<<4)
#define	PS_M_GRAVITY		(1<<5)
#define	PS_M_DELTA_ANGLES	(1<<6)

#define	PS_VIEWOFFSET		(1<<7)
#define	PS_VIEWANGLES		(1<<8)
#define	PS_KICKANGLES		(1<<9)
#define	PS_BLEND			(1<<10)
#define	PS_FOV				(1<<11)
#define	PS_VIEWMODEL_INDEX	(1<<12)
#define	PS_VIEWMODEL_FRAME	(1<<13)
#define	PS_RDFLAGS			(1<<14)

#define PS_EXTRABYTES		(1<<15) // reki -- 28-12-23 added this so we don't shoot ourselves in the foot later, looking for more bytes
#define	PS_M_BBOX_SIZE		(1<<16) // braxi -- added bbox size, maybe make it byte?
#define PS_M_FLAGSLONG		(1<<17) // reki -- 28-12-23 if our mod is using a lot of pmove flags, make sure they're networked


//==============================================

// user_cmd_t communication

// ms and light always sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE2 	(1<<1)
#define	CM_ANGLE3 	(1<<2)
#define	CM_FORWARD	(1<<3)
#define	CM_SIDE		(1<<4)
#define	CM_UP		(1<<5)
#define	CM_BUTTONS	(1<<6)
#define	CM_IMPULSE	(1<<7)

//==============================================

// a sound without an ent or pos will be a local only sound
#define	SND_VOLUME		(1<<0)		// a byte
#define	SND_ATTENUATION	(1<<1)		// a byte
#define	SND_POS			(1<<2)		// three coordinates
#define	SND_ENT			(1<<3)		// a short 0-2: channel, 3-12: entity
#define	SND_OFFSET		(1<<4)		// a byte, msec offset from frame start
#define	SND_INDEX_16	(1<<5)		// soundnum is > 255

#define DEFAULT_SOUND_PACKET_VOLUME	1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//==============================================

// entity_state_t communication

// _8 = byte
// _16 = short

// try to pack the common update flags into the first byte
#define	U_ORIGIN_X			(1<<0)		// current origin
#define	U_ORIGIN_Y			(1<<1)		// current origin
#define	U_ANGLE_Y			(1<<2)		// current angles
#define	U_ANGLE_Z			(1<<3)		// current angles
#define	U_ANIMFRAME_8		(1<<4)		// anim frame is 0-255
#define	U_EVENT_8			(1<<5)		// byte
#define	U_REMOVE			(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS_1		(1<<7)		// -- read one additional byte --

// second byte
#define	U_NUMBER_16			(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN_Z			(1<<9)
#define	U_ANGLE_X			(1<<10)
#define	U_MODELINDEX_8		(1<<11)		// modelindex is 0-255
#define U_RENDERFLAGS_8		(1<<12)		// fullbright, etc
#define U_MODELINDEX_16		(1<<13)		// when modelindex > 255
#define	U_EFFECTS_8			(1<<14)		// effects - EF_ flags
#define	U_MOREBITS_2		(1<<15)		// -- read one additional byte --

// third byte
#define	U_SKIN_8			(1<<16)		// TODO: index to entry in .skin file
#define	U_ANIMFRAME_16		(1<<17)		// animframe greater than 255
#define	U_RENDERFLAGS_16	(1<<18)		// 8 + 16 = 32B
#define	U_EFFECTS_16		(1<<19)		// effects - EF_ flags, 8 + 16 = 32
#define	U_MODELINDEX2_8		(1<<20)		// weapons, flags, etc
#define	U_MODELINDEX3_8		(1<<21)		// unused
#define	U_MODELINDEX4_8		(1<<22)		// unused
#define	U_MOREBITS_3		(1<<23)		// -- read one additional byte --

// fourth byte
#define	U_OLDORIGIN			(1<<24)		// id: FIXME: get rid of this, braxi: WHY?
#define	U_ANIMATION			(1<<25)
#define	U_LOOPSOUND			(1<<26)		// byte/short, index to sounds
#define	U_PACKEDSOLID		(1<<27)
#define	U_RENDERSCALE		(1<<28)
#define	U_RENDERALPHA		(1<<29)
#define	U_RENDERCOLOR		(1<<30)

#define	U_FREE4				(1<<31)
/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute ();

*/

#define	EXEC_NOW	0		// don't return until completed
#define	EXEC_INSERT	1		// insert at current position, but don't run yet
#define	EXEC_APPEND	2		// add to end of the command buffer

void Cbuf_Init (void);
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText (char *text);
// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void Cbuf_InsertText (char *text);
// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.

void Cbuf_ExecuteText (int exec_when, char *text);
// this can be used in place of either Cbuf_AddText or Cbuf_InsertText

void Cbuf_AddEarlyCommands (qboolean clear);
// adds all the +set commands from the command line

qboolean Cbuf_AddLateCommands (void);
// adds all the remaining + commands from the command line
// Returns true if any late commands were added, which
// will keep the demoloop from immediately starting

void Cbuf_Execute (void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

void Cbuf_CopyToDefer (void);
void Cbuf_InsertFromDefer (void);
// These two functions are used to defer any pending commands while a map
// is being loaded

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

*/

typedef void (*xcommand_t) (void);

void	Cmd_Init (void);

void	Cmd_AddCommand (char *cmd_name, xcommand_t function);
void	Cmd_AddCommandCG(char *cmd_name, scr_func_t function);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_stringcmd instead of executed locally
void	Cmd_RemoveCommand (char *cmd_name);
void	Cmd_RemoveClientGameCommands();

qboolean Cmd_Exists (char *cmd_name);
// used by the cvar code to check for cvar / command name overlap

char 	*Cmd_CompleteCommand (char *partial);
// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits

int		Cmd_Argc (void);
char	*Cmd_Argv (int arg);
char	*Cmd_Args (void);
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.

void	Cmd_TokenizeString (char *text, qboolean macroExpand);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void	Cmd_ExecuteString (char *text);
// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console

#ifndef DEDICATED_ONLY
void	Cmd_ForwardToServer (void);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
#endif

/*
==============================================================

CVAR

==============================================================
*/

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
set r_draworder 0	as above, but creates the cvar if not present
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/

extern	cvar_t	*cvar_vars;

cvar_t *Cvar_Get (char *var_name, char *value, int flags);
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags

cvar_t 	*Cvar_Set (char *var_name, char *value);
// will create the variable if it doesn't exist

cvar_t *Cvar_ForceSet (char *var_name, char *value);
// will set the variable even if NOSET or LATCH

cvar_t 	*Cvar_FullSet (char *var_name, char *value, int flags);

void	Cvar_SetValue (char *var_name, float value);
// expands value to a string and calls Cvar_Set

float	Cvar_VariableValue (char *var_name);
// returns 0 if not defined or non numeric

char	*Cvar_VariableString (char *var_name);
// returns an empty string if not defined

char 	*Cvar_CompleteVariable (char *partial);
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

void	Cvar_GetLatchedVars (void);
// any CVAR_LATCHED variables that have been set will now take effect

qboolean Cvar_Command (void);
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)

void 	Cvar_WriteVariables (char *path);
// appends lines containing "set variable value" for all variables
// with the archive flag set to true.

void	Cvar_Init (void);

char	*Cvar_Userinfo (void);
// returns an info string containing all the CVAR_USERINFO cvars

char	*Cvar_Serverinfo (void);
// returns an info string containing all the CVAR_SERVERINFO cvars

extern	qboolean	userinfo_modified;
// this is set each time a CVAR_USERINFO variable is changed
// so that the client knows to send it to the server

/*
==============================================================

NET

==============================================================
*/

// net.h -- quake's interface to the networking layer

#define	PORT_ANY	-1

#define	MAX_MSGLEN		1400		// max length of a message
#define	PACKET_HEADER	10			// two ints and a short, braxi -- unused?

typedef enum {NA_LOOPBACK, NA_BROADCAST, NA_IP } netadrtype_t;

typedef enum {NS_CLIENT, NS_SERVER} netsrc_t;

typedef struct
{
	netadrtype_t	type;
	byte			ip[4];
	unsigned short	port;
} netadr_t;

void		NET_Init (void);
void		NET_Shutdown (void);

void		NET_Config (qboolean multiplayer);

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message);
void		NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to);

qboolean	NET_CompareAdr (netadr_t a, netadr_t b);
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b);
qboolean	NET_IsLocalAddress (netadr_t adr);
char		*NET_AdrToString (netadr_t a);
qboolean	NET_StringToAdr (char *s, netadr_t *a);
void		NET_Sleep(int msec);

//============================================================================

typedef struct
{
	qboolean	fatal_error;

	netsrc_t	sock;

	int			dropped;			// between last packet and previous

	int			last_received;		// for timeouts
	int			last_sent;			// for retransmits

	netadr_t	remote_address;
	int			qport;				// qport value to write when transmitting

// sequencing variables
	int			incoming_sequence;
	int			incoming_acknowledged;
	int			incoming_reliable_acknowledged;	// single bit

	int			incoming_reliable_sequence;		// single bit, maintained local

	int			outgoing_sequence;
	int			reliable_sequence;			// single bit
	int			last_reliable_sequence;		// sequence number of last send

// reliable staging and holding areas
	sizebuf_t	message;		// writing buffer to send to server
	byte		message_buf[MAX_MSGLEN-16];		// leave space for header

// message is copied to this buffer when it is first transfered
	int			reliable_length;
	byte		reliable_buf[MAX_MSGLEN-16];	// unacked reliable message
} netchan_t;

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;
extern	byte		net_message_buffer[MAX_MSGLEN];


void Netchan_Init (void);
void Netchan_Setup (netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);

qboolean Netchan_NeedReliable (netchan_t *chan);
void Netchan_Transmit (netchan_t *chan, int length, byte *data);
void Netchan_OutOfBand (int net_socket, netadr_t adr, int length, byte *data);
void Netchan_OutOfBandPrint (int net_socket, netadr_t adr, char *format, ...);
qboolean Netchan_Process (netchan_t *chan, sizebuf_t *msg);

qboolean Netchan_CanReliable (netchan_t *chan);


/*
==============================================================

CMODEL

==============================================================
*/


#include "../qcommon/qfiles.h"

cmodel_t	*CM_LoadMap (char *name, qboolean clientload, unsigned *checksum);
cmodel_t	*CM_InlineModel (char *name);	// *1, *2, etc

int			CM_NumClusters (void);
int			CM_NumInlineModels (void);
char		*CM_EntityString (void);

// creates a clipping hull for an arbitrary box
int			CM_HeadnodeForBox (vec3_t mins, vec3_t maxs);


// returns an ORed contents mask
int			CM_PointContents (vec3_t p, int headnode);
int			CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles);

trace_t		CM_BoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask);
trace_t		CM_TransformedBoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask,
						  vec3_t origin, vec3_t angles);

byte		*CM_ClusterPVS (int cluster);
byte		*CM_ClusterPHS (int cluster);

int			CM_PointLeafnum (vec3_t p);

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int			CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list,
							int listsize, int *topnode);

int			CM_LeafContents (int leafnum);
int			CM_LeafCluster (int leafnum);
int			CM_LeafArea (int leafnum);

void		CM_SetAreaPortalState (int portalnum, qboolean open);
qboolean	CM_AreasConnected (int area1, int area2);

int			CM_WriteAreaBits (byte *buffer, int area);
qboolean	CM_HeadnodeVisible (int headnode, byte *visbits);

void		CM_WritePortalState (FILE *f);
void		CM_ReadPortalState (FILE *f);

/*
==============================================================

PLAYER MOVEMENT CODE

Common between server and client so prediction matches

==============================================================
*/

#if USE_PMOVE_IN_PROGS == 0
extern float pm_airaccelerate;
void Pmove (pmove_t *pmove);
#endif
/*
==============================================================

FILESYSTEM

==============================================================
*/

void	FS_InitFilesystem (void);
void	FS_SetGamedir (char *dir);
char	*FS_Gamedir (void);
char	*FS_NextPath (char *prevpath);
void	FS_ExecAutoexec (void);

int		FS_FOpenFile (char *filename, FILE **file);
void	FS_FCloseFile (FILE *f);
// note: this can't be called from another DLL, due to MS libc issues

int		FS_LoadFile (char *path, void **buffer);
// a null buffer will just return the file length without loading
// a -1 length is not present

void	FS_Read (void *buffer, int len, FILE *f);
// properly handles partial reads

void	FS_FreeFile (void *buffer);

int		FS_LoadTextFile(char* filename, char** buffer);

void	FS_CreatePath (char *path);


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

float	frand(void);	// 0 ti 1
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
void *Z_TagMalloc (int size, int tag);
void Z_FreeTags (int tag);

char* COM_NewString(char* string, int memtag);
qboolean COM_ParseField(char* key, char* value, byte* basePtr, parsefield_t* f);

void Qcommon_Init (int argc, char **argv);
void Qcommon_Frame (int msec);
void Qcommon_Shutdown (void);

#define MD2_NUMVERTEXNORMALS	162
extern	vec3_t	bytedirs[MD2_NUMVERTEXNORMALS];

#ifndef DEDICATED_ONLY
// this is in the client code, but can be used for debugging from server
void SCR_DebugGraph (float value, vec3_t color);
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

