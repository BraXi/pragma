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

gentity_t	*sv_entity;	// currently run entity

void WriteGame(char* filename, qboolean autosave) {}
void ReadGame(char* filename) {}
void WriteLevel(char* filename) {}
void ReadLevel(char* filename) {}


void Scr_ClientBeginServerFrame(gentity_t* self);
void Scr_ClientEndServerFrame(gentity_t* ent);

void SV_ScriptStartFrame();
void SV_ScriptEndFrame();

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

	if ((int)ent->v.flags & (FL_SWIM | FL_FLY))
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

	trace = SV_Trace(ent->v.origin, ent->v.mins, ent->v.maxs, point, ent, MASK_MONSTERSOLID);

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
		ent->v.groundentity_num = trace.entitynum;
		ent->v.groundentity_linkcount = trace.ent->linkcount;
		ent->v.velocity[2] = 0;
	}
}



/*
================
SV_RunWorldFrame

Advances the world by 0.1 seconds
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

		// if the ground entity moved, make sure we are still on it
		if(groundentity != NULL) 
		{
			if ((groundentity->linkcount != (int)ent->v.groundentity_linkcount))
			{
				ent->v.groundentity_num = ENTITYNUM_NULL;
				if (!((int)ent->v.flags & (FL_SWIM | FL_FLY)) && (int)ent->v.svflags & SVF_MONSTER)
				{
					M_CheckGround(ent);
				}
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
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,gentity_t,area)

typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	link_t	trigger_edicts;
	link_t	solid_edicts;
	link_t	pathnode_edicts;
} areanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

areanode_t	sv_areanodes[AREA_NODES];
int			sv_numareanodes;

float	*area_mins, *area_maxs;
gentity_t	**area_list;
int		area_count, area_maxcount;
int		area_type;

int SV_HullForEntity (gentity_t *ent);

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

	ClearLink (&anode->trigger_edicts);
	ClearLink (&anode->solid_edicts);
	ClearLink (&anode->pathnode_edicts);
	
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
	memset (sv_areanodes, 0, sizeof(sv_areanodes));
	sv_numareanodes = 0;
	SV_CreateAreaNode (0, sv.models[MODELINDEX_WORLD].bmodel->mins, sv.models[MODELINDEX_WORLD].bmodel->maxs);
}


/*
===============
SV_UnlinkEdict

===============
*/
void SV_UnlinkEdict (gentity_t *ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
}

#if PROTOCOL_FLOAT_COORDS == 1
static int SV_PackSolid32(gentity_t* ent)
{
	// Q2PRO code
	int packedsolid;

	packedsolid = MSG_PackSolid32(ent->v.mins, ent->v.maxs); //Q2PRO's MSG_PackSolid32_Ver2

	if (packedsolid == PACKED_BSP)
		packedsolid = 0;  // can happen in pathological case if z mins > maxs

#ifdef _DEBUG
	if (developer->value)
	{
		vec3_t mins, maxs;

		MSG_UnpackSolid32(packedsolid, mins, maxs);// // Q2PRO's MSG_UnpackSolid32_Ver2 for those curious

		if (!VectorCompare(ent->v.mins, mins) || !VectorCompare(ent->v.maxs, maxs))
			Com_Printf("%s: bad mins/maxs on entity %d\n", __FUNCTION__, NUM_FOR_EDICT(ent));
	}
#endif

	return packedsolid;
}
#else
static int SV_PackSolid16(gentity_t* ent)
{
	int i, j, k;
	int packedsolid;
	// assume that x/y are equal and symetric
	i = ent->v.maxs[0] / 8;
	if (i < 1)
		i = 1;
	if (i > 31)
		i = 31;

	// z is not symetric
	j = (-ent->v.mins[2]) / 8;
	if (j < 1)
		j = 1;
	if (j > 31)
		j = 31;

	// and z maxs can be negative...
	k = (ent->v.maxs[2] + 32) / 8;
	if (k < 1)
		k = 1;
	if (k > 63)
		k = 63;

	packedsolid = (k << 10) | (j << 5) | i;

	if (developer->value)
	{
		vec3_t mins, maxs;

		MSG_UnpackSolid16(packedsolid, mins, maxs);

		if (!VectorCompare(ent->v.mins, mins) || !VectorCompare(ent->v.maxs, maxs))
			Com_Printf("%s: bad mins/maxs on entity %d\n", __FUNCTION__, NUM_FOR_EDICT(ent));
	}

	return packedsolid;
}
#endif

/*
===============
SV_LinkEdict

===============
*/
#define MAX_TOTAL_ENT_LEAFS		128
void SV_LinkEdict (gentity_t *ent)
{
	areanode_t	*node;
	int			leafs[MAX_TOTAL_ENT_LEAFS];
	int			clusters[MAX_TOTAL_ENT_LEAFS];
	int			num_leafs;
	int			i, j;
	int			area;
	int			topnode;

	if (ent->area.prev)
		SV_UnlinkEdict (ent);	// unlink from old position
		
	if (ent == sv.edicts)
		return;		// don't add the world

	if (!ent->inuse)
		return;

	// set the size
	VectorSubtract (ent->v.maxs, ent->v.mins, ent->v.size);

	// encode the size into the entity_state for client prediction
	switch ((int)ent->v.solid)
	{
	case SOLID_BBOX:
		if (((int)ent->v.svflags & SVF_DEADMONSTER) || VectorCompare(ent->v.mins, ent->v.maxs))
		{
			ent->s.solid = 0;
		}
		else
		{
#if PROTOCOL_FLOAT_COORDS == 1
			ent->s.solid = SV_PackSolid32(ent);
#else
			ent->s.solid = SV_PackSolid16(ent);
#endif
		}
		break;
	case SOLID_BSP:
		ent->s.solid = PACKED_BSP;      // a SOLID_BBOX will never create this value
		break;
	default:
		ent->s.solid = 0;
		break;
	}

	// set the abs box
	if (ent->v.solid == SOLID_BSP && (ent->v.angles[0] || ent->v.angles[1] || ent->v.angles[2]) )
	{	// expand for rotation
		float		max, v;
		max = 0;
		for (i=0 ; i<3 ; i++)
		{
			v = fabs(ent->v.mins[i]);
			if (v > max)
				max = v;
			v = fabs(ent->v.maxs[i]);
			if (v > max)
				max = v;
		}
		for (i=0 ; i<3 ; i++)
		{
			ent->v.absmin[i] = ent->v.origin[i] - max;
			ent->v.absmax[i] = ent->v.origin[i] + max;
		}
	}
	else
	{	// normal
		VectorAdd (ent->v.origin, ent->v.mins, ent->v.absmin);	
		VectorAdd (ent->v.origin, ent->v.maxs, ent->v.absmax);
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	ent->v.absmin[0] -= 1;
	ent->v.absmin[1] -= 1;
	ent->v.absmin[2] -= 1;
	ent->v.absmax[0] += 1;
	ent->v.absmax[1] += 1;
	ent->v.absmax[2] += 1;

// link to PVS leafs
	ent->num_clusters = 0;
	ent->areanum = 0;
	ent->areanum2 = 0;

	//get all leafs, including solids
	num_leafs = CM_BoxLeafnums (ent->v.absmin, ent->v.absmax, leafs, MAX_TOTAL_ENT_LEAFS, &topnode);

	// Q3A: if none of the leafs were inside the map, the
	// entity is outside the world and can be considered unlinked
	if (!num_leafs) 
	{
		Com_DPrintf(DP_SV, "%s: entity %i is outside the world at %f %f %f\n", __FUNCTION__, NUM_FOR_ENT(ent), ent->v.origin[0], ent->v.origin[1], ent->v.origin[2]);
		return;
	}

	// set areas
	for (i=0 ; i<num_leafs ; i++)
	{
		clusters[i] = CM_LeafCluster (leafs[i]);
		area = CM_LeafArea (leafs[i]);
		if (area)
		{	// doors may legally straggle two areas,
			// but nothing should ever need more than that
			if (ent->areanum && ent->areanum != area)
			{
				if (ent->areanum2 && ent->areanum2 != area && sv.state == ss_loading)
					Com_DPrintf(DP_SV, "Object touching 3 areas at %f %f %f\n", ent->v.absmin[0], ent->v.absmin[1], ent->v.absmin[2]);
				ent->areanum2 = area;
			}
			else
				ent->areanum = area;
		}
	}

	if (num_leafs >= MAX_TOTAL_ENT_LEAFS)
	{	// assume we missed some leafs, and mark by headnode
		ent->num_clusters = -1;
		ent->headnode = topnode;
	}
	else
	{
		ent->num_clusters = 0;
		for (i=0 ; i<num_leafs ; i++)
		{
			if (clusters[i] == -1)
				continue;		// not a visible leaf
			for (j=0 ; j<i ; j++)
				if (clusters[j] == clusters[i])
					break;
			if (j == i)
			{
				if (ent->num_clusters == MAX_ENT_CLUSTERS)
				{	// assume we missed some leafs, and mark by headnode
					ent->num_clusters = -1;
					ent->headnode = topnode;
					break;
				}

				ent->clusternums[ent->num_clusters++] = clusters[i];
			}
		}
	}

	// if first time, make sure old_origin is valid
	if (!ent->linkcount)
	{
		VectorCopy (ent->v.origin, ent->v.old_origin);
	}
	ent->linkcount++;

	if (ent->v.solid == SOLID_NOT)
		return;

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
			break;		// crosses the node
	}
	
	// link it in	
	if (ent->v.solid == SOLID_TRIGGER)
		InsertLinkBefore (&ent->area, &node->trigger_edicts);
	else if (ent->v.solid == SOLID_PATHNODE)
		InsertLinkBefore(&ent->area, &node->pathnode_edicts);
	else
		InsertLinkBefore (&ent->area, &node->solid_edicts);

}


/*
====================
SV_AreaEdicts_r

====================
*/
void SV_AreaEdicts_r (areanode_t *node)
{
	link_t		*l, *next, *start;
	gentity_t		*check;
	int			count;

	count = 0;

	// touch linked edicts
	start = NULL;
	if (area_type == AREA_SOLID)
		start = &node->solid_edicts;
	else if (area_type == AREA_TRIGGERS)
		start = &node->trigger_edicts;
	else  if (area_type == AREA_PATHNODES)
		start = &node->pathnode_edicts;

	if (start == NULL)
	{
		Com_Error(ERR_DROP, "%s: unknown area_type %i\n", __FUNCTION__, area_type);
		return;
	}

	for (l=start->next  ; l != start ; l = next)
	{
		next = l->next;
		check = EDICT_FROM_AREA(l);

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
			Com_Printf ("SV_AreaEdicts: MAXCOUNT\n");
			return;
		}

		area_list[area_count] = check;
		area_count++;
	}
	
	if (node->axis == -1)
		return;		// terminal node

	// recurse down both sides
	if ( area_maxs[node->axis] > node->dist )
		SV_AreaEdicts_r ( node->children[0] );
	if ( area_mins[node->axis] < node->dist )
		SV_AreaEdicts_r ( node->children[1] );
}

/*
================
SV_AreaEdicts
================
*/
int SV_AreaEdicts (vec3_t mins, vec3_t maxs, gentity_t **list, int maxcount, int areatype)
{
	area_mins = mins;
	area_maxs = maxs;
	area_list = list;
	area_count = 0;
	area_maxcount = maxcount;
	area_type = areatype;

	SV_AreaEdicts_r (sv_areanodes);

	return area_count;
}


//===========================================================================

/*
=============
SV_PointContents
=============
*/
int SV_PointContents (vec3_t p)
{
	gentity_t		*touch[MAX_GENTITIES], *hit;
	int			i, num;
	int			contents, c2;
	int			headnode;
	float		*angles;

	// get base contents from world
	contents = CM_PointContents (p, sv.models[MODELINDEX_WORLD].bmodel->headnode);

	// or in contents from all the other entities
	num = SV_AreaEdicts (p, p, touch, MAX_GENTITIES, AREA_SOLID);

	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];

		// might intersect, so do an exact clip
		headnode = SV_HullForEntity (hit);
		angles = hit->v.angles;
		if (hit->v.solid != SOLID_BSP)
			angles = vec3_origin;	// boxes don't rotate

		c2 = CM_TransformedPointContents (p, headnode, hit->v.origin, hit->v.angles);

		contents |= c2;
	}

	return contents;
}



typedef struct
{
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	gentity_t		*passedict;
	int			contentmask;
} moveclip_t;



/*
================
SV_HullForEntity

Returns a headnode that can be used for testing or clipping an
object of mins/maxs size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
int SV_HullForEntity(gentity_t* ent)
{
	cmodel_t* model;

	// decide which clipping hull to use, based on the size
	if (ent->v.solid == SOLID_BSP)
	{
		// explicit hulls in the BSP model
		model = sv.models[(int)ent->v.modelindex].bmodel;

		if (!model)
		{
			Scr_RunError("MOVETYPE_PUSH with a non BSP model for entity %s (%i) at [%i %i %i]\n", Scr_GetString(ent->v.classname),
				NUM_FOR_EDICT(ent), (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);
			return -1; // msvc
		}

		return model->headnode;
	}

	// create a temp hull from bounding box sizes
	return CM_HeadnodeForBox (ent->v.mins, ent->v.maxs);
}


//===========================================================================

/*
====================
SV_ClipMoveToEntities

====================
*/
void SV_ClipMoveToEntities ( moveclip_t *clip )
{
	int			i, num;
	gentity_t	*touchlist[MAX_GENTITIES], *touch;
	trace_t		trace;
	int			headnode;
	float		*angles;

	num = SV_AreaEdicts(clip->boxmins, clip->boxmaxs, touchlist, MAX_GENTITIES, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		touch = touchlist[i];
		if (touch->v.solid == SOLID_NOT)
			continue;

		if (touch == clip->passedict)
			continue;

		if (clip->trace.allsolid)
			return;

		if (clip->passedict)
		{
		 	if (PROG_TO_GENT(touch->v.owner) == clip->passedict)
				continue;	// don't clip against own missiles
			if (PROG_TO_GENT(clip->passedict->v.owner) == touch)
				continue;	// don't clip against owner
		}

		if ( !(clip->contentmask & CONTENTS_DEADMONSTER)
		&& ((int)touch->v.svflags & SVF_DEADMONSTER) )
				continue;

		// might intersect, so do an exact clip
		headnode = SV_HullForEntity (touch);
		angles = touch->v.angles;
		if (touch->v.solid != SOLID_BSP)
			angles = vec3_origin;	// boxes don't rotate

		if ((int)touch->v.svflags & SVF_MONSTER)
			trace = CM_TransformedBoxTrace (clip->start, clip->end,
				clip->mins2, clip->maxs2, headnode, clip->contentmask,
				touch->v.origin, angles);
		else
			trace = CM_TransformedBoxTrace (clip->start, clip->end,
				clip->mins, clip->maxs, headnode,  clip->contentmask,
				touch->v.origin, angles);

		if (trace.allsolid || trace.startsolid ||
		trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
		 	if (clip->trace.startsolid)
			{
				clip->trace = trace;
				clip->trace.startsolid = true;
			}
			else
				clip->trace = trace;
		}
		else if (trace.startsolid)
			clip->trace.startsolid = true;
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
	
	for (i=0 ; i<3 ; i++)
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

Passedict and edicts owned by passedict are explicitly not checked.

==================
*/
trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, gentity_t *passedict, int contentmask)
{
	moveclip_t	clip;

	if (!mins)
		mins = vec3_origin;
	if (!maxs)
		maxs = vec3_origin;

	memset ( &clip, 0, sizeof ( moveclip_t ) );

	// clip to world
	clip.trace = CM_BoxTrace (start, end, mins, maxs, 0, contentmask);
	clip.trace.ent = sv.edicts;
	if (clip.trace.fraction == 0)
		return clip.trace;		// blocked by the world

	clip.contentmask = contentmask;
	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passedict = passedict;

	VectorCopy (mins, clip.mins2);
	VectorCopy (maxs, clip.maxs2);
	
	// create the bounding box of the entire move
	SV_TraceBounds ( start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs );

	// clip to other solid entities
	SV_ClipMoveToEntities ( &clip );

	if (clip.trace.ent == NULL)
		clip.trace.entitynum = ENTITYNUM_NULL;
	else
		clip.trace.entitynum = NUM_FOR_ENT(clip.trace.ent);

	return clip.trace;
}

