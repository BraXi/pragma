/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
// sv_script.c - handles all callbacks to scripts in one place for sanity

#include "server.h"


void Scr_SV_OP(eval_t *a, eval_t* b, eval_t* c)
{
	gentity_t *ed = VM_TO_ENT(sv.script_globals->self);
	ed->v.nextthink = sv.script_globals->g_time + 0.1;
	if (a->_float != ed->v.animFrame)
	{
			ed->v.animFrame = a->_float;
	}
	ed->v.think = b->function;
}

void Scr_EntityPreThink(gentity_t* self)
{
	if (!self->v.prethink)
		return;

	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(self->v.prethink, __FUNCTION__);
}

void Scr_Think(gentity_t* self)
{
	if (!self->v.think)
	{
		Com_Error(ERR_DROP,"Scr_Think: entity %i has no think function set\n", NUM_FOR_EDICT(self));
		return;
	}

	self->v.nextthink = 0;

	sv.script_globals->g_time = sv.gameTime;
	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(self->v.think, __FUNCTION__);
}

// -----------------------------------------------------------------

void Scr_Event_Impact(gentity_t* self, trace_t* trace)
{
	gentity_t* other = trace->ent;

	Scr_Event_Touch(self, other, &trace->plane, trace->surface);
	Scr_Event_Touch(other, self, NULL, NULL);
}

//SV_Physics_Pusher
void Scr_Event_Blocked(gentity_t* self, gentity_t* other)
{
	if (!self->v.blocked)
		return;

	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(other);
	Scr_Execute(self->v.blocked, __FUNCTION__);
}

void Scr_Event_Touch(gentity_t* self, gentity_t* other, cplane_t* plane, csurface_t* surf)
{
	if (!self->v.touch || self->v.solid == SOLID_NOT)
		return;

	sv.script_globals = Scr_GetGlobals();
	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(other);

	//float planeDist, vector planeNormal, float surfaceFlags
	if (plane)
	{
		Scr_AddFloat(0, plane->dist);
		Scr_AddVector(1, plane->normal);
	}
	else
	{
		Scr_AddFloat(0, 0.0f);
		Scr_AddVector(1, vec3_origin);
	}
	Scr_AddFloat(2, surf != NULL ? surf->flags : 0);
	Scr_Execute(self->v.touch, __FUNCTION__);
}


// -----------------------------------------------------------------

void SV_ScriptMain()
{
	sv.script_globals->worldspawn = GENT_TO_PROG(sv.edicts);
	sv.script_globals->self = sv.script_globals->worldspawn;
	sv.script_globals->other = sv.script_globals->worldspawn;

	sv.script_globals->g_time = sv.gameTime;
	sv.script_globals->g_frameNum = sv.gameFrame;
	sv.script_globals->g_frameTime = SV_FRAMETIME;

	Scr_Execute(sv.script_globals->main, __FUNCTION__);
}

void SV_ScriptStartFrame()
{
	// let the progs know that a new frame has started
	sv.script_globals->worldspawn = GENT_TO_PROG(sv.edicts);
	sv.script_globals->self = sv.script_globals->worldspawn;
	sv.script_globals->other = sv.script_globals->worldspawn;
	sv.script_globals->g_time = sv.gameTime;
	sv.script_globals->g_frameNum = sv.gameFrame;
	sv.script_globals->g_frameTime = SV_FRAMETIME;
	Scr_Execute(sv.script_globals->StartFrame, __FUNCTION__);
}

void SV_ScriptEndFrame()
{
	sv.script_globals->worldspawn = GENT_TO_PROG(sv.edicts);
	sv.script_globals->self = sv.script_globals->worldspawn;
	sv.script_globals->other = sv.script_globals->worldspawn;
	sv.script_globals->g_time = sv.gameTime;
	Scr_Execute(sv.script_globals->EndFrame, __FUNCTION__);
}

// -----------------------------------------------------------------

/*
===========
Scr_ClientBegin

called when a client has finished connecting, and is ready to be placed into the game.
This will happen every level load.
============
*/
void Scr_ClientBegin(gentity_t* self)
{
	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(sv.script_globals->ClientBegin, __FUNCTION__);
}


/*
===========
ClientConnect

Called when a player begins connecting to the server. The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but loadgames will.
============
*/
qboolean Scr_ClientConnect(gentity_t* ent, char* userinfo)
{
	char* value;
	qboolean allowed;

	// check for a password
	value = Info_ValueForKey(userinfo, "password");
	if (*sv_password->string && strcmp(sv_password->string, "none") && strcmp(sv_password->string, value))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
		return false;
	}

	// they can connect
	ent->client = &svs.gclients[NUM_FOR_EDICT(ent) - 1];

	// if there is already a body waiting for us (a loadgame), just take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
	}

	ClientUserinfoChanged(ent, userinfo);

	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(sv.script_globals->ClientConnect, __FUNCTION__);

	allowed = Scr_GetReturnFloat();

	ent->client->pers.connected = allowed;
	return allowed;
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void Scr_ClientDisconnect(gentity_t* ent)
{
	if (!ent->client)
		return;

	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(sv.script_globals->ClientDisconnect, __FUNCTION__);

	sv.script_globals->self = GENT_TO_PROG(sv.edicts);

	SV_UnlinkEdict(ent);

	memset(&ent->v, 0, Scr_GetEntityFieldsSize());

	SV_InitEntity(ent); // clear ALL fields, but later on mark it as unised
//	ent->v.modelindex[0] = 0;
//	ent->v.solid = SOLID_NOT;
	ent->inuse = false;
	ent->v.classname = Scr_SetString("disconnected");
	ent->client->pers.connected = false;
}



/*
==============
Scr_ClientThink

This will be called once for each client frame, which will usually be a couple times for each server frame.
==============
*/
void ClientMove(gentity_t* ent, usercmd_t* ucmd);
usercmd_t* last_ucmd;
void Scr_ClientThink(gentity_t* ent, usercmd_t* ucmd)
{
	vec3_t move;

	move[0] = ucmd->forwardmove;
	move[1] = ucmd->sidemove;
	move[2] = ucmd->upmove;

	vec3_t a;
	a[0] = ucmd->angles[0];
	a[1] = ucmd->angles[1];
	a[2] = ucmd->angles[2];

	last_ucmd = ucmd; // for pmove

	sv.script_globals->g_frameNum = sv.framenum;
	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);

	Scr_AddFloat(0, (int)ucmd->buttons);
	Scr_AddFloat(1, (int)ucmd->impulse);
	Scr_AddVector(2, move);
	Scr_AddVector(3, a);
	Scr_AddFloat(4, (int)ucmd->msec);

	// move [forwardmove, sidemove, upmove]
	// void ClientThink(float inButtons, float inImpulse, vector inMove, vector inAngles, float inMsecTime)
	Scr_Execute(sv.script_globals->ClientThink, __FUNCTION__);

//	ClientMove(ent, ucmd);

	pmove_state_t* pm;
	pm = &ent->client->ps.pmove;

	pm->pm_type = (int)ent->v.pm_time;
	pm->pm_flags = (int)ent->v.pm_flags;
	pm->pm_time = ent->v.pm_time;
	pm->gravity = ent->v.pm_gravity;

	//ent->client->ps.pmove = pm;
	ent->client->old_pmove = ent->client->ps.pmove;


	if (ent->v.movetype != MOVETYPE_NOCLIP)
		SV_TouchEntities(ent, AREA_TRIGGERS);
}

#if 0
/*
==============
ClientMove

This is just here untill pmove is entirely handled by script.
==============
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

void ClientMove(gentity_t* ent, usercmd_t* ucmd)
{
	gclient_t* client;
	pmove_t	pm;
	int i;

	client = ent->client;

	// set up for pmove
	pm_passent = ent;
	memset(&pm, 0, sizeof(pm));

	client->ps.pmove.pm_type = ent->v.pm_type;
	client->ps.pmove.pm_flags = ent->v.pm_flags;


#if 1
	if (ent->v.movetype == MOVETYPE_NOCLIP)
		client->ps.pmove.pm_type = PM_SPECTATOR;
	else if (ent->v.health <= -50)
		client->ps.pmove.pm_type = PM_GIB;
	else
		client->ps.pmove.pm_type = PM_NORMAL;
#endif

	client->ps.pmove.gravity = sv_gravity->value;
	pm.s = client->ps.pmove;
	for (i = 0; i < 3; i++)
	{
		pm.s.origin[i] = ent->v.origin[i] * 8;
		pm.s.velocity[i] = ent->v.velocity[i] * 8;
	}
	if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
	{
		pm.snapinitial = true;
		Com_Printf("pmove changed!\n");
	}

	pm.cmd = *ucmd;
	pm.trace = PM_trace;
	pm.pointcontents = SV_PointContents;
	Pmove(&pm); // perform a pmove

	// save results of pmove
	client->ps.pmove = pm.s;
	client->old_pmove = pm.s;

	for (i = 0; i < 3; i++)
	{
		ent->v.origin[i] = pm.s.origin[i] * 0.125;
		ent->v.velocity[i] = pm.s.velocity[i] * 0.125;
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

	ent->v.groundEntityNum = (pm.groundentity == NULL ? -1 : pm.groundentity->s.number); //ent->groundentity = pm.groundentity;
	if (pm.groundentity)
		ent->v.groundEntity_linkcount = pm.groundentity->linkcount; 

	VectorCopy(pm.viewangles, ent->v.v_angle);
	VectorCopy(pm.viewangles, client->ps.viewangles);
	SV_LinkEdict(ent);
}
#endif

void Scr_ClientBeginServerFrame(gentity_t* self)
{
	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(sv.script_globals->ClientBeginServerFrame, __FUNCTION__);
}


void Scr_ClientEndServerFrame(gentity_t* ent)
{
	int i;
	static vec3_t forward, right, up;
	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	// 
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	ent->client->ps.viewoffset[0] = ent->client->ps.viewoffset[1] = 0;
	ent->client->ps.viewoffset[2] = ent->v.viewheight;

	ent->client->ps.pmove.pm_type = ent->v.pm_type;
	ent->client->ps.pmove.pm_flags = ent->v.pm_flags;
	ent->client->ps.pmove.pm_time = ent->v.pm_time;
	ent->client->ps.pmove.gravity = ent->v.pm_gravity;

	for (i = 0; i < 3; i++)
	{
		ent->client->ps.pmove.mins[i] = ent->v.pm_mins[i];
		ent->client->ps.pmove.maxs[i] = ent->v.pm_maxs[i];
		ent->client->ps.pmove.delta_angles[i] = ent->v.pm_delta_angles[i];
	}

	// save results of pmove
//	ent->client->ps.pmove = pm;
	ent->client->old_pmove = ent->client->ps.pmove;

#if PROTOCOL_FLOAT_COORDS == 1
	for (i = 0; i < 3; i++)
	{
		ent->client->ps.pmove.origin[i] = ent->v.origin[i];
		ent->client->ps.pmove.velocity[i] = ent->v.velocity[i];
	}
#else
	for (i = 0; i < 3; i++)
	{
		pm->origin[i] = ent->v.origin[i] * 8.0;
		pm->velocity[i] = ent->v.velocity[i] * 8.0;
	}
#endif

	AngleVectors(ent->v.v_angle, forward, right, up);

//	cl->ps.viewoffset[2] = 128;

	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(sv.script_globals->ClientEndServerFrame, __FUNCTION__);

	//test
	//	gclient_t* cl;
	//	cl = ent->client;
	//	cl->ps.viewoffset[2] = Cvar_Get("playerz", "22", 0)->value;
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.
The game can override any of the settings in place (forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged(gentity_t* ent, char* userinfo)
{
	char* s;
	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		strcpy(userinfo, "\\name\\badpragma");
	}

	// set name
	s = Info_ValueForKey(userinfo, "name");
	strncpy(ent->client->pers.netname, s, sizeof(ent->client->pers.netname) - 1);

	ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
	if (ent->client->ps.fov < 1)
		ent->client->ps.fov = 90;
	else if (ent->client->ps.fov > 160)
		ent->client->ps.fov = 160;

	// save off the userinfo in case we want to check something later
	strncpy(ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo) - 1);
}


static void cprintf(gentity_t* ent, int level, char* fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n;

	if (ent)
	{
		n = NUM_FOR_EDICT(ent);
		if (n < 1 || n > sv_maxclients->value)
			Com_Error(ERR_DROP, "cprintf to a non-client entity %i\n", n);
	}

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	if (ent)
		SV_ClientPrintf(svs.clients + (n - 1), level, "%s", msg);
	else
		Com_Printf("%s", msg);
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f(gentity_t* ent, qboolean team, qboolean arg0)
{
	int			j;
	gentity_t	*other;
	char*		 p;
	char		text[2048];

	if(Cmd_Argc() < 2 && !arg0)
		return;

	Com_sprintf(text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat(text, Cmd_Argv(0));
		strcat(text, " ");
		strcat(text, Cmd_Args());
	}
	else
	{
		p = Cmd_Args();
		if (*p == '"')
		{
			p++;
			p[strlen(p) - 1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (dedicated->value)
		cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= sv_maxclients->value; j++)
	{
		other = EDICT_NUM(j);
		if (!other->inuse || !other->client)
			continue;

		if (team && ent->v.team != other->v.team)
			continue;

		cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void ClientCommand(gentity_t* ent)
{
	char* cmd;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = Cmd_Argv(0);

	if (Q_stricmp(cmd, "say") == 0 && Cmd_Argc() > 1)
	{
		Cmd_Say_f(ent, false, false);
		return;
	}
	else if (Q_stricmp(cmd, "say_team") == 0 && Cmd_Argc() > 1)
	{
		Cmd_Say_f(ent, true, false);
		return;
	}

	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(sv.script_globals->ClientCommand, __FUNCTION__);

	if (Scr_GetReturnFloat() > 0)
		return;

	Cmd_Say_f(ent, false, false);
}