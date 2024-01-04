/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// scr_execution.c

#include "../qcommon/qcommon.h"
#include "script_internals.h"

extern ddef_t* ScrInternal_GlobalAtOfs(int ofs);
extern ddef_t* Scr_FindEntityField(char* name);
extern void Scr_PrintEntityFields(vm_entity_t* ent);

#define ENTVARSOFFSET(ent) (int*)(&ent[0] + active_qcvm->offsetToEntVars)

/*
============
Scr_GetEntityFieldValue
============
*/
eval_t* Scr_GetEntityFieldValue(vm_entity_t *ent, char* field)
{
	ddef_t* def = NULL;
	int				i;
	static int		rep = 0;

	CheckScriptVM(__FUNCTION__);
	for (i = 0; i < SCR_GEFV_CACHESIZE; i++)
	{
		if (!strcmp(field, active_qcvm->gefvCache[i].field))
		{
			def = active_qcvm->gefvCache[i].pcache;
			goto Done;
		}
	}

	def = Scr_FindEntityField(field);
	if (strlen(field) < SCR_MAX_FIELD_LEN)
	{
		active_qcvm->gefvCache[rep].pcache = def;
		strcpy(active_qcvm->gefvCache[rep].field, field);
		rep ^= 1;
	}

Done:
	if (!def)
		return NULL;

	return (eval_t*)((char*)ENTVARSOFFSET(ent) + def->ofs * 4);
}

/*
============
Scr_RunError

Aborts the currently executing function
============
*/
void Scr_PrintStatement(dstatement_t* s);

void Scr_RunError(char* error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start(argptr, error);
	vsprintf(string, error, argptr);
	va_end(argptr);

	CheckScriptVM(__FUNCTION__);

	Scr_PrintStatement(active_qcvm->statements + active_qcvm->xstatement);
	Scr_StackTrace();
	active_qcvm->stackDepth = 0;

#ifdef _DEBUG
	printf("%s\n", string);
#endif
	Com_Error(ERR_DROP, "%s", string);
}

#define QCVMDEBUGLEVEL 0
/*
============
Scr_NumArgs

Returns argc of currently entered function
============
*/
int Scr_NumArgs()
{
	return active_qcvm->argc;
}

/*
====================
ScrInternal_EnterFunction

Returns the new program statement counter
====================
*/
int ScrInternal_EnterFunction(dfunction_t* f)
{
	int		i, j, c, o;

	CheckScriptVM(__FUNCTION__);

	active_qcvm->stack[active_qcvm->stackDepth].s = active_qcvm->xstatement;
	active_qcvm->stack[active_qcvm->stackDepth].f = active_qcvm->xfunction;

	active_qcvm->stackDepth++;
	if (active_qcvm->stackDepth >= SCR_MAX_STACK_DEPTH)
		Scr_RunError("%s qcvm: stack overflow\n", vmDefs[active_qcvm->progsType].name);


	// save off any locals that the new function steps on
	c = f->locals;
	if (active_qcvm->localstack_used + c > SCR_LOCALSTACK_SIZE)
		Scr_RunError("%s qcvm: locals stack overflow\n", vmDefs[active_qcvm->progsType].name);

#if QCVMDEBUGLEVEL > 1
	printf("[%s:#%i] enter %s:%s (%i parms)\n", vmDefs[active_qcvm->progsType].name, active_qcvm->stackDepth, COM_SkipPath(Scr_GetString(f->s_file)), Scr_GetString(f->s_name),f->numparms);
#endif

	for (i = 0; i < c; i++)
		active_qcvm->localstack[active_qcvm->localstack_used + i] = ((int*)active_qcvm->globals)[f->parm_start + i];
	active_qcvm->localstack_used += c;


	// copy parameters
	o = f->parm_start;
	for (i = 0; i < f->numparms; i++)
	{
		for (j = 0; j < f->parm_size[i]; j++)
		{
			((int*)active_qcvm->globals)[o] = ((int*)active_qcvm->globals)[OFS_PARM0 + i * 3 + j];
			o++;
		}
	}

	active_qcvm->xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

/*
====================
PR_LeaveFunction
====================
*/
int ScrInternal_LeaveFunction()
{
	int		i, c;

	CheckScriptVM(__FUNCTION__);

	if (active_qcvm->stackDepth <= 0)
	{
		Scr_RunError("script stack underflow in %s\n", vmDefs[active_qcvm->progsType].filename);
	}

	// restore locals from the stack
	c = active_qcvm->xfunction->locals;
	active_qcvm->localstack_used -= c;
	if (active_qcvm->localstack_used < 0)
		Scr_RunError("locals stack underflow in %s\n", vmDefs[active_qcvm->progsType].filename);

	for (i = 0; i < c; i++)
		((int*)active_qcvm->globals)[active_qcvm->xfunction->parm_start + i] = active_qcvm->localstack[active_qcvm->localstack_used + i];

	// up stack
	active_qcvm->stackDepth--;
#if QCVMDEBUGLEVEL > 1
	printf("[%s:#%i] leave %s:%s()\n", vmDefs[active_qcvm->progsType].name, active_qcvm->stackDepth, COM_SkipPath(Scr_GetString(active_qcvm->xfunction->s_file)), Scr_GetString(active_qcvm->xfunction->s_name));
#endif
	active_qcvm->xfunction = active_qcvm->stack[active_qcvm->stackDepth].f;
	return active_qcvm->stack[active_qcvm->stackDepth].s;
}

/*
====================
Scr_Execute

Execute script program
====================
*/

extern char* qcvm_op_names[];
void Scr_Execute(vmType_t vmtype, scr_func_t fnum, char* callFromFuncName)
{
	eval_t			*a, *b, *c, *ptr;
	int				s, i, exitdepth;
	dstatement_t	*st;
	dfunction_t		*f, *newf;
	qcvm_t			*vm;
	vm_entity_t		*ent;


	Scr_BindVM(vmtype);
//	CheckScriptVM(__FUNCTION__);

	vm = active_qcvm;

	vm->callFromFuncName = callFromFuncName;
	if (!fnum || fnum >= vm->progs->numFunctions)
	{
		if (vm->progsType == VM_SVGAME)
		{
			sv_globalvars_t *g = vm->globals_struct;
			if (g->self)
				Scr_PrintEntityFields(VM_TO_ENT(g->self));
		}

		Scr_RunError("%s: incorrect function index %i in %s, from %s\n", __FUNCTION__, fnum, vmDefs[vm->progsType].filename, callFromFuncName);
		return;
	}

	f = &vm->functions[fnum];

	vm->runawayCounter = (int)vm_runaway->value;
	vm->traceEnabled = false;

	// make a stack frame
	exitdepth = vm->stackDepth;

	s = ScrInternal_EnterFunction(f);

	while (1)
	{
		s++;	// next statement

		st = &vm->statements[s];
		a = (eval_t*)&vm->globals[st->a];
		b = (eval_t*)&vm->globals[st->b];
		c = (eval_t*)&vm->globals[st->c];

		if (!--vm->runawayCounter)
			Scr_RunError("runaway loop error in function %s (%s)", ScrInternal_String(f->s_name), vmDefs[vm->progsType].filename);

		vm->xfunction->profile++;
		vm->xstatement = s;

		if (vm->traceEnabled)
		{
			Scr_PrintStatement(st);
			int addr[] = { st->a, st->b, st->c };
			ddef_t* def;
			for (i = 0; i != 3; i++)
			{
				if (addr[i])
				{
					def = ScrInternal_GlobalAtOfs(addr[i]);
					if (def)
						Com_Printf("%s\n", Scr_GetString(def->s_name));
				}
			}
		}


		switch (st->op)
		{
		case OP_ADD_F: // float + float
			c->_float = a->_float + b->_float;
			break;

		case OP_ADD_V: // vector + vector
			c->vector[0] = a->vector[0] + b->vector[0];
			c->vector[1] = a->vector[1] + b->vector[1];
			c->vector[2] = a->vector[2] + b->vector[2];
			break;

/* FTEQC: begin int */
		case OP_ADD_I: // int + int
			c->_int = a->_int + b->_int;
			break;

		case OP_ADD_FI: // float + int
			c->_float = a->_float + (float)b->_int;
			break;
		case OP_ADD_IF: // int + float
			c->_float = (float)a->_int + b->_float;
			break;

		case OP_SUB_I: // int - int
			c->_int = a->_int - b->_int;
			break;
		case OP_SUB_FI: // float - int
			c->_float = a->_float - (float)b->_int;
			break;

		case OP_SUB_IF: // int - float
			c->_float = (float)a->_int - b->_float;
			break;

		case OP_CONV_ITOF:
			c->_float = (float)a->_int;
			break;
		case OP_CONV_FTOI:
			c->_int = (int)a->_float;
			break;

		case OP_LOADP_ITOF:
		case OP_LOADP_FTOI:
		case OP_LOADA_I:
		case OP_LOADP_I:
			Scr_RunError("Unsupported FTEQC opcode %s in %s", qcvm_op_names[st->op], vmDefs[vm->progsType].filename);
			break;

		case OP_MUL_I:
			c->_int = a->_int * b->_int;
			break;

		case OP_DIV_I:
			if (b->_int == 0)
			{
				Scr_RunError("division by zero in %s", vmDefs[vm->progsType].filename);
//				c->_int = 0;
			}
			else
				c->_int = a->_int / b->_int;
			break;

		case OP_EQ_I:
			c->_int = (a->_int == b->_int);
			break;

		case OP_NE_I:
			c->_int = (a->_int != b->_int);
			break;

		case OP_NOT_I:
			c->_int = !a->_int;
			break;

		case OP_EQ_IF:
			c->_int = (float)(a->_int == b->_float);
			break;

		case OP_EQ_FI:
			c->_int = (float)(a->_float == b->_int);
			break;

		case OP_BITXOR_I:
			c->_int = a->_int ^ b->_int;
			break;

		case OP_RSHIFT_I:
			c->_int = a->_int >> b->_int;
			break;

		case OP_LSHIFT_I:
			c->_int = a->_int << b->_int;
			break;

		case OP_LE_I:
			c->_int = (int)(a->_int <= b->_int);
			break;

		case OP_LE_IF:
			c->_int = (int)(a->_int <= b->_float);
			break;

		case OP_LE_FI:
			c->_int = (int)(a->_float <= b->_int);
			break;

		case OP_GT_I:
			c->_int = (int)(a->_int > b->_int);
			break;

		case OP_GT_IF:
			c->_int = (int)(a->_int > b->_float);
			break;

		case OP_GT_FI:
			c->_int = (int)(a->_float > b->_int);
			break;

		case OP_LT_I:
			c->_int = (int)(a->_int < b->_int);
			break;

		case OP_LT_IF:
			c->_int = (int)(a->_int < b->_float);
			break;

		case OP_LT_FI:
			c->_int = (int)(a->_float < b->_int);
			break;

		case OP_GE_I:
			c->_int = (int)(a->_int >= b->_int);
			break;

		case OP_GE_IF:
			c->_int = (int)(a->_int >= b->_float);
			break;

		case OP_GE_FI:
			c->_int = (int)(a->_float >= b->_int);
			break;


		case OP_MUL_IF:
			c->_float = (a->_int * b->_float);
			break;

		case OP_MUL_FI:
			c->_float = (a->_float * b->_int);
			break;

		case OP_MUL_VI:
			c->vector[0] = a->vector[0] * b->_int;
			c->vector[1] = a->vector[1] * b->_int;
			c->vector[2] = a->vector[2] * b->_int;
			break;

		case OP_MUL_IV:
			c->vector[0] = a->_int * b->vector[0];
			c->vector[1] = a->_int * b->vector[1];
			c->vector[2] = a->_int * b->vector[2];
			break;

		case OP_DIV_IF:
			c->_float = (a->_int / b->_float);
			break;

		case OP_DIV_FI:
			c->_float = (a->_float / b->_int);
			break;

		case OP_BITAND_IF:
			c->_int = (a->_int & (int)b->_float);
			break;

		case OP_BITOR_IF:
			c->_int = (a->_int | (int)b->_float);
			break;

		case OP_BITAND_FI:
			c->_int = ((int)a->_float & b->_int);
			break;

		case OP_BITOR_FI:
			c->_int = ((int)a->_float | b->_int);
			break;


		case OP_AND_I:
			c->_int = (a->_int && b->_int);
			break;

		case OP_OR_I:
			c->_int = (a->_int || b->_int);
			break;

		case OP_AND_IF:
			c->_int = (a->_int && b->_float);
			break;

		case OP_OR_IF:
			c->_int = (a->_int || b->_float);
			break;

		case OP_AND_FI:
			c->_int = (a->_float && b->_int);
			break;

		case OP_OR_FI:
			c->_int = (a->_float || b->_int);
			break;

		case OP_NE_IF:
			c->_int = (a->_int != b->_float);
			break;

		case OP_NE_FI:
			c->_int = (a->_float != b->_int);
			break;
/* FTEQC: end int */

		case OP_SUB_F: // - float
			c->_float = a->_float - b->_float;
			break;

		case OP_SUB_V: // - vector
			c->vector[0] = a->vector[0] - b->vector[0];
			c->vector[1] = a->vector[1] - b->vector[1];
			c->vector[2] = a->vector[2] - b->vector[2];
			break;

		case OP_MUL_F: // * float
			c->_float = a->_float * b->_float;
			break;

		case OP_MUL_V: // * vector
			c->_float = a->vector[0] * b->vector[0]
				+ a->vector[1] * b->vector[1]
				+ a->vector[2] * b->vector[2];
			break;

		case OP_MUL_FV: // float * vector
			c->vector[0] = a->_float * b->vector[0];
			c->vector[1] = a->_float * b->vector[1];
			c->vector[2] = a->_float * b->vector[2];
			break;

		case OP_MUL_VF: // vector * float
			c->vector[0] = b->_float * a->vector[0];
			c->vector[1] = b->_float * a->vector[1];
			c->vector[2] = b->_float * a->vector[2];
			break;

		case OP_DIV_F: // / float
			c->_float = a->_float / b->_float;
			break;

		case OP_BITAND: // (& float)
			c->_float = (int)a->_float & (int)b->_float;
			break;

		case OP_BITOR: // (| float)
			c->_float = (int)a->_float | (int)b->_float;
			break;

		case OP_BITAND_I:
			c->_int = (a->_int & b->_int);
			break;

		case OP_BITOR_I:
			c->_int = (a->_int | b->_int);
			break;

		case OP_GE: // >= float
			c->_float = a->_float >= b->_float;
			break;
		case OP_LE: // <= float
			c->_float = a->_float <= b->_float;
			break;
		case OP_GT: // > float
			c->_float = a->_float > b->_float;
			break;
		case OP_LT: // < float
			c->_float = a->_float < b->_float;
			break;
		case OP_AND: // (&& float)
			c->_float = a->_float && b->_float;
			break;
		case OP_OR:// (|| float)
			c->_float = a->_float || b->_float;
			break;

		case OP_NOT_F: //not float
			c->_float = !a->_float;
			break;
		case OP_NOT_V: // not vector
			c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
			break;
		case OP_NOT_S: // not string
			c->_float = !a->string || !vm->strings[a->string];
			break;
		case OP_NOT_FNC: // not function
			c->_float = !a->function;
			break; 
		case OP_NOT_ENT: // not entity
			c->_float = (VM_TO_ENT(a->edict) == vm->entities);
			break;

		case OP_EQ_F: // == float
			c->_float = a->_float == b->_float;
			break;

		case OP_EQ_V: // == vector
			c->_float = (a->vector[0] == b->vector[0]) &&
				(a->vector[1] == b->vector[1]) &&
				(a->vector[2] == b->vector[2]);
			break;

		case OP_EQ_S: // == string
			c->_float = !strcmp(ScrInternal_String(a->string), ScrInternal_String(b->string));
			break;

		case OP_EQ_E: // equal int
			c->_float = a->_int == b->_int;
			break;

		case OP_EQ_FNC: // equal func
			c->_float = a->function == b->function;
			break;

		case OP_NE_F: // not equal float
			c->_float = a->_float != b->_float;
			break;

		case OP_NE_V: // not equal vector
			c->_float = (a->vector[0] != b->vector[0]) ||
				(a->vector[1] != b->vector[1]) ||
				(a->vector[2] != b->vector[2]);
			break;

		case OP_NE_S: // not equal string
			c->_float = strcmp(ScrInternal_String(a->string), ScrInternal_String(b->string));
			break;

		case OP_NE_E: // not equal int
			c->_float = a->_int != b->_int;
			break;

		case OP_NE_FNC: //not equal function
			c->_float = a->function != b->function;
			break;

			//==================
		case OP_STORE_IF: // FTE: int -> float
			b->_float = (float)a->_int;
			break;
		case OP_STORE_FI:  // FTE: float -> int
			b->_int = (int)a->_float;
			break;

		case OP_STORE_F:
		case OP_STORE_ENT:
		case OP_STORE_FLD:		// integers
		case OP_STORE_S:
		case OP_STORE_I:		// FTE: int
		case OP_STORE_FNC:		// pointers
			b->_int = a->_int;
			break;

		case OP_STORE_V: //store vector
			b->vector[0] = a->vector[0];
			b->vector[1] = a->vector[1];
			b->vector[2] = a->vector[2];
			break;


/* FTE: int store a value to a pointer */
		case OP_STOREP_IF:
			ptr = (eval_t*)((byte*)vm->entities + b->_int);
			ptr->_float = (float)a->_int;
		break;

		case OP_STOREP_FI:
			ptr = (eval_t*)((byte*)vm->entities + b->_int);
			ptr->_int = (int)a->_float;
		break;
/*FTE end of int*/

		case OP_STOREP_I: // FTE
		case OP_STOREP_F:
		case OP_STOREP_ENT:
		case OP_STOREP_FLD:		// integers
		case OP_STOREP_S:
		case OP_STOREP_FNC:		// pointers
			ptr = (eval_t*)((byte*)vm->entities + b->_int);
			ptr->_int = a->_int;
			break;
		case OP_STOREP_V:
			ptr = (eval_t*)((byte*)vm->entities + b->_int);
			ptr->vector[0] = a->vector[0];
			ptr->vector[1] = a->vector[1];
			ptr->vector[2] = a->vector[2];
			break;

		case OP_ADDRESS:
			ent = VM_TO_ENT(a->edict);
			if (ent == vm->entities && (vm->progsType == VM_SVGAME && Com_IsServerActive()))
			{
				Scr_StackTrace();
				Scr_RunError("worldspawn entity fields are read only\n");
			}
			c->_int = (byte*)(ENTVARSOFFSET(ent) + b->_int) - (byte*)vm->entities;
			break;

		//load a field to a value
		case OP_LOAD_F:
		case OP_LOAD_I: // FTE
		case OP_LOAD_FLD:
		case OP_LOAD_ENT:
		case OP_LOAD_S:
		case OP_LOAD_FNC:
			ent = VM_TO_ENT(a->edict);
			a = (eval_t*)(ENTVARSOFFSET(ent) + b->_int);
			c->_int = a->_int;
			break;

		case OP_LOAD_V:
			ent = VM_TO_ENT(a->edict);
			a = (eval_t*)(ENTVARSOFFSET(ent) + b->_int);
			c->vector[0] = a->vector[0];
			c->vector[1] = a->vector[1];
			c->vector[2] = a->vector[2];
			break;

			//==================

		case OP_IFNOT:
			if (!a->_int)
				s += st->b - 1;	// offset the s++
			break;

		case OP_IF:
			if (a->_int)
				s += st->b - 1;	// offset the s++
			break;

		case OP_GOTO:
			s += st->a - 1;	// offset the s++
			break;

		case OP_CALL0:
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
			vm->argc = st->op - OP_CALL0; //sets the number of arguments a function takes
			if (!a->function)
				Scr_RunError("%s: NULL function in %s\n", __FUNCTION__, vmDefs[vm->progsType].filename);

			newf = &vm->functions[a->function];

			if (newf->first_statement < 0)
			{	// negative statements are built in functions
				i = -newf->first_statement;
				if (i >= scr_numBuiltins)
					Scr_RunError("%s: unknown builtin function (funcnum = %i) in %s\n", __FUNCTION__, i, vmDefs[vm->progsType].filename);

				if(scr_builtins[i].execon != PF_ALL && vm->progsType != scr_builtins[i].execon)
					Scr_RunError("%s: call to '%s' builtin in %s VM not allowed\n", __FUNCTION__, scr_builtins[i].name, vmDefs[vm->progsType].name);

				scr_builtins[i].func();
				break;
			}

			s = ScrInternal_EnterFunction(newf);
			break;

		case OP_DONE:
		case OP_RETURN:
			vm->globals[OFS_RETURN] = vm->globals[st->a];
			vm->globals[OFS_RETURN + 1] = vm->globals[st->a + 1];
			vm->globals[OFS_RETURN + 2] = vm->globals[st->a + 2];

			s = ScrInternal_LeaveFunction();
			if (vm->stackDepth == exitdepth)
				return;		// all done
			break;

		case OP_STATE:
			if (vm->progsType == VM_SVGAME)
			{
				extern void Scr_SV_OP(eval_t * a, eval_t * b, eval_t * c);
				Scr_SV_OP(a, b, c);
			}
			else
			{
				Scr_RunError("OP_STATE not implemented for %s", vmDefs[vm->progsType].name);
			}

//			ent = PROG_TO_GENT(ScriptVM->globals_struct->self);
//			ent->v.nextthink = ScriptVM->globals_struct->g_time + 0.1;
//			if (a->_float != ent->v.animFrame)
//			{
//				ent->v.animFrame = a->_float;
//			}
//			ent->v.think = b->function;
			break;

		default:
			if(st->op > 0 && st->op < 269)
				Scr_RunError("%s: unknown opcode %i [%s] in %s\n", __FUNCTION__, st->op, qcvm_op_names[st->op], vmDefs[vm->progsType].filename);
			else
				Scr_RunError("%s: unknown opcode %i in %s\n", __FUNCTION__, st->op, vmDefs[vm->progsType].filename);
		}
	}

}


/*
=============
Scr_PrintEntityFields

For debugging
=============
*/
static int	type_size[8] = { 1,sizeof(scr_string_t) / 4,1,3,1,1,sizeof(scr_func_t) / 4,sizeof(void*) / 4 };
char* Scr_ValueString(etype_t type, eval_t* val);
void Scr_PrintEntityFields(vm_entity_t* ent)
{
	int		l;
	ddef_t* d;
	int* v;
	int		i, j;
	char* name;
	int		type;

//	if (!ent->inuse)
//	{
//		Com_Printf("Scr_PrintEntityFields: entity not in use\n");
//		return;
//	}

	CheckScriptVM(__FUNCTION__);

//	Com_Printf("\nENTITY #%i:\n", NUM_FOR_EDICT(ent));
	for (i = 1; i < active_qcvm->progs->numFieldDefs; i++)
	{
		d = &active_qcvm->fieldDefs[i];
		name = Scr_GetString(d->s_name);
		if (name[strlen(name) - 2] == '_')
			continue;	// skip _x, _y, _z vars

		v = (int*)((char*)ENTVARSOFFSET(ent) + d->ofs * 4);

		// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;

		for (j = 0; j < type_size[type]; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;

		Com_Printf("%s", name);
		l = strlen(name);
		while (l++ < 15)
			Com_Printf(" ");

		Com_Printf("%s\n", Scr_ValueString(d->type, (eval_t*)v));
	}
}

void cmd_printedict_f(void)
{
#if 0
	if (Cmd_Argc() != 2)
	{
		Com_Printf("usage: edict entitynum\n");
		return;
	}
	int num;

	num = atoi(Cmd_Argv(1));
	Scr_PrintServerEdict(ENT_FOR_NUM(num));
#endif
}

void cmd_printedicts_f(void)
{
#if 0
	int y = 0;
	int num;
	gentity_t * ent;

	if (Cmd_Argc() == 2)
	{
		Com_Printf("searching for all entities matching %s\n", Cmd_Argv(1));
	}

	for (num = 0; num <= sv.max_edicts; num++)
	{
		ent = EDICT_NUM(num);
		if (ent->inuse)
		{
			if (Cmd_Argc() == 1 || Cmd_Argc() == 2 && strstr(Scr_GetString(ent->v.classname), Cmd_Argv(1)))
			{
				y++;
				Scr_PrintServerEdict(ent);
				Com_Printf("-----------------------\n");
			}
		}	
	}
	Com_Printf("%i (%i) out of %i entities in use\n", sv.num_edicts,y, sv.max_edicts);
#endif
}


