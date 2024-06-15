/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/


#ifndef scr_func_t
	typedef int32_t scr_func_t;
	typedef int32_t scr_entity_t;
	typedef int32_t scr_string_t;
#endif


// prog globals
typedef struct sv_globalvars_s
{
	int32_t	pad[28];

	scr_entity_t 	worldspawn;
	scr_entity_t	self;
	scr_entity_t	other;

	float			g_time;
	float			g_frameNum;
	float			g_frameTime;
	float			g_intermissionTime;

	int32_t			sv_time;

	vec3_t			v_forward, v_up, v_right;

	float			trace_allsolid, trace_startsolid, trace_fraction, trace_plane_dist;
	vec3_t			trace_endpos, trace_plane_normal;
	scr_entity_t	trace_ent;
	float			trace_entnum;  	// this is tricky, if nothing is hit trace_ent = world, but 
									// trace_entnum will be -1, thats because QC's null entity is world
	int32_t			trace_contents;
	scr_string_t	trace_surface_name;
	float			trace_surface_flags;
	float			trace_surface_value;

	scr_func_t		main;
	scr_func_t		StartFrame;
	scr_func_t		EndFrame;

	scr_func_t 		ClientBeginServerFrame;
	scr_func_t 		ClientEndServerFrame;
	scr_func_t 		ClientThink;
	scr_func_t 		ClientBegin;
	scr_func_t		ClientConnect;
	scr_func_t		ClientDisconnect;
	scr_func_t		ClientCommand;
} sv_globalvars_t;


// gentity_t prog fields
typedef struct sv_entvars_s
{
	// ----------------------------------------------------
	// these are copied to entity_state_t

	float		eType;			// entity type

	vec3_t		origin;
	vec3_t		angles;
	vec3_t		old_origin;		// for lerping

	float		modelindex;		// models
	int32_t		anim;			// index to anim def
	int32_t		animstarttime;	// sv.time when animation started
	float		animFrame;		// current animation frame
	float		skinnum;		// for MD3 this should be index to .skin file
	int32_t		effects;		// PGM - we're filling it, so it needs to be unsigned

	int32_t		renderFlags;	// RF_ flags
	float		renderScale;	// used when renderFlags & RF_SCALE 
	vec3_t		renderColor;	// used when renderFlags & RF_COLOR
	float		renderAlpha;	// used whne renderFlags & RF_TRANSLUCENT

	// used when renderFlags & RF_LIGHT*
	vec3_t		lightColor;		// if RF_LIGHT_NEGATIVE, it will substract this ammount of color
	float		lightRadius;	// in quake units
	float		lightStyle;		// index to lightstyle (see server/ents/lights.q2c:InitLightStyles)

	float		loopsound;		// index to sound from precache_sound() that will constantly loop
	float		loopsound_att;	// sound attenuation, one of ATT_

	float		event;			// impulse events -- muzzle flashes, footsteps, go out for a single frame, they are automatically cleared each frame
	// ----------------------------------------------------

	scr_string_t	classname;	// for spawn functions 
	scr_string_t	model;		// setmodel() sets this

	// physics
	int32_t			linkcount;	// increased each time linkentity() is invoked
	float			solid;		// SOLID_NOT etc
	float			movetype;	// MOVETYPE_NONE etc
	float			gravity;	// [0-1.0]
	int32_t			clipmask;
	int32_t			groundentity_num; // -1 = in air, 0 world, etc..
	int32_t			groundentity_linkcount;
	vec3_t			size;		// DON'T CHANGE! set by linkentity()
	scr_entity_t	owner;		// will not collide with owner

	vec3_t			mins, maxs; //setsize() sets this
	vec3_t			absmin, absmax; // DON'T CHANGE!

	vec3_t			velocity;	// velocity is added to
	vec3_t			avelocity;	// angular velocity

	scr_func_t		blocked;	// void()
	scr_func_t		touch;		// void(float planeDist, vector planeNormal, float surfaceFlags)

	// general
	float			flags;		// FL_* flags
	float			svflags;	// SVF_* flags
	float			showto;		// if (svflags & SVF_SINGLECLIENT) entity will only be sent to this client 
								// if (svflags & SVF_ONLYTEAM) entity will only be sent to clients matching `.team`
	scr_func_t		EntityStateForClient;

	float			nextthink;	// time to run think function
	scr_func_t		think;		// void()
	scr_func_t		prethink;	// void()

	// fixme: rename these two to liquid**?
	float			watertype;	// CONTENTS_WATER etc
	float			waterlevel;	// 0 not in water, 1 touching water (players only: 2 halfway in water, 3 head under water)

	float			health;
	float			team;

	// ai fields
	float			yaw_speed;
	float			ideal_yaw;
//	float			aiflags;
	scr_entity_t	goal_entity;
	float			nodeIndex;

	// client fields
	vec3_t			viewangles;	// was v_angle
	vec3_t			viewoffset;
	vec3_t			kick_angles;
//	float			viewheight; // not used in C code

	vec3_t		viewmodel_angles;
	vec3_t		viewmodel_offset;
	float		viewmodel_index;
	float		viewmodel_frame;

	float		pm_type; //pmtype_t
//	vec3_t		pm_origin;
//	vec3_t		pm_velocity;
	vec3_t		pm_mins;
	vec3_t		pm_maxs;
	float		pm_flags;			// ducked, jump_held, etc
	float		pm_time;			// each unit = 8 ms
	float		pm_gravity;
	vec3_t		pm_delta_angles;	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
} sv_entvars_t;