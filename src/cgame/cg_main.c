#include "../client/client.h"



/*
===============
CL_ShutdownClientGameProgs

Called when engine is closing or it is changing to a different game directory.
===============
*/
void CL_ShutdownClientGameProgs(void)
{
}

/*
===============
CL_InitClientGameProgs

Called when engine is starting or it is changing to a different game (mod) directory.
===============
*/
void CL_InitClientGameProgs(void)
{
	Com_Printf("CL_InitClientGame\n");
}



void CG_Draw2D()
{
	if (!cl.qcvm_active)
		return;
	if (cls.state != ca_active)
		return;

	Scr_Execute(cl.script_globals->Draw2D, __FUNCTION__);
}