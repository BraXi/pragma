/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
		it will then blink in to existance; it will
		kill anything that was in it's way

TOGGLE		only valid for TRIGGER_SPAWN walls
		this allows the wall to be turned on and off

START_ON	only valid for TRIGGER_SPAWN walls
		the wall will initially be present
*/

//func_wall & func_object
float FUNC_SPAWNFLAG_TRIGGER_SPAWN = 1;
float FUNC_SPAWNFLAG_TOGGLE = 2;
float FUNC_SPAWNFLAG_START_ON = 4;
float FUNC_SPAWNFLAG_ANIMATED = 8;
float FUNC_SPAWNFLAG_ANIMATED_FAST = 16;

void(entity acti) func_wall_use =
{
	if (self.solid == SOLID_NOT)
	{
		self.solid = SOLID_BSP;
		self.svflags &= ~SVF_NOCLIENT;
		KillBox();
	}
	else
	{
		self.solid = SOLID_NOT;
		self.svflags |= SVF_NOCLIENT;
	}
	linkentity(self);

	if (!(self.spawnflags & FUNC_SPAWNFLAG_TOGGLE))
		self.use = null_use;
};

float() SP_func_wall =
{
	self.movetype = MOVETYPE_PUSH;
	setmodel(self, self.model);

	if (self.spawnflags & FUNC_SPAWNFLAG_ANIMATED)
		self.effects |= EF_ANIM_ALL;
	if (self.spawnflags & FUNC_SPAWNFLAG_ANIMATED_FAST)
		self.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self.spawnflags & 7) == 0)
	{
		self.solid = SOLID_BSP;
		linkentity(self);
		return true;
	}

	// it must be TRIGGER_SPAWN
	if (!(self.spawnflags & FUNC_SPAWNFLAG_TRIGGER_SPAWN))
	{
		dprint("func_wall at ", vtos(self.origin), " missing TRIGGER_SPAWN\n");
		self.spawnflags |= FUNC_SPAWNFLAG_TRIGGER_SPAWN;
	}

	// yell if the spawnflags are odd
	if (self.spawnflags & FUNC_SPAWNFLAG_START_ON)
	{
		if (!(self.spawnflags & FUNC_SPAWNFLAG_TOGGLE))
		{
			dprint("func_wall START_ON without TOGGLE\n");
			self.spawnflags |= FUNC_SPAWNFLAG_TOGGLE;
		}
	}

	self.use = func_wall_use;
	if (self.spawnflags & FUNC_SPAWNFLAG_START_ON)
	{
		self.solid = SOLID_BSP;
	}
	else
	{
		self.solid = SOLID_NOT;
		self.svflags |= SVF_NOCLIENT;
	}
	linkentity(self);
	return true;
}
