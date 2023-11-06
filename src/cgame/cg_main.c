#include "../client/client.h"



/*
===============
CL_ShutdownClientGame

Called when engine is closing or it is changing to a different game directory.
===============
*/
void CL_ShutdownClientGame()
{
//	Scr_FreeScriptVM(VM_CLGAME);
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


	Scr_Execute(cl.script_globals->CG_Frame, __FUNCTION__);
}


void CG_DrawGUI()
{
	if (CG_IsActive() == false)
		return;

	if (cls.state != ca_active)
		return;

	Scr_Execute(cl.script_globals->CG_DrawGUI, __FUNCTION__);
}