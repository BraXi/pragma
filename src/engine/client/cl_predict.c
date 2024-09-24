/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
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
		for (i = 0; i < 3; i++)
			cl.prediction_error[i] = delta[i];
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

		if (!ent->packedSolid)
			continue;

		if (ent->number == cl.playernum+1)
			continue;

		if (ent->packedSolid == PACKED_BSP)
		{	
			// special value for bmodel
			cmodel = CL_GetClipModel((int)ent->modelindex);
			if (!cmodel)
				continue;
			headnode = cmodel->headnode;
			angles = ent->angles;
		}
		else
		{	
			// encoded bbox
			MSG_UnpackSolid32(ent->packedSolid, bmins, bmaxs);

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

	vec3_t inmove, inangles;

	if (cls.state != CS_ACTIVE)
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

	cl_globalvars_t* cgGlobals = NULL;

	if (cg.qcvm_active && cg.localEntities)
	{
		cgGlobals = cg.script_globals;	// reki -- 27-12-23 Can cg.script_globals be NULL? hopefully not. 
										// BraXi - yup, can be when !cg.qcvm_active

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

//	SCR_DebugGraph (current - ack - 1, 0);

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP-1);
		cmd = &cl.cmds[frame];

		inmove[0] = (float)cmd->forwardmove;
		inmove[1] = (float)cmd->sidemove;
		inmove[2] = (float)cmd->upmove;
		for (i = 0; i < 3; i++)
			inangles[i] = (float)cmd->angles[i];

		if(cg.qcvm_active && cg.localEntities && cgGlobals != NULL)
		{
			//
			// call cgame's pmove
			//	
			Scr_BindVM(VM_CLGAME); // always bing qcvm here, or some weird things may happen when on loopback servers
			Scr_AddVector(0, inmove);
			Scr_AddVector(1, inangles);
			Scr_AddFloat(2, (float)cmd->buttons);
			Scr_AddFloat(3, (float)cmd->msec);
			Scr_Execute(VM_CLGAME, cg.script_globals->CG_PlayerMove, __FUNCTION__);

			//
			// read pmove state FROM cgame
			//
			pm.s.pm_type = cgGlobals->pm_state_pm_type;
			pm.s.gravity = cgGlobals->pm_state_gravity;
			pm.s.pm_flags = cgGlobals->pm_state_pm_flags;
			pm.s.pm_time = cgGlobals->pm_state_pm_time;
			pm.viewheight = cg.script_globals->cam_viewoffset[2];

			for (i = 0; i < 3; i++)
			{
				pm.s.origin[i] = cgGlobals->pm_state_origin[i];
				pm.s.velocity[i] = cgGlobals->pm_state_velocity[i];
				pm.s.delta_angles[i] = cgGlobals->pm_state_delta_angles[i];

				pm.s.mins[i] = cgGlobals->pm_state_mins[i];
				pm.s.maxs[i] = cgGlobals->pm_state_maxs[i];

				pm.mins[i] = cgGlobals->pm_state_mins[i];
				pm.maxs[i] = cgGlobals->pm_state_maxs[i];

				pm.viewangles[i] = cg.script_globals->cam_viewangles[i];
			}
		}
		// save for debug checking
		VectorCopy (pm.s.origin, cl.predicted_origins[frame]);
	}

	oldframe = (ack-2) & (CMD_BACKUP-1);
	oldz = cl.predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND) )
	{
		cl.predicted_step = step;
		
		cl.predicted_step_time = cls.realtime - cls.frametime * 500;
	}

	// copy results out for rendering
	VectorCopy(pm.s.origin, cl.predicted_origin);

	//set view angles
	VectorCopy (pm.viewangles, cl.predicted_angles);
}
