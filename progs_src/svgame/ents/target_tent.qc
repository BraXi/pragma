/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


/*QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)
Fire an origin based temp entity event to the clients.

"style"	type byte (TE_ enum)
*/

void(entity user) target_temp_use =
{
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(self.style);
	MSG_WritePos(self.origin);
	MSG_Multicast(self.origin, MULTICAST_PVS);
};

float() SP_target_temp_entity =
{
	if( !self.style )
		return false;

	if( !strlen(self.targetname) )
		return false;
	
	self.use = target_temp_use;
	return true;
};