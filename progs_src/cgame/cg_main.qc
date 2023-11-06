/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


void() CG_Main =
{
	CG_InitCrosshair();

};

void() CG_Frame =
{
	cg_nocrosshair = cvar("cg_nocrosshair");
};

void() CG_DrawGUI =
{
	CG_DrawCrosshair();
};