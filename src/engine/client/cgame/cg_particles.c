/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"

/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

int cg_numActiveParticles = 0;
cparticle_t cg_particles[MAX_PARTICLES];

prTime_t cg_effectsTime = 0;

void CG_SimulatePragmaParticles();

/*
==================
CG_FreeParticle
==================
*/
void CG_FreeParticle(cparticle_t* part)
{
	if (!part || part && !part->inuse)
	{
		//Com_Error(ERR_DROP, "CG_FreeParticle: not active\n");
		return;
	}

	memset(part, 0, sizeof(cparticle_t));
	//part->inuse = false;
	cg_numActiveParticles--;
}

/*
===============
CG_NumFreeParticlesInPool
Returns true if there are available particles in pool
===============
*/
qboolean CG_NumFreeParticlesInPool(unsigned int count)
{
	if (cg_numActiveParticles >= MAX_PARTICLES)
		return false;

	if (cg_numActiveParticles + count >= MAX_PARTICLES)
		return false;

	return true;
}

/*
===================
CG_AllocParticle
===================
*/
cparticle_t* CG_AllocParticle()
{
	cparticle_t* part = NULL;

	if (!CG_NumFreeParticlesInPool(1))
		return NULL;

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		if (!cg_particles[i].inuse)
		{
			part = &cg_particles[i];
		}
	}

	if (part == NULL)
	{
		return NULL; // no free particles
	}

	cg_numActiveParticles++;

	memset(part, 0, sizeof(*part));
	part->inuse = true;
	part->time = cl.time;

	//printf("cg_numActiveParticles: %i\n", cg_numActiveParticles);

	return part;
}


/*
===============
CG_ParticleFromPool
Grabs particle from pool, returns NULL if all particles are in use
===============
*/

cparticle_t* CG_ParticleFromPool()
{
	return CG_AllocParticle();
}


/*
===============
CG_ClearParticles
Clear all particles
===============
*/
void CG_ClearParticles()
{
	memset(cg_particles, 0, sizeof(cg_particles));
	cg_numActiveParticles = 0;
	cg_effectsTime = 0;
}


/*
===============
CG_SimulateAndAddParticles

Simulate and add to scene all active particles
===============
*/
void CG_SimulateAndAddParticles()
{
	cparticle_t		*p;
	float			alpha;
	float			time, time2;
	vec3_t			org;
	vec3_t			color;

	CG_SimulatePragmaParticles();

	time = 0.000001;

	int i;
	for (i = 0; i < MAX_PARTICLES; i++)
	{
		p = &cg_particles[i];
		if (!p->inuse || p->type != 0)
			continue;

		// PMM - added INSTANT_PARTICLE handling for heat beam
		if (p->alphaVelocity != INSTANT_PARTICLE)
		{
			time = (cl.time - p->time) * 0.001;
			alpha = p->alpha + time * p->alphaVelocity;
			if (alpha <= 0)
			{	
				// faded out
				CG_FreeParticle(p);
				continue;
			}
		}
		else
		{
			alpha = p->alpha;
		}


		if (alpha > 1.0)
			alpha = 1;

		VectorCopy(p->color, color);

		time2 = time * time;

		org[0] = p->origin[0] + p->velocity[0] * time + p->acceleration[0] * time2;
		org[1] = p->origin[1] + p->velocity[1] * time + p->acceleration[1] * time2;
		org[2] = p->origin[2] + p->velocity[2] * time + p->acceleration[2] * time2;

		//V_AddParticle(int flags, vec3_t org, vec3_t up, vec3_t right, vec3_t color, float alpha, vec2_t size, struct image_s *tex)
		V_AddParticle(p->flags, org, p->up, p->right, color, alpha, p->size, p->tex);
		// PMM
		if (p->alphaVelocity == INSTANT_PARTICLE)
		{
			p->alphaVelocity = 0.0;
			p->alpha = 0.0;
		}
	}

//	active_particles = active;
}




/*
===============
CG_GenericParticleEffect2
===============
*/
void CG_GenericParticleEffect2(vec3_t org, vec3_t dir, vec3_t color, int count, int dirspread, float alphavel, float gravity)
{
	int			i, j;
	cparticle_t* p;
	float		d;

	if (!count)
		Com_Error(ERR_DROP, "CG_GenericParticleEffect: !count\n");

	for (i = 0; i < count; i++)
	{
		p = CG_ParticleFromPool();
		if (p == NULL)
			return; // no free particles

		p->time = cl.time;
		VectorCopy(color, p->color);
		d = rand() & dirspread;
		for (j = 0; j < 2; j++)
		{
			p->origin[j] = org[j] + (crand() * dirspread);
			p->velocity[j] = crand() * 6;
		}
		p->origin[2] = org[2] + (rand() % 12);

		p->velocity[2] = -20 - (rand() & 6);

		p->acceleration[0] = p->acceleration[1] = 0;
		p->acceleration[2] = gravity;
		p->alpha = 0.5;

		p->alphaVelocity = alphavel; // -1.0f / (0.1f + frand() * alphaVelocity);
	}
}


/*
=================
CG_ParseParticleEffectCommand

origin [pos]
direction [dir]
color [3x byte]
count [byte]
=================
*/
void CG_ParseParticleEffectCommand()
{
	int		count;
	vec3_t	pos, dir;
	vec3_t	color;

	MSG_ReadPos(&net_message, pos);
	MSG_ReadDir(&net_message, dir);

	color[0] = MSG_ReadByte(&net_message) / 255.0;
	color[1] = MSG_ReadByte(&net_message) / 255.0;
	color[2] = MSG_ReadByte(&net_message) / 255.0;

	count = MSG_ReadByte(&net_message);

	CG_PartFX_Generic(pos, dir, color, count);
}
