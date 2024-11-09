/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cg_fx.c - visual effects simulation and managment

#include "../client.h"
#include "cg_local.h"

extern int cg_numActiveParticles;
extern cparticle_t cg_particles[MAX_PARTICLES];
extern prTime_t cg_effectsTime;
const vec3_t cg_fxGravityVec = { 0, 0, -1 };

void CG_FreeParticle(cparticle_t* part);

static float LerpFloat(float start, float end, float t)
{
	return start + t * (end - start);
}

/*
===============
FX_AddAlphaStage
===============
*/
void FX_AddAlphaStage(cparticle_t* part, float time, float value)
{
	int num;
	num = part->numAlphaStages;
	if (num >= MAX_FX_STAGES)
		return;

	part->alpha_values[num] = value;
	part->alpha_times[num] = time;
	part->numAlphaStages++;
}

/*
===============
FX_AddSizeStage
===============
*/
void FX_AddSizeStage(cparticle_t* part, float time, float width, float height)
{
	int num;
	num = part->numSizeStages;
	if (num >= MAX_FX_STAGES)
		return;

	Vector2Set(part->size_values[num], width, height);
	part->size_times[num] = time;
	part->numSizeStages++;
}

/*
===============
FX_AddColorStage
===============
*/
void FX_AddColorStage(cparticle_t* part, float time, float r, float g, float b)
{
	int num;
	num = part->numColorStages;
	if (num >= MAX_FX_STAGES)
		return;

	VectorSet(part->color_values[num], r, g, b);
	part->color_times[num] = time;
	part->numColorStages++;
}

/*
===============
FX_AddVelocityStage
===============
*/
void FX_AddVelocityStage(cparticle_t* part, float time, float x, float y, float z)
{
	int num;
	num = part->numVelocityStages;
	if (num >= MAX_FX_STAGES)
		return;

	VectorSet(part->velocity_values[num], x, y, z);
	part->velocity_times[num] = time;
	part->numVelocityStages++;
}

/*
===============
FX_AddTextureStage
===============
*/
void FX_AddTextureStage(cparticle_t* part, float time, struct image_s *pTexture)
{
	int num;
	num = part->numTextureStages;
	if (num >= MAX_FX_TEXTURESTAGES)
		return;

	// if the previous stage has the same texture as the stage we're definining, ignore it
	if (num > 0 && pTexture == part->textures[num - 1])
		return;

	part->textures[num] = pTexture;
	part->texture_times[num] = time;
	part->numTextureStages++;
}

/*
===============
FX_SetParticleAngles
Sets particle rotation so it no rotates towards  viewer.
===============
*/
void FX_SetParticleAngles(cparticle_t* part, const vec3_t angles)
{
	AngleVectors(angles, NULL, part->right, part->up);
	part->flags |= 1;
}

/*
===============
CG_LerpParticleStage
===============
*/
static float CG_LerpParticleStage(const cparticle_t* part, const prTime_t simTime, const int numStages, const float* times, int* stage, int* prevStage)
{
	int stage_time, prevStage_time;
	prTime_t time;
	prTime_t total_duration;
	float lerp;
	int i;
	float t;

	stage_time = prevStage_time = 0;
	*stage = *prevStage = 0;
	time = 0;

	//	for (int j = 0; j < numSegments; j++) 
	//		Com_Printf("times[%d] = %f\n", j, times[j]);

	for (i = 0; i < numStages; i++)
	{
		t = times[i];
		time = (t * 1000);
		stage_time += time;
		if (simTime <= stage_time)
		{
			*stage = i;
			if (i != 0)
			{
				*prevStage = i - 1;
				prevStage_time = stage_time - time;
			}
			break;
		}

		if (i == (numStages - 1))
		{
			if (simTime > stage_time)
			{
				*stage = i;
				*prevStage = i;
				prevStage_time = stage_time;
			}
		}
	}

	lerp = 1.0f;
	if (stage > 0)
	{
		total_duration = (stage_time - prevStage_time);
		lerp = (float)(simTime - prevStage_time) / total_duration;

		if (lerp > 1.0f)
			lerp = 1.0f;
		else if (lerp < 0.0f)
			lerp = 0.0f;
	}

	return lerp;
}

/*
===============
CG_Particle_CalcAlpha
Sets the particle's alpha for current simulation time
===============
*/
static void CG_Particle_CalcAlpha(const cparticle_t* part, const prTime_t simTime, float* alpha)
{
	float lerp;
	int stage, prevStage;

	stage = prevStage = 0;
	lerp = CG_LerpParticleStage(part, simTime, part->numAlphaStages, part->alpha_times, &stage, &prevStage);

	if (prevStage != stage)
		*alpha = LerpFloat(part->alpha_values[prevStage], part->alpha_values[stage], lerp);
	else
		*alpha = part->alpha_values[stage];

	if (*alpha > 1.0f)
		*alpha = 1.0f;
	//Com_Printf("%i/%i, lerp=%.4f, alpha=%.4f\n", segment, segment_prev, lerp, alpha);
}

/*
===============
CG_Particle_CalcSize
Sets the particle's size for current simulation time
===============
*/
static void CG_Particle_CalcSize(const cparticle_t* part, const prTime_t simTime, float* width, float* height)
{
	float lerp;
	int segment, segment_prev;

	segment = segment_prev = 0;

	lerp = CG_LerpParticleStage(part, simTime, part->numSizeStages, part->size_times, &segment, &segment_prev);

	if (segment_prev != segment)
	{
		*width = LerpFloat(part->size_values[segment_prev][0], part->size_values[segment][0], lerp);
		*height = LerpFloat(part->size_values[segment_prev][1], part->size_values[segment][1], lerp);
	}
	else
	{
		*width = part->size_values[segment][0];
		*height = part->size_values[segment][1];
	}
	//Com_Printf("%i/%i, lerp=%.4f, alpha=%.4f\n", segment, segment_prev, lerp, alpha);
}

/*
===============
CG_Particle_GetTextureNumForStage
===============
*/
static int CG_Particle_GetTextureNumForStage(cparticle_t* part, const prTime_t simTime)
{
	// not the fastest method heh
	int stage, stage_prev;

	if (!part->numTextureStages)
		return -1;

	stage = stage_prev = 0;
	CG_LerpParticleStage(part, simTime, part->numTextureStages, part->texture_times, &stage, &stage_prev);
	return stage;
}


/*
===============
CG_Particle_CalcOrigin
Calculates particle's origin taking velocity and gravity into the mix
===============
*/
static void CG_Particle_CalcOrigin(cparticle_t* part, const prTime_t simTime)
{
	int stage, stage_prev;
	//float lerp;
	int i;
	prTime_t time;
	float timeAsFloat;

	stage = stage_prev = 0;
	CG_LerpParticleStage(part, simTime, part->numVelocityStages, part->velocity_times, &stage, &stage_prev);

	if (stage_prev != stage)
	{
		// find the stage's start time
		for (time = 0, i = 0; i < stage; i++)
		{
			time += (part->velocity_times[i] * 1000);
		}

		// see how far into stage's simulation we are
		timeAsFloat = (simTime - time) / (part->velocity_times[stage] * 1000);

		// set velocity
		for (i = 0; i < 3; i++)
		{
			part->velocity[i] = (part->velocity_values[stage][i] * timeAsFloat);
		}
	}

	// add velocity to origin
	VectorMA(part->origin, cls.frametime, part->velocity, part->origin);
	//Com_Printf("%i/%i, vel=[%.2f %.2f %.2f]\n", stage_prev, stage, part->velocity[0], part->velocity[1], part->velocity[2]);
}


/*
===============
CG_Particle_CalcColor
Sets the particle's color for current simulation time
===============
*/
static void CG_Particle_CalcColor(const cparticle_t* part, const prTime_t simTime, float* r, float* g, float* b)
{
	float lerp;
	int stage, prevStage;

	stage = prevStage = 0;
	lerp = CG_LerpParticleStage(part, simTime, part->numColorStages, part->color_times, &stage, &prevStage);

	if (prevStage != stage)
	{
		*r = LerpFloat(part->color_values[prevStage][0], part->color_values[stage][0], lerp);
		*g = LerpFloat(part->color_values[prevStage][1], part->color_values[stage][1], lerp);
		*b = LerpFloat(part->color_values[prevStage][2], part->color_values[stage][2], lerp);
	}
	else
	{
		*r = part->color_values[stage][0];
		*g = part->color_values[stage][1];
		*b = part->color_values[stage][2];
	}
	//Com_Printf("%i/%i, lerp=%.4f, alpha=%.4f\n", segment, segment_prev, lerp, alpha);
}

/*
===============
CG_CalcEstimatedParticleLifeTime
Estimates the life time of a particle.
===============
*/
static void CG_CalcEstimatedParticleLifeTime(cparticle_t* part)
{
	prTime_t total_time, time;
	prTime_t toZeroAlphaTime, toZeroSizeTime;
	int i;

	if (part->lifeTime > 0)
	{
		return; // already calculated
	}

	total_time = 0;
	toZeroAlphaTime = toZeroSizeTime = 9999999999;

	// alpha
	for (time = 0, i = 0; i < part->numAlphaStages; i++)
	{
		time += part->alpha_times[i] * 1000;
		if (part->numAlphaStages > 1 && i == part->numAlphaStages - 1)
		{
			if (part->alpha_values[i] <= 0.0f)
				toZeroAlphaTime = time;
		}
	}
	if (time > total_time)
		total_time = time;

	// size
	for (time = 0, i = 0; i < part->numSizeStages; i++)
	{
		time += part->size_times[i] * 1000;
		if (part->numSizeStages > 1 && i == part->numSizeStages - 1)
		{
			if (part->size_values[i][0] <= 0.01f || part->size_values[i][1] <= 0.01f)
				toZeroSizeTime = time;
		}
	}
	if (time > total_time)
		total_time = time;

	// color
	for (time = 0, i = 0; i < part->numColorStages; i++)
	{
		time += part->color_times[i] * 1000;
	}
	if (time > total_time)
		total_time = time;

	// velocity
	for (time = 0, i = 0; i < part->numVelocityStages; i++)
	{
		time += part->velocity_times[i] * 1000;
	}
	if (time > total_time)
		total_time = time;

	// find out the lowest time from total/alpha/size
	time = total_time;
	if (toZeroAlphaTime < total_time)
		time = toZeroAlphaTime;
	if (toZeroSizeTime < time)
		time = toZeroSizeTime;

	part->lifeTime = time;
	//Com_Printf("Estimated particle life time: %i ms\n", part->lifeTime);
}

/*
===============
CG_GetEffectsTime
Returns the time the effects system is at
===============
*/
prTime_t CG_GetEffectsTime()
{
	return cg_effectsTime;
}

/*
===============
CG_SimulatePragmaParticles
===============
*/
void CG_SimulatePragmaParticles()
{
	cparticle_t* part;
	prTime_t	simTime;
	float		simTimeFloat;
	int			num;

	int			textureNum;
	vec3_t		final_color;
	float		final_alpha;
	vec2_t		final_size;

	if (!cl_paused->value)
	{
		// pause effects when the game is paused
		// effects can be technicaly slown-down or fast forwarded , yay!
		cg_effectsTime += (cls.frametime * 1000);
	}

	for (num = 0; num < MAX_PARTICLES; num++)
	{
		part = &cg_particles[num];

		if (!part->inuse || part->type != 1)
		{
			continue; // unused or using old quake particles system
		}

		if (part->time > CG_GetEffectsTime())
		{		
			continue; // we have to wait for particle to start
		}

		simTime = (CG_GetEffectsTime() - part->time); //(cl.time - part->time);
		simTimeFloat = simTime * 0.001;

		if (!part->lifeTime)
		{
			// if the lifeTime wasn't set explictly, we have to estimate it
			CG_CalcEstimatedParticleLifeTime(part);
		}

		textureNum = CG_Particle_GetTextureNumForStage(part, simTime);

		// remove when life is over or particle has no texture (user error)
		if (part->lifeTime > 0 && simTime > part->lifeTime || textureNum == -1)
		{	
			//Com_Printf("Freed particle\n");
			CG_FreeParticle(part);
			return;
		}

		CG_Particle_CalcColor(part, simTime, &final_color[0], &final_color[1], &final_color[2]);
		CG_Particle_CalcAlpha(part, simTime, &final_alpha);
		CG_Particle_CalcSize(part, simTime, &final_size[0], &final_size[1]);
		CG_Particle_CalcOrigin(part, simTime);

		// Add acceleration
		//accelTime = (simTimeFloat * simTimeFloat);
		//VectorMA(final_origin, accelTime, part->acceleration, final_origin);

		//V_AddParticle(int flags, vec3_t org, vec3_t up, vec3_t right, vec3_t color, float alpha, vec2_t size, struct image_s *tex)
		V_AddParticle(part->flags, part->origin, part->up, part->right, final_color, final_alpha, final_size, part->textures[textureNum]);

	}
}

/*
===============
FX_CreateOrientedParticle
===============
*/
cparticle_t* FX_CreateParticle(const vec3_t origin /*, struct texture_s* pTexture*/)
{
	cparticle_t* part;

	part = CG_ParticleFromPool();
	if (part == NULL)
	{
		return NULL;
	}

	// mark this particle as using the new FX system
	part->time = CG_GetEffectsTime();
	part->type = 1;

	VectorCopy(origin, part->origin);
	//part->tex = pTexture;
	return part;
}

/*
===============
FX_CreateOrientedParticle
===============
*/
cparticle_t* FX_CreateOrientedParticle(const vec3_t origin, const vec3_t angles /*, struct texture_s* pTexture*/)
{
	cparticle_t* part;

	part = FX_CreateParticle(origin /*, pTexture*/);
	if (part == NULL)
	{
		return NULL;
	}

	FX_SetParticleAngles(part, angles);
	return part;
}