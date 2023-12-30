/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

void() initAnimTrees;

void() plr_showanim =
{	
	entity player;
	player = getent(1);	
	float numframes = getframescount(self.modelindex);
	
//	if(g_frameNum & 2)
	self.animFrame  ++;

	if(self.animFrame > numframes)
		self.animFrame = 0;
	
	self.angles = player.angles;	
	self.angles = self.angles + '0 90 0';
	self.angles_x = 0;
//	self.loopsound = precache_sound("effects/teleport.wav");
//	self.modelindex2 = precache_model("models/md3_test.md3");
	self.nextthink = g_time + TIME_50_MS;
	
	drawstring(self.origin + '0 0 38', '1 1 1', 0.25, false, TIME_50_MS, "server frame: ", ftos(g_frameNum));
	drawstring(self.origin + '0 0 34', '1 1 1', 0.25, false, TIME_50_MS, ftos(self.animFrame), "/", ftos(numframes) );
};

/*
=================
main

called during server initialization after map has been loaded and
level entities spawned, but right before entities are allowed to think
=================
*/
void() main =
{
	dprint("*** DEVELOPER MODE ENABLED ***\n");

	cvar_maxclients = cvar("sv_maxclients");
	cvar_coop = cvar("coop");
	cvar_deathmatch = cvar("deathmatch");
	cvar_dmflags = cvar("dmflags");
	cvar_skill = cvar("skill");

	developer = cvar("developer");
	developer_script = cvar("developer_script");
	
	initAnimTrees();
#if 1	
	// this is not really a good idea for dedicated server
	if(cvar("dedicated") == 0)
	{
		entity e = spawn();
		//setmodel(e, "models/md3_test.md3");
		setmodel(e, "models/female.md3");
		e.origin = '0 0 24';
//		e.loopsound = precache_sound("effects/teleport.wav");
//		e.avelocity = '0 90 0';
		linkentity(e);
		e.think = plr_showanim;
		e.nextthink = g_time + 1;
	}
#endif
//	InitTriggerVectors(); //calculate movedir for each trigger	
};

/*
=================
StartFrame

Called at the beginning of each server frame before physics
=================
*/
void() StartFrame =
{
//	dprint("StartFrame: ", ftos(g_frameNum), "\n");	
};


/*
=================
EndFrame

run at the end of each server frame
=================
*/
void() EndFrame =
{
};


