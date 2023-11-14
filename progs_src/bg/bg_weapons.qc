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

// --- noweapon ---
	i = WEAPON_NONE;
	g_weapons[i].displayName = "none";
		
// --- automatic rifle ---
	i = WEAPON_AR;
	g_weapons[i].displayName = "AK-47";
	
	g_weapons[i].damage = 10;
	g_weapons[i].dmgmethod = DMG_BULLET;
	
	g_weapons[i].range = 2048; 
	g_weapons[i].spread = 48;
	
	g_weapons[i].clipAmmo = 30; 
	g_weapons[i].maxAmmo = 90;
	
#ifdef SVGAME	
	// only server precaches
	g_weapons[i].viewModel = precache_model("models/v_ak47.md3");
	g_weapons[i].worldModel = 0;
	g_weapons[i].muzzleflash = FX_MUZZLEFLASH_RIFLE;
	
	for( i = 0; i <	NUM_WEAPONS; i++ )
	{
		print("W_InitWeapons: ", ftos(i), " ", g_weapons[i].displayName, "\n");
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
		return 0;	
	if(g_weapons[wpn].maxAmmo > 0)
		return true;
	return false;
};