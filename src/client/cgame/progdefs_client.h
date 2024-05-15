/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/


#ifndef scr_func_t
	typedef int scr_func_t;
//	typedef vec3_t scr_vec_t;
	typedef int scr_entity_t;
	typedef int scr_string_t;
#endif

// prog globals
typedef struct cl_globalvars_s
{
	int	pad[28];

	float			frametime; // seconds since last frame
	int				time; // this is the time value that the clientis rendering at. always <= realtime between oldframe and frame
	int				realtime; // always increasing, no clamping

	int				vid_width;
	int				vid_height;

	float			localplayernum; // packetentity number of local player

	scr_entity_t	self;
	scr_entity_t	other;

	vec3_t			v_forward, v_up, v_right;

	float			trace_allsolid, trace_startsolid, trace_fraction, trace_plane_dist;
	vec3_t			trace_endpos, trace_plane_normal;
	float			trace_entnum;
	int				trace_contents;
	scr_string_t	trace_surface_name;
	float			trace_surface_flags;
	float			trace_surface_value;
	scr_entity_t	trace_ent;

	float			pm_state_pm_type;		// byte pmtype_t
	vec3_t			pm_state_origin;		// floats
	vec3_t			pm_state_velocity;		// floats
	float			pm_state_gravity;		// short
	vec3_t			pm_state_mins;			// char [-127,127 range]
	vec3_t			pm_state_maxs;			// char [-127,127 range]
	float			pm_state_pm_flags;		// byte [0-255]
	float			pm_state_pm_time;		// byte [0-255]
	vec3_t			pm_state_delta_angles;	// shorts, use ANGLE2SHORT/SHORT2ANGLE

	vec3_t			cam_viewangles;			// current camera angles
	vec3_t			cam_viewoffset;			// current camera position

	scr_func_t		CG_Main;
	scr_func_t		CG_Frame;
	scr_func_t		CG_DrawGUI;
	scr_func_t		CG_PlayerMove;			// CG_PlayerMove(vector cmdMove, vector cmdAngles, float cmdMsec)
	scr_func_t		CG_ParseCommandFromServer;		// CG_ParseCommandFromServer(float cmd) // cmd is byte 0-255 limited
} cl_globalvars_t;


typedef struct cl_entvars_s
{
	scr_string_t	classname;
	scr_string_t	model;

	int				modelindex;

	vec3_t			mins;
	vec3_t			maxs;

	vec3_t			origin;
	vec3_t			angles;

	float			scale;
	vec3_t			color;

	float			fullbright;
} cl_entvars_t;