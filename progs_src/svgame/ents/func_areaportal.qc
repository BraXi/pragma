/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/

#define OPEN_ALL_AREAPORTALS 1 // testing

void(entity activator) func_areaportal_use =
{
	self.count ^= 1; // toggle state
	
	if( cvar("developer_script") > 0 )
		dprint("portalstate: ", ftos(self.style)," = ", ftos(self.count), ";\n");

	SetAreaPortalState(self.style, self.count);
};

float() SP_func_areaportal =
{
// testing
#ifdef OPEN_ALL_AREAPORTALS
	SetAreaPortalState(self.style, 1);
	return true;
#else	
	self.use = func_areaportal_use;
	self.count = 0;	// always start closed
	return true;
#endif
};