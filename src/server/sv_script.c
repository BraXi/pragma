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
	Scr_Execute(VM_SVGAME, self->v.prethink, __FUNCTION__);
}

void Scr_Think(gentity_t* self)
{
	if (!self->v.think)
	{
		Com_Error(ERR_DROP,"Scr_Think: entity %i has no think function set\n", NUM_FOR_EDICT(self));
		return;
	}

	self->v.nextthink = 0;

	
	sv.script_globals->sv_time = sv.time;
	sv.script_globals->g_time = sv.gameTime;
	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(VM_SVGAME, self->v.think, __FUNCTION__);
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

	scr_entity_t oldself = sv.script_globals->self;
	scr_entity_t oldother = sv.script_globals->other;

	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(other);
	Scr_Execute(VM_SVGAME, self->v.blocked, __FUNCTION__);

	sv.script_globals->self = oldself;
	sv.script_globals->other = oldother;
}

void Scr_Event_Touch(gentity_t* self, gentity_t* other, cplane_t* plane, csurface_t* surf)
{
	if (!self->v.touch || self->v.solid == SOLID_NOT)
		return;

	scr_entity_t oldself = sv.script_globals->self;
	scr_entity_t oldother = sv.script_globals->other;

	sv.script_globals = Scr_GetGlobals();
	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(other);

	//float planeDist, vector planeNormal, float surfaceFlags
	Scr_BindVM(VM_SVGAME);
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
	Scr_Execute(VM_SVGAME, self->v.touch, __FUNCTION__);

	sv.script_globals->self = oldself;
	sv.script_globals->other = oldother;
}


// -----------------------------------------------------------------

void SV_ScriptMain()
{
	sv.script_globals->worldspawn = GENT_TO_PROG(sv.edicts);
	sv.script_globals->self = sv.script_globals->worldspawn;
	sv.script_globals->other = sv.script_globals->worldspawn;

	sv.script_globals->sv_time = sv.time;
	sv.script_globals->g_time = sv.gameTime;
	sv.script_globals->g_frameNum = sv.gameFrame;
	sv.script_globals->g_frameTime = SV_FRAMETIME;

	Scr_Execute(VM_SVGAME, sv.script_globals->main, __FUNCTION__);
}

void SV_ScriptStartFrame()
{
	// let the progs know that a new frame has started
	sv.script_globals->worldspawn = GENT_TO_PROG(sv.edicts);
	sv.script_globals->self = sv.script_globals->worldspawn;
	sv.script_globals->other = sv.script_globals->worldspawn;
	sv.script_globals->sv_time = sv.time;
	sv.script_globals->g_time = sv.gameTime;
	sv.script_globals->g_frameNum = sv.gameFrame;
	sv.script_globals->g_frameTime = SV_FRAMETIME;
	Scr_Execute(VM_SVGAME, sv.script_globals->StartFrame, __FUNCTION__);
}

void SV_ScriptEndFrame()
{
	sv.script_globals->worldspawn = GENT_TO_PROG(sv.edicts);
	sv.script_globals->self = sv.script_globals->worldspawn;
	sv.script_globals->other = sv.script_globals->worldspawn;
	sv.script_globals->g_time = sv.gameTime;
	sv.script_globals->sv_time = sv.time;
	Scr_Execute(VM_SVGAME, sv.script_globals->EndFrame, __FUNCTION__);
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
	Scr_Execute(VM_SVGAME, sv.script_globals->ClientBegin, __FUNCTION__);
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
	Scr_Execute(VM_SVGAME, sv.script_globals->ClientConnect, __FUNCTION__);

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
void Scr_ClientDisconnect(gentity_t* self)
{
	gentity_t* ent;

	if (!self->client)
		return;

	SV_SetConfigString((CS_CLIENTS + (NUM_FOR_ENT(self) - 1)), NULL);

	Scr_BindVM(VM_SVGAME);
	for (int i = 1; i < sv.num_edicts; i++)
	{
		ent = ENT_FOR_NUM(i);

		// find and clear .owner field
		if (VM_TO_ENT(ent->v.owner) == self)
		{
			ent->v.owner = ENT_TO_VM(sv.edicts);
		}

		// find gentities that were only shown to that particular 
		// client, and make sure they become visible to everyone
		if (((int)ent->v.svflags & SVF_SINGLECLIENT))
		{
			if (ent->v.showto == self->s.number) //NUM_FOR_ENT(self))
			{
				int temp = ent->v.svflags;
				temp &= ~SVF_SINGLECLIENT;
				ent->v.svflags = temp;
				ent->v.showto = 0;
//				break;
			}
		}	
	}

	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(VM_SVGAME, sv.script_globals->ClientDisconnect, __FUNCTION__);

	sv.script_globals->self = GENT_TO_PROG(sv.edicts);

	SV_UnlinkEdict(self);

//	memset(&self->v, 0, Scr_GetEntityFieldsSize());
	SV_InitEntity(self); // clear ALL fields, but mark it as unused
	self->inuse = false;
	self->v.classname = Scr_SetString("disconnected");
	self->client->pers.connected = false;
}



/*
==============
Scr_ClientThink

This will be called once for each client frame, which will usually be a couple times for each server frame.

runs svgame:ClientThink(float inButtons, float inImpulse, vector inMove, vector inAngles, float inMsecTime)
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

	Scr_BindVM(VM_SVGAME);
	Scr_AddFloat(0, (int)ucmd->buttons);
	Scr_AddFloat(1, (int)ucmd->impulse);
	Scr_AddVector(2, move); // [forwardmove, sidemove, upmove]
	Scr_AddVector(3, a);
	Scr_AddFloat(4, (int)ucmd->msec);

	Scr_Execute(VM_SVGAME, sv.script_globals->ClientThink, __FUNCTION__);

//	ClientMove(ent, ucmd);
//	ent->client->ps.pmove = pm;

	pmove_state_t* pm;
	pm = &ent->client->ps.pmove;

	pm->pm_type = (int)ent->v.pm_time;
	pm->pm_flags = (int)ent->v.pm_flags;
	pm->pm_time = ent->v.pm_time;
	pm->gravity = ent->v.pm_gravity;

	ent->client->old_pmove = ent->client->ps.pmove;

//	if (ent->v.movetype != MOVETYPE_NOCLIP)
//		SV_TouchEntities(ent, AREA_TRIGGERS); // moved to script
}

void Scr_ClientBeginServerFrame(gentity_t* self)
{
	sv.script_globals->self = GENT_TO_PROG(self);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(VM_SVGAME, sv.script_globals->ClientBeginServerFrame, __FUNCTION__);
}


void Scr_ClientEndServerFrame(gentity_t* ent)
{
	int i;

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

	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(VM_SVGAME, sv.script_globals->ClientEndServerFrame, __FUNCTION__);
}


/*
=================
SV_ProgVarsToEntityState

Set entity_state_t from prog fields
=================
*/
void SV_ProgVarsToEntityState(gentity_t* ent)
{
	VectorCopy(ent->v.origin, ent->s.origin);
	VectorCopy(ent->v.angles, ent->s.angles);
	VectorCopy(ent->v.old_origin, ent->s.old_origin);

	ent->s.modelindex = ent->v.modelindex;
	ent->s.modelindex2 = ent->v.modelindex2;
	ent->s.modelindex3 = ent->v.modelindex3;
	ent->s.modelindex4 = ent->v.modelindex3;

	ent->s.anim = ent->v.anim;
	ent->s.animtime = ent->v.animtime;
	ent->s.frame = (int)ent->v.animFrame;
	ent->s.skinnum = (int)ent->v.skinnum;
	ent->s.effects = ent->v.effects;

	ent->s.renderFlags = ent->v.renderFlags;
	ent->s.renderScale = ent->v.renderScale;
	VectorCopy(ent->v.renderColor, ent->s.renderColor);
	ent->s.renderAlpha = ent->v.renderAlpha;

	ent->s.loopingSound = (int)ent->v.loopsound;
	ent->s.event = (int)ent->v.event;
}

/*
=================
SV_EntityStateToProgVars

Set prog fields from entity_state_t
=================
*/
void SV_EntityStateToProgVars(gentity_t* ent, entity_state_t* state)
{
	VectorCopy(state->origin, ent->v.origin);
	VectorCopy(state->angles, ent->v.angles);
	VectorCopy(state->old_origin, ent->v.old_origin);

	ent->v.modelindex = state->modelindex;
	ent->v.modelindex2 = state->modelindex2;
	ent->v.modelindex3 = state->modelindex3;
	ent->v.modelindex4 = state->modelindex3;

	ent->v.anim = ent->s.anim;
	ent->v.animtime = state->animtime;
	ent->v.animFrame = state->frame;

	ent->v.skinnum = state->skinnum;
	ent->v.effects = state->effects;

	ent->v.renderFlags = state->renderFlags;
	ent->v.renderScale = state->renderScale;

	VectorCopy(state->renderColor, ent->v.renderColor);
	ent->v.renderAlpha = state->renderAlpha;

	ent->v.loopsound = state->loopingSound;
	ent->v.event = state->event;
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

	SV_SetConfigString((CS_CLIENTS + (NUM_FOR_ENT(ent) - 1)), ent->client->pers.netname);
}


static void cprintf(gentity_t* ent, int level, char* fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n = 0;

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

	scr_entity_t oldself = sv.script_globals->self;
	scr_entity_t oldother = sv.script_globals->other;

	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(VM_SVGAME, sv.script_globals->ClientCommand, __FUNCTION__);

	if (Scr_GetReturnFloat() > 0)
		return;

	Cmd_Say_f(ent, false, false);

	sv.script_globals->self = oldself;
	sv.script_globals->other = oldother;
}
