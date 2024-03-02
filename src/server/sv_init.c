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


/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline (void)
{
	gentity_t			*svent;
	int				entnum;	

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

	if (Cvar_VariableValue ("deathmatch"))
		return;

	Com_sprintf (name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.name);
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

void Nav_Init();
int Nav_AddPathNode(float x, float y, float z);
qboolean Nav_AddPathNodeLink(int nodeId, int linkTo);
int Nav_GetNodeLinkCount(int node);
int Nav_GetMaxLinksCount();

static vec3_t node_origin; // for qsorting distance
typedef struct // for qsorting pathnodes from closest to farthest
{
	int	index;
	vec3_t origin;
} tempnode_t;

float dist3d(vec3_t p1, vec3_t p2)
{
	vec3_t vtemp;
	VectorSubtract(p1, p2, vtemp);
	return VectorLength(vtemp);
}

static char* vtos(vec3_t p)
{
	return va("[%i %i %i]", (int)p[0], (int)p[1], (int)p[2]);
}

int CompareNodeDistances(const void* a, const void* b) 
{
	tempnode_t* nodeA = (tempnode_t*)a;
	tempnode_t* nodeB = (tempnode_t*)b;

	float distanceA = dist3d(nodeA->origin, node_origin);
	float distanceB = dist3d(nodeB->origin, node_origin);

	if (distanceA < distanceB) 
		return -1;
	else if (distanceA > distanceB) 
		return 1;
	else return 0; // distances are equal so the order doesn't matter
}


static int nodeLinkDist = 148;
static void SV_LinkPathNode(gentity_t* self)
{
	int			i, num;
	gentity_t	*other;
	vec3_t		mins, maxs;
	trace_t		trace;
	tempnode_t nodes[32];

	num = 0;
	VectorCopy(self->v.origin, node_origin);

	if ((int)self->v.nodeIndex == -1 || self->v.solid != SOLID_PATHNODE)
	{
		Com_Printf("WARNING: %s at %s is not a path node.\n", Scr_GetString(self->v.classname), vtos(self->v.origin));
		return;
	}


	if (Nav_GetNodeLinkCount(self->v.nodeIndex) >= Nav_GetMaxLinksCount())
	{
		return; // already linked
	}

//	static vec3_t expand = { nodeLinkDist, nodeLinkDist, nodeLinkDist };
//	VectorSubtract(self->v.absmin, expand, mins);
//	VectorAdd(self->v.absmax, expand, maxs);
//	num = SV_AreaEdicts(mins, maxs, nodes, MAX_GENTITIES, AREA_PATHNODES);
	 
	//
	// find nodes within reasonable distance to self
	//
	for (i = svs.max_clients; i < sv.max_edicts; i++)
	{
		other = ENT_FOR_NUM(i);
		if (!other->inuse)
			continue;
		if (other == self)
			continue;
		if (other->v.solid != SOLID_PATHNODE)
			continue;
		if (dist3d(self->v.origin, other->v.origin) > nodeLinkDist)
			continue;
		
		VectorCopy(other->v.origin, nodes[num].origin);
		nodes[num].index = other->v.nodeIndex;
		num++;

		if (num == 32)
		{
			Com_Printf("WARNING: Path node %i at %s is crowded (32 neighbors or more).\n", (int)self->v.nodeIndex, vtos(self->v.origin));
			break;
		}
	}

	if (!num)
	{
		Com_Printf("WARNING: Path node at %s is too far from other nodes (node %i will not be linked).\n", vtos(self->v.origin), (int)self->v.nodeIndex);
		return;
	}


	//
	// sort the nodes from closest to farthest
	//
	qsort(nodes, sizeof(nodes) / sizeof(nodes[0]), sizeof(tempnode_t), CompareNodeDistances);


	//
	// do a trace check to see if we can link with nodes
	//
	VectorSet(mins, -4, -4, -4);
	VectorSet(maxs, 4, 4, 4);

	vec3_t start, end;
	VectorCopy(self->v.origin, start);
	start[2] += 16;

	for (i = 0; i < num; i++)
	{
		VectorCopy(nodes[i].origin, end);
		end[2] += 16;
		trace = SV_Trace(start, mins, maxs, end, self, MASK_MONSTERSOLID);

		if (trace.fraction != 1.0)
			continue;
		
		if (!Nav_AddPathNodeLink(self->v.nodeIndex, nodes[i].index))
		{
			Com_Printf("WARNING: Path node %i at %s reached link count limit (nodes are too crowded)\n", (int)self->v.nodeIndex, vtos(self->v.origin));
			break;
		}
	}
}

/*
================
SV_DropPathNodeToFloor

Drop pathnode to floor, removes nodes that are stuck in solid.
================
*/
static void SV_DropPathNodeToFloor(gentity_t *self)
{
	trace_t trace;
	vec3_t dest;

	VectorCopy(self->v.origin, dest);
	dest[2] -= 128;

	trace = SV_Trace(self->v.origin, self->v.mins, self->v.maxs, dest, self, MASK_MONSTERSOLID);

	if (trace.startsolid)
	{
		Com_Printf("WARNING: Path node %i at %s in solid, removed.\n", (int)self->v.nodeIndex, vtos(self->v.origin));
		//SV_FreeEntity(self);
		//return;
	}

	if (trace.fraction == 1.0)
	{
		Com_Printf("WARNING: Path node %i at %s too far from floor, removed.\n", (int)self->v.nodeIndex, vtos(self->v.origin));
		//SV_FreeEntity(self);
		//return;
	}

	VectorCopy(trace.endpos, self->v.origin);
	SV_LinkEdict(self);

	self->v.nodeIndex = Nav_AddPathNode(self->v.origin[0], self->v.origin[1], self->v.origin[2]);
}
/*
================
SV_LinkAllPathNodes

Link all pathnodes
================
*/
static void SV_LinkAllPathNodes()
{
	int i;
	gentity_t* ent;

	// first off, drop all nodes to ground
	for (i = svs.max_clients; i < sv.max_edicts; i++)
	{
		ent = ENT_FOR_NUM(i);
		if (!ent->inuse || ent->v.solid != SOLID_PATHNODE)
			continue;
		SV_DropPathNodeToFloor(ent);
	}
	
	// and then link them
	for (i = svs.max_clients; i < sv.max_edicts; i++)
	{
		ent = ENT_FOR_NUM(i);
		if (!ent->inuse || ent->v.solid != SOLID_PATHNODE)
			continue;
		SV_LinkPathNode(ent);
	}
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

	// set world model
	sv.edicts[0].v.model = Scr_SetString(sv.configstrings[CS_MODELS + 1]);
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

================
*/
void SV_SpawnServer (char *server, char *spawnpoint, server_state_t serverstate, qboolean attractloop, qboolean loadgame)
{
	int			i;
	unsigned	checksum_map, checksum_cgprogs;
	gentity_t	*ent;

	if (attractloop)
		Cvar_Set ("paused", "0");

	Com_Printf ("------- Server Initialization -------\n");
	Com_DPrintf (DP_ALL,"SpawnServer: %s\n",server);

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
	strcpy (sv.configstrings[CS_NAME], server);

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

	strcpy (sv.name, server);
	strcpy (sv.configstrings[CS_NAME], server);

	sv.models[0].type = MOD_BAD;
	strcpy(sv.models[0].name, "none");

	//sv.models[1] is world
	sv.models[MODELINDEX_WORLD].type = MOD_BRUSH;
	sv.num_models = 2;
	if (serverstate != ss_game)
	{
		sv.models[MODELINDEX_WORLD].bmodel = CM_LoadMap ("", false, &checksum_map);	// no real map
	}
	else
	{
		//set configstring
		Com_sprintf (sv.configstrings[CS_MODELS+1],sizeof(sv.configstrings[CS_MODELS+1]), "maps/%s.bsp", server);

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
	Cvar_FullSet("mapname", sv.name, CVAR_SERVERINFO | CVAR_NOSET);
	Cvar_FullSet("gamename", "pragma", CVAR_SERVERINFO | CVAR_LATCH);
	Cvar_FullSet("gamedate", __DATE__, CVAR_SERVERINFO | CVAR_NOSET);

	// load and spawn all other entities
	SV_SpawnEntities( sv.name, CM_EntityString(), spawnpoint );

	// call the main function in server progs
	SV_ScriptMain();


	Com_sprintf(sv.configstrings[CS_CHEATS_ENABLED], sizeof(sv.configstrings[CS_CHEATS_ENABLED]), "%i", (int)sv_cheats->value);

#if 1
	// run two frames to allow everything to settle
	SV_RunWorldFrame();
	SV_LinkAllPathNodes(); // give it a frame so the entities can spawn and drop to floor
	SV_RunWorldFrame();
#else
	
	for(i = 0; i < 2; i++)
		SV_RunWorldFrame();
#endif


	// all precaches are complete
	sv.state = serverstate;
	Com_SetServerState (sv.state);
	
	// create a baseline for more efficient communications
	SV_CreateBaseline();

	// check for a savegame
//	SV_CheckForSavegame();

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

	if (Cvar_VariableValue ("coop") && Cvar_VariableValue ("deathmatch"))
	{
		Com_Printf("Deathmatch and Coop both set, disabling Coop\n");
		Cvar_FullSet ("coop", "0",  CVAR_SERVERINFO | CVAR_LATCH);
	}

	// dedicated servers are can't be single player and are usually DM
	// so unless they explicity set coop, force it to deathmatch
	if (dedicated->value)
	{
		if (!Cvar_VariableValue ("coop"))
			Cvar_FullSet ("deathmatch", "1",  CVAR_SERVERINFO | CVAR_LATCH);
	}

	// init clients
	if (Cvar_VariableValue ("deathmatch"))
	{
		if (sv_maxclients->value <= 1)
			Cvar_FullSet ("sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);
		else if (sv_maxclients->value > MAX_CLIENTS)
			Cvar_FullSet ("sv_maxclients", va("%i", MAX_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH);
	}
	else if (Cvar_VariableValue ("coop"))
	{
		if (sv_maxclients->value <= 1 || sv_maxclients->value > 4)
			Cvar_FullSet ("sv_maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	}
	else	// non-deathmatch, non-coop is one player
	{
		Cvar_FullSet ("sv_maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	}

	sv_cheats = Cvar_Get("sv_cheats", "0", CVAR_SERVERINFO);
	sv_maxclients = Cvar_Get("sv_maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	sv_password = Cvar_Get("sv_password", "", 0);
	sv_maxentities = Cvar_Get("sv_maxentities", va("%i", MAX_GENTITIES), CVAR_LATCH);

	sv_maxvelocity = Cvar_Get("sv_maxevelocity", "1500", 0);
	sv_gravity = Cvar_Get("sv_gravity", "800", 0);

	// make sure critical cvars are in their proper range
	if (sv_maxclients->value <= 0)
		Cvar_FullSet("sv_maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	else if (sv_maxclients->value > MAX_CLIENTS)
		Cvar_FullSet("sv_maxclients", va("%i", MAX_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH);

	if (sv_maxentities->value < 256)
		Cvar_FullSet("sv_maxentities", va("%i", 256), CVAR_LATCH);
	else if (sv_maxentities->value > MAX_GENTITIES)
		Cvar_FullSet("sv_maxentities", va("%i", MAX_GENTITIES), CVAR_LATCH);

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

	dedicated = Cvar_Get("dedicated", "0", CVAR_NOSET);

	sv_cheats = Cvar_Get("sv_cheats", "0", CVAR_SERVERINFO);
	sv_maxclients = Cvar_Get("sv_maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	sv_password = Cvar_Get("sv_password", "", 0);
	sv_maxentities = Cvar_Get("sv_maxentities", va("%i", MAX_GENTITIES), CVAR_LATCH);

	sv_maxvelocity = Cvar_Get("sv_maxevelocity", "1500", 0);
	sv_gravity = Cvar_Get("sv_gravity", "800", 0);

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
