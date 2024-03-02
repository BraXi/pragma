/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
	
// q_shared.h -- included first by ALL program modules
#pragma once

#include "../pragma_config.h"

#ifdef _WIN32
// unknown pragmas are SUPPOSED to be ignored, but....
#pragma warning(disable : 4244)     
#pragma warning(disable : 4133)     
#pragma warning(disable : 4136)     
#pragma warning(disable : 4051)     

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)		// truncation from const double to float
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define C_ONLY 1
#if (defined _M_IX86 || defined __i386__) && !defined C_ONLY && !defined __sun__
#define id386	1
#else
#define id386	0
#endif


typedef unsigned char 		byte;
typedef enum {false, true}	qboolean;


#ifndef NULL
#define NULL ((void *)0)
#endif


#ifdef __linux__
// linux hacks
#define min(a,b) ((a < b) ? a : b)
#define max(a,b) ((a > b) ? a : b)
#include <string.h>
#define stricmp strcasecmp

char* _strlwr(char* x);
#endif

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over
#define	ROTATEALL			3		// 3 axis

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		128		// max length of an individual token

#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			128		// max length of a filesystem pathname

//
// per-level limits
//
#define	MAX_CLIENTS			32		// absolute limit, [braxi -- was 256]
#define	MAX_GENTITIES		2048	// must change protocol to increase more
#define	MAX_LIGHTSTYLES		256

#ifdef PROTOCOL_EXTENDED_ASSETS
	#define	MAX_MODELS			1024	// these can be sent over the net as shorts
	#define	MAX_SOUNDS			1024	// so theoretical limit is 23768 for each
#else
	#define	MAX_MODELS			256		// these are sent over the net as bytes
	#define	MAX_SOUNDS			256		// so they cannot be higher than 255
#endif
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings


// game print flags
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages



#define	ERR_FATAL			0		// exit the entire game with a popup window
#define	ERR_DROP			1		// print to console and disconnect from game
#define	ERR_DISCONNECT		2		// don't kill server
	

typedef enum
{
	DP_NONE,
	DP_ALL,
	DP_FS,
	DP_SND,
	DP_REND,
	DP_FX,
	DP_NET,
	DP_SV,
	DP_GAME,
	DP_SCRIPT,
} dprintLevel_t;

// destination class for gi.multicast()
typedef enum
{
MULTICAST_ALL,
MULTICAST_PHS,
MULTICAST_PVS,
MULTICAST_ALL_R,
MULTICAST_PHS_R,
MULTICAST_PVS_R
} multicast_t;

//=============================================

#define DEFAULT_ANIM_RATE 10
#define MAX_ANIMS 32

typedef struct animation_s
{
	char		name[MAX_QPATH];
	int			firstFrame;
	int			numFrames;
	int			lastFrame;
	int			lerpTime; // 1000 / fps
	qboolean	looping;
} animation_t;


typedef struct modeldef_s
{
	int			numAnimations;
	int			numSkins;
	animation_t anims[MAX_ANIMS];
	//animation_t	*anims[MAX_ANIMS]; 
} modeldef_t;

//=============================================

extern char* GetTimeStamp(qboolean full);

#include "mathlib.h"

//=============================================

char *COM_SkipPath (char *pathname);
void COM_StripExtension (char *in, char *out);
void COM_FileBase (char *in, char *out);
void COM_FilePath (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);

char *COM_Parse (char **data_p); // data is an in/out parm, returns a parsed out token
void COM_TokenizeString(char* text);
int COM_TokenNumArgs();
char* COM_TokenGetArg(int arg);
char* COM_TokenArgs();


void Com_sprintf (char *dest, int size, char *fmt, ...);

void Com_PageInMemory (byte *buffer, int size);

//=============================================

// portable case insensitive compare
int Q_stricmp (char *s1, char *s2);
int Q_strcasecmp (char *s1, char *s2);
int Q_strncasecmp (char *s1, char *s2, int n);

//=============================================

#define MakeLittleLong(b1,b2,b3,b4) (((unsigned)(b4)<<24)|((b3)<<16)|((b2)<<8)|(b1)) // Q2PRO

short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);

void	Swap_Init (void);
char	*va(char *format, ...);

//=============================================

//
// key / value info strings
//
#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

char *Info_ValueForKey (char *s, char *key);
void Info_RemoveKey (char *s, char *key);
void Info_SetValueForKey (char *s, char *key, char *value);
qboolean Info_Validate (char *s);

/*
==============================================================

SYSTEM SPECIFIC

==============================================================
*/

extern	int	curtime;		// time returned by last Sys_Milliseconds, FIXME: 64BIT

int		Sys_Milliseconds (void);
void	Sys_Mkdir (char *path);

// large block stack allocation routines
void	*Hunk_Begin (int maxsize);
void	*Hunk_Alloc (int size);
void	Hunk_Free (void *buf);
int		Hunk_End (void);

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

/*
** pass in an attribute mask of things you wish to REJECT
*/
char	*Sys_FindFirst (char *path, unsigned musthave, unsigned canthave );
char	*Sys_FindNext ( unsigned musthave, unsigned canthave );
void	Sys_FindClose (void);


// this is only here so the functions in shared.c and win_shared.c can link
void Sys_Error (char *error, ...);
void Com_Printf (char *msg, ...);


/*
==========================================================

CVARS (console variables)

==========================================================
*/

#ifndef CVAR
#define	CVAR

#define	CVAR_ARCHIVE	1	// set to cause it to be saved to vars.rc
#define	CVAR_USERINFO	2	// added to userinfo  when changed
#define	CVAR_SERVERINFO	4	// added to serverinfo when changed
#define	CVAR_NOSET		8	// don't allow change from console at all,
							// but can be set from the command line
#define	CVAR_LATCH		16	// save changes until server restart
#define	CVAR_CHEAT		32	// set only when cheats are enabled, unused yet

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s
{
	char		*name;
	char		*string;
	char		*latched_string;	// for CVAR_LATCH vars
	int			flags;
	qboolean	modified;	// set each time the cvar is changed
	float		value;
	struct cvar_s *next;
} cvar_t;

#endif		// CVAR

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000	// should never be on a brush, only in game
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has SURF_TRANSxx flag
#define	CONTENTS_LADDER			0x20000000


#define	SURF_LIGHT		0x1		// value will hold the light strength
#define	SURF_SLICK		0x2		// effects game physics
#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture


// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)


// SV_AreaEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2
#define	AREA_PATHNODES	3

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for fast side tests
	byte	signbits;		// signx + (signy<<1) + (signz<<1)
	byte	pad[2];
} cplane_t;


typedef struct cmodel_s
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	int			headnode;
} cmodel_t;

typedef struct csurface_s
{
	char		name[16];
	int			flags;
	int			value;
} csurface_t;

typedef struct mapsurface_s  // used internally due to name len probs //ZOID
{
	csurface_t	c;
	char		rname[32];
} mapsurface_t;

// a trace is returned when a box is swept through the world
typedef struct
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact
	csurface_t	*surface;	// surface hit
	int			contents;	// contents on other side of surface hit
	int			entitynum;	// -1 = nothing hit
	struct gentity_s	*ent;		// not set by CM_*() functions
	struct entity_state_s *clent;		//set by CM_*() functions
} trace_t;



// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum 
{
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// fly move, no clipping to world
	PM_SPECTATOR,	// fly move, clip to world
	PM_DEAD,		// no acceleration or turning
	PM_GIB,			// no acceleration or turning, diferent bbox size
	PM_FREEZE		// cannot move and may hover above ground
} pmtype_t;

#define	PMF_ON_GROUND		(1 << 2)
#define PMF_NO_PREDICTION	(1 << 6)	// temporarily disables prediction (used for grappling hook)

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
	pmtype_t	pm_type;

#if PROTOCOL_FLOAT_COORDS == 1
	vec3_t		origin;
	vec3_t		velocity;

	vec3_t		mins;
	vec3_t		maxs;
#else
	short		origin[3];		// 12.3
	short		velocity[3];	// 12.3

	short		mins[3];	// 12.3
	short		maxs[3];	// 12.3
#endif

	int			pm_flags; 		// ducked, jump_held, etc
	byte		pm_time;		// each unit = 8 ms
	short		gravity;

	// add to command angles to get view direction
	// changed by spawns, rotating objects, and teleporters
	short		delta_angles[3];
} pmove_state_t;

#define PACKED_BSP 31

//
// button bits
//
#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define	BUTTON_RELOAD		4
#define	BUTTON_MELEE		8
#define	BUTTON_ANY			128			// any key whatsoever


// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
	byte	msec;
	byte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	byte	impulse;
} usercmd_t;

typedef struct
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t		cmd;

	vec3_t		viewangles;			// clamped
	float		viewheight;

	vec3_t		mins, maxs;			// bounding box size
} pmove_t;


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity. An entity that has 
// effects will be sent to the client even if it has a zero index model.
#define	EF_ROTATEYAW		1		// rotate
#define	EF_GIB				2		// leave a trail
#define	EF_ROCKET			8		// redlight + trail
#define	EF_GRENADE			16		// tiny smoke trail
#define	EF_ANIM01			64		// automatically cycle between frames 0 and 1 at 2 hz
#define	EF_ANIM23			128		// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			256		// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		512		// automatically cycle through all frames at 10hz
#define EF_GREENGIB			32768	// diminishing green blood trail


// entity_state_t->renderfx flags
#define	RF_MINLIGHT			1		// allways have some light, never go full dark
#define	RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define	RF_VIEW_MODEL		4		// only draw through eyes
#define	RF_FULLBRIGHT		8		// allways draw full intensity
#define	RF_DEPTHHACK		16		// entity can not be occluded by other geometry
#define	RF_TRANSLUCENT		32		// entity uses .renderAlpha
#define	RF_LIGHT			64		// entity has gamecode controlled dynamic light attached
#define RF_BEAM				128
#define	RF_COLOR			256		// entity uses .renderColor, was RF_CUSTOMSKIN
#define	RF_GLOW				512		// pulse lighting for bonus items
#define RF_SCALE			1024	// entity model is scaled by .renderScale
#define	RF_NOANIMLERP		2048	// animation is not lerped (software q1 style)
#define RF_NEGATIVE_LIGHT	4096	// light that substracts from world!
#define RF_YAWHACK			8192

// player_state_t->refdef flags
#define	RDF_UNDERWATER		1		// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2		// used for player configuration screen


//
// muzzle flashes / player effects
//
enum
{
	FX_MUZZLEFLASH_PISTOL,
	FX_MUZZLEFLASH_RIFLE,
	FX_MUZZLEFLASH_SHOTGUN,
	FX_WEAPON_MUZZLEFLASHES,
	FX_MUZZLEFLASH_NEGATIVE_LIGHT,
};


// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed and broadcast.
typedef enum
{
	TE_FLAME,
	TE_GUNSHOT,
	TE_BULLET_SPARKS,
	TE_BLOOD,
	TE_EXPLOSION,
	TE_SPLASH,
	TE_RAIN,
	TE_COUNT
} temp_event_t;

#define SPLASH_UNKNOWN		0
#define SPLASH_SPARKS		1
#define SPLASH_BLUE_WATER	2
#define SPLASH_BROWN_WATER	3
#define SPLASH_SLIME		4
#define	SPLASH_LAVA			5
#define SPLASH_BLOOD		6


// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define	CHAN_AUTO               0
#define	CHAN_WEAPON             1
#define	CHAN_VOICE              2
#define	CHAN_ITEM               3
#define	CHAN_BODY               4
#define	CHAN_2D					5
// modifier flags
#define	CHAN_NO_PHS_ADD			8	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define	CHAN_RELIABLE			16	// send by reliable message, not datagram


// sound attenuation values
#define	ATTN_NONE               0	// full volume the entire level
#define	ATTN_NORM               1
#define	ATTN_IDLE               2
#define	ATTN_STATIC             3	// diminish very rapidly with distance


// player_state->stats[] indexes
enum
{
	STAT_HEALTH,
	STAT_SCORE,

	MAX_STATS = 32 // must change network protocol to increase
};

/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))


//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most 64 (MAX_QPATH) characters.
//
#define	CS_NAME				0		// level name is set in worldspawn's 'message' key

#define	CS_SKY				1		// name of sky image without _<size> postfix
#define	CS_SKYAXIS			2		// %f %f %f XYZ format
#define	CS_SKYROTATE		3		// boolean. does the sky rotate? 
#define	CS_SKYCOLOR			4		// %f %f %f RGB format

#define	CS_HUD				5		// display program string

#define	CS_MAXCLIENTS		30

#define	CS_CHECKSUM_MAP		31		
#define	CS_CHECKSUM_CGPROGS 32
#define	CS_CHEATS_ENABLED	33

#define	CS_MODELS			34
#define	CS_SOUNDS			(CS_MODELS+MAX_MODELS)
#define	CS_IMAGES			(CS_SOUNDS+MAX_SOUNDS)
#define	CS_LIGHTS			(CS_IMAGES+MAX_IMAGES)
#define	CS_ITEMS			(CS_LIGHTS+MAX_LIGHTSTYLES)
#define CS_GENERAL			(CS_ITEMS+MAX_ITEMS)
#define	CS_CLIENTS			(CS_GENERAL+MAX_GENERAL)
#define	MAX_CONFIGSTRINGS	(CS_CLIENTS+MAX_CLIENTS)



//==============================================


// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
typedef enum
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT
} entity_event_t;

typedef struct ent_model_s
{
	short	modelindex;		// index to model
	byte	parentTag;		// parent's tag the model is attached to index is offset by +1
} ent_model_t;

#define MAX_ATTACHED_MODELS 3
typedef struct entity_state_s
{
	short		number;			// edict index

	byte		eType;

	vec3_t		origin;
	vec3_t		angles;
	vec3_t		old_origin;		// for lerping

	short		modelindex;		// main model
	byte		hidePartBits;

	byte		animationIdx;
	unsigned int animStartTime;
	short		frame;
	byte		skinnum;		// byte

	ent_model_t attachments[MAX_ATTACHED_MODELS];

	unsigned int effects;		// PGM - we're filling it, so it needs to be unsigned

	int			renderFlags;	// RF_ flags
	float		renderScale;	// used when renderFlags & RF_SCALE 
	vec3_t		renderColor;	// used when renderFlags & RF_COLOR
	float		renderAlpha;	// used whne renderFlags & RF_TRANSLUCENT

	short		loopingSound;	// for looping sounds, to guarantee shutoff

	byte		event;			// impulse events -- muzzle flashes, footsteps, go out for a single frame, they are automatically cleared each frame

	int			packedSolid;	// entity's bounding box

	// not networked
	int			animSpeed; // 1000 / animFps = msec
	float		animLerp;
} entity_state_t;
//==============================================

#ifndef rdViewFX_t
typedef struct rdViewFX_s
{
	float	blend[4];		// rgba 0-1 full screen blend
	float	blur;			// 0 no blur
	float	contrast;
	float	grayscale;		// 0-1 full grayscale
	float	inverse;		// 1 inverse
	float	noise;
	float	intensity;		// 2 default
	float	gamma;
} rdViewFX_t;
#endif

// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view. There will only be [SV_FPS] player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
typedef struct
{
	pmove_state_t	pmove;		// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t		viewangles;		// for fixed views
	vec3_t		viewoffset;		// add to pmovestate->origin
	vec3_t		kick_angles;	// add to view direction to get render angles set by weapon kicks, pain effects, etc

	vec3_t		viewmodel_angles;
	vec3_t		viewmodel_offset;
	int			viewmodel_index;
	int			viewmodel_frame;

	rdViewFX_t	fx;
//	float		blend[4];		// rgba full screen effect
	
	float		fov;			// horizontal field of view

	int			rdflags;		// refdef flags

	short		stats[MAX_STATS];		// fast status bar updates
} player_state_t;


// ==================
#define DEFAULT_RENDERER "ogl1"
// ==================
