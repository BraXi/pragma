/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/


#include "../pragma.h"
#include "qcvm_private.h"

cvar_t* vm_runaway;

qcvm_t* qcvm[NUM_SCRIPT_VMS];
qcvm_t* active_qcvm; // qcvm currently in use

builtin_t* scr_builtins;
int scr_numBuiltins = 0;

const qcvmdef_t vmDefs[NUM_SCRIPT_VMS] =
{
	// type, file name, CRC of defs, internal name

	{VM_NONE, NULL, 0, "shared"}, // dummy for shared builtins

	{VM_SVGAME, "progs/svgame.dat", 49005, "server game"},
	{VM_CLGAME, "progs/cgame.dat", 15591, "client game"},
	{VM_GUI, "progs/gui.dat", 0, "user interface"}
};

void Cmd_PrintVMEntity_f(void);
void Cmd_PrintAllVMEntities_f(void);

extern void CG_InitScriptBuiltins();
extern void UI_InitScriptBuiltins();
extern void SV_InitScriptBuiltins();

/*
============
Scr_GetScriptName
============
*/
const char* Scr_GetScriptName(vmType_t vm)
{
	return vmDefs[vm].name;
}

/*
============
ScrInternal_GlobalAtOfs
============
*/
ddef_t* ScrInternal_GlobalAtOfs(int ofs)
{
	ddef_t* def;
	int			i;

	CheckScriptVM(__FUNCTION__);
	for (i = 0; i < active_qcvm->progs->numGlobalDefs; i++)
	{
		def = &active_qcvm->pGlobalDefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
ED_FieldAtOfs
============
*/
ddef_t* ScrInternal_FieldAtOfs(int ofs)
{
	ddef_t	*def;
	int		i;

	CheckScriptVM(__FUNCTION__);
	for (i = 0; i < active_qcvm->progs->numFieldDefs; i++)
	{
		def = &active_qcvm->pFieldDefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return NULL;
}

/*
============
Scr_FindEntityField
Returns ddef for an entity field matching name in active qcvm
============
*/
ddef_t* Scr_FindEntityField(char* name)
{
	ddef_t	*def;
	int		i;

	CheckScriptVM(__FUNCTION__);
	for (i = 0; i < active_qcvm->progs->numFieldDefs; i++)
	{
		def = &active_qcvm->pFieldDefs[i];
		if (!strcmp(Scr_GetString(def->s_name), name))
		{
			return def;
		}
	}
	return NULL;
}

#if 1
scr_string_t Scr_AllocString(int size, char** ptr);

static scr_string_t Scr_NewString(const char* string)
{
	char* new_p;
	int		i, l;
	scr_string_t num;

	l = (int)strlen(string) + 1;
	num = Scr_AllocString(l, &new_p);

	for (i = 0; i < l; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return num;
}

qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s, int memtag)
{
	int		i;
	char	string[128];
	ddef_t* def;
	char* v, * w;
	void* d;
	scr_func_t func;

	d = (void*)((int*)base + key->ofs);

	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(scr_string_t*)d = Scr_NewString(s);
		break;

	case ev_float:
		*(float*)d = atof(s);
		break;

	case ev_integer:
		*(int*)d = atoi(s);
		break;

	case ev_vector:
		strcpy(string, s);
		v = string;
		w = string;
		for (i = 0; i < 3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float*)d)[i] = atof(w);
			w = v = v + 1;
		}
		break;

	case ev_entity:
		*(int*)d = ENT_TO_VM(ENT_FOR_NUM(atoi(s)));
		break;

	case ev_field:
		def = Scr_FindEntityField(s);
		if (!def)
		{
			return false;
		}
		*(int*)d = G_INT(def->ofs);
		break;

	case ev_function:
		func = Scr_FindFunctionIndex(s);
		if (func == -1)
		{
			Com_Error(ERR_FATAL, "Can't find function '%s' in %s QCVM.\n", s, Scr_GetScriptName(active_qcvm->progsType));
			return false;
		}
		*(scr_func_t*)d = func;
		break;

	default:
		break;
	}
	return true;
}

#else
/*
=============
Scr_ParseEpair

Can parse either fields or globals, MUST BIND VM BEFORE USE! returns false if error
=============
*/
qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s, int memtag)
{
	int		i;
	char	string[128];
	ddef_t* def;
	char* v, * w;
	void* d;
	scr_func_t func;

	d = (void*)((int*)base + key->ofs);

	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(scr_string_t*)d = COM_NewString(s, memtag) - active_qcvm->pStrings;
		break;

	case ev_float:
		*(float*)d = atof(s);
		break;

	case ev_integer:
		*(int*)d = atoi(s);
		break;

	case ev_vector:
		strcpy(string, s);
		v = string;
		w = string;
		for (i = 0; i < 3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float*)d)[i] = atof(w);
			w = v = v + 1;
		}
		break;

	case ev_entity:
		*(int*)d = ENT_TO_VM(ENT_FOR_NUM(atoi(s)));
		break;

	case ev_field:
		def = Scr_FindEntityField(s);
		if (!def)
		{
			//if (strncmp(s, "sky", 3) && strcmp(s, "fog"))
			//	Com_DPrintf(DP_ALL, "Can't find field %s\n", s);
			return false;
		}
		*(int*)d = G_INT(def->ofs);
		break;

	case ev_function:
		func = Scr_FindFunctionIndex(s);
		if (func == -1)
		{
			Com_Error(ERR_FATAL, "Can't find function '%s' in %s program.\n", s, Scr_GetScriptName(active_qcvm->progsType));
			return false;
		}
		*(scr_func_t*)d = func;
		break;

	default:
		break;
	}
	return true;
}
#endif

/*
============
Scr_FindGlobal

Returns ddef for a global variable in active qcvm
============
*/
ddef_t* Scr_FindGlobal(char* name)
{
	ddef_t* def;
	int			i;

	CheckScriptVM(__FUNCTION__);
	for (i = 0; i < active_qcvm->progs->numGlobalDefs; i++)
	{
		def = &active_qcvm->pGlobalDefs[i];
		if (!strcmp(Scr_GetString(def->s_name), name))
			return def;
	}
	return NULL;
}


/*
============
Scr_FindFunctionIndex
Finds a function with specified name in active qcvm or NULL if not found
============
*/
dfunction_t* Scr_FindFunction(const char* name)
{
	dfunction_t* func;
	int				i;

	CheckScriptVM(__FUNCTION__);
	for (i = 0; i < active_qcvm->progs->numFunctions; i++)
	{
		func = &active_qcvm->pFunctions[i];
		if (!strcmp(Scr_GetString(func->s_name), name))
			return func;
	}
	return NULL;
}


/*
===============
Scr_GenerateBuiltinsDefs

Writes builtins QC header file to disk
===============
*/
void Scr_GenerateBuiltinsDefs(char *filename, pb_t execon)
{
	int i, count = 0;
	FILE* file;

	for (i = 1; i != scr_numBuiltins; i++)
	{
		if (scr_builtins[i].execon == execon)
			count++; //this is naive and lazy
	}

	if (!count)
		return;

	FS_CreatePath(filename);
	file = fopen(filename, "wb");

	if (!file)
	{
		Com_Printf("Failed to open %s for writing.\n", filename);
		return;
	}

	fprintf(file, "// This file was generated by PRAGMA %s (build: %s)\n", PRAGMA_VERSION, PRAGMA_TIMESTAMP);
	fprintf(file, "// Do not edit, use vm_generatedefs command to rebuild header.\n");
	fprintf(file, "// %i builtin functions.\n\n", count);

	for (i = 1; i != scr_numBuiltins; i++)
	{
		if (scr_builtins[i].execon != execon)
			continue;

		fprintf(file, "%s %s = #%i;\n", scr_builtins[i].qcstring, scr_builtins[i].name, i);
	}

	fclose(file);

	Com_Printf("Generated QC header: %s (%i builtins)\n", filename, count);
}

extern char** pr_stringTable;
/*
===============
Scr_LoadProgram

Load qc programs from file and set proper entity size
===============
*/
void Scr_LoadProgram(qcvm_t *vm, char* filename)
{
	int		len, i;
	byte	*raw;

	if (!vm)
		Com_Error(ERR_FATAL, "%s with NULL QCVM.\n", __FUNCTION__);

	if(vm->progs)
		Com_Error(ERR_FATAL, "Tried to load second instance of %s QCVM.\n", __FUNCTION__, Scr_GetScriptName(vm->progsType));

	// load file
	len = FS_LoadFile(filename, (void**)&raw);
	if (!len || len == -1)
	{
		Com_Error(ERR_FATAL, "Could not load \"%s\".\n", filename);
		return;
	}

	vm->progsSize = len;
	vm->progs = (dprograms_t*)raw;
	active_qcvm = vm; // just in case..

	// byte swap the header
	for (i = 0; i < sizeof(*vm->progs) / sizeof(int32_t); i++)
		((int*)vm->progs)[i] = LittleLong(((int*)vm->progs)[i]);

	if (vm->progs->version != PROG_VERSION)
	{
		if (vm->progs->version == PROG_VERSION_FTE)
			Com_Printf("%s: \"%s\" is FTE version and not all opcodes are supported in pragma\n", __FUNCTION__, filename);
		else
			Com_Error(ERR_FATAL, "%s: \"%s\" is wrong version %i (should be %i)\n", __FUNCTION__, filename, vm->progs->version, PROG_VERSION);
	}

	vm->crc = CRC_Block(vm->progs, len);

	if (vmDefs[vm->progsType].defs_crc_checksum != 0 && vm->progs->crc != vmDefs[vm->progsType].defs_crc_checksum )
	{
		if (developer->value)
		{
			Com_Error(ERR_DROP, "\"%s\" is compiled against diferent engine version.\nExpected defs checksum %i but found %i.\n", filename, vmDefs[vm->progsType].defs_crc_checksum, vm->progs->crc);
		}
		else
		{
			Com_Error(ERR_DROP, "\"%s\" is compiled against diferent engine version.\n", filename);
		}
	}

	// cast the data from progs
	vm->pFunctions = (dfunction_t*)((byte*)vm->progs + vm->progs->ofs_functions);

	vm->pStrings = (char*)vm->progs + vm->progs->ofs_strings;
	if ((vm->progs->ofs_strings + vm->progs->numstrings) >= len)
		Com_Error(ERR_FATAL, "Strings in %s program are corrupt.\n", filename);

	vm->pGlobalDefs = (ddef_t*)((byte*)vm->progs + vm->progs->ofs_globaldefs);
	vm->pFieldDefs = (ddef_t*)((byte*)vm->progs + vm->progs->ofs_fielddefs);
	vm->pStatements = (dstatement_t*)((byte*)vm->progs + vm->progs->ofs_statements);
	vm->pGlobalsStruct = ((byte*)vm->progs + vm->progs->ofs_globals);	
	vm->pGlobals = (float*)vm->pGlobalsStruct;
	vm->entity_size = vm->entity_size + (vm->progs->entityfields * sizeof(int32_t));

	// [QSS] round off to next highest whole word address (esp for Alpha)
	// this ensures that pointers in the engine data area are always
	// properly aligned
	vm->entity_size += sizeof(void*) - 1;
	vm->entity_size &= ~(sizeof(void*) - 1);

	//vm->entity_size = vm->entity_size + (vm->progs->entityfields * 4);
	
	static qboolean strinit = false;

	if(!strinit)
	{
		strinit = true;
		pr_stringTable = NULL;
		Scr_SetString("");
	}

	// byte swap all the data
	for (i = 0; i < vm->progs->numStatements; i++)
	{
		vm->pStatements[i].op = LittleShort(vm->pStatements[i].op);
		vm->pStatements[i].a = LittleShort(vm->pStatements[i].a);
		vm->pStatements[i].b = LittleShort(vm->pStatements[i].b);
		vm->pStatements[i].c = LittleShort(vm->pStatements[i].c);
	}
	for (i = 0; i < vm->progs->numFunctions; i++)
	{
		vm->pFunctions[i].first_statement = LittleLong(vm->pFunctions[i].first_statement);
		vm->pFunctions[i].parm_start = LittleLong(vm->pFunctions[i].parm_start);
		vm->pFunctions[i].s_name = LittleLong(vm->pFunctions[i].s_name);
		vm->pFunctions[i].s_file = LittleLong(vm->pFunctions[i].s_file);
		vm->pFunctions[i].numparms = LittleLong(vm->pFunctions[i].numparms);
		vm->pFunctions[i].locals = LittleLong(vm->pFunctions[i].locals);
	}

	for (i = 0; i < vm->progs->numGlobalDefs; i++)
	{
		vm->pGlobalDefs[i].type = LittleShort(vm->pGlobalDefs[i].type);
		vm->pGlobalDefs[i].ofs = LittleShort(vm->pGlobalDefs[i].ofs);
		vm->pGlobalDefs[i].s_name = LittleLong(vm->pGlobalDefs[i].s_name);
	}

	for (i = 0; i < vm->progs->numFieldDefs; i++)
	{
		vm->pFieldDefs[i].type = LittleShort(vm->pFieldDefs[i].type);
		if (vm->pFieldDefs[i].type & DEF_SAVEGLOBAL)
			Com_Error(ERR_FATAL, "pr_fielddefs[%i].type & DEF_SAVEGLOBAL\n", i);

		vm->pFieldDefs[i].ofs = LittleShort(vm->pFieldDefs[i].ofs);
		vm->pFieldDefs[i].s_name = LittleLong(vm->pFieldDefs[i].s_name);
	}

	for (i = 0; i < vm->progs->numGlobals; i++)
		((int*)vm->pGlobals)[i] = LittleLong(((int32_t*)vm->pGlobals)[i]);
}

/*
===============
Scr_OpenLogFileForVM

Opens a logfile for qcvm in developer mode, this allows logprint() to save to disk
===============
*/
static void Scr_OpenLogFileForVM(qcvm_t *vm)
{
	char name[MAX_OSPATH];

//	if (vm->logfile)
//	{
//		Com_Printf("closed logfile %s\n");
//		fclose(vm->logfile);
//		vm->logfile = NULL;
//		return;
//	}

	if (!developer->value)
		return;

	sprintf(name, "%s/%s.log", FS_Gamedir(), Scr_GetScriptName(vm->progsType));

	vm->logfile = fopen(name, "w");
	if (!vm->logfile)
		return;

	Com_Printf("opened logfile: %s\n", name);

	fprintf(vm->logfile, "---- opened logfile %s ----\n", GetTimeStamp(true));
	fflush(active_qcvm->logfile);
}

/*
===============
Scr_CreateScriptVM

Create script execution context
===============
*/

void Scr_CreateScriptVM(vmType_t vmType, unsigned int numEntities, size_t entitySize, size_t entvarOfs)
{
	Scr_BindVM(VM_NONE);
	Scr_FreeScriptVM(vmType);

//	if (qcvm[progsType] != NULL)
//		Com_Error(ERR_FATAL, "Tried to create second instance of %s script VM\n", Scr_GetScriptName(progsType));

	qcvm[vmType] = Z_Malloc(sizeof(qcvm_t));
	if (qcvm == NULL)
		Com_Error(ERR_FATAL, "Couldn't allocate %s VM.\n", Scr_GetScriptName(vmType));

	qcvm_t* vm = qcvm[vmType];
	vm = qcvm[vmType];
	vm->progsType = vmType;

	vm->num_entities = numEntities;
	vm->offsetToEntVars = (int)entvarOfs;
	vm->entity_size = (int)entitySize; // invaild now, will be set properly in loadprogs

	// load progs from file
	Scr_LoadProgram(vm, vmDefs[vmType].filename);

	// allocate entities
	vm->entities = (vm_entity_t*)Z_Malloc(vm->num_entities * vm->entity_size);
	if (vm->entities == NULL)
		Com_Error(ERR_FATAL, "Couldn't allocate entities for %s VM.\n", Scr_GetScriptName(vmType));

	// open devlog
	Scr_OpenLogFileForVM(vm);

	Com_Printf("Spawned %s QCVM from file \"%s\".\n", Scr_GetScriptName(vm->progsType), vmDefs[vmType].filename);

	// print statistics
	if (developer->value)
	{
		dprograms_t* progs = vm->progs;
		Com_Printf("-------------------------------------\n");
		Com_Printf("%s QCVM: '%s'\n", Scr_GetScriptName(vm->progsType), vmDefs[vmType].filename);
		Com_Printf("          Functions: %i\n", progs->numFunctions);
		Com_Printf("         Statements: %i\n", progs->numStatements);
		Com_Printf("         GlobalDefs: %i\n", progs->numGlobalDefs);
		Com_Printf("            Globals: %i\n", progs->numGlobals);
		Com_Printf("      Entity fields: %i\n", progs->numFieldDefs);
		Com_Printf("     Strings length: %i\n", progs->numstrings);
		Com_Printf(" Allocated entities: %i, %i bytes\n", vm->num_entities, vm->num_entities * Scr_GetEntitySize());
		Com_Printf("        Entity size: %i bytes\n", Scr_GetEntitySize());
		Com_Printf("\n");
		Com_Printf("       Programs CRC: %i\n", vm->crc);
		Com_Printf("      Programs size: %i bytes (%iKb)\n", vm->progsSize, vm->progsSize / 1024);
		Com_Printf("-------------------------------------\n");
	}
}

/*
===============
Scr_DestroyScriptVM

Destroy QC virtual machine
===============
*/
void Scr_FreeScriptVM(vmType_t vmtype)
{
	qcvm_t *vm = qcvm[vmtype];

	if (!vm)
		return;

	if (vm->entities)
		Z_Free(vm->entities);

	if (vm->progs)
		Z_Free(vm->progs);

	if (vm->logfile)
	{
		fprintf(vm->logfile, "---- closed logfile %s ----\n", GetTimeStamp(true));
		fclose(vm->logfile);
		vm->logfile = NULL;
	}

	if (vm->progsType == VM_CLGAME)
	{
		Cmd_RemoveClientGameCommands();
	}

	Z_Free(vm);
	qcvm[vmtype] = NULL;

	Scr_BindVM(VM_NONE);
	Com_Printf("Freed %s script...\n", Scr_GetScriptName(vmtype));
}

#if 0
void Cmd_Script_PrintFunctions(void)
{
	qcvm_t* vm = active_qcvm;
	if (!vm)
		return;

	for (int i = scr_numBuiltins; i < vm->progs->numFunctions; i++) // unsafe start
	{
		char* srcFile = ScrInternal_String(vm->pFunctions[i].s_file);
		char* funcName = ScrInternal_String(vm->pFunctions[i].s_name);
		int numParms = (vm->pFunctions[i].numparms);
		if (numParms)
			printf("#%i %s:%s(#%i)\n", i, srcFile, funcName, numParms);
		else
			printf("#%i %s:%s()\n", i, srcFile, funcName);
	}
}
#endif

/*
===============
Scr_IsVMLoaded

Return true if a specified qcvm is loaded
===============
*/
qboolean Scr_IsVMLoaded(vmType_t vmtype)
{
	if (qcvm[vmtype] == NULL)
		return false;
	return true;
}

/*
===============
Scr_BindVM

Select (bind) QCVM for use, this is important for Get/Set/Call/Return scr functions
===============
*/
void Scr_BindVM(vmType_t vmtype)
{
	if (qcvm[vmtype] == NULL)
		return;
	active_qcvm = qcvm[vmtype];
	CheckScriptVM(__FUNCTION__);
}

/*
===============
Scr_GetGlobals
===============
*/
void* Scr_GetGlobals()
{
	CheckScriptVM(__FUNCTION__);
	return active_qcvm->pGlobalsStruct;
}

/*
===============
Scr_GetEntitySize
===============
*/
int Scr_GetEntitySize()
{
	CheckScriptVM(__FUNCTION__);
	return active_qcvm->entity_size;
}

/*
===============
Scr_GetEntityPtr
===============
*/
vm_entity_t* Scr_GetEntityPtr()
{
	CheckScriptVM(__FUNCTION__);
	return active_qcvm->entities;
}

/*
===============
Scr_GetProgsCRC
===============
*/
extern unsigned Scr_GetProgsCRC(vmType_t vmType)
{
	if (qcvm[vmType] == NULL)
		return 0;
	return qcvm[vmType]->crc;
}

/*
===============
Scr_GetEntityFieldsSize
===============
*/
int Scr_GetEntityFieldsSize()
{
	CheckScriptVM(__FUNCTION__);
	return (active_qcvm->progs->entityfields * sizeof(int32_t));
}

/*
===============
Cmd_VM_GenerateDefs_f

vm_generatedefs command
===============
*/
void Cmd_VM_GenerateDefs_f(void)
{
	if (!developer->value)
	{
		Com_Printf("developer mode must be enabled for 'vm_generatedefs'\n");
		return;
	}

	for (vmType_t type = 0; type < NUM_SCRIPT_VMS; type++)
	{
		// always write to current gamedir/moddir
		Scr_GenerateBuiltinsDefs(va("%s/progs_src/inc/pragma_funcs_%s.qc", FS_Gamedir(), vmDefs[type].name), type);
	}
}

/*
===============
Scr_PreInitVMs

initialize qcvms builtin functions, commands and cvars
===============
*/
void Scr_PreInitVMs()
{
	vmType_t type;
	active_qcvm = NULL;

	for (type = 0; type < NUM_SCRIPT_VMS; type++)
		qcvm[type] = NULL;

	scr_numBuiltins = 0;

	Scr_InitSharedBuiltins();
	CG_InitScriptBuiltins();
	UI_InitScriptBuiltins();
	SV_InitScriptBuiltins();

	vm_runaway = Cvar_Get("vm_runaway", va("%i", VM_DEFAULT_RUNAWAY), 0, "Count of executed QC instructions to trigger runaway error.");

	// add developer comands
	Cmd_AddCommand("vm_printent", Cmd_PrintVMEntity_f);
	Cmd_AddCommand("vm_printents", Cmd_PrintAllVMEntities_f);
	Cmd_AddCommand("vm_generatedefs", Cmd_VM_GenerateDefs_f);
}

/*
===============
Scr_Shutdown

free all qcvms and associated builtins
===============
*/
void Scr_Shutdown()
{
	Scr_BindVM(VM_NONE);

	Cmd_RemoveCommand("vm_printent");
	Cmd_RemoveCommand("vm_printents");
	Cmd_RemoveCommand("vm_generatedefs");

	if (scr_builtins)
	{
		Z_Free(scr_builtins);
		scr_builtins = NULL;
		Com_DPrintf(DP_SCRIPT, "Freed script VM builtins...\n");
	}

	for (vmType_t i = 1; i < NUM_SCRIPT_VMS; i++)
	{
		if(qcvm[i])
			Scr_FreeScriptVM(i);
	}
}

#ifndef DEDICATED_ONLY

#if 0
typedef enum
{
	XALIGN_NONE = 0,
	XALIGN_LEFT = 0,
	XALIGN_RIGHT = 1,
	XALIGN_CENTER = 2
} UI_AlignX;
void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);

void PR_Profile(int x, int y)
{
	dfunction_t* f, * best;
	int			max;
	int			num;
	int			i;
	static int nexttime = 0;

	static char str[10][96];

	if (Sys_Milliseconds() > nexttime + 100)
	{
		nexttime = Sys_Milliseconds();
		memset(&str, 0, sizeof(str));
		num = 0;
		do
		{
			max = 0;
			best = NULL;
			for (i = 0; i < active_qcvm->progs->numFunctions; i++)
			{
				f = &active_qcvm->pFunctions[i];
				if (f->profile > max)
				{
					max = f->profile;
					best = f;
				}
			}
			if (best)
			{
				if (num < 10)
				{
					Com_sprintf(str[num], sizeof(str[num]), "%7i %s in %s", best->profile, ScrInternal_String(best->s_name), ScrInternal_String(best->s_file));
					Com_sprintf(str[num], sizeof(str[num]), "%7i:%s", best->profile, ScrInternal_String(best->s_name));
					//Com_Printf(str[num]);
					//UI_DrawString(x, y, XALIGN_RIGHT, str[num]);
				}

				num++;
				best->profile = 0;
			}
		} while (best);
	}

	for(i = 0; i < 10; i++)
		UI_DrawString(x, y+10*i, XALIGN_RIGHT, str[i]);
}

#endif
#endif
