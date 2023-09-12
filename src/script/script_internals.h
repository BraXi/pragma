/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

typedef enum { ev_void, ev_string, ev_float, ev_vector, ev_entity, ev_field, ev_function, ev_pointer } etype_t;

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
	OP_DONE,
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,
	OP_DIV_F,
	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_E,
	OP_EQ_FNC,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_E,
	OP_NE_FNC,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_LOAD_F,
	OP_LOAD_V,
	OP_LOAD_S,
	OP_LOAD_ENT,
	OP_LOAD_FLD,
	OP_LOAD_FNC,

	OP_ADDRESS,

	OP_STORE_F,
	OP_STORE_V,
	OP_STORE_S,
	OP_STORE_ENT,
	OP_STORE_FLD,
	OP_STORE_FNC,

	OP_STOREP_F,
	OP_STOREP_V,
	OP_STOREP_S,
	OP_STOREP_ENT,
	OP_STOREP_FLD,
	OP_STOREP_FNC,

	OP_RETURN,
	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_ENT,
	OP_NOT_FNC,
	OP_IF,
	OP_IFNOT,
	OP_CALL0,
	OP_CALL1,
	OP_CALL2,
	OP_CALL3,
	OP_CALL4,
	OP_CALL5,
	OP_CALL6,
	OP_CALL7,
	OP_CALL8,
	OP_STATE,
	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR
};

//char* pr_opnames[];

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
	void (*func)(void);
	qboolean devmode;
	pb_t execon; // client, server or both
	char* qcstring;
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
	dfunction_t* f;
} prstack_t;



typedef struct qcvm_s
{
	scrvmtype_t		progsType;		// SCRVM_CLIENT or SCRVM_SERVER
	dprograms_t		*progs;

	dfunction_t		*functions;
	char			*strings;		// ofset to first string
	ddef_t			*globalDefs;
	ddef_t			*fieldDefs;	
	dstatement_t	*statements;

	sv_globalvars_t	*globals_struct;

	float			*globals;
	int				entity_size;	// size in bytes

	unsigned short	crc;			// crc checksum

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

	char* callFromFuncName;			// printtrace
}qcvm_t;


extern builtin_t	*scr_builtins;
extern int			scr_numBuiltins;
extern qcvm_t		*ScriptVM;

extern char* ScrInternal_String(int str);
extern void Scr_InitSharedBuiltins();