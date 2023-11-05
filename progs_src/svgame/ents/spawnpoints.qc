// spawnpoints.qc

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/


float() SP_info_player_start =
{
	return true;
};


float() SP_info_player_intermission =
{
	return true;
};

float() SP_info_player_coop =
{
	return cvar("coop");

};


float() SP_info_player_deathmatch =
{
	return cvar("deathmatch");
};

// ===================================================== 

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/

entity(string spawnpointname) SelectSpawnPoint = 
{
	entity ent;

	ent = world;
	for( ent = nextent(world); ent != world; ent = nextent(ent) )
	{
		float num = getentnum(ent);
		if( ent.classname == spawnpointname )
		{
			break;
		}
	}
	
	if( ent == world )
	{
		print( spawnpointname, " not found :c\n");
	}
	
	return ent;
}