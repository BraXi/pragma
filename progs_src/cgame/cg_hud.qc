/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

//drawfill(float x, float y, float w, float h, vector color, float alpha)
//drawstring(vector xy_align, float fontsize, vector color, float alpha, string text, ...)
// todo add VA() builtin to save on loc :v

/*
	XALIGN_LEFT = 0,
	XALIGN_RIGHT = 1,
	XALIGN_CENTER = 2
*/
	
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
	float health = getstat(STAT_HEALTH);
	
	xy_align = '5 570 0';
	drawstring(xy_align, 1.2, '1 1 1', 1, ftos(health), " HP");
};

void() HUD_DrawCurrentWeaponStats =
{
	vector xy_align;	
	float weapon;

	weapon = getstat(STAT_WEAPON);
	if(weapon <= 0)
		return;

	// name
	xy_align = '795 560 1';	
	drawstring(xy_align, 0.8, '1 1 1', 1, GetWeaponNameForNum(weapon));

	// ammo
	float clip = getstat(STAT_AMMO);
	float reserve = getstat(STAT_AMMO_RESERVE);

	// ammo in magazine
	xy_align = '765 570 1';	
	if(clip > (GetWeaponMaxClipAmmo(weapon)/3))
		drawstring(xy_align, 1.2, '1 1 1', 1, ftos(clip)); 
	else //draw red to indicate low ammo
		drawstring(xy_align, 1.2, '1 0 0', 1, ftos(clip));

	xy_align = '775 570 0';
	drawstring(xy_align, 1.2, '1 1 1', 1, ftos(reserve)); 
	//reserve ammo
//	xy_align = '770 570 0';	
//	drawstring(xy_align, 1.2, '1 1 1', 1, );
	

};


void() HUD_DeadScreen =
{
	vector xy_align;
	
	drawfill(200, 285, 400, 28, '0 0 0', 0.5);	
		
	xy_align = '400 290 2';
	drawstring(xy_align, 2, '1 0.2 0.2', 1, "You are dead");		
		
	xy_align = '400 330 2';
	drawstring(xy_align, 0.8, '1 1 1', 1, "press SPACE to respawn");
};


//
// draw minimalistic hud
//
void() CG_DrawHUD =
{
	// there should be a spectator hud too
	
	if( getstat(STAT_HEALTH) <= 0 )
	{
		HUD_DeadScreen();
		return;
	}
	
	HUD_DrawHealth();
	HUD_DrawCurrentWeaponStats();
			
	HUD_BossBar();
};
