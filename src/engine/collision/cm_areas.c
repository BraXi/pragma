/*
===========================================================================
PRAGMA Copyright (C) 2023-2024 BraXi.

This file is part of PRAGMA source code, a free open source game engine.

Quake II and Quake III Arena source code are Copyright (C) Id Software, Inc.

See the attached GNU General Public License v2 for more details.

Note: CModel implementation is mostly a direct copy of the one found in Quake III.
===========================================================================
*/

// cm_areas.c - AREA PORTALS

#include "cm_local.h"

/*
====================
CM_FloodAreaConnections
====================
*/
static void CM_FloodArea_r(int areaNum, int floodnum)
{
	int i;
	cArea_t* area;
	int* con;

	area = &cm.areas[areaNum];

	if (area->floodvalid == cm.floodvalid)
	{
		if (area->floodnum == floodnum)
			return;

		Com_Error(ERR_DROP, __FUNCTION__": reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = cm.floodvalid;
	con = cm.areaPortals + areaNum * cm.numAreas;

	for (i = 0; i < cm.numAreas; i++)
	{
		if (con[i] > 0)
		{
			CM_FloodArea_r(i, floodnum);
		}
	}
}

/*
====================
CM_FloodAreaConnections
====================
*/
void CM_FloodAreaConnections()
{
	cArea_t* area;
	int i, floodnum;

	// all current floods are now invalid
	cm.floodvalid++;
	floodnum = 0;

	for (i = 0; i < cm.numAreas; i++)
	{
		area = &cm.areas[i];
		if (area->floodvalid == cm.floodvalid)
		{
			continue; // already flooded into
		}
		floodnum++;
		CM_FloodArea_r(i, floodnum);
	}

}

/*
====================
CM_AdjustAreaPortalState
====================
*/
void CM_AdjustAreaPortalState(int area1, int area2, qboolean open)
{
	if (area1 < 0 || area2 < 0)
	{
		return;
	}

	if (area1 >= cm.numAreas)
	{
		Com_Error(ERR_DROP, __FUNCTION__": bad area1 number");
	}

	if (area2 >= cm.numAreas)
	{
		Com_Error(ERR_DROP, __FUNCTION__": bad area2 number");
	}

	if (open)
	{
		cm.areaPortals[area1 * cm.numAreas + area2]++;
		cm.areaPortals[area2 * cm.numAreas + area1]++;
	}
	else
	{
		cm.areaPortals[area1 * cm.numAreas + area2]--;
		cm.areaPortals[area2 * cm.numAreas + area1]--;

		if (cm.areaPortals[area2 * cm.numAreas + area1] < 0)
		{
			Com_Error(ERR_DROP, __FUNCTION__": negative reference count");
		}
	}

	CM_FloodAreaConnections();
}

/*
====================
CM_AreasConnected
====================
*/
qboolean CM_AreasConnected(int area1, int area2)
{
	if (cm_noAreas->value >= 1.0f)
	{
		return true;
	}

	if (area1 < 0 || area2 < 0)
	{
		return false;
	}

	if (area1 >= cm.numAreas || area2 >= cm.numAreas)
	{
		Com_Error(ERR_DROP, "area >= cm.numAreas");
	}

	if (cm.areas[area1].floodnum == cm.areas[area2].floodnum)
	{
		return true;
	}

	return false;
}


/*
=================
CM_WriteAreaBits

Writes a bit vector of all the areas
that are in the same flood as the area parameter
Returns the number of bytes needed to hold all the bits.

The bits are OR'd in, so you can CM_WriteAreaBits from multiple
viewpoints and get the union of all visible areas.

This is used to cull non-visible entities from snapshots
=================
*/
int CM_WriteAreaBits(byte* buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (cm.numAreas + 7) >> 3;

	if (cm_noAreas->value >= 1.0f || area == -1)
	{
		// for debugging, send everything
		memset(buffer, 255, bytes);
	}
	else
	{
		floodnum = cm.areas[area].floodnum;
		for (i = 0; i < cm.numAreas; i++)
		{
			if (cm.areas[i].floodnum == floodnum || area == -1)
				buffer[i >> 3] |= 1 << (i & 7);
		}
	}

	return bytes;
}