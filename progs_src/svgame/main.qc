/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

void() initAnimTrees;

entity tag_attach;
float mdl_xyz, mdl_gun;

void() testmodel_think =
{	
	entity player;
	player = getent(1);	
	float numframes = getframescount(self.modelindex);
	
//	if(g_frameNum & 20)
		self.animFrame  ++;

	if(self.animFrame > numframes)
		self.animFrame = 0;


	vector tag_pos = gettagorigin(self, "tag_flash");
	tag_attach.origin_z = tag_pos_y;
	tag_attach.origin_y = tag_pos_z;
	tag_attach.origin += '0 0 24';
	
	print("tag_pos= ", vtos(tag_pos), "\n");
	
	self.nextthink = g_time + 0.2;
	
	drawstring(self.origin + '0 0 38', '1 1 1', 0.25, false, TIME_100_MS, "server frame: ", ftos(g_frameNum));
	drawstring(self.origin + '0 0 34', '1 1 1', 0.25, false, TIME_100_MS, ftos(self.animFrame), "/", ftos(numframes) );
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
	
	mdl_xyz = precache_model("models/dev/xyz.md3");
	mdl_gun = precache_model("models/v_ak47.md3");
	
	initAnimTrees();
	
#if 1	
	// this is not really a good idea for dedicated server
	if(cvar("dedicated") == 0)
	{
		
		tag_attach = spawn();
		setmodel(tag_attach, "models/dev/xyz.md3");
		linkentity(tag_attach);
		
		entity e = spawn();

		setmodel(e, "models/v_ak47.md3");
		e.origin = '0 0 24';
		linkentity(e);
		e.think = testmodel_think;
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


