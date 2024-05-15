/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

#define MAX_CLIENT_ENTITIES		2048
//
// all loaded media by cgame
//
typedef struct
{
	struct sfx_s* sfx_ricochet[3];
	struct sfx_s* sfx_footsteps[3];
	struct sfx_s* sfx_splash[3];
	struct sfx_s* sfx_explosion[3];

	struct model_s* mod_v_muzzleflash;
	struct model_s* mod_w_flashlight;
	struct model_s* mod_v_flashlight;
	struct model_s* mod_w_muzzleflash;

	struct model_s* impact_small;
} cgMedia_t;

typedef struct
{
	unsigned int	time;

	//
	// qcvm
	//
	qboolean			qcvm_active;
	cl_globalvars_t		*script_globals;	// qcvm globals

	struct clentity_t			*localEntities;			// local (not broadcasted) entities allocated by qcvm
	int					maxLocalEntities;		// number of progs allocated entities
	int					localEntitySize;		// retrieved from progs
	int					numActiveLocalEnts;		// increases towards MAX_CLENTITIES
} cg_t;

extern cg_t cg;

extern cgMedia_t cgMedia;

//
// cg_main.c
//
void CG_ClearState();
void CL_ShutdownClientGame();
void CG_InitClientGame();
void CG_BeginGame();
void CG_AddEntities();

void CG_Frame();
void CG_DrawGUI();

qboolean CG_CanDrawCall();
void CG_ParseCommandFromServer();

#define AF_NOLERP 1
#define AF_LOOPING 2

typedef struct
{
	int		soundindex;
	int		effects;
} anim_event_t;

typedef struct
{
	char			name[MAX_QPATH];

	int				startframe;
	int				numframes;
	int				rate;

	int				flags;

	int				numevents;
	anim_event_t	*events; // [numevents]
} anim_t;

typedef struct
{
	unsigned int starttime;	//sv.time when animation started

	int		startframe;
	int		numframes;
	int		rate;

	int		flags;
} animstate_t;


//extern	cdlight_t	cl_dlights[MAX_DLIGHTS];

typedef unsigned short cs_index;
