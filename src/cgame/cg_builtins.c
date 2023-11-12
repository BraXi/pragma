#include "../client/client.h"


extern void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);

/*
=================
PFCG_AngleVectors
=================
*/
void PFCG_AngleVectors(void)
{
	AngleVectors(Scr_GetParmVector(0), cl.script_globals->v_forward, cl.script_globals->v_right, cl.script_globals->v_up);
}

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
static void PFCG_getstat(void)
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

/*
=================
PFCG_pointcontents

int contents = pointcontents(vector point)
=================
*/
extern int CG_PointContents(vec3_t point);
void PFCG_pointcontents(void)
{
	float* point = Scr_GetParmVector(0);
	Scr_ReturnInt(CG_PointContents(point));
}

/*
=================
PFSV_trace

Moves the given mins/maxs volume through the world from start to end.
ignoreEntNum is explicitly not checked.

trace(vector start, vector minS, vector maxS, vector end, float ignoreEnt, int contentmask)
=================
*/
extern trace_t CG_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum);
void PFCG_trace(void)
{
	trace_t		trace;
	float* start, * end, * min, * max;
	int ignoreEntNum;
	int			contentmask;

	start = Scr_GetParmVector(0);
	min = Scr_GetParmVector(1);
	max = Scr_GetParmVector(2);
	end = Scr_GetParmVector(3);

	ignoreEntNum = Scr_GetParmInt(4);
	contentmask = Scr_GetParmInt(5);

	trace = CG_Trace(start, min, max, end, contentmask, ignoreEntNum); 
			//SV_Trace(start, min, max, end, ignoreEnt, contentmask);

	// set globals in progs
	cl.script_globals->trace_allsolid = trace.allsolid;
	cl.script_globals->trace_startsolid = trace.startsolid;
	cl.script_globals->trace_fraction = trace.fraction;
	cl.script_globals->trace_plane_dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, cl.script_globals->trace_plane_normal);
	VectorCopy(trace.endpos, cl.script_globals->trace_endpos);
//	sv.script_globals->trace_ent = (trace.ent == NULL ? GENT_TO_PROG(sv.edicts) : GENT_TO_PROG(trace.ent));
	cl.script_globals->trace_entnum = trace.entitynum;
	cl.script_globals->trace_contents = trace.contents;

	if (trace.surface)
	{
		cl.script_globals->trace_surface_name = Scr_SetString(trace.surface->name);
		cl.script_globals->trace_surface_flags = trace.surface->flags;
		cl.script_globals->trace_surface_value = trace.surface->value;
	}
	else
	{
		cl.script_globals->trace_surface_name = Scr_SetString("");
		cl.script_globals->trace_surface_flags = 0;
		cl.script_globals->trace_surface_value = 0;
	}
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
	Scr_DefineBuiltin(PFCG_pointcontents, PF_CL, "pointcontents", "int(vector v)");
	Scr_DefineBuiltin(PFCG_trace, PF_CL, "trace", "void(vector s, vector bmins, vector bmaxs, vector e, float ie, int cm)");

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