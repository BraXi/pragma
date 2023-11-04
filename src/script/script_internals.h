/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

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

#define	DEF_SAVEGLOBAL	(1<<15)

enum
{
#include "qc_opcodes.h"
};

// =============================================================

#define	MAX_PARMS		8

typedef struct
{
	int		first_statement;	// negative numbers are builtins
	int		parm_start;
	int		locals;				// total ints of parms + locals
	int		profile;			// runtime
	int		s_name;				// function name
	int		s_file;				// source file defined in
	int		numparms;
	byte	parm_size[MAX_PARMS];
} dfunction_t;



typedef struct statement_s
{
	unsigned short	op;
	short	a, b, c;
} dstatement_t;

typedef struct
{
	unsigned short	type;		// if DEF_SAVEGLOBGAL bit is set the variable needs to be saved in savegames
	unsigned short	ofs;
	int			s_name;
} ddef_t;


typedef struct
{
	void		(*func)(void);
	qboolean	devmode;
	pb_t		execon; // client, server or both
	char*		qcstring;
	struct builtin_t* next;
} builtin_t;

typedef struct
{
	int		version;
	int		crc;			// check of header file

	int		ofs_statements;
	int		numStatements;	// statement 0 is an error

	int		ofs_globaldefs;
	int		numGlobalDefs;

	int		ofs_fielddefs;
	int		numFieldDefs;

	int		ofs_functions;
	int		numFunctions;	// function 0 is an empty

	int		ofs_strings;
	int		numstrings;		// first string is a null string

	int		ofs_globals;
	int		numGlobals;

	int		entityfields;
} dprograms_t;


typedef struct
{
	ddef_t* pcache;
	char	field[SCR_MAX_FIELD_LEN];
} gefv_cache;

typedef struct
{
	int				s;
	dfunction_t		*f;
} prstack_t;



typedef struct qcvm_s
{
	scrvmtype_t		progsType;		// SCRVM_
	int				progsSize;
	dprograms_t		*progs;

	dfunction_t		*functions;
	char			*strings;		// ofset to first string
	ddef_t			*globalDefs;
	ddef_t			*fieldDefs;	
	dstatement_t	*statements;

//	sv_globalvars_t	*globals_struct;
	unsigned int	num_entities;		// number of allocated entities
	vm_entity_t		*entities;
	size_t			entity_size;		// size of single entity
	size_t			offsetToEntVars;	// *ptr + ofs = ent->v

	void			*globals_struct;	// sv_globalvars_t

	float			*globals;

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
}qcvm_t;

typedef struct
{
	int vmtype;
	const char* filename;
	int defs_crc_checksum;
	const char* name;
} qcvmdef_t;

extern builtin_t	*scr_builtins;
extern int			scr_numBuiltins;
extern qcvm_t		*active_qcvm;
extern const qcvmdef_t vmDefs[NUM_SCRIPT_VMS];

extern char* ScrInternal_String(int str);
extern void Scr_InitSharedBuiltins();
extern void CheckScriptVM(const char* func);