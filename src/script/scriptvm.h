/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#pragma once

#define VM_DEFAULT_RUNAWAY 500000	// number of instructions a single program can execute before it throws infinite loop error
									// use vm_runaway cvar

#define SCRIPTVM_MAXBUILTINS	160			// maximum number of builtins FIXME -- get rid if this

#ifdef _DEBUG
	#define SCRIPTVM_PARANOID	1			// paranoia is a lifestyle, enable checks which shouldn't be in RELEASE
#endif

extern cvar_t* vm_runaway;

typedef byte vm_entity_t;

// FIXME -- move server and client stuff out of here
typedef struct gentity_s gentity_t;
#include "progdefs_server.h" 
typedef struct centity_s centity_t;
#include "progdefs_client.h" 
typedef struct centity_s centity_t;
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
	VM_GUI,		// client gui
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


extern void Scr_CreateScriptVM(vmType_t vmType, unsigned int numEntities, size_t entitySize, size_t entvarOfs);
extern void Scr_FreeScriptVM(vmType_t vmType);
extern void Scr_BindVM(vmType_t vmType);
extern int Scr_GetEntitySize();
extern vm_entity_t* Scr_GetEntityPtr();
extern void* Scr_GetGlobals();
extern int Scr_GetEntityFieldsSize();
extern unsigned Scr_GetProgsCRC(vmType_t vmType);

extern void Scr_PreInitVMs();
extern void Scr_Shutdown();

// scr_debug.c
void Scr_StackTrace();
// scr_exec.c
extern void Scr_RunError(char* error, ...);
extern void Scr_Execute(vmType_t vm, scr_func_t fnum, char* callFromFuncName);
extern int Scr_NumArgs();
extern eval_t* Scr_GetEntityFieldValue(vm_entity_t* ent, char* field); // FIXME 


// scr_utils.c
extern void Scr_DefineBuiltin(void (*function)(void), pb_t type, char* fname, char* qcstring);
extern scr_func_t Scr_FindFunction(char* funcname);
extern int Scr_SetString(char* str);
extern char* Scr_GetString(int num);
extern char* Scr_VarString(int first);


extern gentity_t* Scr_GetParmEdict(unsigned int parm);
extern float Scr_GetParmFloat(unsigned int parm);
extern int Scr_GetParmInt(unsigned int parm);
extern char* Scr_GetParmString(unsigned int parm);
extern float* Scr_GetParmVector(unsigned int parm);
extern void Scr_GetParmVector2(unsigned int parm, float* x, float* y, float* z);

extern float Scr_GetReturnFloat();

extern void Scr_ReturnEntity(void *ed);
extern void Scr_ReturnFloat(float val);
extern void Scr_ReturnInt(int val);
extern void Scr_ReturnString(char* str);
extern void Scr_ReturnVector(float* val);

extern void Scr_AddEntity(unsigned int parm, vm_entity_t* ed);
extern void Scr_AddFloat(unsigned int parm, float val);
extern void Scr_AddInt(unsigned int parm, int val);
extern void Scr_AddString(unsigned int parm, char* str);
extern void Scr_AddVector(unsigned int parm, float* vec);


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