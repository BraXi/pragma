
void() HUD_Init;

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"	environment map name (env/<sky>_*.tga)
"skyaxis"	axis of sky rotation
"skyrotate"	speed of rotation in degrees/second
"gravity"	800 is default gravity
"message"	text to print at user logon
*/
float() SP_worldspawn =
{
	// never change these three lines or weird shit will happen
	self.movetype = MOVETYPE_PUSH;
	self.solid = SOLID_BSP;
	self.modelindex = 1; // world model MUST always be index 1		


	// make some data visible to the server
	
	// --- LEVEL NAME ---
	if(strlen(self.message) == 0)
	{
		self.message = "unknown map";
		print("WARNING: worldspawn with no 'message' set\n");
	}
	configstring(CS_NAME, self.message);
	
	// --- SKYBOX ---
	if( self.skyrotate != 0 && self.skyaxis == vec3_origin )
	{
		// cannot rotate sky when skyaxis is 0,0,0
		self.skyrotate = 0;
		print("WARNING: sky rotation disabled, wrong worldspawn skyaxis vector\n");
	}
	if(strlen(self.sky) == 0)
	{
		// use default sky
		self.sky = "black";
		print("WARNING: map with no worldspawn sky set, using sky 'none'\n");
	}

	configstring(CS_SKY, self.sky);
	configstring(CS_SKYROTATE, ftos(self.skyrotate) );
	configstring(CS_SKYAXIS, vtos(self.skyaxis) );
	
	// --- make max clients visible ---
	float maxc;	
	maxc = cvar("sv_maxclients");	
	configstring(CS_MAXCLIENTS, ftos(maxc)); //FIXME

	// --- GRAVITY ---
	if(!self.gravity)
		self.gravity = 800;	
	cvarforceset("sv_gravity", ftos(self.gravity)); // force set

	// precache sounds that are used in C physics code
	precache_sound("world/land.wav");
	precache_sound("misc/h2ohit1.wav");

	// initialize g_weapons array
	BG_InitWeapons();
	
	// initialize hud
	HUD_Init();
	
	// create lightstyles
	InitLightStyles();
	
	return true;
};
