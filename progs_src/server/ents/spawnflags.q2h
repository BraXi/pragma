/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


//
// spawnflags
//

float SPAWNFLAG_NOT_EASY			= 256;
float SPAWNFLAG_NOT_MEDIUM			= 512;
float SPAWNFLAG_NOT_HARD			= 1024;
float SPAWNFLAG_NOT_DEATHMATCH		= 2048;
float SPAWNFLAG_NOT_COOP			= 4096;
float SPAWNFLAG_NOT_SINGLEPLAYER	= 8192;


//func_wall & func_object
float FUNC_SPAWNFLAG_TRIGGER_SPAWN 	= 1;
float FUNC_SPAWNFLAG_TOGGLE 		= 2;
float FUNC_SPAWNFLAG_START_ON 		= 4;
float FUNC_SPAWNFLAG_ANIMATED 		= 8;
float FUNC_SPAWNFLAG_ANIMATED_FAST	= 16;

// light
float SPAWNFLAG_LIGHTSTART_OFF 		= 1;

// misc_model
float SPAWNFLAG_M_MODEL_START_HIDDEN = 1;

// target_speaker
float SF_SPEAKER_LOOPED_ON 			= 1;
float SF_SPEAKER_LOOPED_OFF 		= 2;
float SF_SPEAKER_RELIABLE 			= 4;


/*
============
CheckSpawnFlags

used to discard entities from various gametypes and skill levels, etc.
============
*/
float() CheckSpawnFlags = 
{
	if((cvar_deathmatch > 0) && (self.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
		return false; // not in deathmatch

	if( (cvar_coop > 0) && (self.spawnflags & SPAWNFLAG_NOT_COOP) )
		return false; // not in coop
	
	if( cvar_coop == 0 && cvar_deathmatch == 0 && (self.spawnflags & SPAWNFLAG_NOT_SINGLEPLAYER) )
		return false; // not in singleplayer
	
	if( cvar_skill == 0 && (self.spawnflags & SPAWNFLAG_NOT_EASY) )
		return false; // not on EASY
	if( cvar_skill == 1 && (self.spawnflags & SPAWNFLAG_NOT_MEDIUM) )
		return false; // not on MEDIUM
	if( cvar_skill == 2 && (self.spawnflags & SPAWNFLAG_NOT_HARD) )
		return false; // not on HARD	
	
	return true;
};