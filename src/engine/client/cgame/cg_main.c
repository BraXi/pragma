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
CG_AssetIndex
Find or load an asset (model, image, sound) and return its index.
=================
*/
int CG_AssetIndex(cgAssetType_t type, const char* name, qboolean allowLoad)
{
	unsigned int index, count, max;
	cgAssetEntry_t* list, *asset;

	if (!name || !name[0])
		return 0;

	if (strlen(name) >= MAX_QPATH-1) // cause null termination
	{
		//Com_Error(ERR_DROP, __FUNCTION__": Name '%s' is too long.\n", name);
		Com_Printf(__FUNCTION__": Name '%s' is too long.\n", name);
		return 0;
	}

	count = max = 0;
	list = NULL;

	switch (type)
	{
	case ASSET_MODEL:
		max = MAX_MODELS;
		count = cgMedia.numModels;
		list = cgMedia.model_list;
		break;
	case ASSET_IMAGE:
		max = MAX_IMAGES;
		count = cgMedia.numImages;
		list = cgMedia.image_list;
		break;
	case ASSET_SOUND:
		max = MAX_SOUNDS;
		count = cgMedia.numSounds;
		list = cgMedia.sound_list;
		break;
	default:
		Com_Error(ERR_DROP, __FUNCTION__": Wrong cgAssetType %i\n", type);
		//return 0;
		break;
	}

	// search in existing entries
	for (index = 1; index < count; index++)
	{
		asset = &list[index];
		if (!strcmp(asset->name, name))
		{
			return index; // found cached
		}
	}

	if (index >= max)
	{
		//Com_Error(ERR_DROP, __FUNCTION__": Hit limit of max assets of type %i\n", type);
		Com_Printf(__FUNCTION__": Hit limit of max assets of type %i\n", type);
		return 0;
	}

	if (!allowLoad)
	{
		return 0; // since we are not allowed to load it, just return the missing/default asset
	}

	// Create new entry, name it and load asset
	asset = &list[index];
	strncpy(asset->name, name, MAX_QPATH);

	switch (type)
	{
	case ASSET_MODEL:
		asset->ptr = re.RegisterModel(name);
		cgMedia.numModels ++;
		break;
	case ASSET_IMAGE:
		asset->ptr = re.RegisterPic(name);
		cgMedia.numImages ++;
		break;
	case ASSET_SOUND:
		asset->ptr = S_RegisterSound(name);
		cgMedia.numSounds ++;
		break;
	}

	return index;
}

/*
=================
CG_LoadAsset
Same as CG_AssetIndex, but returns raw pointer to data.
=================
*/
void *CG_LoadAsset(cgAssetType_t type, const char* name)
{
	int index;

	index = CG_AssetIndex(type, name, true);

	switch (type)
	{
	case ASSET_MODEL:
		return cgMedia.model_list[index].ptr;
		break;
	case ASSET_IMAGE:
		return cgMedia.image_list[index].ptr;
		break;
	case ASSET_SOUND:
		return cgMedia.sound_list[index].ptr;
		break;
	default:
		Com_Error(ERR_DROP, __FUNCTION__": Wrong cgAssetType %i\n", type);
		//return NULL;
		break;
	}
	return NULL;
}

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
	cgAssetEntry_t* asset;

	for (i = 0; i < 3; i++)
	{
		cgMedia.sfx_ricochet[i] = CG_LoadAsset(ASSET_SOUND, va("impacts/ricochet_%i.wav", i));
	}

	for (i = 0; i < 3; i++)
	{
		cgMedia.sfx_footsteps[i] = CG_LoadAsset(ASSET_SOUND, va("footsteps/generic_%i.wav", i));
	}

	cgMedia.sfx_explosion[0] = CG_LoadAsset(ASSET_SOUND, "explosions/med_1.wav");
	cgMedia.sfx_explosion[1] = CG_LoadAsset(ASSET_SOUND, "explosions/med_1.wav");
	cgMedia.sfx_explosion[2] = CG_LoadAsset(ASSET_SOUND, "explosions/med_1.wav");

	// reload all client game sounds
	for (i = 0; i < cgMedia.numSounds; i++)
	{
		asset = &cgMedia.sound_list[i];
		if (asset->name[0] == 0)
			continue;
		asset->ptr = S_RegisterSound(asset->name);
	}
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
	cgMedia.mod_v_muzzleflash = CG_LoadAsset(ASSET_MODEL, "models/fx/muzzleflash_view.md3");
	cgMedia.mod_w_muzzleflash = CG_LoadAsset(ASSET_MODEL, "models/fx/muzzleflash_world.md3");

	cgMedia.mod_v_flashlight = CG_LoadAsset(ASSET_MODEL, "models/fx/flashlight_view.md3");
	cgMedia.mod_w_flashlight = CG_LoadAsset(ASSET_MODEL, "models/fx/flashlight_world.md3");

//	cgMedia.impact_small = re.RegisterModel("models/fx/impact_small.md3"); // unused 
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
	Com_Printf("------ Shutdown Client Game ------\n");
	CG_ClearState();
	cg_allow_drawcalls = false;
	cg.qcvm_active = false;

	Z_FreeTags(TAG_CLIENT_GAME);
	Scr_FreeScriptVM(VM_CLGAME); // also removes client side entities

	// free the old map but not when server is running
//	if(Cvar_VariableValue("cm_flushmap") && !Com_ServerState())
//		CM_ClearMap();

	Com_Printf("------ Client Game Closed ------\n");
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

	//g->self = g->other = ENT_TO_VM(cg.entities);  //test
}

/*
================
CG_CreateConstStrings
Create constant strings which are frequently used in C code for progs
================
*/
static void CG_CreateConstStrings()
{
	cg.cstr.free = Scr_NewString("free"); // when ent is freed
	cg.cstr.no_class = Scr_NewString("no_class"); // when ent is spawned
	cg.cstr.player = Scr_NewString("player");
	cg.cstr.disconnected = Scr_NewString("disconnected"); // when player disconnects
	cg.cstr.worldspawn = Scr_NewString("worldspawn");
}

/*
===============
CG_InitClientGame
Client game is loaded when client connects to server, and unloaded at disconnect.
It is also re initialized every level change.
===============
*/
void CG_InitClientGame()
{
	Com_Printf("------- Client Game Init -------\n");

	// zero cgMedia, they're all unloaded at end of registration anyway
	memset(&cgMedia, 0, sizeof(cgMedia));

	Scr_CreateScriptVM(VM_CLGAME, MAX_CLIENT_ENTITIES, (sizeof(clentity_t) - sizeof(cl_entvars_t)), offsetof(clentity_t, v));
	Scr_BindVM(VM_CLGAME); // so we can get proper entity size and ptrs

	cg.qcvm_active = true;
	cg.maxEntities = MAX_CLIENT_ENTITIES;
	cg.entity_size = Scr_GetEntitySize();
	cg.entities = ((clentity_t*)((byte*)Scr_GetEntityPtr()));
	cg.script_globals = Scr_GetGlobals();

	CG_CreateConstStrings();

	Com_Printf("------- Client Game Init Complete -------\n");
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
	Com_DPrintf(DP_CGAME, "CG_BeginGame()\n");

	// call main() to setup things before entities are spawned
	CG_UpdateScriptGlobals();
	Scr_Execute(VM_CLGAME, cg.script_globals->CG_Main, __FUNCTION__);

	// spawn local entities
	//CG_SpawnEntities(cl.configstrings[CS_MODELS + 1], CM_EntityString());
}

/*
===============
CG_ParseCommandFromServer
Handles incomming 'SVC_CGCMD [command (byte)] [...]' commands from server
===============
*/
void CG_ParseCommandFromServer()
{
	int cmd;
	if (CG_IsActive() == false)
		return;

	CG_UpdateScriptGlobals();
	cmd = MSG_ReadByte(&net_message);

	Scr_BindVM(VM_CLGAME);
	Scr_AddFloat(0, (float)cmd);
	Scr_Execute(VM_CLGAME, cg.script_globals->CG_ParseCommandFromServer, __FUNCTION__);
	Scr_BindVM(VM_NONE);
}

/*
==============
CG_EntityEvent
An entity has event, so call it.
==============
*/
void CG_EntityEvent(clentity_t* ent)
{
	if (CG_IsActive() == false)
		return;

	CG_UpdateScriptGlobals();
	cg.script_globals->self = ENT_TO_VM(ent);
	Scr_Execute(VM_CLGAME, cg.script_globals->EntityEvent, __FUNCTION__);
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
===============
CG_DrawScene
This calls progs function CG_DrawScene and allows rendering via builtins
===============
*/
void CG_DrawScene()
{
	if (CG_IsActive() == false || cls.state != CS_ACTIVE)
		return;

	cg_allow_drawcalls = true;
	Scr_Execute(VM_CLGAME, cg.script_globals->CG_DrawScene, __FUNCTION__);
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
struct sfx_t* CG_FindOrRegisterSound(const char *filename)
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
===============
CG_CalcViewValues
Allows client game progs modify camera parms. This is called before entities are added to scene.
===============
*/
void CG_CalcViewValues()
{
	cl_globalvars_t* g;
	qboolean fromscript;

	if (CG_IsActive() == false || cls.state != CS_ACTIVE)
		return;

	g = cg.script_globals;

	// copy current camera parms to vm
	VectorCopy(cl.refdef.view.origin, g->cam_origin);
	VectorCopy(cl.refdef.view.angles, g->cam_angles);
	g->cam_fov = cl.refdef.view.fov_x;

	VectorCopy(cl.v_forward, g->cam_forward);
	VectorCopy(cl.v_right, g->cam_right);
	VectorCopy(cl.v_up, g->cam_up);

	Scr_BindVM(VM_CLGAME);
	Scr_Execute(VM_CLGAME, cg.script_globals->CalcViewValues, __FUNCTION__);
	fromscript = (Scr_GetReturnFloat() >= 1.0f);

	// did we modify camera parms? if so pass it to C code back
	if (fromscript)
	{
		VectorCopy(g->cam_origin, cl.refdef.view.origin);
		VectorCopy(g->cam_angles, cl.refdef.view.angles);
		cl.refdef.view.fov_x = g->cam_fov;

		AngleVectors(cl.refdef.view.angles, cl.v_forward, cl.v_right, cl.v_up);
		VectorCopy(cl.v_forward, g->cam_forward);
		VectorCopy(cl.v_right, g->cam_right);
		VectorCopy(cl.v_up, g->cam_up);
	}
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

	CG_DrawScene();
}