/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _PRAGMA_QCVM_PRIVATE_H_
#define _PRAGMA_QCVM_PRIVATE_H_

#pragma once

#include "scriptvm.h" // in case..

typedef enum 
{ 
	// vanilla progs
	ev_void,
	ev_string, 
	ev_float, 
	ev_vector, 
	ev_entity, 
	ev_field,
	ev_function,
	ev_pointer,
	// FTE's extended types
	ev_integer,	
	ev_uint,
	ev_int64,
	ev_uint64,
	ev_double
} 
etype_t;

#define	SCR_MAX_FIELD_LEN		64
#define SCR_GEFV_CACHESIZE		2
#define	SCR_MAX_STACK_DEPTH		32
#define	SCR_LOCALSTACK_SIZE		2048

#define	PROG_VERSION		6
#define	PROG_VERSION_FTE	7

#define	OFS_NULL			0
#define	OFS_RETURN			1
#define	OFS_PARM0			4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1			7
#define	OFS_PARM2			10
#define	OFS_PARM3			13
#define	OFS_PARM4			16
#define	OFS_PARM5			19
#define	OFS_PARM6			22
#define	OFS_PARM7			25
#define	RESERVED_OFS		28

#define	MAX_PARMS		8

#define	DEF_SAVEGLOBAL	(1<<15)

enum
{
#include "qc_opcodes.h"
};

// =============================================================



typedef struct
{
	int32_t		first_statement;	// negative numbers are builtins
	int32_t		parm_start;
	int32_t		locals;				// total ints of parms + locals
	int32_t		profile;			// runtime
	int32_t		s_name;				// function name
	int32_t		s_file;				// source file defined in
	int32_t		numparms;
	byte		parm_size[MAX_PARMS];
} dfunction_t;


typedef struct statement_s
{
	uint16_t	op;
	int16_t	a, b, c;
} dstatement_t;


typedef struct
{
	void		(*func)(void);
	qboolean	devmode;
	pb_t		execon; // client, server, gui or both
	char*		qcstring;
	char* name;
	struct builtin_t* next;
} builtin_t;

typedef struct
{
	int32_t		version;
	int32_t		crc;			// check of header file

	int32_t		ofs_statements;
	int32_t		numStatements;	// statement 0 is an error

	int32_t		ofs_globaldefs;
	int32_t		numGlobalDefs;

	int32_t		ofs_fielddefs;
	int32_t		numFieldDefs;

	int32_t		ofs_functions;
	int32_t		numFunctions;	// function 0 is an empty

	int32_t		ofs_strings;
	int32_t		numstrings;		// first string is a null string

	int32_t		ofs_globals;
	int32_t		numGlobals;

	int32_t		entityfields;

	//debug / version 7 extensions
	unsigned int		ofsfiles;	//non list format. no comp
	unsigned int		ofslinenums;	//numstatements big	//comp 64
	unsigned int		ofsbodylessfuncs;	//no comp
	unsigned int		numbodylessfuncs;

	unsigned int	ofs_types;	//comp 128
	unsigned int	numtypes;
	unsigned int	blockscompressed;

	int	secondaryversion;
} dprograms_t;


typedef struct
{
	ddef_t* pcache;
	char	field[SCR_MAX_FIELD_LEN];
} gefv_cache;

typedef struct
{
	int32_t			s;
	dfunction_t		*f;
} prstack_t;

#define PR_TEMP_STRINGS 32 // number of temporary string buffers
#define	PR_TEMP_STRING_LEN 512 // length of a temporary string buffer

typedef struct qcvm_strings_s
{
	const char** stringTable;
	int stringTableSize;
	int numStringsInTable;

	char tempStrings[PR_TEMP_STRINGS][PR_TEMP_STRING_LEN];
	int numTempStrings;
}qcvm_strings_t;

typedef struct qcvm_s
{
	vmType_t		progsType;		// SCRVM_
	int				progsSize;
	dprograms_t		*progs;

	dfunction_t		*pFunctions;
	char			*pStrings;		// ofset to first string
	ddef_t			*pGlobalDefs;
	ddef_t			*pFieldDefs;	
	dstatement_t	*pStatements;

//	sv_globalvars_t	*globals_struct;
	unsigned int	num_entities;		// number of allocated entities
	vm_entity_t		*entities;
	int				entity_size;		// size of single entity
	int				offsetToEntVars;	// *ptr + ofs = ent->v

	void			*pGlobalsStruct;	// sv_globalvars_t

	float			*pGlobals;

	unsigned short	crc;			// crc checksum of entire progs file

	gefv_cache		gefvCache[SCR_GEFV_CACHESIZE];

	qboolean		traceEnabled;

	prstack_t		stack[SCR_MAX_STACK_DEPTH];
	int				stackDepth;

	int				localstack[SCR_LOCALSTACK_SIZE];
	int				localstack_used;

	dfunction_t		*xfunction;
	int				xstatement;
	int				argc;

	int				runawayCounter;	// runaway loop counter
	int				pr_numparms;

	char			*callFromFuncName;			// printtrace
	builtin_t		*currentBuiltinFunc; // development aid

	FILE			*logfile;


	qcvm_strings_t	strTable;
}qcvm_t;

typedef struct
{
	int vmtype;
	char* filename;
	int defs_crc_checksum;
	const char* name;
} qcvmdef_t;

extern builtin_t	*scr_builtins;
extern int			scr_numBuiltins;
extern qcvm_t		*active_qcvm;
extern const qcvmdef_t vmDefs[NUM_SCRIPT_VMS];

void Scr_InitSharedBuiltins();
void CheckScriptVM(const char* func);

dfunction_t* Scr_FindFunction(const char* name);


#define	G_INT(o)			(*(int *)&active_qcvm->pGlobals[o])
#define	G_ENTITY(o)			((vm_entity_t *)((byte *)active_qcvm->entities + *(int *)&active_qcvm->pGlobals[o]))
#define	G_FLOAT(o)			(active_qcvm->pGlobals[o])
//#define	G_STRING(o)			(Scr_GetString((scr_string_t)active_qcvm->pGlobals[o])) //(active_qcvm->pStrings + *(int *)&active_qcvm->pGlobals[o])
#define G_STRING(o)			(Scr_GetString(*(scr_string_t*)&active_qcvm->pGlobals[o]))
#define	G_VECTOR(o)			(&active_qcvm->pGlobals[o])
#define	RETURN_EDICT(e)		(((int *)active_qcvm->pGlobals)[OFS_RETURN] = ENT_TO_VM(e))


#endif /*_PRAGMA_QCVM_PRIVATE_H_*/