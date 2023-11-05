/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#include "../qcommon/qcommon.h"
#include "script_internals.h"

extern void Scr_InitMathBuiltins();

static char* retstr_none = "none"; // the 'none' string

static char pr_temp_string[MAX_PARMS][128];
static int pr_temp_string_num = -1;

/*
=================
progstring()

utility functions can use this to print up to 8 arguments when no quakeworld strings enabled, dirty af.
FIXME someday
=================
*/
static char* progstring()
{
	if (pr_temp_string_num == -1)
		memset(&pr_temp_string, sizeof(pr_temp_string), 0);

	pr_temp_string_num++;
	if (pr_temp_string_num >= MAX_PARMS)
		pr_temp_string_num = 0;

	return pr_temp_string[pr_temp_string_num];
}

/*
=================
PF_none
This must always generate an error
=================
*/
void PF_none(void)
{
	Scr_RunError("wrong builtin call\n");
}

/*
===============
PF_print
Print message to server's console
===============
*/
void PF_print(void)
{
	Com_Printf("%s", Scr_VarString(0));
#ifdef _DEBUG
	printf("%s", Scr_VarString(0));
#endif
}

/*
===============
PF_dprint
Print message to server's console when developer is enabled
===============
*/
void PF_dprint(void)
{
//	Com_Printf("[dev] %s", Scr_GetParmString(0));
	if( developer->value > 0 )
		Com_Printf("[dev] %s", Scr_VarString(0));

#ifdef _DEBUG
	printf("[dev] %s",Scr_VarString(0));
#endif
}


// ===================== script virtual machine =====================

/*
=================
PF_resetrunaway

Resets the infinite loop check counter, to prevent an incorrect 'runaway infinite loop' error when a lot of script must be run.
=================
*/
void PF_resetrunaway(void)
{
	active_qcvm->runawayCounter = (int)vm_runaway->value;
}

/*
=================
PF_traceon

Begin execution tracing, use traceoff() to mark end of trace, devmode only.
Result will be displayed in console after program completes.
At the beginning of program execution (in engine) this is always reset to false.
void traceon()
=================
*/
void PF_traceon(void)
{
	active_qcvm->traceEnabled = true;
}

/*
=================
PF_traceoff

Stop execution tracing, devmode only.
At the beginning of program execution (in engine) this is always reset to false.
void traceoff()
=================
*/
void PF_traceoff(void)
{
	active_qcvm->traceEnabled = false;
}

/*
=================
PF_break
Crashes engine (useful for debugger), devmode only.
break()
=================
*/
void PF_crash(void)
{
	Com_Printf("crash()!!!\n");
	*(int*)-4 = 0;	// dump to debugger
}

/*
=================
PF_printstack

Displays current program execution stack in console, devmode only.
printstack()
=================
*/
void PF_printstack(void)
{
	Com_Printf("Current script execution stack: \n");
	Scr_StackTrace();
}

// ===================== command buffer =====================

/*
=================
PF_localcmd
Executes text in local execution buffer

localcmd(string)
=================
*/
void PF_localcmd(void)
{
	char* str;
		
	str = Scr_GetParmString(0);
	if (!str || str == "")
	{
		Scr_RunError("localcmd(): empty string\n");
		return;
	}
	Cbuf_AddText(str);
}

/*
==============
PF_argc
float argc()
==============
*/
void PF_argc(void)
{
	Scr_ReturnFloat(Cmd_Argc());
}

/*
==============
PF_argv
string argv(float)
==============
*/
void PF_argv(void)
{
	int arg = Scr_GetParmFloat(0);

	if (arg < 0 || arg >= Cmd_Argc())
	{
		Scr_RunError("argv(%i): out of range [0,%i(argc)]\n", arg, Cmd_Argc());
		Scr_ReturnString(retstr_none); // not really needed
	}
	Scr_ReturnString(Cmd_Argv(arg));
}

// ===================== cvars =====================

/*
=================
PF_cvar

float cvar(string)
=================
*/
void PF_cvar(void)
{
	char* str;
	cvar_t* cvar;

	str = Scr_GetParmString(0);
	if (!str || str == "")
	{
		Scr_RunError("cvar(): without name\n");
		return;
	}

	cvar = Cvar_Get(str, NULL, 0);
	if (cvar)
	{
		Scr_ReturnFloat(cvar->value);
		return;
	}

	Com_DPrintf(0, "cvar(%s): not found\n", str);
	Scr_ReturnFloat(0);
}


/*
=================
PF_cvarstring

string cvarstring(string)
=================
*/
void PF_cvarstring(void)
{
	char* str;
	cvar_t* cvar;

	str = Scr_GetParmString(0);
	if (!str || str == "")
	{
		Scr_RunError("cvarstring(): without name\n");
		return;
	}

	cvar = Cvar_Get(str, NULL, 0);
	if (cvar)
	{
		Scr_ReturnString(cvar->string);
		return;
	}

	Com_DPrintf(0, "cvarstring(%s): not found\n", str);
	Scr_ReturnString(retstr_none);
}

/*
=================
PF_cvarset

void cvarset(string, string)
=================
*/
void PF_cvarset(void)
{
	char *str, *value;

	str = Scr_GetParmString(0);
	if (!str || str == "")
	{
		Scr_RunError("cvarset(): without name\n");
		return;
	}

	value = Scr_GetParmString(1);
	if( value )
		Cvar_Set(str, value);
}


/*
=================
PF_cvarset

void cvarforceset(string, string)
=================
*/
void PF_cvarforceset(void)
{
	char* str, * value;

	str = Scr_GetParmString(0);
	if (!str || str == "")
	{
		Scr_RunError("cvarforceset(): empty name\n");
		return;
	}

	value = Scr_GetParmString(1);
	if (value)
		Cvar_ForceSet(str, value);
}


/*
=================
PF_ftos

float to string
string ftos(float)
=================
*/
void PF_ftos(void)
{
	float	v;
	char* string_temp = progstring();
	v = Scr_GetParmFloat(0);
	if (v == (int)v)
		sprintf(string_temp, "%d", (int)v);
	else
		sprintf(string_temp, "%5.1f", v);
	Scr_ReturnString(string_temp);
}

/*
=================
PF_stof

string to float
float stof(string)
=================
*/
void PF_stof(void)
{
	Scr_ReturnFloat( atof(Scr_GetParmString(0)) );
}

/*
=================
PF_strlen

return length of a string
float strlen(string)
=================
*/
void PF_strlen(void)
{
	char* str;
	size_t len;

	str = Scr_GetParmString(0);
	len = strlen(str);

	Scr_ReturnFloat(len);
}

/*
=================
PF_strat

return char from a str at index
string strat(string,float index)

//FIXME: CRASHING
=================
*/
void PF_strat(void)
{
#if 1
	Scr_RunError("PF_strat: borked\n");
#else
	char
	char* str;
	size_t len;
	int at;

	str = Scr_GetParmString(0);
	at = (int)Scr_GetParmFloat(1);
	len = strlen(str);

	if (at >= 0 || at < len)
	{
		str = str[at];
		Scr_ReturnString(str);
		return;
	}
	Scr_ReturnString("");
#endif
}

/*
=================
PF_strstr

returns true if a string B is a part of string A
float strstr(string a,string b)
=================
*/
void PF_strstr(void)
{
	char* str1, *str2;
	
	str1 = Scr_GetParmString(0);
	str2 = Scr_GetParmString(1);

	if (strstr((const char*)str1, (const char*)str2))
		Scr_ReturnFloat(1);
	else
		Scr_ReturnFloat(0);
}

/*
=================
PF_vtos

vector to string
string vtos(vector)
=================
*/
void PF_vtos(void)
{
	float* vec = Scr_GetParmVector(0);
	char* string_temp = progstring();
	sprintf(string_temp, "%f %f %f", vec[0], vec[1], vec[2]);
	Scr_ReturnString(string_temp);
}


/*
=================
Scr_InitSharedBuiltins

Register builtins which can be shared by both client and server progs
=================
*/
void Scr_InitSharedBuiltins()
{
	// this should always error
	Scr_DefineBuiltin(PF_none, PF_ALL, "NONE", "NULL");

	// text prints
	Scr_DefineBuiltin(PF_print, PF_ALL, "print", "void(string s, ...)");
	Scr_DefineBuiltin(PF_dprint, PF_ALL, "dprint", "void(string s, ...)");

	// script virtual machine
	Scr_DefineBuiltin(PF_resetrunaway, PF_ALL, "resetrunaway", "void()");
	Scr_DefineBuiltin(PF_traceon, PF_ALL, "traceon", "void()");
	Scr_DefineBuiltin(PF_traceoff, PF_ALL, "traceoff", "void()");
	Scr_DefineBuiltin(PF_crash, PF_ALL, "crash", "void()");
	Scr_DefineBuiltin(PF_printstack, PF_ALL, "printstack", "void()");

	// command buffer
	Scr_DefineBuiltin(PF_localcmd, PF_ALL, "localcmd", "void(string str)");
	Scr_DefineBuiltin(PF_argc, PF_ALL, "argc", "float()");
	Scr_DefineBuiltin(PF_argv, PF_ALL, "argv", "string(float idx)");

	// cvars
	Scr_DefineBuiltin(PF_cvar, PF_ALL, "cvar", "float(string str)");
	Scr_DefineBuiltin(PF_cvarstring, PF_ALL, "cvarstring", "string(string str)");
	Scr_DefineBuiltin(PF_cvarset, PF_ALL, "cvarset", "void(string str, string val)");
	Scr_DefineBuiltin(PF_cvarforceset, PF_ALL, "cvarforceset", "void(string str, string val)");

	// strings
	Scr_DefineBuiltin(PF_strlen, PF_ALL, "strlen", "float(string s)");
	Scr_DefineBuiltin(PF_strat, PF_ALL, "strat", "string(string s, float idx)");
	Scr_DefineBuiltin(PF_strstr, PF_ALL, "issubstr", "float(string s1, string s2)");
	Scr_DefineBuiltin(PF_ftos, PF_ALL, "ftos", "string(float f)");
	Scr_DefineBuiltin(PF_stof, PF_ALL, "stof", "float(string s)");
	Scr_DefineBuiltin(PF_vtos, PF_ALL, "vtos", "string(vector v1)");

	// math
	Scr_InitMathBuiltins();
}
