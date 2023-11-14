// globals shared by both client and server
// you can add new globals here

float FRAMETIME = 0.1;
float FALSE = 0;
float TRUE = 1;
float false = FALSE;
float true = TRUE;

string string_null = "";
vector vec_null = '0 0 0';
vector vec3_origin = vec_null; //temporary, ugly


// ------------------------------------------------------
// BG - defines shared by cgame and svgame
// ------------------------------------------------------

//
// stats are used to display HUD
//
enum
{
/* these MUST match in C code */
	STAT_LAYOUT = 0,
	STAT_HEALTH_VALUE,
	STAT_ARMOR_VALUE,
	STAT_CURRENT_AMMO,
	STAT_FLASHES, // cleared each frame, 1 = health, 2 = armor
/* add custom stats below */
	STAT_HEALTH_IMAGE,
	STAT_ARMOR_IMAGE,	
	STAT_CURRENT_WEAPON,
	STAT_WEAPON_CLIPAMMO,
	STAT_WEAPON_RESERVEAMMO,
	STAT_UNUSED4,
	STAT_UNUSED5,
	STAT_UNUSED6,
	STAT_UNUSED7,
	STAT_UNUSED8,
	STAT_UNUSED9,
	STAT_UNUSED10,
	STAT_UNUSED11,
	STAT_UNUSED12,
	STAT_UNUSED13,
	STAT_UNUSED14,
	STAT_UNUSED15,
	STAT_UNUSED16,
	STAT_UNUSED17,
	STAT_UNUSED18,
	STAT_UNUSED19,
	STAT_UNUSED20,
	STAT_UNUSED21,
	STAT_UNUSED22,
	STAT_UNUSED23,
	STAT_UNUSED24,
	STAT_UNUSED25,	
};


// damage methods
enum 
{
	DMG_UNKNOWN,
	DMG_SLIME,
	DMG_LAVA,
	DMG_FALLING,
	DMG_CRUSH,
	DMG_DROWNING,
	DMG_EXPLOSIVE,
	DMG_BULLET,
	DMG_ENERGY
};

// weapons list
enum
{
	WEAPON_NONE = 0,
	WEAPON_AR,
	NUM_WEAPONS
};

// weapon definitions
typedef struct 
{
	string	displayName;
	
	float	damage;
	float	dmgmethod;
	
	float	range; //trace dist
	float	spread;
	
	float	clipAmmo;
	float	maxAmmo;	
	
	float	viewModel; //modelindex
	float	worldModel; //modelindex
	float	muzzleflash; //effect+fire sound
	
	vector	anim_idle;
	vector	anim_attack;
	vector	anim_drop;
	vector	anim_raise;
	vector	anim_reload;
} WeaponDef;

void() BG_InitWeapons;
string(float wpn) GetWeaponNameForNum;
float(float wpn) GetWeaponMaxClipAmmo;
float(float wpn) GetWeaponMaxAmmo;
float(float wpn) IsWeaponClipOnly;