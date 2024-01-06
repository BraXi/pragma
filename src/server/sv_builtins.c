/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#include "server.h"

// =================================================================================

static void CheckEmptyString(char* s)  // definitely need to make it a shared code...
{
	if (s[0] <= ' ')
		Scr_RunError("Bad string");
}

/*
=================
PFSV_AngleVectors
=================
*/
void PFSV_AngleVectors(void)
{
	AngleVectors(Scr_GetParmVector(0), sv.script_globals->v_forward, sv.script_globals->v_right, sv.script_globals->v_up);
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
	gentity_t* ent = Scr_GetParmEdict(0);
	if (ent && ent->inuse)
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
		ent = sv.edicts;
		Com_Printf("getent(): %i is not in use\n", entnum);
	}

	Scr_ReturnEntity(ent);
}

/*
=================
PFSV_nextent

finds next active entity, returns world if no entity found
entity nextent(entity previousEnt)

entity firstplayer = nextent(world);
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

not implemented, ugh I forgot
=================
*/
void PFSV_find(void)
{
	Scr_ReturnEntity(sv.edicts);
}


/*
=================
findradius

Returns entities that have origins within a spherical area

findradius(entity from, origin, radius)
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

float num = entnum(self);
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

void setorigin(entity ent, vector origin)

setorigin(player, spawnpoint.origin);
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

Sets entity's model (modelindex0)

void setmodel(entity ent, string modelname)

setmodel(player, "models/characters/paula.md3");
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
	i = SV_ModelIndex(name);
	ent->v.model = Scr_SetString(name);
	ent->v.modelindex = i;

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

void setsize(entity ent, vector mins, vector maxs)

setsize(player, '-16 16 0', '16 16 56');
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

Links entity to the interaction links, should be called when 
entities bbox or solidity changes (setmodel and setsize do this)

void linkentity(entity ent)
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

unlink entity from world, unlinked solid entity 
will not be collidable unless linked again.

void linkentity(entity ent)
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
float contents = pointcontents(vector point)
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

void trace(vector start, vector mins, vector maxs, vector end, entity ignoreEnt, int contentmask)

trace(self.origin, self.mins, self.maxs, self.origin - '0 0 256', self, MASK_PLAYERSOLID);
setorigin(self, trace_endpos);
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

void playsound(vector pos, entity ent, float channel, string soundFileName, float volume, float attenuation, float timeOffset)

Each entity can have eight independant sound sources, like voice, weapon, feet, etc.
If (channel & 8), the sound will be sent to everyone, not just things in the PHS.
Channel 0 (CHAN_AUTO) is an auto-allocate channel, the others override anything already running on that entity/channel pair.
An attenuation of 0 (ATTN_NONE) will play full volume everywhere in the level. Larger attenuations will drop off (max ATTN_STATIC)
Timeofs can range from 0.0 to 0.255 to cause sounds to be started later in the frame than they normally would.
If origin is [0,0,0], the origin is determined from the entity origin or the midpoint of the entity box for bmodels.

FIXME: if entity isn't in PHS, they must be forced to be sent or have the origin explicitly sent.

playsound(vec3_origin, player, CHAN_WEAPON, "weapons/noammoclick.wav", 0.5, ATTN_NORM, 0);
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


/*
=================
PFSV_stopsounds

void stopsounds(entity ent)

Cancel all sounds which are played on an entity
this also removes looping sound
=================
*/
void PFSV_stopsounds(void)
{
	gentity_t* ent;
	ent = Scr_GetParmEdict(0);
	if (!ent->inuse)
		return;

	SV_StopSounds(ent);
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

Checks if two points are in PVS, also checks portalareas so that doors block sight

float inPVS(vecto p1r, vector p1)

float canPotentialySeeEachOther = inPVS(player.origin, monster.origin);
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

float inPHS(vector p1, vector p1)

float canPotentialyHearEachOther = inPHS(player.origin, monster.origin);
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

/*
=================
PFSV_Configstring
Sets the server's configstring and sends update to all clients

void configstring(float csindex, string value)

configstring(CS_SKY, "cloudynight");
=================
*/
void PFSV_configstring(void)
{
	int		index;
	char	*val;

	index = (int)Scr_GetParmFloat(0);
	val = Scr_GetParmString(1);

	SV_SetConfigString(index, val);
}


/*
=================
PFSV_lightstyle

Sets the lightstyle animation string where 'a' is total darkness, 'm' is fullbright, 'z' is double bright

void lightstyle(float lightStyleIndex, string lightStyle)

lightstyle(0, "aamm"); // flickering world lighting!
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
	SV_SetConfigString(CS_LIGHTS + style, val);
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

Sends the contents of the mutlicast buffer to a single client,
if 2nd argument is true message will be sent as reliable.

void unicast(entity who, float isReliable)

unicast(player, true);
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

Sends the contents of sv.multicast which were written with MSG_Write* 
functions to a subset of the clients, then clears sv.multicast.

void multicast(vector position, float sendto)

sendto:
MULTICAST_ALL		broadcast to everyone on server (origin can be NULL)
MULTICAST_PVS		send to clients potentially visible from org
MULTICAST_PHS		send to clients potentially hearable from org
MULTICAST_ALL_R		same as MULTICAST_ALL but reliable (when you want all clients to receive message)
MULTICAST_PHS_R		same as MULTICAST_PHS but reliable
MULTICAST_PVS_R		same as MULTICAST_pvs but reliable


multicast(monster.origin, MULTICAST_PVS);
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

Send reliable string directly to clients' command execution 
buffer, if target entity is world we send to all clients

void stuffcmd(entity who, string text, ...)

stuffcmd(player, "echo redirecting to another server; wait 10; disconnect; wait 1; connect 192.168.0.1:27000");
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
void cprint(entity who, float printlevel, string text, ...)

cprint(player, PRINT_HIGH, "Picked up big pack of ammo!");
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

void bprint(float printlevel, string text, ...)
bprint(PRINT_CHAT, "This string is printed to chat for everyone");
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

void centerprint(entity who, string text, ...)

centerprint(player, "hello", " world!");
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

	if (entnum > sv_maxclients->value || entnum < 0)
	{
		Scr_RunError("centerprint() to a non-client entity %i\n", entnum);
		return;
	}

	if (!ent->inuse)
	{
		Scr_RunError("centerprint() to unused entity %i\n", entnum);
		return;
	}


	MSG_WriteByte(&sv.multicast, SVC_CENTERPRINT);
	MSG_WriteString(&sv.multicast, msg);

	if (entnum == 0) // send to all
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	else // send to a single client
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

Sets view (gun, etc) model for player, if new model is diferent to old it
will reset view model frame, angles and offset
returns current view model index

float newviewmodelindex = setviewmodel(entity player, string viewModelName)
===============
*/
void PFSV_setviewmodel(void)
{
	gentity_t* ent;
	char* model;
	int			newmodelindex;

	ent = Scr_GetParmEdict(0);
	model = Scr_GetParmString(1);

	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setviewmodel(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}

	newmodelindex = SV_ModelIndex(model);

	if (ent->client->ps.viewmodel_index != newmodelindex)
	{
		ent->client->ps.viewmodel_index = newmodelindex;
		ent->client->ps.viewmodel_frame = 0;
		VectorClear(ent->client->ps.viewmodel_angles);
		VectorClear(ent->client->ps.viewmodel_offset);
	}

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

saves persistent client data across map changes

void saveclientfield(entity player, float index, float val)

saveclientfield(self, PS_HEALTH, self.health);
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

loads persistent data

float loadclientfield(entity player, float index)

self.health = loadclientfield(self, PS_HEALTH);
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

Starts a new map, set savepers to true to keep persistent client/globals 
fields across levels. Returns true if the bsp 'maps/[nextmap].bsp' exists on server.

float mapexists = changemap("test", true);
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
		Cbuf_ExecuteText(EXEC_APPEND, va("gamemap %s\n", nextmap));
	else
		Cbuf_ExecuteText(EXEC_APPEND, va("map %s\n", nextmap));

	Scr_ReturnFloat(1);
}

/*
===============
PFSV_kickclient

void kickclient(entity player)

Kick client out from server
===============
*/
void PFSV_kickclient(void)
{
	gentity_t* ent;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("kickclient(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	SV_BroadcastPrintf(PRINT_LOW, "%s was kicked from server.", ent->client->pers.netname);
	SV_DropClient(ent->client);
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
PFSV_setviewangles

void setviewangles(entity player, vector viewAngles)
===============
*/
void PFSV_setviewangles(void)
{
	gentity_t* ent;
	gclient_t* client;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("setviewangles(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	client = ent->client;

	VectorCopy(Scr_GetParmVector(1), client->ps.viewangles);
	VectorCopy(client->ps.viewangles, ent->v.v_angle);
}

/*
===============
PFSV_kickangles

void kickangles(entity player, vector kickDirAndForce)
===============
*/
void PFSV_kickangles(void)
{
	gentity_t* ent;
	gclient_t* client;

	ent = Scr_GetParmEdict(0);
	if (!ent->client || ent->client->pers.connected == false)
	{
		Scr_RunError("kickangles(): on non-client entity %i\n", NUM_FOR_EDICT(ent));
		return;
	}
	client = ent->client;

	VectorCopy(Scr_GetParmVector(1), client->ps.kick_angles);
}


/*
===============
PFSV_pmove

deprecated, subject to remove

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
	float	copytopmove;
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
	copytopmove = Scr_GetParmFloat(2);


	if (copytopmove)
	{
		return;
	}

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



	gentity_t* ground = pm.groundentity;

	ent->v.groundentity_num = (pm.groundentity == NULL ? -1 : ground->s.number);

	if (ground)
		ent->v.groundentity_linkcount = ground->linkcount;


	VectorCopy(pm.viewangles, ent->v.v_angle);
	VectorCopy(pm.viewangles, client->ps.viewangles);

	SV_LinkEdict(ent);
}

/*
=================
PFSV_checkbottom

Returns false if any part of the bottom of the entity is off an edge that is not a staircase.

float isonground = checkbottom(self);
=================
*/
extern qboolean SV_CheckBottom(gentity_t* actor);
void PFSV_checkbottom(void)
{
	gentity_t* ent;

	ent = Scr_GetParmEdict(0);
	if (ent == sv.edicts)
	{
		Scr_RunError("checkbottom(): on world entity\n");
		return;
	}

	Scr_ReturnFloat(SV_CheckBottom(ent));
}

/*
=================
PFSV_movetogoal

float closedist = movetogoal(entity actor, entity goal, float moveDistance);

Searches for a path and moves entity [moveDistance] units towards goal
entity, returns true if the next step hits the enemy, otherwise false.
The move will be adjusted for slopes and stairs.
updates entity's goal_entity to a new goal entity

float closetoEnemy = movetogoal(self, self.goal_entity, 15);
=================
*/
extern qboolean SV_MoveToGoal(gentity_t* actor, gentity_t* goal, float dist);
void PFSV_movetogoal(void)
{
	gentity_t	*actor, *goal;
	float		movedist, result;

	actor = Scr_GetParmEdict(0);
	goal = Scr_GetParmEdict(1);
	movedist = Scr_GetParmFloat(2);

	if (actor == sv.edicts)
	{
		Scr_RunError("MoveToGoal called for world!\n");
		Scr_ReturnFloat(0.0f);
		return;
	}
	if (goal == sv.edicts)
	{
		Scr_RunError("MoveToGoal aborted because goal is world for entity %i\n", NUM_FOR_ENT(actor));
		Scr_ReturnFloat(0.0f);
		return;
	}
	actor->v.goal_entity = ENT_TO_VM(goal);

	result = SV_MoveToGoal(actor, goal, movedist) == true ? 1.0f : 0.0f;
	Scr_ReturnFloat(result);
}


/*
=================
PFSV_walkmove

walkmove(entity actor, float yaw, float moveDistance)

Searches for a path and moves entity [moveDistance] units in yaw direction of [yaw]
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned

float moved = walkmove(self, self.ideal_yaw, 15);
=================
*/
extern qboolean SV_WalkMove(gentity_t* actor, float yaw, float dist);
void PFSV_walkmove(void)
{
	gentity_t	*actor;
	float		movedist, yaw, result;

	actor = Scr_GetParmEdict(0);
	yaw = Scr_GetParmFloat(2);
	movedist = Scr_GetParmFloat(2);

	if (actor == sv.edicts)
	{
		Scr_RunError("WalkMove called for world!\n");
		Scr_ReturnFloat(0.0f);
		return;
	}

	result = SV_WalkMove(actor, yaw, movedist) == true ? 1.0f : 0.0f;
	Scr_ReturnFloat(result);
}


/*
=================
PFSV_touchentities

float touchentities(entity ent, float areatype)

Finds entities directly intersecting [ent] bounding box and calls their touch functions.
areatype defines the type of entities we want to touch, this can either 
be 0 for triggers and 1 for solids returns the number of touched entities
Returns the number of touched entities. THIS DOES DIFFER FROM Q2's!
In Q2 only players and alive SVF_MONSTER entities could touch, in pragma you have to
check for that yourself, it was: if((isplayer(ent) || (ent.svflags & SVF_MONSTER)) && (ent.health <= 0))

float numTouchedEntities = touchentities(self, 0); // touch triggers
=================
*/
void PFSV_touchentities(void)
{
	gentity_t* ent;
	int areatype, numtouched;

	ent = Scr_GetParmEdict(0);

	if (ent == sv.edicts)
	{
		Scr_RunError("touchentities() called for world!\n");
		Scr_ReturnFloat(0);
		return;
	}

	if (Scr_GetParmFloat(1) == 0)
		areatype = AREA_TRIGGERS;
	else //cba to write another error message
		areatype = AREA_SOLID;

	numtouched = SV_TouchEntities(ent, areatype);
	Scr_ReturnFloat(numtouched);
}


/*
=================
PFSV_drawline

void drawline(vector p1, vector p2, vector color, float thickness, float depthTest, float drawtime)

Draws a debug line between two points, does not work on dedicated servers
For listen servers only the host will see the lines.

drawline(self.origin, self.goal_entity.origin, '1 0 0', 1, true, g_frameTime);
=================
*/
extern void SV_AddDebugLine(vec3_t p1, vec3_t p2, vec3_t color, float thickness, float drawtime, qboolean depthtested);
void PFSV_drawline(void)
{
	float	*p1, *p2, *color;
	float	thickness, drawtime;
	qboolean depthtested;

	if(dedicated->value)
		return;

	//	if (sv_maxclients->value > 1)
	//		return;

	p1 = Scr_GetParmVector(0);
	p2 = Scr_GetParmVector(1);
	color = Scr_GetParmVector(2);
	thickness = Scr_GetParmFloat(3);
	depthtested = Scr_GetParmFloat(4) > 0 ? true : false;
	drawtime = Scr_GetParmFloat(5);

	SV_AddDebugLine(p1, p2,color, thickness, drawtime, depthtested);
}

/*
=================
PFSV_drawpoint

void drawpoint(vector p1, vector p2, vector color, float thickness, float depthTest, float drawtime)

Draws a debug point, does not work on dedicated servers
For listen servers only the host will see the point.

drawpoint(self.origin, '1 0 0', 1, false, g_frameTime);
=================
*/
extern void SV_AddDebugPoint(vec3_t p1, vec3_t color, float thickness, float drawtime, qboolean depthtested);
void PFSV_drawpoint(void)
{
	float	*p1, *color;
	float	thickness, drawtime;
	qboolean depthtested;

	if (dedicated->value)
		return;

	//	if (sv_maxclients->value > 1)
	//		return;

	p1 = Scr_GetParmVector(0);
	color = Scr_GetParmVector(1);
	thickness = Scr_GetParmFloat(2);
	depthtested = Scr_GetParmFloat(3) > 0 ? true : false;
	drawtime = Scr_GetParmFloat(4);

	SV_AddDebugPoint(p1, color, thickness, drawtime, depthtested);
}

/*
=================
PFSV_drawbox

void drawbox(vector pos, vector mins, vector maxs, vector color, float thickness, float depthTest, float drawtime)

Draws a debug box, does not work on dedicated servers
For listen servers only the host will see the box.

drawbox(self.origin, self.mins, self.maxs, '1 0 0', 1, false, g_frameTime);
=================
*/
extern void SV_AddDebugBox(vec3_t pos, vec3_t p1, vec3_t p2, vec3_t color, float thickness, float drawtime, qboolean depthtested);
void PFSV_drawbox(void)
{
	float	*origin, *p1, *p2, *color;
	float	thickness, drawtime;
	qboolean depthtested;

	if (dedicated->value)
		return;

	//	if (sv_maxclients->value > 1)
	//		return;

	origin = Scr_GetParmVector(0);
	p1 = Scr_GetParmVector(1);
	p2 = Scr_GetParmVector(2);
	color = Scr_GetParmVector(3);
	thickness = Scr_GetParmFloat(4);
	depthtested = Scr_GetParmFloat(5) > 0 ? true : false;
	drawtime = Scr_GetParmFloat(6);

	SV_AddDebugBox(origin, p1, p2, color, thickness, drawtime, depthtested);
}

/*
=================
PFSV_drawtext

void drawtext(vector pos, vector color, float fontsize, float depthTest, float drawtime, string str, ...)

Draws a debug string.
For listen servers only the host will see the lines.

drawtext(self.origin, '1 1 1', 2.0, false, 0.1, self.classname);
=================
*/
void SV_AddDebugString(vec3_t pos, vec3_t color, float fontSize, float drawtime, qboolean depthtested, char* text);
void PFSV_drawstring(void)
{
	float	*pos, *color;
	float	fontsize, drawtime;
	qboolean depthtested;
	char	*str;

	if (dedicated->value)
		return;

	//	if (sv_maxclients->value > 1)
	//		return;

	pos = Scr_GetParmVector(0);
	color = Scr_GetParmVector(1);
	fontsize = Scr_GetParmFloat(2);
	depthtested = Scr_GetParmFloat(3) > 0 ? true : false;
	drawtime = Scr_GetParmFloat(4);
	str = Scr_VarString(5);

	if(!str || str[0] == 0)
		return;

	SV_AddDebugString(pos, color, fontsize, drawtime, depthtested, str);
}

/*
=================
PFSV_nav_init

void nav_init(float numnodes)
=================
*/
extern void Nav_Init();
void PFSV_nav_init(void)
{
	Nav_Init();
}

/*
=================
PFSV_nav_addpathnode

void nav_addpathnode(vector pos)
=================
*/
extern int Nav_AddPathNode(float x, float y, float z);
void PFSV_nav_addpathnode(void)
{
	float* pos = Scr_GetParmVector(0);
	Scr_ReturnFloat(Nav_AddPathNode(pos[0], pos[1], pos[2]));
}

/*
=================
PFSV_nav_linkpathnode

void nav_linkpathnode(float node, float linkto)
=================
*/
extern void Nav_AddPathNodeLink(int nodeId, int linkTo);
void PFSV_nav_linkpathnode(void)
{
	Nav_AddPathNodeLink((int)Scr_GetParmFloat(0), (int)Scr_GetParmFloat(1));
}


/*
=================
PFSV_nav_getnearestnode

float node = nav_getnearestnode(vector pos)
=================
*/
int Nav_GetNearestNode(vec3_t origin);
void PFSV_nav_getnearestnode(void)
{
	vec3_t v;
	float* pos = Scr_GetParmVector(0);

	VectorCopy(pos, v);

	Scr_ReturnFloat(Nav_GetNearestNode(v));
}

/*
=================
PFSV_nav_searchpath

float nextnode = nav_searchpath(float start, float end)
=================
*/
int Nav_SearchPath(int startWaypoint, int goalWaypoint);
void PFSV_nav_searchpath(void)
{
	float n;
	n = Nav_SearchPath((int)Scr_GetParmFloat(0), (int)Scr_GetParmFloat(1));

	Scr_ReturnFloat(n);
}

/*
=================
PFSV_nav_addpathnode

vector nav_getnodepos(float n)
=================
*/
extern void Nav_GetNodePos(int num, float* x, float* y, float* z);
void PFSV_nav_getnodepos(void)
{
	vec3_t v;
	Nav_GetNodePos(Scr_GetParmFloat(0), &v[0], &v[1], &v[2]);
	Scr_ReturnVector(v);
}


/*
=================
PFSV_nav_getnodescount

float nav_getnodescount()
Returns the number of navigation nodes loaded by engine
=================
*/
extern int Nav_GetNodesCount();
void PFSV_nav_getnodescount(void)
{
	Scr_ReturnFloat( Nav_GetNodesCount() );
}

/*
=================
PFSV_nav_getnodelinkcount

float nav_getnodelinkcount(float nodeIndex)
Returns the links count of a given node
=================
*/
extern int Nav_GetNodeLinkCount(int node);
void PFSV_nav_getnodelinkcount(void)
{
	Scr_ReturnFloat( Nav_GetNodeLinkCount(Scr_GetParmFloat(0)) );
}

/*
=================
PFSV_nav_getnodelink

float nav_getnodelink(float nodeIndex, float linkIndex)
Returns index to a node which its linked to
=================
*/
extern int Nav_GetNodeLink(int node, int link);
void PFSV_nav_getnodelink(void)
{
	Scr_ReturnFloat( Nav_GetNodeLink(Scr_GetParmFloat(0), Scr_GetParmFloat(1)) );
}


/*
=================
PFSV_getframescount

float getframescount(float modelindex)
Returns the number of animation frames in md3 and sp2 models

float numanimframes = getframescount(self.modelindex);
=================
*/
void PFSV_getframescount(void)
{
	int index = (int)Scr_GetParmFloat(0);
	svmodel_t* mod = SV_ModelForNum(index);
	if (!mod)
	{
		Scr_ReturnFloat(0);
		Scr_RunError("getframescount(): no model for index %i\n", index);
		return;
	}
	Scr_ReturnFloat(mod->numFrames);
}

/*
=================
PFSV_gettagscount

float gettagscount(float modelindex)
Returns the number of tags in a specified model

float mdl = precache_model("models/mutant.md3");
float numtags = gettagscount(mdl);
=================
*/
void PFSV_gettagscount(void)
{
	int index = (int)Scr_GetParmFloat(0);
	svmodel_t* mod = SV_ModelForNum(index);
	if (!mod)
	{
		Scr_ReturnFloat(0);
		Scr_RunError("getframescount(): no model for index %i\n", index);
		return;
	}

	if (mod->type == MOD_MD3)
		Scr_ReturnFloat(mod->numTags); 
	else
	{
		Scr_ReturnFloat(0);
//		Scr_RunError("getnumtags(): non MD3 model '%s'\n", mod->name);
	}
}

/*
=================
PFSV_tagexists

float tagexists(entity ent, string tagName)
Returns true if tag exists on entity's model

float has_flash = tagexists(self, "tag_flash");
=================
*/
void PFSV_tagexists(void)
{
	gentity_t* ent;
	char* tagName;
	svmodel_t* model;
	orientation_t* tag;

	ent = Scr_GetParmString(0);
	tagName = Scr_GetParmString(1);

	if (!ent->inuse)
	{
		Scr_RunError("tagexists(): entity %i not in use\n", NUM_FOR_ENT(ent));
		return;
	}

	model = SV_ModelForNum(ent->v.modelindex);
	if (!model || model->type != MOD_MD3 || !model->numTags)
	{
		Scr_ReturnFloat(0.0f);
		return;
	}

	tag = SV_GetTag((int)ent->v.modelindex, 0, tagName);
	Scr_ReturnFloat(tag != NULL ? 1.0f : 0.0f);
}

/*
=================
PFSV_gettagorigin

vector gettagorigin(entity ent, string tagName)

Returns tag origin in worldspace for entity's current animFrame
Only .modelindex ('main model') is taken for calculation
If entity's model has no tags or is not MD3, entity origin will be returned.

vector head_ = gettagorigin(self, "tag_head");
=================
*/
void PFSV_gettagorigin(void)
{
	gentity_t *ent;
	char *tagName;
	svmodel_t *model;
	orientation_t* tag;

	ent = Scr_GetParmEdict(0);
	tagName = Scr_GetParmString(1);

	if (!ent->inuse)
	{
		Scr_RunError("gettagorigin(): entity %i not in use\n", NUM_FOR_ENT(ent));
		return;
	}

	model = SV_ModelForNum(ent->v.modelindex);
	if (!model || model->type != MOD_MD3 || !model->numTags)
	{
		Scr_ReturnVector(ent->v.origin);
		return;
	}

	tag = SV_PositionTagOnEntity(ent, tagName);
	if (!tag)
	{
		Scr_RunError("gettagorigin(): model '%s' nas no tag '%s'\n", model->name, tagName);
		return;
	}

	Scr_ReturnVector(tag->origin);
}


/*
=================
PFSV_gettagangles

vector gettagangles(entity ent, string tagName)
Returns tag angles in worldspace for entity's current animFrame
Only .modelindex ('main model') is taken for calculation
If entity's model has no tags or is not MD3, entity angles will be returned.

vector looking_at = gettagangles(self, "tag_head");
=================
*/

void VectorAngles(const float* forward, const float* up, float* result, qboolean meshpitch)    //up may be NULL
{
	float    yaw, pitch, roll;

	if (forward[1] == 0 && forward[0] == 0)
	{
		if (forward[2] > 0)
		{
			pitch = -M_PI * 0.5;
			yaw = up ? atan2(-up[1], -up[0]) : 0;
		}
		else
		{
			pitch = M_PI * 0.5;
			yaw = up ? atan2(up[1], up[0]) : 0;
		}
		roll = 0;
	}
	else
	{
		yaw = atan2(forward[1], forward[0]);
		pitch = -atan2(forward[2], sqrt(forward[0] * forward[0] + forward[1] * forward[1]));

		if (up)
		{
			vec_t cp = cos(pitch), sp = sin(pitch);
			vec_t cy = cos(yaw), sy = sin(yaw);
			vec3_t tleft, tup;
			tleft[0] = -sy;
			tleft[1] = cy;
			tleft[2] = 0;
			tup[0] = sp * cy;
			tup[1] = sp * sy;
			tup[2] = cp;
			roll = -atan2(DotProduct(up, tleft), DotProduct(up, tup));
		}
		else
			roll = 0;
	}

	pitch *= 180 / M_PI;
	yaw *= 180 / M_PI;
	roll *= 180 / M_PI;
	if (meshpitch)
	{
//		pitch *= r_meshpitch.value;
//		roll *= r_meshroll.value;
	}
	if (pitch < 0)
		pitch += 360;
	if (yaw < 0)
		yaw += 360;
	if (roll < 0)
		roll += 360;

	result[0] = -pitch;
	result[1] = yaw-180;
	result[2] = roll+90;
}

void PFSV_gettagangles(void)
{
	gentity_t* ent;
	char* tagName;
	svmodel_t* model;
	orientation_t* tag;
	vec3_t out;

	ent = Scr_GetParmEdict(0);
	tagName = Scr_GetParmString(1);

	if (!ent->inuse)
	{
		Scr_RunError("gettagangles(): entity %i not in use\n", NUM_FOR_ENT(ent));
		return;
	}

	model = SV_ModelForNum((int)ent->v.modelindex);
	if (!model || model->type != MOD_MD3 || !model->numTags)
	{
		Scr_ReturnVector(ent->v.angles);
		return;
	}

	tag = SV_PositionTagOnEntity(ent, tagName);
	if (!tag)
	{
		Scr_RunError("gettagangles(): model '%s' nas no tag '%s'\n", model->name, tagName);
		return;
	}

	//
	// convert axis to angles
	//
	VectorAngles(tag->axis[0], tag->axis[2], out, true);

//	AxisToAngles(tag->axis, out);

	Scr_ReturnVector(out);
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
	Scr_DefineBuiltin(PFSV_precache_model, PF_SV, "precache_model", "float(string n)");
	Scr_DefineBuiltin(PFSV_precache_sound, PF_SV, "precache_sound", "float(string n)");
	Scr_DefineBuiltin(PFSV_precache_image, PF_SV, "precache_image", "float(string n)");

	// server general
	Scr_DefineBuiltin(PFSV_changemap, PF_SV, "changemap", "float(string nm, float pers)");
	Scr_DefineBuiltin(PFSV_kickclient, PF_SV, "kickclient", "void(entity p)");

	// entity general
	Scr_DefineBuiltin(PFSV_spawn, PF_SV, "spawn", "entity()");
	Scr_DefineBuiltin(PFSV_remove, PF_SV, "remove", "void(entity e)");

	Scr_DefineBuiltin(PFSV_getent, PF_SV, "getent", "entity(float idx)");
	Scr_DefineBuiltin(PFSV_nextent, PF_SV, "nextent", "entity(entity prev)");
	Scr_DefineBuiltin(PFSV_find, PF_SV, "find", "entity(entity e, .string fld, string match)");
	Scr_DefineBuiltin(PFSV_findradius, PF_SV, "findradius", "entity(entity e, vector v, float r)");
	Scr_DefineBuiltin(PFSV_getEntNum, PF_SV, "getentnum", "float(entity e)");

	Scr_DefineBuiltin(PFSV_setorigin, PF_SV, "setorigin", "void(entity e, vector v)");
	Scr_DefineBuiltin(PFSV_setmodel, PF_SV, "setmodel", "void(entity e, string s)");
	Scr_DefineBuiltin(PFSV_setsize, PF_SV, "setsize", "void(entity e, vector v1, vector v2)");
	Scr_DefineBuiltin(PFSV_linkentity, PF_SV, "linkentity", "void(entity e)");
	Scr_DefineBuiltin(PFSV_unlinkentity, PF_SV, "unlinkentity", "void(entity e)");

	// collision and physics
	Scr_DefineBuiltin(PFSV_contents, PF_SV, "pointcontents", "float(vector v)");
	Scr_DefineBuiltin(PFSV_trace, PF_SV, "trace", "void(vector p1, vector v1, vector v2, vector p2, entity e, int c)");

	Scr_DefineBuiltin(PFSV_touchentities, PF_SV, "touchentities", "float(entity e, float at)");

	Scr_DefineBuiltin(PFSV_checkbottom, PF_SV, "checkbottom", "float(entity e)");

	Scr_DefineBuiltin(PFSV_movetogoal, PF_SV, "movetogoal", "float(entity e, entity g, float d)");
	Scr_DefineBuiltin(PFSV_walkmove, PF_SV, "walkmove", "float(entity e, float y, float d)");

	// sound
	Scr_DefineBuiltin(PFSV_sound, PF_SV, "playsound", "void(vector v, entity e, float ch, string snd, float vol, float att, float tofs)");
	Scr_DefineBuiltin(PFSV_stopsounds, PF_SV, "stopsounds", "void(entity e)");

	// visibility and hearability
	Scr_DefineBuiltin(PFSV_SetAreaPortalState, PF_SV, "SetAreaPortalState", "void(float a1, float a2)");
	Scr_DefineBuiltin(PFSV_AreasConnected, PF_SV, "AreasConnected", "float(float a1, float a2)");

	Scr_DefineBuiltin(PFSV_inPVS, PF_SV, "inPVS", "float(vector v1, vector v2)");
	Scr_DefineBuiltin(PFSV_inPHS, PF_SV, "inPHS", "float(vector v1, vector v2)");

	//strings
	Scr_DefineBuiltin(PFSV_sprint, PF_SV, "sprint", "void(entity e, float pl, string s, ...)"); // overloading strings supported
	Scr_DefineBuiltin(PFSV_bprint, PF_SV, "bprint", "void(float pl, string s, ...)"); // overloading strings supported
	Scr_DefineBuiltin(PFSV_centerprint, PF_SV, "centerprint", "void(entity e, string s, ...)"); // overloading strings supported

	//configstrings and lightstyles
	Scr_DefineBuiltin(PFSV_configstring, PF_SV, "configstring", "void(float cs, string v)");
	Scr_DefineBuiltin(PFSV_lightstyle, PF_SV, "lightstyle", "void(float s, string v)");

	// network messages
	Scr_DefineBuiltin(PFSV_Unicast, PF_SV, "MSG_Unicast", "void(entity e, float r)");
	Scr_DefineBuiltin(PFSV_Multicast, PF_SV, "MSG_Multicast", "void(vector v, float to)");
	Scr_DefineBuiltin(PFSV_WriteChar, PF_SV, "MSG_WriteChar", "void(string v)");
	Scr_DefineBuiltin(PFSV_WriteByte, PF_SV, "MSG_WriteByte", "void(float v)");
	Scr_DefineBuiltin(PFSV_WriteShort, PF_SV, "MSG_WriteShort", "void(float v)");
	Scr_DefineBuiltin(PFSV_WriteShort, PF_SV, "MSG_WriteLong", "void(float v)");
	Scr_DefineBuiltin(PFSV_WriteString, PF_SV, "MSG_WriteString", "void(string v)");
	Scr_DefineBuiltin(PFSV_WritePos, PF_SV, "MSG_WritePos", "void(vector v)");
	Scr_DefineBuiltin(PFSV_WriteDir, PF_SV, "MSG_WriteDir", "void(vector v)");
	Scr_DefineBuiltin(PFSV_WriteAngle, PF_SV, "MSG_WriteAngle", "void(float v)");

	// client
	Scr_DefineBuiltin(PFSV_isplayer, PF_SV, "isplayer", "float(entity e)");
	Scr_DefineBuiltin(PFSV_setviewmodel, PF_SV, "setviewmodel", "float(entity e, string s)");
	Scr_DefineBuiltin(PFSV_setviewmodelparms, PF_SV, "setviewmodelparms", "void(entity e, float f, vector v1, vector v2)");
	Scr_DefineBuiltin(PFSV_setfieldofview, PF_SV, "setfieldofview", "void(entity e, float f)");
	Scr_DefineBuiltin(PFSV_getfieldofview, PF_SV, "getfieldofview", "float(entity e)");
	Scr_DefineBuiltin(PFSV_setviewblend, PF_SV, "setviewblend", "void(entity e, vector v1, float f)");
	Scr_DefineBuiltin(PFSV_setviewoffset, PF_SV, "setviewoffset", "void(entity e, vector v1)");
	Scr_DefineBuiltin(PFSV_getviewoffset, PF_SV, "getviewoffset", "vector(entity e)");
	Scr_DefineBuiltin(PFSV_stuffcmd, PF_SV, "stuffcmd", "void(entity e, string s)"); // overloading strings supported
	Scr_DefineBuiltin(PFSV_setstat, PF_SV, "setstat", "float(entity e, float sg, float sc)");
	Scr_DefineBuiltin(PFSV_pmove, PF_SV, "pmove", "float(entity e, vector mv, float cr)");
	Scr_DefineBuiltin(PFSV_setviewangles, PF_SV, "setviewangles", "vector(entity e, vector a)");
	Scr_DefineBuiltin(PFSV_kickangles, PF_SV, "kickangles", "vector(entity e, vector a)");

	// persistant data across map changes
	Scr_DefineBuiltin(PFSV_saveclientfield, PF_SV, "saveclientfield", "void(entity p, float idx, float val)");
	Scr_DefineBuiltin(PFSV_loadclientfield, PF_SV, "loadclientfield", "float(entity p, float idx)");
	Scr_DefineBuiltin(PFSV_saveglobal, PF_SV, "saveglobal", "void(float idx, float val)");
	Scr_DefineBuiltin(PFSV_loadglobal, PF_SV, "loadglobal", "float(float idx)");

	// navigation 
	// TODO -- share with cgame
	Scr_DefineBuiltin(PFSV_nav_init, PF_SV, "nav_init", "void(float n)");
	Scr_DefineBuiltin(PFSV_nav_addpathnode, PF_SV, "nav_addpathnode", "float(vector p)");
	Scr_DefineBuiltin(PFSV_nav_linkpathnode, PF_SV, "nav_linkpathnode", "void(float n, float lt)");
	Scr_DefineBuiltin(PFSV_nav_getnearestnode, PF_SV, "nav_getnearestnode", "float(vector p)");
	Scr_DefineBuiltin(PFSV_nav_searchpath, PF_SV, "nav_searchpath", "float(float n1, float n2)");
	Scr_DefineBuiltin(PFSV_nav_getnodepos, PF_SV, "nav_getnodepos", "vector(float n)");
	Scr_DefineBuiltin(PFSV_nav_getnodescount, PF_SV, "nav_getnodescount", "float()");
	Scr_DefineBuiltin(PFSV_nav_getnodelinkcount, PF_SV, "nav_getnodelinkcount", "float(float n)");
	Scr_DefineBuiltin(PFSV_nav_getnodelink, PF_SV, "nav_getnodelink", "float(float n, float l)");
	

	// models
	// TODO -- share with cgame
	Scr_DefineBuiltin(PFSV_getframescount, PF_SV, "getframescount", "float(float midx)");
	Scr_DefineBuiltin(PFSV_gettagscount, PF_SV, "gettagscount", "float(float midx)");
	Scr_DefineBuiltin(PFSV_tagexists, PF_SV, "tagexists", "float(entity e, string tn)");
	Scr_DefineBuiltin(PFSV_gettagorigin, PF_SV, "gettagorigin", "vector(entity e, string tn)");
	Scr_DefineBuiltin(PFSV_gettagangles, PF_SV, "gettagangles", "vector(entity e, string tn)");

	// debug
	Scr_DefineBuiltin(PFSV_drawline, PF_SV, "drawline", "void(vector p1, vector p2, vector c, float th, float dt, float t)");
	Scr_DefineBuiltin(PFSV_drawpoint, PF_SV, "drawpoint", "void(vector p, vector c, float th, float dt, float t)");
	Scr_DefineBuiltin(PFSV_drawbox, PF_SV, "drawbox", "void(vector p, vector p1, vector p2, vector c, float th, float dt, float t)"); // fixme?
	Scr_DefineBuiltin(PFSV_drawstring, PF_SV, "drawstring", "void(vector p, vector c, float fs, float dt, float t, string s, ...)");
}
