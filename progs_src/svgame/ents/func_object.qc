/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN X X ANIMATED ANIMATED_FAST
This is solid brush model that will fall if it's support is removed.

"dmg" - damage to apply on crushed entities (default 100)
*/

void(float planeDist, vector planeNormal, float surfaceFlags) func_object_touch =
{
	// only squash thing we fall on top of
	if (planeNormal == vec3_origin)
		return;
	
	if (planeNormal_z < 1.0)
		return;

	if (other.takedamage == DAMAGE_NO)
		return;
	
	DoDamage(self.activator, self, other, self.dmg, 0, DMG_CRUSH);
};

void() func_object_release =
{
	self.movetype = MOVETYPE_TOSS;
	self.touch = func_object_touch;
};

void(entity user) func_object_use =
{
	self.solid = SOLID_BSP;
	self.svflags &= ~SVF_NOCLIENT;
	self.use = null_use;
	KillBox();
	func_object_release();
};

float() SP_func_object =
{
	if(!strlen(self.model))
	{
		dprint("func_object with no brushmodel removed\n");
		return false;
	}
	
	setmodel(self, self.model); 

	self.mins = self.mins + '1 1 1';
	self.maxs = self.maxs - '1 1 1';

	if( !self.dmg )
		self.dmg = 100;
	
	if( self.spawnflags == 0 )
	{
		self.solid = SOLID_BSP;
		self.movetype = MOVETYPE_PUSH;
		self.think = func_object_release;
		self.nextthink = g_time + (g_frameTime * 2);
	}
	else
	{
		self.solid = SOLID_NOT;
		self.movetype = MOVETYPE_PUSH;
		self.use = func_object_use;
		self.svflags |= SVF_NOCLIENT;
	}
	
	//a case where we want object to be solid from start, but fall when triggered
	if( self.think == func_object_release && strlen(self.targetname) > 0 )
	{
		self.think = SUB_Null; // so the object doesn't fall on its own
		self.use = func_object_use;
	}
	
	if(self.spawnflags & FUNC_SPAWNFLAG_ANIMATED)
		self.effects |= EF_ANIM_ALL;
	if(self.spawnflags & FUNC_SPAWNFLAG_ANIMATED_FAST)
		self.effects |= EF_ANIM_ALLFAST;
	
	self.clipmask = MASK_MONSTERSOLID; 

	linkentity(self);
	return true;
}
