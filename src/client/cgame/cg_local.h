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
	int		key;				// so entities can reuse same entry
	vec3_t	color;
	vec3_t	origin;
	float	radius;
	float	die;				// stop lighting after this time
	float	decay;				// drop this each second
	float	minlight;			// don't add when contributing less
} cdlight_t;



//
// cg_dlights.c
//
void CG_ClearDynamicLights();
cdlight_t* CG_AllocDynamicLight(int key);
void CL_NewDynamicLight(int key, float x, float y, float z, float radius, float time);


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