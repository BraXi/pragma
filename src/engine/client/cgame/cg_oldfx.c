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

	count = 6;

	for (n = 0; n < count; n++)
	{
		p = CG_ParticleFromPool();
		if (p == NULL)
			return; // no free particles

		p->alpha = 0.6f;
		p->alphaVelocity = -1.0f / (1.0f + frand() * 0.3f);
		Vector2Set(p->size, 17.0f, 17.0f);

		VectorSet(p->color, 0.937255, 0.498039, 0.000000);
		for (j = 0; j < 3; j++)
		{
			if (j == 2)
				p->origin[j] = origin[j] + crand() * 2;
			else
				p->origin[j] = origin[j] + crand() * 9;
			p->velocity[j] = crand() * 5;
		}
		p->velocity[2] = 16;// crand() * -3;
		p->acceleration[2] = -5; // -PARTICLE_GRAVITY;
	}

	count = 3 + (rand() & 2);
	for (n = 0; n < count; n++)
	{
		p = CG_ParticleFromPool();
		if (p == NULL)
			return; // no free particles

		p->alpha = 0.5f;
		p->alphaVelocity = -0.22f;
		
		Vector2Set(p->size, 25.0f, 25.0f);
		VectorSet(p->color, 0.18f, 0.18f, 0.18f);

		for (j = 0; j < 3; j++)
		{
			if (j == 2)
				p->origin[j] = origin[j] + 12.0f + crand() * 12;
			else
				p->origin[j] = origin[j] + crand() * 12;
		}
		p->velocity[2] = 20 + crand() * 5;
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
			p->origin[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->velocity[j] = crand()*20;
		}

		p->acceleration[0] = p->acceleration[1] = 0;
		p->acceleration[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphaVelocity = -1.0 / (0.5 + frand()*0.3);
	}
}




#if 0
/*
===============
CG_PartFX_BloodSplat

Grenade explosion
===============
*/
void CG_PartFX_BloodSplat(vec3_t org)
{
	int i, j;
	cparticle_t* part;

	vec3_t angles = { 90, 0, 0 };
	angles[1] = frand() * 360;

	// decal
	part = CG_CreateOrientedParticle(org, angles, 128.0f, 128.0f, 1.0f, -0.1f, re.RegisterSkin("gfx/fx/blood_splash.tga"));
	if (part == NULL)
		return;

	//part->up[1] = 1;
	//part->right[0] = 1;
	part->type = 1;

	part->origin[2] -= 20;
	VectorSet(part->color, 0.7f, 0.7f, 0.7f);

	// explosion
	part = CG_ParticleFromPool();
	if (part == NULL)
		return;
	part->type = 1;
	VectorCopy(org, part->origin);
	part->origin[2] += 32;
	VectorSet(part->velocity, 0.0f, 0.0f, -6.0f);

	VectorSet(part->color, 1.0f, 1.0f, 1.0f);
	Vector2Set(part->size, 128.0f, 128.0f);

	part->alpha = 1.0f;
	part->alphaVelocity = -0.85f;
	part->tex = re.RegisterSkin("gfx/fx/blood_splash.tga");

	for (i = 0; i < 8; i++)
	{
		part = CG_ParticleFromPool();
		if (part == NULL)
			return; // no free particles
		part->type = 1;
		VectorSet(part->color, 1.0f, 0.67f, 0.027f);
		part->tex = re.RegisterSkin("gfx/fx/blood_splash.tga");

		for (j = 0; j < 3; j++)
		{
			part->origin[j] = org[j] + ((rand() % 32) - 16);
			part->velocity[j] = (rand() % 200) - 100;
		}

		Vector2Set(part->size, 64.0f, 64.0f);
		VectorSet(part->acceleration, 0.0f, 0.0f, -PARTICLE_GRAVITY);

		part->alpha = 0.9;
		part->alphaVelocity = -1.4f;
	}
}
#endif



/*
===============
CG_PartFX_BloodSplat

Grenade explosion
===============
*/
void CG_PartFX_BloodSplat(vec3_t org)
{

	cparticle_t* part;

	vec3_t angles = { 0, 90, 0 };
	angles[1] = frand() * 360;

	//part = FX_CreateOrientedParticle(org, angles);
	part = FX_CreateParticle(org);
	if (part == NULL)
		return;

	part->origin[2] = 20;

	FX_AddTextureStage(part, 1.5f, re.RegisterSkin("gfx/fx/particle_test.tga")); // time, image
	FX_AddTextureStage(part, 1.5f, re.RegisterSkin("gfx/fx/smoke_puff.tga"));
	FX_AddTextureStage(part, 0.5f, re.RegisterSkin("gfx/fx/particle_test.tga"));
	FX_AddTextureStage(part, 0.5f, re.RegisterSkin("gfx/fx/cat.tga"));

	FX_AddTextureStage(part, 0.3f, re.RegisterSkin("gfx/fx/blood_splash.tga"));
	FX_AddTextureStage(part, 0.3f, re.RegisterSkin("gfx/fx/cat.tga"));
	FX_AddTextureStage(part, 0.3f, re.RegisterSkin("gfx/fx/blood_splash.tga"));
	FX_AddTextureStage(part, 0.3f, re.RegisterSkin("gfx/fx/cat.tga"));

	FX_AddTextureStage(part, 0.2f, re.RegisterSkin("gfx/fx/blood_splash.tga"));
	FX_AddTextureStage(part, 0.2f, re.RegisterSkin("gfx/fx/cat.tga"));
	FX_AddTextureStage(part, 0.2f, re.RegisterSkin("gfx/fx/blood_splash.tga"));
	FX_AddTextureStage(part, 0.2f, re.RegisterSkin("gfx/fx/cat.tga"));

	FX_AddTextureStage(part, 0.1f, re.RegisterSkin("gfx/fx/blood_splash.tga"));
	FX_AddTextureStage(part, 0.1f, re.RegisterSkin("gfx/fx/cat.tga"));
	FX_AddTextureStage(part, 0.1f, re.RegisterSkin("gfx/fx/blood_splash.tga"));
	FX_AddTextureStage(part, 0.1f, re.RegisterSkin("gfx/fx/cat.tga"));

	FX_AddTextureStage(part, 0.1f, re.RegisterSkin("gfx/fx/blood_splash.tga"));

	FX_AddColorStage(part, 1.5f, 0.0f, 1.0f, 0.0f); // fadetime, red, gree, blue
	FX_AddColorStage(part, 1.5f, 1.0f, 1.0f, 1.0f);
	FX_AddColorStage(part, 9.0f, 1.0f, 0.0f, 0.0f);

	FX_AddSizeStage(part, 2.0f, 32.0f, 32.0f); // fadetime, width, height
	FX_AddSizeStage(part, 4.0f, 120.0f, 120.0f);
	FX_AddSizeStage(part, 1.5f, 430.0f, 430.0f);

	FX_AddVelocityStage(part, 1.0f, 0.0f, 0.0f, 0.0f); // acceltime, x, y, z
	FX_AddVelocityStage(part, 4.0f, 0.0f, 0.0f, 60.0f);
	FX_AddVelocityStage(part, 4.0f, 0.0f, 0.0f, 0.0f);

	FX_AddAlphaStage(part, 1.0f, 1.0f); // fadetime, alpha
	FX_AddAlphaStage(part, 5.0f, 1.0f);
	FX_AddAlphaStage(part, 1.5f, 0.0f);
}

/*
===============
CG_PartFX_Explosion

Grenade explosion
===============
*/
void CG_PartFX_Explosion(vec3_t org)
{
	int i, j;
	cparticle_t	*part;


	CG_PartFX_BloodSplat(org);
	return;
	// explosion
	part = CG_ParticleFromPool();
	if (part == NULL)
		return;

	VectorCopy(org, part->origin);
	VectorSet(part->velocity, 0.0f, 0.0f, 5.0f);

	VectorSet(part->color, 1.0f, 0.77f, 0.027f);
	Vector2Set(part->size, 96.0f, 96.0f);

	part->alpha = 1.0f;
	part->alphaVelocity = -1.5f;
	part->tex = re.RegisterSkin("gfx/fx/smoke_puff.tga");

	// smoke
	part = CG_ParticleFromPool();
	if (part == NULL)
		return;

	VectorCopy(org, part->origin);
	part->origin[2] += 20.0f;
	VectorSet(part->acceleration, 0.0f, 2.0f, 4.0f);

	VectorSet(part->color, 0.3f, 0.3f, 0.3f);
	Vector2Set(part->size, 100.0f, 100.0f);

	part->alpha = 0.7f;
	part->alphaVelocity = -0.15f;
	part->tex = re.RegisterSkin("gfx/fx/smoke_puff.tga");

#if 1
	// embers
	for (i = 0; i < 12; i++)
	{
		part = CG_ParticleFromPool();
		if (part == NULL)
			return; // no free particles

		VectorSet(part->color, 1.0f, 0.67f, 0.027f);
		part->tex = re.RegisterSkin("gfx/fx/smoke_puff.tga");

		for (j = 0; j < 3; j++)
		{
			part->origin[j] = org[j] + ((rand()%32)-16);
			part->velocity[j] = (rand()%384)-192;
		}

		Vector2Set(part->size, 20.0f, 20.0f);
		VectorSet(part->acceleration, 0.0f, 0.0f, -PARTICLE_GRAVITY);

		part->alpha = 0.8f;
		part->alphaVelocity = -1.2f;
	}
#endif
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

	dec = 8;
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

		if (!CG_NumFreeParticlesInPool(1))
			return;

		// drop less particles as it flies
		if ((rand()&1023) < old->trailcount)
		{
			p = CG_ParticleFromPool();
			if (p == NULL)
				return; // no free particles

			VectorClear (p->acceleration);
			p->time = cl.time;

			p->tex = re.RegisterSkin("gfx/fx/smoke_puff.tga");
			p->size[0] = p->size[1] = 10;

			if (flags & EF_GIB)
			{
				p->alpha = 1.0;
				p->alphaVelocity = -1.0 / (1+frand()*0.4);
				VectorSet(p->color, 0.607843, 0.121569, 0.000000);
				for (j=0 ; j<3 ; j++)
				{
					p->origin[j] = move[j] + crand()*orgscale;
					p->velocity[j] = crand()*velscale;
					p->acceleration[j] = 0;
				}
				p->velocity[2] -= PARTICLE_GRAVITY;
			}
			else if (flags & EF_GREENGIB)
			{
				p->alpha = 1.0;
				p->alphaVelocity = -1.0 / (1+frand()*0.4);
				VectorSet(p->color, 1.000000, 1.000000, 0.325490);
				for (j=0; j< 3; j++)
				{
					p->origin[j] = move[j] + crand()*orgscale;
					p->velocity[j] = crand()*velscale;
					p->acceleration[j] = 0;
				}
				p->velocity[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0;
				p->alphaVelocity = -1.0 / (1+frand()*0.2);
				VectorSet(p->color, 0.247059, 0.247059, 0.247059);
				for (j=0 ; j<3 ; j++)
				{
					p->origin[j] = move[j] + crand()*orgscale;
					p->velocity[j] = crand()*velscale;
				}
				p->acceleration[2] = 20;
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

		if (!CG_NumFreeParticlesInPool(1))
			return;

		if ( (rand()&7) == 0)
		{
			p = CG_ParticleFromPool();
			if (!p)
				return; // no free particles
		
			VectorClear (p->acceleration);
			p->time = cl.time;

			p->alpha = 1.0;
			p->alphaVelocity = -1.0 / (1+frand()*0.2);
			VectorSet(p->color, 1.000000, 1.000000, 0.152941);//p->color = 0xdc + (rand()&3);
			for (j=0 ; j<3 ; j++)
			{
				p->origin[j] = move[j] + crand()*5;
				p->velocity[j] = crand()*20;
			}
			p->acceleration[2] = -PARTICLE_GRAVITY;
		}
		VectorAdd (move, vec, move);
	}
}


