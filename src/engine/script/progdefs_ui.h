/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _PRAGMA_PROGDEFS_GUI_H_
#define _PRAGMA_PROGDEFS_GUI_H_

#pragma once

#ifndef scr_func_t
	typedef int32_t scr_func_t;
	typedef int32_t scr_int_t;
	//	typedef vec3_t scr_vec_t;
	typedef int32_t scr_entity_t;
	typedef int32_t scr_string_t;
#endif

typedef struct ui_globalvars_s
{
	int32_t	pad[28];

	float			frametime;
	int32_t			time;
	float			realtime;

	int32_t			vid_width;
	int32_t			vid_height;
	int32_t			scr_width;
	int32_t			scr_height;

	float			clientState; //cs_*
	float			serverState; //ss_*

	float			numServers;

	scr_entity_t	self;
	scr_string_t	currentGuiName;

	scr_func_t		main;
	scr_func_t		frame;
	scr_func_t		Callback_GenericItemDraw;
	scr_func_t		Callback_ParseItemKeyVal;
	scr_func_t		Callback_InitItemDef;
} ui_globalvars_t;


typedef struct ui_itemvars_s
{
	scr_string_t name;
	scr_func_t	drawfunc;

	// also "rect"
	float		x, y, w, h; 

	// also "rgba"
	vec3_t		color;
	float		alpha;
} ui_itemvars_t;

#endif /*_PRAGMA_PROGDEFS_GUI_H_*/