/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "server.h"

netadr_t	master_adr[MAX_MASTER_SERVERS];	// address of group servers

client_t	*sv_client;			// current client

cvar_t	*sv_paused;
cvar_t	*sv_timedemo;

cvar_t	*sv_enforcetime;

cvar_t	*sv_timeout;				// seconds without any message
cvar_t	*sv_zombietime;			// seconds to sink messages after disconnect

cvar_t	*rcon_password;			// password for remote server commands

cvar_t	*allow_download;
cvar_t *allow_download_models;
cvar_t *allow_download_sounds;
cvar_t *allow_download_maps;

cvar_t	*sv_noreload;			// don't reload level state when reentering

cvar_t	*sv_password;
cvar_t	*sv_maxclients;	
cvar_t	*sv_maxentities;
cvar_t	*sv_showclamp;
cvar_t	*sv_cheats;

cvar_t *sv_nolateloading;

cvar_t	*sv_hostname;
cvar_t	*public_server;			// should heartbeats be sent

cvar_t	*sv_reconnect_limit;	// minimum seconds between connect messages

void Master_Shutdown (void);

qboolean Scr_ClientConnect(gentity_t* ent, char* userinfo);
void Scr_ClientDisconnect(gentity_t* ent);
extern void Nav_Shutdown();
//============================================================================

#ifdef _DEBUG
#ifndef DEDICATED_ONLY
// completly out of place, used just in debug builds to see stats

#if 0
typedef enum
{
	XALIGN_NONE = 0,
	XALIGN_LEFT = 0,
	XALIGN_RIGHT = 1,
	XALIGN_CENTER = 2
} UI_AlignX;

void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);
extern PR_Profile(int x, int y);
extern int		time_before, time_between, time_after;
#endif

void ShowServerStats(int x, int y)
{

#if 0
	UI_DrawString(x, y + 10 * 0, XALIGN_RIGHT, va("MAP: %s", sv.configstrings[CS_MODELS + 1]));
	UI_DrawString(x, y + 10 * 2, XALIGN_RIGHT, va("SV: frame %i time %i", sv.framenum, sv.time));
	UI_DrawString(x, y + 10 * 3, XALIGN_RIGHT, va("GAME: frame %i time %f", sv.gameFrame, sv.gameTime));
	UI_DrawString(x, y + 10 * 5, XALIGN_RIGHT, va("ENTS: %i/%i", sv.num_edicts, sv.max_edicts));

	UI_DrawString(x, y + 10 * 7, XALIGN_RIGHT, va("SV %i ms", time_between - time_before));
	UI_DrawString(x, y + 10 * 8, XALIGN_RIGHT, va("PROGS %i ms", time_after_game - time_before_game));

	UI_DrawString(x, y + 10 * 13, XALIGN_RIGHT, "-- profile --");
	PR_Profile(x, y + 10 * 14);
#endif
}
#endif /*DEDICATED_ONLY*/
#endif /*_DEBUG*/

/*
=====================
Com_IsServerActive

Returns true if server has loaded map and is fully active
=====================
*/
qboolean Com_IsServerActive()
{
// return (sv.state >= ss_game);
	return (sv.state == ss_game);
}
/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly or unwillingly.  
This is NOT called if the entire server is quiting or crashing.
=====================
*/
void SV_DropClient (client_t *drop)
{
	// add the disconnect
	MSG_WriteByte (&drop->netchan.message, SVC_DISCONNECT);

	if (drop->state == cs_spawned)
	{
		// call the prog function for removing a client
		// this will remove the body, among other things
		Scr_ClientDisconnect(drop->edict);
	}

	if (drop->download)
	{
		FS_FreeFile (drop->download);
		drop->download = NULL;
	}

	drop->state = cs_zombie;		// become free in a few seconds
	drop->name[0] = 0;
}



/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/
char* SV_StatusString(void)
{
	char	player[1024];
	static char	status[MAX_MSGLEN - 16];
	int		i;
	client_t	*cl;
	int		statusLength;
	int		playerLength;

	strcpy (status, Cvar_Serverinfo());
	strcat (status, "\n");
	statusLength = (int)strlen(status);

	for (i=0 ; i<sv_maxclients->value ; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == cs_connected || cl->state == cs_spawned )
		{
			Com_sprintf (player, sizeof(player), "%i %i \"%s\"\n", 
				cl->edict->client->ps.stats[STAT_SCORE], cl->ping, cl->name);
			playerLength = (int)strlen(player);
			if (statusLength + playerLength >= sizeof(status) )
				break;		// can't hold any more
			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}
	}

	return status;
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
void SVC_Status (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\n%s", SV_StatusString());
}

/*
================
SVC_Ack

================
*/
void SVC_Ack (void)
{
	Com_Printf ("Ping acknowledge from %s\n", NET_AdrToString(net_from));
}

/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
void SVC_Info (void)
{
	char	string[96];
	int		i, numPlayers;
	int		version;

	// ignore in single player and when client is diferent protocol
	if (sv_maxclients->value == 1)
		return;		

	version = atoi (Cmd_Argv(1));
	if (version != PROTOCOL_VERSION)
		return;

	numPlayers = 0;
	for (i=0 ; i<sv_maxclients->value ; i++)
		if (svs.clients[i].state >= cs_connected)
			numPlayers++;

	// "sv_hostname" "game" "map name", "numplayers", "maxplayers"
	Com_sprintf (string, sizeof(string), "\"%s\" \"%s\" \"%s\" \"%i\" \"%i\"\n", sv_hostname->string, Cvar_VariableString("game"), sv.mapname, numPlayers, (int)sv_maxclients->value);

	Netchan_OutOfBandPrint (NS_SERVER, net_from, "info\n%s", string);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "ack");
}


/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = curtime;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect (void)
{
	char		userinfo[MAX_INFO_STRING];
	netadr_t	adr;
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	gentity_t		*ent;
	int			edictnum;
	int			version;
	int			qport;
	int			challenge;

	adr = net_from;

	Com_DPrintf (DP_SV, "SVC_DirectConnect ()\n");

	version = atoi(Cmd_Argv(1));
	if (version != PROTOCOL_VERSION)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is version %f.\n", PRAGMA_VERSION);
		Com_Printf("[%s] rejected connection from %s (client is diferent version %i)\n", GetTimeStamp(false), NET_AdrToString(net_from), version);
		return;
	}

	qport = atoi(Cmd_Argv(2));

	challenge = atoi(Cmd_Argv(3));

	strncpy (userinfo, Cmd_Argv(4), sizeof(userinfo)-1);
	userinfo[sizeof(userinfo) - 1] = 0;

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey (userinfo, "ip", NET_AdrToString(net_from));

	// attractloop servers are ONLY for local clients
	if (sv.attractloop)
	{
		if (!NET_IsLocalAddress (adr))
		{
			Com_Printf("[%s] rejected connection from %s (server in attract loop)\n", GetTimeStamp(false), NET_AdrToString(net_from));
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// see if the challenge is valid
	if (!NET_IsLocalAddress (adr))
	{
		for (i=0 ; i<MAX_CHALLENGES ; i++)
		{
			if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
					break;		// good
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad challenge.\n");
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nNo challenge for address.\n");
			return;
		}
	}

	newcl = &temp;
	memset (newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i=0,cl=svs.clients ; i<sv_maxclients->value ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (NET_CompareBaseAdr (adr, cl->netchan.remote_address)
			&& ( cl->netchan.qport == qport 
			|| adr.port == cl->netchan.remote_address.port ) )
		{
			if (!NET_IsLocalAddress (adr) && (svs.realtime - cl->lastconnect) < ((int)sv_reconnect_limit->value * 1000))
			{
				Com_Printf("[%s] rejected connection from %s (too soon)\n", GetTimeStamp(false), NET_AdrToString(net_from));
				return;
			}

			Com_Printf("[%s] client %s from %s reconnecting\n", GetTimeStamp(false), (strlen(cl->name) > 0 ? cl->name : ""), NET_AdrToString(net_from));

			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	newcl = NULL;
	for (i=0,cl=svs.clients ; i<sv_maxclients->value ; i++,cl++)
	{
		if (cl->state == cs_free)
		{
			newcl = cl;
			break;
		}
	}
	if (!newcl)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is full.\n");
		Com_DPrintf (DP_SV, "Rejected a connection.\n");
		return;
	}

gotnewcl:	
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	sv_client = newcl;
	edictnum = (newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming


	// give progs a chance to reject this connection or modify the userinfo
	if (!(Scr_ClientConnect(ent, userinfo)))
	{
		if (*Info_ValueForKey (userinfo, "rejmsg")) 
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\nConnection refused.\n",  
				Info_ValueForKey (userinfo, "rejmsg"));
		else
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n" );


		Com_DPrintf (DP_SV, "Game rejected a connection.\n");
		return;
	}

	// parse some info from the info strings
	strncpy (newcl->userinfo, userinfo, sizeof(newcl->userinfo)-1);
	SV_UserinfoChanged (newcl);

	// send the connect packet to the client
	Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect");
	Netchan_Setup (NS_SERVER, &newcl->netchan , adr, qport);

	newcl->state = cs_connected;
	
	SZ_Init (&newcl->datagram, newcl->datagram_buf, sizeof(newcl->datagram_buf) );
	newcl->datagram.allowoverflow = true;
	newcl->lastmessage = svs.realtime;	// don't timeout
	newcl->lastconnect = svs.realtime;
}

int Rcon_Validate (void)
{
	if (!strlen (rcon_password->string))
		return 0;

	if (strcmp (Cmd_Argv(1), rcon_password->string) )
		return 0;

	return 1;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand (void)
{
	int		i;
	char	remaining[1024];

	i = Rcon_Validate ();
	
	if (i == 0)
		Com_Printf("[%s] bad rcon from %s\n", GetTimeStamp(false), NET_AdrToString(net_from));
	else
		Com_Printf("[%s] rcon from %s: %s\n", GetTimeStamp(false), NET_AdrToString(net_from), net_message.data+4);

	Com_BeginRedirect (RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!Rcon_Validate ())
	{
		Com_Printf ("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			strcat (remaining, Cmd_Argv(i) );
			strcat (remaining, " ");
		}

		Cmd_ExecuteString (remaining);
	}

	Com_EndRedirect ();
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);		// skip the -1 marker

	s = MSG_ReadStringLine (&net_message);

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);
	Com_DPrintf (DP_SV, "Packet %s : %s\n", NET_AdrToString(net_from), c);

	if (!strcmp(c, "ping"))
		SVC_Ping ();
	else if (!strcmp(c, "ack"))
		SVC_Ack ();
	else if (!strcmp(c,"status"))
		SVC_Status ();
	else if (!strcmp(c,"info"))
		SVC_Info ();
	else if (!strcmp(c,"getchallenge"))
		SVC_GetChallenge ();
	else if (!strcmp(c,"connect"))
		SVC_DirectConnect ();
	else if (!strcmp(c, "rcon"))
		SVC_RemoteCommand ();
	else
		Com_Printf ("bad connectionless packet from %s:\n%s\n", NET_AdrToString (net_from), s);
}


//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings (void)
{
	int			i, j;
	client_t	*cl;
	int			total, count;

	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != cs_spawned )
			continue;

		total = 0;
		count = 0;
		for (j = 0; j < LATENCY_COUNTS; j++)
		{
			if (cl->frame_latency[j] > 0)
			{
				count++;
				total += cl->frame_latency[j];
			}
		}
		if (!count)
			cl->ping = 0;
		else
			cl->ping = total / count;

		// let the game know about the ping
		cl->edict->client->ping = cl->ping;
	}
}


/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/

void SV_GiveMsec (void)
{
	int			i;
	client_t	*cl;

	static const int num_frames = 15;

	if (sv.framenum & num_frames)
		return;

	for (i = 0; i < sv_maxclients->value; i++)
	{
		cl = &svs.clients[i];
		if(cl->state == cs_free)
			continue;
		
		cl->commandMsec = (num_frames * SV_FRAMETIME_MSEC) + 200;
//		cl->commandMsec = 1800;		// braxi -- that was in Q2: 1600 + some slop
	}
}


/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets (void)
{
	int			i;
	client_t	*cl;
	int			qport;

	while (NET_GetPacket (NS_SERVER, &net_from, &net_message))
	{
		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ConnectionlessPacket ();
			continue;
		}

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		MSG_BeginReading (&net_message);
		MSG_ReadLong (&net_message);		// sequence number
		MSG_ReadLong (&net_message);		// sequence number
		qport = MSG_ReadShort (&net_message) & 0xffff;

		// check for packets from connected clients
		for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
		{
			if (cl->state == cs_free)
				continue;
			if (!NET_CompareBaseAdr (net_from, cl->netchan.remote_address))
				continue;
			if (cl->netchan.qport != qport)
				continue;
			if (cl->netchan.remote_address.port != net_from.port)
			{
				Com_Printf ("SV_ReadPackets: fixing up a translated port for '%s'\n", cl->name);
				cl->netchan.remote_address.port = net_from.port;
			}

			if (Netchan_Process(&cl->netchan, &net_message))
			{	
				// this is a valid, sequenced packet, so process it
				if (cl->state != cs_zombie)
				{
					cl->lastmessage = svs.realtime;	// don't timeout
					SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}
		
		if (i != sv_maxclients->value)
			continue;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.  Server frames are used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl;
	int			droppoint;
	int			zombiepoint;

	droppoint = svs.realtime - (1000 * sv_timeout->value);
	zombiepoint = svs.realtime - (1000 * sv_zombietime->value);

	for (i = 0, cl = svs.clients; i < sv_maxclients->value; i++, cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->lastmessage > svs.realtime)
			cl->lastmessage = svs.realtime;

		if (cl->state == cs_zombie && cl->lastmessage < zombiepoint)
		{
			cl->state = cs_free;	// can now be reused
			continue;
		}

		if (i == 0 && !dedicated->value)
		{
			continue; // _NEVER_ timeout the HOST player
		}

		if ( (cl->state == cs_connected || cl->state == cs_spawned) && cl->lastmessage < droppoint)
		{
			SV_BroadcastPrintf (PRINT_HIGH, "Player '%s' timed out.\n", cl->name);
			SV_DropClient (cl); 
			cl->state = cs_free;	// don't bother with zombie state
		}
	}
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
void SV_PrepWorldFrame (void)
{
	gentity_t	*ent;
	int		i;

	for (i = 0; i < sv.max_edicts; i++, ent++)
	{
		ent = EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}
}


/*
=================
SV_RunGameFrame
=================
*/
void SV_RunGameFrame (void)
{
	time_before_game = Sys_Milliseconds ();

	// we always need to bump framenum, even if we don't run the world, otherwise 
	// the delta compression can get confused when a client has the "current" frame
	sv.framenum++;
	sv.time = (sv.framenum * SV_FRAMETIME_MSEC);

	// don't run if paused
	if (!sv_paused->value || sv_maxclients->value > 1)
	{
		SV_RunWorldFrame();

		// never get more than one tic behind
		if (sv.time < svs.realtime)
		{
			if (sv_showclamp->value)
				Com_Printf("WARNING: server highclamp\n");
			svs.realtime = sv.time;
		}
	}

	time_after_game = Sys_Milliseconds ();
}

/*
================
SV_SetConfigString
================
*/
void SV_SetConfigString(int index, const char *valueString)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "SV_SetConfigString(): bad index %i\n", index);
		return;
	}

	if (!valueString || valueString == NULL)
		valueString = "\0";

	// change the string in sv
	strncpy(sv.configstrings[index], valueString, sizeof(sv.configstrings[index]));

	if (sv.state != ss_loading && sv.state != ss_dead)
	{
		SZ_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, SVC_CONFIGSTRING);
		MSG_WriteShort(&sv.multicast, index);
		MSG_WriteString(&sv.multicast, valueString);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R); // send the update to everyone
	}
}

/*
================
SV_NotifyWhenCvarChanged
================
*/
static inline void SV_NotifyWhenCvarChanged(cvar_t* cvar)
{
	if (cvar->modified)
	{
//		Cvar_Set(cvar->name, cvar->latched_string);
		SV_BroadcastPrintf(PRINT_HIGH, "Server changed cvar `%s` to '%s`.\n", cvar->name, cvar->string);
		cvar->modified = false;

		if (cvar == sv_cheats) // haaack no. 9000
			SV_SetConfigString(CS_CHEATS_ENABLED, cvar->value > 0 ? "1" : "0");  //not cvar->string just in case...
	}
}

/*
================
SV_UpdateCvars
================
*/
extern cvar_t* sv_hostname;
void SV_CheckCvars()
{
	// give server enough time to initialize
	if (sv.gameFrame < SERVER_FPS) 
		return; 
	if (sv.state != ss_game)
		return;

	SV_NotifyWhenCvarChanged(sv_cheats);
	
	if (sv_maxclients->value == 1)
		return;

	SV_NotifyWhenCvarChanged(sv_hostname);
	SV_NotifyWhenCvarChanged(sv_maxvelocity);
	SV_NotifyWhenCvarChanged(sv_gravity);
}

/*
==================
SV_Frame
==================
*/
void SV_Frame (int msec)
{
	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
		return;

	if (sv.qcvm_active)
		Scr_BindVM(VM_SVGAME);

    svs.realtime += msec;
	
	rand();				// keep the random time dependent, WHY SO OFTEN?
	SV_CheckTimeouts();	// check timeouts
	SV_ReadPackets();	// get packets from clients

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->value && svs.realtime < sv.time)
	{
		// never let the time get too far off
		if (sv.time - svs.realtime > SV_FRAMETIME_MSEC)
		{
			if (sv_showclamp->value)
				Com_Printf ("WARNING: server lowclamp\n");
			svs.realtime = sv.time - SV_FRAMETIME_MSEC;
		}
		NET_Sleep(sv.time - svs.realtime);
		return;
	}

	SV_CheckCvars();

	SV_CalcPings();				// update ping based on the last known frame from all clients
	SV_GiveMsec();				// give the clients some timeslices
	SV_RunGameFrame();			// let everything in the world think and move
	SV_SendClientMessages();	// send messages back to the clients that had packets read this frame
	SV_RecordDemoMessage();		// save the entire world state if recording a serverdemo
	Master_Heartbeat();			// send a heartbeat to the master if needed
	SV_PrepWorldFrame();		// clear teleport flags, etc for next frame
}

//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void Master_Heartbeat (void)
{
	char		*string;
	int			i;

	
	if (!dedicated->value)
		return;		// only dedicated servers send heartbeats

	if (!public_server->value)
		return;		// a private dedicated game

	// check for time wraparound
	if (svs.last_heartbeat > svs.realtime)
		svs.last_heartbeat = svs.realtime;

	if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS*1000)
		return;		// not time to send yet

	svs.last_heartbeat = svs.realtime;

	// send the same string that we would give for a status OOB command
	string = SV_StatusString();

	// send to group master
	for (i = 0; i < MAX_MASTER_SERVERS; i++)
	{
		if (master_adr[i].port)
		{
			Com_Printf("Sending heartbeat to %s\n", NET_AdrToString(master_adr[i]));
			Netchan_OutOfBandPrint(NS_SERVER, master_adr[i], "heartbeat\n%s", string);
		}
	}
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown (void)
{
	int			i;

	if (!dedicated->value)
		return;		// only dedicated servers send heartbeats

	if (!public_server->value)
		return;		// a private dedicated game

	// send to group master
	for (i = 0; i < MAX_MASTER_SERVERS; i++)
	{
		if (master_adr[i].port)
		{
			if (i > 0)
				Com_Printf("Sending heartbeat to %s\n", NET_AdrToString(master_adr[i]));
			Netchan_OutOfBandPrint(NS_SERVER, master_adr[i], "shutdown");
		}
	}
}

//============================================================================


/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string into a more C friendly form.
=================
*/

void SV_UserinfoChanged (client_t *cl)
{
	const char	*val;
	int		i;

	// call prog code to allow overrides
	ClientUserinfoChanged (cl->edict, cl->userinfo);
	
	// name for C code
	strncpy (cl->name, Info_ValueForKey (cl->userinfo, "name"), sizeof(cl->name)-1);
	
	// mask off high bit
	for (i=0 ; i<sizeof(cl->name) ; i++)
		cl->name[i] &= 127;

	// rate command
	val = Info_ValueForKey (cl->userinfo, "rate");
	if (strlen(val))
	{
		i = atoi(val);
		cl->rate = i;
		if (cl->rate < NET_RATE_MIN)
			cl->rate = NET_RATE_MIN;
		if (cl->rate > NET_RATE_MAX)
			cl->rate = NET_RATE_MAX;
	}
	else
		cl->rate = NET_RATE_DEFAULT;

	// messagelevel command
	val = Info_ValueForKey (cl->userinfo, "messagelevel");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}
	
	SV_SetConfigString((CS_CLIENTS + (NUM_FOR_ENT(cl->edict) - 1)), cl->name);
}


//============================================================================

/*
===============
SV_Init

Only called at pragma.exe startup, not for each game
===============
*/
void SV_Init (void)
{
	SV_InitOperatorCommands	();

	rcon_password = Cvar_Get ("rcon_password", "", 0, "Password for remote console access.");

	Cvar_Get ("coop", "0", CVAR_LATCH, "Enable to force Cooperative mode (this takes priority over multiplayer cvar).");
	Cvar_Get("multiplayer", "0", CVAR_LATCH, "Regular Multiplayer.");

	Cvar_Get ("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO|CVAR_NOSET, "Protocol version, do not change, seriously.");

	sv_nolateloading = Cvar_Get("sv_nolateloading", "0", 0, "Throw error when trying to precache asset after server is done initializing.");
	sv_password = Cvar_Get("sv_password", "", 0, "When set, server will require password to connect.");
	sv_maxclients = Cvar_Get("sv_maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH, "Maximum number of players.");
	sv_cheats = Cvar_Get("sv_cheats", "0", CVAR_SERVERINFO, "Enable cheats.");
	sv_maxentities = Cvar_Get("sv_maxentities", va("%i", MAX_GENTITIES), CVAR_LATCH, "Maximum number of server entities. Better don't change.");
	sv_maxvelocity = Cvar_Get("sv_maxevelocity", "1500", 0, "Maximum velocity of an entities (excluding players).");
	sv_gravity = Cvar_Get("sv_gravity", "800", 0, "Gravity (default 800).");

	sv_hostname = Cvar_Get ("hostname", "pragma server", CVAR_SERVERINFO | CVAR_ARCHIVE, "This is the server's name.");

	sv_timeout = Cvar_Get ("sv_timeout", "10", 0, "Drop client from server if it hasn't sent any message in this many seconds.");
	sv_zombietime = Cvar_Get ("sv_zombietime", "2", 0, "When client leaves, don't let it connect for this many second to prevent spam.");

	sv_showclamp = Cvar_Get ("sv_showclamp", "0", 0, "When enabled, prints warning to console when the server can't keep up.");
	sv_paused = Cvar_Get ("paused", "0", 0, NULL);
	sv_timedemo = Cvar_Get ("timedemo", "0", 0, NULL);
	sv_enforcetime = Cvar_Get ("sv_enforcetime", "0", 0, "Helps preventing speed hacks.");

	allow_download = Cvar_Get ("allow_download", "0", CVAR_ARCHIVE, "When enabled clients will be allowed to download (mod) files from server.");
	allow_download_models = Cvar_Get ("allow_download_models", "1", CVAR_ARCHIVE, "Allow downloading models.");
	allow_download_sounds = Cvar_Get ("allow_download_sounds", "1", CVAR_ARCHIVE, "Allow downloading sounds.");
	allow_download_maps	  = Cvar_Get ("allow_download_maps", "1", CVAR_ARCHIVE, "Allow downloading maps.");

	sv_noreload = Cvar_Get ("sv_noreload", "1", 0, NULL);

	public_server = Cvar_Get ("public", "1", 0, "Set to 0 if you don't want server to be visible in server browser (no worky atm sowwy).");

	sv_reconnect_limit = Cvar_Get ("sv_reconnect_limit", "3", CVAR_ARCHIVE, "Minimum seconds between connect messages.");

	SZ_Init (&net_message, net_message_buffer, sizeof(net_message_buffer));
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage (char *message, qboolean reconnect)
{
	int			i;
	client_t	*cl;
	
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, SVC_PRINT);
	MSG_WriteByte (&net_message, PRINT_HIGH);
	MSG_WriteString (&net_message, message);

	if (reconnect)
		MSG_WriteByte (&net_message, SVC_RECONNECT);
	else
		MSG_WriteByte (&net_message, SVC_DISCONNECT);

	// send it twice
	// stagger the packets to crutch operating system limited buffers

	for (i=0, cl = svs.clients ; i<sv_maxclients->value ; i++, cl++)
		if (cl->state >= cs_connected)
			Netchan_Transmit (&cl->netchan, net_message.cursize, net_message.data);

	for (i=0, cl = svs.clients ; i<sv_maxclients->value ; i++, cl++)
		if (cl->state >= cs_connected)
			Netchan_Transmit (&cl->netchan, net_message.cursize, net_message.data);
}



/*
================
SV_Shutdown

Called when each game quits, before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (char *finalmsg, qboolean reconnect)
{
	if (svs.clients)
		SV_FinalMessage (finalmsg, reconnect);

	if (dedicated->value)
		Com_Printf("[%s] %s\n", GetTimeStamp(true), finalmsg);

	Master_Shutdown ();

	// Free svgame QCVM
	Scr_FreeScriptVM(VM_SVGAME);
	Z_FreeTags(TAG_SERVER_GAME);
	SV_FreeModels();

	// close sv demo
	if (sv.demofile)
	{
		fclose(sv.demofile);
		sv.demofile = NULL;
	}

	// wipe per level server data
	memset(&sv, 0, sizeof(sv));
	 
	Nav_Shutdown();

	SV_FreeDevTools();
	Com_SetServerState (sv.state);

	//
	// free server static data
	//
	if (svs.gclients)
	{
		Z_Free(svs.gclients);
		svs.gclients = NULL;
	}

	if (svs.clients)
	{
		Z_Free(svs.clients);
		svs.clients = NULL;
	}

	if (svs.client_entities)
	{
		Z_Free(svs.client_entities);
		svs.client_entities = NULL;
	}

	// close another sv sdemo?
	if (svs.demofile)
	{
		fclose(svs.demofile);
		svs.demofile = NULL;
	}

	memset (&svs, 0, sizeof(svs));
}

