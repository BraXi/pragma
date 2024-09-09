/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"
#include "../../script/script_internals.h" // for ddef_t

extern ddef_t* Scr_FindEntityField(char* name); // scr_main.c
extern qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s, int memtag); // scr_main.c

/*
=================
CG_FreeEntity
=================
*/
void CG_FreeEntity(clentity_t* self)
{
	if (!self)
	{
		Com_Error(ERR_DROP, "%s: NULL entity\n", __FUNCTION__);
		return;
	}

	if (self == cg.localEntities)
	{
		Com_Error(ERR_DROP, "%s: tried to remove entity 0\n", __FUNCTION__);
		return;
	}

	//CG_UnlinkLocalEntity(self);

	Scr_BindVM(VM_CLGAME);

	// remove references of self, other
	if (self != cg.localEntities)
	{
		// dereference self and other globals in script if they're us
		if (VM_TO_ENT(cg.script_globals->self) == self)
			cg.script_globals->self = ENT_TO_VM(cg.localEntities);

		if (VM_TO_ENT(cg.script_globals->self) == self)
			cg.script_globals->other = ENT_TO_VM(cg.localEntities);
	}


	if (self && self->inuse)
		cg.numActiveLocalEnts--;

	memset(self, 0, Scr_GetEntitySize());
	self->v.classname = Scr_SetString("free");
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
	//memset(&ent->v, 0, Scr_GetEntityFieldsSize()); // the size is always read from progs

	ent->number = NUM_FOR_ENT(ent);

	ent->v.classname = Scr_SetString("no_class");
	ent->v.scale = 1.0f;
	VectorSet(ent->v.color, 1.0f, 1.0f, 1.0f);
}

/*
=================
CG_SpawnLocalEntity
=================
*/
clentity_t* CG_SpawnLocalEntity()
{
	clentity_t* ent = NULL;
	int		entnum;

	if (cg.numActiveLocalEnts == cg.maxLocalEntities)
	{
		Com_DPrintf(DP_CGAME, "%s: no free local entities\n", __FUNCTION__);
		return NULL;
	}

	// find first free entity
	for (entnum = 1; entnum < cg.maxLocalEntities; entnum++)
	{
		ent = ENT_FOR_NUM(entnum);
		if (!ent->inuse)
			break;
	}

	cg.numActiveLocalEnts++;

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
	clentity_t* oldSelf, * oldOther;
	char* classname;
	scr_func_t	spawnfunc;

	static char spawnFuncName[64];

	Scr_BindVM(VM_CLGAME);

	classname = Scr_GetString(ent->v.classname);

	if (strlen(classname) > 60)
	{
		printf("%s: classname '%s' is too long\n", __FUNCTION__, classname);
		return;
	}

	// check if someone is trying to spawn world...
	oldSelf = ENT_FOR_NUM(0); // abuse var
	if (NUM_FOR_ENT(ent) > 0 && oldSelf->inuse && stricmp(classname, "worldspawn") == 0)
	{
		Com_Error(ERR_DROP, "%s: only one worldspawn allowed\n", __FUNCTION__, classname);
		return;
	}

	if (ent == cg.localEntities)
	{
		// worldspawn hack
		ent->inuse = 1;
		ent->v.modelindex = 1;
		cg.numActiveLocalEnts++;
	}

	// find spawn fuction in progs
	sprintf(spawnFuncName, "SP_%s", classname);
	spawnfunc = Scr_FindFunction(spawnFuncName);
	if (spawnfunc == -1 && ent != cg.localEntities)
	{
		//		Com_DPrintf(DP_CGAME, "%s: unknown classname '%s'\n", __FUNCTION__, classname);
		CG_FreeEntity(ent);
		return;
	}

	// backup 'self' and 'other' and call spawn function
	oldSelf = VM_TO_ENT(cg.script_globals->self);
	oldOther = VM_TO_ENT(cg.script_globals->other);

	cg.script_globals->self = ENT_TO_VM(ent);
	cg.script_globals->other = ENT_TO_VM(cg.localEntities);
	Scr_Execute(VM_CLGAME, spawnfunc, __FUNCTION__);

	if (ent != cg.localEntities && (Scr_GetReturnFloat() <= 0 || spawnfunc == -1))
	{
		// if returned value from prog is false we delete entity right now (unless its world)
		Com_DPrintf(DP_CGAME, "discarded local entity \"%s\" at (%i %i %i)\n", classname, (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);
		CG_FreeEntity(ent);
		oldSelf = cg.localEntities;
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

	if (ent != cg.localEntities)
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

		//if (keyname[0] == '_') // commented out for cgame because sky settings are good to be know
		//	continue; // skip utility coments

		key = Scr_FindEntityField(keyname);
		if (!key)
		{
			// don't spam cgame
			//Com_Printf("%s: \"%s\" is not a field\n", __FUNCTION__, keyname); 
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

		if (!ent)
		{
			// the worldspawn
			ent = cg.localEntities;
			ent->inuse = true;
		}
		else
		{
			ent = CG_SpawnLocalEntity();
		}


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