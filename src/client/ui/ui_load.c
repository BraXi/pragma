
/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
// ui_load.c - loads .gui files

#pragma once

#include "../client.h"
#include "../../script/script_internals.h"
#include "../../script/scriptvm.h"

#include "ui_local.h"

extern ddef_t* Scr_FindEntityField(char* name); //scr_main.c
extern qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s); //scr_main.c

static char* gui_filename;

/*
===============
ParseHack_SetRect
===============
*/
static void ParseHack_SetRect(char* value, byte* basePtr)
{
	float vec[4];
	sscanf(value, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
	current_item->v.x = vec[0];
	current_item->v.y = vec[1];
	current_item->v.w = vec[2];
	current_item->v.h = vec[3];
}

/*
===============
ParseHack_SetColor
===============
*/
static void ParseHack_SetColor(char* value, byte* basePtr)
{
	float vec[4];
	sscanf(value, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
	current_item->v.color[0] = vec[0];
	current_item->v.color[1] = vec[1];
	current_item->v.color[2] = vec[2];
	current_item->v.alpha = vec[3];
}

static parsefield_t field_hacks[] =
{
	{"rect", F_HACK, -1, ParseHack_SetRect},
	{"rgba", F_HACK, -1, ParseHack_SetColor}
};


/*
===============
UI_ParseError
===============
*/
static void UI_ParseError(char* fmt, ...)
{
	va_list			argptr;
	static char		msg[1024];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	Com_Error(ERR_DROP, "Error parsing menu '%s': %s\n", gui_filename, msg);
}


/*
===============
UI_ParseGui
===============
*/
void UI_ParseGui(char* data)
{
	char* token;
	char	key[256];
	char	value[256];
	int		keyvalpairnum;
	ddef_t* scriptVar = NULL;
	ui_item_t* item = NULL;

	if (!current_gui)
	{
		UI_ParseError("called %s when current_gui is NULL", __FUNCTION__);
	}

	// check ident
	token = COM_Parse(&data);
	if (!data || Q_stricmp(GUI_IDENT, token))
		UI_ParseError("it is not a menu");

	// check version
	token = COM_Parse(&data);
	if (GUI_VERSION != atoi(token))
		UI_ParseError("wrong version %s, should be %i", token, GUI_VERSION);

	while (1)
	{
		// parse the opening brace	
		token = COM_Parse(&data);
		if (!data)
			break;

		if (token[0] != '{')
			UI_ParseError("found '%s' when expecting '{'", token);

		// go through all the key value pairs inside of { }
		keyvalpairnum = 0;
		while (1)
		{
			// parse key
			token = COM_Parse(&data);
			if (token[0] == '}')
				break;
			if (!data)
				UI_ParseError("EOF without closing brace");

			strncpy(key, token, sizeof(key) - 1);

			keyvalpairnum++;
			if (keyvalpairnum == 1)
			{
				if (!Q_stricmp("ITEMDEF", key))
				{
					item = UI_CreateItemDef(current_gui);
				}
				else
					UI_ParseError("expected type");

				continue;
			}

			// parse value	
			token = COM_Parse(&data);

			if (!data)
				UI_ParseError("EOF without closing brace");
			if (token[0] == '}')
				UI_ParseError("closing brace without data for key '%s'", key);

			current_item = item;

			Scr_BindVM(VM_GUI);
			if (!COM_ParseField(key, value, (void*)&item, field_hacks)) // first check hacks
			{
				scriptVar = Scr_FindEntityField(key);
				if (!scriptVar)
				{
					Com_Printf("Warning: unknown field '%s' in '%s'\n", key, gui_filename);
					continue;
				}

				if (!Scr_ParseEpair((void*)&item->v, scriptVar, token))
				{
					UI_ParseError("error parsing key '%s'", key);
				}
			}
		}
	}
}


/*
===============
UI_LoadFromFime
===============
*/
qboolean UI_LoadFromFime(char* guiname)
{
	int		len;
	char* data = NULL;
	char filename[MAX_OSPATH];


	Com_sprintf(filename, sizeof(filename), "guis/%s.gui", guiname);

	//
	// load file
	//
	len = FS_LoadTextFile(filename, (void**)&data);
	if (!len || len == -1)
	{
		return false;
	}

	//
	// parse file
	//
	gui_filename = filename;
	UI_ParseGui(data);
	FS_FreeFile(data);
	return true;
}


/*
===============
UI_LoadGui
===============
*/
qboolean UI_LoadGui(char* guiname)
{
	GuiDef_t* newgui;

	// check if it was loaded already
	if (UI_FindGui(guiname) != NULL)
		return true;

	// create new gui
	newgui = Z_TagMalloc(sizeof(GuiDef_t), TAG_GUI);
	strcpy(newgui->name, guiname);

	ui.guis[ui.numGuis] = newgui;
	ui.numGuis++;

	// load it
	current_gui = newgui;
	if (!UI_LoadFromFime(guiname))
	{
		Com_Error(ERR_DROP, "...couldn't find gui: '%s'\n", guiname);
	}

	Com_Printf("...loaded gui: '%s'\n", guiname);
	return true;
}


int UI_ParseGuiList(char* data)
{
	char *token;
	char menuname[MAX_QPATH];
	int numGuis = 0;

	// check ident
	token = COM_Parse(&data);
	if (!data || Q_stricmp(GUI_LIST_IDENT, token))
		return 0;

	while (1)
	{
		token = COM_Parse(&data);
		if (!data)
			break;
		
		strncpy(menuname, token, sizeof(menuname) - 1);

		UI_LoadGui(menuname);
		numGuis++;
	}
	return numGuis;
}

/*
===============
UI_LoadGuisFromFile
===============
*/
void UI_LoadGuisFromFile(char* name)
{
	int		len;
	char* data = NULL;
	char filename[MAX_OSPATH];
	int		numGuis;

	Com_sprintf(filename, sizeof(filename), "guis/%s.txt", name);

	len = FS_LoadTextFile(filename, (void**)&data);
	if (!len || len == -1)
	{
		return;
	}

	numGuis = UI_ParseGuiList(data);

	Com_Printf("%i guis referenced in %s\n", numGuis, filename);
	FS_FreeFile(data);


}