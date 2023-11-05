/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
removed in game
*/
float() SP_func_group =
{
	return false; // remove entity straight away
};