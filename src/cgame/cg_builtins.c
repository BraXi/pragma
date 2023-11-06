#include "../client/client.h"


extern void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);

/*
=================
PFCG_getconfigstring

returns configstring by index
str = getconfigstring(index);
=================
*/
static void PFCG_getconfigstring(void)
{
	int idx = Scr_GetParmInt(0);
	if ( idx < 0 || idx >= MAX_CONFIGSTRINGS)
	{
		Scr_RunError("getconfigstring(): %i out of range\n", idx);
		return;
	}
	Scr_ReturnString(cl.configstrings[idx]);
}

/*
=================
PFCG_getstat

get stat
=================
*/
static void PFCG_getstat()
{
	int index = Scr_GetParmFloat(0);
	if (index < 0 || index >= MAX_STATS)
	{
		Com_Printf("getstat(): out of range %i [0-%i]\n", index, MAX_STATS);
		Scr_ReturnFloat(0);
		return;
	}
	Scr_ReturnFloat(cl.frame.playerstate.stats[index]);
}

// read network packets
static void PFCG_MSG_ReadChar(void)		{ Scr_ReturnInt(MSG_ReadChar(&net_message)); }
static void PFCG_MSG_ReadByte(void)		{ Scr_ReturnInt(MSG_ReadByte(&net_message)); }
static void PFCG_MSG_ReadShort(void)	{ Scr_ReturnInt(MSG_ReadShort(&net_message)); }
static void PFCG_MSG_ReadLong(void)		{ Scr_ReturnInt(MSG_ReadLong(&net_message)); }
static void PFCG_MSG_ReadFloat(void)	{ Scr_ReturnFloat(MSG_ReadFloat(&net_message)); }
static void PFCG_MSG_ReadCoord(void)	{ Scr_ReturnFloat(MSG_ReadCoord(&net_message)); }
static void PFCG_MSG_ReadPos(void)		{ vec3_t v; MSG_ReadPos(&net_message, v); Scr_ReturnVector(v); }
static void PFCG_MSG_ReadAngle(void)	{ Scr_ReturnFloat(MSG_ReadAngle(&net_message)); }
static void PFCG_MSG_ReadAngle16(void)	{ Scr_ReturnFloat(MSG_ReadAngle16(&net_message)); }
static void PFCG_MSG_ReadDir(void)		{ /*PFCG_MSG_ReadPos();*/  vec3_t v; MSG_ReadDir(&net_message, v); Scr_ReturnVector(v); }

/*
=================
PFCG_drawstring

drawstring( vector xy_align, float fontSize, vector color, float alpha, string text, ... );
=================
*/
static void PFCG_drawstring(void)
{
	float color[4];

	float* xy_align = Scr_GetParmVector(0);
	float fontSize = Scr_GetParmFloat(1);

	Scr_GetParmVector2(2, &color[0], &color[1], &color[2]);
	color[3] = Scr_GetParmFloat(3);

	char* string = Scr_VarString(4);

	re.DrawString(string, xy_align[0], xy_align[1], fontSize, xy_align[2], color);
}

/*
=================
PFCG_drawimage

drawimage( float x, float y, float w, float h, vector color, float alpha, string imagename );
=================
*/
static void PFCG_drawimage(void)
{
	float rect[4], color[4];

	rect[0] = Scr_GetParmFloat(0);
	rect[1] = Scr_GetParmFloat(1);
	rect[2] = Scr_GetParmFloat(2);
	rect[3] = Scr_GetParmFloat(3);

	Scr_GetParmVector2(4, &color[0], &color[1], &color[2]);
	color[3] = Scr_GetParmFloat(5);

	char* pic = Scr_GetParmString(6);
	re.DrawStretchedImage(rect, color, pic);
}

/*
=================
PFCG_drawfill

drawfill( float x, float y, float w, float h, vector color, float alpha );
=================
*/
static void PFCG_drawfill(void)
{
	float rect[4], color[4];

	rect[0] = Scr_GetParmFloat(0);
	rect[1] = Scr_GetParmFloat(1);
	rect[2] = Scr_GetParmFloat(2);
	rect[3] = Scr_GetParmFloat(3);

	Scr_GetParmVector2(4, &color[0], &color[1], &color[2]);
	color[3] = Scr_GetParmFloat(5);

	re.NewDrawFill(rect, color);
}

void PFCG_GetCursorPos(vec2_t* out)
{
}

void PFCG_SetCursorPos(int x, int y)
{
}

/*
=================
CG_InitScriptBuiltins

Register builtins which can be shared by both client and server progs
=================
*/
void CG_InitScriptBuiltins()
{
	Scr_DefineBuiltin(PFCG_getconfigstring, PF_CL, "getconfigstring", "string(int idx)");
	Scr_DefineBuiltin(PFCG_getstat, PF_CL, "getstat", "float(float idx)");

	Scr_DefineBuiltin(PFCG_MSG_ReadChar, PF_CL, "MSG_ReadChar", "int()");
	Scr_DefineBuiltin(PFCG_MSG_ReadByte, PF_CL, "MSG_ReadByte", "int()");
	Scr_DefineBuiltin(PFCG_MSG_ReadShort, PF_CL, "MSG_ReadShort", "int()");
	Scr_DefineBuiltin(PFCG_MSG_ReadLong, PF_CL, "MSG_ReadLong", "int()");
	Scr_DefineBuiltin(PFCG_MSG_ReadFloat, PF_CL, "MSG_ReadFloat", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadCoord, PF_CL, "MSG_ReadCoord", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadPos, PF_CL, "MSG_ReadPos", "vector()");
	Scr_DefineBuiltin(PFCG_MSG_ReadAngle, PF_CL, "MSG_ReadAngle", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadAngle16, PF_CL, "MSG_ReadAngle16", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadDir, PF_CL, "MSG_ReadDir", "vector()");

	Scr_DefineBuiltin(PFCG_drawstring, PF_CL, "drawstring", "void(vector xya, float fs, vector c, float a, string s1, ...)");
	Scr_DefineBuiltin(PFCG_drawimage, PF_CL, "drawimage", "void(float x, float y, float w, float h, vector c, float a, string img)");
	Scr_DefineBuiltin(PFCG_drawfill, PF_CL, "drawfill", "void(float x, float y, float w, float h, vector c, float a)");
}