/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// client.h -- primary header for client

//define	PARANOID			// speed sapping error checking

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../qcommon/renderer.h"

#include "vid.h"
#include "screen.h"
#include "sound.h"
#include "input.h"
#include "keys.h"
#include "console.h"

#include "./cgame/cgame.h"

#ifdef NEW_GUI
	#include "./ui/gui.h"
#endif

//=============================================================================

typedef enum
{
	XALIGN_LEFT = 0,
	XALIGN_CENTER = 1,
	XALIGN_RIGHT = 2
} UI_AlignX;

//=============================================================================

typedef struct
{
	qboolean		valid;			// cleared if delta parsing was invalid
	int				serverframe;
	int				servertime;		// server time the message is valid for (in msec)
	int				deltaframe;
	byte			areabits[MAX_MAP_AREAS/8];		// portalarea visibility bits
	player_state_t	playerstate;
	int				num_entities;
	int				parse_entities;	// non-masked index into cl_parse_entities array
} frame_t;


typedef struct
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame
									// servertime = serverframe * sv_fps;

	int			trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)

	animstate_t	anim;

	// local entities
	int			number;
	qboolean	inuse;
	cl_entvars_t v;
} clentity_t;


#define	CMD_BACKUP		64	// allow a lot of command backups for very fast systems

//
// the client_state_t structure is wiped completely at every
// server map change
//
typedef struct
{
	int			timeoutcount;

	int			timedemo_frames;
	int			timedemo_start;

	qboolean	refresh_prepped;	// false if on new level or new ref dll
	qboolean	sound_prepped;		// ambient sounds can start
	qboolean	force_refdef;		// vid has changed, so we can't use a paused refdef

	int			parse_entities;		// index (not anded off) into cl_parse_entities[]

	usercmd_t	cmd;
	usercmd_t	cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int			cmd_time[CMD_BACKUP];	// time sent, for calculating pings

#ifdef PROTOCOL_FLOAT_COORDS
	float		predicted_origins[CMD_BACKUP][3];	// for debug comparing against server
#else
	short		predicted_origins[CMD_BACKUP][3];	// for debug comparing against server
#endif

	float		predicted_step;				// for stair up smoothing
	unsigned	predicted_step_time;

	vec3_t		predicted_origin;	// generated by CL_PredictMovement
	vec3_t		predicted_angles;
	vec3_t		prediction_error;

	frame_t		frame;				// received from server
	int			surpressCount;		// number of messages rate supressed
	frame_t		frames[UPDATE_BACKUP];

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t		viewangles;

	int			time;			// this is the time value that the clientis rendering at.  
								// always <= cls.realtime between oldframe and frame
	float		lerpfrac;

	refdef_t	refdef;

	vec3_t		v_forward, v_right, v_up;	// set when refdef.angles is set

	//
	// transient data from server
	//

	//
	// non-gameserver infornamtion
	// FIXME: move this cinematic stuff into the cin_t structure
	FILE		*cinematic_file;
	int			cinematictime;		// cls.realtime for first cinematic frame
	int			cinematicframe;

	//
	// server state information
	//
	qboolean	attractloop;		// running the attract loop, any key will menu
	int			servercount;		// server identification for prespawns
	char		gamedir[MAX_QPATH];
	int			playernum;

	int			muzzleflash;
	int			muzzleflash_frame;
	int			muzzleflash_time;

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];

	//
	// locally derived information from server state
	// indexes must match server indexes
	//
	struct model_s		*model_draw[MAX_MODELS];
	struct cmodel_s		*model_clip[MAX_MODELS];

	struct sfx_s		*sound_precache[MAX_SOUNDS];
	struct image_s		*image_precache[MAX_IMAGES];
} client_state_t;

extern	client_state_t	cl;

/*
==================================================================

the client_static_t structure is persistant through an arbitrary number
of server connections

==================================================================
*/

typedef enum 
{
	CS_UNINITIALIZED,
	CS_DISCONNECTED, 	// not talking to a server
	CS_CONNECTING,		// sending request packets to the server
	CS_CONNECTED,		// netchan_t established, waiting for svc_serverdata
	CS_ACTIVE			// game views should be displayed
} connstate_t;

#if 0
// download type
typedef enum 
{
	dl_none,
	dl_sound,
	dl_pic,
	dl_map,
	dl_maptexture,
	dl_model,
	dl_modeltexture,
	dl_skytexture,
	dl_md5anim
} dltype_t;		
#endif

typedef enum {key_game, key_console, key_message, key_menu} keydest_t;

typedef struct
{
	connstate_t	state;
	keydest_t	key_dest;

	int			framecount;
	int			realtime;			// always increasing, no clamping, etc
	float		frametime;			// seconds since last frame

// screen rendering information
	float		disable_screen;		// showing loading plaque between levels
									// or changing rendering dlls
									// if time gets > 30 seconds ahead, break it
	int			disable_servercount;	// when we receive a frame and cl.servercount
									// > cls.disable_servercount, clear disable_screen

// connection information
	char		servername[MAX_OSPATH];	// name of server from original connect
	float		connect_time;		// for connection retransmits

	int			quakePort;			// a 16 bit value that allows quake servers
									// to work around address translating routers
	netchan_t	netchan;
	int			serverProtocol;		// in case we are doing some kind of version hack

	int			challenge;			// from the server to use for connecting

	FILE		*download;			// file transfer from server
	char		downloadtempname[MAX_OSPATH];
	char		downloadname[MAX_OSPATH];
	int			downloadnumber;
//	dltype_t	downloadtype;		// braxi -- unused but I may find it useful later
	int			downloadpercent;

// demo recording info must be here, so it isn't cleared on level change
	qboolean	demorecording;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	FILE		*demofile;
} client_static_t;

extern client_static_t	cls;

//=============================================================================

//
// cvars
//
extern	cvar_t	*cl_drawviewmodel;
extern	cvar_t	*cl_add_blend;
extern	cvar_t	*cl_add_lights;
extern	cvar_t	*cl_add_particles;
extern	cvar_t	*cl_add_entities;
extern	cvar_t	*cl_predict;
extern	cvar_t	*cl_footsteps;

extern	cvar_t	*cl_upspeed;
extern	cvar_t	*cl_forwardspeed;
extern	cvar_t	*cl_sidespeed;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;

extern	cvar_t	*cl_run;

extern	cvar_t	*cl_anglespeedkey;

extern	cvar_t	*cl_shownet;
extern	cvar_t	*cl_showmiss;
extern	cvar_t	*cl_showclamp;

extern	cvar_t	*lookspring;
extern	cvar_t	*lookstrafe;
extern	cvar_t	*sensitivity;

extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*m_forward;
extern	cvar_t	*m_side;

extern	cvar_t	*freelook;

extern	cvar_t	*cl_paused;
extern	cvar_t	*cl_timedemo;

typedef struct muzzleflash_s
{
	float dlight_radius;
	vec3_t dlight_color;

	float	flashscale;

	float	volume;
	char* sound;
} muzzleflash_t;


extern	clentity_t	cl_entities[MAX_GENTITIES];


// the cl_parse_entities must be large enough to hold UPDATE_BACKUP frames of
// entities, so that when a delta compressed message arives from the server
// it can be un-deltad from the original 
#define	MAX_PARSE_ENTITIES	1024
extern	entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//=============================================================================

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;

void DrawString (int x, int y, char *s);
void DrawAltString (int x, int y, char *s);	// toggle high bit
qboolean	CL_CheckOrDownloadFile (char *filename);

void CL_AddNetGraph();

void CG_PartFX_Generic (vec3_t org, vec3_t dir, vec3_t color, int count);


//=================================================

typedef struct particle_s
{
	struct particle_s* prev, * next;
	qboolean inuse;

	int		time;
	int num; //dbg

	vec3_t		org;
	vec3_t		vel;
	vec3_t		accel;
	vec3_t		color;
	vec2_t		size;
//	float		color;
	float		colorvel;
	float		alpha;
	float		alphavel;
} cparticle_t;

#define	PARTICLE_GRAVITY	40
#define INSTANT_PARTICLE	-10000.0

void CL_ClearTEnts (void);

void CL_DebugTrail (vec3_t start, vec3_t end);
void CL_SmokeTrail (vec3_t start, vec3_t end, int colorStart, int colorRun, int spacing);
void CL_Flashlight (int ent, vec3_t pos);
void CL_ForceWall (vec3_t start, vec3_t end, vec3_t color);
void CG_PartFX_Flame (clentity_t *ent, vec3_t origin);
void CL_GenericParticleEffect (vec3_t org, vec3_t dir, vec3_t color, int count, int dirspread, float alphavel); // unused
void CL_BubbleTrail2 (vec3_t start, vec3_t end, int dist);
void CL_Heatbeam (vec3_t start, vec3_t end);
void CL_ParticleSteamEffect (vec3_t org, vec3_t dir, vec3_t color, int count, int magnitude);
void CL_TrackerTrail (vec3_t start, vec3_t end, vec3_t particleColor);
void CL_Tracker_Explode(vec3_t origin);
void CL_TagTrail (vec3_t start, vec3_t end, vec3_t color);
void CL_Tracker_Shell(vec3_t origin);
void CL_MonsterPlasma_Shell(vec3_t origin);
void CL_ParticleSmokeEffect (vec3_t org, vec3_t dir, vec3_t color, int count, int magnitude);


int CL_ParseEntityBits (unsigned *bits);
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits);
void CL_ParseFrame (void);

void CG_ParseTempEntityCommand (void);
void CL_ParseConfigString (void);
void CG_ParseMuzzleFlashMessage (void);

void CG_LightStyleFromConfigString (int i);

void CG_RunDynamicLights (void);
void CG_RunLightStyles (void);

void CL_AddEntities();
void CG_AddDynamicLights (void);
void CG_AddTempEntities (void);
void CG_AddLightStyles (void);

//=================================================

void CL_PrepRefresh (void);
void CL_RegisterSounds (void);

void CL_Quit_f (void);


//
// cl_main.c
//
extern	refexport_t	re;		// interface to refresh .dll

void CL_Init (void);

void CL_Disconnect (void);
void CL_Disconnect_f (void);
void CL_PingServers_f (void);
void CL_Snd_Restart_f (void);
void CL_RequestNextDownload (void);

//
// cl_input
//
typedef struct
{
	int			down[2];		// key nums holding it down
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// msec down this frame
	int			state;
} kbutton_t;

extern	kbutton_t	in_mlook, in_klook;
extern 	kbutton_t 	in_strafe;
extern 	kbutton_t 	in_speed;

void CL_InitInput (void);
void CL_SendCmd (void);
void CL_ClearState (void);
void CL_ReadPackets (void);
void CL_BaseMove (usercmd_t *cmd);
void IN_CenterView (void);
float CL_KeyState (kbutton_t *key);
char *Key_KeynumToString (int keynum);

//
// cl_demo.c
//
void CL_WriteDemoMessage (void);
void CL_Stop_f (void);
void CL_Record_f (void);

//
// cl_parse.c
//
extern char *svc_strings[256];

void CL_ParseServerMessage (void);
void SHOWNET(char *s);
void CL_Download_f (void);

qboolean CL_CheatsAllowed();

//
// cl_view.c
//
extern int gun_frame;
extern struct model_s *gun_model;

void V_Init (void);
void V_RenderView( float stereo_separation );
void V_AddEntity (rentity_t *ent);
void V_AddDebugPrimitive(debugprimitive_t *obj);
void V_AddParticle (vec3_t org, vec3_t color, float alpha, vec2_t size);
void V_AddPointLight(vec3_t org, float intensity, float r, float g, float b);
void V_AddSpotLight(vec3_t org, vec3_t dir, float intensity, float cutoff, float r, float g, float b);
void V_AddLightStyle (int style, float r, float g, float b);

//
// cl_tent.c
//
void CG_RegisterSounds();
void CG_RegisterMedia();


//
// cl_pred.c
//
void CL_PredictMovement(void);
void CL_CheckPredictionError (void);

//
// cl_fx.c
//
void CG_PartFX_RocketTrail (vec3_t start, vec3_t end, clentity_t *old);


//
// cg_events.c
//
void CG_EntityEvent(entity_state_t *ent);


//
// cg_particles.c
//
void CG_SimulateAndAddParticles();

