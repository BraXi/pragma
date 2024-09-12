/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
// sv_gentity.c

#include "server.h"
#include "../script/qcvm_private.h" // HAAAACK

extern ddef_t* Scr_FindEntityField(char* name); //scr_main.c
extern qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s, int memtag); //scr_main.c

/*
=================
SV_InitEntity

Initializes newly spawned gentity
=================
*/
void SV_InitEntity(gentity_t* ent)
{
	ent->inuse = true;

	Scr_BindVM(VM_SVGAME);
	memset(&ent->s, 0, sizeof(entity_state_t));
	memset(&ent->v, 0, Scr_GetEntityFieldsSize()); // the size is always read from progs

	ent->s.number = NUM_FOR_EDICT(ent);

	ent->v.classname = Scr_SetString("no_class");
	ent->v.gravity = 1.0;
	ent->v.groundentity_num = ENTITYNUM_NULL;

	ent->v.renderScale = 1.0f;

	ent->v.nodeIndex = ENTITYNUM_NULL;

	ent->teamchain = ent->teammaster = NULL;
	ent->bEntityStateForClientChanged = false;
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
		Com_Error(ERR_DROP, "SV_SpawnEntity: no free entities, hit limit of %i entities\n", sv.max_edicts);
	
	SV_InitEntity(ent);
	return ent;
}

/*
=================
SV_FreeEntity

Marks the entity as free
=================
*/
void SV_FreeEntity(gentity_t* self)
{
	if (!self)
	{
		Com_Error(ERR_DROP, "SV_FreeEntity: !ent\n");
		return; //msvc..
	}

	if (self == sv.edicts)
	{
		Com_Error(ERR_DROP, "SV_FreeEntity: tried to remove world!\n");
		return;
	}

	if (NUM_FOR_EDICT(self) <= sv_maxclients->value)
	{
		Com_DPrintf(DP_SV, "tried to free client entity\n");
		return;
	}

	SV_UnlinkEdict(self);

	Scr_BindVM(VM_SVGAME);

	//
	// walk all entities and remove references of self, other and owner
	//
	if (self != sv.edicts)
	{
		// dereference self and other globals in script if they're us
		if (PROG_TO_GENT(sv.script_globals->self) == self)
			sv.script_globals->self = GENT_TO_PROG(sv.edicts);
		if (PROG_TO_GENT(sv.script_globals->other) == self)
			sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	}

	for (int i = 1; i < sv.num_edicts; i++)
	{
		gentity_t* ent = ENT_FOR_NUM(i);
		if (VM_TO_ENT(ent->v.owner) == self)
			ent->v.owner = ENT_TO_VM(sv.edicts);
	}

	if(self && self->inuse)
		sv.num_edicts--;

	memset(self, 0, Scr_GetEntitySize());

	self->v.classname = Scr_SetString("freed");
	self->freetime = sv.gameTime;
	self->inuse = false;
}

/*
================
SV_RunEntity
================
*/
void SV_RunEntity(gentity_t* ent)
{
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
areatype could either be AREA_SOLID, AREA_TRIGGERS or AREA_PATHNODES
Returns the number of touched ents
============
*/
int SV_TouchEntities(gentity_t* ent, int areatype)
{
	int			i, num, touched;
	gentity_t* touch[MAX_GENTITIES], * hit;
	trace_t		clip;

#if 0
	if (areatype == AREA_PATHNODES && !((int)ent->v.svflags & SVF_MONSTER))
		return;
#endif

	num = SV_AreaEntities(ent->v.absmin, ent->v.absmax, touch, MAX_GENTITIES, areatype);
	touched = 0;
	// be careful, it is possible to have an entity in this list removed before we get to it (killtriggered)
	for (i = 0; i < num; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;

		if (areatype == AREA_TRIGGERS && (int)hit->v.modelindex > 0)
		{
			clip = SV_Clip(hit, ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, ent->v.clipmask);
			if (clip.fraction == 1.0f)
				continue;
		}

		touched++;
		Scr_Event_Touch(hit, ent, NULL, NULL);
	}

	return touched;
}


/*
===============
SV_CallSpawnForEntity

Finds the spawn function for the entity and calls it
===============
*/
void SV_CallSpawnForEntity(gentity_t* ent)
{
	gentity_t	*oldSelf, *oldOther;
	char		*classname;
	scr_func_t	spawnfunc;

	static char spawnFuncName[64];
	
	classname = Scr_GetString(ent->v.classname);

	if (strlen(classname) > 60)
	{
		printf("SV_CallSpawnForEntity: classname '%s' is too long\n", classname);
		return;
	}

	// check if someone is trying to spawn world...
	if( NUM_FOR_ENT(ent) > 0 && EDICT_NUM(0)->inuse && stricmp(classname, "worldspawn") == 0 )
	{
		Com_Error(ERR_DROP, "SV_CallSpawnForEntity: only one worldspawn allowed\n", classname);
		return;
	}

	if (ent == sv.edicts)
	{
		// worldspawn hack
		ent->inuse = 1;
		ent->v.modelindex = 1;
		sv.num_edicts++;
	}

	// remove utility entities straight away
	if (stricmp(classname, "info_null") == 0 || stricmp(classname, "func_group") == 0)
	{
		SV_FreeEntity(ent);
		return;
	}

	// find spawn fuction in progs
	sprintf(spawnFuncName, "SP_%s", classname);
	spawnfunc = Scr_FindFunction(spawnFuncName);
	if (spawnfunc == -1 && ent != sv.edicts)
	{
		Com_DPrintf( DP_SV, "SV_CallSpawnForEntity: unknown classname '%s'\n", classname);
		SV_FreeEntity(ent);
		return;
	}

	// backup script's 'self' and 'other'
	oldSelf = PROG_TO_GENT(sv.script_globals->self);
	oldOther = PROG_TO_GENT(sv.script_globals->other);

	// call spawn function
	sv.script_globals->self = GENT_TO_PROG(ent);
	sv.script_globals->other = GENT_TO_PROG(sv.edicts);
	Scr_Execute(VM_SVGAME, spawnfunc, __FUNCTION__);

	// an entity may refuse to spawn (requires coop, certain skill level, etc..)
	// if returned value from prog is false we delete entity right now (unless its world)
	if (ent != sv.edicts && (Scr_GetReturnFloat() <= 0 || spawnfunc == -1))
	{
//		printf("CallSpawn: \"%s\" at (%i %i %i) discarded\n", classname, (int)ent->v.origin[0], (int)ent->v.origin[1], (int)ent->v.origin[2]);
		SV_FreeEntity(ent);
		oldSelf = sv.edicts;
	}

	//restore self & other globals
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
	ddef_t		*key;
	qboolean	init = false;
	char		keyname[256];
	char		*token;
	qboolean	anglehack;


	if (ent != sv.edicts)
	{
		SV_InitEntity(ent);
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

		if (keyname[0] == '_')
			continue; // skip utility coments

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
		else if (anglehack && token)
		{
			char	temp[32];
			strcpy(temp, token);
			sprintf(token, "0 %s 0", temp);
		}

		if (!Scr_ParseEpair((void*)&ent->v, key, token, TAG_SERVER_GAME))
			Com_Error(ERR_DROP, "%s: parse error", __FUNCTION__);
	}

	ent->inuse = init;
	return data;
}
/*
==============
SV_SpawnEntities

Creates a server's entity / program execution context by parsing textual entity definitions out of an ent file.
==============
*/
void SV_SpawnEntities(char* mapname, char* entities, char* spawnpoint)
{
	gentity_t	*ent;
	int			inhibit, discard, total;
	char		*com_token;
	int			i;

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
			Com_Error(ERR_FATAL, "%s: found %s when expecting {\n", __FUNCTION__, com_token);

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
		SV_CallSpawnForEntity(ent);

		//stats
		if (ent && ent->inuse)
			inhibit++;
		else
			discard++;
		total++;
	}
	Com_Printf("'%s' entities: %i inhibited, %i discarded (%i in map total)\n", sv.mapname, inhibit, discard, total);
}


/*
==============
SV_AttachModel
==============
*/
void SV_AttachModel(gentity_t *self, char* tagname, char *model)
{
	int i, tag;
	ent_model_t* attachInfo;
	svmodel_t* svmod;

	if (!self || !self->inuse)
		return;

	if (self == sv.edicts)
	{
		Com_Error(ERR_DROP, "tried to attach model to world entity\n");
		return;
	}

	svmod = SV_ModelForNum((int)self->v.modelindex);
	if (svmod == NULL || svmod->modelindex == 0)
	{
		Com_DPrintf(DP_GAME, "WARNING: entity %s has no model\n", Scr_GetString(self->v.classname));
		return;
	}

	if (svmod->numTags == 0)
	{
		Com_DPrintf(DP_GAME, "WARNING: entity %s has model without tags\n", Scr_GetString(self->v.classname));
		return;
	}

	svmod = SV_ModelForName(model);
	if (svmod == NULL)
	{
		Com_DPrintf(DP_GAME, "WARNING: cannot attach not precached model '%s'\n", model);
		return;
	}

	if (svmod->type == MOD_BRUSH)
	{
		Com_Error(ERR_DROP, "cannot attach brushmodels!\n");
		return;
	}

	tag = SV_TagIndexForName((int)self->v.modelindex, tagname);
	if (tag == -1)
	{
		Com_DPrintf(DP_GAME, "WARNING: cannot attach '%s' to entity %s (missing `%s` tag)\n", model, Scr_GetString(self->v.classname), tagname);
		return;
	}

	for (i = 0; i < MAX_ATTACHED_MODELS; i++)
	{
		if (self->s.attachments[i].modelindex == svmod->modelindex)
		{
			Com_DPrintf(DP_GAME, "WARNING: '%s' already attached to entity %s\n", model, Scr_GetString(self->v.classname));
			return;
		}
	}

	for (i = 0; i < MAX_ATTACHED_MODELS; i++)
	{
		attachInfo = &self->s.attachments[i];
		if (attachInfo->modelindex == 0)
		{ 
			attachInfo->modelindex = svmod->modelindex;
			attachInfo->parentTag = tag + 1; // must offset tag by 1 for network
			break;
		}
	}
}


/*
==============
SV_DetachModel
==============
*/
void SV_DetachModel(gentity_t* self, char* model)
{
	int i;
	ent_model_t* attachInfo;
	svmodel_t* svmod;

	if (!self || !self->inuse || self == sv.edicts)
		return;

	svmod = SV_ModelForName(model);
	for (i = 0; i < MAX_ATTACHED_MODELS; i++)
	{
		attachInfo = &self->s.attachments[i];
		if (attachInfo->modelindex != svmod->modelindex)
			continue;

		attachInfo->modelindex = 0;
		attachInfo->parentTag = 0;
		return;
	}

	Com_DPrintf(DP_GAME, "WARNING: model '%s' was not attached to entity %s\n", model, Scr_GetString(self->v.classname));
}

/*
==============
SV_DetachAllModels
==============
*/
void SV_DetachAllModels(gentity_t* self)
{
	int i;
	ent_model_t* attachInfo;

	if (!self || !self->inuse || self == sv.edicts)
		return;

	for (i = 0; i < MAX_ATTACHED_MODELS; i++)
	{
		attachInfo = &self->s.attachments[i];
		if (attachInfo->modelindex == 0)
			continue; // parentTag change will cause network broadcast

		attachInfo->modelindex = 0;
		attachInfo->parentTag = 0;
	}
}



/*
==============
SV_HideEntitySurface
==============
*/
void SV_HideEntitySurface(gentity_t* self, char* surfaceName)
{
	int surf;
	svmodel_t* svmod;

	if (!self || !self->inuse || self == sv.edicts)
		return;

	svmod = SV_ModelForNum((int)self->v.modelindex);
	if (svmod == NULL || svmod->type == MOD_BAD || svmod->modelindex == 0)
	{
		Com_DPrintf(DP_GAME, "WARNING: entity %s has no model (%s)\n", Scr_GetString(self->v.classname), __FUNCTION__);
		return;
	}

	if (svmod->type == MOD_BRUSH)
	{
		Com_DPrintf(DP_GAME, "WARNING: brushmodels have no parts! (entity %i)\n", NUM_FOR_ENT(self));
		return;
	}

	surf = SV_ModelSurfIndexForName((int)self->v.modelindex, surfaceName);
	if (surf != -1)
	{
		if(!(self->s.hidePartBits & (1 << surf))) // paranoid me
			self->s.hidePartBits |= (1 << surf);
	}
	else
	{
		Com_DPrintf(DP_GAME, "WARNING: model '%' has no surface '%s' (%s)\n", svmod->name, surfaceName, __FUNCTION__);
	}
}


/*
==============
SV_ShowEntitySurface

NULL surfaceName will show all parts
==============
*/
void SV_ShowEntitySurface(gentity_t* self, char* surfaceName)
{
	int surf;

	if (!self || !self->inuse)
		return;

	if (surfaceName == NULL)
	{
		self->s.hidePartBits = 0; // show all
	}
	else
	{
		surf = SV_ModelSurfIndexForName((int)self->v.modelindex, surfaceName);
		if (surf != -1)
		{
			int bit = (1 << surf);
			if ((self->s.hidePartBits & bit)) // paranoid me
				self->s.hidePartBits &= ~bit;
		}
	}
}


/*
==============
SV_EntityCanBeDrawn

returns false if all parts of a model are hidden or there's no model at all
==============
*/
qboolean SV_EntityCanBeDrawn(gentity_t* self)
{
	svmodel_t* svmod;
	int i, hidden;

	if ((int)self->v.modelindex <= 0)
		return false; // no modelindex

	svmod = SV_ModelForNum((int)self->v.modelindex);
	if (svmod == NULL) // should not happen but meh
	{
		Com_Error(ERR_DROP, "entity has modelindex > 0 but svmodel is NULL");
		return false; 
	}

	if ((self->v.renderFlags & RF_TRANSLUCENT) && self->v.renderAlpha <= 0)
		return false; // totally transparent!

	if (svmod->type == MOD_BRUSH)
		return true;

	if (self->s.hidePartBits <= 0)
		return true; // nothing was hidden

	if (svmod->numSurfaces > 0)
	{
		hidden = 0;
		for (i = 0; i < svmod->numSurfaces; i++)
		{
			if ((self->s.hidePartBits & (1 << i)))
				hidden++;
		}
		if (svmod->numSurfaces == hidden)
			return false; // all parts are hidden
	}

	return true; // visible
}