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
	vector xy_align;
	
	CG_DrawHUD();
	
	//
	// bossbar
	//
	float width;
	
	width = 296 * cvar("hp");
	
	if(!width)
		return;
	drawfill(250, 100, 300, 18, '0 0 0', 0.7);	
	drawfill(252, 103, width, 12, '1 0 0', 0.5);	
	

	xy_align = '400 104 2';
	drawstring(xy_align, 1.0, '1 1 0', 1, "hulktant");
};