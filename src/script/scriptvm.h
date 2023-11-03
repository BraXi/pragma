/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#pragma once

#define SCRIPTVM_INSTRUCTIONS_LIMIT 100000	// number of instructions a single program can execute before it throws infinite loop error
#define SCRIPTVM_MAXBUILTINS		96		// maximum number of builtins FIXME -- get rid if this

#ifdef _DEBUG
	#define SCRIPTVM_PARANOID	1			// paranoia is a lifestyle, enable checks which shouldn't be in RELEASE
#endif


// FIXME -- move server and client stuff out of here
typedef struct gentity_s gentity_t;
#include "progdefs_server.h" 
typedef struct centity_s centity_t;
#include "progdefs_client.h" 


// used to define in which VM a builtin can be used
typedef enum
{
	PF_BOTH,		// can be called from anywhere
	PF_SV,			// server only
	PF_CL			// client only
} pb_t;

// script virtual machines
typedef enum
{
	SCRVM_NONE,		// no program execution at all
	SCRVM_SERVER,	// server
	SCRVM_CLIENT,	// client
	SCRVM_MENU,
	NUM_SCRIPT_VMS
} scrvmtype_t;


typedef union eval_s
{
	scr_string_t	string;
	float			_float;
	float			vector[3];
	scr_func_t		function;
	int				_int;
	scr_entity_t	edict;
} eval_t;


// scr_main.c

extern void Scr_CreateScriptVM(scrvmtype_t vm, int numEntities, int entitySize);
extern void Scr_FreeScriptVM(scrvmtype_t vm);
extern void Scr_BindVM(scrvmtype_t vm);
extern int Scr_GetEntitySize();
extern void* Scr_GetEntityPtr();
extern void* Scr_GetGlobals();
extern int Scr_GetEntityFieldsSize();

extern void Scr_Init();
extern void Scr_Shutdown();

// scr_debug.c
void Scr_StackTrace();

// scr_exec.c
extern void Scr_RunError(char* error, ...);
extern void Scr_Execute(scr_func_t fnum, char* callFromFuncName);
extern int Scr_NumArgs();
extern eval_t* Scr_GetEntityFieldValue(gentity_t* ed, char* field); // FIXME 


// scr_utils.c
extern void Scr_DefineBuiltin(void (*function)(void), pb_t type, qboolean devmode, char* qcstring);
extern scr_func_t Scr_FindFunction(char* funcname);
extern int Scr_SetString(char* str);
extern char* Scr_GetString(int num);
extern char* Scr_VarString(int first);


extern gentity_t* Scr_GetParmEdict(unsigned int parm);
extern float Scr_GetParmFloat(unsigned int parm);
extern int Scr_GetParmInt(unsigned int parm);
extern char* Scr_GetParmString(unsigned int parm);
extern float* Scr_GetParmVector(unsigned int parm);

extern float Scr_RetVal();

extern void Scr_ReturnEntity(void *ed);
extern void Scr_ReturnFloat(float val);
extern void Scr_ReturnString(char* str);
extern void Scr_ReturnVector(float* val);

extern void Scr_AddEntity(unsigned int parm, void* ed);
extern void Scr_AddFloat(unsigned int parm, float val);
extern void Scr_AddString(unsigned int parm, char* str);
extern void Scr_AddVector(unsigned int parm, float* vec);



#if 0
#define	GENT_TO_PROG(e) ((byte *)e - (byte *)sv.edicts)
#define PROG_TO_GENT(e) ((gentity_t *)((byte *)sv.edicts + e))
#define STRING_TO_PROG(str) ((str - ScriptVM->strings))
#define PROG_TO_STRING(str) ((ScriptVM->strings + str))

#define	NEXT_EDICT(e) ((gentity_t *)( (byte *)e + ScriptVM->gentity_size))
#endif



/*

.qc:

void(entity e, float t) testfunc = 
{ 
	e.time = t; 
	return 1337;
};

.c:
Scr_Init();
if( Scr_CreateScriptVM(SCRVM_SERVER) )
{
	if( !Scr_BindVM(SCRVM_SERVER) )
		return;

	scr_func_t test;
	if( (test = Scr_FindFunction("testfunc")) != -1 )
	{
		Scr_AddEntity( EDICT_NUM(0) );
		Scr_AddFloat( sv.time );

		Scr_Execute( test, __FUNCTION__ );

		printf("worldspawn.time=%f\n", Scr_GetEntityFieldValue(EDICT_NUM(0), "time")->_float );
		printf("return=%f\n", Scr_RetVal() );
	}
}


extern void Scr_DestroyScriptVM(scrvmtype_t vm);
extern qboolean Scr_BindVM(scrvmtype_t vm);
extern void Scr_Init();
extern void Scr_Shutdown();
*/