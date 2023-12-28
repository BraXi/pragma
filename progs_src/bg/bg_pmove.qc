/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
/*
bg_pmove.qc 
-----------

Handles all player movement physics and all releated logic like jumping and stances.

PlayerMove is called by both client and server game progs respectively. 
This file must be compiled into cgame and svgame progs respectively or 
client prediction will generate errors causing desynchronization and jitter.

Compile it for progs accordingly with CLGAME or SVGAME define
*/

// enable pmove debug gui
#define PMOVEDEBUGMENU 0 // 1 = client, 2 = server	

//
// macros to convert angles back and forth 
// from shorts (network) to floats (game)
//
#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

// ---------------------------------------------------
//
// player movement types
//
typedef enum 
{
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// fly move, no clipping to world
	PM_DEAD,		// no acceleration or turning
	PM_GIB,			// no acceleration or turning, diferent bbox size
	PM_FREEZE		// cannot move and may hover above ground
} pmtype_t;
// ---------------------------------------------------
//
// player movement flags
//
float PMF_CROUCHING			= 1;	// crouching
float PMF_JUMP_HELD			= 2;	// holding jump
float PMF_ON_GROUND			= 4;	// touching ground
float PMF_TIME_WATERJUMP	= 8;	// pm_time is waterjump
float PMF_TIME_LAND			= 16;	// pm_time is time before rejump
float PMF_TIME_TELEPORT		= 32;	// pm_time is non-moving time
float PMF_NO_PREDICTION		= 64;	// temporarily disables prediction -- !!! THIS MUST MATCH C CODE !!!
float PMF_FREE				= 128;	// the only free for use pmflag, can't have more without changing pm_flags to short in C network code
// ---------------------------------------------------
//
// movement parameters
//
float	pm_stopspeed = 100;
float	pm_maxspeed = 300;			// normal speed
float	pm_crouchspeed = 100;		// crouch speed
float	pm_accelerate = 10;			// normal acceleration
float	pm_airaccelerate = 0;		// acceleration in air
float	pm_wateraccelerate = 10;	// acceleration in water
float	pm_friction = 6;			// normal friction
float	pm_waterfriction = 1;		// friction in water
float	pm_waterspeed = 400;		// speed in water
float	pm_stepsize = 18;			// can step on this high stairs
float	pm_maxtouch = 32;			// can be touching up to this many entities
// ---------------------------------------------------	
#define	STOP_EPSILON	0.1		// make floating point happy
#define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5
// ---------------------------------------------------
// this is communicated to clients for prediction
// used for in/out communication (matches pmove_state_t)
#ifdef SVGAME /* client game has this in globals - TODO maybe server should have it in globals too? */
pmtype_t	pm_state_pm_type;		// byte
vector		pm_state_origin;		// floats
vector		pm_state_velocity;		// floats
float		pm_state_gravity;		// short
vector		pm_state_mins;			// char [-127,127 range]
vector		pm_state_maxs;			// char [-127,127 range]
float		pm_state_pm_flags;		// byte [0-255]
float		pm_state_pm_time;		// byte [0-255]
vector		pm_state_delta_angles;	// shorts, use ANGLE2SHORT/SHORT2ANGLE
#endif
// ---------------------------------------------------
// pmove_t
int			pmove_cmd_msec;				// short, use ANGLE2SHORT/SHORT2ANGLE
vector		pmove_cmd_angles;			// shorts, use SHORT2ANGLE / ANGLE2SHORT
float		pmove_cmd_forwardmove;		// multiplied by `cl_forwardspeed` cvar
float		pmove_cmd_sidemove;			// multiplied by `cl_sidespeed` cvar
float		pmove_cmd_upmove;			// multiplied by `cl_upspeed` cvar
int			pmove_cmd_buttons;			// button bitfield sent by the client
float		pmove_snapinitial;			// if state has been changed outside pmove
int			pmove_numtouch;				// number of entities in touch (unused clientside)
entity		pmove_touchents[pm_maxtouch];	// array of server entities (unused clientside)
vector		pmove_viewangles;			// clamped in CL_ClampAngles and PM_ClampAngles
float		pmove_viewheight;			// added to viewoffset_z
float		pmove_groundentity_num;		// index to sventity we're standing on, -1 = in air, 0 = worldspawn, etc..
int			pmove_watertype;			// CONTENTS_WATER, CONTENTS_LAVA or CONTENTS_SLIME
float		pmove_waterlevel;			// WL_ enum
// ---------------------------------------------------
//pml_t
// locals known only to pmove
vector		pml_origin;					// full float precision
vector		pml_velocity;				// full float precision
vector		pml_forward;				// anglevectors fwd
vector		pml_right;					// anglevectors right
vector		pml_up;						// anglevectors up
float		pml_frametime;				// scale movement by this time
string		pml_groundsurface_name;		// name of texture we're on (ex. dev/floor_grey_01)
float		pml_groundsurface_flags;	// surface flags - SURF_SLICK, SURF_WARP etc
float		pml_groundsurface_value;	// texture's 'value' (conveyor belt speed etc)
int			pml_groundcontents;			// CONTENTS were standing on
vector		pml_previous_origin;		// used as a backup if we somehow get stuck in solid
float		pml_onLadder;				// set to true when we're touching ladder texture
// ---------------------------------------------------

vector(vector veca, float scale, vector vecb) VectorMA;
float(vector v) VectorNormalize;
vector(vector v) VectorNormalize2;

#ifdef SVGAME
entity pm_passent;
#endif

/*
==================
PM_Trace

uses trace function that matches cgame and svgame builtins
==================
*/
void(vector start, vector mins, vector maxs, vector end) PM_Trace =
{
#ifdef SVGAME	
	if (pm_passent.health > 0)
		trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
	
#else
	
	trace(start, mins, maxs, end, localplayernum+1, MASK_PLAYERSOLID);

#if 1
	// ugly gross hack to determine if we're on ground
	if(trace_fraction == 1)
		trace_entnum = ENTITYNUM_NONE;
	else
		trace_entnum = 0;		
#endif
	
#endif
};


/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
vector(vector in, vector normal, float overbounce) PM_ClipVelocity =
{
	float	backoff;
	float	change;
	vector 	out;
	
	backoff = dotproduct(in, normal);
	backoff = backoff * overbounce;

	change = normal_x * backoff;
	out_x = in_x - change;
	if (out_x > -STOP_EPSILON && out_x < STOP_EPSILON)
		out_x = 0;
	
	change = normal_y * backoff;
	out_y = in_y - change;
	if (out_y > -STOP_EPSILON && out_y < STOP_EPSILON)
		out_y = 0;

	change = normal_z * backoff;
	out_z = in_z - change;
	if (out_z > -STOP_EPSILON && out_z < STOP_EPSILON)
		out_z = 0;	

	return out;
};




/*
==================
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of sliding along it.
Returns a new origin, velocity, and contact entity Does not modify any world state?
==================
*/
void() PM_StepSlideMove_ =
{
	float		bumpcount, numbumps;
	vector		dir;
	float		d;
	float		numplanes;
	vector		planes[MAX_CLIP_PLANES];
	vector		primal_velocity;
	float		i, j;
	vector		end;
	float		time_left;
	
	numbumps = 4;
	numplanes = 0;
	
	primal_velocity = pml_velocity;
	time_left = pml_frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		end_x = pml_origin_x + (time_left * pml_velocity_x);
		end_y = pml_origin_y + (time_left * pml_velocity_y);
		end_z = pml_origin_z + (time_left * pml_velocity_z);

		PM_Trace(pml_origin, pm_state_mins, pm_state_maxs, end);

		if (trace_allsolid)
		{	
//			print("PM_StepSlideMove_ trace_allsolid\n");
			// entity is trapped in another solid
			pml_velocity_z = 0;	// don't build up falling damage
			return;
		}

		if (trace_fraction > 0)
		{	
			// actually covered some distance
			pml_origin = trace_endpos;
			numplanes = 0;
		}

		if(trace_fraction == 1)
			 break;		// moved the entire distance

#ifdef SVGAME // server only?
		// save entity for contact
		if(pmove_numtouch < pm_maxtouch && trace_ent)
		{
			pmove_touchents[pmove_numtouch] = trace_ent;
			pmove_numtouch++;
		}
#endif		
		time_left -= time_left * trace_fraction;

		// slide along this plane
		if(numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen			
//			dprint("numplanes >= MAX_CLIP_PLANES\n");
			pml_velocity = vec3_origin;
			break;
		}

		planes[numplanes] = trace_plane_normal;
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for(i = 0; i < numplanes; i++)
		{
			pml_velocity = PM_ClipVelocity(pml_velocity, planes[i], 1.01);
			for( j = 0; j < numplanes; j++)
			{
				if(j != i)
				{
					if(dotproduct(pml_velocity, planes[j]) < 0)
					{
//						print("PM_StepSlideMove_ dotproduct(",vtos(pml_velocity),",",vtos(planes[j]),") < 0\n");
						break;	// not ok
					}
				}
			}
			if (j == numplanes)
				break;	
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
//				dprint("clip velocity, numplanes == ", ftos(numplanes), "\n");
				pml_velocity = vec3_origin;
				break;
			}
			
		
			dir = crossproduct(planes[0], planes[1]);
			d = dotproduct(dir, pml_velocity);			
			pml_velocity_x = dir_x * d;
			pml_velocity_y = dir_y * d;
			pml_velocity_z = dir_z * d;
		}
		
		//
		// if velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (dotproduct(pml_velocity, primal_velocity) <= 0)
		{
//			dprint("dotproduct(pml_velocity, primal_velocity) <= 0\n");
			pml_velocity = vec3_origin;
			break;
		}
	}

	if (pm_state_pm_time)
	{
		pml_velocity = primal_velocity;
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

	start_o = pml_origin;
	start_v = pml_velocity;

	PM_StepSlideMove_ ();

	down_o = pml_origin;
	down_v = pml_velocity;

	up = start_o;
	up_z += pm_stepsize;

	PM_Trace(up, pm_state_mins, pm_state_maxs, up);
	if (trace_allsolid)
		return;		// can't step up

	// try sliding above
	pml_origin = up;
	pml_velocity = start_v;

	PM_StepSlideMove_ ();

	// push down the final amount
	down = pml_origin;
	down_z -= pm_stepsize;
	
	PM_Trace(pml_origin, pm_state_mins, pm_state_maxs, down);
	if (!trace_allsolid)
	{
		pml_origin = trace_endpos;
	}

	up = pml_origin;

	// decide which one went farther
    down_dist = (down_o_x - start_o_x)*(down_o_x - start_o_x)
        + (down_o_y - start_o_y)*(down_o_y - start_o_y);
    up_dist = (up_x - start_o_x)*(up_x - start_o_x)
        + (up_y - start_o_y)*(up_y - start_o_y);

	if (down_dist > up_dist || trace_plane_normal_z < MIN_STEP_NORMAL)
	{
		pml_origin = down_o;
		pml_velocity = down_v;
		return;
	}
	
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pml_velocity_z = down_v_z;
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
	
	vel = pml_velocity;
	
	speed = sqrt(vel_x*vel_x +vel_y*vel_y + vel_z*vel_z);
	if (speed < 1)
	{
		vel_x = 0;
		vel_y = 0;
		pml_velocity = vel;
		return;
	}

	drop = 0;

	//
	//  apply ground friction only if we're on ground and not on a SLICK surface
	//
	if ((pmove_groundentity_num != ENTITYNUM_NONE && !(pml_groundsurface_flags & SURF_SLICK) ) || pml_onLadder )
	{
		friction = pm_friction;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml_frametime;
	}

	// apply water friction
	if (pmove_waterlevel && !pml_onLadder)
		drop += speed * pm_waterfriction * pmove_waterlevel * pml_frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	vel = vel * newspeed;	
	pml_velocity = vel;
};


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
void(vector wishdir, float wishspeed, float accel) PM_Accelerate =
{
	float		addspeed, accelspeed, currentspeed;

	currentspeed = dotproduct (pml_velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	
	if (addspeed <= 0)
		return;
	
	accelspeed = accel * pml_frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	pml_velocity_x += accelspeed * wishdir_x;
	pml_velocity_y += accelspeed * wishdir_y;
	pml_velocity_z += accelspeed * wishdir_z;
};


/*
==============
PM_AirAccelerate

Handles user intended acceleration IN AIR
==============
*/
void(vector wishdir, float wishspeed, float accel) PM_AirAccelerate =
{
	float		addspeed, accelspeed, currentspeed, wishspd;
	
	wishspd = wishspeed;
	if (wishspd > 30)
		wishspd = 30;
	
	currentspeed = dotproduct(pml_velocity, wishdir);
	
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	
	accelspeed = (accel * wishspeed * pml_frametime);
	if (accelspeed > addspeed)
		accelspeed = addspeed;
		
	pml_velocity_x += accelspeed * wishdir_x;
	pml_velocity_y += accelspeed * wishdir_y;
	pml_velocity_z += accelspeed * wishdir_z;
};


/*
=============
PM_AddCurrents

Ladder controls, water currents and conveyor belts
=============
*/
void(__inout vector	wishvel) PM_AddCurrents =
{
	vector	v;
	float	s;

	//
	// account for ladders
	//
	if (pml_onLadder && fabs(pml_velocity_z) <= 200)
	{
		if ((pmove_viewangles_x <= -15) && (pmove_cmd_forwardmove > 0))
			wishvel_z = 200;
		else if ((pmove_viewangles_x >= 15) && (pmove_cmd_forwardmove > 0))
			wishvel_z = -200;
		else if (pmove_cmd_upmove > 0)
			wishvel_z = 200;
		else if (pmove_cmd_upmove < 0)
			wishvel_z = -200;
		else
			wishvel_z = 0;

		// limit horizontal speed when on a ladder
		if (wishvel_x < -25)
			wishvel_x = -25;
		else if (wishvel_x > 25)
			wishvel_x = 25;

		if (wishvel_y < -25)
			wishvel_y = -25;
		else if (wishvel_y > 25)
			wishvel_y = 25;
	}


	//
	// add water currents
	//
	if (pmove_watertype & MASK_CURRENT)
	{
		v = vec3_origin;

		if (pmove_watertype & CONTENTS_CURRENT_0)
			v_x += 1;
		if (pmove_watertype & CONTENTS_CURRENT_90)
			v_y += 1;
		if (pmove_watertype & CONTENTS_CURRENT_180)
			v_x -= 1;
		if (pmove_watertype & CONTENTS_CURRENT_270)
			v_y -= 1;
		if (pmove_watertype & CONTENTS_CURRENT_UP)
			v_z += 1;
		if (pmove_watertype & CONTENTS_CURRENT_DOWN)
			v_z -= 1;

		s = pm_waterspeed;
		if ((pmove_waterlevel == WL_TOUCHING) && (pmove_groundentity_num != ENTITYNUM_NONE))
			s /= 2;

		wishvel = VectorMA (wishvel, s, v);
	}

	//
	// add conveyor belt velocities
	//
	if (pmove_groundentity_num != ENTITYNUM_NONE)
	{
		v = vec3_origin;

		if (pml_groundcontents & CONTENTS_CURRENT_0)
			v_x += 1;
		if (pml_groundcontents & CONTENTS_CURRENT_90)
			v_y += 1;
		if (pml_groundcontents & CONTENTS_CURRENT_180)
			v_x -= 1;
		if (pml_groundcontents & CONTENTS_CURRENT_270)
			v_y -= 1;
		if (pml_groundcontents & CONTENTS_CURRENT_UP)
			v_z += 1;
		if (pml_groundcontents & CONTENTS_CURRENT_DOWN)
			v_z -= 1;

		wishvel = VectorMA (wishvel, 100 /* pmove_groundentity->speed */, v);
	}
};


/*
===================
PM_WaterMove

Handles movement when in water
===================
*/
void() PM_WaterMove =
{
	vector	wishvel;
	float	wishspeed;
	vector	wishdir;

	//
	// user intentions
	//
	wishvel_x = (pml_forward_x * pmove_cmd_forwardmove) + (pml_right_x * pmove_cmd_sidemove);
	wishvel_y = (pml_forward_y * pmove_cmd_forwardmove) + (pml_right_y * pmove_cmd_sidemove);	
	wishvel_z = (pml_forward_z * pmove_cmd_forwardmove) + (pml_right_z * pmove_cmd_sidemove);
	

	// if user doesn't press move keys then drift towards bottom
	// add upmove to Z wishvel
	if(!pmove_cmd_forwardmove && !pmove_cmd_sidemove && !pmove_cmd_upmove)
		wishvel_z -= 60;	
	else
		wishvel_z += pmove_cmd_upmove;

	PM_AddCurrents(wishvel);

	wishdir = wishvel;
	wishspeed = VectorNormalize(wishdir); //ugly
	wishdir = VectorNormalize2(wishdir); //ugly

	if (wishspeed > pm_maxspeed)
	{
		wishvel = wishvel * (pm_maxspeed/wishspeed);
		wishspeed = pm_maxspeed;
	}
	
	// scale down wishspeed in water
	wishspeed *= 0.5;

	PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);
	PM_StepSlideMove();
};


/*
===================
PM_AirMove

Handles all the movement when flying
===================
*/
void() PM_AirMove =
{
	vector		wishvel;
	float		fmove, smove;
	vector		wishdir;
	float		wishspeed;
	float		maxspeed;

	fmove = pmove_cmd_forwardmove;
	smove = pmove_cmd_sidemove;
	
//!!!!! pitch should be 1/3 so this isn't needed??!
#if 0
	pml_forward_z = 0;
	pml_right_z = 0;
	VectorNormalize (pml_forward);
	VectorNormalize (pml_right);
#endif

	wishvel_x = pml_forward_x * fmove + pml_right_x * smove;
	wishvel_y = pml_forward_y * fmove + pml_right_y * smove;
	wishvel_z = 0;

	PM_AddCurrents (wishvel);

	wishdir = wishvel;
	wishspeed = VectorNormalize(wishdir); // <- this returns length	
	wishdir = VectorNormalize2(wishdir); // <- this normalizes vector	

//
// clamp to server defined max speed
//
	maxspeed = (pm_state_pm_flags & PMF_CROUCHING) ? pm_crouchspeed : pm_maxspeed;

	if (wishspeed > maxspeed)
	{
		wishvel = wishvel * (maxspeed/wishspeed);
		wishspeed = maxspeed;
	}
	
	if ( pml_onLadder )
	{
		// when on ladder
		PM_Accelerate (wishdir, wishspeed, pm_accelerate);
		if (!wishvel_z)
		{
			if (pml_velocity_z > 0)
			{
				pml_velocity_z -= pm_state_gravity * pml_frametime;
				if (pml_velocity_z < 0)
					pml_velocity_z  = 0;
			}
			else
			{
				pml_velocity_z += pm_state_gravity * pml_frametime;
				if (pml_velocity_z > 0)
					pml_velocity_z  = 0;
			}
		}
		PM_StepSlideMove ();
	}
	else if ( pmove_groundentity_num != ENTITYNUM_NONE )
	{	
		// when on ground
		
		pml_velocity_z = 0; //!!! this is before the accel
		PM_Accelerate (wishdir, wishspeed, pm_accelerate);

		//fix for negative trigger_gravity fields
		//pml_velocity_z = 0;
		
		if(pm_state_gravity > 0)
			pml_velocity_z = 0;
		else
			pml_velocity_z -= pm_state_gravity * pml_frametime;

		if (!pml_velocity_x && !pml_velocity_y)
			return;
		
		PM_StepSlideMove ();
	}
	else
	{	
		// not on ground, so little effect on velocity
		if (pm_airaccelerate)
			PM_AirAccelerate(wishdir, wishspeed, pm_accelerate);
		else
			PM_Accelerate(wishdir, wishspeed, 1);
		// add gravity
		pml_velocity_z -= pm_state_gravity * pml_frametime;
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
	point_x = pml_origin_x;
	point_y = pml_origin_y;
	point_z = pml_origin_z - 0.25; // braxi -- was -0.25
	
	if (pml_velocity_z > 180) //!!ZOID changed from 100 to 180 (ramp accel)
	{
		pm_state_pm_flags &= ~PMF_ON_GROUND;
		pmove_groundentity_num = ENTITYNUM_NONE;
	}
	else
	{
		PM_Trace(pml_origin, pm_state_mins, pm_state_maxs, point);

		pml_groundsurface_name = trace_surface_name;
		pml_groundsurface_flags = trace_surface_flags;
		pml_groundsurface_value = trace_surface_value;
		
		pml_groundcontents = trace_contents;

		if (trace_entnum == ENTITYNUM_NONE || (trace_plane_normal_z < MIN_STEP_NORMAL && !trace_startsolid) )
		{
			// ground is sloppy, remove onground
			pmove_groundentity_num = ENTITYNUM_NONE;
			pm_state_pm_flags &= ~PMF_ON_GROUND;
		}
		else
		{
			pmove_groundentity_num = trace_entnum;

			// hitting solid ground will end a waterjump
			if (pm_state_pm_flags & PMF_TIME_WATERJUMP)
			{
				pm_state_pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
				pm_state_pm_time = 0;
			}

			if (! (pm_state_pm_flags & PMF_ON_GROUND) )
			{	// just hit the ground
				pm_state_pm_flags |= PMF_ON_GROUND;
				// don't do landing time if we were just going down a slope
				if (pml_velocity_z < -200)
				{
					pm_state_pm_flags |= PMF_TIME_LAND;
					// don't allow another jump for a little while
					if (pml_velocity_z < -400)
						pm_state_pm_time = 25;	
					else
						pm_state_pm_time = 18;
				}
			}
		}
		
#if 0 
		if (trace_fraction < 1.0 && trace_entnum != ENTITYNUM_NONE && pml_velocity_z < 0)
			pml_velocity_z = 0;
#endif

#ifdef SVGAME // server only?
		if (pmove_numtouch < pm_maxtouch && trace_ent)
		{
			pmove_touchents[pmove_numtouch] = trace_ent;
			pmove_numtouch++;
		}
#endif
	}

//
// get waterlevel, accounting for ducking
//
	pmove_waterlevel = WL_NOTINWATER;
	pmove_watertype = 0;

	sample2 = pmove_viewheight - pm_state_mins_z;
	sample1 = sample2 / 2;

	point_z = pml_origin_z + pm_state_mins_z + 1;	
	cont = pointcontents (point);

	if (cont & MASK_WATER)
	{
		pmove_watertype = cont;
		pmove_waterlevel = WL_TOUCHING;
		point_z = pml_origin_z + pm_state_mins_z + sample1;
		
		cont = pointcontents (point);
		if (cont & MASK_WATER)
		{
			pmove_waterlevel = WL_HALFWAY;
			point_z = pml_origin_z + pm_state_mins_z + sample2;
			
			cont = pointcontents (point);
			if (cont & MASK_WATER)
				pmove_waterlevel = WL_UNDERWATER;
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
	if (pm_state_pm_flags & PMF_TIME_LAND)
	{	
		// hasn't been long enough since landing to jump again
		return;
	}

	if(pmove_cmd_upmove < 10)
	{	
		// not holding jump
		pm_state_pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (pm_state_pm_flags & PMF_JUMP_HELD)
		return;

	if (pm_state_pm_type == PM_DEAD)
		return; // dead don't jump

	if (pmove_waterlevel >= WL_HALFWAY)
	{	
		// swimming, not jumping
		pmove_groundentity_num = ENTITYNUM_NONE;

		if (pml_velocity_z <= -300)
			return;

		if (pmove_watertype == CONTENTS_WATER)
			pml_velocity_z = 100;
		else if (pmove_watertype == CONTENTS_SLIME)
			pml_velocity_z = 80;
		else
			pml_velocity_z = 50; // weapest jump in lava
		return;
	}

	if (pmove_groundentity_num == ENTITYNUM_NONE)
		return;		// in air, so no effect

	pm_state_pm_flags |= PMF_JUMP_HELD;

	pmove_groundentity_num = ENTITYNUM_NONE;
	pml_velocity_z += 270;
	if (pml_velocity_z < 270)
		pml_velocity_z = 270;
};


/*
=============
PM_CheckSpecialMovement

check if we're touching ladder and handles jumping out of liquids
=============
*/
void() PM_CheckSpecialMovement =
{
	vector	spot;
	int		cont;
	vector	flatforward;

	if (pm_state_pm_time)
		return;

	pml_onLadder = false;

	// check for ladder
	flatforward_x = pml_forward_x;
	flatforward_y = pml_forward_y;
	flatforward_z = 0;
	
	flatforward = VectorNormalize2(flatforward);
//	VectorNormalize (flatforward);
	
	spot = VectorMA (pml_origin, 1, flatforward);
	
	PM_Trace (pml_origin, pm_state_mins, pm_state_maxs, spot);
	
	if ((trace_fraction < 1) && (trace_contents & CONTENTS_LADDER))
		pml_onLadder = true; //we're on ladder

	// check for water jump
	if (pmove_waterlevel != WL_HALFWAY)
		return;

	spot = VectorMA (pml_origin, 30, flatforward);
	spot_z += 4;
	cont = pointcontents (spot);
	if (!(cont & CONTENTS_SOLID))
		return;

	spot_z += 16;
	cont = pointcontents (spot);
	if (cont)
		return;
	
	// jump out of water
	pml_velocity = (flatforward * 50);
	pml_velocity_z = 350;

	pm_state_pm_flags |= PMF_TIME_WATERJUMP;
	pm_state_pm_time = 255;
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
	vector	wishvel;
	float	fmove, smove;
	vector	wishdir;
	float	wishspeed;
	vector	end;

	pmove_viewheight = 22; // FIXME, why is it hardcoded here?!

	// friction
	speed = vectorlength(pml_velocity);
	if (speed < 1)
	{
		pml_velocity = vec3_origin;
	}
	else
	{
		drop = 0;

		friction = pm_friction * 1.5;	// extra friction
		
		if(speed < pm_stopspeed) //control = speed < pm_stopspeed ? pm_stopspeed : speed;
			control = pm_stopspeed;
		else
			control = speed;
		
		drop += control * friction * pml_frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		pml_velocity = pml_velocity * newspeed;
	}

	// accelerate
	fmove = pmove_cmd_forwardmove;
	smove = pmove_cmd_sidemove;
	
	pml_forward = VectorNormalize2 (pml_forward); //ugly
	pml_right = VectorNormalize2 (pml_right);
	
	wishvel_x = pml_forward_x*fmove + pml_right_x*smove;
	wishvel_y = pml_forward_y*fmove + pml_right_y*smove;
	wishvel_z = pml_forward_z*fmove + pml_right_z*smove;
	
	wishvel_z += pmove_cmd_upmove;

	wishdir = wishvel;
	wishspeed = VectorNormalize(wishdir);	// ugly
	wishdir = VectorNormalize2(wishdir); //ugly
	
	//
	// clamp to server defined max speed
	//
	if (wishspeed > pm_maxspeed)
	{
		wishvel = (wishvel * (pm_maxspeed/wishspeed));
		wishspeed = pm_maxspeed;
	}

	currentspeed = dotproduct(pml_velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	
	accelspeed = pm_accelerate * pml_frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	pml_velocity_x += accelspeed * wishdir_x;	
	pml_velocity_y += accelspeed * wishdir_y;	
	pml_velocity_z += accelspeed * wishdir_z;	


	if (doclip) 
	{
		end_x = pml_origin_x + pml_frametime * pml_velocity_x;
		end_y = pml_origin_y + pml_frametime * pml_velocity_y;
		end_z = pml_origin_z + pml_frametime * pml_velocity_z;

		PM_Trace(pml_origin, pm_state_mins, pm_state_maxs, end);

		pml_origin = trace_endpos;
	} 
	else 
	{
		// move
		pml_origin = VectorMA (pml_origin, pml_frametime, pml_velocity);
	}
};


/*
==============
PM_SetSizeAndViewHeight

Sets players bbox mins, maxs, and viewheight

pmtype,    mins,          maxs,        viewheight
standing   [-16 -16 -24]  [16 16 32]   22
crouching  [-16 -16 -24]  [16 16  4]   -2
gibbed     [-16 -16 0.0]  [16 16 16]    8
==============
*/
void() PM_SetSizeAndViewHeight =
{
#if 0
	if (pm_state_pm_type == PM_NORMAL_NO_CROUCH)
	{
		pm_state_pm_flags &= ~PMF_CROUCHING;
		return;
	}
#endif 

	pm_state_mins_x = -16;
	pm_state_mins_y = -16;

	pm_state_maxs_x = 16;
	pm_state_maxs_y = 16;

	if (pm_state_pm_type == PM_GIB)
	{
		pm_state_mins_z = 0;
		pm_state_maxs_z = 16;
		pmove_viewheight = 8;
		return;
	}

	pm_state_mins_z = -24;

	if (pm_state_pm_type == PM_DEAD)
	{
		pm_state_pm_flags |= PMF_CROUCHING;
	}
	else if (pmove_cmd_upmove < 0 && (pm_state_pm_flags & PMF_ON_GROUND))
	{	
		// crouch
		pm_state_pm_flags |= PMF_CROUCHING;
	}
	else
	{	
		// stand up if possible
		if (pm_state_pm_flags & PMF_CROUCHING)
		{
			// try to stand up
			pm_state_maxs_z = 32;
			PM_Trace (pml_origin, pm_state_mins, pm_state_maxs, pml_origin);
			
			if (!trace_allsolid)
				pm_state_pm_flags &= ~PMF_CROUCHING;
		}
	}

	if (pm_state_pm_flags & PMF_CROUCHING)
	{
		pm_state_maxs_z = 4;
		pmove_viewheight = -2;
	}
	else
	{
		pm_state_maxs_z = 32;
		pmove_viewheight = 22;
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

	if (pmove_groundentity_num == ENTITYNUM_NONE)
		return;

	// extra friction
	forward = vectorlength (pml_velocity);
	forward -= 20;
	
	if (forward <= 0)
	{
		pml_velocity = vec3_origin;
	}
	else
	{
		pml_velocity = VectorNormalize2 (pml_velocity); //ugly
		//VectorNormalize (pml_velocity);
		pml_velocity = (pml_velocity * forward);
	}
};


float() PM_GoodPosition =
{
	if (pm_state_pm_type == PM_NOCLIP)
		return true;

	PM_Trace (pm_state_origin, pm_state_mins, pm_state_maxs, pm_state_origin);
	
	return !trace_allsolid;
}


/*
================
PM_SnapPosition
================
*/
void() PM_SnapPosition =
{
	pm_state_velocity = pml_velocity;
	pm_state_origin = pml_origin;

	if (PM_GoodPosition())
		return;

	// go back to the last position if position is wrong
	pm_state_origin = pml_previous_origin;

	// kex
	//	if (G_FixStuckObject_Generic(pm_state_origin, pm_state_mins, pm_state_maxs, PM_Trace_Auto) == stuck_result_t::NO_GOOD_POSITION) {
	//		pm_state_origin = pml_previous_origin;
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
	float			x, y, z;
	vector      base;
	static float offset[3] = { 0, -1, 1 };

	base = pm_state_origin;

	for ( z = 0; z < 3; z++ ) 
	{
		pm_state_origin_z = base_z + offset[ z ];
		for ( y = 0; y < 3; y++ ) 
		{
			pm_state_origin_y = base_y + offset[ y ];
			for ( x = 0; x < 3; x++ ) 
			{
				pm_state_origin_x = base_x + offset[ x ];
				if (PM_GoodPosition ()) 
				{
					pml_origin_x = pm_state_origin_x;
					pml_origin_y = pm_state_origin_y;
					pml_origin_z = pm_state_origin_z;

					pml_previous_origin = pm_state_origin;
					return;
				}
			}
		}
	}

//	dprint("Bad InitialSnapPosition\n");
};



/*
================
PM_ClampAngles
================
*/
void() PM_ClampAngles =
{
	float	temp;

	if (pm_state_pm_flags & PMF_TIME_TELEPORT)
	{
		pmove_viewangles_x = 0;
		pmove_viewangles_y = SHORT2ANGLE(pmove_cmd_angles_y + pm_state_delta_angles_y);
		pmove_viewangles_z = 0;
	}
	else
	{
		// circularly clamp the angles with deltas
		temp = pmove_cmd_angles_x + pm_state_delta_angles_x;
		pmove_viewangles_x = SHORT2ANGLE(temp);
		
		temp = pmove_cmd_angles_y + pm_state_delta_angles_y;
		pmove_viewangles_y = SHORT2ANGLE(temp);
		
		temp = pmove_cmd_angles_z + pm_state_delta_angles_z;
		pmove_viewangles_z = SHORT2ANGLE(temp); // temp*0.0054931640625;

		// don't let the player look up or down more than 90 degrees
		// FIXME: Also clamped clientside in CL_ClampPitch
		if (pmove_viewangles_x > 89 && pmove_viewangles_x < 180)
			pmove_viewangles_x = 89;
		else if (pmove_viewangles_x < 271 && pmove_viewangles_x >= 180)
			pmove_viewangles_x = 271;
	}
	
	anglevectors(pmove_viewangles);
	
	pml_forward = v_forward;
	pml_up = v_up;
	pml_right = v_right;
};


/*
================
ClearPlayerMoveLocals

Clears all pmove local vars before performing simulation
================
*/
void() ClearPlayerMoveLocals =
{
	pml_origin = vec3_origin;
	pml_velocity = vec3_origin;
	pml_forward = vec3_origin;
	pml_right = vec3_origin;
	pml_up = vec3_origin;
	pml_frametime = 0;
	pml_groundsurface_name = "";
	pml_groundsurface_flags = 0;
	pml_groundsurface_value = 0;
	pml_groundcontents = 0;
	pml_previous_origin = vec3_origin;	
	pml_onLadder = false;
};


/*
================
ClearPlayerMove

Clears all pmove vars before performing simulation
================
*/
void() ClearPlayerMove =
{
	entity none; // don't set, this must be null entity


	// commands
	pmove_cmd_msec = 0;
	pmove_cmd_angles = vec3_origin;
	pmove_cmd_forwardmove = 0;
	pmove_cmd_sidemove = 0;
	pmove_cmd_upmove = 0;
#if 0		
	pmove_snapinitial = false;

	// state
	pm_state_pm_type = PM_NORMAL;	
	pm_state_origin = vec3_origin;
	pm_state_velocity = vec3_origin;
	pm_state_gravity = 0;	
	pm_state_mins = vec3_origin;
	pm_state_maxs = vec3_origin;
	pm_state_pm_flags = 0;
	pm_state_pm_time = 0;
	pm_state_delta_angles = vec3_origin;
#endif
		
	// results
#ifdef SVGAME	
	pmove_numtouch = 0;
	
	for(float i = 0; i < pm_maxtouch; i++)
		pmove_touchents[i] = none;
#endif

	pmove_viewangles = vec3_origin;
	pmove_viewheight = 0;

	pmove_groundentity_num = ENTITYNUM_NONE;
	pmove_watertype = 0;
	pmove_waterlevel = WL_NOTINWATER;
};

/*
================
BG_PlayerMove

Can be called by either the server or the client
================
*/
void() BG_PlayerMove =
{
	// clear results
	pmove_numtouch = 0;
	pmove_viewangles = vec3_origin;
	pmove_viewheight = 0;
	pmove_groundentity_num = ENTITYNUM_NONE;
	pmove_watertype = 0;
	pmove_waterlevel = WL_NOTINWATER;

	// clear all pmove local vars
	ClearPlayerMoveLocals();

	// copy to locals
	pml_origin = pm_state_origin;
	pml_velocity = pm_state_velocity;

	// save old org in case we get stuck
	pml_previous_origin = pm_state_origin;

	pml_frametime = pmove_cmd_msec * 0.001;

	PM_ClampAngles ();
	
//	pm_state_pm_type = PM_NOCLIP; //test

	if (pm_state_pm_type == PM_NOCLIP)
	{
		PM_FlyMove (false);
		PM_SnapPosition ();
		return;
	}

	if (pm_state_pm_type >= PM_DEAD)
	{
		pmove_cmd_forwardmove = 0;
		pmove_cmd_sidemove = 0;
		pmove_cmd_upmove = 0;
	}

	if (pm_state_pm_type == PM_FREEZE)
		return;		// no movement at all


	// set crouching, mins, maxs, and viewheight
	PM_SetSizeAndViewHeight();

	if (pmove_snapinitial)
		PM_InitialSnapPosition();

	// set groundentity, watertype, and waterlevel
	PM_CatagorizePosition();

	if (pm_state_pm_type == PM_DEAD)
		PM_DeadMove();

	PM_CheckSpecialMovement();

	// drop timing counter
	if (pm_state_pm_time)
	{
		int		msec;

		msec = pmove_cmd_msec >> 3;
		if (!msec)
			msec = 1;
		if ( msec >= pm_state_pm_time) 
		{
			pm_state_pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm_state_pm_time = 0;
		}
		else
			pm_state_pm_time -= msec;
	}

	if (pm_state_pm_flags & PMF_TIME_TELEPORT)
	{	
		// teleport pause stays exactly in place
	}
	else if (pm_state_pm_flags & PMF_TIME_WATERJUMP)
	{	
		// waterjump has no control, but falls
		pml_velocity_z -= pm_state_gravity * pml_frametime;
		if (pml_velocity_z < 0)
		{	// cancel as soon as we are falling down again
			pm_state_pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm_state_pm_time = 0;
		}

		PM_StepSlideMove ();
	}
	else
	{
		PM_CheckJump ();
		PM_Friction ();

		if (pmove_waterlevel >= WL_HALFWAY)
		{
			// in water - swim
			PM_WaterMove ();
		}
		else 
		{
			vector	angles;

			angles = pmove_viewangles;
			if (angles_x > 180)
				angles_x = angles_x - 360;
			angles_x /= 3;
		
			anglevectors(angles);
	
			pml_forward = v_forward;
			pml_up = v_up;
			pml_right = v_right;

			PM_AirMove ();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PM_CatagorizePosition ();
	PM_SnapPosition ();
};


/*
================
CG_PlayerMove

cmdMove = [forwardmove, sidemove, upmove]
cmdAngles = [pitch, yaw, roll]

Client's entry point to pmove
================
*/
#ifdef CLGAME
void(vector cmdMove, vector cmdAngles, float cmdButtons, float cmdMsec) CG_PlayerMove
{
	//
	// wipe all pmove data
	//
	ClearPlayerMove();
	
	//
	// copy data to pmove
	//	
	pmove_viewheight = cam_viewoffset_z;
	pmove_cmd_forwardmove = cmdMove_x;
	pmove_cmd_sidemove = cmdMove_y;
	pmove_cmd_upmove = cmdMove_z;
	pmove_cmd_msec = cmdMsec;
	pmove_cmd_angles = cmdAngles;
	pmove_cmd_buttons = cmdButtons;

#if PMOVEDEBUGMENU == 1
	cvarset("pmdebug_0", "movecmd: ", ftos(cmdMove_x)," FWD, ", ftos(cmdMove_y), " SIDE, ", ftos(cmdMove_z)," UP");
	cvarset("pmdebug_1", "[in] origin  : ", vtos(pm_state_origin));
	cvarset("pmdebug_2", "[in] velocity: ", vtos(pm_state_velocity));
#endif
		
	//
	// perform a player move simulation
	//
	BG_PlayerMove();	
	

#if PMOVEDEBUGMENU == 1
	cvarset("pmdebug_3", "[out] origin  : ", vtos(pm_state_origin));
	cvarset("pmdebug_4", "[out] velocity: ", vtos(pm_state_velocity));	
	cvarset("pmdebug_5", "GRND ", ftos(pmove_groundentity_num), " (", pml_groundsurface_name, "), pm_type ", ftos(pm_state_pm_type));
#endif	
	
	vector viewofs;
	viewofs_z = pmove_viewheight;

	cam_viewoffset = viewofs;
	cam_viewangles = pmove_viewangles;	
}
#endif


/*
================
BG_PlayerMove

cmdMove = [forwardmove, sidemove, upmove]
cmdAngles = [pitch, yaw, roll]

Can be called by either the server or the client
================
*/
#ifdef SVGAME
void(vector cmdMove, vector cmdAngles, int cmdButtons, int cmdMsec) SV_PlayerMove =
{
	entity ground;
	
	//
	// wipe all pmove data
	//
	ClearPlayerMove();
	
	// we don't want to collide with ourselves
	pm_passent = self; 
	
	//
	// copy data to pmove
	//
	pmove_viewheight = self.viewheight;
	
	//cmdMove = [forwardmove, sidemove, upmove]
	pmove_cmd_forwardmove = cmdMove_x;
	pmove_cmd_sidemove = cmdMove_y;
	pmove_cmd_upmove = cmdMove_z;
	pmove_cmd_msec = cmdMsec;
	pmove_cmd_angles = cmdAngles;
	pmove_cmd_buttons = cmdButtons;
	
	pm_state_pm_type = self.pm_type;
	pm_state_origin = self.origin;
	pm_state_velocity = self.velocity;
	pm_state_mins = self.mins;
	pm_state_maxs = self.maxs;
	pm_state_pm_flags = self.pm_flags;
	pm_state_pm_time = self.pm_time;
	pm_state_gravity = (cvar("sv_gravity") * self.gravity);

#if PMOVEDEBUGMENU == 2
	cvarset("pmdebug_0", "movecmd: ", ftos(cmdMove_x)," FWD, ", ftos(cmdMove_y), " SIDE, ", ftos(cmdMove_z)," UP");
	cvarset("pmdebug_1", "[in] origin  : ", vtos(pm_state_origin));
	cvarset("pmdebug_2", "[in] velocity: ", vtos(pm_state_velocity));
#endif

	//
	// perform a player move simulation
	//
	BG_PlayerMove();
	
	// play a jump sfx if needed	
	if(self.groundentity_num != ENTITYNUM_NONE && pmove_groundentity_num == ENTITYNUM_NONE && (pmove_cmd_upmove >= 10) && (pmove_waterlevel == WL_NOTINWATER))
	{	
		//braxi -- compiled in too early to see these
		//if(self.playerAnimTree)
		//	playsound(vec3_origin, self, CHAN_VOICE, animTree[self.playerAnimTree].snd_jump, 1, ATTN_NORM, 0);
		playsound(vec3_origin, self, CHAN_VOICE, "player/female/jump1.wav", 1, ATTN_NORM, 0);
	}


#if PMOVEDEBUGMENU == 2	
	cvarset("pmdebug_3", "[out] origin  : ", vtos(pm_state_origin));
	cvarset("pmdebug_4", "[out] velocity: ", vtos(pm_state_velocity));	
	if(self.groundentity_num > 0)
	{
		ground = getent(self.groundentity_num);	
		cvarset("pmdebug_5", "GRND ", ftos(pmove_groundentity_num), " (", ground.classname, "), pm_type ", ftos(pm_state_pm_type));
	}
	else
		cvarset("pmdebug_5", "GRND ", ftos(pmove_groundentity_num), " (", pml_groundsurface_name, "), pm_type ", ftos(pm_state_pm_type));
#endif
	
	//
	// save results of pmove
	//	
	// no setorigin() as we linkentity at the end
	self.origin = pm_state_origin;		
	self.velocity = pm_state_velocity;
		
	self.pm_type = pm_state_pm_type;
	self.pm_flags = pm_state_pm_flags;
	self.pm_time = pm_state_pm_time;
	self.pm_gravity = pm_state_gravity;	
	
	self.viewheight = pmove_viewheight;
	
	self.waterlevel = pmove_waterlevel;
	self.watertype = pmove_watertype;

	// set ground entity
	self.groundentity_num = pmove_groundentity_num;
	if (self.groundentity_num != ENTITYNUM_NONE)
	{
		ground = getent(self.groundentity_num);
		self.groundentity_linkcount = ground.groundentity_linkcount;
	}

	vector viewofs;
	viewofs_z = self.viewheight;
	setviewoffset(self, viewofs);

	self.v_angle = pmove_viewangles;
	setviewangles(self, pmove_viewangles);

	self.mins = pm_state_mins;
	self.maxs = pm_state_maxs;
	setsize(self, pm_state_mins, pm_state_maxs); // set new bbox
	linkentity(self);
};
#endif


vector(vector veca, float scale, vector vecb) VectorMA =
{
	vector out;
	out_x = veca_x + scale * vecb_x;
	out_y = veca_y + scale * vecb_y;
	out_z = veca_z + scale * vecb_z;
	return out;
};

// this should be a builtin!
float(vector v) VectorNormalize =
{
	float	length; //, ilength;

	length = v_x*v_x + v_y*v_y + v_z*v_z;
	length = sqrt (length);

//	if (length)
//	{
//		ilength = 1/length;
//		v_x = v_x * ilength;
//		v_y = v_y * ilength;
//		v_z = v_z * ilength;
//	}
		
	return length;
}

vector(vector v) VectorNormalize2 =
{
	vector out;
	float	length, ilength;

	length = v_x*v_x + v_y*v_y + v_z*v_z;
	length = sqrt (length);

	if (length)
	{
		ilength = 1/length;
		v_x = v_x * ilength;
		v_y = v_y * ilength;
		v_z = v_z * ilength;
	}
		
	out = v;
	return out;
}