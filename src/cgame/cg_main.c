/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

#include "cg_local.h"

cgame_import_t	gi;
cgame_export_t	globals;


void printcs(char* csname, int start, int max)
{
	int i, j;
	j = 0;

	char* configstring;

	for (i = start; i < start + max; i++)
	{
		configstring = gi.GetConfigString(i);

		if (configstring[0]);
		{
			Com_Printf("CG_PrintCS(%s) %i: %s\n", csname, j, configstring);
			j++;
		}
	}
}

void CG_NewMap(char* mapname)
{
	printcs("CS_MODELS", CS_MODELS, MAX_MODELS);
	printcs("CS_SOUNDS", CS_SOUNDS, MAX_SOUNDS);
	printcs("CS_IMAGES", CS_IMAGES, MAX_IMAGES);
}
extern void CG_DrawHUD(void);
extern void CG_DrawUI(void);
extern void CG_ParseTempEntityMessage(int type);

void CG_Frame(float frametime, float time, float realtime, int width, int height)
{
//	gi.Printf("CG_Frame : %f\n", time);
}

int InitClientGame(void)
{
	gi.Printf("====== InitClientGame ======\n");
	gi.Printf("cgame build: %s %s\n", __DATE__, __TIME__);
	gi.Printf("cgame API: %i\n", CGAME_API_VERSION);
	gi.Printf("============================\n");
	return 1;
}

void ShutdownClientGame(void)
{
	gi.FreeTags(TAG_CGAME);
	gi.FreeTags(TAG_CGAME_LEVEL);
}

/*
=================
GetClientGameAPI
=================
*/
cgame_export_t* GetClientGameAPI(cgame_import_t* import)
{
	gi = *import;

	globals.apiversion = CGAME_API_VERSION;

	globals.Init = InitClientGame;
	globals.Shutdown = ShutdownClientGame;

	globals.Frame = CG_Frame;
	globals.NewMap = CG_NewMap;
	globals.DrawHUD = CG_DrawHUD;
	globals.DrawUI = CG_DrawUI;

	globals.ParseTempEnt = CG_ParseTempEntityMessage;

	return &globals;
}