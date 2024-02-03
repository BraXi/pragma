/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"

/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

cparticle_t		*active_particles, * free_particles;
cparticle_t		particles[MAX_PARTICLES];


/*
===============
CG_ClearParticles

Clear all particles
===============
*/
void CG_ClearParticles()
{
	int		i;

//	memset(particles, 0, sizeof(particles));
	free_particles = &particles[0];
	active_particles = NULL;

	for (i = 0; i < MAX_PARTICLES; i++)
	{
		particles[i].next = &particles[i + 1];
		particles[i].num = i;
	}


	particles[MAX_PARTICLES - 1].next = NULL;
}

/*
===============
CG_SimulateAndAddParticles

Simulate and add to scene all active particles
===============
*/
void CG_SimulateAndAddParticles()
{
	cparticle_t		*p, *next;
	float			alpha;
	float			time, time2;
	vec3_t			org;
	vec3_t			color;
	cparticle_t* active, * tail;

	active = NULL;
	tail = NULL;

	time = 0.000001;

	for (p = active_particles; p; p = next)
	{
		next = p->next;

		// PMM - added INSTANT_PARTICLE handling for heat beam
		if (p->alphavel != INSTANT_PARTICLE)
		{
			time = (cl.time - p->time) * 0.001;
			alpha = p->alpha + time * p->alphavel;
			if (alpha <= 0)
			{	// faded out
				p->next = free_particles;
				free_particles = p;
				continue;
			}
		}
		else
		{
			alpha = p->alpha;
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else
		{
			tail->next = p;
			tail = p;
		}

		if (alpha > 1.0)
			alpha = 1;

		VectorCopy(p->color, color);

		time2 = time * time;

		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;

		V_AddParticle(org, color, alpha);
		// PMM
		if (p->alphavel == INSTANT_PARTICLE)
		{
			p->alphavel = 0.0;
			p->alpha = 0.0;
		}
	}

	active_particles = active;
}

/*
===============
CG_ParticleFromPool

Grabs particle from pool, returns NULL if all particles are in use
===============
*/
cparticle_t* CG_ParticleFromPool()
{
	cparticle_t* p = NULL;

	if (!free_particles)
		return p; // no free particles

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;

	return p;
}

/*
===============
CG_AreThereFreeParticles

Returns true if there are any free particles left
===============
*/
qboolean CG_AreThereFreeParticles()
{
	if (!free_particles)
		return false;
	return true;
}

/*
===============
CG_GenericParticleEffect
===============
*/
void CG_GenericParticleEffect(vec3_t org, vec3_t dir, vec3_t color, int count, int dirspread, float alphavel, float gravity)
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
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = gravity;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (0.5f + frand() * alphavel);
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