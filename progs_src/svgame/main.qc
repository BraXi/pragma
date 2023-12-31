/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

void() initAnimTrees;

entity tag_attach;
float mdl_xyz, mdl_gun;
float ttime = 0.5;

void() testmodel_think =
{	
	entity player;
	player = getent(1);	
	
	float numframes = getframescount(self.modelindex);
		
//	self.animFrame  ++;
//	if(self.animFrame > numframes)
//		self.animFrame = 0;
	
	self.angles_y += 4;
	self.angles_x += 2;

	vector tag_pos = gettagorigin(self, "tag_flash");
	vector tag_rot = gettagangles(self, "tag_flash");
	tag_attach.origin = tag_pos;
	tag_attach.angles = tag_rot;
	
	print("'tag_flash' in frame ", ftos(self.animFrame)," tag_pos = ", vtos(tag_pos), ", tag_rot = ", vtos(tag_rot), "\n");
	
	self.nextthink = g_time + ttime;
	
	drawstring(self.origin + '0 0 38', '1 1 1', 0.25, false, ttime, "parent angles: ", vtos(self.angles));
	drawstring(self.origin + '0 0 34', '1 1 1', 0.25, false, ttime, ftos(self.animFrame), "/", ftos(numframes) );
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
		tag_attach.renderFlags = RF_DEPTHHACK;
		linkentity(tag_attach);
		
		entity e = spawn();

		setmodel(e, "models/v_ak47.md3");
		e.origin = '0 0 30';
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


