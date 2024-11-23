/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// world.c -- world query functions

#include "server.h"

#ifdef __linux__
#include <limits.h>
#endif

#define MAX_TOTAL_ENT_LEAFS 128

gentity_t *sv_entity; // currently run entity

void Scr_ClientBeginServerFrame(gentity_t* self);
void Scr_ClientEndServerFrame(gentity_t* ent);

void SV_ScriptStartFrame();
void SV_ScriptEndFrame();

void SV_CheckGround(gentity_t* ent);

void SV_ProgVarsToEntityState(gentity_t* ent);
//void SV_EntityStateToProgVars(gentity_t* ent, entity_state_t* state);

//======================================================================

/*
=================
SV_EndWorldFrame
=================
*/
void SV_EndWorldFrame(void)
{
	int		i;
	gentity_t* ent;

	// build the playerstate_t structures for all players
	for (i = 1; i <= svs.max_clients; i++)
	{
		ent = EDICT_NUM(i);

		if (!ent->inuse || !ent->client)
			continue;

		Scr_ClientEndServerFrame(ent);
	}

	// build entity_state_t structure for all server entities
	for (i = 0; i < sv.max_edicts; i++)//sv.num_edicts
	{
		ent = EDICT_NUM(i);
		if (!ent->inuse)
			continue;

		SV_ProgVarsToEntityState(ent);

	}
	SV_ScriptEndFrame();
}

void M_CheckGround(gentity_t* ent)
{
	vec3_t		point;
	trace_t		trace;

	if (ent->v.flags & (FL_SWIM | FL_FLY))
		return;

	if (ent->v.velocity[2] > 100)
	{
		ent->v.groundentity_num = ENTITYNUM_NULL;
		return;
	}

	// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->v.origin[0];
	point[1] = ent->v.origin[1];
	point[2] = ent->v.origin[2] - 0.25;

	trace = SV_Trace(ent->v.origin, ent->v.mins, ent->v.maxs, point, ent, MASK_MONSTERSOLID, false);

	// check steepness
	if (trace.plane.normal[2] < 0.7 && !trace.startsolid)
	{
		ent->v.groundentity_num = ENTITYNUM_NULL;
		return;
	}

	//	ent->groundentity = trace.ent;
	//	ent->groundentity_linkcount = trace.ent->linkcount;
	//	if (!trace.startsolid && !trace.allsolid)
	//		VectorCopy (trace.endpos, ent->s.origin);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(trace.endpos, ent->s.origin);
		ent->v.groundentity_num = trace.entityNum;
		ent->v.groundentity_linkcount = trace.ent->v.linkcount;
		
		ent->v.velocity[2] = 0;
	}
}



/*
================
SV_RunWorldFrame
Advances the world by SV_FRAMETIME seconds
================
*/

void SV_RunWorldFrame(void)
{
	int		i;
	gentity_t* ent;

	sv.gameFrame++;
	sv.gameTime = sv.gameFrame * SV_FRAMETIME;

	if (sv.gameFrame >= (INT_MAX - 100))
	{
		Com_Error(ERR_DROP, "sv.gameFrame is about to overflow, this is due to server runing for a long period of time\n");
		return;
	}

	SV_ScriptStartFrame();

	// run prethink!
	for (i = 0; i < sv.max_edicts; i++)
	{
		ent = EDICT_NUM(i);
		if (!ent->inuse)
			continue;

		sv_entity = ent;
		Scr_EntityPreThink(ent);
	}

	for (i = 0; i < sv.max_edicts; i++)
	{
		ent = EDICT_NUM(i);
		if (!ent->inuse)
			continue;

		sv_entity = ent;

		VectorCopy(ent->v.origin, ent->v.old_origin);

		gentity_t* groundentity = (ent->v.groundentity_num == ENTITYNUM_NULL) ? NULL : ENT_FOR_NUM((int)ent->v.groundentity_num);
//		groundentity = ent->groundentity;
		
		// braxi -- fixme
		// looks like players have wrong: groundentity->linkcount != (int)ent->v.groundentity_linkcount
		// there's a problem to fix, qc has no access to ->linkcount variable, thus making most of .groundentity_linkcount invaild
		// solved
		// if the ground entity moved, make sure we are still on it
		if(groundentity != NULL) 
		{
			if ((groundentity->v.linkcount != (int)ent->v.groundentity_linkcount))
			{
				ent->v.groundentity_num = ENTITYNUM_NULL;

#if 0
				if (ent->v.solid != SOLID_NOT && !(ent->v.flags & (FL_SWIM | FL_FLY)))
					SV_CheckGround(ent); // temporary fix

				if(!(ent->v.flags & (FL_SWIM | FL_FLY)) && ent->v.svflags & SVF_MONSTER)
				{
					M_CheckGround(ent);
				}
#endif
			}
		}
		if (i > 0 && i <= svs.max_clients)
		{
			Scr_ClientBeginServerFrame(ent);
			continue;
		}

		SV_RunEntity(ent);
	}
	SV_EndWorldFrame();
}

/*
===============================================================================

ENTITY AREA CHECKING

FIXME: this use of "area" is different from the bsp file use
===============================================================================
*/

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,centity_t,order)
// FIXME: remove this mess!
//#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - offsetof(t,m)))

#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,gentity_t,area)

typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	link_t	trigger_ents;
	link_t	solid_ents;
	link_t	pathnode_ents;
} areanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

areanode_t	sv_areanodes[AREA_NODES];
int			sv_numareanodes;

float	*area_mins, *area_maxs;
gentity_t	**area_list;
int		area_count, area_maxcount;
int		area_type;


// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

/*
===============
SV_CreateAreaNode

Builds a uniformly subdivided tree for the given world size
===============
*/
areanode_t *SV_CreateAreaNode (int depth, vec3_t mins, vec3_t maxs)
{
	areanode_t	*anode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes];
	sv_numareanodes++;

	ClearLink (&anode->trigger_ents);
	ClearLink (&anode->solid_ents);
	ClearLink (&anode->pathnode_ents);
	
	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}
	
	VectorSubtract (maxs, mins, size);
	if (size[0] > size[1])
		anode->axis = 0;
	else
		anode->axis = 1;
	
	anode->dist = 0.5 * (maxs[anode->axis] + mins[anode->axis]);
	VectorCopy (mins, mins1);	
	VectorCopy (mins, mins2);	
	VectorCopy (maxs, maxs1);	
	VectorCopy (maxs, maxs2);	
	
	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;
	
	anode->children[0] = SV_CreateAreaNode (depth+1, mins2, maxs2);
	anode->children[1] = SV_CreateAreaNode (depth+1, mins1, maxs1);

	return anode;
}

/*
===============
SV_ClearWorld
===============
*/
void SV_ClearWorld (void)
{
	vec3_t mins, maxs;

	memset (sv_areanodes, 0, sizeof(sv_areanodes));
	sv_numareanodes = 0;

	CM_ModelBounds(0, mins, maxs); 
	SV_CreateAreaNode (0, mins, maxs);
}

/*
===============
SV_PackSolid32
Compresses bounding box using Q2PRO's MSG_PackSolid32_Ver2 method.
===============
*/
static int SV_PackSolid32(gentity_t* ent)
{
	int packedsolid;

	packedsolid = MSG_PackSolid32(ent->v.mins, ent->v.maxs); 

	if (packedsolid == SOLID_PACKED_BMODEL)
		packedsolid = 0;  // can happen in pathological case if z mins > maxs

	if (sv_debug->value)
	{
		vec3_t mins, maxs;

		MSG_UnpackSolid32(packedsolid, mins, maxs);// // Q2PRO's MSG_UnpackSolid32_Ver2 for those curious

		if (!VectorCompare(ent->v.mins, mins) || !VectorCompare(ent->v.maxs, maxs))
			Com_Printf(__FUNCTION__": bad mins/maxs on entity %d\n", ent->s.number);
	}

	return packedsolid;
}

/*
===============
SV_UnlinkEntity
===============
*/
void SV_UnlinkEntity(gentity_t *ent)
{
	if (!ent->area.prev)
	{
		return; // not linked in anywhere
	}

	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
}

/*
===============
SV_LinkEntity
===============
*/

void SV_LinkEntity(gentity_t *ent)
{
	areanode_t *node;
	int leafs[MAX_TOTAL_ENT_LEAFS];
	int cluster, area, num_leafs, lastLeaf;
	int max, i;
	
#ifdef _DEBUG
	if (!ent)
	{
		SV_Error(__FUNCTION__": NULL ent");
	}
#endif

	if (ent == sv.edicts || !ent->inuse)
	{
		return; // don't add the world and unused entities
	}

	if (ent->area.prev)
	{
		SV_UnlinkEntity(ent); // unlink from old position
	}

	// set the size
	VectorSubtract (ent->v.maxs, ent->v.mins, ent->v.size);

	// encode the size into the entity_state_t for client prediction
	ent->s.packedSolid = SOLID_NOT;
	switch ((int)ent->v.solid)
	{
	case SOLID_BBOX:
	case SOLID_CAPSULE:
		if (ent->v.contents & ( CONTENTS_SOLID | CONTENTS_BODY ) && !VectorCompare(ent->v.mins, ent->v.maxs))
		{
			ent->s.packedSolid = SV_PackSolid32(ent);
		}
		break;

	case SOLID_BSP:	
		ent->s.packedSolid = SOLID_PACKED_BMODEL;
		break;

	default:
		ent->s.packedSolid = SOLID_NOT;
		break;
	}

	// set the abs box
	if ((ent->v.solid == SOLID_BSP || ent->v.solid == SOLID_TRIGGER) && !VectorCompare(ent->v.angles, vec3_origin))
	{	
		// expand for rotation
		max = RadiusFromBounds(ent->v.mins, ent->v.maxs);
		for (i = 0; i < 3; i++) 
		{
			// add one pixel
			ent->v.absmin[i] = ent->v.origin[i] - max;
			ent->v.absmax[i] = ent->v.origin[i] + max;
		}
	}
	else
	{	
		VectorAdd (ent->v.origin, ent->v.mins, ent->v.absmin);	
		VectorAdd (ent->v.origin, ent->v.maxs, ent->v.absmax);
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	for (i = 0; i < 3; i++)
	{
		ent->v.absmin[i] -= 1;
		ent->v.absmax[i] += 1;
	}

	// link to PVS leafs
	ent->numClusters = 0;
	ent->lastCluster = 0;
	ent->areanum = ent->areanum2 = -1;

	//get all leafs, including solids
	num_leafs = CM_BoxLeafnums (ent->v.absmin, ent->v.absmax, leafs, MAX_TOTAL_ENT_LEAFS, &lastLeaf);

	if (!num_leafs) 
	{
		// if none of the leafs were inside the map, the entity is considered to be outside the world and can be unlinked
		if (sv_debug->value)
		{
			Com_Printf(__FUNCTION__"(%i): Outside the world at [%i %i %i]\n", ent->s.number, (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);
		}
		return;
	}

	// set areas, even from clusters that don't fit in the entity array
	for (i = 0; i < num_leafs; i++) 
	{
		area = CM_LeafArea(leafs[i]);
		if (area != -1) 
		{
			// doors may legally straggle two areas, but nothing should evern need more than that
			if (ent->areanum != -1 && ent->areanum != area) 
			{
				if (ent->areanum2 != -1 && ent->areanum2 != area && sv.state == ss_loading) 
				{
					if (sv_debug->value)
					{
						Com_Printf( __FUNCTION__"(%i): Touching 3 areas at [%i %i %i]\n", ent->s.number, (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);
					}
				}
				ent->areanum2 = area;
			}
			else 
			{
				ent->areanum = area;
			}
		}
	}

	// store as many explicit clusters as we can
	ent->numClusters = 0;
	for (i = 0; i < num_leafs; i++) 
	{
		cluster = CM_LeafCluster(leafs[i]);
		if (cluster != -1) 
		{
			ent->clusternums[ent->numClusters++] = cluster;
			if (ent->numClusters == MAX_ENT_CLUSTERS) 
			{
				break;
			}
		}
	}

	// store off a last cluster if we need to in case there are more clusternums[] than we store
	if (i != num_leafs) 
	{
		ent->lastCluster = CM_LeafCluster(lastLeaf);
	}

	// make sure old_origin is valid if linking this entity for the first time
	if (!ent->v.linkcount)
	{
		VectorCopy (ent->v.origin, ent->v.old_origin);
	}

	// let gamecode know we've been relinked
	ent->v.linkcount++;

	// if the entity isn't nonsolid add it to solids, triggers or path nodes lists
	if (ent->v.solid == SOLID_NOT)
	{
		return;
	}

	// find the first node that the ent's box crosses
	node = sv_areanodes;
	while (1)
	{
		if (node->axis == -1)
			break;
		if (ent->v.absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->v.absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;	// crosses the node
	}
	
	// link it in	
	if (ent->v.solid == SOLID_TRIGGER)
		InsertLinkBefore (&ent->area, &node->trigger_ents);
	else if (ent->v.solid == SOLID_PATHNODE)
		InsertLinkBefore(&ent->area, &node->pathnode_ents);
	else
		InsertLinkBefore(&ent->area, &node->solid_ents);

}


/*
====================
SV_AreaEntities_r

====================
*/
void SV_AreaEntities_r (areanode_t *node)
{
	link_t *l, *next, *start;
	gentity_t *check;
	int count;

	count = 0;

	// touch linked entities
	start = NULL;
	if (area_type == AREA_SOLID)
		start = &node->solid_ents;
	else if (area_type == AREA_TRIGGERS)
		start = &node->trigger_ents;
	else  if (area_type == AREA_PATHNODES)
		start = &node->pathnode_ents;

	if (start == NULL)
	{
		Com_Error(ERR_DROP, __FUNCTION__": Unknown area_type %i\n", area_type);
		return;
	}

	for (l=start->next  ; l != start ; l = next)
	{
		next = l->next;
		check = (gentity_t*)EDICT_FROM_AREA(l);

		if (check->v.solid == SOLID_NOT)
			continue;		// deactivated

		if (check->v.absmin[0] > area_maxs[0]
		|| check->v.absmin[1] > area_maxs[1]
		|| check->v.absmin[2] > area_maxs[2]
		|| check->v.absmax[0] < area_mins[0]
		|| check->v.absmax[1] < area_mins[1]
		|| check->v.absmax[2] < area_mins[2])
			continue;		// not touching

		if (area_count == area_maxcount)
		{
			Com_Printf(__FUNCTION__": Hit MAXCOUNT (%i)\n", area_maxcount);
			return;
		}

		area_list[area_count] = check;
		area_count++;
	}
	
	if (node->axis == -1)
		return;		// terminal node

	// recurse down both sides
	if ( area_maxs[node->axis] > node->dist )
		SV_AreaEntities_r ( node->children[0] );
	if ( area_mins[node->axis] < node->dist )
		SV_AreaEntities_r ( node->children[1] );
}

/*
================
SV_AreaEntities
Returns the **list of entities within mins/maxs of a given type
================
*/
int SV_AreaEntities(vec3_t mins, vec3_t maxs, gentity_t **list, int maxcount, int areatype)
{
	area_mins = mins;
	area_maxs = maxs;
	area_list = list;
	area_count = 0;
	area_maxcount = maxcount;
	area_type = areatype;

	SV_AreaEntities_r (sv_areanodes);

	return area_count;
}


//===========================================================================

/*
================
SV_ClipHandleForEntity

Returns a headnode that can be used for testing or clipping to a given entity.
If the entity is a bsp model, the headnode will be returned, otherwise a custom box tree will be constructed.

Inline models are mandatory for SOLID_BSP and optional for SOLID_TRIGGER
SOLID_BSP entities must error when they have no inline model set
SOLID_TRIGGER entities will explictly use inline model instead of bounding box when they have inline model set
================
*/
clipHandle_t SV_ClipHandleForEntity(const gentity_t* ent)
{
	int capsule = 0;

	if (SV_IsBrushModel(ent->v.modelindex))
	{
		if (ent->v.solid == SOLID_BSP || ent->v.solid == SOLID_TRIGGER)
		{
			// Use explicit inline model
			int idx = 0 - ent->v.modelindex; // Do this because model index is negated and CM doesn't like it
			return CM_InlineModel(idx);
		}
	}

	// create a temp capsule from bounding box sizes if SVF_CAPSULE
	// otherwise create a temp box from bounding box sizes
	if (ent->v.svflags & SVF_CAPSULE) 
	{
		capsule = 1;
	}

	if (sv_debug->value && ent->v.solid == SOLID_BSP)
	{
		Com_Printf(__FUNCTION__": solid_bsp entity %i has no bmodel (using BBOX)\n", NUM_FOR_ENT(ent));
	}

	return CM_TempBoxModel(ent->v.mins, ent->v.maxs, capsule);
}

/*
=============
SV_PointContents
=============
*/
int SV_PointContents(vec3_t p)
{
	gentity_t	*touch[MAX_GENTITIES], *pEnt;
	int			i, num;
	int			contents, contents_entity;
	clipHandle_t clip;
	float		*angles;

	// get base contents from world
	contents = CM_PointContents(p, 0);

	// Or in contents from all the other entities
	num = SV_AreaEntities(p, p, touch, MAX_GENTITIES, AREA_SOLID);

	for (i = 0; i < num; i++)
	{
		pEnt = touch[i];

		// might intersect, so do an exact clip
		clip = SV_ClipHandleForEntity(pEnt);
		
		// SOLID_BSP & SOLID_TRIGGER entities with bmodel rotate, others don't
		if (SV_IsBrushModel(pEnt->v.modelindex) && (pEnt->v.solid == SOLID_BSP || pEnt->v.solid == SOLID_TRIGGER))
		{
			angles = pEnt->v.angles;
		}
		else
		{
			angles = vec3_origin;
		}

		// bbox entities have their contents set in code
		if (clip == BOX_MODEL_HANDLE || clip == CAPSULE_MODEL_HANDLE)
		{
			if(pEnt->v.contents != CONTENTS_NONE) // FIXME: eliminate when qc variable .solid is turned into makesolid() func
				CM_SetTempBoxModelContents(pEnt->v.contents);
		}

		contents_entity = CM_TransformedPointContents(p, clip, pEnt->v.origin, angles);
		contents |= contents_entity;

		if (clip == BOX_MODEL_HANDLE || clip == CAPSULE_MODEL_HANDLE)
		{
			CM_SetTempBoxModelContents(CONTENTS_BODY);
		}
	}
	return contents;
}



typedef struct
{
	vec3_t		boxmins, boxmaxs; // enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	float		*start, *end;
	trace_t		trace;
	gentity_t	*ignoreEntity;
	int			contentmask;
	int			capsule;
} moveclip_t;


//===========================================================================

/*
==================
SV_SetTraceEnt

If NULL  : entityNum = -1     | entity = world
If WORLD : entityNum = 0      | entity = world
Others   : entityNum = entNum | entity = ent

Note: trace.ent can not be NULL in QCVM
==================
*/
static void SV_SetTraceEnt(trace_t* trace, gentity_t* ent)
{
	if (ent == NULL)
	{
		trace->entityNum = ENTITYNUM_NULL;
		trace->ent = sv.edicts; // FIXME: this is incorrect!
		//trace->ent = NULL; // this is correct but causes crashes in sv_movestep (which could've been fixed faster than writing this comment)
	}
	else
	{
		trace->entityNum = NUM_FOR_ENT(ent);
		trace->ent = ent;
	}
}

/*
====================
SV_ClipToEntity
====================
*/
void SV_ClipToEntity(trace_t *trace, gentity_t* clipent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentmask, int capsule) 
{
	clipHandle_t clipHandle;
	float* angles;

	CM_ClearTrace(trace);

	if (!mins)
		mins = vec3_origin;
	if (!maxs)
		maxs = vec3_origin;

	// if it doesn't have any contents of a type we're looking for, ignore it
	if (clipent->v.contents != CONTENTS_NONE && !(contentmask & clipent->v.contents))
	{
		trace->fraction = 1.0;
		return;
	}

	clipHandle = SV_ClipHandleForEntity(clipent);

	// boxes don't rotate, bmodels do
	if (SV_IsBrushModel(clipent->v.modelindex))
		angles = clipent->v.angles;
	else
		angles = vec3_origin;

	if (clipent == sv.edicts)
	{
		// world never moves
		CM_BoxTrace(trace, start, end, mins, maxs, 0, contentmask, capsule);
	}
	else
	{
		CM_TransformedBoxTrace(trace, start, end, mins, maxs, clipHandle, contentmask, clipent->v.origin, angles, capsule);
	}

	if (trace->fraction < 1.0f) 
		SV_SetTraceEnt(trace, clipent);
}

/*
====================
SV_EntityContact
Returns true if the entity overlaps with bounding box.
====================
*/
qboolean SV_EntityContact(vec3_t mins, vec3_t maxs, const gentity_t* ent, int capsule) 
{
	trace_t trace;
	clipHandle_t clipHandle;
	const float* angles;

	CM_ClearTrace(&trace);
	clipHandle = SV_ClipHandleForEntity(ent);

	if (clipHandle == BOX_MODEL_HANDLE || ent == sv.edicts)
	{
		angles = vec3_origin;
	}
	else
	{
		angles = ent->v.angles;
	}

	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs, clipHandle, MASK_ALL, ent->v.origin, angles, capsule);
	return trace.startsolid;
}

/*
====================
SV_ClipMoveToEntities
====================
*/
void SV_ClipMoveToEntities( moveclip_t *clip )
{
	int			i, num;
	gentity_t	*touchlist[MAX_GENTITIES], *touch;
	trace_t		trace;
	clipHandle_t clipHandle;
	float		*angles;
	qboolean	oldStart;

	num = SV_AreaEntities(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES, AREA_SOLID);

	// it is possible to have an entity in this list removed before we get to it (killtriggered)
	for (i = 0; i < num; i++)
	{
		if (clip->trace.allsolid)
			return; 

		touch = touchlist[i];

		if (!touch->inuse)
			continue; // the entity was likely killtriggered

		if ((int)touch->v.solid == SOLID_NOT)
			continue; // the entity isn't solid

		if (clip->ignoreEntity && clip->ignoreEntity != sv.edicts) // see if the entity should be ignored
		{
			if (touch == clip->ignoreEntity) 
				continue; // don't clip against the ignored entity

			if (PROG_TO_GENT(touch->v.owner) == clip->ignoreEntity)
				continue; // don't clip against entities that have ignoreEntity as their owner
		}

		// might intersect, so do an exact clip
		clipHandle = SV_ClipHandleForEntity(touch);

		if (touch->v.contents != CONTENTS_NONE && !(clip->contentmask & touch->v.contents))
		{
			// if the entity lacks the contents we trace against ignore it
			// inline model entities don't do this check and cannot have their contents overwritten
			continue;
		}

		angles = vec3_origin;

		if (clipHandle == BOX_MODEL_HANDLE || clipHandle == CAPSULE_MODEL_HANDLE) 
		{
			if(touch->v.contents != CONTENTS_NONE)
				CM_SetTempBoxModelContents(touch->v.contents);
		}
		else
		{
			// SOLID_BSP & SOLID_TRIGGER entities with bmodel rotate
			if (touch->v.solid == SOLID_BSP || touch->v.solid == SOLID_TRIGGER)
			{
				angles = touch->v.angles;
			}
		}

		CM_TransformedBoxTrace(&trace, clip->start, clip->end, clip->mins, clip->maxs, clipHandle, clip->contentmask, touch->v.origin, angles, clip->capsule);

		if (clipHandle == BOX_MODEL_HANDLE || clipHandle == CAPSULE_MODEL_HANDLE)
		{
			CM_SetTempBoxModelContents(CONTENTS_BODY);
		}

		if (trace.allsolid) 
		{
			clip->trace.allsolid = true;
			//clip->trace.entityNum = NUM_FOR_ENT(touch); //touch->s.number;
			SV_SetTraceEnt(&clip->trace, touch);
		}
		else if (trace.startsolid) 
		{
			clip->trace.startsolid = true;
			SV_SetTraceEnt(&clip->trace, touch);
		}

		if (trace.fraction < clip->trace.fraction)
		{
			// make sure we keep a startsolid from a previous trace
			oldStart = clip->trace.startsolid;	
			clip->trace = trace;
			clip->trace.startsolid |= oldStart;
			SV_SetTraceEnt(&clip->trace, touch);
		}
	}
}


/*
==================
SV_TraceBounds
==================
*/
void SV_TraceBounds (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
{
#if 0
// debug to test against everything
boxmins[0] = boxmins[1] = boxmins[2] = -9999;
boxmaxs[0] = boxmaxs[1] = boxmaxs[2] = 9999;
#else
	int		i;
	
	for (i = 0; i < 3; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] + mins[i] - 1;
			boxmaxs[i] = end[i] + maxs[i] + 1;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1;
			boxmaxs[i] = start[i] + maxs[i] + 1;
		}
	}
#endif
}


/*
==================
SV_Trace
Moves the given mins/maxs volume through the world from start to end.
ignoreEntity and entities owned by ignoreEntity are explicitly not checked.
==================
*/
trace_t SV_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, gentity_t *ignoreEntity, int contentmask, qboolean bCapsule)
{
	moveclip_t	clip;

	if (!mins)
	{
		mins = vec3_origin;
	}
	if (!maxs)
	{
		maxs = vec3_origin;
	}

	memset(&clip, 0, sizeof(moveclip_t));
	CM_ClearTrace(&clip.trace);
	//SV_SetTraceEnt(&clip.trace, NULL);

	// clip to world
	CM_BoxTrace(&clip.trace, start, end, mins, maxs, 0, contentmask, bCapsule);

	if (clip.trace.fraction == 0)
	{
		// blocked by the world
		SV_SetTraceEnt(&clip.trace, sv.edicts);
		return clip.trace;
	}

	clip.contentmask = contentmask;
	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.ignoreEntity = ignoreEntity;
	
	// create the bounding box of the entire move
	SV_TraceBounds(start, clip.mins, clip.maxs, end, clip.boxmins, clip.boxmaxs);

	// clip to other solid entities
	SV_ClipMoveToEntities( &clip );
	SV_SetTraceEnt(&clip.trace, clip.trace.ent);
	return clip.trace;
}

