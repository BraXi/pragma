#include "client.h"

//#include "../cgame/cgame.h"

extern void* Sys_GetClientGameAPI(void* parms);
extern void Sys_UnloadClientGame(void);
extern void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);

cgame_import_t* import;
cgame_export_t* cgame;

//
// functions exported to cgame
//
static void CG_Error(char* fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	Com_Error(ERR_FATAL, "Client Game Error: %s", msg);
}

static char* CL_GetConfigString(int index)
{
	if (index >= MAX_CONFIGSTRINGS || index < 0)
		Com_Error(ERR_FATAL, "CL_GetConfigString out of range\n");
	return cl.configstrings[index];
}

static int	CG_MSG_ReadChar(void) { return MSG_ReadChar(&net_message); }
static int	CG_MSG_ReadByte(void) { return MSG_ReadByte(&net_message); }
static int	CG_MSG_ReadShort(void) { return MSG_ReadShort(&net_message); }
static int	CG_MSG_ReadLong(void) { return MSG_ReadLong(&net_message); }
static float CG_MSG_ReadFloat(void) { return MSG_ReadFloat(&net_message); }
static float CG_MSG_ReadCoord(void) { return MSG_ReadCoord(&net_message); }
static void CG_MSG_ReadPos(vec3_t pos) { MSG_ReadPos(&net_message, pos); }
static float CG_MSG_ReadAngle(void) { return MSG_ReadAngle(&net_message); }
static float CG_MSG_ReadAngle16(void) { return MSG_ReadAngle16(&net_message); }
static void CG_MSG_ReadDir(vec3_t vector) { MSG_ReadDir(&net_message, vector); }

void CG_DrawString(char* string, float x, float y, float fontSize, int alignx, rgba_t color)
{
	re.DrawString(string, x, y, fontSize, alignx, color);
}

void CG_DrawStretchedImage(rect_t rect, rgba_t color, char* pic)
{
	re.DrawStretchedImage(rect, color, pic);
}

void CG_DrawFill(rect_t rect, rgba_t color)
{
	re.NewDrawFill(rect, color);
}

void CG_GetCursorPos(vec2_t* out)
{
}

void CG_SetCursorPos(int x, int y)
{
}

//==============================================

/*
===============
CL_ShutdownClientGame

Called when engine is closing or it is changing to a different game directory.
===============
*/
void CL_ShutdownClientGame(void)
{
	if (!cgame)
		return;
	cgame->Shutdown();
	Sys_UnloadClientGame();
	cgame = NULL;
}



/*
===============
CL_InitClientGame

Called when engine is starting or it is changing to a different game (mod) directory.
===============
*/
void CL_InitClientGame(void)
{
	Com_Printf("CL_InitClientGame\n");

	cgame_import_t	import;

	// unload anything we have now
	if (cgame)
		CL_ShutdownClientGame();

	import.Printf = Com_Printf;
	import.Error = CG_Error;

	import.GetConfigString = CL_GetConfigString;

	import.TagMalloc = Z_TagMalloc;
	import.TagFree = Z_Free;
	import.FreeTags = Z_FreeTags;

	import.MSG_ReadChar = CG_MSG_ReadChar;
	import.MSG_ReadByte = CG_MSG_ReadByte;
	import.MSG_ReadShort = CG_MSG_ReadShort;
	import.MSG_ReadLong = CG_MSG_ReadLong;
	import.MSG_ReadFloat = CG_MSG_ReadFloat;
	import.MSG_ReadCoord = CG_MSG_ReadCoord;
	import.MSG_ReadPos = CG_MSG_ReadPos;
	import.MSG_ReadAngle = CG_MSG_ReadAngle;
	import.MSG_ReadAngle16 = CG_MSG_ReadAngle16;
	import.MSG_ReadDir = CG_MSG_ReadDir;

	import.DrawString = CG_DrawString;
	import.DrawStretchedImage = CG_DrawStretchedImage;
	import.DrawFill = CG_DrawFill;

	import.GetCursorPos = CG_GetCursorPos;
	import.SetCursorPos = CG_SetCursorPos;

	import.cvar = Cvar_Get;
	import.cvar_set = Cvar_Set;
	import.cvar_forceset = Cvar_ForceSet;

	cgame = (cgame_export_t*)Sys_GetClientGameAPI(&import);

	if (!cgame)
	{
		Com_Error(ERR_FATAL, "Failed to load Client Game");
		return; // to shut up msvc
	}

	if (cgame->apiversion != CGAME_API_VERSION)
		Com_Error(ERR_DROP, "Client Game is version %i, not %i", cgame->apiversion, CGAME_API_VERSION);

	cgame->Init();
}