/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// scr_utils.c

//#include "../qcommon/qcommon.h"
//#include "scriptvm.h"
//#include "script_internals.h"
//#include "../server/sv_game.h"

#include "../server/server.h"
#include "script_internals.h"


#define	G_INT(o)			(*(int *)&ScriptVM->globals[o])
#define	G_EDICT(o)			((gentity_t *)((byte *)sv.edicts + *(int *)&ScriptVM->globals[o]))
#define	G_FLOAT(o)			(ScriptVM->globals[o])
#define	G_STRING(o)			(ScriptVM->strings + *(scr_string_t *)&ScriptVM->globals[o])
#define	G_VECTOR(o)			(&ScriptVM->globals[o])
#define	RETURN_EDICT(e)		(((int *)ScriptVM->globals)[OFS_RETURN] = GENT_TO_PROG(e))


// unused
//#define E_FLOAT(e,o)		(((float*)&e->v)[o])
//#define E_INT(e,o)			(*(int *)&((float*)&e->v)[o])
//#define E_VECTOR(e,o)		(&((float*)&e->v)[o])
//#define E_STRING(e,o)		(ScriptVM->strings + *(string_t *)&((float*)&e->v)[o])
//#define E_STRING(e,o)		(Scr_GetString(*(string_t *)&((float*)&e->v)[o])) // QuakeWorld
#define	RETURN_STRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_SetString(s))


/*
============
Scr_DefineBuiltin

Adds new builtin method
============
*/
void Scr_DefineBuiltin(void (*function)(void), pb_t type, qboolean devmode, char* qcstring)
{
	builtin_t* func;

	if (scr_numBuiltins == 0)
	{
		scr_builtins = Z_Malloc(sizeof(builtin_t) * SCRIPTVM_MAXBUILTINS);
	}

	if (scr_numBuiltins == (SCRIPTVM_MAXBUILTINS - 1))
		Com_Error(ERR_FATAL, "increase pr_maxbuiltins\n");

	func = &scr_builtins[scr_numBuiltins];
	func->devmode = devmode;
	func->execon = type;
	func->func = function;
	func->qcstring = qcstring;
	scr_numBuiltins++;
}

dfunction_t* ScrInternal_FindFunction(char* name);

/*
============
Scr_FindFunction

Finds function in progs and returns its number (for execution), returns -1 when not found
============
*/
scr_func_t Scr_FindFunction(char* funcname)
{
	dfunction_t* f = NULL;
	if (funcname && funcname != "" && (f = ScrInternal_FindFunction(funcname)) != NULL)
		return (scr_func_t)(f - ScriptVM->functions);
	return -1;
}

/*
============
ScrInternal_String

Returns string from script
============
*/
char* ScrInternal_String(int str)
{
	return ScriptVM->strings + str;
}

/*
============
Scr_GetString

Returns string from script
============
*/
char* Scr_GetString(int str)
{
	return ScriptVM->strings + str;
}

/*
============
Scr_SetString

Sets string in script
============
*/
int Scr_SetString(char* str)
{
	return (int)(str - ScriptVM->strings);
}

/*
============
ScrInternal_GetParmOffset

Returns the correct offset for param number
============
*/
static int ScrInternal_GetParmOffset(unsigned int parm)
{
	int ofs = OFS_PARM0 + (parm * 3);
	if (ofs > OFS_PARM7 )//|| parm < OFS_PARM0)
		Com_Error(ERR_FATAL, "ScrInternal_GetParmOffset: parm %i is out of range [0,7]\n", parm);
	return ofs;
}

/*
============
Scr_VarString
============
*/
char* Scr_VarString(int first)
{
	int		i, param;
	static char out[256];

	out[0] = 0;
	for (i = first; i < Scr_NumArgs(); i++)
	{
		param = ScrInternal_GetParmOffset(i);
		strcat(out, G_STRING(param));
	}
	return out;
}

#define	G_INT(o) (*(int *)&ScriptVM->globals[o])
int* Scr_GetGlobal(int num)
{
	return G_INT(ScrInternal_GetParmOffset(num));
}

/*
============
Scr_GetParmEdict

Returns param as server edict
============
*/
gentity_t *Scr_GetParmEdict(unsigned int parm)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	return G_EDICT(ofs);
}

/*
============
Scr_GetParmFloat

Returns param as a float
============
*/
float Scr_GetParmFloat(unsigned int parm)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	return G_FLOAT(ofs);
}

/*
============
Scr_GetParmString

Returns param as a string
============
*/
char* Scr_GetParmString(unsigned int parm)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	return G_STRING(ofs);
}

/*
============
Scr_GetParmVector

Returns param as a vector
============
*/
float* Scr_GetParmVector(unsigned int parm)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	return G_VECTOR(ofs);
}

/*
============
Scr_RetVal

Grabs return value from script
============
*/
float Scr_RetVal()
{
	return ScriptVM->globals[OFS_RETURN];
}

/*
============
Scr_ReturnEdict

Returns entity to script
============
*/
void Scr_ReturnEntity(void *ed)
{
	RETURN_EDICT(ed);
}

/*
============
Scr_ReturnFloat

Returns float to script
============
*/
void Scr_ReturnFloat(float val)
{
	G_FLOAT(OFS_RETURN) = val;
}

/*
============
Scr_ReturnString

Returns string to script
============
*/
void Scr_ReturnString(char* str)
{
	G_INT(OFS_RETURN) = (str - ScriptVM->strings);
}

/*
============
Scr_ReturnVector

Returns vector to script
============
*/
void Scr_ReturnVector(float* val)
{
	int ofs = OFS_RETURN;
	G_FLOAT(ofs) = val[0];
	G_FLOAT(ofs + 1) = val[1];
	G_FLOAT(ofs + 2) = val[2];
}

/*
============
Scr_AddEntity

Add entity to call args
============
*/
void Scr_AddEntity(unsigned int parm, void* ed)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	G_INT(ofs) = GENT_TO_PROG(ed);
}


/*
============
Scr_AddFloat

Add float to call args
============
*/
void Scr_AddFloat(unsigned int parm, float val)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	G_FLOAT(ofs) = val;
}

/*
============
Scr_AddString

Add string to call args
============
*/
void Scr_AddString(unsigned int parm, char* str)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	G_INT(ofs) = Scr_SetString(str);
}


/*
============
Scr_AddVector

Add vector to call args
============
*/
void Scr_AddVector(unsigned int parm, float* vec)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	G_FLOAT(ofs) = vec[0];
	G_FLOAT(ofs + 1) = vec[1];
	G_FLOAT(ofs + 2) = vec[2];
}