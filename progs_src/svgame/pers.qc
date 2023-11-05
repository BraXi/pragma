/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// pers.qc - persistant info across level changes

/*
MAX_PERS_FIELDS = 64 // for each client and globals

//client fields
void saveclientfield(entity player, float index, float val)
float loadclientfield(entity player, float index)

// the better 'serverflags'
void saveglobal(float index, float val)
float loadglobal(float index)


float changemap(string nextmap, float saveGlobals, float saveClients) // returns 0 if no map found so we can try unstuck server
*/

const float PERS_HEALTH = 0;
const float PERS_MAX_HEALTH = 1;

void(entity player) SaveClientPersData = 
{
	if(isplayer(player) == 0)
		return;	
	
	// give some health if dead or near death
	if(player.health < 10)
		player.health = 25;
	
	// cap max health
	if(player.max_health > 100)
		player.max_health = 100;	
	
	saveclientfield(player, PERS_HEALTH, player.health);
	saveclientfield(player, PERS_MAX_HEALTH, player.max_health);
};


void() RestoreClientPers = 
{
	if( loadclientfield(self, PERS_HEALTH) )
	{
		// no health means we probably connected for the first time
		return;
	}
		
	self.health = loadclientfield(self, PERS_HEALTH);
	self.max_health = loadclientfield(self, PERS_MAX_HEALTH);
};