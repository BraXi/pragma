/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "client.h"

/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

cparticle_t		*active_particles, * free_particles;
cparticle_t		particles[MAX_PARTICLES];


/*
===============
CL_ClearParticles
===============
*/
void CL_ClearParticles(void)
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
CL_AddParticles
===============
*/
void CL_AddParticles(void)
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




void CL_RunParticleSegment(vec3_t org, vec3_t dir, vec3_t color, int count)
{
	int			i, j;
	cparticle_t* p;
	float		d;

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		VectorCopy(color, p->color);

		d = rand() & 31;
		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() & 7) - 4) + d * dir[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}
