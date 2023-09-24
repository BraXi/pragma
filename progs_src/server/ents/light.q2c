/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

void(entity activ) light_use =
{
	if (self.spawnflags & SPAWNFLAG_LIGHTSTART_OFF)
	{
		lightstyle(self.style, "m");
		self.spawnflags &= ~SPAWNFLAG_LIGHTSTART_OFF;
	}
	else
	{
		lightstyle(self.style, "a");
		self.spawnflags |= SPAWNFLAG_LIGHTSTART_OFF;
	}
};


float() SP_light =
{
	if(IsStringEmpty(self.targetname))
		return false;
	
	if (self.style >= 32)
	{
		self.use = light_use;
		if (self.spawnflags & SPAWNFLAG_LIGHTSTART_OFF)
			lightstyle(self.style, "a");
		else
			lightstyle(self.style, "m");
	}
	return true;
};

// ======nn=============================================== 

/*
============
Initlightstyles

Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
============
*/
void() InitLightStyles =
{
	
	// 0 normal
	lightstyle(0, "m");
	
	// 1 FLICKER (first variety)
	lightstyle(1, "mmnmmommommnonmmonqnmmo");
	
	// 2 SLOW STRONG PULSE
	lightstyle(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	
	// 3 CANDLE (first variety)
	lightstyle(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	
	// 4 FAST STROBE
	lightstyle(4, "mamamamamama");
	
	// 5 GENTLE PULSE 1
	lightstyle(5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	
	// 6 FLICKER (second variety)
	lightstyle(6, "nmonqnmomnmomomno");
	
	// 7 CANDLE (second variety)
	lightstyle(7, "mmmaaaabcdefgmmmmaaaammmaamm");
	
	// 8 CANDLE (third variety)
	lightstyle(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	
	// 9 SLOW STROBE (fourth variety)
	lightstyle(9, "aaaaaaaazzzzzzzz");
	
	// 10 FLUORESCENT FLICKER
	lightstyle(10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	lightstyle(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	
	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	lightstyle(63, "a");
};	