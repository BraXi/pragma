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
// cl_ents.c -- entity parsing and management

#include "client.h"


extern	struct model_s	*cl_mod_powerscreen;

//PGM
int	vidref_val;
//PGM

/*
=========================================================================

FRAME PARSING

=========================================================================
*/


/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int	bitcounts[32];	/// just for protocol profiling
int CL_ParseEntityBits (unsigned *bits)
{
	unsigned	b, total;
	int			i;
	int			number;

	total = MSG_ReadByte (&net_message);
	if (total & U_MOREBITS1)
	{
		b = MSG_ReadByte (&net_message);
		total |= b<<8;
	}
	if (total & U_MOREBITS2)
	{
		b = MSG_ReadByte (&net_message);
		total |= b<<16;
	}
	if (total & U_MOREBITS3)
	{
		b = MSG_ReadByte (&net_message);
		total |= b<<24;
	}

	// count the bits for net profiling
	for (i=0 ; i<32 ; i++)
		if (total&(1<<i))
			bitcounts[i]++;

	if (total & U_NUMBER_16)
		number = MSG_ReadShort (&net_message);
	else
		number = MSG_ReadByte (&net_message);

	*bits = total;

	return number;
}

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy (from->origin, to->old_origin);
	to->number = number;


	// main model
	if (bits & U_MODELINDEX_8)
		to->modelindex = MSG_ReadByte (&net_message);
	if (bits & U_MODELINDEX_16)
		to->modelindex = MSG_ReadShort(&net_message);

	// attached models
	if (bits & U_MODELINDEX2_8)
		to->modelindex2 = MSG_ReadByte (&net_message);
	if (bits & U_MODELINDEX3_8)
		to->modelindex3 = MSG_ReadByte (&net_message);
	if (bits & U_MODELINDEX4_8)
		to->modelindex4 = MSG_ReadByte (&net_message);
	
	// animation frame
	if (bits & U_FRAME_8)
		to->frame = MSG_ReadByte (&net_message);
	if (bits & U_FRAME_16)
		to->frame = MSG_ReadShort (&net_message);

	// index to model skin
	if (bits & U_SKIN_8)
		to->skinnum = MSG_ReadByte(&net_message);

	// effects
	if ( (bits & (U_EFFECTS_8|U_EFFECTS_16)) == (U_EFFECTS_8|U_EFFECTS_16) )
		to->effects = MSG_ReadLong(&net_message);
	else if (bits & U_EFFECTS_8)
		to->effects = MSG_ReadByte(&net_message);
	else if (bits & U_EFFECTS_16)
		to->effects = MSG_ReadShort(&net_message);

	// render flags
	if ( (bits & (U_RENDERFLAGS_8|U_RENDERFLAGS_16)) == (U_RENDERFLAGS_8|U_RENDERFLAGS_16) )
		to->renderFlags = MSG_ReadLong(&net_message);
	else if (bits & U_RENDERFLAGS_8)
		to->renderFlags = MSG_ReadByte(&net_message);
	else if (bits & U_RENDERFLAGS_16)
		to->renderFlags = MSG_ReadShort(&net_message);

	// render scale
	if (bits & U_RENDERSCALE)
		to->renderScale = MSG_ReadByte(&net_message) * (1.0f / 16.0f);

	// render color
	if (bits & U_RENDERCOLOR)
	{
		to->renderColor[0] = MSG_ReadByte(&net_message) * (1.0f / 255.0f);
		to->renderColor[1] = MSG_ReadByte(&net_message) * (1.0f / 255.0f);
		to->renderColor[2] = MSG_ReadByte(&net_message) * (1.0f / 255.0f);
	}

	// render alpha
	if (bits & U_RENDERALPHA)
		to->renderAlpha = MSG_ReadByte(&net_message) * (1.0f / 255.0f);

	// current origin
	if (bits & U_ORIGIN_X)
		to->origin[0] = MSG_ReadCoord (&net_message);
	if (bits & U_ORIGIN_Y)
		to->origin[1] = MSG_ReadCoord (&net_message);
	if (bits & U_ORIGIN_Z)
		to->origin[2] = MSG_ReadCoord (&net_message);

	// current angles
	if (bits & U_ANGLE_X)
		to->angles[0] = MSG_ReadAngle(&net_message);
	if (bits & U_ANGLE_Y)
		to->angles[1] = MSG_ReadAngle(&net_message);
	if (bits & U_ANGLE_Z)
		to->angles[2] = MSG_ReadAngle(&net_message);

	// old origin (used for smoothing move)
	if (bits & U_OLDORIGIN)
		MSG_ReadPos (&net_message, to->old_origin);

	// looping sound
	if (bits & U_LOOPSOUND)
	{
#ifdef PROTOCOL_EXTENDED_ASSETS
		to->loopingSound = MSG_ReadShort(&net_message);
#else
		to->loopingSound = MSG_ReadByte(&net_message);
#endif
	}

	// event
	if (bits & U_EVENT)
		to->event = MSG_ReadByte (&net_message);
	else
		to->event = 0;

	// solid
	if (bits & U_PACKEDSOLID)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		to->solid = MSG_ReadLong(&net_message);
#else
		to->solid = MSG_ReadShort(&net_message);
#endif
	}
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity (frame_t *frame, int newnum, entity_state_t *old, int bits)
{
	ccentity_t	*ent;
	entity_state_t	*state;

	ent = &cl_entities[newnum];

	state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];
	cl.parse_entities++;
	frame->num_entities++;

	CL_ParseDelta (old, state, newnum, bits);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex
		|| state->modelindex2 != ent->current.modelindex2
		|| state->modelindex3 != ent->current.modelindex3
		|| state->modelindex4 != ent->current.modelindex4
		|| abs(state->origin[0] - ent->current.origin[0]) > 512
		|| abs(state->origin[1] - ent->current.origin[1]) > 512
		|| abs(state->origin[2] - ent->current.origin[2]) > 512
		|| state->event == EV_PLAYER_TELEPORT
		|| state->event == EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		ent->trailcount = 1024;		// for diminishing rocket / grenade trails
		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == EV_OTHER_TELEPORT)
		{
			VectorCopy (state->origin, ent->prev.origin);
			VectorCopy (state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy (state->old_origin, ent->prev.origin);
			VectorCopy (state->old_origin, ent->lerp_origin);
		}
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities (frame_t *oldframe, frame_t *newframe)
{
	int				newnum;
	unsigned int	bits;
	entity_state_t	*oldstate = NULL;
	int			oldindex, oldnum;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if (!oldframe)
		oldnum = 99999;
	else
	{
		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		newnum = CL_ParseEntityBits (&bits);
		if (newnum >= MAX_GENTITIES)
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: bad number:%i", newnum);

		if (net_message.readcount > net_message.cursize)
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum)
		{	// one or more entities from the old packet are unchanged
			if (cl_shownet->value == 3)
				Com_Printf ("   unchanged: %i\n", oldnum);
			CL_DeltaEntity (newframe, oldnum, oldstate, 0);
			
			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & U_REMOVE)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet->value == 3)
				Com_Printf ("   remove: %i\n", newnum);
			if (oldnum != newnum)
				Com_Printf ("U_REMOVE: oldnum != newnum\n");

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum)
		{	// delta from previous state
			if (cl_shownet->value == 3)
				Com_Printf ("   delta: %i\n", newnum);
			CL_DeltaEntity (newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{	// delta from baseline
			if (cl_shownet->value == 3)
				Com_Printf ("   baseline: %i\n", newnum);
			CL_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{	// one or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
			Com_Printf ("   unchanged: %i\n", oldnum);
		CL_DeltaEntity (newframe, oldnum, oldstate, 0);
		
		oldindex++;

		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}



/*
===================
CL_ParsePlayerstate
===================
*/
void CL_ParsePlayerstate(frame_t *oldframe, frame_t *newframe)
{
	int			flags;
	player_state_t	*state;
	int			i;
	int			statbits;

	state = &newframe->playerstate;

	// clear to old value before delta parsing
	if (oldframe)
		*state = oldframe->playerstate;
	else
		memset (state, 0, sizeof(*state));

	flags = MSG_ReadShort (&net_message);
	if (flags & PS_EXTRABYTES)
		flags = (MSG_ReadShort (&net_message) << 16) | (flags & 0xFFFF); // reki --  Allow extra bytes, so we don't choke ourselves

	//
	// parse the pmove_state_t
	//
	if (flags & PS_M_TYPE)
		state->pmove.pm_type = MSG_ReadByte (&net_message);

	if (flags & PS_M_ORIGIN)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		for (i = 0; i < 3; i++)
			state->pmove.origin[i] = MSG_ReadFloat(&net_message);
#else
		for (i = 0; i < 3; i++)
			state->pmove.origin[i] = MSG_ReadShort(&net_message);
#endif
	}

	if (flags & PS_M_VELOCITY)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		for (i = 0; i < 3; i++)
			state->pmove.velocity[i] = MSG_ReadFloat(&net_message);
#else
		for (i = 0; i < 3; i++)
			state->pmove.velocity[i] = MSG_ReadShort(&net_message);
#endif
	}

	if (flags & PS_M_TIME)
		state->pmove.pm_time = MSG_ReadByte (&net_message);

	if (flags & PS_M_FLAGS)
	{
		state->pmove.pm_flags = MSG_ReadShort (&net_message);
		if (flags & PS_M_FLAGSLONG)
			state->pmove.pm_flags = (MSG_ReadShort (&net_message) << 16) | (state->pmove.pm_flags & 0xFFFF);
	}

	if (flags & PS_M_GRAVITY)
		state->pmove.gravity = MSG_ReadShort (&net_message);

	if (flags & PS_M_DELTA_ANGLES)
	{
		for (i = 0; i < 3; i++)
		{
#if PROTOCOL_FLOAT_PLAYERANGLES == 1
			state->pmove.delta_angles[i] = MSG_ReadFloat(&net_message);
#else
			state->pmove.delta_angles[i] = MSG_ReadShort(&net_message);
#endif
		}
	}

	if (flags & PS_M_BBOX_SIZE) // mins and maxs
	{
		for( i = 0; i < 3; i++)
			state->pmove.mins[i] = MSG_ReadChar(&net_message);

		for (i = 0; i < 3; i++)
			state->pmove.maxs[i] = MSG_ReadChar(&net_message);
	}

	if (cl.attractloop)
		state->pmove.pm_type = PM_FREEZE;		// demo playback

	//
	// parse the rest of the player_state_t
	//
	if (flags & PS_VIEWOFFSET)
	{
		for (i = 0; i < 3; i++)
			state->viewoffset[i] = MSG_ReadChar (&net_message);
	}

	if (flags & PS_VIEWANGLES)
	{
		for (i = 0; i < 3; i++)
			state->viewangles[i] = MSG_ReadAngle16 (&net_message);
	}

	if (flags & PS_KICKANGLES)
	{
		for (i = 0; i < 3; i++)
			state->kick_angles[i] = MSG_ReadChar (&net_message) * 0.25;
	}

	if (flags & PS_VIEWMODEL_INDEX)
	{
#ifdef PROTOCOL_EXTENDED_ASSETS
		state->viewmodel_index = MSG_ReadShort(&net_message);
#else
		state->viewmodel_index = MSG_ReadByte(&net_message);
#endif
	}

	if (flags & PS_VIEWMODEL_FRAME)
	{
		state->viewmodel_frame = MSG_ReadByte (&net_message); // braxi -- do I need more than 256 frames for viewmodel?

		for (i = 0; i < 3; i++)
			state->viewmodel_offset[i] = MSG_ReadChar (&net_message)*0.25;

		for (i = 0; i < 3; i++)
			state->viewmodel_angles[i] = MSG_ReadChar (&net_message)*0.25;
	}

	if (flags & PS_BLEND)
	{
		for (i = 0; i < 4; i++)
			state->blend[i] = MSG_ReadByte (&net_message)/255.0;
	}

	if (flags & PS_FOV)
		state->fov = MSG_ReadByte (&net_message);

	if (flags & PS_RDFLAGS)
		state->rdflags = MSG_ReadByte (&net_message);

	// parse stats
	statbits = MSG_ReadLong (&net_message);
	for (i=0 ; i<MAX_STATS ; i++)
		if (statbits & (1<<i) )
			state->stats[i] = MSG_ReadShort(&net_message);
}


/*
==================
CL_FireEntityEvents

==================
*/
void CL_FireEntityEvents (frame_t *frame)
{
	entity_state_t		*s1;
	int					pnum, num;

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		num = (frame->parse_entities + pnum)&(MAX_PARSE_ENTITIES-1);
		s1 = &cl_parse_entities[num];
		if (s1->event)
			CL_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if ((int)s1->effects & EF_TELEPORTER)
			CL_TeleporterParticles (s1);
	}
}


/*
================
CL_ParseFrame
================
*/
void CL_ParseFrame (void)
{
	int			cmd;
	int			len;
	frame_t		*old;

	memset (&cl.frame, 0, sizeof(cl.frame));

	cl.frame.serverframe = MSG_ReadLong (&net_message);
	cl.frame.deltaframe = MSG_ReadLong (&net_message);
	cl.frame.servertime = cl.frame.serverframe*SV_FRAMETIME_MSEC;

	cl.surpressCount = MSG_ReadByte (&net_message);

	if (cl_shownet->value == 3)
		Com_Printf ("   frame:%i  delta:%i\n", cl.frame.serverframe, cl.frame.deltaframe);

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message 
	if (cl.frame.deltaframe <= 0)
	{
		cl.frame.valid = true;		// uncompressed frame
		old = NULL;
		cls.demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.frames[cl.frame.deltaframe & UPDATE_MASK];
		if (!old->valid)
		{	// should never happen
			Com_Printf ("Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverframe != cl.frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf ("Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES-128)
		{
			Com_Printf ("Delta parse_entities too old.\n");
		}
		else
			cl.frame.valid = true;	// valid delta parse
	}

	// clamp time 
	if (cl.time > cl.frame.servertime)
		cl.time = cl.frame.servertime;
	else if (cl.time < cl.frame.servertime - SV_FRAMETIME_MSEC)
		cl.time = cl.frame.servertime - SV_FRAMETIME_MSEC;

	// read areabits
	len = MSG_ReadByte (&net_message);
	MSG_ReadData (&net_message, &cl.frame.areabits, len);

	// read playerinfo
	cmd = MSG_ReadByte (&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != SVC_PLAYERINFO)
		Com_Error (ERR_DROP, "CL_ParseFrame: not playerinfo");
	CL_ParsePlayerstate (old, &cl.frame);

	// read packet entities
	cmd = MSG_ReadByte (&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != SVC_PACKET_ENTITIES)
		Com_Error (ERR_DROP, "CL_ParseFrame: not packetentities");
	CL_ParsePacketEntities (old, &cl.frame);

	// save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverframe & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != ca_active)
		{
			cls.state = ca_active;
			cl.force_refdef = true;

#if PROTOCOL_FLOAT_COORDS == 1
			cl.predicted_origin[0] = cl.frame.playerstate.pmove.origin[0];
			cl.predicted_origin[1] = cl.frame.playerstate.pmove.origin[1];
			cl.predicted_origin[2] = cl.frame.playerstate.pmove.origin[2];
#else
			cl.predicted_origin[0] = cl.frame.playerstate.pmove.origin[0] * 0.125;
			cl.predicted_origin[1] = cl.frame.playerstate.pmove.origin[1] * 0.125;
			cl.predicted_origin[2] = cl.frame.playerstate.pmove.origin[2] * 0.125;
#endif

			VectorCopy (cl.frame.playerstate.viewangles, cl.predicted_angles);
			
			if (cls.disable_servercount != cl.servercount && cl.refresh_prepped)
				SCR_EndLoadingPlaque ();	// get rid of loading plaque
		}
		cl.sound_prepped = true;	// can start mixing ambient sounds
	
		// fire entity events
		CL_FireEntityEvents (&cl.frame);
		CL_CheckPredictionError ();
	}
}

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
static inline void CL_EntityAnimation(ccentity_t* clent, entity_state_t* state, centity_t *refent)
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
CL_EntityPositionAndRotation
===============
*/
static inline void CL_EntityPositionAndRotation(ccentity_t* clent, entity_state_t* state, centity_t *refent)
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
}

/*
===============
CL_EntityAddAttachedModels

Add attached models, but don't use custom skins on them
===============
*/
static inline void CL_EntityAddAttachedModels(ccentity_t* clent, entity_state_t* state, centity_t *refent)
{
	refent->skin = NULL;
	refent->skinnum = 0;
	refent->renderfx = 0;

	if (state->modelindex2)
	{
		refent->model = cl.model_draw[state->modelindex2];
		V_AddEntity(refent);

//		rent.renderfx = 0;
//		rent.alpha = 1;
	}

	if (state->modelindex3)
	{
		refent->model = cl.model_draw[state->modelindex3];
		V_AddEntity(refent);
	}

	if (state->modelindex4)
	{
		refent->model = cl.model_draw[state->modelindex4];
		V_AddEntity(refent);
	}
}

/*
===============
CL_EntityAddParticleTrails

Add particle trails to entity, they may have dlight attached to them
===============
*/
static inline void CL_EntityAddParticleTrails(ccentity_t* clent, entity_state_t* state, centity_t *refent)
{
	unsigned int effects = state->effects;
	float intensity;

	/* rocket trail */
	if (effects & EF_ROCKET) 
	{
		CL_RocketTrail(clent->lerp_origin, refent->origin, clent);
		V_AddLight(refent->origin, 200, 1, 1, 0);
	}
	/* blaster trail */
	else if (effects & EF_BLASTER)
	{
		if (effects & EF_TRACKER_DLIGHT) /* (EF_BLASTER | EF_TRACKER) special case */
		{
			CL_BlasterTrail2(clent->lerp_origin, refent->origin);
			V_AddLight(refent->origin, 200, 0, 1, 0);
		}
		else
		{
			CL_BlasterTrail(clent->lerp_origin, refent->origin);
			V_AddLight(refent->origin, 200, 1, 1, 0);
		}
	}
	/* hyper blaster trail */
	else if (effects & EF_HYPERBLASTER)
	{
		if (effects & EF_TRACKER_DLIGHT) /* (EF_HYPERBLASTER | EF_TRACKER) special case */
			V_AddLight(refent->origin, 200, 0, 1, 0);
		else
			V_AddLight(refent->origin, 200, 1, 1, 0);
	}
	/* diminishing blood trail */
	else if (effects & EF_GIB)
	{
		CL_DiminishingTrail(clent->lerp_origin, refent->origin, clent, effects);
	}
	/* diminishing 'smoke' trail */
	else if (effects & EF_GRENADE)
	{
		CL_DiminishingTrail(clent->lerp_origin, refent->origin, clent, effects);
	}
	else if (effects & EF_FLAG1)
	{
		vec3_t c = { 1.000000, 0.000000, 0.000000 };
		CL_FlagTrail(clent->lerp_origin, refent->origin, c);
		V_AddLight(refent->origin, 225, 1, 0.1, 0.1);
	}
	else if (effects & EF_FLAG2)
	{
		vec3_t c = { 0.184314, 0.403922, 0.498039 };
		CL_FlagTrail(clent->lerp_origin, refent->origin, c);
		V_AddLight(refent->origin, 225, 0.1, 0.1, 1);
	}
	else if (effects & EF_TAGTRAIL)
	{
		vec3_t c = { 1.000000, 1.000000, 0.152941 };
		CL_TagTrail(clent->lerp_origin, refent->origin, c);
		V_AddLight(refent->origin, 225, 1.0, 1.0, 0.0);
	}
	else if (effects & EF_TRACKERTRAIL)
	{
		if (effects & EF_TRACKER_DLIGHT) /* (EF_TRACKERTRAIL | EF_TRACKER) special case */
		{
			intensity = 50 + (500 * (sin(cl.time / 500.0) + 1.0));
			
			// FIXME - check out this effect in rendition
			if (vidref_val == VIDREF_GL)
				V_AddLight(refent->origin, intensity, -1.0, -1.0, -1.0);
			else
				V_AddLight(refent->origin, -1.0 * intensity, 1.0, 1.0, 1.0);
		}
		else
		{
			CL_Tracker_Shell(clent->lerp_origin);
			V_AddLight(refent->origin, 155, -1.0, -1.0, -1.0);
		}
	}
	else if (effects & EF_TRACKER_DLIGHT)
	{
		vec3_t c = { 0,0,0 };
		CL_TrackerTrail(clent->lerp_origin, refent->origin, c);
		// FIXME - check out this effect in rendition
		if (vidref_val == VIDREF_GL)
			V_AddLight(refent->origin, 200, -1, -1, -1);
		else
			V_AddLight(refent->origin, -200, 1, 1, 1);
	}
	else if (effects & EF_GREENGIB)
	{
		CL_DiminishingTrail(clent->lerp_origin, refent->origin, clent, effects);
	}
	else if (effects & EF_IONRIPPER)
	{
		CL_IonripperTrail(clent->lerp_origin, refent->origin);
		V_AddLight(refent->origin, 100, 1, 0.5, 0.5);
	}
	else if (effects & EF_BLUEHYPERBLASTER)
	{
		V_AddLight(refent->origin, 200, 0, 0, 1);
	}
	else if (effects & EF_PLASMA)
	{
		if (effects & EF_ANIM_ALLFAST) /* (EF_PLASMA | EF_ANIM_ALLFAST) special case */
		{
			CL_BlasterTrail(clent->lerp_origin, refent->origin);
		}
		V_AddLight(refent->origin, 130, 1, 0.5, 0.5);
	}
}


/*
===============
CL_EntityAddMiscEffects

Mostly effects that were previously in trails code but shouldn't be
===============
*/
static inline void CL_EntityAddMiscEffects(ccentity_t* clent, entity_state_t* state, centity_t *refent)
{
	if (state->effects & EF_FLIES)
	{
		CL_FlyEffect(clent, refent->origin);
	}
	else if (state->effects & EF_TRAP)
	{
		refent->origin[2] += 32;
		CL_TrapParticles(refent);
		V_AddLight(refent->origin, ((rand() % 100) + 100), 1, 0.8, 0.1);
	}
}

/*
===============
CL_AddPacketEntities

FIXME - add beams back
FIXME - if server sets this entity a dlight, then skip adding dlights from effects

===============
*/
void CL_AddPacketEntities(frame_t* frame)
{
	ccentity_t		*clent;		// currently parsed client entity
	entity_state_t	*state;		// current client entity's state
	centity_t		rent;		// this is refdef entity passed to renderer
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
		rent.skin = NULL;
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


#if 0 // old messy shit, keept to readd beams later
void CL_AddPacketEntities (frame_t *frame)
{
	centity_t			ent;
	entity_state_t		*s1;
	float				autorotate;
	int					i;
	int					pnum;
	ccentity_t			*cent;
	int					autoanim;
	clientinfo_t		*ci;
	unsigned int		effects, renderfx;

	// bonus items rotate at a fixed rate
	autorotate = anglemod(cl.time/10.0);

	// brush models can auto animate their frames
	autoanim = 2*cl.time/1000;

	memset (&ent, 0, sizeof(ent));

	for (pnum = 0 ; pnum < frame->num_entities; pnum++)
	{
		s1 = &cl_parse_entities[(frame->parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];

		cent = &cl_entities[(int)s1->number];

		effects = s1->effects;
		renderfx = s1->renderFlags;


		// set frame
		if (effects & EF_ANIM01)
			ent.frame = autoanim & 1;
		else if (effects & EF_ANIM23)
			ent.frame = 2 + (autoanim & 1);
		else if (effects & EF_ANIM_ALL)
			ent.frame = autoanim;
		else if (effects & EF_ANIM_ALLFAST)
			ent.frame = cl.time / SV_FRAMETIME_MSEC;
		else
			ent.frame = s1->frame;


		ent.oldframe = cent->prev.frame;
		ent.backlerp = 1.0 - cl.lerpfrac;

		ent.alpha = cent->current.renderAlpha;

		if (renderfx & (RF_FRAMELERP|RF_BEAM))
		{	// step origin discretely, because the frames
			// do the animation properly
			VectorCopy (cent->current.origin, ent.origin);
			VectorCopy (cent->current.old_origin, ent.oldorigin);
		}
		else
		{	// interpolate origin
			for (i=0 ; i<3 ; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac * (cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		// create a new entity
	
		// tweak the color of beams
		if ( renderfx & RF_BEAM )
		{	// the four beam colors are encoded in 32 bits of skinnum (hack)
			ent.alpha = 0.30;
			(int)ent.skinnum = ((int)s1->skinnum >> ((rand() % 4)*8)) & 0xff;
			ent.model = NULL;
		}
		else
		{
#if 0
			// set skin
			if (s1->modelindex == 255)
			{	// use custom player skin
				ent.skinnum = 0;
				ci = &cl.clientinfo[(int)s1->skinnum & 0xff];
				ent.skin = ci->skin;
				ent.model = ci->model;
				if (!ent.skin || !ent.model)
				{
					ent.skin = cl.baseclientinfo.skin;
					ent.model = cl.baseclientinfo.model;
				}

//============
//PGM
				if (renderfx & RF_USE_DISGUISE)
				{
					if(!strncmp((char *)ent.skin, "players/male", 12))
					{
						ent.skin = re.RegisterSkin ("players/male/disguise.pcx");
						ent.model = re.RegisterModel ("players/male/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/female", 14))
					{
						ent.skin = re.RegisterSkin ("players/female/disguise.pcx");
						ent.model = re.RegisterModel ("players/female/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/cyborg", 14))
					{
						ent.skin = re.RegisterSkin ("players/cyborg/disguise.pcx");
						ent.model = re.RegisterModel ("players/cyborg/tris.md2");
					}
				}
//PGM
//============
			}
			else
#endif
			{
				ent.skinnum = s1->skinnum;
				ent.skin = NULL;
				ent.model = cl.model_draw[(int)s1->modelindex];
			}
		}


		//if (renderfx == RF_TRANSLUCENT)
		//	ent.alpha = s1->renderAlpha;


		// render effects (fullbright, translucent, etc)
		if ((effects & EF_COLOR_SHELL))
			ent.renderfx = 0;	// renderfx go on color shell entity
		else
			ent.renderfx = renderfx;


		// calculate angles
		if (effects & EF_ROTATE)
		{	// some bonus items auto-rotate
			ent.angles[0] = 0;
			ent.angles[1] = autorotate;
			ent.angles[2] = 0;
		}
		// RAFAEL
		else if (effects & EF_SPINNINGLIGHTS)
		{
			ent.angles[0] = 0;
			ent.angles[1] = anglemod(cl.time/2) + s1->angles[1];
			ent.angles[2] = 180;
			{
				vec3_t forward;
				vec3_t start;

				AngleVectors (ent.angles, forward, NULL, NULL);
				VectorMA (ent.origin, 64, forward, start);
				V_AddLight (start, 100, 1, 0, 0);
			}
		}
		else
		{	// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);
			}
		}

		if (s1->number == cl.playernum+1)
		{
			ent.renderfx |= RF_VIEWERMODEL;	// only draw from mirrors
			// FIXME: still pass to refresh

			if (effects & EF_FLAG1)
				V_AddLight (ent.origin, 225, 1.0, 0.1, 0.1);
			else if (effects & EF_FLAG2)
				V_AddLight (ent.origin, 225, 0.1, 0.1, 1.0);
			else if (effects & EF_TAGTRAIL)						//PGM
				V_AddLight (ent.origin, 225, 1.0, 1.0, 0.0);	//PGM
			else if (effects & EF_TRACKERTRAIL)					//PGM
				V_AddLight (ent.origin, 225, -1.0, -1.0, -1.0);	//PGM

			continue;
		}


		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		if (effects & EF_BFG)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			ent.alpha = 0.30;
		}

		// RAFAEL
		if (effects & EF_PLASMA)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			ent.alpha = 0.6;
		}

		if (effects & EF_SPHERETRANS)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			if (effects & EF_TRACKERTRAIL)
				ent.alpha = 0.6;
			else
				ent.alpha = 0.3;
		}
//pmm
		ent.renderfx = cent->current.renderFlags;
		ent.alpha = cent->current.renderAlpha;
		VectorCopy(cent->current.renderColor, ent.renderColor);
		ent.scale = cent->current.renderScale;
		

		if (ent.renderfx & RF_LIGHT) //hack
		{
			ent.angles[1] -= 90;
			//V_AddLight(ent.origin, 200, cent->current.renderColor[0], cent->current.renderColor[1], cent->current.renderColor[2]);
		}

		// add to refresh list
		V_AddEntity (&ent);

		// color shells generate a seperate entity for the main model
		if (effects & EF_COLOR_SHELL)
		{
			ent.renderfx = renderfx | RF_TRANSLUCENT;
			ent.alpha = 0.30;
			V_AddEntity (&ent);
		}

		ent.skin = NULL;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.renderfx = 0;
		ent.alpha = 0;

		// duplicate for linked models
		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255)
			{	// custom weapon
				ci = &cl.clientinfo[(int)s1->skinnum & 0xff];
				i = ((int)s1->skinnum >> 8); // 0 is default weapon model
				if (!cl_vwep->value || i > MAX_CLIENTWEAPONMODELS - 1)
					i = 0;
				ent.model = ci->weaponmodel[i];
				if (!ent.model) {
					if (i != 0)
						ent.model = ci->weaponmodel[0];
					if (!ent.model)
						ent.model = cl.baseclientinfo.weaponmodel[0];
				}
			}
			//PGM - hack to allow translucent linked models (defender sphere's shell)
			//		set the high bit 0x80 on modelindex2 to enable translucency
			else if((int)s1->modelindex2 & 0x80)
			{
				ent.model = cl.model_draw[(int)s1->modelindex2 & 0x7F];
				ent.alpha = 0.32;
				ent.renderfx = RF_TRANSLUCENT;
			}
			//PGM
			else
				ent.model = cl.model_draw[(int)s1->modelindex2];
			V_AddEntity (&ent);

			//PGM - make sure these get reset.
			ent.renderfx = 0;
			ent.alpha = 0;
			//PGM
		}
		if (s1->modelindex3)
		{
			ent.model = cl.model_draw[(int)s1->modelindex3];
			V_AddEntity (&ent);
		}
		if (s1->modelindex4)
		{
			ent.model = cl.model_draw[(int)s1->modelindex4];
			V_AddEntity (&ent);
		}

		if ( effects & EF_POWERSCREEN )
		{
			ent.model = cl_mod_powerscreen;
			ent.oldframe = 0;
			ent.frame = 0;
			ent.renderfx |= (RF_TRANSLUCENT | RF_SHELL_GREEN);
			ent.alpha = 0.30;
			V_AddEntity (&ent);
		}

		// add automatic particle trails
		if ( (effects&~EF_ROTATE) )
		{
			if (effects & EF_ROCKET)
			{
				CL_RocketTrail (cent->lerp_origin, ent.origin, cent);
				V_AddLight (ent.origin, 200, 1, 1, 0);
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER. 
			// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
			else if (effects & EF_BLASTER)
			{
//				CL_BlasterTrail (cent->lerp_origin, ent.origin);
//PGM
				if (effects & EF_TRACKER)	// lame... problematic?
				{
					CL_BlasterTrail2 (cent->lerp_origin, ent.origin);
					V_AddLight (ent.origin, 200, 0, 1, 0);		
				}
				else
				{
					CL_BlasterTrail (cent->lerp_origin, ent.origin);
					V_AddLight (ent.origin, 200, 1, 1, 0);
				}
//PGM
			}
			else if (effects & EF_HYPERBLASTER)
			{
				if (effects & EF_TRACKER)						// PGM	overloaded for blaster2.
					V_AddLight (ent.origin, 200, 0, 1, 0);		// PGM
				else											// PGM
					V_AddLight (ent.origin, 200, 1, 1, 0);
			}
			else if (effects & EF_GIB)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & EF_GRENADE)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & EF_FLIES)
			{
				CL_FlyEffect (cent, ent.origin);
			}
			else if (effects & EF_BFG)
			{
				static int bfg_lightramp[6] = {300, 400, 600, 300, 150, 75};

				if (effects & EF_ANIM_ALLFAST)
				{
					CL_BfgParticles (&ent);
					i = 200;
				}
				else
				{
					i = bfg_lightramp[(int)s1->frame];
				}
				V_AddLight (ent.origin, i, 0, 1, 0);
			}
			// RAFAEL
			else if (effects & EF_TRAP)
			{
				ent.origin[2] += 32;
				CL_TrapParticles (&ent);
				i = (rand()%100) + 100;
				V_AddLight (ent.origin, i, 1, 0.8, 0.1);
			}	
			else if (effects & EF_FLAG1)
			{
				vec3_t c = { 1.000000, 0.000000, 0.000000 }; //242
				CL_FlagTrail (cent->lerp_origin, ent.origin, c);
				V_AddLight (ent.origin, 225, 1, 0.1, 0.1);
			}
			else if (effects & EF_FLAG2)
			{
				vec3_t c = { 0.184314, 0.403922, 0.498039 }; //115
				CL_FlagTrail (cent->lerp_origin, ent.origin, c);
				V_AddLight (ent.origin, 225, 0.1, 0.1, 1);
			}
//======
//ROGUE
			else if (effects & EF_TAGTRAIL)
			{
				vec3_t c = { 1.000000, 1.000000, 0.152941 }; //220
				CL_TagTrail (cent->lerp_origin, ent.origin, c);
				V_AddLight (ent.origin, 225, 1.0, 1.0, 0.0);
			}
			else if (effects & EF_TRACKERTRAIL)
			{
				if (effects & EF_TRACKER)
				{
					float intensity;

					intensity = 50 + (500 * (sin(cl.time/500.0) + 1.0));
					// FIXME - check out this effect in rendition
					if(vidref_val == VIDREF_GL)
						V_AddLight (ent.origin, intensity, -1.0, -1.0, -1.0);
					else
						V_AddLight (ent.origin, -1.0 * intensity, 1.0, 1.0, 1.0);
					}
				else
				{
					CL_Tracker_Shell (cent->lerp_origin);
					V_AddLight (ent.origin, 155, -1.0, -1.0, -1.0);
				}
			}
			else if (effects & EF_TRACKER)
			{
				vec3_t c = { 0,0,0 };
				CL_TrackerTrail (cent->lerp_origin, ent.origin, c);
				// FIXME - check out this effect in rendition
				if(vidref_val == VIDREF_GL)
					V_AddLight (ent.origin, 200, -1, -1, -1);
				else
					V_AddLight (ent.origin, -200, 1, 1, 1);
			}
//ROGUE
//======
			// RAFAEL
			else if (effects & EF_GREENGIB)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);				
			}
			// RAFAEL
			else if (effects & EF_IONRIPPER)
			{
				CL_IonripperTrail (cent->lerp_origin, ent.origin);
				V_AddLight (ent.origin, 100, 1, 0.5, 0.5);
			}
			// RAFAEL
			else if (effects & EF_BLUEHYPERBLASTER)
			{
				V_AddLight (ent.origin, 200, 0, 0, 1);
			}
			// RAFAEL
			else if (effects & EF_PLASMA)
			{
				if (effects & EF_ANIM_ALLFAST)
				{
					CL_BlasterTrail (cent->lerp_origin, ent.origin);
				}
				V_AddLight (ent.origin, 130, 1, 0.5, 0.5);
			}
		}

		VectorCopy (ent.origin, cent->lerp_origin);
	}
}
#endif


/*
==============
CL_AddViewWeapon
==============
*/

struct model_s* cl_mod_view_muzzleflash;
extern muzzleflash_t cl_muzzleflashes[FX_WEAPON_MUZZLEFLASHES];

void CL_AddViewWeapon (player_state_t *ps, player_state_t *ops)
{
	centity_t	viewmodel; 
	int			i;


#if 1
	centity_t	flash;
	vec3_t  v_fwd, v_right, v_up;
	muzzleflash_t* mz;
#endif

	// allow the gun to be completely removed
	if (!cl_drawviewmodel->value)
		return;

	//
	// set up viewmodel
	//
	memset (&viewmodel, 0, sizeof(viewmodel));

	if (gun_model)
		viewmodel.model = gun_model;	// development tool
	else
		viewmodel.model = cl.model_draw[ps->viewmodel_index];

	if (!viewmodel.model)
		return;

	// set up position
	for (i = 0; i < 3; i++)
	{
		viewmodel.origin[i] = cl.refdef.vieworg[i] + ops->viewmodel_offset[i]
			+ cl.lerpfrac * (ps->viewmodel_offset[i] - ops->viewmodel_offset[i]);
		viewmodel.angles[i] = cl.refdef.viewangles[i] + LerpAngle (ops->viewmodel_angles[i],
			ps->viewmodel_angles[i], cl.lerpfrac);
	}

	if (gun_frame)
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

	VectorCopy(viewmodel.origin, viewmodel.oldorigin);
	viewmodel.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_VIEW_MODEL;
	V_AddEntity (&viewmodel);

#if 1
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
	flash.model = cl_mod_view_muzzleflash;
	V_AddEntity(&flash);
#endif
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
	ccentity_t	*ent;
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
			cl.refdef.vieworg[i] = cl.predicted_origin[i] + ops->viewoffset[i] 
				+ cl.lerpfrac * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * cl.prediction_error[i];
		}

		// smooth out stair climbing
		delta = cls.realtime - cl.predicted_step_time;
		if (delta < SV_FRAMETIME_MSEC)
			cl.refdef.vieworg[2] -= cl.predicted_step * (SV_FRAMETIME_MSEC - delta) * 0.01;
	}
	else
	{	// just use interpolated values
#if PROTOCOL_FLOAT_COORDS == 1
		for (i = 0; i < 3; i++)
			cl.refdef.vieworg[i] = ops->pmove.origin[i] + ops->viewoffset[i] + lerp * (ps->pmove.origin[i] + ps->viewoffset[i] - (ops->pmove.origin[i] + ops->viewoffset[i]));
#else
		for (i = 0; i < 3; i++)
			cl.refdef.vieworg[i] = ops->pmove.origin[i] * 0.125 + ops->viewoffset[i] + lerp * (ps->pmove.origin[i] * 0.125 + ps->viewoffset[i] - (ops->pmove.origin[i] * 0.125 + ops->viewoffset[i]));
#endif
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if ( cl.frame.playerstate.pmove.pm_type < PM_DEAD )
	{	// use predicted values
		for (i=0 ; i<3 ; i++)
			cl.refdef.viewangles[i] = cl.predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cl.refdef.viewangles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	for (i=0 ; i<3 ; i++)
		cl.refdef.viewangles[i] += LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);

	AngleVectors (cl.refdef.viewangles, cl.v_forward, cl.v_right, cl.v_up);

	// interpolate field of view
	cl.refdef.fov_x = ops->fov + lerp * (ps->fov - ops->fov);

	// don't interpolate blend color
	for (i=0 ; i<4 ; i++)
		cl.refdef.blend[i] = ps->blend[i];

	// add the weapon
	CL_AddViewWeapon (ps, ops);
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CL_AddEntities (void)
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
	}

	if (cl_timedemo->value)
		cl.lerpfrac = 1.0;


	// calculate view first so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CL_CalcViewValues ();

	CL_AddPacketEntities (&cl.frame);
	CL_AddTEnts ();
	CL_AddParticles ();
	CL_AddDLights ();
	CL_AddLightStyles ();
}

/*
===============
CL_GetEntitySoundOrigin

Called to get the sound spatialization origin
===============
*/
void CL_GetEntitySoundOrigin (int ent, vec3_t org)
{
	ccentity_t	*old;

	if (ent < 0 || ent >= MAX_GENTITIES)
		Com_Error (ERR_DROP, "CL_GetEntitySoundOrigin: bad ent");
	old = &cl_entities[ent];
	VectorCopy (old->lerp_origin, org);

	// FIXME: bmodel issues...
}
