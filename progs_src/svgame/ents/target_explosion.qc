/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

/*QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8) ONCE
Spawns an explosion temporary entity when used

ONCE 		entity will be deleted after first explosion

"delay"		wait this long before going off
"dmg"		how much radius damage should be done, defaults to 0
"radius"	entities in this distance will be damaged
*/

void() target_explosion_explode =
{
	float save;

	FX_Explosion( self.origin, "medium" );
	
	if( self.dmg > 0 && self.radius > 0 )
		RadiusDamage(self, self.activator, self.dmg, world, self.radius, DMG_EXPLOSIVE);

	save = self.delay;
	self.delay = 0;
	SUB_UseTargets (self, self.activator);
	self.delay = save;
	
	if(self.spawnflags & 1)
		remove(self);
};

void(entity who) use_target_explosion =
{
	self.activator = who;

	if (!self.delay)
	{
		target_explosion_explode();
		return;
	}

	self.think = target_explosion_explode;
	self.nextthink = g_time + self.delay;
};

float() SP_target_explosion =
{
	self.use = use_target_explosion;
	self.svflags = SVF_NOCLIENT;
	return true;
};