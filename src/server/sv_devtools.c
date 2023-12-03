/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
// sv_devtools.c

#include "server.h"
#define MAX_DEBUG_PRIMITIVES 512 // ref.h
debugprimitive_t *sv_debugPrimitives;// [MAX_DEBUG_PRIMITIVES] ;

#ifndef DEDICATED_ONLY
extern void V_AddDebugPrimitive(debugprimitive_t* line);
#endif

/*
=================
SV_FreeDevTools
=================
*/
void SV_FreeDevTools()
{
	if (sv_debugPrimitives)
		Z_Free(sv_debugPrimitives);
	sv_debugPrimitives = NULL;
}

/*
=================
SV_InitDevTools
=================
*/
void SV_InitDevTools()
{
//	if (!developer->value)
//		return;

	SV_FreeDevTools();

	sv_debugPrimitives = Z_Malloc(sizeof(debugprimitive_t) * MAX_DEBUG_PRIMITIVES);
}

/*
=================
SV_AddDebugLine
=================
*/
void SV_AddDebugLine(vec3_t p1, vec3_t p2, vec3_t color, float thickness, float drawtime, qboolean depthtested)
{
	if(!sv_debugPrimitives || !developer->value || dedicated->value > 0)
		return;

	debugprimitive_t* object = sv_debugPrimitives;
	for (int i = 0; i < MAX_DEBUG_PRIMITIVES; i++, object++)
	{
		if (sv.gameTime >= object->drawtime)
		{
			object->type = DPRIMITIVE_LINE;
			object->drawtime = sv.gameTime + drawtime;
			object->depthTest = depthtested;
			object->thickness = thickness;
			VectorCopy(p1, object->p1);
			VectorCopy(p2, object->p2);
			VectorCopy(color, object->color);
			return;
		}
	}
	Com_Printf("SV_AddDebugLine: no free debug primitives\n");
}

/*
=================
SV_AddDebugPoint
=================
*/
void SV_AddDebugPoint(vec3_t pos, vec3_t color, float thickness, float drawtime, qboolean depthtested)
{
	if (!sv_debugPrimitives || !developer->value || dedicated->value > 0)
		return;

	debugprimitive_t* line = sv_debugPrimitives;
	for (int i = 0; i < MAX_DEBUG_PRIMITIVES; i++, line++)
	{
		if (sv.gameTime > line->drawtime)
		{
			line->type = DPRIMITIVE_POINT;
			line->drawtime = sv.gameTime + drawtime;
			line->depthTest = depthtested;
			line->thickness = thickness;
			VectorCopy(pos, line->p1);
			VectorCopy(color, line->color);
			return;
		}
	}
	Com_Printf("SV_AddDebugPoint: no free debug primitives\n");
}

/*
=================
SV_AddDebugLine
=================
*/
void SV_AddDebugBox(vec3_t pos, vec3_t p1, vec3_t p2, vec3_t color, float thickness, float drawtime, qboolean depthtested)
{
	if (!sv_debugPrimitives || !developer->value || dedicated->value > 0)
		return;

	debugprimitive_t* line = sv_debugPrimitives;
	for (int i = 0; i < MAX_DEBUG_PRIMITIVES; i++, line++)
	{
		if (sv.gameTime > line->drawtime)
		{
			line->type = DPRIMITIVE_BOX;
			line->drawtime = sv.gameTime + drawtime;
			line->depthTest = depthtested;
			line->thickness = thickness;
			VectorCopy(p1, line->p1);
			VectorCopy(p2, line->p2);
			VectorCopy(color, line->color);

			VectorAdd(line->p1, pos, line->p1);
			VectorAdd(line->p2, pos, line->p2);
			return;
		}
	}
	Com_Printf("SV_AddDebugLine: no free debug primitives\n");
}

/*
=================
SV_AddDebugString
=================
*/
void SV_AddDebugString(vec3_t pos, vec3_t color, float fontSize, float drawtime, qboolean depthtested, char* text)
{
#if 1
	if (!sv_debugPrimitives || !developer->value || dedicated->value > 0)
		return;

	debugprimitive_t* object = sv_debugPrimitives;
	for (int i = 0; i < MAX_DEBUG_PRIMITIVES; i++, object++)
	{
		if (sv.gameTime > object->drawtime)
		{
			object->type = DPRIMITIVE_TEXT;
			object->drawtime = sv.gameTime + drawtime;
			object->depthTest = depthtested;
			object->thickness = fontSize;

			Com_sprintf(object->text, sizeof(object->text), text);

			VectorCopy(pos, object->p1);
			//VectorClear(object->p2);
			VectorCopy(color, object->color);
			return;
		}
	}
	Com_Printf("SV_AddDebugString: no free debug primitives\n");
#endif
}
/*
=================
SV_AddDebugPrimitives
=================
*/
void SV_AddDebugPrimitives()
{
#ifndef DEDICATED_ONLY
	if (sv.state != ss_game)
		return;

	debugprimitive_t *line = sv_debugPrimitives;
	for (int i = 0; i < MAX_DEBUG_PRIMITIVES; i++, line++)
	{
		if (line->drawtime > sv.gameTime)
		{
			V_AddDebugPrimitive(line);
		}

	}
#endif
}