/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "client.h"

/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

cvar_t* scr_netgraph;
cvar_t* scr_timegraph;
cvar_t* scr_debuggraph;
cvar_t* scr_graphheight;
cvar_t* scr_graphscale;
cvar_t* scr_graphshift;

typedef struct
{
	float	value;
	vec3_t	color;
} graphsamp_t;

static	int				current;
static	graphsamp_t		values[1024];

/*
==============
CL_DebugGraph
==============
*/
void CL_DebugGraph(float value, vec3_t color)
{
	values[current & 1023].value = value;
	VectorCopy(color, values[current & 1023].color);
	current++;
}

/*
==============
CL_AddNetGraph

A new packet was just parsed
==============
*/
void CL_AddNetGraph()
{
	int		i;
	int		in;
	int		ping;

	static vec3_t c1 = { 0.654902, 0.231373, 0.168627 };
	static vec3_t c2 = { 1.000000, 0.749020, 0.058824 };

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i = 0; i < cls.netchan.dropped; i++)
		CL_DebugGraph(30, c1);

	for (i = 0; i < cl.surpressCount; i++)
		CL_DebugGraph(30, c2);

	// see what the latency was on this packet
	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP - 1);
	ping = cls.realtime - cl.cmd_time[in];
	ping /= 30;
	if (ping > 30)
		ping = 30;

	static vec3_t c = { 0.000000, 1.000000, 0.000000 };
	CL_DebugGraph(ping, c);
}


/*
==============
CL_DrawDebugGraph
==============
*/
void CL_DrawDebugGraph()
{
	int		a, x, y, w, i, h;
	float	v;

	//
	// draw the graph
	//
	w = scr_vrect.width;

	x = scr_vrect.x;
	y = scr_vrect.y + scr_vrect.height;

	re.SetColor(0.482353, 0.482353, 0.482353, 1);
	re.DrawFill(x, y - scr_graphheight->value, w, scr_graphheight->value);

	for (a = 0; a < w; a++)
	{
		i = (current - 1 - a + 1024) & 1023;
		v = values[i].value;

		v = v * scr_graphscale->value + scr_graphshift->value;

		if (v < 0)
			v += scr_graphheight->value * (1 + (int)(-v / scr_graphheight->value));
		h = (int)v % (int)scr_graphheight->value;

		re.SetColor(values[i].color[0], values[i].color[1], values[i].color[2], 1);
		re.DrawFill(x + w - 1 - a, y - h, 1, h);
	}
	re.SetColor(1, 1, 1, 1);

}

/*
==============
CL_DrawGraphOnScreen
==============
*/
void CL_DrawGraphOnScreen()
{
	static vec3_t col_black = { 0,0,0 };
	if (scr_timegraph->value)
		CL_DebugGraph(cls.frametime * 300, col_black);

	if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
		CL_DrawDebugGraph();
}

/*
==============
CL_InitGraph
==============
*/
void CL_InitGraph()
{
	scr_netgraph = Cvar_Get("netgraph", "0", 0, NULL);
	scr_timegraph = Cvar_Get("timegraph", "0", 0, NULL);
	scr_debuggraph = Cvar_Get("debuggraph", "0", 0, NULL);
	scr_graphheight = Cvar_Get("graphheight", "32", 0, NULL);
	scr_graphscale = Cvar_Get("graphscale", "1", 0, NULL);
	scr_graphshift = Cvar_Get("graphshift", "0", 0, NULL);
}