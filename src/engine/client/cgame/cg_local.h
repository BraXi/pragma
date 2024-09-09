/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

typedef struct
{
	int		length;
	float	value[3];
	float	map[MAX_QPATH];
} CLightStyle_t;

typedef struct
{
	dLightType_t type;
	int		key;				// so entities can reuse same entry
	vec3_t	color;
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less

	vec3_t	dir;
	float	cutoff;
} cdlight_t;

typedef struct localEntity_s
{
	struct localEntity_s* prev, * next;

//	float			lifeRate;			// 1.0 / (endTime - startTime)

	vec3_t			origin;
	vec3_t			angles;

	rentity_t		refEntity;

} localEntity_t;


//
// cg_dlights.c
//
void CG_ClearDynamicLights();
cdlight_t* CG_AllocDynamicLight(int key);
void CL_NewDynamicPointLight(int key, float x, float y, float z, float radius, float time);
void CL_NewDynamicSpotLight(int key, float x, float y, float z, vec3_t dir, float radius, float cutoff, float time);

//
// cg_lightstyles.c
//
void CG_ClearLightStyles();


//
// cg_particles.c
//
void CG_ClearParticles();
cparticle_t* CG_ParticleFromPool();
qboolean CG_AreThereFreeParticles();
void CG_GenericParticleEffect(vec3_t org, vec3_t dir, vec3_t color, int count, int dirspread, float alphavel, float gravity);
void CG_ParseParticleEffectCommand();


//
// cg_physics.c
//
trace_t CG_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum);
int CG_PointContents(vec3_t point);


//
// cg_localents.c
//
//void CG_InitLocalEntities();
//void CG_FreeEntity(localEntity_t* le);
//localEntity_t* CG_AllocLocalEntity();