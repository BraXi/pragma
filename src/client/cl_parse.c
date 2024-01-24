/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cl_parse.c  -- parse a message received from the server

#include "client.h"

char *svc_strings[256] =
{
	"svc_bad",

	"svc_muzzleflash",
	"svc_temp_entity",
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
extern void CL_FireEntityEvents(frame_t* frame);

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
		Com_Error (ERR_DROP,"Server is diferent version");

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
		CL_SetLightStyleFromCS(i - CS_LIGHTS);
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

		case SVC_PLAYERINFO:
		case SVC_PACKET_ENTITIES:
		case SVC_DELTA_PACKET_ENTITIES:
			Com_Error (ERR_DROP, "Out of place frame data");
			break;
		}
	}

	CL_AddNetGraph ();

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (cls.demorecording && !cls.demowaiting)
		CL_WriteDemoMessage ();

}



/*
=========================================================================

FRAME PARSING

=========================================================================
*/



/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
static int	bitcounts[32];	/// just for protocol profiling
int CL_ParseEntityBits(unsigned* bits)
{
	unsigned	b, total;
	int			i;
	int			number;

	total = MSG_ReadByte(&net_message);
	if (total & U_MOREBITS_1)
	{
		b = MSG_ReadByte(&net_message);
		total |= b << 8;
	}
	if (total & U_MOREBITS_2)
	{
		b = MSG_ReadByte(&net_message);
		total |= b << 16;
	}
	if (total & U_MOREBITS_3)
	{
		b = MSG_ReadByte(&net_message);
		total |= b << 24;
	}

	// count the bits for net profiling
	for (i = 0; i < 32; i++)
		if (total & (1 << i))
			bitcounts[i]++;

	if (total & U_NUMBER_16)
		number = MSG_ReadShort(&net_message);
	else
		number = MSG_ReadByte(&net_message);

	*bits = total;

	return number;
}

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CL_ParseDelta(entity_state_t* from, entity_state_t* to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy(from->origin, to->old_origin);
	to->number = number;


	// main model
	if (bits & U_MODELINDEX_8)
		to->modelindex = MSG_ReadByte(&net_message);
	if (bits & U_MODELINDEX_16)
		to->modelindex = MSG_ReadShort(&net_message);

	// attached models
	if (bits & U_MODELINDEX2_8)
		to->modelindex2 = MSG_ReadByte(&net_message);
	if (bits & U_MODELINDEX3_8)
		to->modelindex3 = MSG_ReadByte(&net_message);
	if (bits & U_MODELINDEX4_8)
		to->modelindex4 = MSG_ReadByte(&net_message);

	// animation frame
	if (bits & U_ANIMFRAME_8)
		to->frame = MSG_ReadByte(&net_message);
	if (bits & U_ANIMFRAME_16)
		to->frame = MSG_ReadShort(&net_message);

	// animation sequence
	if (bits & U_ANIMATION)
	{
		to->anim = MSG_ReadByte(&net_message);
		to->animtime = MSG_ReadLong(&net_message);
	}

	// index to model skin
	if (bits & U_SKIN_8)
		to->skinnum = MSG_ReadByte(&net_message);

	// effects
	if ((bits & (U_EFFECTS_8 | U_EFFECTS_16)) == (U_EFFECTS_8 | U_EFFECTS_16))
		to->effects = MSG_ReadLong(&net_message);
	else if (bits & U_EFFECTS_8)
		to->effects = MSG_ReadByte(&net_message);
	else if (bits & U_EFFECTS_16)
		to->effects = MSG_ReadShort(&net_message);

	// render flags
	if ((bits & (U_RENDERFLAGS_8 | U_RENDERFLAGS_16)) == (U_RENDERFLAGS_8 | U_RENDERFLAGS_16))
		to->renderFlags = MSG_ReadLong(&net_message);
	else if (bits & U_RENDERFLAGS_8)
		to->renderFlags = MSG_ReadByte(&net_message);
	else if (bits & U_RENDERFLAGS_16)
		to->renderFlags = MSG_ReadShort(&net_message);

	// render scale
	if (bits & U_RENDERSCALE)
		to->renderScale = MSG_ReadByte(&net_message) * (1.0f / 16.0f);

	// render color
	if (bits & U_RENDERCOLOR)
	{
		to->renderColor[0] = MSG_ReadByte(&net_message) * (1.0f / 255.0f);
		to->renderColor[1] = MSG_ReadByte(&net_message) * (1.0f / 255.0f);
		to->renderColor[2] = MSG_ReadByte(&net_message) * (1.0f / 255.0f);
	}

	// render alpha
	if (bits & U_RENDERALPHA)
		to->renderAlpha = MSG_ReadByte(&net_message) * (1.0f / 255.0f);

	// current origin
	if (bits & U_ORIGIN_X)
		to->origin[0] = MSG_ReadCoord(&net_message);
	if (bits & U_ORIGIN_Y)
		to->origin[1] = MSG_ReadCoord(&net_message);
	if (bits & U_ORIGIN_Z)
		to->origin[2] = MSG_ReadCoord(&net_message);

	// current angles
	if (bits & U_ANGLE_X)
		to->angles[0] = MSG_ReadAngle(&net_message);
	if (bits & U_ANGLE_Y)
		to->angles[1] = MSG_ReadAngle(&net_message);
	if (bits & U_ANGLE_Z)
		to->angles[2] = MSG_ReadAngle(&net_message);

	// old origin (used for smoothing move)
	if (bits & U_OLDORIGIN)
		MSG_ReadPos(&net_message, to->old_origin);

	// looping sound
	if (bits & U_LOOPSOUND)
	{
#ifdef PROTOCOL_EXTENDED_ASSETS
		to->loopingSound = MSG_ReadShort(&net_message);
#else
		to->loopingSound = MSG_ReadByte(&net_message);
#endif
	}

	// event
	if (bits & U_EVENT_8)
		to->event = MSG_ReadByte(&net_message);
	else
		to->event = 0;

	// solid
	if (bits & U_PACKEDSOLID)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		to->solid = MSG_ReadLong(&net_message);
#else
		to->solid = MSG_ReadShort(&net_message);
#endif
	}
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity(frame_t* frame, int newnum, entity_state_t* old, int bits)
{
	ccentity_t* ent;
	entity_state_t* state;

	ent = &cl_entities[newnum];

	state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES - 1)];
	cl.parse_entities++;
	frame->num_entities++;

	CL_ParseDelta(old, state, newnum, bits);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex
		|| state->modelindex2 != ent->current.modelindex2
		|| state->modelindex3 != ent->current.modelindex3
		|| state->modelindex4 != ent->current.modelindex4
		|| abs(state->origin[0] - ent->current.origin[0]) > 512
		|| abs(state->origin[1] - ent->current.origin[1]) > 512
		|| abs(state->origin[2] - ent->current.origin[2]) > 512
		|| state->event == EV_PLAYER_TELEPORT
		|| state->event == EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		ent->trailcount = 1024;		// for diminishing rocket / grenade trails
		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == EV_OTHER_TELEPORT)
		{
			VectorCopy(state->origin, ent->prev.origin);
			VectorCopy(state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy(state->old_origin, ent->prev.origin);
			VectorCopy(state->old_origin, ent->lerp_origin);
		}
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities(frame_t* oldframe, frame_t* newframe)
{
	int				newnum;
	unsigned int	bits;
	entity_state_t* oldstate = NULL;
	int			oldindex, oldnum;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if (!oldframe)
		oldnum = 99999;
	else
	{
		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		newnum = CL_ParseEntityBits(&bits);
		if (newnum >= MAX_GENTITIES)
			Com_Error(ERR_DROP, "CL_ParsePacketEntities: bad number:%i", newnum);

		if (net_message.readcount > net_message.cursize)
			Com_Error(ERR_DROP, "CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum)
		{	// one or more entities from the old packet are unchanged
			if (cl_shownet->value == 3)
				Com_Printf("   unchanged: %i\n", oldnum);
			CL_DeltaEntity(newframe, oldnum, oldstate, 0);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & U_REMOVE)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet->value == 3)
				Com_Printf("   remove: %i\n", newnum);
			if (oldnum != newnum)
				Com_Printf("U_REMOVE: oldnum != newnum\n");

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum)
		{	// delta from previous state
			if (cl_shownet->value == 3)
				Com_Printf("   delta: %i\n", newnum);
			CL_DeltaEntity(newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{	// delta from baseline
			if (cl_shownet->value == 3)
				Com_Printf("   baseline: %i\n", newnum);
			CL_DeltaEntity(newframe, newnum, &cl_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{	// one or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
			Com_Printf("   unchanged: %i\n", oldnum);
		CL_DeltaEntity(newframe, oldnum, oldstate, 0);

		oldindex++;

		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldnum = oldstate->number;
		}
	}
}



/*
===================
CL_ParsePlayerstate
===================
*/
void CL_ParsePlayerstate(frame_t* oldframe, frame_t* newframe)
{
	int			flags;
	player_state_t* state;
	int			i;
	int			statbits;

	state = &newframe->playerstate;

	// clear to old value before delta parsing
	if (oldframe)
		*state = oldframe->playerstate;
	else
		memset(state, 0, sizeof(*state));

	flags = MSG_ReadShort(&net_message);
	if (flags & PS_EXTRABYTES)
		flags = (MSG_ReadShort(&net_message) << 16) | (flags & 0xFFFF); // reki --  Allow extra bytes, so we don't choke ourselves

	//
	// parse the pmove_state_t
	//
	if (flags & PS_M_TYPE)
		state->pmove.pm_type = MSG_ReadByte(&net_message);

	if (flags & PS_M_ORIGIN)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		for (i = 0; i < 3; i++)
			state->pmove.origin[i] = MSG_ReadFloat(&net_message);
#else
		for (i = 0; i < 3; i++)
			state->pmove.origin[i] = MSG_ReadShort(&net_message);
#endif
	}

	if (flags & PS_M_VELOCITY)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		for (i = 0; i < 3; i++)
			state->pmove.velocity[i] = MSG_ReadFloat(&net_message);
#else
		for (i = 0; i < 3; i++)
			state->pmove.velocity[i] = MSG_ReadShort(&net_message);
#endif
	}

	if (flags & PS_M_TIME)
		state->pmove.pm_time = MSG_ReadByte(&net_message);

	if (flags & PS_M_FLAGS)
	{
		state->pmove.pm_flags = MSG_ReadShort(&net_message);
		if (flags & PS_M_FLAGSLONG)
			state->pmove.pm_flags = (MSG_ReadShort(&net_message) << 16) | (state->pmove.pm_flags & 0xFFFF);
	}

	if (flags & PS_M_GRAVITY)
		state->pmove.gravity = MSG_ReadShort(&net_message);

	if (flags & PS_M_DELTA_ANGLES)
	{
		for (i = 0; i < 3; i++)
		{
			state->pmove.delta_angles[i] = MSG_ReadShort(&net_message);
		}
	}

	if (flags & PS_M_BBOX_SIZE) // mins and maxs
	{
		for (i = 0; i < 3; i++)
			state->pmove.mins[i] = MSG_ReadChar(&net_message);

		for (i = 0; i < 3; i++)
			state->pmove.maxs[i] = MSG_ReadChar(&net_message);
	}

	if (cl.attractloop)
		state->pmove.pm_type = PM_FREEZE;		// demo playback

	//
	// parse the rest of the player_state_t
	//
	if (flags & PS_VIEWOFFSET)
	{
		for (i = 0; i < 3; i++)
			state->viewoffset[i] = MSG_ReadChar(&net_message);
	}

	if (flags & PS_VIEWANGLES)
	{
		for (i = 0; i < 3; i++)
			state->viewangles[i] = MSG_ReadAngle16(&net_message);
	}

	if (flags & PS_KICKANGLES)
	{
		for (i = 0; i < 3; i++)
			state->kick_angles[i] = MSG_ReadChar(&net_message) * 0.25;
	}

	if (flags & PS_VIEWMODEL_INDEX)
	{
#ifdef PROTOCOL_EXTENDED_ASSETS
		state->viewmodel_index = MSG_ReadShort(&net_message);
#else
		state->viewmodel_index = MSG_ReadByte(&net_message);
#endif
	}

	if (flags & PS_VIEWMODEL_FRAME)
	{
		state->viewmodel_frame = MSG_ReadByte(&net_message); // braxi -- do I need more than 256 frames for viewmodel?

		for (i = 0; i < 3; i++)
			state->viewmodel_offset[i] = MSG_ReadChar(&net_message) * 0.25;

		for (i = 0; i < 3; i++)
			state->viewmodel_angles[i] = MSG_ReadChar(&net_message) * 0.25;
	}

	if (flags & PS_BLEND)
	{
		for (i = 0; i < 4; i++)
			state->blend[i] = MSG_ReadByte(&net_message) / 255.0;
	}

	if (flags & PS_FOV)
		state->fov = MSG_ReadByte(&net_message);

	if (flags & PS_RDFLAGS)
		state->rdflags = MSG_ReadByte(&net_message);

	// parse stats
	statbits = MSG_ReadLong(&net_message);
	for (i = 0; i < MAX_STATS; i++)
		if (statbits & (1 << i))
			state->stats[i] = MSG_ReadShort(&net_message);
}





/*
================
CL_ParseFrame
================
*/
void CL_ParseFrame(void)
{
	int			cmd;
	int			len;
	frame_t* old;

	memset(&cl.frame, 0, sizeof(cl.frame));

	cl.frame.serverframe = MSG_ReadLong(&net_message);
	cl.frame.deltaframe = MSG_ReadLong(&net_message);
	cl.frame.servertime = cl.frame.serverframe * SV_FRAMETIME_MSEC;

	cl.surpressCount = MSG_ReadByte(&net_message);

	if (cl_shownet->value == 3)
		Com_Printf("   frame:%i  delta:%i\n", cl.frame.serverframe, cl.frame.deltaframe);

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message 
	if (cl.frame.deltaframe <= 0)
	{
		cl.frame.valid = true;		// uncompressed frame
		old = NULL;
		cls.demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.frames[cl.frame.deltaframe & UPDATE_MASK];
		if (!old->valid)
		{	// should never happen
			Com_Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverframe != cl.frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf("Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES - 128)
		{
			Com_Printf("Delta parse_entities too old.\n");
		}
		else
			cl.frame.valid = true;	// valid delta parse
	}

	// clamp time 
	if (cl.time > cl.frame.servertime)
		cl.time = cl.frame.servertime;
	else if (cl.time < cl.frame.servertime - SV_FRAMETIME_MSEC)
		cl.time = cl.frame.servertime - SV_FRAMETIME_MSEC;

	// read areabits
	len = MSG_ReadByte(&net_message);
	MSG_ReadData(&net_message, &cl.frame.areabits, len);

	// read playerinfo
	cmd = MSG_ReadByte(&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != SVC_PLAYERINFO)
		Com_Error(ERR_DROP, "CL_ParseFrame: not playerinfo");
	CL_ParsePlayerstate(old, &cl.frame);

	// read packet entities
	cmd = MSG_ReadByte(&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != SVC_PACKET_ENTITIES)
		Com_Error(ERR_DROP, "CL_ParseFrame: not packetentities");
	CL_ParsePacketEntities(old, &cl.frame);

	// save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverframe & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != ca_active)
		{
			cls.state = ca_active;
			cl.force_refdef = true;

#if PROTOCOL_FLOAT_COORDS == 1
			cl.predicted_origin[0] = cl.frame.playerstate.pmove.origin[0];
			cl.predicted_origin[1] = cl.frame.playerstate.pmove.origin[1];
			cl.predicted_origin[2] = cl.frame.playerstate.pmove.origin[2];
#else
			cl.predicted_origin[0] = cl.frame.playerstate.pmove.origin[0] * 0.125;
			cl.predicted_origin[1] = cl.frame.playerstate.pmove.origin[1] * 0.125;
			cl.predicted_origin[2] = cl.frame.playerstate.pmove.origin[2] * 0.125;
#endif

			VectorCopy(cl.frame.playerstate.viewangles, cl.predicted_angles);

			if (cls.disable_servercount != cl.servercount && cl.refresh_prepped)
				SCR_EndLoadingPlaque();	// get rid of loading plaque
		}
		cl.sound_prepped = true;	// can start mixing ambient sounds

		// fire entity events
		CL_FireEntityEvents(&cl.frame);
		CL_CheckPredictionError();
	}
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