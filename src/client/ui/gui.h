/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

//
// ui_main.c
//
qboolean UI_IsGuiOpen(char* name);
void UI_OpenGui(char* name);
void UI_CloseGui(char* name);
void UI_CloseAllGuis();

void UI_Main();

void UI_Init();
void UI_Shutdown();

void UI_KeyInputHandler(int key);
void UI_Draw();