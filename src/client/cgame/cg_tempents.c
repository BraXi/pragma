/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"

#if 0
// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like gibs, physics objects, shells, etc.

#define			MAX_LOCAL_ENTITIES	1024
localEntity_t	cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t	cg_activeLocalEntities;		// double linked list
localEntity_t	*cg_freeLocalEntities;		// single linked list

/*
===================
CG_InitLocalEntities

This is called every time cgame is initialized
===================
*/
void CG_InitLocalEntities() 
{
	int		i;

	memset(cg_localEntities, 0, sizeof(cg_localEntities));

	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;

	for (i = 0; i < MAX_LOCAL_ENTITIES - 1; i++) 
	{
		cg_localEntities[i].next = &cg_localEntities[i + 1];
	}
}


/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity(localEntity_t* le) 
{
	if (!le || !le->prev) 
	{
		Com_Error(ERR_DROP, "CG_FreeLocalEntity: not active\n");
		return;
	}

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t* CG_AllocLocalEntity() 
{
	localEntity_t* le;

	if (!cg_freeLocalEntities) 
	{
		// no free entities, so free the one at the end of the chain, remove the oldest active entity
		CG_FreeLocalEntity(cg_activeLocalEntities.prev);
	}

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset(le, 0, sizeof(*le));

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}
#endif