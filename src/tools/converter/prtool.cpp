/*
prtool, part of pragma
Copyright (C) 2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "prtool.h"

char g_gamedir[MAXPATH];
char g_devdir[MAXPATH];
char g_controlFileName[MAXPATH];
bool g_verbose = false;

static void Opt_NotImplemented()
{
	Com_Error("Option Not implemented.");
}

extern void Opt_CreatePack();
extern void Opt_ConvertAsset();

command_t commands[] =
{
	{"convert", Opt_ConvertAsset},
	{"pack", Opt_CreatePack},
	{"extract", Opt_NotImplemented},
	{"info", Opt_NotImplemented},
	{"decompile", Opt_NotImplemented}
};

static const int numCommands = sizeof(commands) / sizeof(command_t);


static int LoadConfig()
{
	static char line[MAXLINELEN];
	FILE* f = NULL;
	int line_num = 0;

	memset(g_gamedir, 0, sizeof(g_gamedir));
	memset(g_devdir, 0, sizeof(g_devdir));

	f = Com_OpenReadFile("prtool.qc", true);


	Com_Printf("Loading configuration file prtool.qc ...\n");
	while (fgets(line, sizeof(line), f) != NULL)
	{
		line_num++;
		Com_Tokenize(line);

		if (Com_NumArgs() == 0)
			continue;

		if (!strcmp("$gamedir", Com_GetArg(0)))
		{
			if (g_gamedir[0] != 0)
				Com_Error("redefinition of $gamedir at line %i", line_num);

			if(Com_NumArgs() == 1)
				Com_Error("$gamedir without value at line %i", line_num);

			strncpy(g_gamedir, Com_GetArg(1), sizeof(g_gamedir));
			
		}
		else if (!strcmp("$devdir", Com_GetArg(0)))
		{
			if (g_devdir[0] != 0)
				Com_Error("redefinition of $devdir at line %i", line_num);

			if (Com_NumArgs() == 1)
				Com_Error("$devdir without value at line %i", line_num);

			strncpy(g_devdir, Com_GetArg(1), sizeof(g_devdir));
		}
		else if (!strcmp("$verbose", Com_GetArg(0)))
		{
			g_verbose = true;
			Com_Printf("Verbose mode enabled.\n");
		}
	}
	
	if (g_gamedir[0] == 0)
	{
		strncpy(g_gamedir, DEFAULT_GAME_DIR, sizeof(g_gamedir));
		//Com_Printf("Using default game directory: %s\n", g_gamedir);
	}

	if (g_devdir[0] == 0)
	{
		strncpy(g_devdir, DEFAULT_DEV_DIR, sizeof(g_devdir));
		//Com_Printf("Using default development directory: %s\n", g_devdir);
	}

	Com_Printf("Game directory        : %s\n", g_gamedir);
	Com_Printf("Development directory : %s\n", g_devdir);

	Com_Printf("\n");
	return 1;
}

int main(int argc, char** argv)
{
	int i;

	Com_Printf("Pragma SDK %s " __DATE__ " (c) 2024 Paulina 'braxi'.\n", PRTOOL_VERSION);
	Com_Printf("github.com/braxi/pragma \n\n", PRTOOL_VERSION);

	if (argc != 3)
	{
		Com_Warning("usage: prtool -option file.ext\n");

		Com_Printf("Options: \n");
		Com_Printf("   prtool -convert file.qc : Convert assets defined in control file.\n");
		Com_Printf("   -pack filelist.txt : Build PAK archive, will automaticaly find and pack dependencies.\n");
		//Com_Printf("   -extract data.pak outdir : Extract PAK archive into directory.\n");
		//Com_Printf("   -info file.ext : Print informations about BSP, PAK, MD3, TGA, MOD or ANM file.\n");
		//Com_Printf("   -decompile file.ext : Convert MOD or ANM file back into SMD(s) and write QC control file.\n\n");
		Com_Exit(1);
	}

	LoadConfig();

	memset(g_controlFileName, 0, sizeof(g_controlFileName));
	strncpy(g_controlFileName, argv[2], sizeof(g_controlFileName));

	for (i = 0; i < numCommands; i++)
	{
		if (!strcmp(argv[1]+1, commands[i].name))
		{
			commands[i].pFunc();
			break;
		}
	}
	Com_Exit(0);
}



