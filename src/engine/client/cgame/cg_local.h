/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _PRAGMA_CG_LOCAL_H_
#define _PRAGMA_CG_LOCAL_H_

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
qboolean CG_NumFreeParticlesInPool(unsigned int count);
cparticle_t* CG_ParticleFromPool();
void CG_GenericParticleEffect(vec3_t org, vec3_t dir, vec3_t color, int count, int dirspread, float alphavel, float gravity);
void CG_ParseParticleEffectCommand();


//
// cg_fx.c
// 
void FX_AddAlphaStage(cparticle_t* part, float time, float value);
void FX_AddSizeStage(cparticle_t* part, float time, float width, float height);
void FX_AddColorStage(cparticle_t* part, float time, float r, float g, float b);
void FX_AddVelocityStage(cparticle_t* part, float time, float x, float y, float z);
void FX_AddTextureStage(cparticle_t* part, float time, struct image_s* pTexture);

cparticle_t* FX_CreateParticle(const vec3_t origin);
cparticle_t* FX_CreateOrientedParticle(const vec3_t origin, const vec3_t angles);

prTime_t CG_GetEffectsTime();

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


#endif /*_PRAGMA_CG_LOCAL_H_*/