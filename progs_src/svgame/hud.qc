/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// hud.q2c

//float hud_pic_health;
//float hud_pic_armor;

/*	
	"xl <val>" - X pos [0+x] from left to right
	"xr <val>" - X pos from right [width-val]
	"xv <val>" - X middle [width/2+val]
	"yt <val>" - Y pos [0+val] from top to bottom
	"yb <val>" - Y pos from right [height-val]
	
	"yv <val>" - Y pos middle [height/2+x]

	"if <statnum> ... endif" -  if X > 0 ..ops.. endif

	"pic <image index>"	 - draw image (does render it at pic's size)
	"img <width> <height> <image index>" - draw image strecthed to W/H

	"str <align_x> <string>" - print string
	"cstr <align_x> <statval that holds configstring index>" - print config string
*/


string hud_layoutstring_empty = "";
string hud_layoutstring_mp = "";

string hud_layoutstring_sp =
"yb	-35 "

// health
"if 1 " //STAT_HEALTH_VALUE
"xl 10 "
"pic 5 "
"xl 40 "
"hnum "
"endif "
;

void() HUD_SetStats =
{
	// update hud
	
	setstat(self, STAT_HEALTH, self.health);
//	setstat(self, STAT_ARMOR, self.armor);
	
	// weapon
	setstat(self, STAT_WEAPON, self.weapon);
	setstat(self, STAT_AMMO, self.clipammo);
	setstat(self, STAT_AMMO_RESERVE, self.reserveammo);	
};



void() HUD_SetInitialStats =
{
// UNUSED	
//	setstat(self, STAT_HEALTH_IMAGE, hud_pic_health);
//	setstat(self, STAT_ARMOR_IMAGE, hud_pic_armor);
};

/*
==============
HUD_SetLayout

Set hud layout string for everyone or for a single player
==============
*/
void(entity who, string hudprogram) HUD_SetLayout =
{
	if(isplayer(who))
	{
		MSG_WriteByte(SVC_LAYOUT);
		MSG_WriteString(hudprogram);
		MSG_Unicast(who, true); // send reliable message
	}
	else
	{
		// send to everyone
		configstring(CS_STATUSBAR, hudprogram);	
	}		
};


/*
==============
HUD_Init

Precache hud images and set default HUD program
==============
*/
void() HUD_Init =
{
	// precache default images 
//	hud_pic_health = precache_image("i_health");
//	hud_pic_armor = precache_image("i_powershield");

//	hud_layoutstring_mp = hud_layoutstring_sp;
	
	// set default HUD based on gametype
	if(cvar("deathmatch"))
		HUD_SetLayout( world, hud_layoutstring_mp );
	else
		HUD_SetLayout( world, hud_layoutstring_empty );
};