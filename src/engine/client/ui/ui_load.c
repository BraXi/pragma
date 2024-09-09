
/*
pragma
Copyright (C) 2023-2024 BraXi.

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
extern qboolean Scr_ParseEpair(void* base, ddef_t* key, char* s, int memtag); //scr_main.c

static char* gui_filename;

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
	static char	temp[256];
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

			memset(key, 0, sizeof(key));
			strncpy(key, token, sizeof(key) - 1);

			keyvalpairnum++;
			if (keyvalpairnum == 1)
			{
				if (!Q_stricmp("ITEMDEF", key))
				{
					item = UI_CreateItemDef(current_gui);
					current_item = item;
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

			strncpy(value, token, sizeof(value) - 1);
			strncpy(temp, token, sizeof(temp) - 1);

			Cmd_TokenizeString(temp, false); //for qc argc/argv

			Scr_BindVM(VM_GUI);
			ui.script_globals->self = ENT_TO_VM(current_item);
			Scr_AddString(0, key);
			Scr_AddString(1, token);
			Scr_Execute(VM_GUI, ui.script_globals->Callback_ParseItemKeyVal, __FUNCTION__);

			if (Scr_GetReturnFloat() <= 0)
			{
				scriptVar = Scr_FindEntityField(key);
				if (!scriptVar)
				{
					Com_Printf("Warning: unknown field '%s' in '%s'\n", key, gui_filename);
					continue;
				}

				if (item != NULL && !Scr_ParseEpair((void*)&item->v, scriptVar, value, TAG_GUI))
				{
					UI_ParseError("error parsing key '%s'", key);
				}
			}
		}
	}
	current_item = NULL;
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

	newgui->openTime = -1;

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

/*
===============
UI_ParseGuiList
===============
*/
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
	char	*data = NULL;
	char	filename[MAX_OSPATH];
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