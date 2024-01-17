/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../../pragma_config.h"

#ifdef DEDICATED_ONLY
	#include "../../server/server.h"
	void CG_StubScriptBuiltins();
#else

#include "../client.h"

extern void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);
extern struct sfx_t* CG_FindOrRegisterSound(char* filename);

static void CheckEmptyString(char* s) // definitely need to make it a shared code...
{
	if(s[0] <= ' ')
		Scr_RunError("Bad string");
}

/*
=================
PFCG_AngleVectors

calculate fwd/right/up vectors for given angles and set globals v_forward, v_right and v_up

anglevectors(self.v_angle);
=================
*/
void PFCG_AngleVectors(void)
{
	AngleVectors(Scr_GetParmVector(0), cl.script_globals->v_forward, cl.script_globals->v_right, cl.script_globals->v_up);
}

/*
=================
PFCG_precache_model

Returns true if MD3/SP2 model was loaded, false otherwise.

float precache_model(string filename)
=================
*/
static void PFCG_precache_model(void)
{
	float loaded;
	char* filename = Scr_GetParmString(0);
	CheckEmptyString(filename);

	loaded = (re.RegisterModel(filename) != NULL ? 1.0f : 0.0f);
	Scr_ReturnFloat(loaded);
}

/*
=================
PFSV_precache_sound

Returns true if sound was loaded, false otherwise.
File must be in 'sound/' directory.

float precache_sound(string filename)
=================
*/
static void PFCG_precache_sound(void)
{
	float loaded;
	char* filename = Scr_GetParmString(0);
	CheckEmptyString(filename);

	loaded = (CG_FindOrRegisterSound(filename) != NULL ? 1.0f : 0.0f);
	Scr_ReturnFloat(loaded);
}

/*
=================
PFSV_precache_image

Returns true if image was loaded, false otherwise.
File must be in 'guipics/' directory.

float precache_image(string filename)
=================
*/
static void PFCG_precache_image(void)
{
	float loaded;
	char* filename = Scr_GetParmString(0);
	CheckEmptyString(filename);

	loaded = (re.RegisterPic(filename) != NULL ? 1.0f : 0.0f);
	Scr_ReturnFloat(loaded);
}

/*
=================
PFCG_getconfigstring

returns configstring by index
string bspname = getconfigstring(CS_MODELS+1);
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
PFCG_getclientname

returns the name of player 
string name = getclientname(0);
=================
*/
static void PFCG_getclientname(void)
{
	int idx = Scr_GetParmInt(0);
	if (idx < 0 || idx >= MAX_CLIENTS)
	{
		Scr_RunError("getclientname(): %i out of range\n", idx);
		return;
	}
	Scr_ReturnString(cl.configstrings[CS_CLIENTS+idx]);
}

/*
=================
PFCG_getstat

return client's stat value for given index
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

returns CONTENTS_ mask for given point
int contents = pointcontents(vector point)
=================
*/
extern int CG_PointContents(vec3_t point);
static void PFCG_pointcontents(void)
{
	float* point = Scr_GetParmVector(0);
	Scr_ReturnFloat(CG_PointContents(point));
}

/*
=================
PFSV_trace

Moves the given mins/maxs volume through the world from start to end.
ignoreEntNum is explicitly not checked. contentmask is the collision contents mask

trace(vector start, vector minS, vector maxS, vector end, float ignoreEnt, int contentmask)
=================
*/
extern trace_t CG_Trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int contentsMask, int ignoreEntNum);
static void PFCG_trace(void)
{
	trace_t		trace;
	float* start, * end, * min, * max;
	int ignoreEntNum;
	int			contentmask;

	start = Scr_GetParmVector(0);
	min = Scr_GetParmVector(1);
	max = Scr_GetParmVector(2);
	end = Scr_GetParmVector(3);

	ignoreEntNum = Scr_GetParmFloat(4); //cl.playernum + 1;
	contentmask = Scr_GetParmInt(5);

	trace = CG_Trace(start, min, max, end, contentmask, ignoreEntNum); 

	// set globals in progs
	cl.script_globals->trace_allsolid = trace.allsolid;
	cl.script_globals->trace_startsolid = trace.startsolid;
	cl.script_globals->trace_fraction = trace.fraction;
	cl.script_globals->trace_plane_dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, cl.script_globals->trace_plane_normal);
	VectorCopy(trace.endpos, cl.script_globals->trace_endpos);
//	cl.script_globals->trace_ent = cl.entities;
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
static void PFCG_MSG_ReadChar(void)		{ Scr_ReturnFloat(MSG_ReadChar(&net_message)); }
static void PFCG_MSG_ReadByte(void)		{ Scr_ReturnFloat(MSG_ReadByte(&net_message)); }
static void PFCG_MSG_ReadShort(void)	{ Scr_ReturnFloat(MSG_ReadShort(&net_message)); }
static void PFCG_MSG_ReadLong(void)		{ Scr_ReturnFloat(MSG_ReadLong(&net_message)); }
static void PFCG_MSG_ReadFloat(void)	{ Scr_ReturnFloat(MSG_ReadFloat(&net_message)); }
static void PFCG_MSG_ReadCoord(void)	{ Scr_ReturnFloat(MSG_ReadCoord(&net_message)); }
static void PFCG_MSG_ReadPos(void)		{ vec3_t v; MSG_ReadPos(&net_message, v); Scr_ReturnVector(v); }
static void PFCG_MSG_ReadAngle(void)	{ Scr_ReturnFloat(MSG_ReadAngle(&net_message)); }
static void PFCG_MSG_ReadAngle16(void)	{ Scr_ReturnFloat(MSG_ReadAngle16(&net_message)); }
static void PFCG_MSG_ReadDir(void)		{ /*PFCG_MSG_ReadPos();*/  vec3_t v; MSG_ReadDir(&net_message, v); Scr_ReturnVector(v); }
static void PFCG_MSG_ReadString(void)	{ char *str = MSG_ReadString(&net_message); Scr_ReturnString(str); }
/*
=================
PFCG_drawstring

drawstring( vector xy_align, float fontSize, vector color, float alpha, string text, ... );
=================
*/
static void PFCG_drawstring(void)
{
	if (!CG_CanDrawCall())
	{
//		Scr_RunError("call to 'drawstring' outside of rendering phase\n");
		return;
	}

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

	if (!CG_CanDrawCall())
	{
//		Scr_RunError("call to 'drawimage' outside of rendering phase\n");
		return;
	}

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

	if (!CG_CanDrawCall())
	{
//		Scr_RunError("call to 'drawfill' outside of rendering phase\n");
		return;
	}

	rect[0] = Scr_GetParmFloat(0);
	rect[1] = Scr_GetParmFloat(1);
	rect[2] = Scr_GetParmFloat(2);
	rect[3] = Scr_GetParmFloat(3);

	Scr_GetParmVector2(4, &color[0], &color[1], &color[2]);
	color[3] = Scr_GetParmFloat(5);

	re.NewDrawFill(rect, color);
}

/*
=================
PFCG_localsound

plays 2D sound for local client with no attenuation

void localsound( string filename, float volume );
=================
*/
static void PFCG_localsound(void)
{
	struct sfx_t* sfx;
	char* filename;
	float volume;

	filename = Scr_GetParmString(0);
	volume = Scr_GetParmFloat(1);

	sfx = CG_FindOrRegisterSound(filename);
	if (!sfx)
		return;

	S_StartSound(NULL, cl.playernum + 1, CHAN_2D /*CHAN_AUTO*/, sfx, volume, ATTN_NORM, 0);
}

/*
=================
PFCG_playsound

plays sound for local client

if origin is vec3_origin, the sound will be dynamically 
sourced from the entity to which index was specifiednum

void playsound( vector pos, float entNum, string fileName, float channel, float volume, float attenuation, float timeOffset );

playsound( vec3_origin, localplayernum+1, "player/pain1.wav", CHAN_BODY, 1.0, ATTN_NORM, 0.0 );
=================
*/
static void PFCG_playsound(void)
{
	char* filename;

	float	*pos;
	int		entNum;
	int		chan;
	float	volume;
	float	attenuation;
	float	timeoffset;

	pos = Scr_GetParmVector(0);
	entNum = (int)Scr_GetParmFloat(1);
	filename = Scr_GetParmString(2);
	chan = (int)Scr_GetParmFloat(3);
	volume = Scr_GetParmFloat(4);
	attenuation = Scr_GetParmFloat(5);
	timeoffset = Scr_GetParmFloat(6);

	struct sfx_t* sfx = CG_FindOrRegisterSound(filename);
	if (!sfx)
		return;

	if ((int)pos[0] == 0 && (int)pos[1] == 0 && (int)pos[2] == 0)
		pos = NULL; //hack

	S_StartSound(pos, entNum, chan, sfx, volume, attenuation, timeoffset);
}

/*
=================
PFCG_addcommand

addcommand( string cmdname, void() function )

register console command, all cg commands are removed when dropped from server and during map changes

addcommand( "respawn", cmd_respawn );
=================
*/
static void PFCG_addcommand(void)
{
	char* name = Scr_GetParmString(0);
	Cmd_AddCommandCG(name, Scr_GetParmInt(1));
}

/*
=================
PFCG_getbindkey

string getbindkey( string bind )

returns the key name for bind
=================
*/
static void PFCG_getbindkey(void)
{
	char* binding = Scr_GetParmString(0);
	char* key;

	key = "unbound";
	for (int i = 0; i < 256; i++)
	{
		if (keybindings[i] && !Q_stricmp(keybindings[i], binding))
		{
			key = Key_KeynumToString(i);
			break;
		}
	}
	Scr_ReturnString(key);
}


static void PFCG_GetCursorPos(void)
{
}

static void PFCG_SetCursorPos(void)
{
}
#endif /*not DEDICATED_ONLY*/

/*
=================
CG_InitScriptBuiltins

Register builtins which can be shared by both client and server progs
=================
*/
void CG_InitScriptBuiltins()
{
#ifdef DEDICATED_ONLY
	CG_StubScriptBuiltins();
#else
	// precache
	Scr_DefineBuiltin(PFCG_precache_model, PF_CL, "precache_model", "float(string fn)");
	Scr_DefineBuiltin(PFCG_precache_sound, PF_CL, "precache_sound", "float(string fn)");
	Scr_DefineBuiltin(PFCG_precache_image, PF_CL, "precache_image", "float(string fn)");

	// collision
	Scr_DefineBuiltin(PFCG_pointcontents, PF_CL, "pointcontents", "float(vector v)");
	Scr_DefineBuiltin(PFCG_trace, PF_CL, "trace", "void(vector s, vector bmins, vector bmaxs, vector e, float ie, int cm)");

	// config strings and stats
	Scr_DefineBuiltin(PFCG_getconfigstring, PF_CL, "getconfigstring", "string(int idx)");
	Scr_DefineBuiltin(PFCG_getstat, PF_CL, "getstat", "float(float idx)");
	Scr_DefineBuiltin(PFCG_getclientname, PF_CL, "getclientname", "string(int idx)");

	// message reading
	Scr_DefineBuiltin(PFCG_MSG_ReadChar, PF_CL, "MSG_ReadChar", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadByte, PF_CL, "MSG_ReadByte", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadShort, PF_CL, "MSG_ReadShort", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadLong, PF_CL, "MSG_ReadLong", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadFloat, PF_CL, "MSG_ReadFloat", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadCoord, PF_CL, "MSG_ReadCoord", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadPos, PF_CL, "MSG_ReadPos", "vector()");
	Scr_DefineBuiltin(PFCG_MSG_ReadAngle, PF_CL, "MSG_ReadAngle", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadAngle16, PF_CL, "MSG_ReadAngle16", "float()");
	Scr_DefineBuiltin(PFCG_MSG_ReadDir, PF_CL, "MSG_ReadDir", "vector()");
	Scr_DefineBuiltin(PFCG_MSG_ReadString, PF_CL, "MSG_ReadString", "string()");

	// drawing
	Scr_DefineBuiltin(PFCG_drawstring, PF_CL, "drawstring", "void(vector xya, float fs, vector c, float a, string s1, ...)");
	Scr_DefineBuiltin(PFCG_drawimage, PF_CL, "drawimage", "void(float x, float y, float w, float h, vector c, float a, string img)");
	Scr_DefineBuiltin(PFCG_drawfill, PF_CL, "drawfill", "void(float x, float y, float w, float h, vector c, float a)");

	// sound
	Scr_DefineBuiltin(PFCG_localsound, PF_CL, "localsound", "void(string s, float v)");
	Scr_DefineBuiltin(PFCG_playsound, PF_CL, "playsound", "void(vector v, float en, string snd, float ch, float vol, float att, float tofs)");

	// commands
	Scr_DefineBuiltin(PFCG_addcommand, PF_CL, "addcommand", "void(string cn, void() f)");
	Scr_DefineBuiltin(PFCG_getbindkey, PF_CL, "getbindkey", "string(string bind)");
#endif
}



#ifdef DEDICATED_ONLY
static void PFCG_none(void)
{
	Com_Error(ERR_FATAL, "client game qc builtin invoked in dedicated server!\n");
}

void CG_StubScriptBuiltins()
{
	// precache
	Scr_DefineBuiltin(PFCG_none, PF_CL, "precache_model", "float(string fn)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "precache_sound", "float(string fn)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "precache_image", "float(string fn)");

	// collision
	Scr_DefineBuiltin(PFCG_none, PF_CL, "pointcontents", "float(vector v)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "trace", "void(vector s, vector bmins, vector bmaxs, vector e, float ie, int cm)");

	// config strings and stats
	Scr_DefineBuiltin(PFCG_none, PF_CL, "getconfigstring", "string(int idx)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "getstat", "float(float idx)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "getclientname", "string(int idx)");

	// message reading
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadChar", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadByte", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadShort", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadLong", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadFloat", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadCoord", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadPos", "vector()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadAngle", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadAngle16", "float()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadDir", "vector()");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "MSG_ReadString", "string()");

	// drawing
	Scr_DefineBuiltin(PFCG_none, PF_CL, "drawstring", "void(vector xya, float fs, vector c, float a, string s1, ...)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "drawimage", "void(float x, float y, float w, float h, vector c, float a, string img)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "drawfill", "void(float x, float y, float w, float h, vector c, float a)");

	// sound
	Scr_DefineBuiltin(PFCG_none, PF_CL, "localsound", "void(string s, float v)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "playsound", "void(vector v, float en, string snd, float ch, float vol, float att, float tofs)");

	// commands
	Scr_DefineBuiltin(PFCG_none, PF_CL, "addcommand", "void(string cn, void() f)");
	Scr_DefineBuiltin(PFCG_none, PF_CL, "getbindkey", "string(string bind)");
}
#endif