/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

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
	rentity_t*			 entities;			// allocated by qcvm
	int					max_entities;
	int					entity_size;		// retrieved from progs
	int					num_edicts;			// increases towards MAX_EDICTS
} cg_t;

extern cg_t cg;

extern cgMedia_t cgMedia;

void CG_ClearState();

void CL_ShutdownClientGame();
void CG_InitClientGame();
void CG_AddEntities();

void CG_Frame(float frametime, int time, float realtime);
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
