/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

//
// ui_main.c
//

qboolean UI_IsGuiOpen(char* name);
void UI_OpenGui(char* name);
void UI_CloseGui(char* name);
void UI_CloseAllGuis();
int UI_NumOpenedGuis();

void UI_Main();

void UI_Init();
void UI_Shutdown();

void UI_KeyInputHandler(int key);
void UI_Draw();


#define MAX_SERVERS 32
typedef struct server_entry_s
{
	qboolean good;

	char	address[32];

	char	name[32];
	char	mod[16];
	char	map[16];

	int		numcl, maxcl;

	int		ping;
}server_entry_t;

extern server_entry_t ui_servers[MAX_SERVERS];
extern int ui_numServers;