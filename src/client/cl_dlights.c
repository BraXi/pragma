/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "client.h"

/*
==============================================================

DYNAMIC LIGHT MANAGEMENT

==============================================================
*/

cdlight_t		cl_dlights[MAX_DLIGHTS];

/*
================
CL_ClearDynamicLights
================
*/
void CL_ClearDynamicLights(void)
{
	memset(cl_dlights, 0, sizeof(cl_dlights));
}


/*
===============
CL_AllocDynamicLight
===============
*/
cdlight_t* CL_AllocDynamicLight(int key)
{
	int		i;
	cdlight_t* dl;

	// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

/*
===============
CL_NewDynamicLight
===============
*/
void CL_NewDynamicLight(int key, float x, float y, float z, float radius, float time)
{
	cdlight_t* dl;

	dl = CL_AllocDynamicLight(key);
	dl->origin[0] = x;
	dl->origin[1] = y;
	dl->origin[2] = z;
	dl->radius = radius;
	dl->die = cl.time + time;
}


/*
===============
CL_RunDynamicLights
===============
*/
void CL_RunDynamicLights(void)
{
	int			i;
	cdlight_t* dl;

	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
			continue;

		if (dl->die < cl.time)
		{
			dl->radius = 0;
			return;
		}
		dl->radius -= cls.frametime * dl->decay;
		if (dl->radius < 0)
			dl->radius = 0;
	}
}


/*
===============
CL_AddDynamicLights
===============
*/
void CL_AddDynamicLights(void)
{
	int			i;
	cdlight_t* dl;

	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
			continue;
		V_AddLight(dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}