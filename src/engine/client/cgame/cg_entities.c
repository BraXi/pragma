/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"
#include "../../script/qcvm_private.h" // for ddef_t

extern ddef_t* Scr_FindEntityField(char* name); // scr_main.c
extern qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s, int memtag); // scr_main.c

/*
	Networked Entities are indexed from 0 to MAX_GENTITIES
	they update at fixed intervals (1000ms/sv_fps), usualy 10, 20 or 40 times per second
	and are not allowed to be removed, unless server tells us to, they use server's PVS
	and thus will only appear in game if the server tells us to add them to scene

	Local Entities start at MAX_GENTITIES and end at [MAX_GENTITIES + MAX_LOCAL_ENTITIES]
	cl_entities[MAX_GENTITIES] == worldspawn, which is opposite to server where entity 0 is world
	local entities can be created from entity string from loading a level, and dynamicaly via script

	they are rendered independently from what server tells us and use local camera's PVS

	local entities DO NOT interact with networked entities, by default, and SHOULD NEVER be SOLID
	to the player, otherwise pmove prediction errors will happen because server doesn't know about them
	they can interact in a non blocking way

	already fixed:
	The first entity(0) will fail `if(entity)` statement in QCVM, because entity 0 is 
	considered "not entity", but with the current approach cl_entity[0] is usualy a player

*/

/*
=================
CG_IsNetworkedEntity
False if this is local entity, true if its networked.
=================
*/
qboolean CG_IsNetworkedEntity(const clentity_t* ent)
{
	Scr_BindVM(VM_CLGAME);
	if (NUM_FOR_ENT(ent) >= MAX_GENTITIES)
		return false;
	return true;
}

/*
=================
CG_FreeLocalEntity
=================
*/
void CG_FreeLocalEntity(clentity_t* self)
{
	if (!self)
	{
		Com_Error(ERR_DROP, __FUNCTION__": NULL entity\n");
		return;
	}

	if (CG_IsNetworkedEntity(self))
	{
		Com_Error(ERR_DROP, __FUNCTION__": tried to remove networked entity\n");
		return;
	}

	if (!self->inuse)
		return;

#if 0
	// FIXME: CLIENT PROGS
	// remove references of self, other
	if (self != cg.entities)
	{
		// dereference self and other globals in script if they're us
		if (VM_TO_ENT(cg.script_globals->self) == self)
			cg.script_globals->self = ENT_TO_VM(cg.entities); 

		if (VM_TO_ENT(cg.script_globals->self) == self)
			cg.script_globals->other = ENT_TO_VM(cg.entities);
	}
#endif

	cg.numLocalEntities--;

	memset(self, 0, Scr_GetEntitySize());
	self->v.classname = cg.cstr.free;
	self->inuse = false;
}

/*
=================
CG_InitEntity
=================
*/
void CG_InitEntity(clentity_t* ent)
{
	ent->inuse = true;

	Scr_BindVM(VM_CLGAME);
	memset(ent, 0, Scr_GetEntitySize());

	ent->number = NUM_FOR_ENT(ent);

	ent->v.classname = cg.cstr.no_class;
	ent->v.scale = 1.0f;
	VectorSet(ent->v.color, 1.0f, 1.0f, 1.0f);
}

/*
=================
CG_SpawnLocalEntity
Note: Local entities start from MAX_GENTITIES
=================
*/
clentity_t* CG_SpawnLocalEntity()
{
	clentity_t* ent = NULL;
	int		entnum;

	if (cg.numLocalEntities == cg.maxEntities)
	{
		Com_DPrintf(DP_CGAME, "%s: no free local entities\n", __FUNCTION__);
		return NULL;
	}

	// find first free entity
	for (entnum = 0; entnum < cg.maxEntities; entnum++)
	{
		ent = ENT_FOR_NUM(MAX_GENTITIES + entnum);
		if (!ent->inuse)
			break;
	}

	cg.numLocalEntities++;

	CG_InitEntity(ent);
	return ent;
}



/*
===============
CG_CallSpawnForEntity

Finds the spawn function for the entity and calls it
===============
*/
static void CG_CallSpawnForEntity(clentity_t* ent)
{
	clentity_t	*oldSelf, *oldOther;
	const char* classname;
	scr_func_t	spawnfunc;

	Scr_BindVM(VM_CLGAME);

	classname = Scr_GetString(ent->v.classname);
	spawnfunc = Scr_FindFunctionIndex(va("SP_%s", classname));
	if (spawnfunc == -1 && ent != cg.entities)
	{
		//Com_DPrintf(DP_CGAME, "%s: unknown classname '%s'\n", __FUNCTION__, classname);
		CG_FreeLocalEntity(ent);
		return;
	}

	// backup 'self' and 'other' and call spawn function
	oldSelf = VM_TO_ENT(cg.script_globals->self);
	oldOther = VM_TO_ENT(cg.script_globals->other);

	cg.script_globals->self = ENT_TO_VM(ent);
	cg.script_globals->other = ENT_TO_VM(ent);
	Scr_Execute(VM_CLGAME, spawnfunc, __FUNCTION__);

	if (Scr_GetReturnFloat() <= 0 || spawnfunc == -1)
	{
		// entity was discarded
		//Com_DPrintf(DP_CGAME, "discarded local entity \"%s\" at (%i %i %i)\n", classname, (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);
		CG_FreeLocalEntity(ent);
	}

	//restore self & other globals
	cg.script_globals->self = ENT_TO_VM(oldSelf);
	cg.script_globals->other = ENT_TO_VM(oldOther);
}

/*
====================
CG_ParseEntityFromString

Parses an entity out of the given string, returning the new
positioned should be a properly initialized empty entity.
Used for initial level load.
====================
*/
static char* CG_ParseEntityFromString(char* data, clentity_t* ent)
{
	ddef_t* key;
	qboolean	init = false;
	char		keyname[256];
	char* token;
	qboolean	anglehack;

	if (ent != cg.entities) // CLFIXME
	{
		CG_InitEntity(ent);
	}

	// go through all the dictionary pairs
	while (1)
	{
		token = COM_Parse(&data); // parse key
		if (token[0] == '}')
			break;

		if (!data)
			Com_Error(ERR_DROP, "%s: EOF without closing brace\n", __FUNCTION__);

		anglehack = false;
		if (!strcmp(token, "angle"))
		{
			// anglehack is to allow QuakeEd to write single scalar angles and allow them to be turned into vectors
			strcpy(token, "angles");
			anglehack = true;
		}

		strcpy(keyname, token);

		token = COM_Parse(&data); // parse value
		if (!token)
		{
			Com_Error(ERR_DROP, "%s: NULL token\n", __FUNCTION__);
			return NULL; //msvc
		}

		if (token[0] == '}')
			Com_Error(ERR_DROP, "%s: closing brace without data\n", __FUNCTION__);

		init = true;

		key = Scr_FindEntityField(keyname);
		if (!key)
		{
			continue;
		}
		else if (anglehack && token)
		{
			char	temp[32];
			strcpy(temp, token);
			sprintf(token, "0 %s 0", temp);
		}

		if (!Scr_ParseEpair((void*)&ent->v, key, token, TAG_CLIENT_GAME))
			Com_Error(ERR_DROP, "%s: parse error", __FUNCTION__);
	}

	ent->inuse = init;
	return data;
}

/*
==============
CG_SpawnEntities
==============
*/
void CG_SpawnEntities(char* mapname, char* entities)
{
	clentity_t* ent;
	int			inhibit, discard, total;
	char* com_token;
	int			i;

	ent = NULL;
	inhibit = discard = total = 0;

	i = 0;

	// parse ents
	while (1)
	{
		// parse the opening brace	
		com_token = COM_Parse(&entities);

		if (!entities)
			break;

		if (com_token[0] != '{')
			Com_Error(ERR_FATAL, "%s: found %s when expecting {\n", __FUNCTION__, com_token);


		ent = CG_SpawnLocalEntity();
		entities = CG_ParseEntityFromString(entities, ent);
		CG_CallSpawnForEntity(ent);

		//stats
		if (ent && ent->inuse)
			inhibit++;
		else
			discard++;
		total++;
	}
	Com_Printf("Inhibited %i local entities from entitystring, (%i discarded).\n", inhibit, discard, total);
}