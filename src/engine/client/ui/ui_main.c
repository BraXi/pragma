/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

#include "../client.h"
#include "../../script/scriptvm.h"

#include "ui_local.h"

server_entry_t ui_servers[MAX_SERVERS];
int ui_numServers = 0;

static void UI_UpdateScriptGlobals();

uistate_t ui;

GuiDef_t* current_gui;
ui_item_t* current_item;

cvar_t* ui_maxitems;
cvar_t* ui_maxguis;



/*
===============
UI_CreateItemDef
===============
*/
ui_item_t* UI_CreateItemDef(GuiDef_t* gui)
{
	int i;
	ui_item_t* newitem = NULL;

	if (gui->numItems >= MAX_ITEMS_PER_GUI)
	{
		Com_Error(ERR_DROP, "%s: max elements in GuiDef '%s'\n", __FUNCTION__, gui->name);
		return NULL;
	}

	if (ui.itemDefs_current == ui.itemDefs_limit)
	{
		Com_Error(ERR_DROP, "%s: no free itemDefs\n", __FUNCTION__);
		return NULL;
	}

	Scr_BindVM(VM_GUI);
	for (i = 0; i < ui.itemDefs_limit; i++)
	{
		newitem = ENT_FOR_NUM(i);
		if (!newitem->inuse)
			break;
	}

	ui.itemDefs_current++;

	gui->items[gui->numItems] = newitem;
	gui->numItems++;

	//
	// set defaults
	//
	if (newitem != NULL) // msvc...
	{
		memset(&newitem->v, 0, Scr_GetEntityFieldsSize());

		VectorSet(newitem->v.color, 1.0f, 1.0f, 1.0f);
		newitem->v.alpha = 1.0f;

		newitem->inuse = true;

		ui.script_globals->self = ENT_TO_VM(newitem);
		Scr_Execute(VM_GUI, ui.script_globals->Callback_InitItemDef, __FUNCTION__);
	}

	return newitem;
}

/*
===============
UI_RemoveItemDef
===============
*/
void UI_RemoveItemDef(ui_item_t *self)
{
	int i, j;
//	ui_item_t* item = NULL;
	GuiDef_t* gui;

	Scr_BindVM(VM_GUI);

	if (!self || !self->inuse)
		return;

	for (i = 0; i < MAX_GUIS; i++)
	{
		gui = ui.guis[i];
		if (gui == NULL)
			continue;

		for (j = 0; j < MAX_ITEMS_PER_GUI; j++)
		{
			if (!gui->items[j] || self != gui->items[j])
				continue;

			gui->items[j] = NULL;
			gui->numItems--;
			memset(self, 0, Scr_GetEntitySize());
			//memset(&self->v, 0, Scr_GetEntityFieldsSize());

			ui.itemDefs_current--;	
			return;
		}
	}
}

/*
===============
===============
*/
void UI_Error(char* fmt, ...)
{
	va_list			argptr;
	static char		msg[1024];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	Com_Printf("UI Error: %s\n", msg);
}

/*
===============
UI_FindGui
===============
*/
GuiDef_t *UI_FindGui(char* name)
{
	for (int i = 0; i < MAX_GUIS; i++)
	{
		if (!ui.guis[i] || !ui.guis[i]->name[0])
			continue;
		if (!Q_stricmp(name, ui.guis[i]->name))
			return ui.guis[i];
	}
	return NULL;
}


/*
===============
UI_IsGuiOpen
===============
*/
qboolean UI_IsGuiOpen(char* name)
{
	GuiDef_t* gui;

	if((gui = UI_FindGui(name)) == NULL)
		return false;

	if (gui->openTime == -1)
		return false;

	return true;
}

/*
===============
UI_OpenGui
===============
*/
void UI_OpenGui(char* name)
{
	GuiDef_t* gui =  UI_FindGui(name);
	if (gui == NULL)
		return;
	gui->openTime = cls.realtime;

	cls.key_dest = key_menu;
	Com_Printf("opened menu %s\n", name);
}

/*
===============
UI_CloseGui
===============
*/
void UI_CloseGui(char* name)
{
	GuiDef_t* gui = UI_FindGui(name);
	if (gui == NULL)
		return;
	gui->openTime = -1;
}

/*
===============
UI_CloseAllGuis
===============
*/
void UI_CloseAllGuis()
{
	cls.key_dest = key_game;
	for (int i = 0; i < MAX_GUIS; i++)
	{
		if (ui.guis[i])
			ui.guis[i]->openTime = -1;
	}
}

/*
===============
UI_NumOpenedGuis
===============
*/
int UI_NumOpenedGuis()
{
	int open = 0;
	for (int i = 0; i < MAX_GUIS; i++)
	{
		if (ui.guis[i] && ui.guis[i]->openTime != -1)
			open++;
	}
	return open;
}


/*
===============
UI_Draw
===============
*/
void UI_Draw()
{
	static float fadeColor[4] = { 0,0,0,0.4 };
//	if (cls.key_dest != key_menu)
//		return;

	// repaint everything next frame
	SCR_DirtyScreen();

	// dim everything behind it down
	if (cl.cinematictime > 0)
	{
		re.DrawFill(0, 0, viddef.width, viddef.height);
	}
	else
	{
		//if (Com_ServerState() == 0 && cls.state != CS_ACTIVE)
		//	fadeColor[3] = 1;
		//else
		//	fadeColor[3] = 0.3;
		//re.DrawFadeScreen(fadeColor);
	}

	int i, j;
	for (i = 0; i < MAX_GUIS; i++)
	{
		if (ui.guis[i] == NULL)
			continue;

		if (ui.guis[i]->openTime == -1)
			continue;

		Scr_BindVM(VM_GUI);

		current_gui = ui.guis[i];

		ui.script_globals->currentGuiName = Scr_SetString(current_gui->name);

		for (j = 0; j < current_gui->numItems; j++)
		{
			if (current_gui->items[j] == NULL)
				continue;

			UI_UpdateScriptGlobals();
			ui.script_globals->self = ENT_TO_VM(current_gui->items[j]);

			if(current_gui->items[j]->v.drawfunc != 0)
				Scr_Execute(VM_GUI, current_gui->items[j]->v.drawfunc, __FUNCTION__);
			else
				Scr_Execute(VM_GUI, ui.script_globals->Callback_GenericItemDraw, __FUNCTION__);
		}
	}
}

/*
===============
UI_KeyInputHandler
===============
*/
void UI_KeyInputHandler(int key)
{
	key = key;
}

static void Cmd_LoadGui_f(void)
{
	char* name = Cmd_Argv(1);
	if (Cmd_Argc() != 2)
	{
		Com_Printf("loadgui <filename> : load gui file\n");
		return;
	}

	if (strlen(name) == 0)
		return;

	UI_LoadGui(Cmd_Argv(1));
}

static void Cmd_OpenGui_f(void)
{
	char* name = Cmd_Argv(1);
	if (Cmd_Argc() != 2)
	{
		Com_Printf("opengui <guiname> : open gui\n");
		return;
	}
	if (strlen(name) == 0)
		return;
	UI_OpenGui(name);
}

static void Cmd_CloseGui_f(void)
{
	char* name = Cmd_Argv(1);
	if (Cmd_Argc() != 2)
	{
		Com_Printf("closegui <guiname> : close gui\n");
		return;
	}
	if (strlen(name) == 0)
		return;
	UI_CloseGui(name);
}

static void Cmd_CloseAllGuis_f(void)
{
	UI_CloseAllGuis();
}

static void Cmd_ListGuis_f(void)
{
	int i;
	GuiDef_t* gui = ui.guis[0];

	for (i = 0; i < MAX_GUIS; i++, gui++)
	{
		if (gui == NULL)
			continue;
		if (!gui->name[0])
			continue;

		Com_Printf("GuiDef `%s`: %i itemsDefs, %s %s\n", gui->name, gui->numItems, (gui->active == true ? "active" : ""), (gui->openTime != -1 ? "open" : ""));
	}
}


/*
===============
UI_UpdateScriptGlobals
===============
*/
static void UI_UpdateScriptGlobals()
{
	ui.script_globals->frametime = cls.frametime;
	ui.script_globals->time = cl.time;
	ui.script_globals->realtime = cls.realtime;

	ui.script_globals->vid_width = viddef.width;
	ui.script_globals->vid_height = viddef.height;

	ui.script_globals->scr_width = scr_vrect.width;
	ui.script_globals->scr_height = scr_vrect.height;

	ui.script_globals->clientState = cls.state;
	ui.script_globals->serverState = Com_ServerState();

	ui.script_globals->numServers = ui_numServers;
}


/*
===============
UI_Execute
===============
*/
void UI_Execute(scr_func_t progFunc)
{
	// pass most recent data to guivm and execute function
	Scr_BindVM(VM_GUI);
	UI_UpdateScriptGlobals();
	Scr_Execute(VM_GUI, progFunc, __FUNCTION__);
}

/*
===============
===============
*/
void UI_LoadProgs()
{
	ui.itemDefs_limit = (int)ui_maxitems->value;

	Scr_CreateScriptVM(VM_GUI, ui.itemDefs_limit, (sizeof(ui_item_t) - sizeof(ui_itemvars_t)), offsetof(ui_item_t, v));
	Scr_BindVM(VM_GUI);

	ui.itemDef_size = Scr_GetEntitySize();
	ui.itemDefs = ((ui_item_t*)((byte*)Scr_GetEntityPtr()));
	ui.script_globals = Scr_GetGlobals();
}

/*
===============
UI_Init

Called every time engine starts and when connecting or disconnecting from server
===============
*/
void UI_Init()
{
	Com_Printf("------- GUI Initialization -------\n");
	memset(&ui, 0, sizeof(uistate_t));

	ui_maxitems = Cvar_Get("ui_maxitems", "4096", CVAR_NOSET, "For mod authors, increase if you hit max items per GUI limit");
	ui_maxguis = Cvar_Get("ui_maxguis", "32", CVAR_NOSET, "For mod authors, increase if you hit max GUIs limit");

	if (ui_maxitems->value < 1024)
		Cvar_ForceSet("ui_maxitems", "1024");

	if (ui_maxguis->value < 8)
		Cvar_ForceSet("ui_maxguis", "8");

	Cmd_AddCommand("ui_load", Cmd_LoadGui_f);
	Cmd_AddCommand("ui_open", Cmd_OpenGui_f);
	Cmd_AddCommand("ui_close", Cmd_CloseGui_f);
	Cmd_AddCommand("ui_closeall", Cmd_CloseAllGuis_f);
	Cmd_AddCommand("ui_list", Cmd_ListGuis_f);

	UI_InitActions();

	// load gui kernel
	UI_LoadProgs();

	// execute main()
	UI_Execute(ui.script_globals->main);

	// precache guis used by code
	UI_LoadGuisFromFile("preload");

	Com_Printf("GUI system initialied.\n");
}

/*
===============
UI_Shutdown
===============
*/
void UI_Shutdown()
{
	UI_RemoveActions();

	Scr_FreeScriptVM(VM_GUI);
	Z_FreeTags(TAG_GUI);

	Cmd_RemoveCommand("ui_load");
	Cmd_RemoveCommand("ui_open");
	Cmd_RemoveCommand("ui_close");
	Cmd_RemoveCommand("ui_closeall");
	Cmd_RemoveCommand("ui_list");
}
