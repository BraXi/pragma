/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#include "server.h"

// =================================================================================

static void CheckEmptyString(char* s)
{
	if (s[0] <= ' ')
		Scr_RunError("Bad string");
}

/*
=================
index precache_model(string)
=================
*/
void PFSV_precache_model(void)
{
	char* name = Scr_GetParmString(0);
	CheckEmptyString(name);
	Scr_ReturnFloat(SV_ModelIndex(name));
}

/*
=================
index precache_sound(string)
=================
*/
void PFSV_precache_sound(void)
{
	char* name = Scr_GetParmString(0);
	CheckEmptyString(name);
	Scr_ReturnFloat(SV_SoundIndex(name));
}

/*
=================
index precache_image(string)
=================
*/
void PFSV_precache_image(void)
{
	char* name = Scr_GetParmString(0);
	CheckEmptyString(name);
	Scr_ReturnFloat(SV_ImageIndex(name));
}

// =================================================================================

/*
=================
PFSV_spawn
entity spawn()
=================
*/
void PFSV_spawn(void)
{
	gentity_t* ent;
	ent = SV_SpawnEntity();
	Scr_ReturnEntity(ent);
}

/*
=================
PFSV_remove
remove(entity)
=================
*/
void PFSV_remove(void)
{
	gentity_t* ent;
	if ((ent = Scr_GetParmEdict(0)) && ent->inuse)
		SV_FreeEntity(ent);
}

/*
=================
PFSV_getent

returns entity by its index, if entity is not in use returns world

entity getent(float)
=================
*/
void PFSV_getent(void)
{
	gentity_t* ent;
	int entnum;

	entnum = (int)Scr_GetParmFloat(0);

	if (entnum < 0 || entnum >= sv.max_edicts)
	{
		Scr_RunError("getent(): entnum %i is out of range [0-%i]\n", entnum, sv.max_edicts);
		return;
	}
	ent = EDICT_NUM(entnum);
	if (!ent->inuse)
	{
	//	ent = sv.edicts;
		Com_Printf("getent(): %i is not in use\n", entnum);
	}

	Scr_ReturnEntity(ent);
}

/*
=================
PFSV_nextent

finds next active entity, returns world if no entity found

entity nextent(entity previousEnt)
=================
*/
void PFSV_nextent(void)
{
	gentity_t* ent;
	int entnum;

	entnum = NUM_FOR_EDICT( Scr_GetParmEdict(0) ) + 1;// start from next entity
	ent = sv.edicts; //world

	if (entnum >= sv.max_edicts-1)
		goto retent;
	
	for(; entnum < sv.max_edicts; entnum++ )
	{
		ent = EDICT_NUM(entnum);
		if(ent->inuse)
			break;
		
	}

retent:
	Scr_ReturnEntity(ent);
}

/*
=================
PFSV_find

entity find(entity start, .string field, string match);
=================
*/
void PFSV_find(void)
{
	Scr_ReturnEntity(sv.edicts);
}


/*
=================
PFSV_findradius

Returns a chain of entities that have origins within a spherical area
findradius(origin, radius, nonsolid)
=================
*/

/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (entity from, origin, radius)
=================
*/
void PFSV_findradius(void)
{
	gentity_t* from; 
	float *org;
	float rad;

	vec3_t	eorg;
	int		i, j;

	from = Scr_GetParmEdict(0);
	org = Scr_GetParmVector(1);
	rad = Scr_GetParmFloat(2);

	for (i = NUM_FOR_EDICT(from) + 1; i < sv.max_edicts; i++)
	{
		from = EDICT_NUM(i);//NEXT_EDICT(from);

		if(!from->inuse)
			continue;

//		if (from->v.solid == SOLID_NOT)
//			continue;

		for(j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->v.origin[j] + (from->v.mins[j] + from->v.maxs[j]) * 0.5);

		if (VectorLength(eorg) > rad)
			continue;

		Scr_ReturnEntity(from);
		return;

	}
	Scr_ReturnEntity(sv.edicts);
}

/*
=================
PFSV_entnum

returns entity's index

float entnum(entity ent)
=================
*/
void PFSV_getEntNum(void)
{
	Scr_ReturnFloat( NUM_FOR_EDICT(Scr_GetParmEdict(0)) );
}


// =================================================================================

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  
Directly changing origin will not set internal links correctly, so clipping would be messed up.  
This should be called when an object is spawned, and then only if it is teleported.

setorigin(entity, origin)
=================
*/
void PFSV_setorigin(void)
{
	gentity_t	*ent;
	float		*org;

	ent = Scr_GetParmEdict(0);
	org = Scr_GetParmVector(1);

	VectorCopy(org, ent->v.origin);
	SV_LinkEdict(ent);
}
/*
=================
PFSV_setmodel
setmodel(entity,string)
=================
*/
void PFSV_setmodel(void)
{
	gentity_t* ent;
	cmodel_t* mod;
	char* name;
	int i;

	ent = Scr_GetParmEdict(0);
	if (!ent->inuse)
		return;

	if (ent == sv.edicts)  //don't change world
	{
		Scr_RunError("setmodel(): cannot change world model\n");
		return;
	}

	name = Scr_GetParmString(1);
	if (!name || name == "")
	{
		Scr_RunError("setmodel(): empty model name for entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
//	name = "*3";
	i = SV_ModelIndex(name);
	ent->v.model = Scr_SetString(name);
	ent->v.modelindex[0] = i;

	// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		mod = CM_InlineModel(name);
		VectorCopy(mod->mins, ent->v.mins);
		VectorCopy(mod->maxs, ent->v.maxs);
		SV_LinkEdict(ent);
	}
}


/*
=================
PFSV_setsize

This will set entity's bbox size and link entity to the world
For players it additionaly sets bbox size used in prediction and pmove

setsize(entity,vector,vector)
=================
*/
void PFSV_setsize(void)
{
	gentity_t* ent;
	float* min, * max;

	ent = Scr_GetParmEdict(0);
	if (!ent->inuse || ent == sv.edicts /*don't change world*/)
		return;

	min = Scr_GetParmVector(1);
	max = Scr_GetParmVector(2);

	VectorCopy(min, ent->v.mins);
	VectorCopy(max, ent->v.maxs);
	VectorSubtract(max, min, ent->v.size);

	if (ent->client)
	{
		// if entity is a player, set pmove bbox too
		VectorCopy(min, ent->client->ps.pmove.mins);
		VectorCopy(max, ent->client->ps.pmove.maxs);
	}

	SV_LinkEdict(ent);
}

// =================================================================================

/*
=================
PFSV_linkentity
linkentity(entity)
=================
*/
void PFSV_linkentity(void)
{
	gentity_t* ent;
	ent = Scr_GetParmEdict(0);
	if (!ent->inuse || ent == sv.edicts /*don't change world*/)
		return;
	SV_LinkEdict(ent);
}

/*
=================
PFSV_unlinkentity
linkentity(entity)
=================
*/
void PFSV_unlinkentity(void)
{
	gentity_t* ent;
	ent = Scr_GetParmEdict(0);
	if (!ent->inuse || ent == sv.edicts /*don't change world*/)
		return;
	SV_UnlinkEdict(ent);
}

/*
=================
PFSV_PointContents
contents pointcontents(vector)
=================
*/
void PFSV_contents(void)
{
	float* point = Scr_GetParmVector(0);
	Scr_ReturnFloat( SV_PointContents(point) );
}

/*
=================
PFSV_trace

Moves the given mins/maxs volume through the world from start to end.
ignoreEnt and entities owned by ignoreEnt are explicitly not checked.

trace(start, min, max, end, ignoreEnt, contentmask)
=================
*/
void PFSV_trace(void)
{
	trace_t		trace;
	float		*start, *end, *min, *max;
	gentity_t	*ignoreEnt;
	int			contentmask;

	start = Scr_GetParmVector(0);
	min = Scr_GetParmVector(1);
	max = Scr_GetParmVector(2);
	end = Scr_GetParmVector(3);

	ignoreEnt = Scr_GetParmEdict(4);
	contentmask = Scr_GetParmInt(5);

	trace = SV_Trace(start, min, max, end, ignoreEnt, contentmask);

	// set globals in progs
	sv.script_globals->trace_allsolid = trace.allsolid;
	sv.script_globals->trace_startsolid = trace.startsolid;
	sv.script_globals->trace_fraction = trace.fraction;
	sv.script_globals->trace_plane_dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, sv.script_globals->trace_plane_normal);
	VectorCopy(trace.endpos, sv.script_globals->trace_endpos);
	sv.script_globals->trace_ent = (trace.ent == NULL ? GENT_TO_PROG(sv.edicts) : GENT_TO_PROG(trace.ent));
	sv.script_globals->trace_entnum = (trace.ent == NULL ? -1 : trace.ent->s.number);
	sv.script_globals->trace_contents = trace.contents;

	if (trace.surface)
	{
		sv.script_globals->trace_surface_name = Scr_SetString(trace.surface->name);
		sv.script_globals->trace_surface_flags = trace.surface->flags;
		sv.script_globals->trace_surface_value = trace.surface->value;
	}
	else
	{
		sv.script_globals->trace_surface_name = Scr_SetString("");
		sv.script_globals->trace_surface_flags = 0;
		sv.script_globals->trace_surface_value = 0;
	}
}

// =================================================================================

/*
=================
PFSV_sound
playsound(vector pos, entity ent, float channel, float sound_num, float volume, float attenuation, float timeOffset)

Each entity can have eight independant sound sources, like voice, weapon, feet, etc.
If (channel & 8), the sound will be sent to everyone, not just things in the PHS.
Channel 0 (CHAN_AUTO) is an auto-allocate channel, the others override anything already running on that entity/channel pair.
An attenuation of 0 (ATTN_NONE) will play full volume everywhere in the level. Larger attenuations will drop off (max ATTN_STATIC)
Timeofs can range from 0.0 to 0.255 to cause sounds to be started later in the frame than they normally would.
If origin is [0,0,0], the origin is determined from the entity origin or the midpoint of the entity box for bmodels.

FIXME: if entity isn't in PHS, they must be forced to be sent or have the origin explicitly sent.
=================
*/
void PFSV_sound(void)
{
	gentity_t* ent;
	int channel, sound_num;
	float volume, attenuation, timeofs;
	float* pos;
	
	ent = Scr_GetParmEdict(1);
	if (!ent->inuse)
		return;

	pos = Scr_GetParmVector(0);
	channel = (int)Scr_GetParmFloat(2);
	sound_num = SV_SoundIndex(Scr_GetParmString(3)); // todo: maybe revert string to index for better perf?
	volume = Scr_GetParmFloat(4);
	attenuation = Scr_GetParmFloat(5);
	timeofs = Scr_GetParmFloat(6);

	if( (int)pos[0] == 0 && (int)pos[1] == 0 && (int)pos[2] == 0) // this is quite wacky..
		SV_StartSound(NULL, ent, channel, sound_num, volume, attenuation, timeofs);
	else
		SV_StartSound(pos, ent, channel, sound_num, volume, attenuation, timeofs);
}

// =================================================================================

/*
==============
SetAreaPortalState(portal,state)
==============
*/
void PFSV_SetAreaPortalState(void)
{
	CM_SetAreaPortalState((int)Scr_GetParmFloat(0), (int)Scr_GetParmFloat(1));
}

/*
==============
float AreasConnected(area1,area2)
==============
*/
void PFSV_AreasConnected(void)
{
	Scr_ReturnFloat( CM_AreasConnected((int)Scr_GetParmFloat(0), (int)Scr_GetParmFloat(1)) );
}

/*
=================
PFSV_inPVS

Checks if two points are in PVS, lso checks portalareas so that doors block sight
float inPVS(vector,vector)
=================
*/
void PFSV_inPVS(void)
{
	float	*p1, *p2;
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	p1 = Scr_GetParmVector(0);
	p2 = Scr_GetParmVector(1);

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		Scr_ReturnFloat(0);
		return;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		// a door blocks sight
		Scr_ReturnFloat(0);
		return;
	}
	Scr_ReturnFloat(1);
}

/*
=================
PF_inPHS

Checks if two points are in PHS, also checks portalareas so that doors block sound

float inPHS(vector, vector)
=================
*/
void PFSV_inPHS(void)
{
	float	*p1, *p2;
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	p1 = Scr_GetParmVector(0);
	p2 = Scr_GetParmVector(1);

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPHS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);

	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		// more than one bounce away
		Scr_ReturnFloat(0);
		return;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		// a door blocks hearing
		Scr_ReturnFloat(0);
		return;
	}
	Scr_ReturnFloat(1);
}

// =================================================================================

static void SV_Configstring(int index, char *val) //move to sv_main
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		Com_Error(ERR_DROP, "configstring(): bad index %i\n", index);
		return;
	}

	if (!val)
		val = "";

	strcpy(sv.configstrings[index], val); // change the string in sv

	if (sv.state != ss_loading)
	{
		SZ_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, SVC_CONFIGSTRING);
		MSG_WriteShort(&sv.multicast, index);
		MSG_WriteString(&sv.multicast, val);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R); // send the update to everyone
	}
}

/*
=================
PFSV_Configstring
Sets the server's configstring and sends update to all clients
configstring(key,value)
=================
*/
void PFSV_configstring(void)
{
	int		index;
	char	*val;

	index = (int)Scr_GetParmFloat(0);
	val = Scr_GetParmString(1);

	SV_Configstring(index, val);
}


/*
=================
PFSV_lightstyle
Sets the lightstyle animation string where 'a' is total darkness, 'm' is fullbright, 'z' is double bright
lightstyle(num,style)
=================
*/
void PFSV_lightstyle(void)
{
	int		style;
	char	*val;

	style = (int)Scr_GetParmFloat(0);
	val = Scr_GetParmString(1);

	if (style < 0 || style >= MAX_LIGHTSTYLES)
	{
		Scr_RunError("lightstyle(): style ID must be in range [0-%i] but is %i\n", MAX_LIGHTSTYLES, style);
		return;
	}

	if (strlen(val) >= MAX_QPATH) // MAX_QPATH is the maximum length of configstring, if thats changed it must be updated here too
	{
		Scr_RunError("lightstyle(): style is too long (max is %i)\n", MAX_QPATH);
		return;
	}
	SV_Configstring(CS_LIGHTS + style, val);
}

// =================================================================================

static void MSG_Unicast(gentity_t* ent, qboolean reliable)
{
	int			entnum;
	client_t* client;

	entnum = NUM_FOR_EDICT(ent);
	if (entnum < 1 || entnum > sv_maxclients->value)
	{
		Com_Error(ERR_DROP, "MSG_Unicast() to a non-client entity %i\n", entnum);
		return;
	}

	client = svs.clients + (entnum - 1);
	if (reliable)
		SZ_Write(&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
	else
		SZ_Write(&client->datagram, sv.multicast.data, sv.multicast.cursize);

	SZ_Clear(&sv.multicast);
}

/*
===============
PFSV_unicast

Sends the contents of the mutlicast buffer to a single client
unicast(entity,float)
===============
*/
void PFSV_Unicast(void)
{
	gentity_t	*ent;
	float		reliable;

	ent = Scr_GetParmEdict(0);
	reliable = Scr_GetParmFloat(1) > 0 ? true : false;

	MSG_Unicast(ent, reliable);
}

/*
===============
PFSV_multicast

Sends the contents of sv.multicast to a subset of the clients, then clears sv.multicast.
multicast(vector,float)

MULTICAST_ALL		broadcast to everyone on server (origin can be NULL)
MULTICAST_PVS		send to clients potentially visible from org
MULTICAST_PHS		send to clients potentially hearable from org
MULTICAST_ALL_R		same as MULTICAST_ALL but reliable (when you want all clients to receive message)
MULTICAST_PHS_R		same as MULTICAST_PHS but reliable
MULTICAST_PVS_R		same as MULTICAST_pvs but reliable
===============
*/
void PFSV_Multicast(void)
{
	float			*pos;
	multicast_t		sendTo;

	pos = Scr_GetParmVector(0);
	sendTo = (int)Scr_GetParmFloat(1);

	if (sendTo < MULTICAST_ALL || sendTo > MULTICAST_PVS_R)
	{
		Scr_RunError("multicast() sendTo %i is out of range [MULTICAST_ALL-MULTICAST_PVS_R]\n", sendTo);
		return;
	}
	SV_Multicast(pos, sendTo);
}

void PFSV_WriteChar(void)		{ MSG_WriteChar(&sv.multicast,		(int)Scr_GetParmString(0)[0]); }
void PFSV_WriteByte(void)		{ MSG_WriteByte(&sv.multicast,		(int)Scr_GetParmFloat(0)); }
void PFSV_WriteShort(void)		{ MSG_WriteShort(&sv.multicast,		(int)Scr_GetParmFloat(0)); }
void PFSV_WriteLong(void)		{ MSG_WriteLong(&sv.multicast,		(int)Scr_GetParmFloat(0)); }
void PFSV_WriteFloat(void)		{ MSG_WriteFloat(&sv.multicast,		Scr_GetParmFloat(0)); }
void PFSV_WriteString(void)		{ MSG_WriteString(&sv.multicast,	Scr_GetParmString(0)); }
void PFSV_WritePos(void)		{ MSG_WritePos(&sv.multicast,		Scr_GetParmVector(0)); }
void PFSV_WriteDir(void)		{ MSG_WriteDir(&sv.multicast,		Scr_GetParmVector(0)); }
void PFSV_WriteAngle(void)		{ MSG_WriteAngle(&sv.multicast,		Scr_GetParmFloat(0)); }

// =================================================================================

/*
===============
PFSV_stuffcmd

Send reliable string directly to clients' command execution buffer, if target entity is world we send to all clients
stuffcmd(entity, string, ...)
===============
*/
void PFSV_stuffcmd(void)
{
	gentity_t	*ent;
	char		*cmd;
	int			entnum;

	ent = Scr_GetParmEdict(0);
	cmd = Scr_VarString(1);

	entnum = NUM_FOR_EDICT(ent);
	if (entnum != 0 && (entnum < 1 || entnum > sv_maxclients->value))
	{
		Scr_RunError("stuffcmd(%s) to a non-client entity %i\n", cmd, entnum);
		return;
	}

	MSG_WriteByte(&sv.multicast, SVC_STUFFTEXT);
	MSG_WriteString(&sv.multicast, va("%s\n", cmd));

	if(entnum == 0)
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	else
		MSG_Unicast(ent, true);
	
}
// =================================================================================

/*
===============
PFSV_cprint

Print message to a single client
cprint(entity, float printlevel, string, ...)
===============
*/
void PFSV_sprint(void)
{
	gentity_t	*ent;
	char		*msg;
	int			entnum, printlevel;

	ent = Scr_GetParmEdict(0);
	printlevel = Scr_GetParmFloat(1);
	msg = Scr_VarString(2);

	entnum = NUM_FOR_EDICT(ent);
	if (entnum < 1 || entnum > sv_maxclients->value)
	{
		Scr_RunError("sprint() to a non-client entity %i\n", entnum);
		return;
	}
	SV_ClientPrintf(svs.clients + (entnum - 1), printlevel, "%s", msg);
}

/*
===============
PFSV_bprint
Print message to a all clients
bprint(float printlevel, string, ...)
===============
*/
void PFSV_bprint(void)
{
	SV_BroadcastPrintf(Scr_GetParmFloat(0), "%s", Scr_VarString(1));
}

/*
===============
PFSV_centerprint

Center print message to a single client, if entity is world broadcast to everyone
centerprint(entity, string, ...)
===============
*/
void PFSV_centerprint(void)
{
	gentity_t	*ent;
	char		*msg;
	int			entnum;

	ent = Scr_GetParmEdict(0);
	msg = Scr_VarString(1);

	entnum = NUM_FOR_EDICT(ent);
	if (entnum != 0 && (entnum < 1 || entnum > sv_maxclients->value))
	{
		Scr_RunError("centerprint() to a non-client entity %i\n", entnum);
		return;
	}

	MSG_WriteByte(&sv.multicast, SVC_CENTERPRINT);
	MSG_WriteString(&sv.multicast, msg);

	if (entnum == 0)
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	else
		MSG_Unicast(ent, true);
}


/*
===============
PFSV_isplayer

returns true if entity is player
float isplayer(entity)
===============
*/
void PFSV_isplayer(void)
{
	gentity_t* ent;
	ent = Scr_GetParmEdict(0);
	Scr_ReturnFloat(ent->client == NULL ? 0 : 1);
}

/*
===============
PFSV_setviewmodel

Sets view model for player, zeroes view model frame, angles and offset
returns model index

setviewmodel(entity, string)
===============
*/
void PFSV_setviewmodel(void)
{
	gentity_t	*ent;
	char		*model;

	ent = Scr_GetParmEdict(0);
	model = Scr_GetParmString(1);

	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setviewmodel(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}

	if (model == "")
		ent->client->ps.viewmodel_index = 0;
	else
		ent->client->ps.viewmodel_index = SV_ModelIndex(model);

	ent->client->ps.viewmodel_frame = 0;
	VectorClear(ent->client->ps.viewmodel_angles);
	VectorClear(ent->client->ps.viewmodel_offset);
	Scr_ReturnFloat(ent->client->ps.viewmodel_index);
}

/*
===============
PFSV_setviewmodelparms

Sets view model frame, angles and offset

setviewmodelparms(entity, frame, angles, offset)
===============
*/
void PFSV_setviewmodelparms(void)
{
	gentity_t* ent;
	gclient_t* cl;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setviewmodelparms(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;
	if(cl->ps.viewmodel_index == 0)
	{
		Scr_RunError("setviewmodelparms(): client %i has no viewmodel set\n", NUM_FOR_EDICT(ent));
		return;
	}

	cl->ps.viewmodel_frame = Scr_GetParmFloat(1);
	VectorCopy(Scr_GetParmVector(2), cl->ps.viewmodel_angles);
	VectorCopy(Scr_GetParmVector(3), cl->ps.viewmodel_offset);
}

/*
===============
PFSV_setfieldofview

Sets players fov

setfieldofview(entity, float)
===============
*/
void PFSV_setfieldofview(void)
{
	gentity_t* ent;
	gclient_t* cl;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setfieldofview(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;
	cl->ps.fov = Scr_GetParmFloat(1);
	if (cl->ps.fov <= 0)
		cl->ps.fov = 10;
}

/*
===============
PFSV_getfieldofview

returns players fov

float getfieldofview(entity)
===============
*/
void PFSV_getfieldofview(void)
{
	gentity_t* ent;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("getfieldofview(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	Scr_ReturnFloat(ent->client->ps.fov);
}

/*
===============
PFSV_setviewblend

sets players screen blend colors

setviewblend(entity, vector rgb, float a)
===============
*/
void PFSV_setviewblend(void)
{
	gentity_t* ent;
	gclient_t* cl;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setviewblend(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;
	cl->ps.blend[0] = Scr_GetParmVector(1)[0];
	cl->ps.blend[1] = Scr_GetParmVector(1)[1];
	cl->ps.blend[2] = Scr_GetParmVector(1)[2];
	cl->ps.blend[3] = Scr_GetParmFloat(2);
}

/*
===============
PFSV_setviewoffset

setviewoffset(entity, vector)
===============
*/
void PFSV_setviewoffset(void)
{
	gentity_t* ent;
	gclient_t* cl;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setviewoffset(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;
	VectorCopy(Scr_GetParmVector(1), cl->ps.viewoffset);
}


/*
===============
PFSV_getviewoffset

vector getviewoffset(entity)
===============
*/
void PFSV_getviewoffset(void)
{
	gentity_t* ent;
	gclient_t* cl;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("getviewoffset(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;
	Scr_ReturnVector(cl->ps.viewoffset);
}


/*
===============
PFSV_saveclientfield

void saveclientfield(entity player, float index, float val)
===============
*/
void PFSV_saveclientfield(void)
{
	gentity_t* ent;
	gclient_t* cl;
	int idx;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("saveclientfield(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;

	idx = Scr_GetParmFloat(1);
	if (idx < 0 || idx >= 32)
	{
		Scr_RunError("saveclientfield(): index %i is invaild\n", idx);
		return;
	}
	cl->pers.saved[idx] = Scr_GetParmFloat(2);
}


/*
===============
PFSV_loadclientfield

float loadclientfield(entity player, float index)
===============
*/
void PFSV_loadclientfield(void)
{
	gentity_t* ent;
	gclient_t* cl;
	int idx;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("loadclientfield(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;

	idx = (int)Scr_GetParmFloat(1);
	if (idx < 0 || idx >= MAX_PERS_FIELDS)
	{
		Scr_RunError("loadclientfield(): index %i is invaild\n", idx);
		return;

	}
	Scr_ReturnFloat(cl->pers.saved[idx]);
}

/*
===============
PFSV_saveglobal

void saveglobal(float index, float val)
===============
*/
void PFSV_saveglobal(void)
{
	int idx;
	idx = Scr_GetParmFloat(0);
	if (idx < 0 || idx >= MAX_PERS_FIELDS)
	{
		Scr_RunError("saveglobal(): index %i is invaild\n", idx);
		return;
	}
	svs.saved[idx] = Scr_GetParmFloat(1);
}


/*
===============
PFSV_loadglobal

float loadglobal(float index)
===============
*/
void PFSV_loadglobal(void)
{
	int idx;

	idx = Scr_GetParmFloat(0);
	if (idx < 0 || idx >= MAX_PERS_FIELDS)
	{
		Scr_RunError("loadglobal(): index %i is invaild\n", idx);
		return;

	}
	Scr_ReturnFloat(svs.saved[idx]);
}

/*
===============
PFSV_changemap

float changemap(string nextmap, float savepers)
===============
*/
void PFSV_changemap(void)
{
	char	*nextmap;
	char	expanded[MAX_QPATH];

	nextmap = Scr_GetParmString(0);

	// make sure the map exists on server, return false to script if it 
	// doesn't so server can try to recover and pick a diferent map at last
	if (!strstr(nextmap, "."))
	{
		Com_sprintf(expanded, sizeof(expanded), "maps/%s.bsp", nextmap);
		if (FS_LoadFile(expanded, NULL) == -1)
		{
			Com_Printf("changemap(): can't change map to %s, map not on the server\n", expanded);
			Scr_ReturnFloat(0);
			return;
		}
	}

	// not the best place to call it
	//	SV_Map(false, nextmap, false, Scr_GetParmFloat(1), Scr_GetParmFloat(2)); 

	if(Scr_GetParmFloat(1) > 0)
		Cbuf_ExecuteText(EXEC_APPEND, va("gamemap %s", nextmap));
	else
		Cbuf_ExecuteText(EXEC_APPEND, va("map %s", nextmap));

	Scr_ReturnFloat(1);

}

/*
===============
PFSV_saveglobal

void saveglobal(float index, float val)
===============
*/
void PFSV_setstat(void)
{
	gentity_t* ent;
	gclient_t* cl;
	int idx;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setstat(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	cl = ent->client;

	idx = Scr_GetParmFloat(1);
	if (idx < 0 || idx >= 32)
	{
		Scr_RunError("saveclientfield(): index %i is invaild\n", idx);
		return;
	}
	cl->ps.stats[idx] = Scr_GetParmFloat(2);
}

/*
===============
PFSV_pmove

void pmove(float index, float val)
===============
*/

gentity_t* pm_passent;
// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->v.health > 0)
		return SV_Trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return SV_Trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

extern usercmd_t* last_ucmd;
void PFSV_pmove(void)
{
	gentity_t* ent;
	gclient_t* client;
	float* inmove;
	float	allowCrouch;
	pmove_t	pm;
	int i;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("pmove(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	client = ent->client;

	inmove = Scr_GetParmVector(1);
	allowCrouch = Scr_GetParmFloat(2);

	// set up for pmove
	pm_passent = ent;
	memset(&pm, 0, sizeof(pm));

	client->ps.pmove.pm_type = ent->v.pm_type;
	client->ps.pmove.pm_flags = ent->v.pm_flags;
	client->ps.pmove.gravity = (sv_gravity->value * ent->v.gravity);

	pm.s = client->ps.pmove;
	for (i = 0; i < 3; i++)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		pm.s.origin[i] = ent->v.origin[i];
		pm.s.velocity[i] = ent->v.velocity[i];
#else
		pm.s.origin[i] = ent->v.origin[i] * 8;
		pm.s.velocity[i] = ent->v.velocity[i] * 8;
#endif
	}
	if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
	{
		pm.snapinitial = true;
		Com_Printf("pmove changed!\n");
	}

	// grab inMove changes from script
	last_ucmd->forwardmove = inmove[0];
	last_ucmd->sidemove = inmove[1];
	last_ucmd->upmove = inmove[2];

	VectorCopy(ent->v.mins, pm.mins);
	VectorCopy(ent->v.maxs, pm.maxs);

	pm.viewheight = ent->v.viewheight;

	pm.cmd = *last_ucmd;
	pm.trace = PM_trace;
	pm.pointcontents = SV_PointContents;
	Pmove(&pm); // perform a pmove

	// save results of pmove
	client->ps.pmove = pm.s;
	client->old_pmove = pm.s;

	for (i = 0; i < 3; i++)
	{
#if PROTOCOL_FLOAT_COORDS == 1
		ent->v.origin[i] = pm.s.origin[i];
		ent->v.velocity[i] = pm.s.velocity[i];
#else
		ent->v.origin[i] = pm.s.origin[i] * 0.125;
		ent->v.velocity[i] = pm.s.velocity[i] * 0.125;
#endif
	}

	VectorCopy(pm.mins, ent->v.mins);
	VectorCopy(pm.maxs, ent->v.maxs);

	ent->client->ps.viewoffset[2] = pm.viewheight;
	ent->v.viewheight = pm.viewheight;
	ent->v.waterlevel = pm.waterlevel;
	ent->v.watertype = pm.watertype;

	ent->v.pm_type = client->ps.pmove.pm_type;
	ent->v.pm_flags = client->ps.pmove.pm_flags;

	ent->groundentity = pm.groundentity;
	if (pm.groundentity)
		ent->groundentity_linkcount = pm.groundentity->linkcount;

	ent->v.groundEntityNum = (pm.groundentity == NULL ? -1 : pm.groundentity->s.number); 
	if (pm.groundentity)
		ent->v.groundEntity_linkcount = pm.groundentity->linkcount;

	VectorCopy(pm.viewangles, ent->v.v_angle);
	VectorCopy(pm.viewangles, client->ps.viewangles);

	SV_LinkEdict(ent);
}


/*
=================
SV_InitScriptBuiltins

Register server script functions
=================
*/
void SV_InitScriptBuiltins()
{
	// precache functions return index of loaded asset
	Scr_DefineBuiltin(PFSV_precache_model, PF_SV, false, "float(string n) precache_model");
	Scr_DefineBuiltin(PFSV_precache_sound, PF_SV, false, "float(string n) precache_sound");
	Scr_DefineBuiltin(PFSV_precache_image, PF_SV, false, "float(string n) precache_image");

	// entity general
	Scr_DefineBuiltin(PFSV_spawn, PF_SV, false, "entity() spawn");
	Scr_DefineBuiltin(PFSV_remove, PF_SV, false, "void(entity e) remove");

	Scr_DefineBuiltin(PFSV_getent, PF_SV, false, "entity(float idx) getent");
	Scr_DefineBuiltin(PFSV_nextent, PF_SV, false, "entity(entity prev) nextent");
	Scr_DefineBuiltin(PFSV_find, PF_SV, false, "entity(entity e, .string fld, string match) find");
	Scr_DefineBuiltin(PFSV_findradius, PF_SV, false, "entity(entity e, vector v, float r) findradius");
	Scr_DefineBuiltin(PFSV_getEntNum, PF_SV, false, "float(entity e) getentnum");

	Scr_DefineBuiltin(PFSV_setorigin, PF_SV, false, "void(entity e, vector v) setorigin");
	Scr_DefineBuiltin(PFSV_setmodel, PF_SV, false, "void(entity e, string s) setmodel");
	Scr_DefineBuiltin(PFSV_setsize, PF_SV, false, "void(entity e, vector v1, vector v2) setsize");
	Scr_DefineBuiltin(PFSV_linkentity, PF_SV, false, "void(entity e) linkentity");
	Scr_DefineBuiltin(PFSV_unlinkentity, PF_SV, false, "void(entity e) unlinkentity");

	// collision
	Scr_DefineBuiltin(PFSV_contents, PF_SV, false, "float(vector v) pointcontents");
	Scr_DefineBuiltin(PFSV_trace, PF_SV, false, "void(vector p1, vector v1, vector v2, vector p2, entity e, float c) trace");

	// sound
	Scr_DefineBuiltin(PFSV_sound, PF_SV, false, "void(vector v, entity e, float ch, string snd, float vol, float att, float tofs) playsound");

	// visibility and hearability
	Scr_DefineBuiltin(PFSV_SetAreaPortalState, PF_SV, false, "void(float a1, float a2) SetAreaPortalState");
	Scr_DefineBuiltin(PFSV_AreasConnected, PF_SV, false, "float(float a1, float a2) AreasConnected");
	Scr_DefineBuiltin(PFSV_inPVS, PF_SV, false, "float(vector v1, vector v2) inPVS");
	Scr_DefineBuiltin(PFSV_inPHS, PF_SV, false, "float(vector v1, vector v2) inPHS");

	//strings
	Scr_DefineBuiltin(PFSV_sprint, PF_SV, false, "void(entity e, float pl, string s) sprint"); // overloading strings supported
	Scr_DefineBuiltin(PFSV_bprint, PF_SV, false, "void(float pl, string s) bprint"); // overloading strings supported
	Scr_DefineBuiltin(PFSV_centerprint, PF_SV, false, "void(entity e, string s) centerprint"); // overloading strings supported

	//configstrings and lightstyles
	Scr_DefineBuiltin(PFSV_configstring, PF_SV, false, "void(float  k, string v) configstring");
	Scr_DefineBuiltin(PFSV_lightstyle, PF_SV, false, "void(float s, string v) lightstyle");

	// network messages
	Scr_DefineBuiltin(PFSV_Unicast, PF_SV, false, "void(entity e, float r) MSG_Unicast");
	Scr_DefineBuiltin(PFSV_Multicast, PF_SV, false, "void(vector v, float to) MSG_Multicast");
	Scr_DefineBuiltin(PFSV_WriteChar, PF_SV, false, "void(string v) MSG_WriteChar");
	Scr_DefineBuiltin(PFSV_WriteByte, PF_SV, false, "void(float v) MSG_WriteByte");
	Scr_DefineBuiltin(PFSV_WriteShort, PF_SV, false, "void(float v) MSG_WriteShort");
	Scr_DefineBuiltin(PFSV_WriteShort, PF_SV, false, "void(float v) MSG_WriteLong");
	Scr_DefineBuiltin(PFSV_WriteString, PF_SV, false, "void(string v) MSG_WriteString");
	Scr_DefineBuiltin(PFSV_WritePos, PF_SV, false, "void(vector v) MSG_WritePos");
	Scr_DefineBuiltin(PFSV_WriteDir, PF_SV, false, "void(vector v) MSG_WriteDir");
	Scr_DefineBuiltin(PFSV_WriteAngle, PF_SV, false, "void(float v) MSG_WriteAngle");

	Scr_DefineBuiltin(PFSV_stuffcmd, PF_SV, false, "void(entity e, string s) stuffcmd"); // overloading strings supported

	// client
	Scr_DefineBuiltin(PFSV_isplayer, PF_SV, false, "float(entity e) isplayer");
	Scr_DefineBuiltin(PFSV_setviewmodel, PF_SV, false, "float(entity e, string s) setviewmodel");
	Scr_DefineBuiltin(PFSV_setviewmodelparms, PF_SV, false, "void(entity e, float f, vector v1, vector v2) setviewmodelparms");
	Scr_DefineBuiltin(PFSV_setfieldofview, PF_SV, false, "void(entity e, float f) setfieldofview");
	Scr_DefineBuiltin(PFSV_getfieldofview, PF_SV, false, "float(entity e) getfieldofview");
	Scr_DefineBuiltin(PFSV_setviewblend, PF_SV, false, "void(entity e, vector v1, float f) setviewblend");
	Scr_DefineBuiltin(PFSV_setviewoffset, PF_SV, false, "void(entity e, vector v1) setviewoffset");
	Scr_DefineBuiltin(PFSV_getviewoffset, PF_SV, false, "vector(entity e) getviewoffset");

	// persistant data across map changes
	Scr_DefineBuiltin(PFSV_saveclientfield, PF_SV, false, "void(entity p, float idx, float val) saveclientfield");
	Scr_DefineBuiltin(PFSV_loadclientfield, PF_SV, false, "float(entity player, float index) loadclientfield");
	Scr_DefineBuiltin(PFSV_saveglobal, PF_SV, false, "void(float idx, float val) saveglobal");
	Scr_DefineBuiltin(PFSV_loadglobal, PF_SV, false, "float(float index) loadglobal");

	Scr_DefineBuiltin(PFSV_changemap, PF_SV, false, "float(string nm, float sp) changemap");

	Scr_DefineBuiltin(PFSV_setstat, PF_SV, false, "float(entity e, float sg, float sc) setstat");
	Scr_DefineBuiltin(PFSV_pmove, PF_SV, false, "float(entity e, float sg, float sc) pmove");
}
