/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

WeaponDef g_weapons[NUM_WEAPONS];

void() BG_InitWeapons =
{
	float i;
	
	//
	// --- noweapon ---
	//
	g_weapons[WEAPON_NONE].displayName = "none";
	
	
	//
	// AUTOMATIC RIFLE
	//
	g_weapons[WEAPON_AR].displayName = "AK-47";
	
	g_weapons[WEAPON_AR].damage = 15;
	g_weapons[WEAPON_AR].dmgmethod = DMG_BULLET;
	
	g_weapons[WEAPON_AR].range = 2048; 
	g_weapons[WEAPON_AR].spread = 38;
	
	g_weapons[WEAPON_AR].clipAmmo = 30; 
	g_weapons[WEAPON_AR].maxAmmo = 120;
	
	g_weapons[WEAPON_AR].fireTime = 0.19;
	g_weapons[WEAPON_AR].reloadTime = 1.79;
	
	g_weapons[WEAPON_AR].vkick = 3;
	
	g_weapons[WEAPON_AR].viewModel = "models/v_ak47.md3";
	g_weapons[WEAPON_AR].worldModel = "";
	
	g_weapons[WEAPON_AR].muzzleflash = FX_MUZZLEFLASH_RIFLE;
	g_weapons[WEAPON_AR].sfx_reload = "weapons/ak47/reload.wav";
		
	g_weapons[WEAPON_AR].anim_idle = '0 0 1';
	g_weapons[WEAPON_AR].anim_fire = '2 8 2';
	g_weapons[WEAPON_AR].anim_drop = '0 0 1';
	g_weapons[WEAPON_AR].anim_raise = '15 67 2';
	g_weapons[WEAPON_AR].anim_reload = '15 67 2';	
	
	
	//
	// PISTOL
	//
	g_weapons[WEAPON_PISTOL].displayName = "Desert Eagle";
	
	g_weapons[WEAPON_PISTOL].damage = 25;
	g_weapons[WEAPON_PISTOL].dmgmethod = DMG_BULLET;
	
	g_weapons[WEAPON_PISTOL].range = 1024; 
	g_weapons[WEAPON_PISTOL].spread = 56;
	
	g_weapons[WEAPON_PISTOL].clipAmmo = 8;
	g_weapons[WEAPON_PISTOL].maxAmmo = 32;
	
	g_weapons[WEAPON_PISTOL].fireTime = 0.39;
	g_weapons[WEAPON_PISTOL].reloadTime = 2.09;

	g_weapons[WEAPON_PISTOL].vkick = 2;
	
	g_weapons[WEAPON_PISTOL].viewModel = "models/v_deagle.md3";
	g_weapons[WEAPON_PISTOL].worldModel = "";
	
	g_weapons[WEAPON_PISTOL].muzzleflash = FX_MUZZLEFLASH_PISTOL;	
	g_weapons[WEAPON_PISTOL].sfx_reload = "weapons/deagle/reload.wav";
	
	g_weapons[WEAPON_PISTOL].anim_idle = '0 0 1';
	g_weapons[WEAPON_PISTOL].anim_fire = '1 21 3';
	g_weapons[WEAPON_PISTOL].anim_drop = '0 0 1';
	g_weapons[WEAPON_PISTOL].anim_raise = '0 0 1';
	g_weapons[WEAPON_PISTOL].anim_reload = '22 103 3';
	

#ifdef SVGAME
	// only server precaches
	for( i = 1; i <	NUM_WEAPONS; i++ )
	{
		print("W_InitWeapons: ", ftos(i), " ", g_weapons[i].displayName, "\n");
		
		if(strlen(g_weapons[i].viewModel) > 1)
			precache_model(g_weapons[i].viewModel);
		
		if(strlen(g_weapons[i].worldModel) > 1)
			precache_model(g_weapons[i].worldModel);
		
		if(strlen(g_weapons[i].sfx_reload) > 1)
			precache_sound(g_weapons[i].sfx_reload);		
	}
	print("W_InitWeapons: ", ftos(NUM_WEAPONS), " weapons total\n");
#endif	
};


string(float wpn) GetWeaponNameForNum =
{
	if(wpn < 0 || wpn >= NUM_WEAPONS)
		return "undefined";
	return g_weapons[wpn].displayName;
};

float(float wpn) GetWeaponMaxClipAmmo =
{
	if(wpn < 0 || wpn >= NUM_WEAPONS)
		return 0;
	return g_weapons[wpn].clipAmmo;
};

float(float wpn) GetWeaponMaxAmmo =
{
	if(wpn < 0 || wpn >= NUM_WEAPONS)
		return 0;
	return g_weapons[wpn].maxAmmo;
};

float(float wpn) IsWeaponClipOnly =
{
	if(wpn < 0 || wpn >= NUM_WEAPONS)
		return true;
	
	if(g_weapons[wpn].maxAmmo > 0)
		return false;
	
	return false;
};