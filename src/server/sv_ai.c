/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "server.h"

int Nav_AddPathNode(float x, float y, float z);
qboolean Nav_AddPathNodeLink(int nodeId, int linkTo);
int Nav_GetNodeLinkCount(int node);
int Nav_GetMaxLinksCount();

static vec3_t node_origin; // for qsorting distance
typedef struct // for qsorting pathnodes from closest to farthest
{
	int	index;
	vec3_t origin;
} tempnode_t;

float dist3d(vec3_t p1, vec3_t p2)
{
	vec3_t vtemp;
	VectorSubtract(p1, p2, vtemp);
	return VectorLength(vtemp);
}

static char* vtos(vec3_t p)
{
	return va("[%i %i %i]", (int)p[0], (int)p[1], (int)p[2]);
}

int CompareNodeDistances(const void* a, const void* b)
{
	tempnode_t* nodeA = (tempnode_t*)a;
	tempnode_t* nodeB = (tempnode_t*)b;

	float distanceA = dist3d(nodeA->origin, node_origin);
	float distanceB = dist3d(nodeB->origin, node_origin);

	if (distanceA < distanceB)
		return -1;
	else if (distanceA > distanceB)
		return 1;
	else return 0; // distances are equal so the order doesn't matter
}

/*
================
SV_LinkPathNode

Link pathnode to neighbors
================
*/
static void SV_LinkPathNode(gentity_t* self)
{
	int			i, num;
	gentity_t* other;
	vec3_t		mins, maxs;
	trace_t		trace;
	tempnode_t nodes[32];

	static const int nodeLinkDist = 148;

	num = 0;
	VectorCopy(self->v.origin, node_origin);

	if ((int)self->v.nodeIndex == -1 || self->v.solid != SOLID_PATHNODE)
	{
		Com_Printf("WARNING: %s at %s is not a path node.\n", Scr_GetString(self->v.classname), vtos(self->v.origin));
		return;
	}


	if (Nav_GetNodeLinkCount(self->v.nodeIndex) >= Nav_GetMaxLinksCount())
	{
		return; // already linked
	}

	//	static vec3_t expand = { nodeLinkDist, nodeLinkDist, nodeLinkDist };
	//	VectorSubtract(self->v.absmin, expand, mins);
	//	VectorAdd(self->v.absmax, expand, maxs);
	//	num = SV_AreaEntities(mins, maxs, nodes, MAX_GENTITIES, AREA_PATHNODES);

		//
		// find nodes within reasonable distance to self
		//
	for (i = svs.max_clients; i < sv.max_edicts; i++)
	{
		other = ENT_FOR_NUM(i);
		if (!other->inuse)
			continue;
		if (other == self)
			continue;
		if (other->v.solid != SOLID_PATHNODE)
			continue;
		if (dist3d(self->v.origin, other->v.origin) > nodeLinkDist)
			continue;

		VectorCopy(other->v.origin, nodes[num].origin);
		nodes[num].index = other->v.nodeIndex;
		num++;

		if (num == 32)
		{
			Com_Printf("WARNING: Path node %i at %s is crowded (32 neighbors or more).\n", (int)self->v.nodeIndex, vtos(self->v.origin));
			break;
		}
	}

	if (!num)
	{
		Com_Printf("WARNING: Path node at %s is too far from other nodes (node %i will not be linked).\n", vtos(self->v.origin), (int)self->v.nodeIndex);
		return;
	}


	//
	// sort the nodes from closest to farthest
	//
	qsort(nodes, sizeof(nodes) / sizeof(nodes[0]), sizeof(tempnode_t), CompareNodeDistances);


	//
	// do a trace check to see if we can link with nodes
	//
	VectorSet(mins, -4, -4, -4);
	VectorSet(maxs, 4, 4, 4);

	vec3_t start, end;
	VectorCopy(self->v.origin, start);
	start[2] += 16;

	for (i = 0; i < num; i++)
	{
		VectorCopy(nodes[i].origin, end);
		end[2] += 16;
		trace = SV_Trace(start, mins, maxs, end, self, MASK_MONSTERSOLID);

		if (trace.fraction != 1.0)
			continue;

		if (!Nav_AddPathNodeLink(self->v.nodeIndex, nodes[i].index))
		{
			Com_Printf("WARNING: Path node %i at %s reached link count limit (nodes are too crowded)\n", (int)self->v.nodeIndex, vtos(self->v.origin));
			break;
		}
	}
}

/*
================
SV_DropPathNodeToFloor

Drop pathnode to floor, removes nodes that are stuck in solid.
================
*/
static void SV_DropPathNodeToFloor(gentity_t* self)
{
	trace_t trace;
	vec3_t dest;

	VectorCopy(self->v.origin, dest);
	dest[2] -= 128;

	trace = SV_Trace(self->v.origin, self->v.mins, self->v.maxs, dest, self, MASK_MONSTERSOLID);

	if (trace.startsolid)
	{
		Com_Printf("WARNING: Path node %i at %s in solid, removed.\n", (int)self->v.nodeIndex, vtos(self->v.origin));
		SV_FreeEntity(self);
		return;
	}

	if (trace.fraction == 1.0)
	{
		Com_Printf("WARNING: Path node %i at %s too far from floor, removed.\n", (int)self->v.nodeIndex, vtos(self->v.origin));
		SV_FreeEntity(self);
		return;
	}

	VectorCopy(trace.endpos, self->v.origin);
	SV_LinkEdict(self);

	self->v.nodeIndex = Nav_AddPathNode(self->v.origin[0], self->v.origin[1], self->v.origin[2]);
}

/*
================
SV_LinkAllPathNodes

Link all pathnodes
================
*/
void SV_LinkAllPathNodes()
{
	int i;
	gentity_t* ent;

	// first off, drop all nodes to ground
	for (i = svs.max_clients; i < sv.max_edicts; i++)
	{
		ent = ENT_FOR_NUM(i);
		if (!ent->inuse || ent->v.solid != SOLID_PATHNODE)
			continue;
		SV_DropPathNodeToFloor(ent);
	}

	// and then link them
	for (i = svs.max_clients; i < sv.max_edicts; i++)
	{
		ent = ENT_FOR_NUM(i);
		if (!ent->inuse || ent->v.solid != SOLID_PATHNODE)
			continue;
		SV_LinkPathNode(ent);
	}
}

/*
================
SV_GetNearestNode

Like Nav_GetNearestNode, but does a trace so the node doesn't ent up behind a wall
================
*/
static void SV_GetNearestPathNode(vec3_t point)
{
	int			i, num, cnt;
	gentity_t* node;
	vec3_t		mins, maxs;
	trace_t		trace;
	tempnode_t nodes[32];

	VectorSet(mins, 196, 196, 196);
	VectorAdd(mins, point, mins);
	VectorSet(maxs, 196, 196, 196);
	VectorAdd(maxs, point, maxs);

	gentity_t* areanodes[MAX_GENTITIES];
	num = SV_AreaEntities(mins, maxs, areanodes, MAX_GENTITIES, AREA_PATHNODES);

	if (!num)
	{
		Com_Printf("SV_GetNearestPathNode: no nearby nodes\n");
		return;
	}

	cnt = 0;
	for (i = 0; i < num; i++)
	{
		node = areanodes[i];
		if (!node->inuse)
			continue;

		VectorCopy(node->v.origin, nodes[cnt].origin);
		nodes[cnt].index = node->v.nodeIndex;

		if (cnt == 32)
			break;
	}

	// sort the nodes from closest to farthest
	qsort(nodes, sizeof(nodes) / sizeof(nodes[0]), sizeof(tempnode_t), CompareNodeDistances);

	vec3_t start, end;
	VectorCopy(point, start);
	start[2] += 16;

	for (i = 0; i < num; i++)
	{
		VectorCopy(nodes[i].origin, end);
		end[2] += 16;
		trace = SV_Trace(start, vec3_origin, vec3_origin, end, NULL, MASK_MONSTERSOLID);

		if (trace.fraction != 1.0)
			continue;
	}
}


/*

	goal = VM_TO_ENT(ent->v.goal_entity);
	if (goal == sv.edicts)
		goal = NULL;

qboolean SV_CheckBottom(gentity_t* actor)
void SV_MoveToGoal(gentity_t* actor, gentity_t* goal, float dist)
SV_WalkMove(gentity_t* actor, float yaw, float dist)

	goal = VM_TO_ENT(actor->v.goal_entity);
	if (goal == sv.edicts)
	{
		Com_sprintf("SV_MoveToGoal: aborted because goal is world for entity %i\n", NUM_FOR_ENT(actor));
	}

*/

#define	STEPSIZE 18

#define	DI_NODIR	-1
/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.
=============
*/

#if 0
static int c_yes, c_no;
qboolean SV_CheckBottom(gentity_t* actor)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	int stepsize = 18;
	int contentmask = MASK_MONSTERSOLID;

	VectorAdd(actor->v.origin, actor->v.mins, mins);
	VectorAdd(actor->v.origin, actor->v.maxs, maxs);

	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks
	// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for (x = 0; x <= 1; x++)
		for (y = 0; y <= 1; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (SV_PointContents(start) != CONTENTS_SOLID)
				goto realcheck;
		}

	c_yes++;
	return true;		// we got out easy

realcheck:
	c_no++;
	//
	// check it for real...
	//
	start[2] = mins[2];

	// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
	start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
	stop[2] = start[2] - 2 * stepsize;
	trace = SV_Trace(start, vec3_origin, vec3_origin, stop, actor, contentmask);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

	// the corners must be within 16 of the midpoint	
	for (x = 0; x <= 1; x++)
		for (y = 0; y <= 1; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = SV_Trace(start, vec3_origin, vec3_origin, stop, actor, contentmask);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > stepsize)
				return false;
		}

	c_yes++;
	return true;
}
#else
extern qboolean SV_CheckBottom(gentity_t* actor); //sv_physics.c
#endif


/*
=============
SV_MoveStep

The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned
*/
qboolean SV_MoveStep(gentity_t* actor, vec3_t move, qboolean relink)
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	int			i;
	float		stepsize;
	vec3_t		test;
	int			contents;

	int contentmask = MASK_MONSTERSOLID;
	gentity_t* goal = VM_TO_ENT(actor->v.goal_entity);

	// try the move	
	VectorCopy(actor->v.origin, oldorg);
	VectorAdd(actor->v.origin, move, neworg);

	// flying monsters don't step up
	if (actor->v.flags & (FL_SWIM | FL_FLY))
	{
		// try one move with vertical motion, then one without
		for (i = 0; i < 2; i++)
		{
			VectorAdd(actor->v.origin, move, neworg);
			if (i == 0 && goal)
			{
//				if (!goal)
//				{
//					ent->v.goal_entity = ent->v.enemy;
//					goal = enemy;
//				}

				dz = actor->v.origin[2] - goal->v.origin[2];
				if (goal->client)
				{
					if (dz > 40)
						neworg[2] -= 8;
					if (!((actor->v.flags & FL_SWIM) && (actor->v.waterlevel < 2)))
						if (dz < 30)
							neworg[2] += 8;
				}
				else
				{
					if (dz > 8)
						neworg[2] -= 8;
					else if (dz > 0)
						neworg[2] -= dz;
					else if (dz < -8)
						neworg[2] += 8;
					else
						neworg[2] += dz;
				}
			}
			trace = SV_Trace(actor->v.origin, actor->v.mins, actor->v.maxs, neworg, actor, contentmask);

			// fly monsters don't enter water voluntarily
			if (actor->v.flags & FL_FLY)
			{
				if (!actor->v.waterlevel)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + actor->v.mins[2] + 1;

					contents = SV_PointContents(test);
					if (contents & MASK_WATER)
					{
						return false;
					}
				}
			}

			// swim monsters don't exit water voluntarily
			if (actor->v.flags & FL_SWIM)
			{
				if (actor->v.waterlevel < 2)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + actor->v.mins[2] + 1;
					contents = SV_PointContents(test);
					if (!(contents & MASK_WATER))
						return false;
				}
			}

			if (trace.fraction == 1)
			{
				VectorCopy(trace.endpos, actor->v.origin);
				if (relink)
				{
					SV_LinkEdict(actor);
//					SV_TouchEntities(ent, AREA_TRIGGERS);
				}
				return true;
			}

			if (!goal)
				break;
		}

		return false;
	}

	// push down from a step height above the wished position
	if (!(actor->v.flags & FL_NOSTEP))
		stepsize = STEPSIZE;
	else
		stepsize = 1;

	neworg[2] += stepsize;
	VectorCopy(neworg, end);
	end[2] -= stepsize * 2;

	trace = SV_Trace(neworg, actor->v.mins, actor->v.maxs, end, actor, contentmask);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = SV_Trace(neworg, actor->v.mins, actor->v.maxs, end, actor, contentmask);
		if (trace.allsolid || trace.startsolid)
			return false;
	}


	// don't go in to water
	if (actor->v.waterlevel == 0)
	{
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + actor->v.mins[2] + 1;
		contents = SV_PointContents(test);

		if (contents & MASK_WATER)
			return false;
	}

	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if (actor->v.flags & FL_PARTIALGROUND)
		{
			VectorAdd(actor->v.origin, move, actor->v.origin);
			if (relink)
			{
				SV_LinkEdict(actor);
				SV_TouchEntities(actor, AREA_TRIGGERS);
				SV_TouchEntities(actor, AREA_PATHNODES);
			}
			actor->v.groundentity_num = ENTITYNUM_NULL; //remove groundent
			return true;
		}

		return false;		// walked off an edge
	}

	// check point traces down for dangling corners
	VectorCopy(trace.endpos, actor->v.origin);

	if (!SV_CheckBottom(actor))
	{
		if (actor->v.flags & FL_PARTIALGROUND)
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				SV_LinkEdict(actor);
				SV_TouchEntities(actor, AREA_TRIGGERS);
				SV_TouchEntities(actor, AREA_PATHNODES);
			}
			return true;
		}
		VectorCopy(oldorg, actor->v.origin);
		return false;
	}

	if (actor->v.flags & FL_PARTIALGROUND)
	{
		//int temp = (int)actor->v.flags; // TODO: see if linux build still need this since v.flags is int now
		//temp &= ~FL_PARTIALGROUND;
		//actor->v.flags = temp;

		actor->v.flags &= ~FL_PARTIALGROUND;
	}

	actor->v.groundentity_num = trace.entitynum;
	actor->v.groundentity_linkcount = trace.ent->v.linkcount;

	// the move is ok
	if (relink)
	{
		SV_LinkEdict(actor);
		SV_TouchEntities(actor, AREA_TRIGGERS);
		SV_TouchEntities(actor, AREA_PATHNODES);
	}
	return true;
}

/*
===============
SV_ChangeYaw

note: move to qc?
===============
*/
void SV_ChangeYaw(gentity_t* actor)
{
	float	ideal;
	float	current;
	float	move;
	float	speed;

	current = anglemod(actor->v.angles[YAW]);
	ideal = actor->v.ideal_yaw;

	if (current == ideal)
		return;

	move = ideal - current;
	speed = actor->v.yaw_speed;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	actor->v.angles[YAW] = anglemod(current + move);
}


/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if facing it.
======================
*/

qboolean SV_StepDirection(gentity_t* actor, float yaw, float dist)
{
	vec3_t		move, oldorigin;
	float		delta;

	actor->v.ideal_yaw = yaw;
	SV_ChangeYaw(actor);

	yaw = yaw * M_PI * 2 / 360;
	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

	VectorCopy(actor->v.origin, oldorigin);
	if (SV_MoveStep(actor, move, false))
	{
		delta = actor->v.angles[YAW] - actor->v.ideal_yaw;
		if (delta > 45 && delta < 315)
		{		
			// not turned far enough, so don't take the step
			VectorCopy(oldorigin, actor->v.origin);
		}
		SV_LinkEdict(actor);
		SV_TouchEntities(actor, AREA_TRIGGERS);
		SV_TouchEntities(actor, AREA_PATHNODES);
		return true;
	}
	SV_LinkEdict(actor);
	SV_TouchEntities(actor, AREA_TRIGGERS);
	SV_TouchEntities(actor, AREA_PATHNODES);
	return false;
}


/*
======================
SV_FixCheckBottom
======================
*/
static void SV_FixCheckBottom(gentity_t* actor)
{
	//int temp = (int)actor->v.flags; // se if linux still needs this
	//temp |= FL_PARTIALGROUND;
	//actor->v.flags = temp;
	actor->v.flags |= FL_PARTIALGROUND;
}


/*
================
SV_NewChaseDir
================
*/
void SV_NewChaseDir(gentity_t* actor, gentity_t* goal, float dist)
{
	float	deltax, deltay;
	float	d[3];
	float	tdir, olddir, turnaround;

	//FIXME: how did we get here with no goal
	if (!goal)
		return;

	olddir = anglemod((int)(actor->v.ideal_yaw / 45) * 45);
	turnaround = anglemod(olddir - 180);

	deltax = goal->v.origin[0] - actor->v.origin[0];
	deltay = goal->v.origin[1] - actor->v.origin[1];
	if (deltax > 10)
		d[1] = 0;
	else if (deltax < -10)
		d[1] = 180;
	else
		d[1] = DI_NODIR;
	if (deltay < -10)
		d[2] = 270;
	else if (deltay > 10)
		d[2] = 90;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;

		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			return;
	}

	// try other directions
	if (((rand() & 3) & 1) || abs(deltay) > abs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] != DI_NODIR && d[1] != turnaround
		&& SV_StepDirection(actor, d[1], dist))
		return;

	if (d[2] != DI_NODIR && d[2] != turnaround
		&& SV_StepDirection(actor, d[2], dist))
		return;

	/* there is no direct path to the player, so pick another direction */

	if (olddir != DI_NODIR && SV_StepDirection(actor, olddir, dist))
		return;

	if (rand() & 1) 	/*randomly determine direction of search*/
	{
		for (tdir = 0; tdir <= 315; tdir += 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
				return;
	}
	else
	{
		for (tdir = 315; tdir >= 0; tdir -= 45)
			if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
				return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist))
		return;

	actor->v.ideal_yaw = olddir;	// can't move

	// if a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all
	if (!SV_CheckBottom(actor))
		SV_FixCheckBottom(actor);
}




/*
======================
SV_CloseEnough
======================
*/
qboolean SV_CloseEnough(gentity_t* ent, gentity_t* goal, float dist)
{
	int		i;

	for (i = 0; i < 3; i++)
	{
		if (goal->v.absmin[i] > ent->v.absmax[i] + dist)
			return false;
		if (goal->v.absmax[i] < ent->v.absmin[i] - dist)
			return false;
	}
	return true;
}



/*
======================
SV_MoveToGoal
======================
*/
qboolean SV_MoveToGoal(gentity_t* actor, gentity_t* goal, float dist)
{
	if (!goal)
		return false;

	if (actor->v.groundentity_num == ENTITYNUM_NULL && !(actor->v.flags & (FL_FLY | FL_SWIM)))
		return false;

	// if the next step hits the enemy, return immediately
	if (goal && SV_CloseEnough(actor, goal, dist))
		return true;

	// bump around...
	if ((rand() & 3) == 1 || !SV_StepDirection(actor, actor->v.ideal_yaw, dist))
	{
		if (actor->inuse)
			SV_NewChaseDir(actor, goal, dist);
	}
	return false;
}


/*
===============
SV_WalkMove
===============
*/
qboolean SV_WalkMove(gentity_t* actor, float yaw, float dist)
{
	vec3_t	move;

	if (actor->v.groundentity_num == ENTITYNUM_NULL && !(actor->v.flags & (FL_FLY | FL_SWIM)))
		return false;

	yaw = yaw * M_PI * 2 / 360;

	move[0] = cos(yaw) * dist;
	move[1] = sin(yaw) * dist;
	move[2] = 0;

	int result = SV_MoveStep(actor, move, true);
	return result;
}
