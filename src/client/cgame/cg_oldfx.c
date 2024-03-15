/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cg_oldfx.c -- old Q2 effects

#include "../client.h"
#include "cg_local.h"

//static vec3_t avelocities [MD2_NUMVERTEXNORMALS];


/*
===============
CG_PartFX_Flame

small flame
===============
*/
void CG_PartFX_Flame(clentity_t* ent, vec3_t origin)
{
	int			n, count;
	int			j;
	cparticle_t* p;

#if 1
	count = 10 + (rand() & 5);

	for (n = 0; n < count; n++)
	{
		p = CG_ParticleFromPool();
		if (p == NULL)
			return; // no free particles

		VectorClear(p->accel);
		p->time = cl.time;

		p->alpha = 0.7;
		p->alphavel = -1.0 / (1 + frand() * 0.2);
		Vector2Set(p->size, 12, 10);
		VectorSet(p->color, 0.937255, 0.498039, 0.000000); // p->color = 226 + (rand() % 4);
		for (j = 0; j < 3; j++)
		{
			if (j == 2)
				p->org[j] = origin[j] + crand() * 2;
			else
				p->org[j] = origin[j] + crand() * 9;
			p->vel[j] = crand() * 5;
		}
		p->vel[2] = 16;// crand() * -3;
		p->accel[2] = -5; // -PARTICLE_GRAVITY;
	}
#endif

	count = 6 + (rand() & 0x7);
	for (n = 0; n < count; n++)
	{
		p = CG_ParticleFromPool();
		if (p == NULL)
			return; // no free particles

		VectorClear(p->accel);

		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frand() * 0.5);
		Vector2Set(p->size, 11, 11);
		VectorSet(p->color, 0.184314, 0.184314, 0.184314); //p->color = 0 + (rand() % 4);
		for (j = 0; j < 3; j++)
		{
			if (j == 2)
				p->org[j] = origin[j] + (rand() & 10);
			else
				p->org[j] = origin[j] + crand() * 8;
		}
		p->vel[2] = 20 + crand() * 5;
	}

}



/*
===============
CG_PartFX_Generic

Wall impact puffs
===============
*/
void CG_PartFX_Generic(vec3_t org, vec3_t dir, vec3_t color, int count)
{
	int			i, j;
	cparticle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		p = CG_ParticleFromPool();
		if (p == NULL)
			return; // no free particles

		p->time = cl.time;
		VectorCopy(color, p->color);

		d = rand()&31;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}

/*
===============
CG_PartFX_Explosion

Grenade explosion
===============
*/
void CG_PartFX_Explosion(vec3_t org)
{
	int			i, j;
	cparticle_t	*p;

	for (i= 0; i < 128; i++)
	{
		p = CG_ParticleFromPool();
		if (p == NULL)
			return; // no free particles


		p->time = cl.time;
		VectorSet(p->color, 1.000000, 0.670588, 0.027451);

#if 0
		if (i == 0)
		{
			VectorCopy(org, p->org);
			Vector2Set(p->size, 96, 96);

			p->accel[0] = p->accel[1] = 0;
			p->accel[2] = -PARTICLE_GRAVITY;
			p->alpha = 1.0;
			p->alphavel = -0.8 / (0.5 + frand() * 0.3);
			continue;
		}
#endif

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%384)-192;
		}

		Vector2Set(p->size, 3, 3);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand()*0.3);
	}
}


/*
===============
CG_PartFX_DiminishingTrail

Used for various diminishing trails
===============
*/
void CG_PartFX_DiminishingTrail (vec3_t start, vec3_t end, clentity_t *old, int flags)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	float		dec;
	float		orgscale;
	float		velscale;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 0.5;
	VectorScale (vec, dec, vec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		if (!CG_AreThereFreeParticles())
			return;

		// drop less particles as it flies
		if ((rand()&1023) < old->trailcount)
		{
			p = CG_ParticleFromPool();
			if (p == NULL || p->next == NULL)
				return; // no free particles

			VectorClear (p->accel);
			p->time = cl.time;

			if (flags & EF_GIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1+frand()*0.4);
				VectorSet(p->color, 0.607843, 0.121569, 0.000000);//p->color = 0xe8 + (rand()&7);
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else if (flags & EF_GREENGIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1+frand()*0.4);
				VectorSet(p->color, 1.000000, 1.000000, 0.325490); // p->color = 0xdb + (rand() & 7);
				for (j=0; j< 3; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1+frand()*0.2);
				VectorSet(p->color, 0.247059, 0.247059, 0.247059); //p->color = 4 + (rand()&7);
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
				}
				p->accel[2] = 20;
			}
		}

		old->trailcount -= 5;
		if (old->trailcount < 100)
			old->trailcount = 100;
		VectorAdd (move, vec, move);
	}
}


/*
===============
CG_PartFX_RocketTrail

Rocket trail
===============
*/
void CG_PartFX_RocketTrail (vec3_t start, vec3_t end, clentity_t *old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p = NULL;
	float		dec;

	// smoke
	CG_PartFX_DiminishingTrail (start, end, old, EF_ROCKET);

	// fire
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 1;
	VectorScale (vec, dec, vec);


	while (len > 0)
	{
		len -= dec;

		if (!CG_AreThereFreeParticles())
			return;

		if ( (rand()&7) == 0)
		{
			p = CG_ParticleFromPool();
			if (!p || p == NULL || !p->next || p->next == NULL)
				return; // no free particles
		
			VectorClear (p->accel);
			p->time = cl.time;

			p->alpha = 1.0;
			p->alphavel = -1.0 / (1+frand()*0.2);
			VectorSet(p->color, 1.000000, 1.000000, 0.152941);//p->color = 0xdc + (rand()&3);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + crand()*5;
				p->vel[j] = crand()*20;
			}
			p->accel[2] = -PARTICLE_GRAVITY;
		}
		VectorAdd (move, vec, move);
	}
}


