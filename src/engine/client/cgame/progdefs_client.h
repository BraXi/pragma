/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _PRAGMA_PROGDEFS_CLIENT_H_
#define _PRAGMA_PROGDEFS_CLIENT_H_

#pragma once

#ifndef scr_func_t
	typedef int32_t scr_func_t;
//	typedef vec3_t scr_vec_t;
	typedef int32_t scr_entity_t;
	typedef int32_t scr_string_t;
#endif

// prog globals
typedef struct cl_globalvars_s
{
	int32_t	pad[28];

	// seconds since previous frame
	float			frametime; 

	// this is the time value that the client is rendering at. always <= realtime between oldframe and frame
	int32_t			time;

	// always increasing, no clamping
	int32_t			realtime;

	int32_t			vid_width;
	int32_t			vid_height;

	// entity number of a local player
	float			localplayernum;

	scr_entity_t	self;
	scr_entity_t	other;

	// set by makevectors()
	vec3_t			v_forward;
	vec3_t			v_up;
	vec3_t			v_right;

	// set by trace*() functions
	float			trace_allsolid;
	float			trace_startsolid;
	float			trace_fraction;
	float			trace_planedist;
	vec3_t			trace_normal;
	vec3_t			trace_endpos;
	int32_t			trace_contents;
	int32_t			trace_flags;
	scr_entity_t	trace_entity;
	float			trace_entitynum;
	scr_string_t	trace_material;

	// player movement vars for prediction
	float			pm_state_pm_type;		// byte pmtype_t
	vec3_t			pm_state_origin;		// floats
	vec3_t			pm_state_velocity;		// floats
	float			pm_state_gravity;		// short
	vec3_t			pm_state_mins;			// char [-127,127 range]
	vec3_t			pm_state_maxs;			// char [-127,127 range]
	float			pm_state_pm_flags;		// byte [0-255]
	float			pm_state_pm_time;		// byte [0-255]
	vec3_t			pm_state_delta_angles;	// shorts, use ANGLE2SHORT/SHORT2ANGLE
	vec3_t			pm_state_viewangles;	// view angles for locked views (cinematics, intermissions, dead..)

	// camera params - by default C sets these, CalcViewValues() can override
	vec3_t			cam_origin;
	vec3_t			cam_angles;
	float			cam_fov;
	vec3_t			cam_forward;			// derived from cam_angles
	vec3_t			cam_up;					// derived from cam_angles
	vec3_t			cam_right;				// derived from cam_angles

	// Called when client progs are first initialized, every time
	// client begins connecting to server, and at every map change.
	scr_func_t		CG_Main;

	// Called once a client frame
	scr_func_t		CG_Frame;

	// 2D drawing can be done in this function.
	scr_func_t		CG_DrawGUI;

	// 3D drawing can be done in this function.
	scr_func_t		CG_DrawScene;

	// CG_PlayerMove(vector cmdMove, vector cmdAngles, float cmdMsec)
	scr_func_t		CG_PlayerMove;  // void(vector cmdMove, vector cmdAngles, float inButtons, float cmdMsec) CG_PlayerMove;

	// CG_ParseCommandFromServer(float cmd) 
	// cmd is byte 0-255 limited
	scr_func_t		CG_ParseCommandFromServer;

	// Allows client game progs modify camera parms. This is called before entities are added to scene.
	// Should return true to override params set by C code.
	scr_func_t		CalcViewValues; // float() CalcViewValues;

	// Called when the client finalizes connection process and is put in game.
	//scr_func_t		BeginGame;

	// Called when an entity in latest server frame has an event.
	scr_func_t		EntityEvent;
} cl_globalvars_t;


typedef struct cl_entvars_s
{
	scr_string_t	classname;

	scr_string_t	model;
	int32_t			modelindex;

	vec3_t			mins;
	vec3_t			maxs;

	vec3_t			origin;
	vec3_t			angles;

	float			scale;
	vec3_t			color;

	float			fullbright;
} cl_entvars_t;

#endif /*_PRAGMA_PROGDEFS_CLIENT_H_*/