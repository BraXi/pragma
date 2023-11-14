/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// DO. NOT. EDIT.
// structures shared with engine
// this must be the first file to compile .entityo server progs!
// add anything at the *end* of the file, it will help you ma.entityaining parity with current server version

// ========================================================
// begin of engine defs 
// ========================================================

//
// globals
//
	entity 		world;
	entity		self;
	entity		other;

	float		g_time;
	float		g_frameNum;
	float		g_frameTime;
	float		g_intermissionTime;

	vector		v_forward, v_up, v_right;

	float		trace_allsolid, trace_startsolid, trace_fraction, trace_plane_dist;
	vector		trace_endpos, trace_plane_normal;
	entity		trace_ent;
	float		trace_entnum;  		// this is tricky, if nothing is hit trace_ent = world, but 
						// trace_entnum will be -1, thats because QC's null entity is world
	int			trace_contents;
	string		trace_surface_name;
	float		trace_surface_flags;
	float 		trace_surface_value;

	void()		main;
	void()		StartFrame;
	void()		EndFrame;

	void() 		ClientBeginServerFrame;
	void() 		ClientEndServerFrame;
	void(float inButtons, float inImpulse, vector inMove, vector inAngles, float inLightLevel, float inMsecTime)		ClientThink;
	void() 		ClientBegin;
	float()		ClientConnect;
	void()		ClientDisconnect;
	float()		ClientCommand;
void end_sys_globals; // end of struct
//
// entity fields
//
	// ----------------------------------------------------
	// these are copied to entity_state_t
	.vector		origin;			// current origin
	.vector		angles;			// current angles
	.vector		old_origin;		// for lerping

	.float		modelindex;		// main model
	.float		modelindex2;	// attached model
	.float		modelindex3;	// attached model
	.float		modelindex4;	// attached model

	.float		animFrame;		// current animation frame
	.float		skinnum;		// for MD3 this should be index to .skin file, otherwise ignored
	.int		effects;

	.int		renderFlags;	// RF_ flags
	.float		renderScale;	// used when renderFlags & RF_SCALE 
	.vector		renderColor;	// used when renderFlags & RF_COLOR
	.float		renderAlpha;	// used when renderFlags & RF_TRANSLUCENT
	
	// used when renderFlags & RF_LIGHT*
	.vector		lightColor;		// if RF_LIGHT_NEGATIVE, it will substract this ammount of color
	.float		lightRadius;	// in quake units
	.float		lightStyle;		// index to lightstyle (see server/ents/lights.q2c:InitLightStyles)

	.float		loopsound;		// index to sound from precache_sound() that will constantly loop
	.float		loopsound_att;	// sound attenuation, one of ATT_

	.int		event;			// impulse events -- muzzle flashes, footsteps, go out for a single frame, they are automatically cleared each frame
	// ----------------------------------------------------

	.string			classname;	// for spawn functions 
	.string			model;		// setmodel() sets this

	// physics
	.float			solid;		// SOLID_NOT etc
	.float			movetype;	// MOVETYPE_NONE etc
	.float			gravity;	// [0-1.0]
	.int			clipmask;	
	.float			groundentity_num; // -1 = in air, 0 world, etc..
	.float			groundentity_linkcount;
	.vector			size;		// DON'T CHANGE! set by linkentity()
	.entity			owner;		
	.entity			chain;		// DON'T CHANGE!

	.vector			mins, maxs; //setsize() sets this
	.vector			absmin, absmax; // DON'T CHANGE!

	.vector			velocity;	// velocity is added to
	.vector			avelocity;	// angular velocity

	.void()			blocked;	// void()
	.void( float pd, vector pn, float sf ) touch;	// void(float planeDist, vector planeNormal, float surfaceFlags)

	// general
	.float			svflags;	// SVF_* flags
	.float			flags;		// FL_* flags

	.float			nextthink;	// time to run think function
	.void()			think;		// void()
	.void()			prethink;	// void()

	// fixme: rename these two to liquid**?
	.float			watertype;	// CONTENTS_WATER etc
	.float			waterlevel;	// 0 not in water, 1 touching water (players only: 2 halfway in water, 3 head under water)

	.float			health;
	.float			team;

	// client fields
	.vector			v_angle;	
	.float			viewheight;

	// pmove vars
	.float		pm_type; //pmtype_t
	.vector		pm_origin;
	.vector		pm_velocity;
	.vector		pm_mins;
	.vector		pm_maxs;
	.float		pm_flags;			// ducked, jump_held, etc
	.float		pm_time;			// each unit = 8 ms
	.float		pm_gravity;
	.vector		pm_delta_angles;	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
									
void end_sys_fields; // end of struct


// g_local.h: deadflag
float DEAD_NO			= 0;
float DEAD_DYING		= 1;
float DEAD_DEAD			= 2;
float DEAD_RESPAWNABLE	= 3;

// g_local.h: typedef enum damage_t
float DAMAGE_NO			= 0;
float DAMAGE_YES		= 1; // will take damage if hit
float DAMAGE_AIM		= 2; // auto targeting recognizes this

// edict->v.flags
float FL_FLY					= 0x00000001;
float FL_SWIM					= 0x00000002;	// implied immunity to drowining
float FL_IMMUNE_LASER			= 0x00000004;
float FL_INWATER				= 0x00000008;
float FL_GODMODE				= 0x00000010;
float FL_NOTARGET				= 0x00000020;
float FL_IMMUNE_SLIME			= 0x00000040;
float FL_IMMUNE_LAVA			= 0x00000080;
float FL_PARTIALGROUND			= 0x00000100;	// not all corners are valid
float FL_WATERJUMP				= 0x00000200;	// player jumping out of water
float FL_TEAMSLAVE				= 0x00000400;	// not the first on the team
float FL_NO_KNOCKBACK			= 0x00000800;
float FL_POWER_ARMOR			= 0x00001000;	// power armor (if any) is active
float FL_RESPAWN				= 0x80000000;	// used for item respawning
float FL_ONGROUND				= 0x100000000;


// game.h : edict->movetype values
// typedef enum float MOVETYPE_t
float MOVETYPE_NONE			= 0;	// never moves
float MOVETYPE_NOCLIP		= 1;	// origin and angles change with no .entityeraction
float MOVETYPE_PUSH			= 2;	// no clip to world, push on box contact
float MOVETYPE_STOP			= 3;	// no clip to world, stops on box contact
float MOVETYPE_WALK			= 4;	// gravity
float MOVETYPE_STEP			= 5;	// gravity, special edge handling
float MOVETYPE_FLY			= 6;
float MOVETYPE_TOSS			= 7;	// gravity
float MOVETYPE_FLYMISSILE 	= 8;	// extra size to monsters
float MOVETYPE_BOUNCE		= 9;

// game.h : typedef enum solid_t;
float SOLID_NOT			= 0;	// no .entityeraction with other objects
float SOLID_TRIGGER		= 1;	// only touch when inside, after moving
float SOLID_BBOX		= 2;	// touch on edge
float SOLID_BSP			= 3;	// bsp clip, touch on edge


// defines from q_shared.h
enum 
{
	SVC_MUZZLEFLASH = 1,
	SVC_TEMP_ENTITY,
	SVC_LAYOUT,
	SVC_INVENTORY,
};
	
// typedef enum multicast_t
float MULTICAST_ALL 	= 0;
float MULTICAST_PHS		= 1;
float MULTICAST_PVS		= 2;
float MULTICAST_ALL_R	= 3;
float MULTICAST_PHS_R	= 4;
float MULTICAST_PVS_R	= 5;

// pmove flags
float PMF_DUCKED			= 1;
float PMF_JUMP_HELD			= 2;
float PMF_ON_GROUND			= 4;
float PMF_TIME_WATERJUMP	= 8; 	// pm_time is waterjump
float PMF_TIME_LAND			= 16;	// pm_time is time before rejump
float PMF_TIME_TELEPORT		= 32;	// pm_time is non-moving time
float PMF_NO_PREDICTION		= 64;	// temporarily disables prediction (used for grappling hook)

// ========================================================
// end of engine defs 
// ========================================================


// server cvars
float cvar_maxclients;
float cvar_coop;
float cvar_deathmatch;
float cvar_dmflags;
float cvar_skill;

// DAMAGE FLAGS
float DAMAGE_NO_PROTECTION = 2; 	// nothing can stop the damage
float DAMAGE_NO_KNOCKBACK = 4;		// no knockback


//editor keys
.float spawnflags;