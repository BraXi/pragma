/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// scr_entity.c

//#include "../qcommon/qcommon.h"
//#include "scriptvm.h"

//#include "../server/sv_game.h"

#include "../server/server.h"
#include "script_internals.h"

#if 0
gentity_t* EDICT_NUM2(int n)
{
	if (n < 0 || n >= sv.max_edicts)
		Com_Error(ERR_DROP, "EDICT_NUM: bad number %i", n);
	return (gentity_t*)((byte*)sv.edicts + (n)*pr_edict_size);
}

int NUM_FOR_EDICT2(gentity_t* e)
{
	int		b;

	b = (byte*)e - (byte*)sv.edicts;
	b = b / pr_edict_size;

	if (b < 0 || b >= sv.num_edicts)
		Com_Error(ERR_DROP, "NUM_FOR_EDICT: bad pointer");
	return b;
}
#endif

ddef_t* Scr_FindEntityField(char* name);

#define TAG_LEVEL 778
/*
=============
ED_NewString
=============
*/
char* ED_NewString(char* string)
{
	char* newb, * new_p;
	int		i, l;

	l = strlen(string) + 1;

	newb = Z_TagMalloc(l, TAG_LEVEL);

	new_p = newb;

	for (i = 0; i < l; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}


#define	G_INT(o)			(*(int *)&ScriptVM->globals[o])

/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
qboolean ED_ParseEpair(void* base, ddef_t* key, char* s)
{
	int		i;
	char	string[128];
	ddef_t* def;
	char* v, * w;
	void* d;
	scr_func_t func;

	d = (void*)((int*)base + key->ofs);

	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(scr_string_t*)d = ED_NewString(s) - ScriptVM->strings;
		break;

	case ev_float:
		*(float*)d = atof(s);
		break;

	case ev_vector:
		strcpy(string, s);
		v = string;
		w = string;
		for (i = 0; i < 3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float*)d)[i] = atof(w);
			w = v = v + 1;
		}
		break;

	case ev_entity:
		*(int*)d = GENT_TO_PROG(EDICT_NUM(atoi(s)));
		break;

	case ev_field:
		def = Scr_FindEntityField(s);
		if (!def)
		{
			if (strncmp(s, "sky", 3) && strcmp(s, "fog"))
				Com_DPrintf(DP_ALL, "Can't find field %s\n", s);
			return false;
		}
		*(int*)d = G_INT(def->ofs);
		break;

	case ev_function:
		func = Scr_FindFunction(s);
		if (func == -1)
		{
			Com_Error(ERR_FATAL, "Can't find function %s\n", s);
			return false;
		}
		*(scr_func_t*)d = func;
		break;

	default:
		break;
	}
	return true;
}


/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
Used for initial level load and for savegames.
====================
*/
char* ED_ParseEdict(char* data, gentity_t* ent)
{
	ddef_t* key;
	qboolean	init;
	char		keyname[256];
	char* token;
	qboolean		anglehack;

	init = false;

	// clear it
	if (ent != sv.edicts)
		memset(&ent->v, 0, Scr_GetEntityFieldsSize());

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

		if (anglehack)
		{
			char	temp[32];
			strcpy(temp, token);
			sprintf(token, "0 %s 0", temp);
		}

		if (!ED_ParseEpair((void*)&ent->v, key, token))
			Com_Error(ERR_DROP, "%s: parse error", __FUNCTION__);
	}

	//	if (!init)
	ent->inuse = init;
	return data;
}
