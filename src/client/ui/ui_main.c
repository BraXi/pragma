#pragma once

#include "../client.h"
#include "../../script/scriptvm.h"

#include "ui_local.h"



uistate_t ui;

GuiDef_t* current_gui;
ui_item_t* current_item;

cvar_t* ui_enable;
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
		Com_Error(ERR_DROP, "%s: max elements in GuiDef\n", __FUNCTION__);
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
	ui_item_t* item = NULL;
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
UI_Draw
===============
*/
void UI_Draw()
{
	static float fadeColor[4] = { 0,0,0,0.4 };
	if (cls.key_dest != key_menu)
		return;

	// repaint everything next frame
	SCR_DirtyScreen();

	// dim everything behind it down
	if (cl.cinematictime > 0)
		re.DrawFill(0, 0, viddef.width, viddef.height);
	else
	{
		if (Com_ServerState() == 0)
			fadeColor[3] = 1;
		else
			fadeColor[3] = 0.3;

		re.DrawFadeScreen(fadeColor);
	}
}

/*
===============
UI_KeyInputHandler
===============
*/
void UI_KeyInputHandler(int key)
{
}

void Cmd_LoadGui_f(void)
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

void Cmd_OpenGui_f(void)
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

void Cmd_CloseGui_f(void)
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

void Cmd_CloseAllGuis_f(void)
{
	UI_CloseAllGuis();
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

	ui.script_globals->clientState = cls.state;
}


/*
===============
UI_Main
===============
*/
void UI_Main()
{
	if (1)
		return;

	UI_UpdateScriptGlobals();

	Scr_BindVM(VM_GUI);
	Scr_Execute(VM_GUI, ui.script_globals->main, __FUNCTION__);
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

Called every time client starts and when connecting or disconnecting from server
===============
*/
void UI_Init()
{
	if (1)
		return;

	ui_enable = Cvar_Get("ui_enable", "1", CVAR_NOSET);
	ui_maxitems = Cvar_Get("ui_maxitems", "4096", CVAR_NOSET);
	ui_maxguis = Cvar_Get("ui_maxguis", "32", CVAR_NOSET);

	UI_LoadProgs();
	UI_UpdateScriptGlobals();

	UI_LoadGuisFromFile("preload");

	// precache code guis
//	UI_LoadGui("connect");
//	UI_LoadGui("loadmap");
//	UI_LoadGui("main");

	UI_CloseAllGuis();

	Cmd_AddCommand("ui_load", Cmd_LoadGui_f);
	Cmd_AddCommand("ui_open", Cmd_OpenGui_f);
	Cmd_AddCommand("ui_close", Cmd_CloseGui_f);
	Cmd_AddCommand("ui_closeall", Cmd_CloseAllGuis_f);
	Cmd_AddCommand("ui_list", Cmd_LoadGui_f);
}

/*
===============
===============
*/
void UI_Shutdown()
{
	Scr_FreeScriptVM(VM_GUI);
	Z_FreeTags(TAG_GUI);

	Cmd_RemoveCommand("loadgui");
	Cmd_RemoveCommand("ui_load");
	Cmd_RemoveCommand("ui_open");
	Cmd_RemoveCommand("ui_close");
	Cmd_RemoveCommand("ui_closeall");
	Cmd_RemoveCommand("ui_list");
}


