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

#include "client.h"


/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError (void)
{
	int		frame;
	int		delta[3];
	int		i;
	int		len;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (CMD_BACKUP-1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear (cl.prediction_error);
	}
	else
	{
		if (cl_showmiss->value && (delta[0] || delta[1] || delta[2]) )
			Com_Printf ("prediction miss on serverframe %i: %i\n", cl.frame.serverframe, delta[0] + delta[1] + delta[2]);

		VectorCopy (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame]);

		// save for error itnerpolation
#if PROTOCOL_FLOAT_COORDS == 1
		for (i = 0; i < 3; i++)
			cl.prediction_error[i] = delta[i];
#else
		for (i = 0; i < 3; i++)
			cl.prediction_error[i] = delta[i] * 0.125;
#endif
	}
}





/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities ( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr )
{
	trace_t		trace;
	int			headnode;
	float		*angles;
	entity_state_t	*ent;
	int			num;
	cmodel_t		*cmodel;
	vec3_t		bmins, bmaxs;
	int i;
	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playernum+1)
			continue;

		if (ent->solid == PACKED_BSP)
		{	// special value for bmodel
			cmodel = cl.model_clip[(int)ent->modelindex];
			if (!cmodel)
				continue;
			headnode = cmodel->headnode;
			angles = ent->angles;
		}
		else
		{	// encoded bbox
#if PROTOCOL_FLOAT_COORDS == 1
			MSG_UnpackSolid32(ent->solid, bmins, bmaxs);
#else
			MSG_UnpackSolid16(ent->solid, bmins, bmaxs);
#endif

			headnode = CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
			return;

		trace = CM_TransformedBoxTrace (start, end, mins, maxs, headnode,  MASK_PLAYERSOLID, ent->origin, angles);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = (struct gentity_s *)ent;
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
================
CL_PMTrace
================
*/
trace_t CL_PMTrace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	t;

	// check against world
	t = CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
	{
		t.clent = cl.entities;
		t.entitynum = 0;
	}

	// check all other solid models
	CL_ClipMoveToEntities (start, mins, maxs, end, &t);

	t.entitynum = 0;
	return t;
}

int CL_PMpointcontents(vec3_t point)
{
	int			i;
	entity_state_t	*ent;
	int			num;
	cmodel_t		*cmodel;
	int			contents;

	contents = CM_PointContents(point, 0);

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		if (ent->solid != PACKED_BSP) // special value for bmodel
			continue;

		cmodel = cl.model_clip[(int)ent->modelindex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents (point, cmodel->headnode, ent->origin, ent->angles);
	}

	return contents;
}


/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/

void CL_PredictMovement (void)
{
	int			ack, current;
	int			frame;
	int			oldframe;
	usercmd_t	*cmd;
	pmove_t		pm;
	int			i;
	int			step;
	int			oldz;

#ifdef USE_PMOVE_IN_PROGS
	vec3_t inmove, inangles;
#endif

	if (cls.state != ca_active)
		return;

	if (cl_paused->value)
		return;

	if (!cl_predict->value || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// just set angles
		for (i=0 ; i<3 ; i++)
		{
			cl.predicted_angles[i] = cl.viewangles[i] + SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
		}
		return;
	}

	ack = cls.netchan.incoming_acknowledged;
	current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP)
	{
		if (cl_showmiss->value)
			Com_Printf ("exceeded CMD_BACKUP\n");
		return;	
	}


	//
	// copy current state to pmove
	//
	memset (&pm, 0, sizeof(pm));
	pm.s = cl.frame.playerstate.pmove;

#ifdef USE_PMOVE_IN_PROGS
	cl_globalvars_t* cgGlobals = NULL;

	if (cl.qcvm_active && cl.entities)
	{
		cgGlobals = cl.script_globals;	// reki -- 27-12-23 Can cl.script_globals be NULL? hopefully not. 
										// BraXi - yup, can be when !cl.qcvm_active

		//
		// copy pmove state TO cgame
		//
		cgGlobals->pm_state_pm_type = (int)pm.s.pm_type;
		cgGlobals->pm_state_gravity = (int)pm.s.gravity;
		cgGlobals->pm_state_pm_flags = (int)pm.s.pm_flags;
		cgGlobals->pm_state_pm_time = (int)pm.s.pm_time;

		for (i = 0; i < 3; i++)
		{
			cgGlobals->pm_state_origin[i] = pm.s.origin[i];
			cgGlobals->pm_state_velocity[i] = pm.s.velocity[i];
			cgGlobals->pm_state_delta_angles[i] = (float)pm.s.delta_angles[i];

			cgGlobals->pm_state_mins[i] = pm.s.mins[i];
			cgGlobals->pm_state_maxs[i] = pm.s.maxs[i];
		}

		// make sure qc knows our number for trace function
		cgGlobals->localplayernum = cl.playernum;
	}
#else
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMpointcontents;
#endif

//	SCR_DebugGraph (current - ack - 1, 0);

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP-1);
		cmd = &cl.cmds[frame];

#ifdef USE_PMOVE_IN_PROGS
		inmove[0] = (float)cmd->forwardmove;
		inmove[1] = (float)cmd->sidemove;
		inmove[2] = (float)cmd->upmove;
		for (i = 0; i < 3; i++)
			inangles[i] = (float)cmd->angles[i];

		if(cl.qcvm_active && cl.entities && cgGlobals != NULL)
		{
			//
			// call cgame's pmove
			//	
			Scr_BindVM(VM_CLGAME); // always bing qcvm here, or some weird things may happen when on loopback servers
			Scr_AddVector(0, inmove);
			Scr_AddVector(1, inangles);
			Scr_AddFloat(2, (float)cmd->buttons);
			Scr_AddFloat(3, (float)cmd->msec);
			Scr_Execute(VM_CLGAME, cl.script_globals->CG_PlayerMove, __FUNCTION__);

			//
			// read pmove state FROM cgame
			//
			pm.s.pm_type = cgGlobals->pm_state_pm_type;
			pm.s.gravity = cgGlobals->pm_state_gravity;
			pm.s.pm_flags = cgGlobals->pm_state_pm_flags;
			pm.s.pm_time = cgGlobals->pm_state_pm_time;
			pm.viewheight = cl.script_globals->cam_viewoffset[2];

			for (i = 0; i < 3; i++)
			{
				pm.s.origin[i] = cgGlobals->pm_state_origin[i];
				pm.s.velocity[i] = cgGlobals->pm_state_velocity[i];
				pm.s.delta_angles[i] = cgGlobals->pm_state_delta_angles[i];

				pm.s.mins[i] = cgGlobals->pm_state_mins[i];
				pm.s.maxs[i] = cgGlobals->pm_state_maxs[i];

				pm.mins[i] = cgGlobals->pm_state_mins[i];
				pm.maxs[i] = cgGlobals->pm_state_maxs[i];

				pm.viewangles[i] = cl.script_globals->cam_viewangles[i];
			}
		}
#else
		pm.cmd = *cmd;
		Pmove (&pm);
#endif
		// save for debug checking
		VectorCopy (pm.s.origin, cl.predicted_origins[frame]);
	}

	oldframe = (ack-2) & (CMD_BACKUP-1);
	oldz = cl.predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND) )
	{
#if PROTOCOL_FLOAT_COORDS == 1
		cl.predicted_step = step;
#else
		cl.predicted_step = step * 0.125;
#endif
		
		cl.predicted_step_time = cls.realtime - cls.frametime * 500;
	}

	// copy results out for rendering
#if PROTOCOL_FLOAT_COORDS == 1
	VectorCopy(pm.s.origin, cl.predicted_origin);
#else
	cl.predicted_origin[0] = pm.s.origin[0] * 0.125;
	cl.predicted_origin[1] = pm.s.origin[1] * 0.125;
	cl.predicted_origin[2] = pm.s.origin[2] * 0.125;
#endif
	//set view angles
	VectorCopy (pm.viewangles, cl.predicted_angles);
}
