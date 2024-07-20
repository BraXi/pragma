/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"

cg_t cg;
cgMedia_t cgMedia;

void CG_SpawnEntities(char* mapname, char* entities);

static qboolean cg_allow_drawcalls;

/*
=================
CG_RegisterSounds

This is the only right place to load audio.
Called before entering a new level, and when sound system is restarting
=================
*/
void CG_RegisterSounds()
{
	int i;

	for( i = 0; i < 3; i++)
		cgMedia.sfx_ricochet[i] = S_RegisterSound(va("impacts/ricochet_%i.wav", i));

	for (i = 0; i < 3; i++)
		cgMedia.sfx_footsteps[i] = S_RegisterSound(va("footsteps/generic_%i.wav", i));


	cgMedia.sfx_explosion[0] = S_RegisterSound("explosions/med_1.wav");
	cgMedia.sfx_explosion[1] = cgMedia.sfx_explosion[0];
	cgMedia.sfx_explosion[2] = cgMedia.sfx_explosion[0];
}

/*
=================
CG_RegisterModels

This is the only right place to load visuals.
Called before entering a new level, and when renderer is restarting
=================
*/
void CG_RegisterMedia()
{
	cgMedia.mod_v_muzzleflash = re.RegisterModel("models/fx/v_muzzleflash.md3");
	cgMedia.mod_w_muzzleflash = re.RegisterModel("models/fx/w_muzzleflash.md3");

	cgMedia.mod_v_flashlight = re.RegisterModel("models/fx/v_flashlight.md3");
	cgMedia.mod_w_flashlight = re.RegisterModel("models/fx/w_flashlight.md3");

	cgMedia.impact_small = re.RegisterModel("models/fx/impact_small.md3");
}

/*
=================
CG_ClearState

Called before entering a new level, and when sound system is restarting
=================
*/
void CG_ClearState()
{
	CG_ClearParticles();
	CG_ClearDynamicLights();
	CG_ClearLightStyles();

	CL_ClearTEnts();
}

/*
===============
CL_ShutdownClientGame

Called when:
	engine is closing
	map is changing
	changing to a different game directory
	connecting to server
	.. and when disconnecting from server.
===============
*/
void CL_ShutdownClientGame()
{
	Com_Printf("------ ShutdownClientGame ------\n");
	CG_ClearState();
	cg_allow_drawcalls = false;
	cg.qcvm_active = false;
	Scr_FreeScriptVM(VM_CLGAME);

	// free the map but not when server is running
	if(Cvar_VariableValue("cm_flushmap") && !Com_ServerState())
		CM_FreeMap();
}

/*
===============
CG_UpdateScriptGlobals

Bind cgame vm and update globals
===============
*/
static void CG_UpdateScriptGlobals()
{
	cl_globalvars_t	*g;

	if (!cg.qcvm_active || cg.script_globals == NULL)
		return;

	Scr_BindVM(VM_CLGAME);

	g = cg.script_globals;

	g->frametime = cls.frametime;
	g->time = cl.time;
	g->realtime = cls.realtime;

	g->vid_width = viddef.width;
	g->vid_height = viddef.height;

	g->localplayernum = cl.playernum;

	//g->self = g->other = ENT_TO_VM(cg.localEntities);  //test
}


/*
===============
CG_InitClientGame

Called when engine is starting, connection to server is established, or it is changing to a different game (mod) directory.
Create client game instance of progs vm and allocate entities for it

===============
*/
void CG_InitClientGame()
{
	Com_Printf("------- Client Game Init -------\n");
	
	//
	// create cgame instance of progs vm and allocate entities for it
	//
	Scr_CreateScriptVM(VM_CLGAME, MAX_CLIENT_ENTITIES, (sizeof(clentity_t) - sizeof(cl_entvars_t)), offsetof(clentity_t, v));
	Scr_BindVM(VM_CLGAME); // so we can get proper entity size and ptrs

	cg.qcvm_active = true;
	cg.maxLocalEntities = MAX_CLIENT_ENTITIES;
	cg.localEntitySize = Scr_GetEntitySize();
	cg.localEntities = ((clentity_t*)((byte*)Scr_GetEntityPtr()));
	cg.script_globals = Scr_GetGlobals();

	Com_Printf("Client game VM spawned.\n");
}

/*
===============
CG_IsActive

Returns true if CG qcvm is active
===============
*/
static qboolean CG_IsActive()
{
	return (cg.qcvm_active == true);
}


/*
===============
CG_BeginGame

Client has established with server, has all the configstrings and baselines and loaded map. 
It is the last moment to finish initialization before client is let into the game.
===============
*/
void CG_BeginGame()
{
	// call main() to setup things before entities are spawned
	CG_UpdateScriptGlobals();
	Scr_Execute(VM_CLGAME, cg.script_globals->CG_Main, __FUNCTION__);

	// spawn local entities
	//CG_SpawnEntities(cl.configstrings[CS_MODELS + 1], CM_EntityString());

//	Com_Printf("CG_BeginGame\n");
}

/*
===============
CG_ParseCommandFromServer

Handles incomming 'SVC_CGCMD [command (byte)] [...]' commands from server
===============
*/
void CG_ParseCommandFromServer()
{
	float cmd;

	if (CG_IsActive() == false)
		return;

	cmd = (float)MSG_ReadByte(&net_message);

	Scr_BindVM(VM_CLGAME);

	Scr_AddFloat(0, cmd);
	Scr_Execute(VM_CLGAME, cg.script_globals->CG_ParseCommandFromServer, __FUNCTION__);

	Scr_BindVM(VM_NONE);
}


/*
===============
CG_Frame

This calls progs function CG_Frame at the beginning of each client frame
===============
*/
void CG_Frame()
{
	// advance local effects for next frame
	CG_RunDynamicLights();
	CG_RunLightStyles();

	if (CG_IsActive() == false)
		return;

//	CG_UpdateScriptGlobals();
//	CG_RunLocalEntities();

	CG_UpdateScriptGlobals();
	Scr_Execute(VM_CLGAME, cg.script_globals->CG_Frame, __FUNCTION__);
}


/*
===============
CG_DrawGUI

This calls progs function CG_DrawGUI and allows rendering via builtins
===============
*/
void CG_DrawGUI()
{
	if (CG_IsActive() == false || cls.state != CS_ACTIVE)
		return;

	cg_allow_drawcalls = true;
	Scr_Execute(VM_CLGAME, cg.script_globals->CG_DrawGUI, __FUNCTION__);
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
//	if (cls.state != CS_ACTIVE)
//		return false; // not actively in game
	if (!cg_allow_drawcalls)
		return false; // no drawing outside of draw phase
	return true;
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

/*
=================
CG_AddEntities

Emits all entities, particles, and lights to the refresh
=================
*/
void CG_AddEntities()
{
//	CG_AddViewWeapon(ps, ops);
	CG_AddTempEntities();
//	CG_AddLocalEntities();
	CG_SimulateAndAddParticles();
	CG_AddDynamicLights();
	CG_AddLightStyles();
}