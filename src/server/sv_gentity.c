/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
// sv_gentity.c

#include "server.h"
#include "../script/script_internals.h"

extern ddef_t* Scr_FindEntityField(char* name); //scr_main.c
extern qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s); //scr_main.c

/*
=================
SV_InitEntity

Initializes newly spawned gentity
=================
*/
void SV_InitEntity(gentity_t* ent)
{
	ent->inuse = true;

	//memset(&ent->s, 0, sizeof(entity_state_t));
	memset(&ent->v, 0, Scr_GetEntityFieldsSize()); // the size is always read from progs

	ent->s.number = NUM_FOR_EDICT(ent);
	ent->v.classname = Scr_SetString("no_class");
	ent->v.gravity = 1.0;
	ent->v.groundEntityNum = -1;
}

/*
=================
SV_SpawnEntity

Finds a free entity. Try to avoid reusing an entity that was recently freed, 
because it can cause the client to think the entity morphed into something else instead of being removed 
and recreated, which can cause interpolated angles and bad trails.
=================
*/
gentity_t* SV_SpawnEntity(void)
{
	int			entnum;
	gentity_t	*ent = NULL;

	entnum = sv_maxclients->value + 1;
	ent = EDICT_NUM(entnum);

	// we seek for the first free gentity after worldspawn and players that wasn't recently freed
	for (entnum = (sv_maxclients->value + 1); entnum < sv.max_edicts; entnum++)
	{
		ent = EDICT_NUM(entnum);
		if (!ent->inuse)
		{		
			if(ent->freetime == 0.0f /*game is starting*/ || sv.gameTime >= (ent->freetime + SV_FRAMETIME*2) /*give it 2 frames before use*/)
				break;
		}
	}

	sv.num_edicts++;

	if (sv.num_edicts == sv.max_edicts)
		Com_Error(ERR_DROP, "SV_SpawnEntity: no free entities, hit %i limit\n", sv.max_edicts);
	
	SV_InitEntity(ent);
	return ent;
}

/*
=================
SV_FreeEntity

Marks the entity as free
=================
*/
void SV_FreeEntity(gentity_t* ent)
{
	if (!ent)
		Com_Error(ERR_DROP, "SV_FreeEntity: !ent\n");

	if (NUM_FOR_EDICT(ent) <= sv_maxclients->value)
	{
		Com_DPrintf(DP_SV, "tried to free client entity\n");
		return;
	}
	SV_UnlinkEdict(ent);

	if (ent != sv.edicts)
	{
		// dereference self and other globals in script if they're us
		if (PROG_TO_GENT(sv.script_globals->self) == ent)
			sv.script_globals->self = GENT_TO_PROG(sv.edicts);
		if (PROG_TO_GENT(sv.script_globals->other) == ent)
			sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	}

	if(ent && ent->inuse)
		sv.num_edicts--;

//	memset(ent, 0, Scr_GetEntitySize()); // clear whole entity
	memset(&ent->v, 0, Scr_GetEntityFieldsSize()); // clear script fields

	ent->v.classname = Scr_SetString("freed");
	ent->freetime = sv.gameTime;
	ent->inuse = false;
}

/*
================
SV_RunEntity
================
*/
void SV_RunEntity(gentity_t* ent)
{
	Scr_EntityPreThink(ent);
	SV_RunEntityPhysics(ent);
}

/*
=============
SV_RunThink
Runs thinking code for this frame if necessary
=============
*/
qboolean SV_RunThink(gentity_t* ent)
{
	float	thinktime;

	thinktime = ent->v.nextthink;
	if (thinktime <= 0)
		return true;
	if (thinktime > sv.gameTime + 0.001)
		return true;

	Scr_Think(ent);

	return false;
}

/*
============
SV_TouchEntities

Touches all entities overlaping with ent's bbox
areatype could either be AREA_SOLID or AREA_TRIGGERS
============
*/
void SV_TouchEntities(gentity_t* ent, int areatype)
{
	int			i, num;
	gentity_t* touch[MAX_GENTITIES], * hit;

// dead things don't activate triggers!
// BRAXI -- WHAT IF DEAD THINGS WANT TO 
//	if ((ent->client || ((int)ent->v.svflags & SVF_MONSTER)) && (ent->v.health <= 0))
//		return;

	num = SV_AreaEdicts(ent->v.absmin, ent->v.absmax, touch, MAX_GENTITIES, areatype);

	// be careful, it is possible to have an entity in this list removed before we get to it (killtriggered)
	for (i = 0; i < num; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		Scr_Event_Touch(hit, ent, NULL, NULL);
	}
}


/*
===============
SV_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void SV_CallSpawn(gentity_t* ent)
{
	gentity_t	*oldSelf, *oldOther;
	char		*classname;
	scr_func_t	spawnfunc;

	static char spawnFuncName[64];
	
	classname = Scr_GetString(ent->v.classname);

	if (strlen(classname) > 60)
	{
		printf("CallSpawn: classname \"%s\" is too long\n", classname);
		return;
	}

	// check if someone is trying to spawn world... fixme
//	if( EDICT_NUM(0)->inuse && stricmp(classname, "worldspawn") == 0 )
//	{
//		Com_Error(ERR_DROP, "CallSpawn: only one worldspawn allowed\n", classname);
//		return;
//	}

	if (ent == sv.edicts)
	{
		// worldspawn hack
		ent->inuse = 1;
		ent->v.modelindex[0] = 1;
		sv.num_edicts++;
	}

	// remove utility entities straight away
	//if (stricmp(classname, "info_null") == 0 || stricmp(classname, "func_group") == 0)
	//{
		//SV_FreeEntity(ent);
	//	return;
	//}

	// find spawn fuction in progs
	sprintf(spawnFuncName, "SP_%s", classname);
	spawnfunc = Scr_FindFunction(spawnFuncName);
	if (spawnfunc == -1 && ent != sv.edicts)
	{
		Com_DPrintf( DP_SV, "unknown entity: %s\n", classname);
		SV_FreeEntity(ent);
		return;
	}

	// backup script's 'self' and 'other'
	oldSelf = PROG_TO_GENT(sv.script_globals->self);
	oldOther = PROG_TO_GENT(sv.script_globals->other);

	// call spawn function
	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(spawnfunc, __FUNCTION__);

	// an entity may refuse to spawn (requires coop, certain skill level, etc..)
	// if returned value from prog is false we delete entity right now (unless its world)
	if (ent != sv.edicts && Scr_GetReturnFloat() <= 0)
	{
		printf("CallSpawn: \"%s\" at (%i %i %i) discarded\n", classname, (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);
		SV_FreeEntity(ent);
		oldSelf = sv.edicts;
	}
//	else
//		printf("spawned %i:\"%s\" at (%i %i %i)\n",  NUM_FOR_EDICT(ent), classname, (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);

	//restore self & other
	sv.script_globals->self = GENT_TO_PROG(oldSelf);
	sv.script_globals->other = GENT_TO_PROG(oldOther);
}


/*
====================
SV_ParseEntity

Parses an edict out of the given string, returning the new 
positioned should be a properly initialized empty edict.
Used for initial level loadand for savegames.
====================
*/
char* SV_ParseEntity(char* data, gentity_t * ent)
{
	ddef_t* key;
	qboolean	init;
	char		keyname[256];
	char* token;
	qboolean		anglehack;

	init = false;

	// clear it
	if (ent != sv.edicts)
	{
		memset(&ent->v, 0, Scr_GetEntityFieldsSize());
		SV_InitEntity(ent);
	}

	// go through all the dictionary pairs
	while (1)
	{
		// parse key
		token = COM_Parse(&data);

		if (token[0] == '}')
			break;

		if (!data)
			Com_Error(ERR_DROP, "%s: EOF without closing brace\n", __FUNCTION__);

		// anglehack is to allow QuakeEd to write single scalar angles
		// and allow them to be turned into vectors. (FIXME...)
		if (!strcmp(token, "angle"))
		{
			strcpy(token, "angles");
			anglehack = true;
		}
		else
			anglehack = false;

		strcpy(keyname, token);

		// parse value
		token = COM_Parse(&data);
		if (!token)
			Com_Error(ERR_DROP, "%s: !token\n", __FUNCTION__);

		if (token[0] == '}')
			Com_Error(ERR_DROP, "%s: closing brace without data\n", __FUNCTION__);

		init = true;

		// skip utility coments
		if (keyname[0] == '_')
			continue;

		key = Scr_FindEntityField(keyname);
		if (!key)
		{
			Com_Printf("%s: \"%s\" is not a field\n", __FUNCTION__, keyname);
			//			if (strncmp(keyname, "sky", 3))
			//			{
			//				gi.dprintf("\"%s\" is not a field\n", keyname);
			//			}
			continue;
		}
		else

			if (anglehack)
			{
				char	temp[32];
				strcpy(temp, token);
				sprintf(token, "0 %s 0", temp);
			}

		if (!Scr_ParseEpair((void*)&ent->v, key, token))
			Com_Error(ERR_DROP, "%s: parse error", __FUNCTION__);
	}

	ent->inuse = init;
	return data;
}
/*
==============
SpawnEntities

Creates a server's entity / program execution context by parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities(char* mapname, char* entities, char* spawnpoint)
{
	gentity_t	*ent;
	int			inhibit, discard, total;
	char		*com_token;
	int			i;

//	Z_FreeTags(778);

	// set client fields on player ents
//	for (i = 0; i < sv_maxclients->value; i++)
//		EDICT_NUM(i+1)->client = game.clients + i;

	ent = NULL;
	inhibit = discard = total = 0;

	ent = NULL;
	i = 0;
	// parse ents
	while (1)
	{
		// parse the opening brace	
		com_token = COM_Parse(&entities);

		if (!entities)
			break;

		if (com_token[0] != '{')
			Com_Error(ERR_FATAL, "SpawnEntities: found %s when expecting {\n", com_token);

		if (!ent)
		{
			// the worldspawn
			ent = sv.edicts;
			ent->inuse = true;
		}
		else
		{
			ent = SV_SpawnEntity();
		}

		entities = SV_ParseEntity(entities, ent);
		//printf("ent %s\n", Scr_GetString(ent->v.classname));
		SV_CallSpawn(ent);

		//stats
		if (ent && ent->inuse)
			inhibit++;
		else
			discard++;
		total++;
	}
	Com_Printf("'%s' entities: %i inhibited, %i discarded (%i in map total)\n", sv.name, inhibit, discard, total);
	
#if 0	
	for (i = 0; i < 23; i++)
	{
		ent = EDICT_NUM(i);
		printf("inuse %i: %i %s\n", i, ent->inuse, Scr_GetString(ent->v.classname));
	}

	i = 1;	
	while (i < sv.num_edicts) 
	{
		ent = EDICT_NUM(i);
		if (ent && (ent->inuse != 0 || ent->inuse != 1))
			Com_Printf("Invalid entity %d\n", i);
		i++;
	}
#endif


}

