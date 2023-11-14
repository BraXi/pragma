#include "../client/client.h"

static qboolean cg_allow_drawcalls;

/*
===============
CL_ShutdownClientGame

Called when engine is closing or it is changing to a different game directory.
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

Called when engine is starting or it is changing to a different game (mod) directory.
===============
*/
void CL_InitClientGame()
{
	Com_Printf("CL_InitClientGame\n");
//	return;
	Scr_BindVM(VM_NONE);

	Scr_CreateScriptVM(VM_CLGAME, 512, (sizeof(clentity_t) - sizeof(cl_entvars_t)), offsetof(clentity_t, v));
	Scr_BindVM(VM_CLGAME); // so we can get proper entity size and ptrs

	cl.max_entities = 512;
	cl.entity_size = Scr_GetEntitySize();
	cl.entities = ((clentity_t*)((byte*)Scr_GetEntityPtr()));
	cl.qcvm_active = true;
	cl.script_globals = Scr_GetGlobals();

	Scr_Execute(cl.script_globals->CG_Main, __FUNCTION__);
}

static qboolean CG_IsActive()
{
	return cl.qcvm_active;
}

void CG_Main()
{
	if (CG_IsActive() == false)
		return;

	Scr_Execute(cl.script_globals->CG_Main, __FUNCTION__);
}


void CG_Frame(float frametime, int time, float realtime)
{
	if (CG_IsActive() == false)
		return;

	Scr_BindVM(VM_CLGAME);

	cl.script_globals->frametime = frametime;
	cl.script_globals->time = time;
	cl.script_globals->realtime = realtime;
	cl.script_globals->localplayernum = cl.playernum;

	Scr_Execute(cl.script_globals->CG_Frame, __FUNCTION__);
}


void CG_DrawGUI()
{
	if (CG_IsActive() == false || cls.state != ca_active)
		return;

	cg_allow_drawcalls = true;
	Scr_Execute(cl.script_globals->CG_DrawGUI, __FUNCTION__);
	cg_allow_drawcalls = false;
}

/*
====================
CG_CanDrawCall
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
CG_ClipMoveToEntities
====================
*/
static void CG_ClipMoveToEntities(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum, trace_t* tr)
{
	trace_t		trace;
	int			headnode;
	float* angles;
	entity_state_t* ent;
	int			num;
	cmodel_t* cmodel;
	vec3_t		bmins, bmaxs;
	int i;
	for (i = 0; i < cl.frame.num_entities; i++)
	{
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == ignoreEntNum ) //cl.playernum + 1)
			continue; // ignore local player

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
			headnode = CM_HeadnodeForBox(bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
			return;

		trace = CM_TransformedBoxTrace(start, end, mins, maxs, headnode, contentsMask, ent->origin, angles);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = (struct gentity_s*)ent;
			trace.entitynum = ent->number;
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

trace_t CG_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum)
{
	trace_t	t;

	// check against world
	t = CM_BoxTrace(start, end, mins, maxs, 0, contentsMask);
	if (t.fraction < 1.0)
	{
		t.ent = NULL;
		t.entitynum = -1;
	}

	// check all other solid models
	CG_ClipMoveToEntities(start, mins, maxs, end, contentsMask, ignoreEntNum, &t);

	return t;
}

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