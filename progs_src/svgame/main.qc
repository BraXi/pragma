/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

void() initAnimTrees;

entity tag_attach;
float mdl_xyz, mdl_gun;


void( entity ent, float first, float last, float fps, float aflags ) SetAnim =
{
/*	
	anim_firstframe = state->frame & 255;
	anim_lastframe = ( state->frame >> 8 ) & 255;
	anim_rate = (state->frame >> 16) & 255;
	anim_flags = (state->frame >> 24) & 255;		
*/
#if 1
	ent.animtime = sv_time + 0;
	ent.animFrame = first;
	
#else	
	if ( first > 255 )	first = 255;
	if ( last > 255 )	last = 255;
	if ( fps > 255 )	fps = 255;	
	if ( aflags > 255 )	aflags = 255;	
	
	ent.animtime = sv_time;
	ent.animFrame = first | ( last << 8 ) | ( fps << 16 ) | ( aflags << 24 );
#endif
};

float ttime = 0.1;
void() testmodel_think =
{	
	float numframes = getframescount(self.modelindex);

	int cframe = self.animFrame + ((sv_time - self.animtime) / (1000/10));
	if(cframe > 125)
	{
		print("anim reset qc\n");
		SetAnim(self, 62, 125, 5, 0);
	}
	
//	print("acframe = ", ftos(cframe), "\n");
	

	//vector tag_pos = gettagorigin(self, "tag_flash");
	//vector tag_rot = gettagangles(self, "tag_flash");
	//tag_attach.origin = tag_pos;
	//tag_attach.angles = tag_rot;	
//	print("'tag_flash' in frame ", ftos(self.animFrame)," tag_pos = ", vtos(tag_pos), ", tag_rot = ", vtos(tag_rot), "\n");
	
	self.nextthink = g_time + ttime;
	
	
	drawstring(self.origin + '0 0 38', '1 1 1', 0.25, false, ttime, "anim_rate: ", ftos(10));
	drawstring(self.origin + '0 0 34', '1 1 1', 0.25, false, ttime, ftos(cframe), "/", ftos(numframes) );
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

	entity e = spawn();
		
	e.angles_y = 90;
	e.animtime = sv_time + 0;
	e.animFrame = 1;		
	setmodel(e, "models/bc/tank3.md3");
	e.origin = '-32 -160 0';
	e.renderFlags = RF_SCALE;
	e.renderScale = 0.6;
	linkentity(e);
		
#if 0	
	// this is not really a good idea for dedicated server
	if(cvar("dedicated") == 0)
	{
		
		//tag_attach = spawn();
		//setmodel(tag_attach, "models/dev/xyz.md3");
		//tag_attach.renderFlags = RF_DEPTHHACK;
		//linkentity(tag_attach);
		
		entity e = spawn();
		
		e.angles_y = 90;
		e.animtime = sv_time + 0;
		e.animFrame = 1;		
		setmodel(e, "models/mutant.md3");
		e.origin = '0 0 30';
		linkentity(e);
		
		SetAnim(e, 62, 125, 10, 0);
		
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



