/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

#define GUI_LIST_IDENT "PRAGMA_PRELOAD_GUIS"

#define GUI_IDENT "PRAGMA_GUI"
#define GUI_VERSION 1

#define MAX_GUIS 64
#define MAX_ITEMS_PER_GUI 256
#define MAX_GUI_ITEMS 4096



typedef struct ui_item_s
{
	qboolean inuse;
	int	gui; // index to ui.guis

	ui_itemvars_t	v; //qc
} ui_item_t;

typedef struct GuiDef_s
{
	char			name[MAX_QPATH];

	qboolean		active;
	int				openTime; // cl.realtime, qsorted

	ui_item_t		*items[MAX_ITEMS_PER_GUI];
	ui_item_t		*items_default[MAX_ITEMS_PER_GUI];

	int				numItems;

	// qc
	scr_func_t		Callback_Active;
	scr_func_t		Callback_OnOpen;
	scr_func_t		Callback_OnClose;
	scr_func_t		Callback_OnKey;
} GuiDef_t;

typedef struct uistate_s
{
	GuiDef_t			*guis[MAX_GUIS];
	int					numGuis;

	int					itemDef_size;
	int					itemDefs_limit;
	int					itemDefs_current;

	ui_globalvars_t		*script_globals;
	ui_item_t			*itemDefs;
} uistate_t;


extern GuiDef_t* current_gui;
extern ui_item_t* current_item;

extern uistate_t ui;

//
// ui_main.c
// 
extern GuiDef_t* UI_FindGui(char* name);
extern ui_item_t* UI_CreateItemDef(GuiDef_t* menu);

//
// ui_load.c
//
extern qboolean UI_LoadGui(char* guiname);
extern void UI_LoadGuisFromFile(char* name);

//
// ui_actions.c
//
void UI_AddAction(const char* cmd_name, xcommand_t function, scr_func_t progfunc);
void UI_RemoveActions();
void UI_ExecuteAction(char* actionstring);
void UI_InitActions();