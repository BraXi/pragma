/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


string gfx_crosshair;
float cg_nocrosshair;
vector crosshair_size;

void() CG_InitCrosshair =
{
	gfx_crosshair = "hud/crosshair_1"; //registerimage("hud/crosshair_1");
	crosshair_size = '32 36 0';
};


void() CG_DrawCrosshair =
{
	if (cg_nocrosshair)
		return;

//	if (getstat(STAT_CROSSHAIR) == 0)
//		return;	

	drawimage(400-(crosshair_size_x/2), 300-(crosshair_size_y/2), crosshair_size_x, crosshair_size_y, '1 1 1', 1.0, gfx_crosshair);
};

