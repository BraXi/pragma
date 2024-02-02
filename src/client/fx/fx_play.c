/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "fx_local.h"

static vec3_t fx_v_forward, fx_v_right, fx_v_up;

#define random()	( ( rand() & 0x7fff ) / ( (float)0x7fff ) )
#define crandom()	( 2.0 * ( random() - 0.5 ) )

fx_runner_t* FX_SpawnFX()
{
	int i;
	fx_runner_t* runner = NULL;

	for (i = 0; i < FX_MAX_ACTIVE_EFFECTS; i++)
	{
		if (cl.time > fxSys.fxRunners[i].endTime) // can this be problematic on looping effects??
		{
			memset(&fxSys.fxRunners[i], 0, sizeof(fx_runner_t));
			runner = &fxSys.fxRunners[i];
			runner->entity = -1;
			runner->tag = -1;
			runner->inuse = true;
			break;
		}
	}
	return runner;
}

/*
===============
FX_GetLength

Returns the length of an effect in seconds
===============
*/
float FX_GetLength(int effectIndex)
{
	fxdef_t* fx;
	int i;
	float segmentLifeTime, len = 0.0f;

	if (effectIndex < 0 || effectIndex >= fxSys.numLoadedEffects)
	{
		Com_Printf("WARNING: wrong effect index %i (%s)", effectIndex, __FUNCTION__);
		return len;
	}

	fx = fxSys.fxDefs[effectIndex];
	if (fx == NULL)
		return len; // should likely never happen (bulletproofing editor)

	// particles
	for (i = 0; i < fx->numPartSegments; i++)
	{
		if (fx->part[i] == NULL)
			continue; // should likely never happen (bulletproofing editor)

		segmentLifeTime = (fx->part[i]->delay + fx->part[i]->lifetime);
		if (segmentLifeTime > len)
			len = segmentLifeTime;
	}

	// dlights
	for (i = 0; i < fx->numDLightSegments; i++)
	{
		if (fx->dlight[i] == NULL)
			continue; // should likely never happen (bulletproofing editor)

		segmentLifeTime = (fx->dlight[i]->delay + fx->dlight[i]->lifetime);
		if (segmentLifeTime > len)
			len = segmentLifeTime;
	}

	if (len <= 0.0f)
	{
		Com_Printf("WARNING: effect '%s' has no length!", fx->name);
		return 0.0f;
	}
	return len;
}


/*
===============
FX_StartFX
===============
*/
void FX_StartFX(int effectIndex, vec3_t origin, vec3_t dir)
{
	fx_runner_t* runner;
	float fxlength;

	if (effectIndex < 0 || effectIndex >= fxSys.numLoadedEffects)
	{
		Com_Printf("WARNING: wrong effect index %i (%s)", effectIndex, __FUNCTION__);
		return;
	}

	fxlength = FX_GetLength(effectIndex);
	if(fxlength <= 0)
		return;

	runner = FX_SpawnFX();
	if (runner == NULL)
	{
		Com_Printf("%s: no free fx runners", __FUNCTION__);
		return;
	}


	runner->startTime = cl.time;
	runner->endTime = runner->startTime + (int)(fxlength * 1000);
	runner->effectIndex = effectIndex;
	VectorCopy(origin, runner->origin);
	VectorCopy(dir, runner->dir);
}

void CL_AddEffectParticlesToScene(fx_runner_t* fxrunner)
{
	int r; // runner
	int p; // particle
	int v; // vector
	int cnt; // total count
	fxdef_t* fx;
	fx_particle_def_t* part;

	vec3_t start, velocity;
	int gravity;

	fx = fxSys.fxDefs[fxrunner->effectIndex];
	part = fx->part[0];

	// for each particle segment
	for (r = 0; r < fx->numPartSegments; r++, part++)
	{
		cnt = part->count;
		if (part->addRandomCount > 0)
			cnt += (rand() % part->addRandomCount);

		// for each particle in segment
		for (p = 0; p < cnt; p++)
		{
			// TODO: these should be definitely shared
			// FIXME: origin and velocity should be relative to angles

			//
			// calculate initial position
			// 
			for (v = 0; v < 3; v++)
			{
				start[v] = fxrunner->origin[v] + part->origin[v]; /* * fxrunner->dir[v];*/

				if (part->randomDistrubution[v] > 0.0f)
					start[v] += (crandom() * part->randomDistrubution[v]);
			}

			//
			// calculate initial velocity
			//
			for (v = 0; v < 3; v++)
			{
				velocity[v] = part->velocity[v];
				if(part->addRandomVelocity[v] != 0.0f)
					velocity[v] += (crandom() * part->addRandomVelocity[v]);
			}

			//
			// calculate initial gravity
			//
			gravity = part->gravity;
			if (part->addRandomGravity > 0)
				gravity += (rand() & part->addRandomGravity);

			VectorMA(start, 1, fx_v_forward, start);
			VectorMA(start, 1, fx_v_right, start);
			VectorMA(start, 1, fx_v_up, start);
		}
	}
}

/*
===============
CL_AddEffectsToScene

Add effects to scene
===============
*/
void CL_AddEffectsToScene()
{
	int i;
	fx_runner_t	*fxrunner;
	fxdef_t		*fx;
	rentity_t	rent;


	fxrunner = &fxSys.fxRunners[0];
	for (i = 0; i < FX_MAX_ACTIVE_EFFECTS; i++, fxrunner++)
	{
		if (cl.time > fxrunner->endTime)
			continue;

		AngleVectors(fxrunner->dir, fx_v_forward, fx_v_right, fx_v_up);

		fx = fxSys.fxDefs[fxrunner->effectIndex];

		CL_AddEffectParticlesToScene(fxrunner);

		//memset(&rent, 0, sizeof(rent));
		//void V_AddEntity(rentity_t * ent);
		//void V_AddParticle(vec3_t org, vec3_t color, float alpha);
		//V_AddLight(dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}


/*
===============
FX_StartFXAttachedToEntity
===============
*/
void FX_StartFXAttachedToEntity(int effectIndex, int entityIndex, char *tagName)
{
	ccentity_t* cent;
	fx_runner_t* runner;
	float fxlength;

	if (effectIndex < 0 || effectIndex >= fxSys.numLoadedEffects)
	{
		Com_Printf("WARNING: wrong effect index %i (%s)", effectIndex, __FUNCTION__);
		return;
	}

	if (entityIndex < 0 || entityIndex >= MAX_GENTITIES)
	{
		Com_Printf("WARNING: wrong entity index %i (%s)", entityIndex, __FUNCTION__);
		return;
	}

	fxlength = FX_GetLength(effectIndex);
	if (fxlength <= 0)
		return;

	cent = &cl_entities[entityIndex];
	if (cent->current.modelindex <= 0)
	{
		Com_Printf("WARNING: entity  %i has no model (%s)", entityIndex, __FUNCTION__);
		return;
	}
	
	runner = FX_SpawnFX();
	if (runner == NULL)
	{
		Com_Printf("%s: no free fx runners", __FUNCTION__);
		return;
	}

	runner->startTime = cl.time;
	runner->endTime = runner->startTime + (int)(fxlength * 1000);
	runner->effectIndex = effectIndex;
	VectorCopy(cent->current.origin, runner->origin);

	// fixme: calc direction from tag rotation
	runner->dir[2] = 1; // up	
}

/*
===============
CL_PlayOneShotEffect

Plays effects that will delete itself when its over
===============
*/
void CL_PlayOneShotEffect(char* name, vec3_t origin, vec3_t dir)
{
	int fxindex;

	fxindex = CL_FindEffect(name);
	if (fxindex == -1)
	{
		Com_Printf("%s: effect '%s' is not loaded", __FUNCTION__, name);
		return;
	}

	FX_StartFX(fxindex, origin, dir);
}

/*
=================
CL_ParsePlayFX

SVC_PLAYFX -- [byte] effect_index, [pos] origin, [dir] direction
=================
*/
void CL_ParsePlayFX()
{
	int		effect;
	vec3_t	origin, dir;

	effect = MSG_ReadByte(&net_message);
	if (effect < 0 || effect >= FX_GetNumLoadedEffects())
	{
		Com_Error(ERR_DROP, "%s: wrong effect %i", __FUNCTION__, effect);
		return;
	}

	MSG_ReadPos(&net_message, origin);
	MSG_ReadDir(&net_message, dir);


	FX_StartFX(effect, origin, dir);
}


/*
=================
CL_ParsePlayFXOnTag

SVC_PLAYFXONTAG -- [short] entity_num, [byte] tag_id, [byte] effect_index
=================
*/
void CL_ParsePlayFXOnTag()
{
	int		entity_num, effect_index, tag_index;
	ccentity_t* cent;

	entity_num = MSG_ReadShort(&net_message);
	if (entity_num < 0 || entity_num >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "%s: wrong entity index %i", __FUNCTION__, entity_num);
		return;
	}

	effect_index = MSG_ReadByte(&net_message);
	if (effect_index < 0 || effect_index >= FX_GetNumLoadedEffects())
	{
		Com_Error(ERR_DROP, "%s: wrong effect index %i", __FUNCTION__, effect_index);
		return;
	}

	//cent = &cl_entities[entity_num];
	//cent->current.modelindex;

	tag_index = MSG_ReadByte(&net_message);
	if (tag_index < 0 || tag_index >= FX_GetNumTags())
	{
		Com_Error(ERR_DROP, "%s: wrong tag index %i", __FUNCTION__, tag_index);
		return;
	}
}


/*p->vel[0] = crandom() * 16;
p->vel[1] = crandom() * 16;*/