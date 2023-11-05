/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

/*QUAKED prop_model (0.5 0.5 0) (-16 -16 -16) (16 16 6) START_HIDDEN
model for decorations

START_HIDDEN - must be triggered to show

"model"		model or sprite to render
"animFrame"		animation frame (default 0)
"solid"		set to 1 to make it collidable (default 0)
"mins"		bbox mins for collision (default '-16 -16 0')
"maxs"		bbox maxs for collision (default '16 16 32')
*/

void(entity activ) prop_model_use =
{
	if (self.spawnflags & SPAWNFLAG_M_MODEL_START_HIDDEN)
	{
		self.svflags = 0;
		self.spawnflags &= ~SPAWNFLAG_M_MODEL_START_HIDDEN;
	}
	else
	{
		self.svflags = SVF_NOCLIENT;
		self.spawnflags |= SPAWNFLAG_M_MODEL_START_HIDDEN;
	}
};


float() SP_prop_model =
{
	if( !strlen(self.model) )
	{
		dprint("prop_model with no model at ", vtos(self.origin), ", removed\n");
		return false;
	}
	
	if (self.spawnflags & SPAWNFLAG_M_MODEL_START_HIDDEN)
	{
		if( !strlen(self.targetname) )
		{
			dprint("prop_model with START_HIDDEN flag at ", vtos(self.origin), " has no targetname, removed\n");
			return false;
		}	
		self.svflags = SVF_NOCLIENT;
	}
	
	precache_model(self.model);
	setmodel(self, self.model);	
		
	if( self.animFrame < 0 )
		self.animFrame = 0;
	
	if( self.solid )
	{
		self.solid = SOLID_BBOX;	
		if( self.mins == vec3_origin || self.maxs == vec3_origin )
		{
			self.mins = '-16 -16 0';
			self.maxs = '16 16 32';
		}
		setsize(self, self.mins, self.maxs);
	}
	
	self.movetype = MOVETYPE_NONE;
	self.use = prop_model_use;	
	linkentity(self);
	return true;
};