/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// scr_debug.c

#include "../qcommon/qcommon.h"
#include "scriptvm.h"
#include "script_internals.h"
#include "../server/sv_game.h"

static int	type_size[8] = { 1,sizeof(scr_string_t) / 4,1,3,1,1,sizeof(scr_func_t) / 4,sizeof(void*) / 4 };

ddef_t* ScrInternal_GlobalAtOfs(int ofs);
ddef_t* ScrInternal_FieldAtOfs(int ofs);

char* pr_opnames[] = /* same as above, but for debugging */
{
"DONE",

"MUL_F",
"MUL_V",
"MUL_FV",
"MUL_VF",

"DIV",

"ADD_F",
"ADD_V",

"SUB_F",
"SUB_V",

"EQ_F",
"EQ_V",
"EQ_S",
"EQ_E",
"EQ_FNC",

"NE_F",
"NE_V",
"NE_S",
"NE_E",
"NE_FNC",

"LE",
"GE",
"LT",
"GT",

"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",
"INDIRECT",

"ADDRESS",

"STORE_F",
"STORE_V",
"STORE_S",
"STORE_ENT",
"STORE_FLD",
"STORE_FNC",

"STOREP_F",
"STOREP_V",
"STOREP_S",
"STOREP_ENT",
"STOREP_FLD",
"STOREP_FNC",

"RETURN",

"NOT_F",
"NOT_V",
"NOT_S",
"NOT_ENT",
"NOT_FNC",

"IF",
"IFNOT",

"CALL0",
"CALL1",
"CALL2",
"CALL3",
"CALL4",
"CALL5",
"CALL6",
"CALL7",
"CALL8",

"STATE",

"GOTO",

"AND",
"OR",

"BITAND",
"BITOR"
};



inline CheckScriptVM()
{
#ifdef SCRIPTVM_PARANOID
	if (ScriptVM == NULL)
		Com_Error(ERR_FATAL, "Script VM is NULL in %s\n", __FUNCTION__);
#endif
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

	CheckScriptVM();

	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", ScrInternal_String(val->string));
		break;
	case ev_entity:
		//sprintf(line, "entity %i", NUM_FOR_EDICT(PROG_TO_GENT(val->edict))); // braxi -- fixme
		break;
	case ev_function:
		f = ScriptVM->functions + val->function;
		sprintf(line, "%s()", ScrInternal_String(f->s_name));
		break;
	case ev_field:
		def = ScrInternal_FieldAtOfs(val->_int);
		sprintf(line, ".%s", ScrInternal_String(def->s_name));
		break;
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%5.1f", val->_float);
		break;
	case ev_vector:
		sprintf(line, "'%5.1f %5.1f %5.1f'", val->vector[0], val->vector[1], val->vector[2]);
		break;
	case ev_pointer:
		sprintf(line, "pointer");
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
	CheckScriptVM();
	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", ScrInternal_String(val->string));
		break;
	case ev_entity:
		//sprintf(line, "%i", NUM_FOR_EDICT(PROG_TO_GENT(val->edict))); // braxi -- fixme
		break;
	case ev_function:
		f = ScriptVM->functions + val->function;
		sprintf(line, "%s", ScrInternal_String(f->s_name));
		break;
	case ev_field:
		def = ScrInternal_FieldAtOfs(val->_int);
		sprintf(line, "%s", ScrInternal_String(def->s_name));
		break;
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%f", val->_float);
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
	CheckScriptVM();

	val = (void*)&ScriptVM->globals[ofs];
	def = ScrInternal_GlobalAtOfs(ofs);
	if (!def)
		sprintf(line, "%i(???)", ofs);
	else
	{
		s = Scr_ValueString(def->type, val);
		sprintf(line, "%i(%s)%s", ofs, ScrInternal_String(def->s_name), s);
	}

	i = strlen(line);
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
	CheckScriptVM();

	def = ScrInternal_GlobalAtOfs(ofs);
	if (!def)
		sprintf(line, "%i(???)", ofs);
	else
		sprintf(line, "%i(%s)", ofs, ScrInternal_String(def->s_name));

	i = strlen(line);
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
	CheckScriptVM();

	if ((unsigned)s->op < sizeof(pr_opnames) / sizeof(pr_opnames[0]))
	{
		Com_Printf("%s ", pr_opnames[s->op]);
		i = strlen(pr_opnames[s->op]);
		for (; i < 10; i++)
			Com_Printf(" ");
	}

	if (s->op == OP_IF || s->op == OP_IFNOT)
		Com_Printf("%sbranch %i", Scr_GlobalString(s->a), s->b);
	else if (s->op == OP_GOTO)
	{
		Com_Printf("branch %i", s->a);
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

	CheckScriptVM();

	if (ScriptVM->stackDepth == 0)
	{
		Com_Printf("<NO STACK>\n");
		return;
	}

	ScriptVM->stack[ScriptVM->stackDepth].f = ScriptVM->xfunction;
	for (i = ScriptVM->stackDepth; i >= 0; i--)
	{
		f = ScriptVM->stack[i].f;

		if (!f)
		{
			if (ScriptVM->callFromFuncName != NULL)
				Com_Printf("%i: <pragma:%s>\n", i, ScriptVM->callFromFuncName);
			else
				Com_Printf("%i: <NO FUNCTION>\n", i);
		}
		else
			Com_Printf("%i: %s : %s\n", i, ScrInternal_String(f->s_file), ScrInternal_String(f->s_name));
	}
}


