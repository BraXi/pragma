/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

/*QUAKED func_rotating (0 .5 .8) ? X X X X X
rotating brush model
*/

void() func_rotating_think =
{
	self.avelocity = '0 45 0';
	self.think = func_rotating_think;
	self.nextthink = g_time + 1;	
};


float() SP_func_rotating =
{
	setmodel(self, self.model); //setmodel sets bbox sizes for brushmodel
	self.takedamage = DAMAGE_NO;

	if( self.avelocity == vec3_origin )
		self.avelocity = '0 45 0';

	self.solid = SOLID_BSP;	
	self.movetype = MOVETYPE_PUSH;
	self.clipmask = MASK_MONSTERSOLID;
	linkentity(self);
	
	self.think = func_rotating_think;
	self.nextthink = g_time + 1;

	return true;
}