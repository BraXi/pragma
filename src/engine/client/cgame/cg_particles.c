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

int				cg_numparticles = 0;
cparticle_t		cg_particles[MAX_PARTICLES];


/*
==================
CG_FreeParticle
==================
*/
void CG_FreeParticle(cparticle_t* part)
{
	if (!part || !part->inuse)
	{
		//Com_Error(ERR_DROP, "CG_FreeParticle: not active\n");
		return;
	}
	memset(part, 0, sizeof(cparticle_t));
	part->inuse = false;
	cg_numparticles--;
}

/*
===================
CG_AllocParticle
===================
*/
cparticle_t* CG_AllocParticle()
{
	cparticle_t* part = NULL;

	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		if (!cg_particles[i].inuse)
		{
			part = &cg_particles[i];
		}
	}

	if(part != NULL)
		cg_numparticles ++;


	part->inuse = true;

//	printf("cg_numparticles=%i\n", cg_numparticles);

	return part;
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
	cg_numparticles = 0;

//	for (int i = 0; i < MAX_PARTICLES - 1; i++)
//	{
//		cg_particles[i].next = &cg_particles[i + 1];
//	}
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


	time = 0.000001;

	int i;
	for (i = 0; i < MAX_PARTICLES; i++)
	{
		p = &cg_particles[i];
		if (!p->inuse)
			continue;

		// PMM - added INSTANT_PARTICLE handling for heat beam
		if (p->alphavel != INSTANT_PARTICLE)
		{
			time = (cl.time - p->time) * 0.001;
			alpha = p->alpha + time * p->alphavel;
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

		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;

		V_AddParticle(org, color, alpha, p->size);
		// PMM
		if (p->alphavel == INSTANT_PARTICLE)
		{
			p->alphavel = 0.0;
			p->alpha = 0.0;
		}
	}

//	active_particles = active;
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
CG_AreThereFreeParticles

Returns true if there are any free particles left
===============
*/
qboolean CG_AreThereFreeParticles()
{
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
			p->org[j] = org[j] + (crand() * dirspread);
			p->vel[j] = crand() * 6;
		}
		p->org[2] = org[2] + (rand() % 12);

		p->vel[2] = -20 - (rand() & 6);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = gravity;
		p->alpha = 0.5;

		p->alphavel = alphavel; // -1.0f / (0.1f + frand() * alphavel);
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