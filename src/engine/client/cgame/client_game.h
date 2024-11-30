/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cgame is the ugliest code i ever wrote, needs serious refactoring, some time, some day...

#pragma once

#define MAX_LOCAL_ENTS			2048 // number of local entities
#define MAX_CLIENT_ENTITIES		(MAX_GENTITIES + MAX_LOCAL_ENTS)



typedef enum
{
	ASSET_MODEL,
	ASSET_IMAGE,
	ASSET_SOUND
} cgAssetType_t;

typedef struct
{
	char name[MAX_QPATH];
	void* ptr;
} cgAssetEntry_t;

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

	unsigned int numImages, numSounds, numModels;
	cgAssetEntry_t	image_list[MAX_IMAGES];
	cgAssetEntry_t  sound_list[MAX_SOUNDS];
	cgAssetEntry_t  model_list[MAX_MODELS];
} cgMedia_t;

typedef struct client_strings_s
{
	scr_string_t free;
	scr_string_t no_class;
	scr_string_t player;
	scr_string_t disconnected;
	scr_string_t worldspawn;
} client_strings_t;

typedef struct
{
	unsigned int		time;

	//
	// qcvm
	//
	qboolean	qcvm_active;
	cl_globalvars_t		*script_globals;	// qcvm globals

	struct clentity_t	*entities;			// both game and local entitie
	int			maxEntities;		// max allowed local entities
	int			entity_size;		// retrieved from progs
	int			numLocalEntities;		// increases towards MAX_CLENTITIES

	client_strings_t cstr;
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

//
// cg_world.c
//

void CG_BuildSolidEntitiesList();

//extern	cdlight_t	cl_dlights[MAX_DLIGHTS];

typedef unsigned short cs_index;
