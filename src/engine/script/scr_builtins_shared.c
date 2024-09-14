/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#include "../pragma.h"
#include "qcvm_private.h"

extern void Scr_InitMathBuiltins();

static char* retstr_none = "none"; // the 'none' string

static char pr_temp_string[12][256];
static int pr_temp_string_num = 0;

/*
=================
progstring()

utility functions can use this to print up to 8 arguments when no quakeworld strings enabled, dirty af.
FIXME someday
=================
*/
static char* progstring()
{
	if (pr_temp_string_num >= 12)
		pr_temp_string_num = 0;

	memset(pr_temp_string[pr_temp_string_num], sizeof(pr_temp_string[0]), 0);
	pr_temp_string_num++;
	return pr_temp_string[pr_temp_string_num-1];
}

/*
=================
PF_none
This must always generate an error
=================
*/
void PF_none(void)
{
	Scr_RunError("Wrong builtin!");
}

/*
===============
PF_print

Print message to server's console

void print(string text, ...)

print("hello world\n");
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

void dprint(string text, ...)

dprint("entity at ", vtos(self.origin), " is stuck in solid!\n");
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
Useful when iterating on a large number of entities in a loop

void resetrunaway()
=================
*/
void PF_resetrunaway(void)
{
	active_qcvm->runawayCounter = (int)vm_runaway->value;
}

/*
=================
PF_traceon

Begin execution tracing, use traceoff() to mark end of trace.
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

Stop script execution tracing.
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
PF_crash

Forces engine to crash (useful for debugger).

void crash()
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
void printstack()
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

Executes [commandstring] in local command execution buffer

void localcmd(string commandstring)

localcmd("gamemap test2");
=================
*/
void PF_localcmd(void)
{
	const char* str;
		
	str = Scr_GetParmString(0);
	if (!str || !strlen(str))
	{
		Scr_RunError("%s() with empty string.", Scr_BuiltinFuncName());
		return;
	}
	Cbuf_AddText(str);
}

/*
==============
PF_argc

returns the number of arguments of a command received from client

float argc()

float numargs = argc();
==============
*/
void PF_argc(void)
{
	Scr_ReturnFloat(Cmd_Argc());
}

/*
==============
PF_argv

returns the value of argument [argumentnum] of a command received from client

string argv(float argnum)

string cmdname = argv(0);
string cmdvalue = argv(1);
==============
*/
void PF_argv(void)
{
	int arg = Scr_GetParmFloat(0);

	if (arg < 0 || arg >= Cmd_Argc())
	{
		Scr_RunError("%s(%i): out of range [0,%i(argc)]\n", Scr_BuiltinFuncName() ,arg, Cmd_Argc());
		Scr_ReturnString(retstr_none); // not really needed
	}
	Scr_ReturnString(Cmd_Argv(arg));
}

// ===================== cvars =====================

/*
=================
PF_cvar
returns the value of a cvar as a float

float cvar(string cvarname)
float cheatsenabled = cvar("sv_cheats");
=================
*/
void PF_cvar(void)
{
	const char* str;
	cvar_t* cvar;

	str = Scr_GetParmString(0);
	if (!str || !strlen(str))
	{
		Scr_RunError("cvar() without name.");
		return;
	}

	cvar = Cvar_Get(str, NULL, 0, NULL);
	if (cvar)
	{
		Scr_ReturnFloat(cvar->value);
		return;
	}

//	Com_DPrintf(0, "cvar(%s): not found\n", str);
	Scr_ReturnFloat(0);
}


/*
=================
PF_cvarstring

returtns the value of a cvar as a string
string cvarstring(string cvarname)

string message = cvarstring("motd");
=================
*/
void PF_cvarstring(void)
{
	const char* str;
	cvar_t* cvar;

	str = Scr_GetParmString(0);
	if (!str || !strlen(str))
	{
		Scr_RunError("cvarstring() without name.");
		return;
	}

	cvar = Cvar_Get(str, NULL, 0, NULL);
	if (cvar)
	{
		Scr_ReturnString(cvar->string);
		return;
	}

//	Com_DPrintf(0, "cvarstring(%s): not found\n", str);
	Scr_ReturnString(retstr_none);
}

/*
=================
PF_cvarset

sets value of a cvar

void cvarset(string cvarname, string value, ...)

cvarset("motd", "welcome to ", sv_hostname, " server!");
=================
*/
void PF_cvarset(void)
{
	const char *str, *value;

	str = Scr_GetParmString(0);
	if (!str || !strlen(str))
	{
		Scr_RunError("cvarset() without name.");
		return;
	}

	value = Scr_VarString(1); // Scr_GetParmString(1);
	if( value )
		Cvar_Set(str, value);
}


/*
=================
PF_cvarforceset

void cvarforceset(string, string)
=================
*/
void PF_cvarforceset(void)
{
	const char* str, * value;

	str = Scr_GetParmString(0);
	if (!str || !strlen(str))
	{
		Scr_RunError("cvarforceset() with empty name.");
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
	char* tempstr;
	
	tempstr = progstring();
	v = Scr_GetParmFloat(0);

	if (v == (int)v)
		sprintf(tempstr, "%d", (int)v);
	else
		sprintf(tempstr, "%f", v);

	Scr_ReturnString(tempstr); // G_INT(OFS_RETURN) = (str - active_qcvm->pStrings);
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
	const char* str = Scr_GetParmString(0);
	Scr_ReturnFloat( (float)atof(str) );
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

returns a char at a given pos in src string

string strat(string src, float pos)

string str = "abc";
string letter_b = strat(str,1); // letter_b == b
=================
*/
void PF_strat(void)
{
	char* str;
	size_t len;
	int at;

	static char tempchar[2];

	str = Scr_GetParmString(0);
	at = (int)Scr_GetParmFloat(1);
	len = strlen(str);

	if (at >= 0 || at < len)
	{
		tempchar[0] = str[at];
		tempchar[1] = 0; // null terminated
		Scr_ReturnString(tempchar);
		return;
	}
	Scr_ReturnString("");
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
	sprintf(string_temp, "%.2f %.2f %.2f", vec[0], vec[1], vec[2]);
	Scr_ReturnString(string_temp);
}

/*
=================
PF_logprint

Appends text to logfile in gamedir, inserts date and time when timeStamp is true 
void logprint(float timeStamp, string text, ...)

logprint(true, self.name, " killed ", other.name, "\n" );
=================
*/
void PF_logprint(void)
{
	char	*str;
	qboolean timestamp;

	timestamp = Scr_GetParmFloat(0) > 0 ? true : false;
	str = Scr_VarString(1);

	if (timestamp)
	{
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		fprintf(active_qcvm->logfile, "%d-%02d-%02d %02d:%02d:%02d: ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	}

	if(active_qcvm->logfile)
	{
		fprintf(active_qcvm->logfile, str);
		fflush(active_qcvm->logfile);
	}
}

/*
=================
PF_crcfile

float crcfile(string filename)

Calculates and returns CRC checksum of a file

float crc_checksum = crcfile("models/player.md3");
=================
*/
void PF_crcfile(void)
{
	unsigned short checksum;
	char* str;
	str = Scr_GetParmString(0);

	if (strlen(str) <= 0)
		return;

	checksum = CRC_ChecksumFile(str, false);
	Scr_ReturnFloat((float)checksum);
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
	Scr_DefineBuiltin(PF_cvarset, PF_ALL, "cvarset", "void(string str, string val, ...)");
	Scr_DefineBuiltin(PF_cvarforceset, PF_ALL, "cvarforceset", "void(string str, string val)");

	// strings
	Scr_DefineBuiltin(PF_strlen, PF_ALL, "strlen", "float(string s)");
	Scr_DefineBuiltin(PF_strat, PF_ALL, "strat", "string(string s, float idx)");
	Scr_DefineBuiltin(PF_strstr, PF_ALL, "issubstr", "float(string s1, string s2)");
	Scr_DefineBuiltin(PF_ftos, PF_ALL, "ftos", "string(float f)");
	Scr_DefineBuiltin(PF_stof, PF_ALL, "stof", "float(string s)");
	Scr_DefineBuiltin(PF_vtos, PF_ALL, "vtos", "string(vector v1)");

	// files
	Scr_DefineBuiltin(PF_logprint, PF_ALL, "logprint", "void(float ts, string s, ...)");
	Scr_DefineBuiltin(PF_crcfile, PF_ALL, "crcfile", "float(string fn)");
	

//	Scr_DefineBuiltin(PF_itoc, PF_ALL, "itoc", "string(float i)");

	// math
	Scr_InitMathBuiltins();
}
