/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../../pragma_config.h"

#ifdef DEDICATED_ONLY
#include "../../server/server.h"
void UI_StubScriptBuiltins();

static void PFGUI_none(void)
{
}
#else

#include "../client.h"
#include "ui_local.h"

extern void PFCG_precache_image(void);
extern void PFCG_getstat(void);
extern void PFCG_getclientname(void);
extern void PFCG_getconfigstring(void);
extern void PFCG_localsound(void);
extern void PFCG_getbindkey(void);

/*
=================
PFGUI_drawstring

drawstring( vector xy_align, float fontSize, vector color, float alpha, string text, ... );
=================
*/
static void PFGUI_drawstring(void)
{
	float color[4];
	float* xy_align = Scr_GetParmVector(0);
	float fontSize = Scr_GetParmFloat(1);
	Scr_GetParmVector2(2, &color[0], &color[1], &color[2]);
	color[3] = Scr_GetParmFloat(3);
	char* string = Scr_VarString(4);
	re._DrawString(string, xy_align[0], xy_align[1], fontSize, xy_align[2], color);
}

/*
=================
PFGUI_drawimage

drawimage( float x, float y, float w, float h, vector color, float alpha, string imagename );
=================
*/
static void PFGUI_drawimage(void)
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
PFGUI_drawfill

drawfill( float x, float y, float w, float h, vector color, float alpha );
=================
*/
static void PFGUI_drawfill(void)
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

/*
=================
PFGUI_getserverinfo

string getserverinfo(float server, float whatinfo)
=================
*/
#define SI_NAME 0
#define SI_MAP 1
#define SI_MOD 2
#define SI_PING 3
#define SI_ADDRESS 4
#define SI_PLAYERS 5 // "1/23" format
static void PFGUI_getserverinfo(void)
{
	static char		tempStr[8][64];
	static int		numtempstr = 0;
	int				server, info;

	if (numtempstr & 8)
		numtempstr = 0;

	server = (int)Scr_GetParmFloat(0);
	info = (int)Scr_GetParmFloat(1);


	if (server >= ui_numServers || server < 0)
	{
		Scr_ReturnString("none");
		return;
	}

	server_entry_t* si = &ui_servers[server];

	if(!si->good)
	{
		Scr_ReturnString("none");
		return;
	}

	switch (info)
	{
	case SI_NAME:
		Scr_ReturnString(si->name);
		break;
	case SI_MAP:
		Scr_ReturnString(si->map);
		break;
	case SI_MOD:
		Scr_ReturnString(si->mod);
		break;
	case SI_PLAYERS:
		Com_sprintf(tempStr[numtempstr], sizeof(tempStr), "%i/%i", si->numcl, si->maxcl);
		Scr_ReturnString(tempStr[numtempstr]);
		break;
	case SI_PING:
		Com_sprintf(tempStr[numtempstr], sizeof(tempStr), "%i", si->ping);
		Scr_ReturnString(tempStr[numtempstr]);
		break;
	case SI_ADDRESS:
		Scr_ReturnString(si->address);
		break;
	default:
		Scr_ReturnString("none");
		break;

	}
}

static void PFGUI_GetCursorPos(void)
{
}

static void PFGUI_SetCursorPos(void)
{
}
#endif /*not DEDICATED_ONLY*/


/*
=================
UI_InitScriptBuiltins

Register builtins for gui system
=================
*/
void UI_InitScriptBuiltins()
{
#ifdef DEDICATED_ONLY
	UI_StubScriptBuiltins();
#else
	Scr_DefineBuiltin(PFCG_precache_image, PF_GUI, "precache_image", "float(string fn)");
	Scr_DefineBuiltin(PFCG_getconfigstring, PF_GUI, "getconfigstring", "string(int idx)");
	Scr_DefineBuiltin(PFCG_getstat, PF_GUI, "getstat", "float(float idx)");
	Scr_DefineBuiltin(PFCG_getclientname, PF_GUI, "getclientname", "string(int idx)");

	Scr_DefineBuiltin(PFCG_localsound, PF_GUI, "localsound", "void(string s, float v)");
	Scr_DefineBuiltin(PFCG_getbindkey, PF_GUI, "getbindkey", "string(string bind)");

	Scr_DefineBuiltin(PFGUI_GetCursorPos, PF_GUI, "getcursorpos", "vector()");
	Scr_DefineBuiltin(PFGUI_SetCursorPos, PF_GUI, "setcursorpos", "void(float x, float y)");

	Scr_DefineBuiltin(PFGUI_drawstring, PF_GUI, "drawstring", "void(vector xya, float fs, vector c, float a, string s1, ...)");
	Scr_DefineBuiltin(PFGUI_drawimage, PF_GUI, "drawimage", "void(float x, float y, float w, float h, vector c, float a, string img)");
	Scr_DefineBuiltin(PFGUI_drawfill, PF_GUI, "drawfill", "void(float x, float y, float w, float h, vector c, float a)");

	Scr_DefineBuiltin(PFGUI_getserverinfo, PF_GUI, "getserverinfo", "string(float si, float it)");
#endif
}



#ifdef DEDICATED_ONLY
void UI_StubScriptBuiltins()
{
	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "precache_image", "float(string fn)");
	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "getconfigstring", "string(int idx)");
	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "getstat", "float(float idx)");
	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "getclientname", "string(int idx)");

	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "localsound", "void(string s, float v)");

	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "getbindkey", "string(string bind)");

	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "getcursorpos", "vector()");
	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "setcursorpos", "void(float x, float y)");

	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "drawstring", "void(vector xya, float fs, vector c, float a, string s1, ...)");
	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "drawimage", "void(float x, float y, float w, float h, vector c, float a, string img)");
	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "drawfill", "void(float x, float y, float w, float h, vector c, float a)");

	Scr_DefineBuiltin(PFGUI_none, PF_GUI, "getserverinfo", "string(float si, float it)");
}
#endif