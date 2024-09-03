/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _mod_pragma_h_
#define _mod_pragma_h_

/*
MODEL
-----

[pmodel_header_s]
[pmodel_bone_s] * numBones
[pmodel_bonetrans_t] * numBones
[pmodel_vertex_s] * numVertexes
[pmodel_surf_s] * numSurfaces
[pmodel_part_s] * numParts
*/

/*
ANIMATION
---------

[panim_header_s]
[panim_bone_s] * numBones
[panim_bonetrans_s] * numBones * numFrames
[panim_event_s] * numFrames
*/

#define PMODEL_IDENT		(('D'<<24)+('M'<<16)+('R'<<8)+'P') // little-endian "PRMD"
#define PMODEL_VERSION		1

#define PANIM_IDENT			(('N'<<24)+('A'<<16)+('R'<<8)+'P') // little-endian "PRAN"
#define PANIM_VERSION		2

// common
#define PMOD_MAX_SURFNAME 64 // MAX_QPATH
#define PMOD_MAX_MATNAME 64 // MAX_QPATH
#define PMOD_MAX_BONENAME 32
#define PMOD_MAX_HITPARTNAME 32
#define PMOD_MAX_EVENTSTRING 64 // MAX_QPATH


typedef struct pmodel_bone_s
{
	int32_t number;
	int32_t parentIndex;
	char name[PMOD_MAX_BONENAME];

	// hitboxes
	char partname[PMOD_MAX_HITPARTNAME]; // head, arm_upper, torso_lower, hand_left etc..
	vec3_t mins, maxs;
} pmodel_bone_t;

typedef struct pmodel_vertex_s
{
	vec3_t xyz;
	vec3_t normal;
	vec2_t uv;
	int32_t boneId;
	//short boneId;
} pmodel_vertex_t;

typedef struct pmodel_surface_s
{
	char material[PMOD_MAX_MATNAME];
	uint32_t firstVert;
	uint32_t numVerts;

	// reserved for engine use
	int32_t materialIndex;
	int32_t flags;
} pmodel_surface_t;

typedef struct pmodel_part_s
{
	char name[PMOD_MAX_SURFNAME];
	uint32_t firstSurf;
	uint32_t numSurfs;
	int32_t flags; // reserved for engine use
} pmodel_part_t;

typedef struct pmodel_header_s
{
	int32_t ident;
	int32_t version;

	uint32_t flags;

	vec3_t mins, maxs;

	uint32_t numBones;
	uint32_t numVertexes;
	uint32_t numSurfaces;
	uint32_t numParts;
} pmodel_header_t;

typedef struct panim_bone_s
{
	int32_t number;
	int32_t parentIndex;
	char name[PMOD_MAX_BONENAME];
} panim_bone_t;

typedef struct panim_bonetrans_s
{
	int32_t bone;
	vec3_t origin;
	vec3_t rotation;
	float quat[4]; // for engine use
} panim_bonetrans_t;

typedef struct panim_event_s
{
	char str[PMOD_MAX_EVENTSTRING];
} panim_event_t;

typedef struct panim_header_s
{
	int32_t ident;
	int32_t version;

	uint32_t flags;
	uint32_t rate;

	uint32_t numBones;
	uint32_t numFrames;
} panim_header_t;

#endif /*_mod_pragma_h_*/