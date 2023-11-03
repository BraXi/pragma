/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/


#ifndef scr_func_t
	typedef int scr_func_t;
	typedef vec3_t scr_vec_t;
	typedef int scr_entity_t;
	typedef int scr_string_t;
#endif


// prog globals
typedef struct cl_globalvars_s
{
	int	pad[28];

	float			g_time;
	float			g_frameNum;
	float			g_frameTime;

	scr_vec_t		v_forward, v_up, v_right;

	scr_func_t		main;
	scr_func_t		StartFrame;
	scr_func_t		EndFrame;
} cl_globalvars_t;


// gentity_t prog fields
typedef struct cl_entvars_s
{
	// these are copied from entity_state_t
	scr_string_t	str;
	float			var;
} cl_entvars_t;