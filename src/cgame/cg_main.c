#include "../client/client.h"

extern void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);

static char* CL_GetConfigString(int index)
{
	if (index >= MAX_CONFIGSTRINGS || index < 0)
		Com_Error(ERR_FATAL, "CL_GetConfigString out of range\n");
	return cl.configstrings[index];
}

static int	PFCG_MSG_ReadChar(void) { return MSG_ReadChar(&net_message); }
static int	PFCG_MSG_ReadByte(void) { return MSG_ReadByte(&net_message); }
static int	PFCG_MSG_ReadShort(void) { return MSG_ReadShort(&net_message); }
static int	PFCG_MSG_ReadLong(void) { return MSG_ReadLong(&net_message); }
static float PFCG_MSG_ReadFloat(void) { return MSG_ReadFloat(&net_message); }
static float PFCG_MSG_ReadCoord(void) { return MSG_ReadCoord(&net_message); }
static void PFCG_MSG_ReadPos(vec3_t pos) { MSG_ReadPos(&net_message, pos); }
static float PFCG_MSG_ReadAngle(void) { return MSG_ReadAngle(&net_message); }
static float PFCG_MSG_ReadAngle16(void) { return MSG_ReadAngle16(&net_message); }
static void PFCG_MSG_ReadDir(vec3_t vector) { MSG_ReadDir(&net_message, vector); }

void PFCG_DrawString(char* string, float x, float y, float fontSize, int alignx, rgba_t color)
{
	re.DrawString(string, x, y, fontSize, alignx, color);
}

void PFCG_DrawStretchedImage(rect_t rect, rgba_t color, char* pic)
{
	re.DrawStretchedImage(rect, color, pic);
}

void PFCG_DrawFill(rect_t rect, rgba_t color)
{
	re.NewDrawFill(rect, color);
}

void CG_GetCursorPos(vec2_t* out)
{
}

void CG_SetCursorPos(int x, int y)
{
}

/*
===============
CL_ShutdownClientGameProgs

Called when engine is closing or it is changing to a different game directory.
===============
*/
void CL_ShutdownClientGameProgs(void)
{
}

/*
===============
CL_InitClientGameProgs

Called when engine is starting or it is changing to a different game (mod) directory.
===============
*/
void CL_InitClientGameProgs(void)
{
	Com_Printf("CL_InitClientGame\n");
}