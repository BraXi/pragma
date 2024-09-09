/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"

/*
==============
CG_EntityEvent

An entity has just been parsed that has an event value
==============
*/

void CG_EntityEvent(entity_state_t* ent)
{
	switch (ent->event)
	{
	case EV_FOOTSTEP:
		S_StartSound (NULL, ent->number, CHAN_BODY, cgMedia.sfx_footsteps[rand()&3], 1, ATTN_NORM, 0); // TODO: footstep sounds based on surface type (wood, metal, grass, etc)
		break;
	case EV_FALLSHORT:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("player/fall.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		S_StartSound(NULL, ent->number, CHAN_AUTO, S_RegisterSound("player/fall_far.wav"), 1, ATTN_NORM, 0);
		break;
	}
}
