/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

//drawfill(float x, float y, float w, float h, vector color, float alpha)
//drawstring(vector xy_align, float fontsize, vector color, float alpha, string text, ...)

void() HUD_BossBar =
{
	vector xy_align;
	float width = 296 * cvar("bosshp");
	
	if(!width)
		return;
	
	drawfill(250, 100, 300, 18, '0 0 0', 0.7);	
	drawfill(252, 103, width, 12, '1 0 0', 0.5);	
	
	xy_align = '400 104 2';
	drawstring(xy_align, 1.0, '1 1 0', 1, cvarstring("bossname"));
};


void() HUD_DrawHealth =
{
	vector xy_align;	
	float health = getstat(STAT_HEALTH_VALUE);
	
	xy_align = '5 570 0';
	drawstring(xy_align, 1.2, '1 1 1', 1, ftos(health), " HP");
};

void() HUD_DrawCurrentWeaponStats =
{
	vector xy_align;	
	float weapon = getstat(STAT_CURRENT_WEAPON);
	
//	if(!weapon)
//		return;
	 
	// current weapon
	xy_align = '795 570 1';
	drawstring(xy_align, 1.2, '1 1 1', 1, "30/90");
		
	xy_align = '795 560 1';
	drawstring(xy_align, 0.8, '1 1 1', 1, "AK-47");
};


void() HUD_DeadScreen =
{
	vector xy_align;
	
	drawfill(200, 285, 400, 28, '0 0 0', 0.5);	
		
	xy_align = '400 290 2';
	drawstring(xy_align, 2, '1 1 1', 1, "You are dead");		
		
	xy_align = '400 330 2';
	drawstring(xy_align, 0.8, '1 1 1', 1, "type respawn in console");
};


//
// draw minimalistic hud
//
void() CG_DrawHUD =
{
	// there should be a spectator hud too
	
	if( getstat(STAT_HEALTH_VALUE) <= 0 )
	{
		HUD_DeadScreen();
		return;
	}
	

	HUD_DrawHealth();
	HUD_DrawCurrentWeaponStats();
	
	// crosshair
	CG_DrawCrosshair();		
	
	HUD_BossBar();
};
