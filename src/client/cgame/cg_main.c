/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"

static qboolean cg_allow_drawcalls;

/*
===============
CL_ShutdownClientGame

Called when engine is closing, changing map or it is changing to a different game directory.
===============
*/
void CL_ShutdownClientGame()
{
	cg_allow_drawcalls = false;
	Scr_FreeScriptVM(VM_CLGAME);
}

/*
===============
CL_InitClientGame

Called when engine is starting, connection to server is established, or it is changing to a different game (mod) directory.

Calls progs function CG_Main so scripts can begin initialization
===============
*/
void CL_InitClientGame()
{
	Com_Printf("CL_InitClientGame\n");
	Scr_BindVM(VM_NONE);
	Scr_CreateScriptVM(VM_CLGAME, 512, (sizeof(clentity_t) - sizeof(cl_entvars_t)), offsetof(clentity_t, v));
	Scr_BindVM(VM_CLGAME); // so we can get proper entity size and ptrs

	cl.max_entities = 512;
	cl.entity_size = Scr_GetEntitySize();
	cl.entities = ((clentity_t*)((byte*)Scr_GetEntityPtr()));
	cl.qcvm_active = true;
	cl.script_globals = Scr_GetGlobals();

	cl.script_globals->localplayernum = cl.playernum;
	Scr_Execute(VM_CLGAME, cl.script_globals->CG_Main, __FUNCTION__);
}

/*
===============
CG_IsActive

Returns true if CG qcvm is properly loaded
===============
*/
static qboolean CG_IsActive()
{
	return cl.qcvm_active;
}


/*
===============
CG_ServerCommand

Handles incomming 'SVC_CGCMD [command (byte)] [...]' commands from server
===============
*/
void CG_ServerCommand()
{
	float cmd;

	if (CG_IsActive() == false)
		return;

	cmd = (float)MSG_ReadByte(&net_message);

	Scr_BindVM(VM_CLGAME);

	Scr_AddFloat(0, cmd);
	Scr_Execute(VM_CLGAME, cl.script_globals->CG_ServerCommand, __FUNCTION__);

	Scr_BindVM(VM_NONE);
}


/*
===============
CG_Frame

This calls progs function CG_Frame at the beginning of each client frame
===============
*/
void CG_Frame(float frametime, int time, float realtime)
{
	if (CG_IsActive() == false)
		return;

	Scr_BindVM(VM_CLGAME);

	cl.script_globals->frametime = frametime;
	cl.script_globals->time = time;
	cl.script_globals->realtime = realtime;
	cl.script_globals->localplayernum = cl.playernum;

	Scr_Execute(VM_CLGAME, cl.script_globals->CG_Frame, __FUNCTION__);
}


/*
===============
CG_DrawGUI

This calls progs function CG_DrawGUI and allows rendering via builtins
===============
*/
void CG_DrawGUI()
{
	if (CG_IsActive() == false || cls.state != ca_active)
		return;

	cg_allow_drawcalls = true;
	Scr_Execute(VM_CLGAME, cl.script_globals->CG_DrawGUI, __FUNCTION__);
	cg_allow_drawcalls = false;
}

/*
====================
CG_CanDrawCall

Returns true if progs are allowed to draw
====================
*/
qboolean CG_CanDrawCall()
{
	if(!cl.refresh_prepped)
		return false; // ref not ready
//	if (cls.state != ca_active)
//		return false; // not actively in game
	if (!cg_allow_drawcalls)
		return false; // no drawing outside of draw phase
	return true;
}


/*
====================
CG_HullForEntity
====================
*/
static int CG_HullForEntity(entity_state_t* ent)
{
	cmodel_t	*model;
	vec3_t		bmins, bmaxs;

	// decide which clipping hull to use
	if (ent->solid == PACKED_BSP)
	{
		// explicit hulls in the BSP model
		model = cl.model_clip[ent->modelindex];
		if (!model)
		{
			Com_Error(ERR_DROP, "CG_HullForEntity: non BSP model for entity %i\n", ent->number);
			return -1;
		}
		return model->headnode;
	}

	// extract bbox size
#if PROTOCOL_FLOAT_COORDS == 1
	MSG_UnpackSolid32(ent->solid, bmins, bmaxs);
#else
	MSG_UnpackSolid16(ent->solid, bmins, bmaxs);
#endif

	// create a temp hull from bounding box sizes
	return CM_HeadnodeForBox(bmins, bmaxs);
}

/*
====================
CG_ClipMoveToEntities
====================
*/
static void CG_ClipMoveToEntities(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum, trace_t* tr)
{
	trace_t		trace;
	int			headnode, i, num;
	float		*angles;
	entity_state_t* ent;

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->solid == 0)
			continue; // not solid

		if (ent->number == ignoreEntNum)
			continue; // ignored entity

		if (tr->allsolid)
			return;

		// might intersect
		headnode = CG_HullForEntity(ent);
		if (ent->solid == PACKED_BSP)
			angles = ent->angles;
		else
			angles = vec3_origin;	// boxes don't rotate

		trace = CM_TransformedBoxTrace(start, end, mins, maxs, headnode, contentsMask, ent->origin, angles);
	
		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.clent = ent;
			tr->entitynum = ent->number;
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
====================
CG_Trace
====================
*/
trace_t CG_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum)
{
	trace_t	trace;

	if (!mins)
		mins = vec3_origin;
	if (!maxs)
		maxs = vec3_origin;

	// check against world
	trace = CM_BoxTrace(start, end, mins, maxs, 0, contentsMask);
	if (trace.fraction == 0.0f)
	{
		// blocked by world
		trace.clent = cl.entities; // fixme this is not so good but better than null
		trace.entitynum = 0;
		return trace;
	}

	// check all other solid models
	CG_ClipMoveToEntities(start, mins, maxs, end, contentsMask, ignoreEntNum, &trace);

	return trace;
}

/*
====================
CG_PointContents
====================
*/
int	CG_PointContents(vec3_t point)
{
	int			i;
	entity_state_t* ent;
	int			num;
	cmodel_t* cmodel;
	int			contents;

	contents = CM_PointContents(point, 0);

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (ent->solid != PACKED_BSP) // special value for bmodel
			continue;

		cmodel = cl.model_clip[(int)ent->modelindex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents(point, cmodel->headnode, ent->origin, ent->angles);
	}

	return contents;
}

/*
====================
CG_FindOrRegisterSound

Returns sfx or NULL if sound file cannot be found or cannot be loaded
====================
*/
struct sfx_t* CG_FindOrRegisterSound(char *filename)
{
	struct sfx_t* sfx;
	sfx = S_RegisterSound(filename);
	if (!sfx)
	{
		Com_Printf("CG_FindOrRegisterSound: cannot load '%s'\n", filename);
		return NULL;
	}
	return sfx;
}