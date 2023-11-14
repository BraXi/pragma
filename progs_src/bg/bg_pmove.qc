/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//FIXME if ((pm.groundentity && pml.groundsurface && !(pml.groundsurface->flags & SURF_SLICK) ) || (pml.ladder) )
	
#define DotProduct(x,y)			(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define VectorCopy(a,b)			(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define VectorClear(a)			(a[0]=a[1]=a[2]=0)
#define VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

#define NULL 0

entity null_ent;
void(vector veca, float scale, vector vecb, __out vector vecc) VectorMA =
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
};

void(vector in, float scale, __out vector out) VectorScale =
{
	out_x = out_x * scale;
	out_y = out_y * scale;	
	out_z = out_z * scale;
};

float(vector v) VectorNormalize =
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);		// FIXME

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}

float PMF_DUCKED			= 1;
float PMF_JUMP_HELD			= 2;
float PMF_ON_GROUND			= 4;
float PMF_TIME_WATERJUMP	= 8;	// pm_time is waterjump
float PMF_TIME_LAND			= 16;	// pm_time is time before rejump
float PMF_TIME_TELEPORT		= 32;	// pm_time is non-moving time
float PMF_NO_PREDICTION		= 64;	// temporarily disables prediction (used for grappling hook)

// movement parameters
float	pm_stopspeed = 100;
float	pm_maxspeed = 300;
float	pm_duckspeed = 100;
float	pm_accelerate = 10;
float	pm_airaccelerate = 0;
float	pm_wateraccelerate = 10;
float	pm_friction = 6;
float	pm_waterfriction = 1;
float	pm_waterspeed = 400;

#define	STEPSIZE		18
#define	MAXTOUCH		32
#define	STOP_EPSILON	0.1
#define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5

typedef struct
{
	int	msec;
	int	buttons;
	int	angles[3];
	int	forwardmove, sidemove, upmove;
	int	impulse;
	int	lightlevel;
} usercmd_t;


// pmove_state_t is the information necessary for client side movement prediction
typedef enum 
{
	PM_NORMAL,
	PM_SPECTATOR,
	PM_DEAD,
	PM_GIB,
	PM_FREEZE
} pmtype_t;


// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
	pmtype_t	pm_type;
	
	vector		origin;
	vector		velocity;
	float		gravity;
	
	vector		mins;
	vector		maxs;

	float		pm_flags;		// ducked, jump_held, etc
	float		pm_time;		// each unit = 8 ms
	
	vector		delta_angles;	// add to command angles to get view direction
								// changed by spawns, rotating objects, and teleporters
} pmove_state_t;

typedef struct
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t	cmd;
	float		snapinitial;	// if s has been changed outside pmove

	// results (out)
	int			numtouch;
	entity		touchents[MAXTOUCH];

	vector		viewangles;			// clamped
	float		viewheight;

	vector		mins, maxs;			// bounding box size

	entity		groundentity;
	int			watertype;
	int			waterlevel;
} pmove_t;



// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
typedef struct
{
	vector		origin;			// full float precision
	vector		velocity;		// full float precision

	vector		forward, right, up;
	float		frametime;

//	csurface_t	*groundsurface;
//	cplane_t	groundplane;

	string		groundsurface_name;
	float		groundsurface_flags;
	float		groundsurface_value;
	
	int			groundcontents;

	vector		previous_origin;
	float	ladder;
} pml_t;

pmove_t		pm;
pml_t		pml;

#ifdef SVGAME
entity pm_passent;
#endif

void(vector start, vector mins, vector maxs, vector end) pm_trace =
{
#ifdef CGAME
	trace(start, mins, maxs, end, localplayernum+1, MASK_PLAYERSOLID);
#endif

#ifdef SVGAME
	if (pm_passent.health > 0)
		return trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
#endif
};

/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/


void(vector in, vector normal, vector out, float overbounce) PM_ClipVelocity =
{
	float	backoff;
	float	change;
	int		i;
	
	backoff = dotproduct(in, normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
};




/*
==================
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/

void() PM_StepSlideMove_ =
{
	int			bumpcount, numbumps;
	vector		dir;
	float		d;
	int			numplanes;
	vector		planes[MAX_CLIP_PLANES];
	vector		primal_velocity;
	int			i, j;
	vector		end;
	float		time_left;
	
	numbumps = 4;
	
	VectorCopy (pml.velocity, primal_velocity);
	numplanes = 0;
	
	time_left = pml.frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for(i = 0; i < 3; i++)
			end[i] = pml.origin[i] + time_left * pml.velocity[i];

		pm_trace(pml.origin, pm.mins, pm.maxs, end);

		if (trace_allsolid)
		{	
			// entity is trapped in another solid
			pml.velocity[2] = 0;	// don't build up falling damage
			return;
		}

		if (trace_fraction > 0)
		{	
			// actually covered some distance
			VectorCopy (trace_endpos, pml.origin);
			numplanes = 0;
		}

		if(trace_fraction == 1)
			 break;		// moved the entire distance

		// save entity for contact
		if(pm.numtouch < MAXTOUCH && trace_ent)
		{
			pm.touchents[pm.numtouch] = trace_ent;
			pm.numtouch++;
		}
		
		time_left -= time_left * trace_fraction;

		// slide along this plane
		if(numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			VectorCopy (vec3_origin, pml.velocity);
			break;
		}

		VectorCopy(trace_plane_normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for(i = 0; i < numplanes; i++)
		{
			PM_ClipVelocity(pml.velocity, planes[i], pml.velocity, 1.01);
			for( j = 0; j < numplanes; j++)
			{
				if(j != i)
				{
					if(dotproduct(pml.velocity, planes[j]) < 0)
						break;	// not ok
				}
				
				if (j == numplanes)
					break;
			}
		}
		
		if (i != numplanes)
		{	
			// go along this plane
		}
		else
		{	
			// go along the crease
			if (numplanes != 2)
			{
				dprint("clip velocity, numplanes ==", ftos(numplanes), "\n");
				VectorCopy (vec3_origin, pml.velocity);
				break;
			}
			dir = crossproduct(planes[0], planes[1]);
			d = dotproduct(dir, pml.velocity);
			VectorScale (dir, d, pml.velocity);
		}
		//
		// if velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct (pml.velocity, primal_velocity) <= 0)
		{
			VectorCopy (vec3_origin, pml.velocity);
			break;
		}
	}

	if (pm.s.pm_time)
	{
		VectorCopy (primal_velocity, pml.velocity);
	}
};



/*
==================
PM_StepSlideMove

==================
*/
void() PM_StepSlideMove =
{
	vector		start_o, start_v;
	vector		down_o, down_v;
	float		down_dist, up_dist;
//	vector		delta;
	vector		up, down;

	VectorCopy (pml.origin, start_o);
	VectorCopy (pml.velocity, start_v);

	PM_StepSlideMove_ ();

	VectorCopy (pml.origin, down_o);
	VectorCopy (pml.velocity, down_v);

	VectorCopy (start_o, up);
	up[2] += STEPSIZE;

	pm_trace(up, pm.mins, pm.maxs, up);
	if (trace_allsolid)
		return;		// can't step up

	// try sliding above
	VectorCopy (up, pml.origin);
	VectorCopy (start_v, pml.velocity);

	PM_StepSlideMove_ ();

	// push down the final amount
	VectorCopy (pml.origin, down);
	down[2] -= STEPSIZE;
	
	pm_trace(pml.origin, pm.mins, pm.maxs, down);
	if (!trace_allsolid)
	{
		VectorCopy (trace_endpos, pml.origin);
	}

	VectorCopy(pml.origin, up);

	// decide which one went farther
    down_dist = (down_o[0] - start_o[0])*(down_o[0] - start_o[0])
        + (down_o[1] - start_o[1])*(down_o[1] - start_o[1]);
    up_dist = (up[0] - start_o[0])*(up[0] - start_o[0])
        + (up[1] - start_o[1])*(up[1] - start_o[1]);

	if (down_dist > up_dist || trace_plane_normal_z < MIN_STEP_NORMAL)
	{
		VectorCopy (down_o, pml.origin);
		VectorCopy (down_v, pml.velocity);
		return;
	}
	
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pml.velocity[2] = down_v[2];
};


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
void() PM_Friction =
{
	vector	vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	
	vel = pml.velocity;
	
	speed = sqrt(vel[0]*vel[0] +vel[1]*vel[1] + vel[2]*vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	drop = 0;

// apply ground friction
//FIXME if ((pm.groundentity && pml.groundsurface && !(pml.groundsurface->flags & SURF_SLICK) ) || (pml.ladder) )
	{
		friction = pm_friction;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;
	}

// apply water friction
	if (pm.waterlevel && !pml.ladder)
		drop += speed * pm_waterfriction * pm.waterlevel * pml.frametime;

// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
};


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
void(vector wishdir, float wishspeed, float accel) PM_Accelerate =
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = dotproduct (pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	
	if (addspeed <= 0)
		return;
	
	accelspeed = accel*pml.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		pml.velocity[i] += accelspeed*wishdir[i];	
};

void(vector wishdir, float wishspeed, float accel) PM_AirAccelerate =
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;
		
	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct (pml.velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * pml.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		pml.velocity[i] += accelspeed*wishdir[i];	
};


/*
=============
PM_AddCurrents
=============
*/
void(vector	wishvel) PM_AddCurrents =
{
	vector	v;
	float	s;

	//
	// account for ladders
	//

	if (pml.ladder && fabs(pml.velocity[2]) <= 200)
	{
		if ((pm.viewangles[PITCH] <= -15) && (pm.cmd.forwardmove > 0))
			wishvel[2] = 200;
		else if ((pm.viewangles[PITCH] >= 15) && (pm.cmd.forwardmove > 0))
			wishvel[2] = -200;
		else if (pm.cmd.upmove > 0)
			wishvel[2] = 200;
		else if (pm.cmd.upmove < 0)
			wishvel[2] = -200;
		else
			wishvel[2] = 0;

		// limit horizontal speed when on a ladder
		if (wishvel[0] < -25)
			wishvel[0] = -25;
		else if (wishvel[0] > 25)
			wishvel[0] = 25;

		if (wishvel[1] < -25)
			wishvel[1] = -25;
		else if (wishvel[1] > 25)
			wishvel[1] = 25;
	}


	//
	// add water currents
	//

	if (pm.watertype & MASK_CURRENT)
	{
		VectorClear (v);

		if (pm.watertype & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pm.watertype & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pm.watertype & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pm.watertype & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pm.watertype & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pm.watertype & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		s = pm_waterspeed;
		if ((pm.waterlevel == 1) && (pm.groundentity))
			s /= 2;

		VectorMA (wishvel, s, v, wishvel);
	}

	//
	// add conveyor belt velocities
	//

	if (pm.groundentity) // FIXME
	{
		VectorClear (v);

		if (pml.groundcontents & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pml.groundcontents & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pml.groundcontents & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		VectorMA (wishvel, 100 /* pm.groundentity->speed */, v, wishvel);
	}
};


/*
===================
PM_WaterMove

===================
*/
void() PM_WaterMove =
{
	int		i;
	vector	wishvel;
	float	wishspeed;
	vector	wishdir;

//
// user intentions
//
	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*pm.cmd.forwardmove + pml.right[i]*pm.cmd.sidemove;

	if (!pm.cmd.forwardmove && !pm.cmd.sidemove && !pm.cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += pm.cmd.upmove;

	PM_AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}
	wishspeed *= 0.5;

	PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);

	PM_StepSlideMove ();
};


/*
===================
PM_AirMove

===================
*/
void() PM_AirMove =
{
	int			i;
	vector		wishvel;
	float		fmove, smove;
	vector		wishdir;
	float		wishspeed;
	float		maxspeed;

	fmove = pm.cmd.forwardmove;
	smove = pm.cmd.sidemove;
	
//!!!!! pitch should be 1/3 so this isn't needed??!
#if 0
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);
#endif

	for (i = 0; i < 2; i++) // pitch & yaw, NOT height!
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] = 0;

	PM_AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

//
// clamp to server defined max speed
//
	maxspeed = (pm.s.pm_flags & PMF_DUCKED) ? pm_duckspeed : pm_maxspeed;

	if (wishspeed > maxspeed)
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}
	
	if ( pml.ladder )
	{
		PM_Accelerate (wishdir, wishspeed, pm_accelerate);
		if (!wishvel[2])
		{
			if (pml.velocity[2] > 0)
			{
				pml.velocity[2] -= pm.s.gravity * pml.frametime;
				if (pml.velocity[2] < 0)
					pml.velocity[2]  = 0;
			}
			else
			{
				pml.velocity[2] += pm.s.gravity * pml.frametime;
				if (pml.velocity[2] > 0)
					pml.velocity[2]  = 0;
			}
		}
		PM_StepSlideMove ();
	}
	else if ( pm.groundentity )
	{	
		// walking on ground
		pml.velocity[2] = 0; //!!! this is before the accel
		PM_Accelerate (wishdir, wishspeed, pm_accelerate);

		//  fix for negative trigger_gravity fields
		//pml.velocity[2] = 0;
		
		if(pm.s.gravity > 0)
			pml.velocity[2] = 0;
		else
			pml.velocity[2] -= pm.s.gravity * pml.frametime;


		if (!pml.velocity[0] && !pml.velocity[1])
			return;
		PM_StepSlideMove ();
	}
	else
	{	// not on ground, so little effect on velocity
		if (pm_airaccelerate)
			PM_AirAccelerate(wishdir, wishspeed, pm_accelerate);
		else
			PM_Accelerate(wishdir, wishspeed, 1);
		// add gravity
		pml.velocity[2] -= pm.s.gravity * pml.frametime;
		PM_StepSlideMove ();
	}
};


/*
=============
PM_CatagorizePosition

=============
*/

void() PM_CatagorizePosition =
{
	vector		point;
	int			cont;
	int			sample1;
	int			sample2;

	// if the player hull point one unit down is solid, the player is on ground

	// see if standing on something solid	
	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 0.25;
	
	if (pml.velocity[2] > 180) //!!ZOID changed from 100 to 180 (ramp accel)
	{
		pm.s.pm_flags &= ~PMF_ON_GROUND;
		pm.groundentity = null_ent;
	}
	else
	{
		pm_trace(pml.origin, pm.mins, pm.maxs, point);

		pml.groundsurface_name = trace_surface_name;
		pml.groundsurface_flags = trace_surface_flags;
		pml.groundsurface_value = trace_surface_value;
		
		pml.groundcontents = trace_contents;

		if (!trace_ent || (trace_plane_normal[2] < 0.7 && !trace_startsolid) )
		{
			pm.groundentity = null_ent;
			pm.s.pm_flags &= ~PMF_ON_GROUND;
		}
		else
		{
			pm.groundentity = trace_ent;

			// hitting solid ground will end a waterjump
			if (pm.s.pm_flags & PMF_TIME_WATERJUMP)
			{
				pm.s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
				pm.s.pm_time = 0;
			}

			if (! (pm.s.pm_flags & PMF_ON_GROUND) )
			{	// just hit the ground
				pm.s.pm_flags |= PMF_ON_GROUND;
				// don't do landing time if we were just going down a slope
				if (pml.velocity[2] < -200)
				{
					pm.s.pm_flags |= PMF_TIME_LAND;
					// don't allow another jump for a little while
					if (pml.velocity[2] < -400)
						pm.s.pm_time = 25;	
					else
						pm.s.pm_time = 18;
				}
			}
		}
		
#if 0
		if (trace.fraction < 1.0 && trace.ent && pml.velocity[2] < 0)
			pml.velocity[2] = 0;
#endif

		if (pm.numtouch < MAXTOUCH && trace_ent)
		{
			pm.touchents[pm.numtouch] = trace_ent;
			pm.numtouch++;
		}
	}

//
// get waterlevel, accounting for ducking
//
	pm.waterlevel = 0;
	pm.watertype = 0;

	sample2 = pm.viewheight - pm.mins[2];
	sample1 = sample2 / 2;

	point[2] = pml.origin[2] + pm.mins[2] + 1;	
	cont = pointcontents (point);

	if (cont & MASK_WATER)
	{
		pm.watertype = cont;
		pm.waterlevel = 1;
		point[2] = pml.origin[2] + pm.mins[2] + sample1;
		
		cont = pointcontents (point);
		if (cont & MASK_WATER)
		{
			pm.waterlevel = 2;
			point[2] = pml.origin[2] + pm.mins[2] + sample2;
			
			cont = pointcontents (point);
			if (cont & MASK_WATER)
				pm.waterlevel = 3;
		}
	}

};


/*
=============
PM_CheckJump
=============
*/
void() PM_CheckJump =
{
	if (pm.s.pm_flags & PMF_TIME_LAND)
	{	// hasn't been long enough since landing to jump again
		return;
	}

	if (pm.cmd.upmove < 10)
	{	// not holding jump
		pm.s.pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (pm.s.pm_flags & PMF_JUMP_HELD)
		return;

	if (pm.s.pm_type == PM_DEAD)
		return;

	if (pm.waterlevel >= 2)
	{	
		// swimming, not jumping
		pm.groundentity = NULL;

		if (pml.velocity[2] <= -300)
			return;

		if (pm.watertype == CONTENTS_WATER)
			pml.velocity[2] = 100;
		else if (pm.watertype == CONTENTS_SLIME)
			pml.velocity[2] = 80;
		else
			pml.velocity[2] = 50;
		return;
	}

	if (pm.groundentity == NULL)
		return;		// in air, so no effect

	pm.s.pm_flags |= PMF_JUMP_HELD;

	pm.groundentity = NULL;
	pml.velocity[2] += 270;
	if (pml.velocity[2] < 270)
		pml.velocity[2] = 270;
};


/*
=============
PM_CheckSpecialMovement
=============
*/
void() PM_CheckSpecialMovement =
{
	vector	spot;
	int		cont;
	vector	flatforward;

	if (pm.s.pm_time)
		return;

	pml.ladder = false;

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);
	
	VectorMA (pml.origin, 1, flatforward, spot);
	
	pm_trace (pml.origin, pm.mins, pm.maxs, spot);
	
	if ((trace_fraction < 1) && (trace_contents & CONTENTS_LADDER))
		pml.ladder = true; //we're on ladder

	// check for water jump
	if (pm.waterlevel != 2)
		return;

	VectorMA (pml.origin, 30, flatforward, spot);
	spot[2] += 4;
	cont = pointcontents (spot);
	if (!(cont & CONTENTS_SOLID))
		return;

	spot[2] += 16;
	cont = pointcontents (spot);
	if (cont)
		return;
	// jump out of water
	VectorScale (flatforward, 50, pml.velocity);
	pml.velocity[2] = 350;

	pm.s.pm_flags |= PMF_TIME_WATERJUMP;
	pm.s.pm_time = 255;
};


/*
===============
PM_FlyMove
===============
*/
void(float doclip) PM_FlyMove =
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vector		wishvel;
	float		fmove, smove;
	vector		wishdir;
	float		wishspeed;
	vector		end;

	pm.viewheight = 22; // FIXME, why hardcoded here?!

	// friction

	speed = vectorlength(pml.velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pml.velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pml.velocity, newspeed, pml.velocity);
	}

	// accelerate
	fmove = pm.cmd.forwardmove;
	smove = pm.cmd.sidemove;
	
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm.cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}


	currentspeed = dotproduct(pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	
	accelspeed = pm_accelerate*pml.frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		pml.velocity[i] += accelspeed*wishdir[i];	

	if (doclip) {
		for (i=0 ; i<3 ; i++)
			end[i] = pml.origin[i] + pml.frametime * pml.velocity[i];

		pm_trace(pml.origin, pm.mins, pm.maxs, end);

		VectorCopy (trace_endpos, pml.origin);
	} 
	else 
	{
		// move
		VectorMA (pml.origin, pml.frametime, pml.velocity, pml.origin);
	}
};


/*
==============
PM_CheckDuck

Sets mins, maxs, and pm.viewheight
==============
*/
void() PM_CheckDuck =
{
#if 0
	if (pm.s.pm_type == PM_NORMAL_NO_CROUCH)
	{
		pm.s.pm_flags &= ~PMF_DUCKED;
		return;
	}
#endif 

	pm.mins[0] = -16;
	pm.mins[1] = -16;

	pm.maxs[0] = 16;
	pm.maxs[1] = 16;

	if (pm.s.pm_type == PM_GIB)
	{
		pm.mins[2] = 0;
		pm.maxs[2] = 16;
		pm.viewheight = 8;
		return;
	}

	pm.mins[2] = -24;

	if (pm.s.pm_type == PM_DEAD)
	{
		pm.s.pm_flags |= PMF_DUCKED;
	}
	else if (pm.cmd.upmove < 0 && (pm.s.pm_flags & PMF_ON_GROUND) )
	{	
		// duck
		pm.s.pm_flags |= PMF_DUCKED;
	}
	else
	{	
		// stand up if possible
		if (pm.s.pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm.maxs[2] = 32;
			pm_trace (pml.origin, pm.mins, pm.maxs, pml.origin);
			
			if (!trace_allsolid)
				pm.s.pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm.s.pm_flags & PMF_DUCKED)
	{
		pm.maxs[2] = 4;
		pm.viewheight = -2;
	}
	else
	{
		pm.maxs[2] = 32;
		pm.viewheight = 22;
	}
};


/*
==============
PM_DeadMove
==============
*/
void() PM_DeadMove =
{
	float	forward;

	if (!pm.groundentity)
		return;

	// extra friction
	forward = vectorlength (pml.velocity);
	forward -= 20;
	
	if (forward <= 0)
	{
		VectorClear (pml.velocity);
	}
	else
	{
		VectorNormalize (pml.velocity);
		VectorScale (pml.velocity, forward, pml.velocity);
	}
};


float() PM_GoodPosition =
{
	if (pm.s.pm_type == PM_SPECTATOR)
		return true;

	pm_trace (pm.s.origin, pm.mins, pm.maxs, pm.s.origin);
	
	return !trace_allsolid;
}


/*
================
PM_SnapPosition

On exit, the origin will have a value that is pre-quantized to the 0.125
precision of the network channel and in a valid position.
================
*/
void() PM_SnapPosition =
{
	VectorCopy(pml.velocity, pm.s.velocity);
	VectorCopy(pml.origin, pm.s.origin);

	if (PM_GoodPosition())
		return;

	// go back to the last position if position is wrong
	VectorCopy(pml.previous_origin, pm.s.origin);

	// kex
	//	if (G_FixStuckObject_Generic(pm.s.origin, pm.mins, pm.maxs, PM_Trace_Auto) == stuck_result_t::NO_GOOD_POSITION) {
	//		pm.s.origin = pml.previous_origin;
	//		return;
	//	}
	// kex
};



/*
================
PM_InitialSnapPosition
================
*/
void() PM_InitialSnapPosition =
{
	int        x, y, z;
	int      base[3];
	static int offset[3] = { 0, -1, 1 };

	VectorCopy (pm.s.origin, base);

	for ( z = 0; z < 3; z++ ) 
	{
		pm.s.origin[2] = base[2] + offset[ z ];
		for ( y = 0; y < 3; y++ ) 
		{
			pm.s.origin[1] = base[1] + offset[ y ];
			for ( x = 0; x < 3; x++ ) 
			{
				pm.s.origin[0] = base[0] + offset[ x ];
				if (PM_GoodPosition ()) 
				{
					pml.origin[0] = pm.s.origin[0];
					pml.origin[1] = pm.s.origin[1];
					pml.origin[2] = pm.s.origin[2];

					VectorCopy (pm.s.origin, pml.previous_origin);
					return;
				}
			}
		}
	}

	dprint( "Bad InitialSnapPosition\n");
};



/*
================
PM_ClampAngles
================
*/
#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

void() PM_ClampAngles =
{
	int	temp;
	int		i;

	if (pm.s.pm_flags & PMF_TIME_TELEPORT)
	{
		pm.viewangles[YAW] = SHORT2ANGLE(pm.cmd.angles[YAW] + pm.s.delta_angles[YAW]);
		pm.viewangles[PITCH] = 0;
		pm.viewangles[ROLL] = 0;
	}
	else
	{
		// circularly clamp the angles with deltas
		for (i=0 ; i<3 ; i++)
		{
			temp = pm.cmd.angles[i] + pm.s.delta_angles[i];
			pm.viewangles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if (pm.viewangles[PITCH] > 89 && pm.viewangles[PITCH] < 180)
			pm.viewangles[PITCH] = 89;
		else if (pm.viewangles[PITCH] < 271 && pm.viewangles[PITCH] >= 180)
			pm.viewangles[PITCH] = 271;
	}
	
	anglevectors(pm.viewangles);
	
	pml.forward = v_forward;
	pml.up = v_up;
	pml.right = v_right;
};


/*
================
Pmove

Can be called by either the server or the client
================
*/
void(pmove_t pmove) Pmove =
{
	pm = pmove;

	// clear results
	pm.numtouch = 0;
	VectorClear (pm.viewangles);
	pm.viewheight = 0;
	pm.groundentity = 0;
	pm.watertype = 0;
	pm.waterlevel = 0;

	// clear all pmove local vars
//	memset (&pml, 0, sizeof(pml));

	// copy to locals
	VectorCopy(pm.s.origin, pml.origin);
	VectorCopy(pm.s.velocity, pml.velocity);



	// save old org in case we get stuck
	VectorCopy (pm.s.origin, pml.previous_origin);

	pml.frametime = pm.cmd.msec * 0.001;

	PM_ClampAngles ();

	if (pm.s.pm_type == PM_SPECTATOR)
	{
		PM_FlyMove (false);
		PM_SnapPosition ();
		return;
	}

	if (pm.s.pm_type >= PM_DEAD)
	{
		pm.cmd.forwardmove = 0;
		pm.cmd.sidemove = 0;
		pm.cmd.upmove = 0;
	}

	if (pm.s.pm_type == PM_FREEZE)
		return;		// no movement at all


	VectorCopy(pm.s.mins, pm.mins);
	VectorCopy(pm.s.maxs, pm.maxs);
//	pm.viewheight = 22;

//	Com_Printf("pmove mins [%i %i %i] maxs [%i %i %i]\n", (int)pm.mins[0], (int)pm.mins[1], (int)pm.mins[2], (int)pm.maxs[0], (int)pm.maxs[1], (int)pm.maxs[2]);

	// setmins, maxs, and viewheight
	PM_CheckDuck ();

	if (pm.snapinitial)
		PM_InitialSnapPosition ();

	// set groundentity, watertype, and waterlevel
	PM_CatagorizePosition ();

	if (pm.s.pm_type == PM_DEAD)
		PM_DeadMove ();

	PM_CheckSpecialMovement ();

	// drop timing counter
	if (pm.s.pm_time)
	{
		int		msec;

		msec = pm.cmd.msec >> 3;
		if (!msec)
			msec = 1;
		if ( msec >= pm.s.pm_time) 
		{
			pm.s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm.s.pm_time = 0;
		}
		else
			pm.s.pm_time -= msec;
	}

	if (pm.s.pm_flags & PMF_TIME_TELEPORT)
	{	// teleport pause stays exactly in place
	}
	else if (pm.s.pm_flags & PMF_TIME_WATERJUMP)
	{	// waterjump has no control, but falls
		pml.velocity[2] -= pm.s.gravity * pml.frametime;
		if (pml.velocity[2] < 0)
		{	// cancel as soon as we are falling down again
			pm.s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm.s.pm_time = 0;
		}

		PM_StepSlideMove ();
	}
	else
	{
		PM_CheckJump ();

		PM_Friction ();

		if (pm.waterlevel >= 2)
			PM_WaterMove ();
		else {
			vector	angles;

			VectorCopy(pm.viewangles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;
		
			anglevectors(angles);
	
			pml.forward = v_forward;
			pml.up = v_up;
			pml.right = v_right;

			PM_AirMove ();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PM_CatagorizePosition ();

	PM_SnapPosition ();
};

#ifdef SVGAME

void(entity ent) SV_PMove =
{
};

#endif