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

typedef struct ui_globalvars_s
{
	int	pad[28];

	float			frametime;
	int				time; 
	float			realtime;

	int				vid_width;
	int				vid_height;

	float			clientState;

	scr_string_t	currentGuiName;

	scr_func_t		main;
	scr_func_t		frame;
	scr_func_t		drawgui;
} ui_globalvars_t;


typedef struct ui_itemvars_s
{
	scr_string_t name;
	// also "rect"
	float		x, y, w, h; 

	// also "rgba"
	vec3_t		color;
	float		alpha;
} ui_itemvars_t;