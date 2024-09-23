/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "server.h"

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/


void SV_ModelList_f(void)
{
	svmodel_t *mod;
	int i, usedsize = 0;

	static const char *mods[] = { "BAD", "BSP", "ALIAS", "SMDL" };

	if (sv.state == ss_dead)
	{
		Com_Printf("Server must be running for sv_modelindex.");
		return;
	}

#if 0
	Com_Printf("\nCS MODELS:\n");
	Com_Printf("==============\n");

	char* name;
	int start = CS_MODELS;
	for (int index = 1; index < start+MAX_MODELS && sv.configstrings[start + index][0]; index++)
		Com_Printf("%i: %s\n", index, sv.configstrings[start + index]);
#endif

	Com_Printf("\nModels referenced by server:\n");

	for (i = MODELINDEX_WORLD, mod = &sv.models[MODELINDEX_WORLD]; i < sv.numModels; i++, mod++)
	{
		if (!mod->name[0])
			continue;

		Com_Printf("%4i: %s (%s)\n", i, mod->name, mods[mod->type]);

		usedsize += mod->extradatasize;
	}


	Com_Printf("\n");
	Com_Printf("Server references %i external models and %i inline models (Using %iKB of memory)\n", sv.numModels-1, CM_NumInlineModels(), usedsize/1024);
}


/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
void SV_SetMaster_f (void)
{
	int		i, slot;

	// only dedicated servers send heartbeats
	if (!dedicated->value)
	{
		Com_Printf ("Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_Set ("public", "1");

	for (i = 1; i < MAX_MASTER_SERVERS; i++)
		memset(&master_adr[i], 0, sizeof(master_adr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i = 1; i < Cmd_Argc(); i++)
	{
		if (slot == MAX_MASTER_SERVERS)
			break;

		if (!NET_StringToAdr (Cmd_Argv(i), &master_adr[i]))
		{
			Com_Printf ("Bad address: %s\n", Cmd_Argv(i));
			continue;
		}
		if (master_adr[slot].port == 0)
			master_adr[slot].port = BigShort (PORT_MASTER);

		Com_Printf ("Master server at %s\n", NET_AdrToString (master_adr[slot]));
		Com_Printf ("Sending a ping.\n");

		Netchan_OutOfBandPrint (NS_SERVER, master_adr[slot], "ping");

		slot++;
	}

	svs.last_heartbeat = -9999999;
}



/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer (void)
{
	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	if (Cmd_Argc() < 2)
		return false;

	s = Cmd_Argv(1);

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		idnum = atoi(Cmd_Argv(1));
		if (idnum < 0 || idnum >= sv_maxclients->value)
		{
			Com_Printf ("Bad client slot: %i\n", idnum);
			return false;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if (!sv_client->state)
		{
			Com_Printf ("Client %i is not active\n", idnum);
			return false;
		}
		return true;
	}

	// check for a name match
	for (i=0,cl=svs.clients ; i<sv_maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (!strcmp(cl->name, s))
		{
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	Com_Printf ("Userid %s is not on the server\n", s);
	return false;
}


/*
===============================================================================

SAVEGAME FILES

===============================================================================
*/

/*
=====================
SV_WipeSavegame

Delete save/<XXX>/
=====================
*/
void SV_WipeSavegame (char *savename)
{
	char	name[MAX_OSPATH];
	char	*s;

	Com_DPrintf(DP_SV,"SV_WipeSaveGame(%s)\n", savename);

	Com_sprintf (name, sizeof(name), "%s/save/%s/server.ssv", FS_Gamedir (), savename);
	remove (name);
	Com_sprintf (name, sizeof(name), "%s/save/%s/game.ssv", FS_Gamedir (), savename);
	remove (name);

	Com_sprintf (name, sizeof(name), "%s/save/%s/*.sav", FS_Gamedir (), savename);
	s = Sys_FindFirst( name, 0, 0 );
	while (s)
	{
		remove (s);
		s = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose ();
	Com_sprintf (name, sizeof(name), "%s/save/%s/*.sv2", FS_Gamedir (), savename);
	s = Sys_FindFirst(name, 0, 0 );
	while (s)
	{
		remove (s);
		s = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose ();
}


/*
================
CopyFile
================
*/
void CopyFile (char *src, char *dst)
{
	FILE	*f1, *f2;
	int		l;
	byte	buffer[65536];

	Com_DPrintf(DP_SV, "CopyFile (%s, %s)\n", src, dst);

	f1 = fopen (src, "rb");
	if (!f1)
		return;
	f2 = fopen (dst, "wb");
	if (!f2)
	{
		fclose (f1);
		return;
	}

	while (1)
	{
		l = (int)fread (buffer, (size_t)1, sizeof(buffer), f1);
		if (!l)
			break;
		fwrite (buffer, (size_t)1, (size_t)l, f2);
	}

	fclose (f1);
	fclose (f2);
}


/*
================
SV_CopySaveGame
================
*/
void SV_CopySaveGame (char *src, char *dst)
{
	src = dst = NULL;
#if 0
	char	name[MAX_OSPATH], name2[MAX_OSPATH];
	int		l, len;
	char	*found;

	Com_DPrintf(DP_SV, "SV_CopySaveGame(%s, %s)\n", src, dst);

	SV_WipeSavegame (dst);

	// copy the savegame over
	Com_sprintf (name, sizeof(name), "%s/save/%s/server.ssv", FS_Gamedir(), src);
	Com_sprintf (name2, sizeof(name2), "%s/save/%s/server.ssv", FS_Gamedir(), dst);
	FS_CreatePath (name2);
	CopyFile (name, name2);

	Com_sprintf (name, sizeof(name), "%s/save/%s/game.ssv", FS_Gamedir(), src);
	Com_sprintf (name2, sizeof(name2), "%s/save/%s/game.ssv", FS_Gamedir(), dst);
	CopyFile (name, name2);

	Com_sprintf (name, sizeof(name), "%s/save/%s/", FS_Gamedir(), src);
	len = strlen(name);
	Com_sprintf (name, sizeof(name), "%s/save/%s/*.sav", FS_Gamedir(), src);
	found = Sys_FindFirst(name, 0, 0 );
	while (found)
	{
		strcpy (name+len, found+len);

		Com_sprintf (name2, sizeof(name2), "%s/save/%s/%s", FS_Gamedir(), dst, found+len);
		CopyFile (name, name2);

		// change sav to sv2
		l = strlen(name);
		strcpy (name+l-3, "sv2");
		l = strlen(name2);
		strcpy (name2+l-3, "sv2");
		CopyFile (name, name2);

		found = Sys_FindNext( 0, 0 );
	}
	Sys_FindClose ();
#endif
}


/*
==============
SV_WriteLevelFile

==============
*/
void SV_WriteLevelFile (void)
{
#if 0
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_DPrintf(DP_SV,"SV_WriteLevelFile()\n");

	Com_sprintf (name, sizeof(name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.mapname);
	f = fopen(name, "wb");
	if (!f)
	{
		Com_Printf ("Failed to open %s\n", name);
		return;
	}
	fwrite (sv.configstrings, sizeof(sv.configstrings), 1, f);
	CM_WritePortalState (f);
	fclose (f);

	Com_sprintf (name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.mapname);
	WriteLevel (name);
#endif
}

/*
==============
SV_ReadLevelFile

==============
*/
void SV_ReadLevelFile (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_DPrintf(DP_SV, "SV_ReadLevelFile()\n");

	Com_sprintf (name, sizeof(name), "%s/save/current/%s.sv2", FS_Gamedir(), sv.mapname);
	f = fopen(name, "rb");
	if (!f)
	{
		Com_Printf ("Failed to open %s\n", name);
		return;
	}
	FS_Read (sv.configstrings, sizeof(sv.configstrings), f);
	CM_ReadPortalState (f);
	fclose (f);

	Com_sprintf (name, sizeof(name), "%s/save/current/%s.sav", FS_Gamedir(), sv.mapname);
	ReadLevel (name);
}

/*
==============
SV_WriteServerFile

autosave is true when changing levels
==============
*/
void SV_WriteServerFile (qboolean autosave)
{
	autosave = false;
#if 0
	FILE	*f;
	cvar_t	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	time_t	aclock;
	struct tm	*newtime;

	Com_DPrintf(DP_SV, "SV_WriteServerFile(%s)\n", autosave ? "true" : "false");

	Com_sprintf (name, sizeof(name), "%s/save/current/server.ssv", FS_Gamedir());
	f = fopen (name, "wb");
	if (!f)
	{
		Com_Printf ("Couldn't write %s\n", name);
		return;
	}

	// write the comment field
	memset (comment, 0, sizeof(comment));

	if (!autosave)
	{
		time (&aclock);
		newtime = localtime (&aclock);
		Com_sprintf (comment,sizeof(comment), "%2i:%i%i %2i/%2i  ", newtime->tm_hour, newtime->tm_min/10, newtime->tm_min%10, newtime->tm_mon+1, newtime->tm_mday); // write current time
		strncat (comment, sv.configstrings[CS_NAME], sizeof(comment)-1-strlen(comment) ); // write bsp name or level name
	}
	else
	{	
		Com_sprintf (comment, sizeof(comment), "ENTERING %s", sv.configstrings[CS_NAME]);  // autosaved
	}

	fwrite (comment, 1, sizeof(comment), f);

	// write the mapcmd
	fwrite (svs.mapcmd, 1, sizeof(svs.mapcmd), f);

	// write all CVAR_LATCH cvars
	// these will be things like coop, skill, multiplayer, etc
	for (var = cvar_vars ; var ; var=var->next)
	{
		if (!(var->flags & CVAR_LATCH))
			continue;

		if (strlen(var->name) >= sizeof(name)-1 || strlen(var->string) >= sizeof(string)-1)
		{
			Com_Printf ("Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}

		memset (name, 0, sizeof(name));
		memset (string, 0, sizeof(string));
		strcpy (name, var->name);
		strcpy (string, var->string);
		fwrite (name, 1, sizeof(name), f);
		fwrite (string, 1, sizeof(string), f);
	}

	fclose (f);

	// write game state
	Com_sprintf (name, sizeof(name), "%s/save/current/game.ssv", FS_Gamedir());
	WriteGame (name, autosave);
#endif
}

/*
==============
SV_ReadServerFile

==============
*/
void SV_ReadServerFile (void)
{
	FILE	*f;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	char	mapcmd[MAX_TOKEN_CHARS];

	Com_DPrintf(DP_SV, "SV_ReadServerFile()\n");

	Com_sprintf (name, sizeof(name), "%s/save/current/server.ssv", FS_Gamedir());
	f = fopen (name, "rb");
	if (!f)
	{
		Com_Printf ("Couldn't read %s\n", name);
		return;
	}
	
	FS_Read (comment, sizeof(comment), f);	// read the comment field
	FS_Read (mapcmd, sizeof(mapcmd), f);	// read the mapcmd

	// read all CVAR_LATCH cvars, these will be things like coop, skill, multiplayer, etc
	while (1)
	{
		if (!fread (name, 1, sizeof(name), f))
			break;

		FS_Read (string, sizeof(string), f);
		Com_DPrintf(DP_SV, "SV_ReadServerFile: Set %s = %s\n", name, string);
		Cvar_ForceSet (name, string);
	}

	fclose (f);

	// start a new game fresh with new cvars
	SV_InitGame();

	strcpy (svs.mapcmd, mapcmd);

	// read game state
	Com_sprintf (name, sizeof(name), "%s/save/current/game.ssv", FS_Gamedir());
	ReadGame (name);
}


//=========================================================




/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f (void)
{
	SV_Map (true, Cmd_Argv(1), false, false, false);
}

/*
==================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
==================
*/
void SV_GameMap_f (void)
{
	char		*map;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: gamemap <map>\n");
		return;
	}

//	Com_DPrintf(DP_SV, "SV_GameMap(%s)\n", Cmd_Argv(1));

//	FS_CreatePath (va("%s/save/current/", FS_Gamedir()));

	// check for clearing the current savegame
	map = Cmd_Argv(1);
	if (map[0] == '*')
	{
		// wipe all the *.sav files
//		SV_WipeSavegame ("current");
	}
	else
	{
#if 0	
		int		i;
		client_t* cl;
		qboolean* savedInuse = NULL;
		// save the map just exited
		if (sv.state == ss_game)
		{
			// clear all the client inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			// braxi -- otherwise we'd end up in changelevel triggers and infinitely jump between maps - tested :))
			savedInuse = malloc(sv_maxclients->value+1 * sizeof(qboolean));
			for(i = 0, cl=svs.clients; i < sv_maxclients->value; i++, cl++)
			{
				savedInuse[i] = cl->edict->inuse;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile ();

			// we must restore these for clients to transfer over correctly
			for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
				cl->edict->inuse = savedInuse[i];
			free (savedInuse);
		}
#endif
	}

	// start up the next map
	SV_Map (false, Cmd_Argv(1), false, true, true );

	// archive server state
	strncpy (svs.mapcmd, Cmd_Argv(1), sizeof(svs.mapcmd)-1);

	// copy off the level to the autosave slot
	if (!dedicated->value)
	{
//		SV_WriteServerFile (true);
//		SV_CopySaveGame ("current", "save0");
	}
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
void SV_Map_f (void)
{
	char	*map;
	char	expanded[MAX_QPATH];

	// if not image, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv(1);
	if (!strstr (map, "."))
	{
		Com_sprintf (expanded, sizeof(expanded), "maps/%s.bsp", map);
		if (FS_LoadFile (expanded, NULL) == -1)
		{
			Com_Printf ("Server cannot find map: `%s`\n", expanded);
			return;
		}
	}

	sv.state = ss_dead;		// don't save current level when changing
//	SV_WipeSavegame("current");
	SV_GameMap_f ();
}

/*
=====================================================================

  SAVEGAMES

=====================================================================
*/


/*
==============
SV_Loadgame_f

==============
*/
void SV_Loadgame_f (void)
{
#if 0
	char	name[MAX_OSPATH];
	FILE	*f;
	char	*dir;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: loadgame <directory>\n");
		return;
	}

	Com_Printf ("Loading game...\n");

	dir = Cmd_Argv(1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\") )
	{
		Com_Printf ("Bad savedir.\n");
	}

	// make sure the server.ssv file exists
	Com_sprintf (name, sizeof(name), "%s/save/%s/server.ssv", FS_Gamedir(), Cmd_Argv(1));
	f = fopen (name, "rb");
	if (!f)
	{
		Com_Printf ("No such savegame: %s\n", name);
		return;
	}
	fclose (f);

	SV_CopySaveGame (Cmd_Argv(1), "current");

	SV_ReadServerFile ();

	// go to the map
	sv.state = ss_dead;		// don't save current level when changing
	SV_Map (false, svs.mapcmd, true, false, false);
#endif
}



/*
==============
SV_Savegame_f

==============
*/
void SV_Savegame_f (void)
{
	char	*dir;

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a game to save.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: savegame <directory>\n");
		return;
	}

	if (Cvar_VariableValue("multiplayer"))
	{
		Com_Printf ("Can't savegame in a multiplayer\n");
		return;
	}

	if (!strcmp (Cmd_Argv(1), "current"))
	{
		Com_Printf ("Can't save to 'current'\n");
		return;
	}

	if (sv_maxclients->value == 1 && svs.clients[0].edict->v.health <= 0)	//	if (sv_maxclients->value == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)
	{
		Com_Printf ("\nCan't savegame while dead!\n");
		return;
	}

	dir = Cmd_Argv(1);
	if (strstr (dir, "..") || strstr (dir, "/") || strstr (dir, "\\") )
	{
		Com_Printf ("Bad savedir.\n");
	}

	Com_Printf ("Saving game...\n");

	// archive current level, including all client edicts.
	// when the level is reloaded, they will be shells awaiting
	// a connecting client
	SV_WriteLevelFile ();

	// save server state
//	SV_WriteServerFile (false);

	// copy it off
//	SV_CopySaveGame ("current", dir);

	Com_Printf ("Done.\n");
}

//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f (void)
{
	if (!svs.initialized)
	{
		Com_Printf ("No server running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: kick <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", sv_client->name);
	// print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf (sv_client, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient (sv_client);
	sv_client->lastmessage = svs.realtime;	// min case there is a funny zombie
}


/*
================
SV_Status_f
================
*/
void SV_Status_f (void)
{
	int			i, j, l;
	client_t	*cl;
	char		*s;
	int			ping;
	int			numplayers = 0;

	if (!svs.clients)
	{
		Com_Printf ("No server running.\n");
		return;
	}

	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
	{
		if (!cl->state)
			continue;
		numplayers++;
	}

	print_time = false;
	Com_Printf("\n--- server status ---\n\n");

	if(developer->value)
		Com_Printf("Server is running in developer mode (%i).\n", (int)developer->value);
	if (sv_cheats->value)
		Com_Printf("Cheats are allowed.\n");

	if (developer->value || sv_cheats->value)
		Com_Printf("\n");


	Com_Printf("hostname  : %s\n", Cvar_VariableString("hostname"));
	Com_Printf("map       : %s\n", sv.mapname);
	Com_Printf("gamedir   : %s\n\n", FS_Gamedir());

	Com_Printf("Clients   : %i / %i\n", numplayers, svs.max_clients);
	Com_Printf("Entities  : %i / %i\n", sv.num_edicts, sv.max_edicts);

	Com_Printf ("num score ping name            lastmsg address               qport \n");
	Com_Printf ("--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
	{
		if (!cl->state)
			continue;

		Com_Printf ("%3i ", i);

		Com_Printf ("%5i ", cl->edict->client->ps.stats[STAT_SCORE]);

		if (cl->state == cs_connected)
			Com_Printf ("CNCT ");
		else if (cl->state == cs_zombie)
			Com_Printf ("ZMBI ");
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf ("%4i ", ping);
		}

		Com_Printf ("%s", cl->name);
		l = 16 - (int)strlen(cl->name);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");

		Com_Printf ("%7i ", svs.realtime - cl->lastmessage );

		s = NET_AdrToString ( cl->netchan.remote_address);
		Com_Printf ("%s", s);
		l = 22 - (int)strlen(s);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");
		
		Com_Printf ("%5i", cl->netchan.qport);

		Com_Printf ("\n");
	}
	Com_Printf ("\n");
	print_time = true;
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy(text, "server: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < sv_maxclients->value; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f (void)
{
	svs.last_heartbeat = -9999999;
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f (void)
{
	Com_Printf ("Server info settings:\n");
	Info_Print (Cvar_Serverinfo());
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
void SV_DumpUser_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: dumpuser <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Com_Printf ("userinfo\n");
	Com_Printf ("--------\n");
	Info_Print (sv_client->userinfo);

}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored. Primarily for demo merging.
==============
*/
void SV_ServerRecord_f (void)
{
	char	name[MAX_OSPATH];
	byte	buf_data[32768]; // was char
	sizebuf_t	buf;
	int		len;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("serverrecord <demoname>\n");
		return;
	}

	if (svs.demofile)
	{
		Com_Printf ("Already recording.\n");
		return;
	}

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	Com_sprintf (name, sizeof(name), "%s/demos/%s.pdm", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("Recording server demo to `%s`.\n", name);
	FS_CreatePath (name);
	svs.demofile = fopen (name, "wb");
	if (!svs.demofile)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// setup a buffer to catch all multicasts
	SZ_Init (&svs.demo_multicast, svs.demo_multicast_buf, sizeof(svs.demo_multicast_buf));

	//
	// write a single giant fake message with all the startup info
	//
	SZ_Init (&buf, buf_data, sizeof(buf_data));

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	MSG_WriteByte (&buf, SVC_SERVERDATA);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, svs.spawncount);
	MSG_WriteByte (&buf, 2);	// demos are always attract loops -- 2 means server demo
	MSG_WriteString (&buf, Cvar_VariableString ("gamedir"));
	MSG_WriteShort (&buf, -1);
	// send full levelname
	MSG_WriteString (&buf, sv.configstrings[CS_NAME]);

	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (sv.configstrings[i][0]) // braxi -- fix
		{
			MSG_WriteByte(&buf, SVC_CONFIGSTRING);
			MSG_WriteShort(&buf, i);
			MSG_WriteString(&buf, sv.configstrings[i]);
		}
	}
	// write it to the demo file
	Com_DPrintf (DP_SV, "signon message length: %i\n", buf.cursize);
	len = LittleLong (buf.cursize);
	fwrite (&len, 4, 1, svs.demofile);
	fwrite (buf.data, buf.cursize, 1, svs.demofile);

	// the rest of the demo file will be individual frames
}


/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
void SV_ServerStop_f (void)
{
	if (!svs.demofile)
	{
		Com_Printf ("Not doing a serverrecord.\n");
		return;
	}
	fclose (svs.demofile);
	svs.demofile = NULL;
	Com_Printf ("Recording completed.\n");
}


/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game

===============
*/
void SV_KillServer_f (void)
{
	if (!svs.initialized)
		return;
	SV_Shutdown ("Server was killed.", false);
	NET_Config ( false );	// close network sockets
}


//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);

	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_AddCommand ("demomap", SV_DemoMap_f);
	Cmd_AddCommand ("gamemap", SV_GameMap_f);
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);

	Cmd_AddCommand("sv_modellist", SV_ModelList_f);

	if ( dedicated->value )
		Cmd_AddCommand ("say", SV_ConSay_f);

	Cmd_AddCommand ("serverrecord", SV_ServerRecord_f);
	Cmd_AddCommand ("serverstop", SV_ServerStop_f);

//	Cmd_AddCommand ("save", SV_Savegame_f);
//	Cmd_AddCommand ("load", SV_Loadgame_f);

	Cmd_AddCommand ("killserver", SV_KillServer_f);
}

