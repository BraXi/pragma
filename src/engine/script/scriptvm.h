/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _PRAGMA_QCVM_H_
#define _PRAGMA_QCVM_H_

#pragma once

#define VM_DEFAULT_RUNAWAY 500000	// number of instructions a single program can execute before it throws infinite loop error
									// use vm_runaway cvar

#define SCRIPTVM_MAXBUILTINS	200			// maximum number of builtins FIXME -- get rid if this

#ifdef _DEBUG
	#define SCRIPTVM_PARANOID	1			// paranoia is a lifestyle, enable checks which shouldn't be in RELEASE
#endif

extern cvar_t* vm_runaway;

typedef byte vm_entity_t;

typedef int32_t scr_func_t;
typedef int32_t scr_entity_t;
typedef int32_t scr_string_t;

#include "../client/cgame/progdefs_client.h" 
#include "progdefs_server.h" 
#include "progdefs_ui.h" 

// used to define in which VM a builtin can be used
// THESE MUST MATCH vmType_t
typedef enum
{
	PF_ALL,		// can be called from anywhere
	PF_SV,		// server only
	PF_CL,		// client only
	PF_GUI		// client GUI only
} pb_t;

// script virtual machines
typedef enum
{
	VM_NONE,	// no program execution at all
	VM_SVGAME,	// server game
	VM_CLGAME,	// client game
	VM_GUI,		// gui
	NUM_SCRIPT_VMS
} vmType_t;

typedef union eval_s
{
	scr_string_t	string;
	float			_float;
	float			vector[3];
	scr_func_t		function;
	int				_int;
	scr_entity_t	edict;
} eval_t;

typedef struct
{
	unsigned short	type;		// if DEF_SAVEGLOBGAL bit is set the variable needs to be saved in savegames
	unsigned short	ofs;
	int		s_name;
} ddef_t;


void Scr_CreateScriptVM(vmType_t vmType, unsigned int numEntities, size_t entitySize, size_t entvarOfs);
void Scr_FreeScriptVM(vmType_t vmType);
qboolean Scr_IsVMLoaded(vmType_t vmtype);
void Scr_BindVM(vmType_t vmType);
int Scr_GetEntitySize();
vm_entity_t* Scr_GetEntityPtr();
void* Scr_GetGlobals();
int Scr_GetEntityFieldsSize();
unsigned Scr_GetProgsCRC(vmType_t vmType);

void Scr_PreInitVMs();
void Scr_Shutdown();

// scr_debug.c

void Scr_StackTrace();

// scr_exec.c

void Scr_RunError(char* error, ...);

void Scr_Execute(vmType_t vm, scr_func_t fnum, char* callFromFuncName);

int Scr_NumArgs();

char *Scr_BuiltinFuncName();

eval_t* Scr_GetEntityFieldValue(vm_entity_t* ent, char* field); // FIXME 


// scr_utils.c

void Scr_DefineBuiltin(void (*function)(void), pb_t type, char* fname, char* qcstring);

scr_func_t Scr_FindFunctionIndex(const char* funcname);

int Scr_SetString(char* str);

char* Scr_GetString(int num);

char* Scr_VarString(int first);

vm_entity_t* Scr_GetParmEntity(unsigned int parm);
float Scr_GetParmFloat(unsigned int parm);
int Scr_GetParmInt(unsigned int parm);
char* Scr_GetParmString(unsigned int parm);
float* Scr_GetParmVector(unsigned int parm);
void Scr_GetParmVector2(unsigned int parm, float* x, float* y, float* z);

float Scr_GetReturnFloat();

void Scr_ReturnEntity(void *ed);
void Scr_ReturnFloat(float val);
void Scr_ReturnInt(int val);
void Scr_ReturnString(char* str);
void Scr_ReturnVector(float* val);

void Scr_AddEntity(unsigned int parm, vm_entity_t* ed);
void Scr_AddFloat(unsigned int parm, float val);
void Scr_AddInt(unsigned int parm, int val);
void Scr_AddString(unsigned int parm, char* str);
void Scr_AddVector(unsigned int parm, float* vec);


//
// MACROS FOR CONVINIENCE
// 

// pass entity to script vm
#define	ENT_TO_VM(ent)		((vm_entity_t*)ent - (vm_entity_t*)Scr_GetEntityPtr())

// grab entity from script vm
#define VM_TO_ENT(ent)		((vm_entity_t*)Scr_GetEntityPtr() + ent)

// grab entity by index
#define ENT_FOR_NUM(num)	((vm_entity_t*)Scr_GetEntityPtr() + Scr_GetEntitySize()*(num))

// grab next entity
#define	NEXT_ENT(ent)		((vm_entity_t*)ent + Scr_GetEntitySize())

// get entity index
#define NUM_FOR_ENT(ent)	( ((vm_entity_t*)(ent)- Scr_GetEntityPtr()) / Scr_GetEntitySize())

#endif /*_PRAGMA_QCVM_H_*/