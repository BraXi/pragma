/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// scr_debug.c

#include "../pragma.h"
#include "qcvm_private.h"


static int type_size[9] = 
{ 
	/*vanilla QC*/
	1,							// ev_void
	sizeof(scr_string_t) / 4,	// ev_string
	1,							// ev_float
	3,							// ev_vector
	1,							// ev_entity
	1,							// ev_field
	sizeof(scr_func_t) / 4,		// ev_function
	sizeof(void*) / 4,			// ev_pointer
	1							// FTEQC: ev_integer
};

ddef_t* ScrInternal_GlobalAtOfs(int ofs);
ddef_t* ScrInternal_FieldAtOfs(int ofs);

#define NO_QC_OPC "unsupported opcode"
char* qcvm_op_names[] = /* qc op names for debugging */
{
#include "qc_opnames.h"
};


/*
============
CheckScriptVM

for debugging
=============
*/
void CheckScriptVM(const char *func)
{
//#ifdef SCRIPTVM_PARANOID
	if (active_qcvm == NULL)
		Com_Error(ERR_FATAL, "Script VM is NULL in %s\n", func);
//#endif
}

/*
============
PR_ValueString

Returns a string describing *data in a type specific manner
=============
*/
char* Scr_ValueString(etype_t type, eval_t* val)
{
	static char	line[256];
	ddef_t* def;
	dfunction_t* f;

	type &= ~DEF_SAVEGLOBAL;

	CheckScriptVM(__FUNCTION__);

	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", Scr_GetString(val->string));
		break;
	case ev_entity:
		sprintf(line, "entity %i", (int)NUM_FOR_ENT(VM_TO_ENT(val->edict)));
		break;
	case ev_function:
		f = active_qcvm->pFunctions + val->function;
		sprintf(line, "%s()", Scr_GetString(f->s_name));
		break;
	case ev_field:
		def = ScrInternal_FieldAtOfs(val->_int);
		sprintf(line, ".%s", Scr_GetString(def->s_name));
		break;
	case ev_void:
		sprintf(line, "*void*");
		break;
	case ev_float:
		sprintf(line, "%f", val->_float);
		break;
	case ev_integer:
		sprintf(line, "%i", val->_int);
		break;
	case ev_vector:
		sprintf(line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		sprintf(line, "*pointer*");
		break;
	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
}


/*
============
Scr_ValueStringDeveloper

Returns a string describing *data in a type specific manner
=============
*/
char* Scr_ValueStringDeveloper(etype_t type, eval_t* val)
{
	static char	line[256];
	ddef_t* def;
	dfunction_t* f;

	type &= ~DEF_SAVEGLOBAL;

	CheckScriptVM(__FUNCTION__);

	switch (type)
	{
	case ev_string:
		sprintf(line, "'%s' [string]", Scr_GetString(val->string));
		break;
	case ev_entity:
		sprintf(line, "%i [entity]", (int)NUM_FOR_ENT(VM_TO_ENT(val->edict)));
		break;
	case ev_function:
		f = active_qcvm->pFunctions + val->function;
		sprintf(line, "%s [function]", Scr_GetString(f->s_name));
		break;
	case ev_field:
		def = ScrInternal_FieldAtOfs(val->_int);
		sprintf(line, "%s [field]", Scr_GetString(def->s_name));
		break;
	case ev_void:
		sprintf(line, "*void*");
		break;
	case ev_float:
		sprintf(line, "%f [float]", val->_float);
		break;
	case ev_integer:
		sprintf(line, "%i [int]", val->_int);
		break;
	case ev_vector:
		sprintf(line, "%f %f %f [vector]", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		sprintf(line, "*pointer*");
		break;
	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_UglyValueString

Returns a string describing *data in a type specific manner
Easier to parse than PR_ValueString
=============
*/
char* Scr_UglyValueString(etype_t type, eval_t* val)
{
	static char	line[256];
	ddef_t* def;
	dfunction_t* f;
	type &= ~DEF_SAVEGLOBAL;
	CheckScriptVM(__FUNCTION__);
	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", Scr_GetString(val->string));
		break;
	case ev_entity:
		//sprintf(line, "%i", NUM_FOR_EDICT(PROG_TO_GENT(val->edict))); // braxi -- fixme
		break;
	case ev_function:
		f = active_qcvm->pFunctions + val->function;
		sprintf(line, "%s", Scr_GetString(f->s_name));
		break;
	case ev_field:
		def = ScrInternal_FieldAtOfs(val->_int);
		sprintf(line, "%s", Scr_GetString(def->s_name));
		break;
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%f", val->_float);
		break;
	case ev_integer:
		sprintf(line, "%i", val->_int);
		break;
	case ev_vector:
		sprintf(line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
}

/*
============
PR_GlobalString

Returns a string with a description and the contents of a global,
padded to 20 field width
============
*/
char* Scr_GlobalString(int ofs)
{
	char* s;
	int		i;
	ddef_t* def;
	void* val;
	static char	line[128];


	CheckScriptVM(__FUNCTION__);

	val = (void*)&active_qcvm->pGlobals[ofs];
	def = ScrInternal_GlobalAtOfs(ofs);
	if (!def)
	{
		sprintf(line, "%i(???)", ofs);
	}
	else
	{
		s = Scr_ValueString(def->type, val);
		sprintf(line, "%i(%s)%s", ofs, Scr_GetString(def->s_name), s);
	}

	i = (int)strlen(line);
	for (; i < 20; i++)
		strcat(line, " ");
	strcat(line, " ");

	return line;
}

char* Scr_GlobalStringNoContents(int ofs)
{
	int		i;
	ddef_t* def;
	static char	line[128];
	CheckScriptVM(__FUNCTION__);

	def = ScrInternal_GlobalAtOfs(ofs);
	if (!def)
		sprintf(line, "%i(???)", ofs);
	else
		sprintf(line, "%i(%s)", ofs, Scr_GetString(def->s_name));

	i = (int)strlen(line);
	for (; i < 20; i++)
		strcat(line, " ");
	strcat(line, " ");

	return line;
}


/*
=================
PR_PrintStatement
=================
*/
void Scr_PrintStatement(dstatement_t* s)
{
	int		i;
	CheckScriptVM(__FUNCTION__);

	if ((unsigned)s->op < sizeof(qcvm_op_names) / sizeof(qcvm_op_names[0]))
	{
		Com_Printf("%s ", qcvm_op_names[s->op]);
		i = (int)strlen(qcvm_op_names[s->op]);
		for (; i < 10; i++)
			Com_Printf(" ");
	}

	if (s->op == OP_IF )
	{
		Com_Printf("%s IF %i", Scr_GlobalString(s->a), s->b);
	}
	else if ( s->op == OP_IFNOT)
	{
		Com_Printf("%s IFNOT %i", Scr_GlobalString(s->a), s->b);
	}
	else if (s->op == OP_GOTO)
	{
		Com_Printf("GOTO %i", s->a);
	}
	else if ((unsigned)(s->op - OP_STORE_F) < 6)
	{
		Com_Printf("%s", Scr_GlobalString(s->a));
		Com_Printf("%s", Scr_GlobalStringNoContents(s->b));
	}
	else
	{
		if (s->a)
			Com_Printf("%s", Scr_GlobalString(s->a));
		if (s->b)
			Com_Printf("%s", Scr_GlobalString(s->b));
		if (s->c)
			Com_Printf("%s", Scr_GlobalStringNoContents(s->c));
	}
	Com_Printf("\n");
}


/*
============
Scr_StackTrace
============
*/
void Scr_StackTrace()
{
	dfunction_t* f;
	int			i;

	CheckScriptVM(__FUNCTION__);

	if (active_qcvm->stackDepth == 0)
	{
		Com_Printf("<NO STACK>\n");
		return;
	}
	active_qcvm->stack[active_qcvm->stackDepth].f = active_qcvm->xfunction;
	for (i = active_qcvm->stackDepth; i >= 0; i--)
	{
		f = active_qcvm->stack[i].f;

		if (!f)
		{
			if (active_qcvm->callFromFuncName != NULL)
				Com_Printf("%i: <pragma:%s>\n", i, active_qcvm->callFromFuncName);
			else
				Com_Printf("%i: <NO FUNCTION>\n", i);
		}
		else
			Com_Printf("%i: %s : %s\n", i, Scr_GetString(f->s_file), Scr_GetString(f->s_name));
	}
}


