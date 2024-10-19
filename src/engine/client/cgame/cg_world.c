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
====================
CG_HullForEntity
====================
*/
static clipHandle_t CG_HullForEntity(entity_state_t* ent)
{
	clipHandle_t model;
	vec3_t		bmins, bmaxs;
	int capsule;

	// decide which clipping hull to use
	if (ent->packedSolid == PACKED_BSP)
	{
		// explicit hulls in the BSP model
		model = CL_GetClipModel(ent->modelindex);
		if (!model)
		{
			Com_Error(ERR_DROP, "CG_HullForEntity: non BSP model for entity %i\n", ent->number);
			return -1;
		}
		return model;
	}

	// extract bbox size
	MSG_UnpackSolid32(ent->packedSolid, bmins, bmaxs);

	capsule = 0; // FIXME: Q3BSP CAPSULE

	// create a temp hull from bounding box sizes
	return CM_TempBoxModel(bmins, bmaxs, capsule);
}

/*
====================
CG_ClipMoveToEntities
====================
*/
static void CG_ClipMoveToEntities(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum, trace_t* tr)
{
	trace_t		trace;
	int			i, num;
	clipHandle_t headnode;
	float* angles;
	entity_state_t* ent;

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->packedSolid == 0)
			continue; // not solid

		if (ent->number == ignoreEntNum)
			continue; // ignored entity

		if (tr->allsolid)
			return;

		// might intersect
		headnode = CG_HullForEntity(ent);
		if (ent->packedSolid == PACKED_BSP)
			angles = ent->angles;
		else
			angles = vec3_origin;	// boxes don't rotate

		CM_TransformedBoxTrace(&trace, start, end, mins, maxs, headnode, contentsMask, ent->origin, angles, 0);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.clent = ent;
			tr->entityNum = ent->number;
			if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
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

	if (!mins)
		mins = vec3_origin;
	if (!maxs)
		maxs = vec3_origin;

	memset(&trace, 0, sizeof(trace_t));

	// check against world
	CM_BoxTrace(&trace, start, end, mins, maxs, 0, contentsMask, 0);
	if (trace.fraction == 0.0f)
	{
		// blocked by world
		trace.clent = cg.localEntities; // fixme this is not so good but better than null
		trace.entityNum = 0;
		return trace;
	}

	// check all other solid models
	CG_ClipMoveToEntities(start, mins, maxs, end, contentsMask, ignoreEntNum, &trace);

	return trace;
}

/*
====================
CG_PointContents
====================
*/
int	CG_PointContents(vec3_t point)
{
	int			i;
	entity_state_t* ent;
	int			num;
	clipHandle_t clip;
	int			contents;

	contents = CM_PointContents(point, 0);

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->packedSolid != PACKED_BSP) // special value for bmodel
			continue;

		clip = CL_GetClipModel((int)ent->modelindex);
		if (!clip)
			continue;

		contents |= CM_TransformedPointContents(point, clip, ent->origin, ent->angles);
	}

	return contents;
}




#if 0
/*
===============
CG_UnlinkEntity
===============
*/
void CG_UnlinkEntity(clentity_t* ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink(&ent->area);
	ent->area.prev = ent->area.next = NULL;
}
#endif


