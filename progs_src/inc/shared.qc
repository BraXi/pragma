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

#define ENTITYNUM_NONE -1

// angle indexes
#define	PITCH		0		// up / down
#define	YAW			1		// left / right
#define	ROLL		2		// fall over

// ------------------------------------------------------
// BG - defines shared by cgame and svgame
// ------------------------------------------------------

// .waterlevel levels
enum
{
	WL_NOTINWATER = 0, 	// not in water
	WL_TOUCHING, 		// touching water
	
/* players only below */	
	WL_HALFWAY,			// half in water
	WL_UNDERWATER		// head under water
};

//
// stats are used to display HUD
//
// player_state->stats[] indexes
enum
{
/* these MUST match in C code */	
	STAT_HEALTH,
	STAT_AMMO,				// only used in layoutstring
	STAT_SCORE,
	STAT_ARMOR,				// only used in layoutstring
	STAT_LAYOUT,			// 0 don't draw layout programs at all, 1 draw layout, 2 draw inventory and layout
	STAT_FLASHES,			// cleared each frame, 1 = health, 2 = armor, used only in layoutstring programs
	STAT_SELECTED_ITEM,		// TODO: remove! used in inventory
/* add custom stats below */
	STAT_WEAPON,
	STAT_AMMO_RESERVE,
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