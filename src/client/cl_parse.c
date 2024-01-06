/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_parse.c  -- parse a message received from the server

#include "client.h"

char *svc_strings[256] =
{
	"svc_bad",

	"svc_muzzleflash",
	"svc_temp_entity",
	"svc_layout",
	"svc_inventory",
	"svc_cgcmd",

	"svc_nop",

	"svc_disconnect",
	"svc_reconnect",

	"svc_sound",
	"svc_stopsound",

	"svc_print",

	"svc_stufftext",

	"svc_serverdata",
	"svc_configstring",
	"svc_spawnbaseline",

	"svc_centerprint",

	"svc_download",

	"svc_playerinfo",

	"svc_packet_entities",
	"svc_delta_packet_entities",
	"svc_frame"
};

extern void CL_ParseDownload(void);

/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds (void)
{
	int		i;

	S_BeginRegistration ();
	CL_RegisterTEntSounds ();
	for (i=1 ; i<MAX_SOUNDS; i++)
	{
		if (!cl.configstrings[CS_SOUNDS+i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound (cl.configstrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}
	S_EndRegistration ();
}



/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData (void)
{
	extern cvar_t	*fs_gamedirvar;
	char	*str;
	int		i;
	
	Com_DPrintf (DP_NET, "Serverdata packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();
	cls.state = ca_connected;

// parse protocol version number
	i = MSG_ReadLong (&net_message);
	cls.serverProtocol = i;

	if (i != PROTOCOL_VERSION)
		Com_Error (ERR_DROP,"Server returned version %i, not %i", i, PROTOCOL_VERSION);

	cl.servercount = MSG_ReadLong (&net_message);
	cl.attractloop = MSG_ReadByte (&net_message);

	// game directory
	str = MSG_ReadString (&net_message);
	strncpy (cl.gamedir, str, sizeof(cl.gamedir)-1);

	// set gamedir
	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string || strcmp(fs_gamedirvar->string, str))) || (!*str && (fs_gamedirvar->string || *fs_gamedirvar->string)))
		Cvar_Set("game", str);

	// parse player entity number
	cl.playernum = MSG_ReadShort (&net_message);

	// get the full level name
	str = MSG_ReadString (&net_message);

	if (cl.playernum == -1)
	{	// playing a cinematic or showing a pic, not a level
		SCR_PlayCinematic (str);
	}
	else
	{
		// seperate the printfs so the server message can have a color
		Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf ("%c%s\n", 2, str);

		// need to prep refresh at next oportunity
		cl.refresh_prepped = false;
	}

	CL_InitClientGame();
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (void)
{
	entity_state_t	*es;
	int				bits;
	int				newnum;
	entity_state_t	nullstate;

	memset (&nullstate, 0, sizeof(nullstate));

	newnum = CL_ParseEntityBits (&bits);
	es = &cl_entities[newnum].baseline;
	CL_ParseDelta (&nullstate, es, newnum, bits);
}


/*
================
CL_ParseConfigString
================
*/
extern void CL_SetSkyFromConfigstring();
void CL_ParseConfigString (void)
{
	int		i;
	char	*s;

	i = MSG_ReadShort (&net_message);
	if (i < 0 || i >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "configstring > MAX_CONFIGSTRINGS");
		return; //silence compiler warning
	}

	s = MSG_ReadString(&net_message);
	strcpy (cl.configstrings[i], s);

	// do something apropriate 

	if (i >= CS_LIGHTS && i < CS_LIGHTS + MAX_LIGHTSTYLES)
	{
		CL_SetLightstyle(i - CS_LIGHTS);
	}
	else if (i >= CS_SKY && i < CS_HUD)
	{
		if (cl.refresh_prepped)
			CL_SetSkyFromConfigstring();
	}
	else if (i >= CS_MODELS && i < CS_MODELS+MAX_MODELS)
	{
		if (cl.refresh_prepped)
		{
			cl.model_draw[i-CS_MODELS] = re.RegisterModel (cl.configstrings[i]);
			if (cl.configstrings[i][0] == '*')
				cl.model_clip[i-CS_MODELS] = CM_InlineModel (cl.configstrings[i]);
			else
				cl.model_clip[i-CS_MODELS] = NULL;
		}
	}
	else if (i >= CS_SOUNDS && i < CS_SOUNDS+MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.sound_precache[i-CS_SOUNDS] = S_RegisterSound (cl.configstrings[i]);
	}
	else if (i >= CS_IMAGES && i < CS_IMAGES+MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.image_precache[i-CS_IMAGES] = re.RegisterPic (cl.configstrings[i]);
	}
}


/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos_v;
	float	*pos;
    int 	channel, ent;
    int 	sound_num;
    float 	volume;
    float 	attenuation;  
	int		flags;
	float	ofs;

	flags = MSG_ReadByte (&net_message);

	if (flags & SND_INDEX_16)
		sound_num = MSG_ReadShort(&net_message);
	else
		sound_num = MSG_ReadByte(&net_message);

    if (flags & SND_VOLUME)
		volume = MSG_ReadByte (&net_message) / 255.0;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte (&net_message) / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;	

    if (flags & SND_OFFSET)
		ofs = MSG_ReadByte (&net_message) / 1000.0;
	else
		ofs = 0;

	if (flags & SND_ENT)
	{	// entity reletive
		channel = MSG_ReadShort(&net_message); 
		ent = channel>>3;
		if (ent > MAX_GENTITIES)
			Com_Error (ERR_DROP,"CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS)
	{	// positioned in space
		MSG_ReadPos (&net_message, pos_v);
 
		pos = pos_v;
	}
	else	// use entity number
		pos = NULL;

	if (!cl.sound_precache[sound_num])
		return;

	S_StartSound (pos, ent, channel, cl.sound_precache[sound_num], volume, attenuation, ofs);
}       

/*
==================
CL_ParseStopSoundPacket
==================
*/
void CL_ParseStopSoundPacket(void)
{
	int entnum = MSG_ReadShort(&net_message);

	if (entnum > MAX_GENTITIES || entnum < 0)
		Com_Error(ERR_DROP, "CL_ParseStopSoundPacket: ent = %i", entnum);

	S_StopEntitySounds(entnum);
}


void SHOWNET(char *s)
{
	if (cl_shownet->value>=2)
		Com_Printf ("%3i:%s\n", net_message.readcount-1, s);
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i;

//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
		Com_Printf ("%i ",net_message.cursize);
	else if (cl_shownet->value >= 2)
		Com_Printf ("------------------\n");


//
// parse the message
//
	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte (&net_message);

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (cl_shownet->value>=2)
		{
			if (!svc_strings[cmd])
				Com_Printf ("%3i:BAD CMD %i\n", net_message.readcount-1,cmd);
			else
				SHOWNET(svc_strings[cmd]);
		}
	
	// other commands
		switch (cmd)
		{
		default:
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message %i\n", cmd);
			break;
			
		case SVC_NOP:
			SHOWNET("SVC_NOP\n");
//			Com_Printf ("svc_nop\n");
			break;
			
		case SVC_DISCONNECT:
			Com_Error(ERR_DISCONNECT, "Server disconnected\n");
			break;

		case SVC_RECONNECT:
			Com_Printf ("Server disconnected, reconnecting\n");
			if (cls.download) 
			{
				//close download
				fclose (cls.download);
				cls.download = NULL;
			}
			cls.state = ca_connecting;
			cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
			break;

		case SVC_PRINT:
			i = MSG_ReadByte (&net_message);
			if (i == PRINT_CHAT)
			{
				S_StartLocalSound ("misc/talk.wav");
				con.ormask = 128;
			}
			Com_Printf ("%s", MSG_ReadString (&net_message));
			con.ormask = 0;
			break;
			
		case SVC_CENTERPRINT:
			SCR_CenterPrint (MSG_ReadString (&net_message));
			break;
			
		case SVC_STUFFTEXT:
			s = MSG_ReadString (&net_message);
//			Com_DPrintf (DP_NET,"SVC_STUFFTEXT: %s\n", s);
			Cbuf_AddText (s);
			break;
			
		case SVC_SERVERDATA:
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CL_ParseServerData ();
			break;
			
		case SVC_CONFIGSTRING:
			CL_ParseConfigString();
			break;
			
		case SVC_SOUND:
			CL_ParseStartSoundPacket();
			break;

		case SVC_STOPSOUND:
			CL_ParseStopSoundPacket();
			break;
			
		case SVC_SPAWNBASELINE:
			CL_ParseBaseline ();
			break;

		case SVC_TEMP_ENTITY:
			CL_ParseTEnt ();
			break;

		case SVC_MUZZLEFLASH:
			CL_ParseMuzzleFlash ();
			break;

		case SVC_CGCMD:
			CG_ServerCommand();
			break;
\
		case SVC_DOWNLOAD:
			CL_ParseDownload ();
			break;

		case SVC_FRAME:
			CL_ParseFrame ();
			break;

		case SVC_INVENTORY:
			CL_ParseInventory ();
			break;

		case SVC_LAYOUT:
			s = MSG_ReadString (&net_message);
			strncpy (cl.layout, s, sizeof(cl.layout)-1);
			break;

		case SVC_PLAYERINFO:
		case SVC_PACKET_ENTITIES:
		case SVC_DELTA_PACKET_ENTITIES:
			Com_Error (ERR_DROP, "Out of place frame data");
			break;
		}
	}

	CL_AddNetgraph ();

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (cls.demorecording && !cls.demowaiting)
		CL_WriteDemoMessage ();

}

/*
====================
CL_ServerAllowsCheats

Returns true if server allows for cheating
====================
*/
qboolean CL_CheatsAllowed()
{
	if (cls.state == ca_disconnected)
		return true;

	if (cl.configstrings[CS_CHEATS_ENABLED][0] && atoi(cl.configstrings[CS_CHEATS_ENABLED]) > 0)
		return true;

//	if (cl.configstrings[CS_CHEATS_ENABLED][0] && (int)cl.configstrings[CS_CHEATS_ENABLED][0] > 0)
//		return true;

	return false;
}