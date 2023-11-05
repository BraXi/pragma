/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


/*QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8) SAVE_PERS
Changes level to "message" when fired (maps/<message>.bsp)

SAVE_PERS	save inventory and global variables across levels
"message"	map name
*/

void(entity who) target_changelevel_use =
{
	float savepers;
	
	if(g_intermissionTime > 0)
		return;	//activated
// commented out for testing TODO: uncomment later
//	if(cvar_deathmatch)
//		return; //not in multiplayer
	if( !isplayer(who) )
		return;	
	if( who.health <= 0 )
		return;	// dead player in single/coop
	
	if( self.spawnflags & 1 )
		savepers = true;
	else
		savepers = false;
	
	g_intermissionTime = g_time;	
	changemap(self.message, savepers);
};

float() SP_target_changelevel =
{
	if(!strlen(self.message))
	{
		print("target_changelevel with no map ('message' key) at ", vtos(self.origin), "\n");
		return false;
	}
	
	self.use = target_changelevel_use;
//	self.svflags = SVF_NOCLIENT;
	self.loopsound = precache_sound("world/quake.wav");
	linkentity(self);
	return true;
};
