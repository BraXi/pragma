/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "server.h"

#ifdef __linux__
#include <stddef.h>
#endif

server_static_t	svs;				// persistant server info
server_t		sv;					// local server

void SV_ScriptMain();
void Nav_Init();
void SV_LinkAllPathNodes();

/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages to the clients.
Only the fields that differ from the baseline will be transmitted.
================
*/
static void SV_CreateBaseline()
{
	gentity_t	*svent;
	int			entnum;	

	for (entnum = 1; entnum < sv.max_edicts; entnum++)
	{
		svent = EDICT_NUM(entnum);
		if (!svent->inuse)
			continue;
		if (!svent->s.modelindex && !svent->s.loopingSound && !svent->s.effects)
			continue;
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		VectorCopy (svent->s.origin, svent->s.old_origin);
		sv.baselines[entnum] = svent->s;
	}
}


/*
=================
SV_CheckForSavegame
=================
*/
void SV_CheckForSavegame (void)
{
	char		name[MAX_OSPATH];
	FILE		*f;
	int			i;

	if (sv_noreload->value)
		return;

	if (Cvar_VariableValue ("multiplayer"))
		return;

	Com_sprintf (name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.mapname);
	f = fopen (name, "rb");
	if (!f)
		return;		// no savegame

	fclose (f);

	SV_ClearWorld ();

	// get configstrings and areaportals
	SV_ReadLevelFile ();

	if (!sv.loadgame)
	{	// coming back to a level after being in a different
		// level, so run it for ten seconds

		// rlava2 was sending too many lightstyles, and overflowing the
		// reliable data. temporarily changing the server state to loading
		// prevents these from being passed down.
		server_state_t		previousState;		// PGM

		previousState = sv.state;				// PGM
		sv.state = ss_loading;					// PGM
		for (i = 0; i < 100; i++)
			SV_RunWorldFrame();

		sv.state = previousState;				// PGM
	}
}


/*
================
SV_SetWorldEntityFields

We actually do it twice, once when QCVM initializes and later to enforce it if any dumbass decided its fun to mess up world fields
================
*/
static void SV_SetWorldEntityFields()
{
	gentity_t* world = &sv.edicts[ENTITYNUM_WORLD];
	world->v.classname = Scr_SetString("worldspawn");
	world->v.model = Scr_SetString(sv.configstrings[CS_MODELS + 1]); // set model name
	world->v.modelindex = 1; // world model MUST always be index 1	
	world->v.movetype = MOVETYPE_PUSH;
	world->v.solid = SOLID_BSP;
}

/*
================
SV_InitGameProgs

Create QCVM and entities
================
*/
static void SV_InitGameProgs()
{
	Scr_CreateScriptVM(VM_SVGAME, sv_maxentities->value, (sizeof(gentity_t) - sizeof(sv_entvars_t)), offsetof(gentity_t, v));
	Scr_BindVM(VM_SVGAME); // so we can get proper entity size and ptrs

	// initialize all entities for this game
	sv.max_edicts = sv_maxentities->value;
	sv.entity_size = Scr_GetEntitySize();
	sv.edicts = ((gentity_t*)((byte*)Scr_GetEntityPtr()));
	sv.qcvm_active = true;
	sv.script_globals = Scr_GetGlobals();

	SV_SetWorldEntityFields();
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

================
*/
void SV_SpawnServer (char *mapname, char *spawnpoint, server_state_t serverstate, qboolean attractloop, qboolean loadgame)
{
	int			i;
	unsigned	checksum_map, checksum_cgprogs;
	gentity_t	*ent;

	if (attractloop)
		Cvar_Set ("paused", "0");

	Com_Printf ("------- Server Initialization -------\n");
	Com_DPrintf (DP_ALL,"SpawnServer: %s\n",mapname);

	if (sv.demofile)
		fclose (sv.demofile);

	svs.spawncount++;		// any partially connected client will be restarted
	sv.state = ss_dead;
	Com_SetServerState (sv.state);

	//
	// clean memory and wipe the entire per-level structure
	//
	Z_FreeTags(TAG_SERVER_GAME);
	Z_FreeTags(TAG_SERVER_MODELDATA);
	memset (&sv, 0, sizeof(sv));

	svs.realtime = 0;
	sv.loadgame = loadgame;
	sv.attractloop = attractloop;
	sv.time = 1000;

	// save bsp name for levels that don't set message key properly
	strcpy (sv.configstrings[CS_NAME], mapname);

	SZ_Init (&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));

	if (!svs.clients)
	{
		svs.clients = Z_Malloc(sizeof(client_t) * sv_maxclients->value);
		svs.num_client_entities = sv_maxclients->value * UPDATE_BACKUP * 64;
		svs.client_entities = Z_Malloc(sizeof(entity_state_t) * svs.num_client_entities);
	}
	// leave slots at start for clients only
	for (i=0 ; i<sv_maxclients->value ; i++)
	{
		// needs to reconnect
		if (svs.clients[i].state > cs_connected)
			svs.clients[i].state = cs_connected;
		svs.clients[i].lastframe = -1;
	}

	strcpy (sv.mapname, mapname);
	strcpy (sv.configstrings[CS_NAME], mapname);

	//
	// initialize server models, sv.models[1] is world model, followed by all inline models and md3s
	//
	sv.models[0].type = MOD_BAD;
	strcpy(sv.models[0].name, "none");

	sv.models[MODELINDEX_WORLD].type = MOD_BRUSH;
	sv.num_models = 2; // because modelindex 0 is no model

	if (serverstate != ss_game)
	{
		// no real map -- cinematic server
		sv.models[MODELINDEX_WORLD].bmodel = CM_LoadMap ("", false, &checksum_map);	
	}
	else
	{
		Com_sprintf (sv.configstrings[CS_MODELS+1],sizeof(sv.configstrings[CS_MODELS+1]), "maps/%s.bsp", mapname);
		strcpy(sv.models[MODELINDEX_WORLD].name, sv.configstrings[CS_MODELS + 1]);

		sv.models[MODELINDEX_WORLD].bmodel = CM_LoadMap (sv.models[1].name, false, &checksum_map);
	}

	//
	// we want to do a CRC checksums for currently loaded map and client-game progs
	// to validate that the client is using exactly _the same data_ as the server and simulation is identical
	// TODO: server checksums should be private and the server should ask client to send them nicely to compare
	// otherwise, a malicious client can just slap an "is matching!" response and connect with different files loaded
	//
	checksum_cgprogs = CRC_ChecksumFile("progs/cgame.dat", true);

	Com_sprintf(sv.configstrings[CS_CHECKSUM_MAP], sizeof(sv.configstrings[CS_CHECKSUM_MAP]), "%i", checksum_map);
	Com_sprintf(sv.configstrings[CS_CHECKSUM_CGPROGS], sizeof(sv.configstrings[CS_CHECKSUM_CGPROGS]), "%i", checksum_cgprogs);

	Com_Printf("client-game progs crc: %d\n", checksum_cgprogs);


	//
	// dev tools
	//
	SV_InitDevTools();

	//
	// create qcvm for svgame progs
	//
	SV_InitGameProgs();

	for (i = 0; i < sv_maxclients->value; i++)
	{
		ent = EDICT_NUM(i + 1);
		ent->s.number = i + 1;
		svs.clients[i].edict = ent;
		memset(&svs.clients[i].lastcmd, 0, sizeof(svs.clients[i].lastcmd));
	}

	//
	// clear physics interaction links
	//
	Nav_Init();
	SV_ClearWorld ();
	
	// brushmodels
	for (i = 1; i < CM_NumInlineModels() ; i++)
	{
		Com_sprintf(sv.configstrings[CS_MODELS+1+i], sizeof(sv.configstrings[CS_MODELS+1+i]), "*%i", i);

		sv.models[sv.num_models].type = MOD_BRUSH;
		strcpy(sv.models[sv.num_models].name, sv.configstrings[CS_MODELS + 1 + i]);
		sv.models[sv.num_models].bmodel = CM_InlineModel(sv.configstrings[CS_MODELS+1+i]);

		sv.num_models++;
	}

	//
	// spawn the rest of the entities on the map
	//	

	// precache and static commands can be issued during map initialization
	sv.state = ss_loading;
	Com_SetServerState (sv.state);

	// set serverinfo variables before executing any scripts so they can query them
	Cvar_FullSet("mapname", sv.mapname, CVAR_SERVERINFO | CVAR_NOSET, NULL);
	Cvar_FullSet("gamename", "pragma", CVAR_SERVERINFO | CVAR_LATCH, NULL);
	Cvar_FullSet("gamedate", __DATE__, CVAR_SERVERINFO | CVAR_NOSET, NULL);

	// load and spawn all other entities
	SV_SpawnEntities( sv.mapname, CM_EntityString(), spawnpoint );

	// call the main function in server progs
	SV_ScriptMain();

	Com_sprintf(sv.configstrings[CS_CHEATS_ENABLED], sizeof(sv.configstrings[CS_CHEATS_ENABLED]), "%i", (int)sv_cheats->value);

	// give it a frame so the entities can spawn and drop to floor
	SV_RunWorldFrame();

	// link pathnodes
	SV_LinkAllPathNodes(); 

	// one more frame to settle everything
	SV_RunWorldFrame();

	// all precaches are complete
	SV_SetWorldEntityFields();
	sv.state = serverstate;
	Com_SetServerState (sv.state);
	
	// create a baseline for more efficient communications
	SV_CreateBaseline();

	// check for a savegame
	//SV_CheckForSavegame();

	if (dedicated->value)
	{
		Com_Printf( "[%s] finished spawning server on map %s (%i max clients).\n", GetTimeStamp(false), sv.configstrings[CS_MODELS + 1], (int)sv_maxclients->value);
	}
	else
		Com_Printf("spawned server on map %s.\n", sv.configstrings[CS_MODELS + 1]);

	Com_Printf ("-------------------------------------\n");
}

/*
==============
SV_InitGame

A brand new game has been started or a save game is loaded.
==============
*/
void SV_InitGame (void)
{
	char	masterserver[32];

	Com_Printf("Starting brand new game...\n");

	if (svs.initialized)
	{
		// cause any connected clients to reconnect
		SV_Shutdown ("Server has restarted.", true);
	}
	else
	{
#ifndef DEDICATED_ONLY
		// make sure the client is down
		CL_Drop ();
		SCR_BeginLoadingPlaque ();
#endif
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars ();

	svs.initialized = true;

	if (Cvar_VariableValue ("coop") && Cvar_VariableValue ("multiplayer"))
	{
		Com_Printf("Multiplayer and Cooperative both set, disabling Multiplayer\n");

		Cvar_FullSet("multiplayer", "0", CVAR_SERVERINFO | CVAR_LATCH, NULL);
		Cvar_FullSet("coop", "1",  CVAR_SERVERINFO | CVAR_LATCH, NULL);
	}

	// dedicated servers can't be single player and are usually COOP
	// so unless they explicity set multiplayer, force it to coop
	if (dedicated->value)
	{
		if (!Cvar_VariableValue ("multiplayer"))
			Cvar_FullSet ("coop", "1",  CVAR_SERVERINFO | CVAR_LATCH, NULL);
	}

	// init clients
	if (Cvar_VariableValue ("multiplayer"))
	{
		if (sv_maxclients->value <= 1)
			Cvar_FullSet ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH, NULL);
		else if (sv_maxclients->value > MAX_CLIENTS)
			Cvar_FullSet ("sv_maxclients", va("%i", MAX_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH, NULL);
	}
	else if (Cvar_VariableValue ("coop"))
	{
		if (sv_maxclients->value <= 1 || sv_maxclients->value > 8)
			Cvar_FullSet ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH, NULL);
	}
	else	// non-multiplayer, non-coop is one player
	{
		Cvar_FullSet ("sv_maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH, NULL);
	}

	sv_cheats = Cvar_Get("sv_cheats", "0", CVAR_SERVERINFO, NULL);
	sv_maxclients = Cvar_Get("sv_maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH, NULL);
	sv_password = Cvar_Get("sv_password", "", 0, NULL);
	sv_maxentities = Cvar_Get("sv_maxentities", va("%i", MAX_GENTITIES), CVAR_LATCH, NULL);

	sv_maxvelocity = Cvar_Get("sv_maxevelocity", "1500", 0, NULL);
	sv_gravity = Cvar_Get("sv_gravity", "800", 0, NULL);

	// make sure critical cvars are in their proper range
	if (sv_maxclients->value <= 0)
		Cvar_FullSet("sv_maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH, NULL);
	else if (sv_maxclients->value > MAX_CLIENTS)
		Cvar_FullSet("sv_maxclients", va("%i", MAX_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH, NULL);

	if (sv_maxentities->value < 256)
		Cvar_FullSet("sv_maxentities", va("%i", 256), CVAR_LATCH, NULL);
	else if (sv_maxentities->value > MAX_GENTITIES)
		Cvar_FullSet("sv_maxentities", va("%i", MAX_GENTITIES), CVAR_LATCH, NULL);

	svs.spawncount = rand();
	svs.clients = Z_Malloc (sizeof(client_t)*sv_maxclients->value);
	svs.num_client_entities = sv_maxclients->value*UPDATE_BACKUP*64;
	svs.client_entities = Z_Malloc (sizeof(entity_state_t)*svs.num_client_entities);

	// init network stuff
	NET_Config ( (sv_maxclients->value > 1) );

	// heartbeats will always be sent to the id master
	svs.last_heartbeat = -99999;		// send immediately
	Com_sprintf(masterserver, sizeof(masterserver), "192.246.40.37:%i", PORT_MASTER);
	NET_StringToAdr (masterserver, &master_adr[0]);

	dedicated = Cvar_Get("dedicated", "0", CVAR_NOSET, NULL);

	sv_cheats = Cvar_Get("sv_cheats", "0", CVAR_SERVERINFO, NULL);
	sv_maxclients = Cvar_Get("sv_maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH, NULL);
	sv_password = Cvar_Get("sv_password", "", 0, NULL);
	sv_maxentities = Cvar_Get("sv_maxentities", va("%i", MAX_GENTITIES), CVAR_LATCH, NULL);

	sv_maxvelocity = Cvar_Get("sv_maxevelocity", "1500", 0, NULL);
	sv_gravity = Cvar_Get("sv_gravity", "800", 0, NULL);

	// initialize all clients for this game
	svs.max_clients = sv_maxclients->value;
	svs.gclients = Z_Malloc(svs.max_clients * sizeof(gclient_t));

	sv.num_edicts = svs.max_clients + 1; // first 'free' entity is after world and client reserved slots

#if 0
	for (i=0 ; i<sv_maxclients->value ; i++)
	{
		ent = EDICT_NUM(i+1);
		ent->s.number = i+1;
		svs.clients[i].edict = ent;
		memset (&svs.clients[i].lastcmd, 0, sizeof(svs.clients[i].lastcmd));
	}
#endif
}


/*
======================
SV_Map

  the full syntax is:

  map [*]<map>$<startspot>+<nextserver>

command from the console or progs.
Map can also be a .cin, .tga, or .demo file
Nextserver is used to allow a cinematic to play, then proceed to
another level:

	map tram.cin+jail_e3
======================
*/
void SV_Map (qboolean attractloop, char *levelstring, qboolean loadgame, qboolean persGlobals, qboolean persClients)
{
	char	level[MAX_QPATH];
	char	*ch;
	int		i, l;
	char	spawnpoint[MAX_QPATH];

	sv.loadgame = loadgame;
	sv.attractloop = attractloop;

	if (sv.state == ss_dead && !sv.loadgame)
		SV_InitGame ();	// the game is just starting

	if (!persGlobals)
		memset(&svs.saved, 0, sizeof(svs.saved));

	if (!persClients && svs.gclients)
	{
		for (i = 0; i < sv_maxclients->value; i++)
			memset(&svs.gclients[i].pers.saved, 0, sizeof(svs.gclients[0].pers.saved));
	}

	strcpy (level, levelstring);

	// if there is a + in the map, set nextserver to the remainder
	ch = strstr(level, "+");
	if (ch)
	{
		*ch = 0;
			Cvar_Set ("nextserver", va("gamemap \"%s\"", ch+1));
	}
	else
		Cvar_Set ("nextserver", "");

// leaving here just for later reference so I don't forget about it
//	if (Cvar_VariableValue ("coop") && !Q_stricmp(level, "victory.pcx"))
	//	Cvar_Set ("nextserver", "gamemap \"*base1\"");

	// if there is a $, use the remainder as a spawnpoint
	ch = strstr(level, "$");
	if (ch)
	{
		*ch = 0;
		strcpy (spawnpoint, ch+1);
	}
	else
		spawnpoint[0] = 0;

	// skip the end-of-unit flag if necessary
	if (level[0] == '*')
		strcpy (level, level+1);


	if (dedicated->value)
	{
		Com_Printf("[%s] *** server is changing map to '%s' ***\n", GetTimeStamp(true), level);
	}

	l = strlen(level);
	if (l > 4 && !strcmp (level+l-4, ".cin") )
	{
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnpoint, ss_cinematic, attractloop, loadgame);
	}
	else if (l > 4 && !strcmp (level+l-4, ".demo") )
	{
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnpoint, ss_demo, attractloop, loadgame);
	}
	else if (l > 4 && !strcmp (level+l-4, ".tga") )
	{
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		SV_SpawnServer (level, spawnpoint, ss_pic, attractloop, loadgame);
	}
	else
	{
#ifndef DEDICATED_ONLY
		SCR_BeginLoadingPlaque ();			// for local system
#endif
		SV_BroadcastCommand ("changing\n");
		SV_SendClientMessages ();
		SV_SpawnServer (level, spawnpoint, ss_game, attractloop, loadgame);
		Cbuf_CopyToDefer ();
	}

	SV_BroadcastCommand ("reconnect\n");
}
