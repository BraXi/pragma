/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"


static int cg_numSolidEntities;
static clentity_t* cg_solidEntities[MAX_PARSE_ENTITIES];

/*
====================
CG_BuildSolidEntitiesList
Builds a list of entities that have solidity.
====================
*/
void CG_BuildSolidEntitiesList()
{
	clentity_t* ent;
	entity_state_t* state;
	int	i, num;

	cg_numSolidEntities = 0;

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		if (cg_numSolidEntities == MAX_PARSE_ENTITIES)
		{
			Com_Printf(__FUNCTION__": WARNING! Max solid entities in server frame %i\n", cl.frame.serverframe);
			return;
		}
			

		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		state = &cl_parse_entities[num];
		ent = &cl_entities[state->number];

		if (!ent->current.packedSolid)
		{
			continue;
		}

		cg_solidEntities[cg_numSolidEntities] = ent;
		cg_numSolidEntities++;

		if (state->packedSolid == SOLID_PACKED_BMODEL)
		{
			// brush models use inline models for clip
			//clipHandle_t clip = CL_GetClipModel(state->modelindex);
			//CM_ModelBounds(clip, ent->mins, ent->maxs);
			continue; 
		}

		// other entities use bounding boxes or capsules
		MSG_UnpackSolid32(state->packedSolid, ent->mins, ent->maxs);

	}

	//Com_Printf("cg_numSolidEntities: %i\n", cg_numSolidEntities);
}

/*
====================
CG_ClipHandleForEntity
Returns clipHandle for entity and unpacks bounding box size for non bmodel entities.
====================
*/
static clipHandle_t CG_ClipHandleForEntity(clentity_t* ent)
{
	clipHandle_t clip;
	int			capsule;

	if (ent->current.packedSolid == SOLID_PACKED_BMODEL)
	{
		// Use explicit inline model
		clip = CL_GetClipModel(ent->current.modelindex);
		if (!clip)
		{
			Com_Error(ERR_DROP, __FUNCTION__": non BSP model for entity %i\n", ent->number);
			return -1;
		}
		return clip;
	}

	// Extract bounding box
	//MSG_UnpackSolid32(ent->packedSolid, mins, maxs);


	// create a temp capsule from bounding box sizes if 
	// this is the local player, otherwise create a temp box
	if (ent->number == cl.playernum+1)
		capsule = 1;
	else
		capsule = 0;

	// create a temp hull from bounding box sizes
	return CM_TempBoxModel(ent->mins, ent->maxs, capsule);
}

/*
====================
CG_ClipMoveToEntities
====================
*/
static void CG_ClipMoveToEntities(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum, trace_t* tr)
{
	int				i;
	entity_state_t	*touch;
	trace_t			trace;
	clipHandle_t	clipHandle;
	float			*angles;
	int				capsule;

	capsule = 0; // FIXME: Q3BSP

	trace.entityNum = -1;

	for (i = 0; i < cg_numSolidEntities; i++)
	{
		touch = &cg_solidEntities[i]->current;

		// see if we should ignore this entity
		if (touch->packedSolid == 0)
		{
			continue; // the entity isn't solid
		}

		if (touch->number == ignoreEntNum)
		{
			continue; // don't clip against the ignored ent
		}

		// might intersect, so do an exact clip
		clipHandle = CG_ClipHandleForEntity(cg_solidEntities[i]);

		// only brush models rotate
		if (touch->packedSolid == SOLID_PACKED_BMODEL)
			angles = touch->angles;
		else
			angles = vec3_origin;

		CM_TransformedBoxTrace(&trace, start, end, mins, maxs, clipHandle, contentsMask, touch->origin, angles, capsule);

		if (trace.allsolid || trace.fraction < tr->fraction) 
		{
			trace.entityNum = touch->number;
			*tr = trace;
		}
		else if (trace.startsolid) 
		{
			tr->startsolid = true;
		}
		if (tr->allsolid) 
		{
			return;
		}
	}
}

/*
====================
CG_Trace
====================
*/
trace_t CG_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum)
{
	trace_t	trace;
	int capsule;

	capsule = 0; // FIXME: Q3BSP

	if (!mins)
	{
		mins = vec3_origin;
	}
	if (!maxs)
	{
		maxs = vec3_origin;
	}

	CM_ClearTrace(&trace);

	// clip to world
	CM_BoxTrace(&trace, start, end, mins, maxs, 0, contentsMask, capsule);
	trace.entityNum = (trace.fraction != 1.0) ? 0 : -1; // ENTITYNUM_WORLD : ENTITYNUM_NONE;
	if (trace.fraction == 0.0f)
	{
		// blocked by world
		trace.clent = cg.localEntities; // FIXME: revisit when qcvm gets entity access
		return trace;
	}

	// clip to other solid entities
	CG_ClipMoveToEntities(start, mins, maxs, end, contentsMask, ignoreEntNum, &trace);
	return trace;
}

/*
====================
CG_PointContents

NOTE! Unlike server, clients don't know the entity contents 
for solid bbox entities and will return CONTENTS_BODY for them.
====================
*/
int	CG_PointContents(vec3_t point)
{
	entity_state_t	*ent;
	int				i;
	int				contents, contents_entity;
	clipHandle_t	clip;
	float			*angles;

	// Get base contents from world
	contents = CM_PointContents(point, 0);

	// Or in contents from all the other entities in a packet
	for (i = 0; i < cg_numSolidEntities; i++)
	{
		ent = &cg_solidEntities[i]->current;
	
		// might intersect, so do an exact clip
		clip = CL_GetClipModel((int)ent->modelindex);
		if (!clip)
		{
			continue; // has no clip handle
		}

		// Only brush model entities can rotate
		if (ent->packedSolid == SOLID_PACKED_BMODEL)
		{
			angles = ent->angles; 
		}
		else
		{
			angles = vec3_origin;
		}
			
		contents_entity = CM_TransformedPointContents(point, clip, ent->origin, angles);
		contents |= contents_entity;
	}

	return contents;
}


