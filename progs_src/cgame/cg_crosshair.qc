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
	gfx_crosshair = "hud/crosshair_1";
	crosshair_size = '28 36 0';
};


void() CG_DrawCrosshair =
{
	float alpha;
	if(cg_nocrosshair)
		return;

	if(getstat(STAT_HEALTH) <= 0)
		return;	//dont draw when dead or spectating

// snipet to scale crosshair
//	crosshair_size = '28 36 0';
//	crosshair_size  = crosshair_size * cvar("cs");
//	float alpha = 2.0 - cvar("cs");

	if(getstat(STAT_AMMO) <= 0)	
		alpha = 0.1;
	else
		alpha = 0.6;
	
	drawimage(400-(crosshair_size_x/2), 300-(crosshair_size_y/2), crosshair_size_x, crosshair_size_y, '1 1 1', alpha, gfx_crosshair);
};

