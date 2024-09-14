/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/


#include "../server/server.h"
#include "qcvm_private.h"


/*
============
Scr_DefineBuiltin
Adds new builtin function
============
*/
void Scr_DefineBuiltin(void (*function)(void), pb_t type, char* fname, char* qcstring)
{
	builtin_t* func;

	if (scr_numBuiltins == 0)
	{
		scr_builtins = Z_Malloc(sizeof(builtin_t) * SCRIPTVM_MAXBUILTINS);
	}

	if (scr_numBuiltins == (SCRIPTVM_MAXBUILTINS - 1))
		Com_Error(ERR_FATAL, "Too manu builtins, increase SCRIPTVM_MAXBUILTINS\n");

	func = &scr_builtins[scr_numBuiltins];
	func->execon = type;
	func->func = function;
	func->name = fname;
	func->qcstring = qcstring;
	scr_numBuiltins++;
}



/*
============
Scr_FindFunctionIndex
Finds function in progs and returns its number (for execution), returns -1 when function was not found.
============
*/
scr_func_t Scr_FindFunctionIndex(const char* funcname)
{
	dfunction_t* f = NULL;

	if (!funcname || !funcname[0])
	{
		return -1;
	}
		
	if((f = Scr_FindFunction(funcname)) != NULL)
		return (scr_func_t)(f - active_qcvm->pFunctions);

	return -1;
}



/*
============
ScrInternal_GetParmOffset
Returns the correct offset for param number
============
*/
int ScrInternal_GetParmOffset(unsigned int parm)
{
	int ofs = OFS_PARM0 + (parm * 3);

	if (ofs > OFS_PARM7 )//|| parm < OFS_PARM0)
		Com_Error(ERR_FATAL, "ScrInternal_GetParmOffset: parm %i is out of range [0,7]\n", parm);

	return ofs;
}

/*
============
Scr_GetParmEntity

Returns param as server edict
============
*/
vm_entity_t *Scr_GetParmEntity(unsigned int parm)
{
	int ofs = ScrInternal_GetParmOffset(parm);

	CheckScriptVM(__FUNCTION__);
//	return ((vm_entity_t*)((byte*)active_qcvm->entities + *(int*)&active_qcvm->globals[ofs]));
	return G_ENTITY(ofs);
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

	CheckScriptVM(__FUNCTION__);
	return G_FLOAT(ofs);
}

/*
============
Scr_GetParmFloat

Returns param as a int
============
*/
int Scr_GetParmInt(unsigned int parm)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	CheckScriptVM(__FUNCTION__);
	return G_INT(ofs);
}


/*
============
Scr_GetParmString

Returns param as a string
============
*/
const char* Scr_GetParmString(unsigned int parm)
{
	int32_t ofs = ScrInternal_GetParmOffset(parm);
	CheckScriptVM(__FUNCTION__);
	const char* str = G_STRING(ofs);
	return str;
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
	CheckScriptVM(__FUNCTION__);
	return G_VECTOR(ofs);
}

/*
============
Scr_GetParmVector2
Returns param as a vector and stores them into three floats
============
*/
void Scr_GetParmVector2(unsigned int parm, float *x, float *y, float *z)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	CheckScriptVM(__FUNCTION__);
	*x = G_VECTOR(ofs)[0];
	*y = G_VECTOR(ofs)[1];
	*z = G_VECTOR(ofs)[2];
}

/*
============
Scr_GetReturnFloat

Grabs returned float from script
============
*/
float Scr_GetReturnFloat()
{
	CheckScriptVM(__FUNCTION__);
	return active_qcvm->pGlobals[OFS_RETURN];
}

/*
============
Scr_ReturnEdict

Returns entity to script
============
*/
void Scr_ReturnEntity(void *ed)
{
	CheckScriptVM(__FUNCTION__);
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
	CheckScriptVM(__FUNCTION__);
	G_FLOAT(OFS_RETURN) = val;
}

/*
============
Scr_ReturnInt
Returns int to script
============
*/
void Scr_ReturnInt(int val)
{
	CheckScriptVM(__FUNCTION__);
	G_FLOAT(OFS_RETURN) = val;

	// this should be G_INT? i remember there was some problem with it, making this a commit to test later
	//G_INT(OFS_RETURN) = val; 
}

/*
============
Scr_ReturnString

Returns string to script
============
*/
void Scr_ReturnString(char* str)
{
	CheckScriptVM(__FUNCTION__);
	G_INT(OFS_RETURN) = Scr_SetString(str);//(str - active_qcvm->pStrings);
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

	CheckScriptVM(__FUNCTION__);
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
void Scr_AddEntity(unsigned int parm, vm_entity_t* ed)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	CheckScriptVM(__FUNCTION__);
	G_INT(ofs) = ENT_TO_VM(ed);
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
	CheckScriptVM(__FUNCTION__);
	G_FLOAT(ofs) = val;
}

/*
============
Scr_AddFloat

Add int to call args
============
*/
void Scr_AddInt(unsigned int parm, int val)
{
	int ofs = ScrInternal_GetParmOffset(parm);
	CheckScriptVM(__FUNCTION__);
	G_INT(ofs) = val;
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
	CheckScriptVM(__FUNCTION__);
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
	CheckScriptVM(__FUNCTION__);
	G_FLOAT(ofs) = vec[0];
	G_FLOAT(ofs + 1) = vec[1];
	G_FLOAT(ofs + 2) = vec[2];
}