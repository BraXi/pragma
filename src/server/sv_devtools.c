/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
// sv_devtools.c

#include "server.h"
#define MAX_DEBUG_LINES 512 // ref.h
debugline_t *sv_debugLines;// [MAX_DEBUG_LINES] ;

/*
=================
SV_FreeDevTools
=================
*/
void SV_FreeDevTools()
{
	if (sv_debugLines)
		Z_Free(sv_debugLines);
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

	sv_debugLines = Z_Malloc(sizeof(debugline_t) * MAX_DEBUG_LINES);
}

/*
=================
SV_AddDebugLine
=================
*/
void SV_AddDebugLine(vec3_t p1, vec3_t p2, vec3_t color, float thickness, float drawtime, qboolean depthtested)
{
	if(!sv_debugLines || !developer->value || dedicated->value > 0)
		return;

	debugline_t* line = sv_debugLines;
	for (int i = 0; i < MAX_DEBUG_LINES; i++, line++)
	{
		if (sv.gameTime > line->drawtime)
		{
			line->drawtime = sv.gameTime + drawtime;
			line->depthTest = depthtested;
			line->thickness = thickness;
			VectorCopy(p1, line->p1);
			VectorCopy(p2, line->p2);
			VectorCopy(color, line->color);
			return;
		}
	}
	Com_Printf("SV_AddDebugLine: no free debug lines\n");
}

/*
=================
SV_AddDebugLines
=================
*/
extern void V_AddDebugLine(debugline_t *line);
void SV_AddDebugLines()
{
#ifndef DEDICATED_ONLY
	if (sv.state != ss_game)
		return;

	debugline_t *line = sv_debugLines;
	for (int i = 0; i < MAX_DEBUG_LINES; i++, line++)
	{
		if (line->drawtime >= sv.gameTime)
			V_AddDebugLine(line);

	}
#endif
}