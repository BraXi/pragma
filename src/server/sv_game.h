/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define MAX_PERS_FIELDS		64

// link_t is only used for entity area links now
typedef struct link_s
{
	struct link_s* prev, * next;
} link_t;

#define	MAX_ENT_CLUSTERS	16

// edict->v.svflags
#define	SVF_NOCLIENT		1	// don't send entity to clients, even if it has effects
#define	SVF_DEADMONSTER		2	// treat as CONTENTS_DEADMONSTER for collision
#define	SVF_MONSTER			4	// treat as CONTENTS_MONSTER for collision
#define	SVF_NOCULL			8	// entity will be _always_ sent regardless of PVS/PHS
#define	SVF_SINGLECLIENT	16	// send to only one client (.showto must be set to desider player entity number)
#define	SVF_ONLYTEAM		32	// send only to players in matching team team (.showto must match team)

// edict->v.flags
#define	FL_FLY				1	// phys
#define	FL_SWIM				2	// phys implied immunity to drowining
#define FL_TEAMSLAVE		2	// phys

// edict->solid values
typedef enum
{
	SOLID_NOT,				// no interaction with other objects
	SOLID_TRIGGER,			// only touch when inside, after moving
	SOLID_BBOX,				// touch on edge
	SOLID_BSP				// bsp clip, touch on edge
} solid_t;

// entity->v.movetype values
typedef enum
{
	MOVETYPE_NONE,			// never moves
	MOVETYPE_NOCLIP,		// origin and angles change with no interaction
	MOVETYPE_PUSH,			// no clip to world, push on box contact
	MOVETYPE_STOP,			// no clip to world, stops on box contact

	MOVETYPE_WALK,			// gravity
	MOVETYPE_STEP,			// gravity, special edge handling
	MOVETYPE_FLY,
	MOVETYPE_TOSS,			// gravity
	MOVETYPE_FLYMISSILE,	// extra size to monsters
	MOVETYPE_BOUNCE
} movetype_t;

//===============================================================

typedef struct sv_entvars_s sv_entvars_t;
typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

// client data that stays across multiple level loads
typedef struct
{
	char		userinfo[MAX_INFO_STRING];
	char		netname[16];

	qboolean	connected;

	float		saved[MAX_PERS_FIELDS];
} client_persistant_t;

struct gclient_s
{
	player_state_t			ps;		// communicated by server to clients
	int						ping;
	client_persistant_t		pers;

	vec3_t			v_angle;
	pmove_state_t	old_pmove;
};

#define ENTITYNUM_NULL -1
struct gentity_s
{
	entity_state_t		s;
	struct gclient_s	*client;	// NULL if not a player the server expects the first part of gclient_s to be a player_state_t but the rest of it is opaque

	qboolean	inuse;
	int			linkcount;
	link_t		area;				// linked to a division node or leaf	
	int			num_clusters;		// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;			// unused if num_clusters != -1
	int			areanum, areanum2;

	float		freetime;			// time when entity was freed

	// this will hold the original state when EntityStateForClient is used
	entity_state_t		stateBackup;
	qboolean			bEntityStateForClientChanged;

	gentity_t	*teamchain;
	gentity_t	*teammaster;

	sv_entvars_t	v;
};

extern gentity_t	*sv_entity;


extern void Scr_EntityPreThink(gentity_t* self);
extern void Scr_Think(gentity_t* self);
extern void Scr_Event_Impact(gentity_t* self, trace_t* trace);
extern void Scr_Event_Blocked(gentity_t* self, gentity_t* other);
extern void Scr_Event_Touch(gentity_t* self, gentity_t* other, cplane_t* plane, csurface_t* surf);

extern void SV_SpawnEntities(char* mapname, char* entities, char* spawnpoint);

extern void ClientUserinfoChanged(gentity_t* ent, char* userinfo);

extern void ClientCommand(gentity_t* ent);
extern void SV_RunWorldFrame(void);

// savegames stubs
extern void WriteGame(char* filename, qboolean autosave);
extern void ReadGame(char* filename);
extern void WriteLevel(char* filename);
extern void ReadLevel(char* filename);
