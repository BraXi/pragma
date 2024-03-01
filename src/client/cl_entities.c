/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cl_ents.c -- entity management

#include "client.h"

/*
==========================================================================

INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/

static unsigned int c_effects, c_renderfx;

/*
===============
CL_EntityAnimation
===============
*/
static inline void CL_EntityAnimationOld(clentity_t* clent, entity_state_t* state, rentity_t *refent)
{
	int		autoanim;
	unsigned int effects = state->effects;

	// models can auto animate their frames
	autoanim = 2 * cl.time / 1000;

	//
	// set animation frame
	//
	if (effects & EF_ANIM01)				/* automatically cycle between frames 0 and 1 at 2 hz */
		refent->frame = autoanim & 1;
	else if (effects & EF_ANIM23)			/* automatically cycle between frames 2 and 3 at 2 hz */
		refent->frame = 2 + (autoanim & 1);
	else if (effects & EF_ANIM_ALL)			/* automatically cycle through all frames at 2hz */
		refent->frame = autoanim;
	else if (effects & EF_ANIM_ALLFAST)		/* automatically cycle through all frames at 10hz */
		refent->frame = cl.time / SV_FRAMETIME_MSEC;
	else
		refent->frame = state->frame;		/* .. or just let gamecode drive animation */

	refent->oldframe = clent->prev.frame;
	refent->backlerp = 1.0 - cl.lerpfrac;
}


/*
===============
CL_EntityAnimation
===============
*/
static inline void CL_EntityAnimation(clentity_t* clent, entity_state_t* state, rentity_t* refent)
{
	animstate_t* anim = NULL;
	int	progress;

	/* Case One:
	* We have animtime set
	*/
	if (state->animStartTime > 0 && clent->current.frame == clent->prev.frame)
	{
//		int anim_firstframe = state->frame & 255;
//		int anim_lastframe = (state->frame >> 8) & 255;
//		int anim_rate = (state->frame >> 16) & 255;
//		int anim_flags = (state->frame >> 24) & 255;

		clent->anim.starttime = state->animStartTime;
		clent->anim.startframe = state->frame;
		clent->anim.rate = (1000.0f / 10.0f);
		anim = &clent->anim;
	}
	else
	{
		refent->frame = clent->current.frame;
		refent->oldframe = clent->prev.frame;
		refent->animbacklerp = refent->backlerp;
		return;
	}

	if (anim != NULL && clent->anim.rate > 0)
	{
		progress = ((cl.frame.servertime - state->animStartTime) / clent->anim.rate);
		int curanimtime = clent->anim.starttime + (progress * clent->anim.rate);

		refent->animbacklerp = 1.0f - ((cl.time - ((float)curanimtime - SV_FRAMETIME_MSEC)) / clent->anim.rate);
		clamp(refent->animbacklerp, 0.0f, 1.0f);

		refent->frame = anim->startframe + progress;
		refent->oldframe = refent->frame -1;
	}
	//	state->animtime = (int)(1000.0 * ent->v.animtime) / 1000.0;
}

/*
===============
CL_EntityPositionAndRotation
===============
*/
static inline void CL_EntityPositionAndRotation(clentity_t* clent, entity_state_t* state, rentity_t *refent)
{
	int i;
	float	current_angles, previous_angles;

	//
	// ORIGIN
	// 

	// interpolate origin
	for (i = 0; i < 3; i++)
	{
		refent->origin[i] = refent->oldorigin[i] = clent->prev.origin[i] + cl.lerpfrac * (clent->current.origin[i] - clent->prev.origin[i]);
	}

	//
	// ROTATION
	//
	if (state->effects & EF_ROTATEYAW)
	{	// some bonus items auto-rotate
		refent->angles[0] = 0;
		refent->angles[1] = anglemod(cl.time / 10.0);
		refent->angles[2] = 0;
	}
	else
	{
		// interpolate angles
		for (i = 0; i < 3; i++)
		{
			current_angles = clent->current.angles[i];
			previous_angles = clent->prev.angles[i];
			refent->angles[i] = LerpAngle(previous_angles, current_angles, cl.lerpfrac);
		}
	}

	AnglesToAxis(refent->angles, refent->axis);
}

static void PositionRotatedEntityOnTag(rentity_t* entity, rentity_t* parent, int parentModel, int tagIndex)
{
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

//	Com_Printf("old %i cur %i lerp %f\n", parent->oldframe, parent->frame, 1.0 - parent->animbacklerp);
	re.LerpTag(&lerped, cl.model_draw[parentModel], parent->oldframe, parent->frame, 1.0 - parent->animbacklerp, tagIndex);

	VectorCopy(parent->origin, entity->origin);
	for (i = 0; i < 3; i++) 
	{
		VectorMA(entity->origin, lerped.origin[i], parent->axis[i], entity->origin);
	}

	// cast away const because of compiler problems
	MatrixMultiply(entity->axis, parent->axis, tempAxis);
	MatrixMultiply(lerped.axis, tempAxis, entity->axis);
	entity->backlerp = parent->backlerp;
}

/*
===============
CL_EntityAddAttachedModels

Add attached models, but don't use custom skins on them
===============
*/
static inline void CL_EntityAddAttachedModels(clentity_t* clent, entity_state_t* state, rentity_t *refent)
{
	ent_model_t* attachInfo;
	rentity_t attachEnt;
	vec3_t angles;
	rentity_t r;

//	refent->frame = 0;
	refent->skinnum = 0;
//	refent->renderfx = 0;
	for (int i = 0; i < MAX_ATTACHED_MODELS; i++)
	{
		attachInfo = &state->attachments[i];
		if (attachInfo->modelindex == 0 || attachInfo->parentTag == 0)
			continue;

		//memset(&attachEnt, 0, sizeof(attachEnt));
		attachEnt = *refent; // copy all of it
		attachEnt.frame = 0;
		r = *refent;

		AxisClear(attachEnt.axis);
		PositionRotatedEntityOnTag(&attachEnt, &r, clent->current.modelindex, (attachInfo->parentTag - 1));

		attachEnt.model = cl.model_draw[state->attachments[i].modelindex];
//		attachEnt.inheritLight = refent->index;
		VectorAngles(attachEnt.axis[0], attachEnt.axis[2], angles);
		VectorCopy(angles, attachEnt.angles);

		V_AddEntity(&attachEnt);
	}
}

/*
===============
CL_EntityAddParticleTrails

Add particle trails to entity, they may have dlight attached to them
===============
*/
extern void CG_PartFX_DiminishingTrail(vec3_t start, vec3_t end, clentity_t* old, int flags);
static inline void CL_EntityAddParticleTrails(clentity_t* clent, entity_state_t* state, rentity_t* refent)
{
	unsigned int effects = state->effects;

	/* rocket trail */
	if (effects & EF_ROCKET)
	{
		CG_PartFX_RocketTrail(clent->lerp_origin, refent->origin, clent);
		V_AddLight(refent->origin, 128, 1.000000, 0.670588, 0.027451);
	}
	/* diminishing blood trail */
	else if (effects & EF_GIB)
	{
		CG_PartFX_DiminishingTrail(clent->lerp_origin, refent->origin, clent, effects);
	}
	/* diminishing 'smoke' trail */
	else if (effects & EF_GRENADE)
	{
		CG_PartFX_DiminishingTrail(clent->lerp_origin, refent->origin, clent, effects);
	}
}


/*
===============
CL_EntityAddMiscEffects

Mostly effects that were previously in trails code but shouldn't be
===============
*/
static inline void CL_EntityAddMiscEffects(clentity_t* clent, entity_state_t* state, rentity_t *refent)
{
	// unused currently
}

/*
===============
CL_AddPacketEntities
===============
*/
static char *etypes[] = { "ET_GENERAL", "ET_PLAYER","ET_PATHNODE", "ET_ACTOR" };
void CL_AddPacketEntities(frame_t* frame)
{
	clentity_t		*clent;		// currently parsed client entity
	entity_state_t	*state;		// current client entity's state
	rentity_t		rent;		// this is refdef entity passed to renderer
	int				entnum;
	unsigned int	effects, renderfx;

	memset(&rent, 0, sizeof(rent)); // move to loop so nothing will ever leak to next clent?

	//
	// parse all client entities
	//
	for (entnum = 0; entnum < frame->num_entities; entnum++)
	{
		state = &cl_parse_entities[(frame->parse_entities + entnum) & (MAX_PARSE_ENTITIES - 1)];
		clent = &cl_entities[(int)state->number];

		effects = state->effects;
		renderfx = state->renderFlags;

		rent.backlerp = (1.0 - cl.lerpfrac);
		rent.inheritLight = -1;
		rent.hiddenPartsBits = state->hidePartBits;

		//if (clent->current.eType > 0)
		//	Com_Printf("clent %i is %s\n", state->number, etypes[clent->current.eType]);

		//
		// create a new render entity
		//
		CL_EntityAnimation(clent, state, &rent);
		CL_EntityPositionAndRotation(clent, state, &rent);

		// special case for local player entity, otherwise camera would be inside of a player model
		if (state->number == cl.playernum + 1)
		{
#if 0
			// draw own model, better to draw it not here, but at the end of frame so it sticks to view
			rent.angles[0] = rent.angles[2] = 0;
			vec3_t forward;
			AngleVectors(rent.angles, forward, NULL, NULL);
			VectorMA(rent.origin, -10, forward, rent.origin);
#else
			rent.renderfx |= RF_VIEWERMODEL;	// only draw from mirrors
			continue;
#endif
		}


		rent.skinnum = state->skinnum;
		rent.model = cl.model_draw[state->modelindex];
//		rent.renderfx = state->renderFlags; // set later on


#if 0
		//
		// add dynamic light
		// TODO: figure out lightstyle
		//
		if (state->effects & RF_LIGHT)
		{
			V_AddLight(rent.origin, state->lightIntensity, state->lightColor[0], state->lightColor[1], state->lightColor[2]);
		}
		else if (state->effects & RF_NEGATIVE_LIGHT)
		{
			V_AddLight(rent.origin, state->lightIntensity, -state->lightColor[0], -state->lightColor[1], -state->lightColor[2]);
		}
#endif
		//
		// if entity has no model just skip at this point
		//
		if (!state->modelindex)
			continue;

		//
		// add entity model
		//
		rent.renderfx = state->renderFlags; // clent->current.
		rent.alpha = state->renderAlpha;
		VectorCopy(state->renderColor, rent.renderColor);
		rent.scale = state->renderScale;

		// FIXME: this is a big temporary hack for (borrowed from) Q2R models :v
		if (rent.renderfx & RF_YAWHACK)
			rent.angles[1] -= 90; 

		// add entity to refresh list
		V_AddEntity(&rent);

		//
		// add attached models if any
		//
		CL_EntityAddAttachedModels(clent, state, &rent);

		//
		// add trails
		//
		CL_EntityAddParticleTrails(clent, state, &rent);

		//
		// add misc effects
		//
		CL_EntityAddMiscEffects(clent, state, &rent);

		//
		// save origin to for lerping between frames
		//
		VectorCopy(rent.origin, clent->lerp_origin);
	}
}

/*
==============
CL_AddViewWeapon
==============
*/
extern muzzleflash_t cl_muzzleflashes[FX_WEAPON_MUZZLEFLASHES];

void CL_AddViewWeapon (player_state_t *ps, player_state_t *ops)
{
	rentity_t	viewmodel; 
	int			i;
	rentity_t	flash;
	vec3_t  v_fwd, v_right, v_up;
	muzzleflash_t* mz;

	// allow the gun to be completely removed
	if (!cl_drawviewmodel->value)
		return;

	//
	// set up viewmodel
	//
	memset (&viewmodel, 0, sizeof(viewmodel));

	if (gun_model && CL_CheatsAllowed())
	{
		// development tool
		viewmodel.model = gun_model;
	}
	else
	{
		viewmodel.model = cl.model_draw[ps->viewmodel_index];
	}

	if (!viewmodel.model)
		return;

	// set up position
	for (i = 0; i < 3; i++)
	{
		viewmodel.origin[i] = cl.refdef.view.origin[i] + ops->viewmodel_offset[i]
			+ cl.lerpfrac * (ps->viewmodel_offset[i] - ops->viewmodel_offset[i]);
		viewmodel.angles[i] = cl.refdef.view.angles[i] + LerpAngle (ops->viewmodel_angles[i],
			ps->viewmodel_angles[i], cl.lerpfrac);
	}

	if (gun_frame && CL_CheatsAllowed())
	{
		// development tool
		viewmodel.frame = gun_frame;
		viewmodel.oldframe = gun_frame;
		viewmodel.backlerp = 1.0 - cl.lerpfrac;
	}
	else
	{
		viewmodel.frame = ps->viewmodel_frame;

		// just changed weapons, don't lerp from oldframe and remove muzzleflash if present
		if (ps->viewmodel_index != ops->viewmodel_index)
		{
			cl.muzzleflash_time = 0;
			viewmodel.oldframe = viewmodel.frame;
			viewmodel.backlerp = 1;
		}
		else
		{
			viewmodel.oldframe = ops->viewmodel_frame;
			viewmodel.backlerp = 1.0 - cl.lerpfrac;
		}
	}
	viewmodel.animbacklerp = viewmodel.backlerp;
	VectorCopy(viewmodel.origin, viewmodel.oldorigin);
	viewmodel.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_VIEW_MODEL;
	V_AddEntity (&viewmodel);

	//
	// set up muzzleflash
	//
	if (cl.time > cl.muzzleflash_time)
		return;

	memset(&flash, 0, sizeof(flash));

	VectorCopy(viewmodel.angles, flash.angles);
	VectorCopy(viewmodel.origin, flash.origin);

	AngleVectors(flash.angles, v_fwd, v_right, v_up);

	mz = &cl_muzzleflashes[cl.muzzleflash];
	VectorMA(flash.origin, mz->forward, v_fwd, flash.origin);
	VectorMA(flash.origin, mz->up, v_up, flash.origin);
	VectorMA(flash.origin, mz->right, v_right, flash.origin);

	VectorCopy(flash.origin, flash.oldorigin);	// don't lerp at all

	flash.frame = flash.oldframe = cl.muzzleflash_frame;
	flash.renderfx = RF_FULLBRIGHT | RF_DEPTHHACK | RF_VIEW_MODEL | RF_SCALE | RF_TRANSLUCENT;
	flash.scale = mz->flashscale;
	flash.alpha = 0.7;
	flash.model = cgMedia.mod_v_muzzleflash;
	V_AddEntity(&flash);
}


/*
===============
CL_CalcViewValues

Sets cl.refdef view values
===============
*/
void CL_CalcViewValues (void)
{
	int			i;
	float		lerp, backlerp;
	clentity_t	*ent;
	frame_t		*oldframe;
	player_state_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerstate;
	i = (cl.frame.serverframe - 1) & UPDATE_MASK;
	oldframe = &cl.frames[i];
	if (oldframe->serverframe != cl.frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.frame;		// previous frame was dropped or involid
	ops = &oldframe->playerstate;

	// see if the player entity was teleported this frame
#if PROTOCOL_FLOAT_COORDS == 1
	if (fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256
		|| abs(ops->pmove.origin[1] - ps->pmove.origin[1]) > 256
		|| abs(ops->pmove.origin[2] - ps->pmove.origin[2]) > 256)
		ops = ps;		// don't interpolate
#else
	if (fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256 * 8
		|| abs(ops->pmove.origin[1] - ps->pmove.origin[1]) > 256 * 8
		|| abs(ops->pmove.origin[2] - ps->pmove.origin[2]) > 256 * 8)
		ops = ps;		// don't interpolate
#endif


	ent = &cl_entities[cl.playernum+1];
	lerp = cl.lerpfrac;

	// calculate the origin
	if ((cl_predict->value) && !(cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// use predicted values
		unsigned	delta;

		backlerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++)
		{
			cl.refdef.view.origin[i] = cl.predicted_origin[i] + ops->viewoffset[i] 
				+ cl.lerpfrac * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * cl.prediction_error[i];
		}

		// smooth out stair climbing
		delta = cls.realtime - cl.predicted_step_time;
		if (delta < SV_FRAMETIME_MSEC)
			cl.refdef.view.origin[2] -= cl.predicted_step * (float)(SV_FRAMETIME_MSEC - delta) * 0.01f;
	}
	else
	{	// just use interpolated values
#if PROTOCOL_FLOAT_COORDS == 1
		for (i = 0; i < 3; i++)
			cl.refdef.view.origin[i] = ops->pmove.origin[i] + ops->viewoffset[i] + lerp * (ps->pmove.origin[i] + ps->viewoffset[i] - (ops->pmove.origin[i] + ops->viewoffset[i]));
#else
		for (i = 0; i < 3; i++)
			cl.refdef.vieworigin[i] = ops->pmove.origin[i] * 0.125 + ops->viewoffset[i] + lerp * (ps->pmove.origin[i] * 0.125 + ps->viewoffset[i] - (ops->pmove.origin[i] * 0.125 + ops->viewoffset[i]));
#endif
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if ( cl.frame.playerstate.pmove.pm_type < PM_DEAD )
	{	// use predicted values
		for (i=0 ; i<3 ; i++)
			cl.refdef.view.angles[i] = cl.predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cl.refdef.view.angles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	for (i=0 ; i<3 ; i++)
		cl.refdef.view.angles[i] += LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);

	AngleVectors (cl.refdef.view.angles, cl.v_forward, cl.v_right, cl.v_up);

	// interpolate field of view
	cl.refdef.view.fov_x = ops->fov + lerp * (ps->fov - ops->fov);

	// don't interpolate blend color
	for (i=0 ; i<4 ; i++)
		cl.refdef.view.fx.blend[i] = ps->blend[i];

	// add the weapon
	CL_AddViewWeapon (ps, ops);
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CL_AddEntities()
{
	if (cls.state != ca_active)
		return;

	if (cl.time > cl.frame.servertime)
	{
		if (cl_showclamp->value)
			Com_Printf ("high clamp %i\n", cl.time - cl.frame.servertime);
		cl.time = cl.frame.servertime;
		cl.lerpfrac = 1.0;
	}
	else if (cl.time < cl.frame.servertime - SV_FRAMETIME_MSEC)
	{
		if (cl_showclamp->value)
			Com_Printf ("low clamp %i\n", cl.frame.servertime - SV_FRAMETIME_MSEC - cl.time);
		cl.time = cl.frame.servertime - SV_FRAMETIME_MSEC;
		cl.lerpfrac = 0;
	}
	else
	{
		cl.lerpfrac = 1.0 - ((float)(cl.frame.servertime - cl.time) / (float)SV_FRAMETIME_MSEC);
//		cl.lerpfrac = 1.0 - (cl.frame.servertime - cl.time) * 0.01; // Q2
	}

	if (cl_timedemo->value)
		cl.lerpfrac = 1.0;

	// calculate view first so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CL_CalcViewValues ();

	CL_AddPacketEntities (&cl.frame);
	CG_AddEntities();
}

/*
===============
CL_GetEntitySoundOrigin

Called to get the sound spatialization origin
===============
*/
void CL_GetEntitySoundOrigin (int ent, vec3_t org)
{
	clentity_t	*old;

	if (ent < 0 || ent >= MAX_GENTITIES)
		Com_Error (ERR_DROP, "CL_GetEntitySoundOrigin: bad ent");
	old = &cl_entities[ent];
	VectorCopy (old->lerp_origin, org);

	// FIXME: bmodel issues...
}


/*
==================
CL_FireEntityEvents
==================
*/
void CL_FireEntityEvents(frame_t* frame)
{
	entity_state_t* s1;
	int					pnum, num;

	for (pnum = 0; pnum < frame->num_entities; pnum++)
	{
		num = (frame->parse_entities + pnum) & (MAX_PARSE_ENTITIES - 1);
		s1 = &cl_parse_entities[num];
		if (s1->event)
			CG_EntityEvent(s1);
	}
}