/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "server.h"

cvar_t* sv_maxvelocity;
cvar_t* sv_gravity;

//FIXME: hacked in for E3 demo for 25 years
#define	sv_stopspeed		100
#define sv_friction			6
#define sv_waterfriction	1

#define	STEPSIZE	18

/*
=============
SV_CheckBottom
Returns false if any part of the bottom of the entity is off an edge that is not a staircase.
=============
*/
static int c_yes, c_no;
qboolean SV_CheckBottom(gentity_t* ent)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	VectorAdd(ent->v.origin, ent->v.mins, mins);
	VectorAdd(ent->v.origin, ent->v.maxs, maxs);

	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks the corners must be within 16 of the midpoint
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
	return true; // we got out easy

realcheck:
	c_no++;
	//
	// check it for real...
	//
	start[2] = mins[2];

	// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
	start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
	stop[2] = start[2] - 2 * STEPSIZE;
	trace = SV_Trace(start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

	// the corners must be within 16 of the midpoint	
	for (x = 0; x <= 1; x++)
		for (y = 0; y <= 1; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];

			trace = SV_Trace(start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);

			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
				return false;
		}

	c_yes++;
	return true;
}


void SV_CheckGround(gentity_t* ent)
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

	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(trace.endpos, ent->v.origin);
		ent->v.groundentity_num = trace.entitynum;
		ent->v.groundentity_linkcount = trace.ent->linkcount;
		ent->v.velocity[2] = 0;
	}
}


/*
============
SV_TestEntityPosition
============
*/
gentity_t* SV_TestEntityPosition(gentity_t* ent)
{
	trace_t	trace;
	int		mask;

	if (ent->v.clipmask)
		mask = (int)ent->v.clipmask;
	else
		mask = MASK_SOLID;
	trace = SV_Trace(ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, ent, mask);

	if (trace.startsolid)
		return sv.edicts;

	return NULL;
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity(gentity_t* ent)
{
	float vel;
	int i;

	vel = VectorLength(ent->v.velocity);
	if (vel > sv_maxvelocity->value)
	{
		for (i = 0; i < 3; i++)
			ent->v.velocity[i] = (ent->v.velocity[i] / vel) * sv_maxvelocity->value;
	}
}


/*
==================
ClipVelocity
Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

int ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i, blocked;

	blocked = 0;
	if (normal[2] > 0)
		blocked |= 1;		// floor
	if (!normal[2])
		blocked |= 2;		// step

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}


/*
============
SV_FlyMove
The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
============
*/
#define	MAX_CLIP_PLANES	5
int SV_FlyMove(gentity_t* ent, float time, int mask)
{
	gentity_t* hit;
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;

	numbumps = 4;

	blocked = 0;
	VectorCopy(ent->v.velocity, original_velocity);
	VectorCopy(ent->v.velocity, primal_velocity);
	numplanes = 0;

	time_left = time;

	ent->v.groundentity_num = ENTITYNUM_NULL;
	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for (i = 0; i < 3; i++)
			end[i] = ent->v.origin[i] + time_left * ent->v.velocity[i];

		trace = SV_Trace(ent->v.origin, ent->v.mins, ent->v.maxs, end, ent, mask);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy(vec3_origin, ent->v.velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy(trace.endpos, ent->v.origin);
			VectorCopy(ent->v.velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

		hit = trace.ent;

		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
			if (hit->v.solid == SOLID_BSP)
			{
				ent->v.groundentity_num = NUM_FOR_ENT(hit);
				ent->v.groundentity_linkcount = hit->linkcount;
			}
		}
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step
		}

		//
		// run the impact function
		//
		Scr_Event_Impact(ent, &trace);
		if (!ent->inuse)
			break;		// removed by the impact function


		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy(vec3_origin, ent->v.velocity);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for (i = 0; i < numplanes; i++)
		{
			ClipVelocity(original_velocity, planes[i], new_velocity, 1);

			for (j = 0; j < numplanes; j++)
				if ((j != i) && !VectorCompare(planes[i], planes[j]))
				{
					if (DotProduct(new_velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{	// go along this plane
			VectorCopy(new_velocity, ent->v.velocity);
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				gi.dprintf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy(vec3_origin, ent->v.velocity);
				return 7;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->v.velocity);
			VectorScale(dir, d, ent->v.velocity);
		}

		//
		// if original velocity is against the original velocity, stop dead to avoid tiny occilations in sloping corners
		//
		if (DotProduct(ent->v.velocity, primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, ent->v.velocity);
			return blocked;
		}
	}

	return blocked;
}


/*
============
SV_AddGravity
============
*/
void SV_AddGravity(gentity_t* ent)
{
	ent->v.velocity[2] -= ent->v.gravity * sv_gravity->value * (float)SV_FRAMETIME;
}

/*
===============================================================================
PUSHMOVE
===============================================================================
*/

/*
============
SV_PushEntity
Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity(gentity_t* ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	start;
	vec3_t	end;
	int		mask;

	VectorCopy(ent->v.origin, start);
	VectorAdd(start, push, end);

retry:
	if (ent->v.clipmask)
		mask = (int)ent->v.clipmask;
	else
		mask = MASK_SOLID;

	trace = SV_Trace(start, ent->v.mins, ent->v.maxs, end, ent, mask);

	VectorCopy(trace.endpos, ent->v.origin);
	SV_LinkEdict(ent);

	if (trace.fraction != 1.0)
	{
		Scr_Event_Impact(ent, &trace);

		// if the pushed entity went away and the pusher is still there
		if (!trace.ent->inuse && ent->inuse)
		{
			// move the pusher back and try again
			VectorCopy(start, ent->v.origin);
			SV_LinkEdict(ent);
			goto retry;
		}
	}

	if (ent->inuse)
		SV_TouchEntities(ent, AREA_TRIGGERS);

	return trace;
}


typedef struct // FIME: find a better place for this?
{
	gentity_t* ent;
	vec3_t	origin;
	vec3_t	angles;
	float	deltayaw;
} pushed_t;

static pushed_t pushed[MAX_GENTITIES], *pushed_p;
static gentity_t* obstacle;

/*
============
SV_Push
Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
============
*/
qboolean SV_Push(gentity_t* pusher, vec3_t move, vec3_t amove)
{
	int			i, e;
	gentity_t* check, * block;
	vec3_t		mins, maxs;
	pushed_t* p;
	vec3_t		org, org2, move2, forward, right, up;

#if PROTOCOL_FLOAT_COORDS == 0
	// clamp the move to 1/8 units, so the position will be accurate for client side prediction
	for (i = 0; i < 3; i++)
	{
		float	temp;
		temp = move[i] * 8.0;
		if (temp > 0.0)
			temp += 0.5;
		else
			temp -= 0.5;
		move[i] = 0.125 * (int)temp;
	}
#endif

	// find the bounding box
	for (i = 0; i < 3; i++)
	{
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	// we need this for pushing things later
	VectorSubtract(vec3_origin, amove, org);
	AngleVectors(org, forward, right, up);

	// save the pusher's original position
	pushed_p->ent = pusher;
	VectorCopy(pusher->v.origin, pushed_p->origin);
	VectorCopy(pusher->v.angles, pushed_p->angles);
	if (pusher->client)
	{
		pushed_p->deltayaw = pusher->client->ps.pmove.delta_angles[YAW];
	}
	pushed_p++;

	// move the pusher to it's final position
	VectorAdd(pusher->v.origin, move, pusher->v.origin);
	VectorAdd(pusher->v.angles, amove, pusher->v.angles);
	SV_LinkEdict(pusher);

	// see if any solid entities are inside the final position
	for (e = 1; e < sv.max_edicts; e++) //sv.num_edicts
	{
		check = EDICT_NUM(e);

		if (!check->inuse)
			continue;

		if (check->v.movetype == MOVETYPE_PUSH || check->v.movetype == MOVETYPE_STOP || check->v.movetype == MOVETYPE_NONE || check->v.movetype == MOVETYPE_NOCLIP)
			continue;

		if (!check->area.prev)
			continue;		// not linked in anywhere

		// if the entity is standing on the pusher, it will definitely be moved
		if ((int)check->v.groundentity_num != NUM_FOR_ENT(pusher))
		{
			// see if the ent needs to be tested
			if (check->v.absmin[0] >= maxs[0]
				|| check->v.absmin[1] >= maxs[1]
				|| check->v.absmin[2] >= maxs[2]
				|| check->v.absmax[0] <= mins[0]
				|| check->v.absmax[1] <= mins[1]
				|| check->v.absmax[2] <= mins[2])
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		if ((pusher && pusher->v.movetype == MOVETYPE_PUSH) || ((int)check->v.groundentity_num == NUM_FOR_ENT(pusher)))
		{
			// move this entity
			pushed_p->ent = check;
			VectorCopy(check->v.origin, pushed_p->origin);
			VectorCopy(check->v.angles, pushed_p->angles);
			pushed_p++;

//	Com_Printf("pushed %s\n", Scr_GetString(check->v.classname));
			// try moving the contacted entity 
			VectorAdd(check->v.origin, move, check->v.origin);
			if (check->client)
			{	
				// FIXME PRAGMA: does not rotate player at all
				check->client->ps.pmove.delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
				check->v.pm_delta_angles[YAW] += ANGLE2SHORT(amove[YAW]);
			}
			else
			{
				// rotate monsters and anything thats standing on it
				check->v.angles[YAW] += amove[YAW]; // rotate objects standing on a pusher
			}

			// figure movement due to the pusher's amove
			VectorSubtract(check->v.origin, pusher->v.origin, org);
			org2[0] = DotProduct(org, forward);
			org2[1] = -DotProduct(org, right);
			org2[2] = DotProduct(org, up);
			VectorSubtract(org2, org, move2);
			VectorAdd(check->v.origin, move2, check->v.origin);

			// may have pushed them off an edge
			if (check->v.groundentity_num != NUM_FOR_ENT(pusher))
				check->v.groundentity_num = ENTITYNUM_NULL;

			block = SV_TestEntityPosition(check);
			if (!block)
			{	// pushed ok
				SV_LinkEdict(check);
				// impact?
				continue;
			}

			// if it is ok to leave in the old position, do it this is only relevent for riding entities, not pushed
			// FIXME: this doesn't acount for rotation
			VectorSubtract(check->v.origin, move, check->v.origin);
			block = SV_TestEntityPosition(check);
			if (!block)
			{
				pushed_p--;
				continue;
			}
		}

		// save off the obstacle so we can call the block function
		obstacle = check;

		// move back any entities we already moved go backwards, so if the same 
		// entity was pushed twice, it goes back to the original position
		for (p = pushed_p - 1; p >= pushed; p--)
		{
			VectorCopy(p->origin, p->ent->v.origin);
			VectorCopy(p->angles, p->ent->v.angles);
			if (p->ent->client)
			{
				p->ent->client->ps.pmove.delta_angles[YAW] = p->deltayaw;
				p->ent->v.pm_delta_angles[YAW] = p->deltayaw;
			}
			SV_LinkEdict(p->ent);
		}
		return false;
	}

	//FIXME: is there a better way to handle this?
	// see if anything we moved has touched a trigger
	for (p = pushed_p - 1; p >= pushed; p--)
		SV_TouchEntities(p->ent, AREA_TRIGGERS);

	return true;
}

/*
================
SV_Physics_Pusher
Bmodel objects don't interact with each other, but push all box objects
================
*/
void SV_Physics_Pusher(gentity_t* ent)
{
	vec3_t		move, amove;
	gentity_t* part, * mv;

	// if not a team captain, so movement will be handled elsewhere
	if ((int)ent->v.flags & FL_TEAMSLAVE)
		return;

	// make sure all team slaves can move before commiting any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out

	pushed_p = pushed;
	for (part = ent; part; part = part->teamchain)
	{
		if (part->v.velocity[0] || part->v.velocity[1] || part->v.velocity[2] ||
			part->v.avelocity[0] || part->v.avelocity[1] || part->v.avelocity[2]
			)
		{	// object is moving
			VectorScale(part->v.velocity, SV_FRAMETIME, move);
			VectorScale(part->v.avelocity, SV_FRAMETIME, amove);

			if (!SV_Push(part, move, amove))
				break;	// move was blocked
		}
	}
	if (pushed_p > &pushed[MAX_GENTITIES])
		Com_Error(ERR_DROP, "pushed_p > &pushed[MAX_EDICTS], memory corrupted");

	if (part)
	{
		// the move failed, bump all nextthink times and back out moves
		for (mv = ent; mv; mv = mv->teamchain)
		{
			if (mv->v.nextthink > 0)
				mv->v.nextthink += SV_FRAMETIME;
		}

		// if the pusher has a "blocked" function, call it, otherwise just stay in place until the obstacle is gone
		Scr_Event_Blocked(part, obstacle);
	}
	else
	{
		// the move succeeded, so call all think functions
		for (part = ent; part; part = part->teamchain)
		{
			SV_RunThink(part);
			if (!part->inuse)
			{
				Com_Error(ERR_DROP, "SV_Physics_Pusher: entity %i deleted itself\n", NUM_FOR_EDICT(part));
				return; 
			}

		}
	}
}

//==================================================================

/*
=============
SV_Physics_None
Non moving objects can only think
=============
*/
void SV_Physics_None(gentity_t* ent)
{
	SV_RunThink(ent);

	VectorMA(ent->v.angles, SV_FRAMETIME, ent->v.avelocity, ent->v.angles);
}

/*
=============
SV_Physics_Noclip
A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip(gentity_t* ent)
{
	// regular thinking
	if (!SV_RunThink(ent))
		return;

	if (!ent->inuse)
		return; // could have deleted itself in think

	VectorMA(ent->v.angles, SV_FRAMETIME, ent->v.avelocity, ent->v.angles);
	VectorMA(ent->v.origin, SV_FRAMETIME, ent->v.velocity, ent->v.origin);

	SV_LinkEdict(ent);
}

/*
==============================================================================
TOSS / BOUNCE
==============================================================================
*/

/*
=============
SV_Physics_Toss
Toss, bounce, and fly movement. When onground, do nothing.
=============
*/
void SV_Physics_Toss(gentity_t* ent)
{
	trace_t		trace;
	vec3_t		move;
	float		backoff;
	gentity_t* slave;
	qboolean	wasinwater;
	qboolean	isinwater;
	vec3_t		old_origin;

	// regular thinking
	SV_RunThink(ent);
	if (!ent->inuse)
		return; // could have deleted itself in think

	// if not a team captain, so movement will be handled elsewhere
	if ((int)ent->v.flags & FL_TEAMSLAVE)
		return;

	if (ent->v.velocity[2] > 0)
		ent->v.groundentity_num = ENTITYNUM_NULL;

	gentity_t* groundent = NULL;

	if (ent->v.groundentity_num != ENTITYNUM_NULL)
		groundent = ENT_FOR_NUM((int)ent->v.groundentity_num);

	// check for the groundentity going away
	if (groundent)
		if (!groundent->inuse)
			ent->v.groundentity_num = ENTITYNUM_NULL;

	// if onground, return without moving
	if (groundent)
		return;

	VectorCopy(ent->v.origin, old_origin);

	SV_CheckVelocity(ent);

	// add gravity
	if (ent->v.movetype != MOVETYPE_FLY && ent->v.movetype != MOVETYPE_FLYMISSILE)
		SV_AddGravity(ent);

	// move angles
	VectorMA(ent->v.angles, SV_FRAMETIME, ent->v.avelocity, ent->v.angles);

	// move origin
	VectorScale(ent->v.velocity, SV_FRAMETIME, move);
	trace = SV_PushEntity(ent, move);
	if (!ent->inuse)
		return;

	if (trace.fraction < 1)
	{
		if (ent->v.movetype == MOVETYPE_BOUNCE)
			backoff = 1.5;
		else
			backoff = 1;

		ClipVelocity(ent->v.velocity, trace.plane.normal, ent->v.velocity, backoff);

		// stop if on ground
		if (trace.plane.normal[2] > 0.7)
		{
			if (ent->v.velocity[2] < 60 || ent->v.movetype != MOVETYPE_BOUNCE)
			{
				ent->v.groundentity_num = trace.entitynum;
				ent->v.groundentity_linkcount = trace.ent->linkcount;
				VectorCopy(vec3_origin, ent->v.velocity);
				VectorCopy(vec3_origin, ent->v.avelocity);
			}
		}

//		if (ent->touch)
//			ent->touch (ent, trace.ent, &trace.plane, trace.surface);
	}

	// check for water transition
	wasinwater = ((int)ent->v.watertype & MASK_WATER);
	ent->v.watertype = SV_PointContents(ent->v.origin);
	isinwater = (int)ent->v.watertype & MASK_WATER;

	if (isinwater)
		ent->v.waterlevel = 1;
	else
		ent->v.waterlevel = 0;

	if (!wasinwater && isinwater)
		SV_StartSound(old_origin, sv.edicts, CHAN_AUTO, SV_SoundIndex("misc/h2ohit1.wav"), 1, 1, 0);
	else if (wasinwater && !isinwater)
		SV_StartSound(ent->v.origin, sv.edicts, CHAN_AUTO, SV_SoundIndex("misc/h2ohit1.wav"), 1, 1, 0);

	// move teamslaves
	for (slave = ent->teamchain; slave; slave = slave->teamchain)
	{
		VectorCopy(ent->v.origin, slave->v.origin);
		SV_LinkEdict(slave);
	}
}

/*
===============================================================================
STEPPING MOVEMENT
===============================================================================
*/

/*
=============
SV_Physics_Step
Monsters freefall when they don't have a ground entity, otherwise all movement is done with discrete steps.
This is also used for objects that have become still on the ground, but will fall if the floor is pulled out from under them.
FIXME: is this true?
=============
*/
void SV_AddRotationalFriction(gentity_t* ent)
{
	int		n;
	float	adjustment;

	VectorMA(ent->v.angles, SV_FRAMETIME, ent->v.avelocity, ent->v.angles);
	adjustment = SV_FRAMETIME * sv_stopspeed * sv_friction;
	for (n = 0; n < 3; n++)
	{
		if (ent->v.avelocity[n] > 0)
		{
			ent->v.avelocity[n] -= adjustment;
			if (ent->v.avelocity[n] < 0)
				ent->v.avelocity[n] = 0;
		}
		else
		{
			ent->v.avelocity[n] += adjustment;
			if (ent->v.avelocity[n] > 0)
				ent->v.avelocity[n] = 0;
		}
	}
}

void SV_Physics_Step(gentity_t* ent)
{
	qboolean	wasonground;
	qboolean	hitsound = false;
	float* vel;
	float		speed, newspeed, control;
	float		friction;
	gentity_t* groundentity = NULL;
	int			mask;

	// airborn monsters should always check for ground
	if ((int)ent->v.groundentity_num == ENTITYNUM_NULL)
		SV_CheckGround(ent);

	groundentity = ((int)ent->v.groundentity_num == ENTITYNUM_NULL) ? NULL : ENT_FOR_NUM((int)ent->v.groundentity_num);

	SV_CheckVelocity(ent);

	if (groundentity)
		wasonground = true;
	else
		wasonground = false;

	if (ent->v.avelocity[0] || ent->v.avelocity[1] || ent->v.avelocity[2])
		SV_AddRotationalFriction(ent);

	// add gravity except:
	//   flying monsters
	//   swimming monsters who are in the water
	if (!wasonground)
	{
		if (!((int)ent->v.flags & FL_FLY) && !(((int)ent->v.flags & FL_SWIM) && (ent->v.waterlevel > 2)))
		{
			if (ent->v.velocity[2] < sv_gravity->value * -0.1)
				hitsound = true;
			if (ent->v.waterlevel == 0)
				SV_AddGravity(ent);
		}
	}

	// friction for flying monsters that have been given vertical velocity
	if (((int)ent->v.flags & FL_FLY) && (ent->v.velocity[2] != 0))
	{
		speed = fabs(ent->v.velocity[2]);
		control = speed < sv_stopspeed ? sv_stopspeed : speed;
		friction = sv_friction / 3;
		newspeed = speed - (SV_FRAMETIME * control * friction);
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		ent->v.velocity[2] *= newspeed;
	}

	// friction for flying monsters that have been given vertical velocity
	if (((int)ent->v.flags & FL_SWIM) && (ent->v.velocity[2] != 0))
	{
		speed = fabs(ent->v.velocity[2]);
		control = speed < sv_stopspeed ? sv_stopspeed : speed;
		newspeed = speed - (SV_FRAMETIME * control * sv_waterfriction * ent->v.waterlevel);
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		ent->v.velocity[2] *= newspeed;
	}

	if (ent->v.velocity[2] || ent->v.velocity[1] || ent->v.velocity[0])
	{
		// apply friction
		// let dead monsters who aren't completely onground slide
		if ((wasonground) || ((int)ent->v.flags & (FL_SWIM | FL_FLY)))
			if (!(ent->v.health <= 0.0 && !SV_CheckBottom(ent)))
			{
				vel = ent->v.velocity;
				speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
				if (speed)
				{
					friction = sv_friction;

					control = speed < sv_stopspeed ? sv_stopspeed : speed;
					newspeed = speed - SV_FRAMETIME * control * friction;

					if (newspeed < 0)
						newspeed = 0;
					newspeed /= speed;

					vel[0] *= newspeed;
					vel[1] *= newspeed;
				}
			}

		if ((int)ent->v.svflags & SVF_MONSTER)
			mask = MASK_MONSTERSOLID;
		else
			mask = MASK_SOLID;
		SV_FlyMove(ent, SV_FRAMETIME, mask);

		SV_LinkEdict(ent);
		SV_TouchEntities(ent, AREA_TRIGGERS);
		if (!ent->inuse)
			return;

		if ((int)ent->v.groundentity_num != ENTITYNUM_NULL && !wasonground && hitsound)
			SV_StartSound(NULL, ent, 0, SV_SoundIndex("world/land.wav"), 1, 1, 0);
	}

	// regular thinking
	SV_RunThink(ent);
}

void SV_RunEntityPhysics(gentity_t* ent)
{
	switch ((int)ent->v.movetype)
	{
	case MOVETYPE_PUSH:
	case MOVETYPE_STOP:
		SV_Physics_Pusher(ent);
		break;
	case MOVETYPE_NONE:
		SV_Physics_None(ent);
		break;
	case MOVETYPE_NOCLIP:
		SV_Physics_Noclip(ent);
		break;
	case MOVETYPE_STEP:
		SV_Physics_Step(ent);
		break;
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_FLY:
	case MOVETYPE_FLYMISSILE:
		SV_Physics_Toss(ent);
		break;
	default:
		Com_Error(ERR_DROP, "SV_Physics: entity %i has bad movetype %i\n", NUM_FOR_EDICT(ent), (int)ent->v.movetype);
	}
}