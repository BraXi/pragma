/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

// cg_hud.c

#include "cg_local.h"

cvar_t* cg_crosshair;
qboolean bMenuActive = false;

int ps_stats[32];

static char* layout_string;
static char* layout_token;
static hud_element_t hudelem;


//
// layout operations
//
typedef struct
{
	char* name;
	void (*func)(void);
} layoutOperations_t;

static void HudElem_Layout_RectX(void)			{ hudelem.x = atoi(layout_token); };
static void HudElem_Layout_RectXRight(void)		{ hudelem.x = loc.width - atoi(layout_token); }
static void HudElem_Layout_RectXMiddle(void)	{ hudelem.x = loc.width / 2 - 160 + atoi(layout_token); }
static void HudElem_Layout_RectY(void)			{ hudelem.y = atoi(layout_token); };
static void HudElem_Layout_RectYBottom(void)	{ hudelem.y = loc.height - atoi(layout_token); }
static void HudElem_Layout_RectYMiddle(void)	{ hudelem.y = loc.height / 2 - 120 + atoi(layout_token); }

static void HudElem_Layout_Operator_IF(void)
{
	if (ps_stats[atoi(layout_token)] > 0)
		return; // condition > 0

	// skip to endif
	while (layout_string && strcmp(layout_token, "endif"))
		layout_token = COM_Parse(&layout_string);
}

static void HudElem_Layout_Pic(void)
{
	int		value;
//	char*	name;

	value = ps_stats[atoi(layout_token)];
	if (value >= MAX_IMAGES)
		gi.Error("%s: %i >= MAX_IMAGES\n", __FUNCTION__, value);

	////if ((name = gi.GetConfigString(CS_IMAGES + value)))
	//	gi.DrawPic(hudelem.x, hudelem.y, name);
}

static void HudElem_Layout_DrawImage(void)
{
	int		value;
	char* name;

	value = ps_stats[atoi(layout_token)];

	if (value >= MAX_IMAGES)
		gi.Error("%s: %i >= MAX_IMAGES\n", __FUNCTION__, value);

	layout_token = COM_Parse(&layout_string);
	hudelem.w = atoi(layout_token);
	layout_token = COM_Parse(&layout_string);
	hudelem.h = atoi(layout_token);

	name = gi.GetConfigString(CS_IMAGES + value);
	if (name)
	{
		//if (hudelem.w > 0 && hudelem.h > 0)
		//	gi.DrawImage(hudelem.x, hudelem.y, hudelem.w, hudelem.h, name);
	//	else
		//	gi.DrawPic(hudelem.x, hudelem.y, name);
	}
}

static void HudElem_Layout_DrawString(void)
{
	hudelem.xalign = atoi(layout_token);
	layout_token = COM_Parse(&layout_string);
//	gi.DrawString(hudelem.x, hudelem.y, hudelem.xalign, layout_token);
}

static void HudElem_Layout_DrawStringFromCS(void)
{
	int index;

	hudelem.xalign = atoi(layout_token);

	layout_token = COM_Parse(&layout_string);
	index = atoi(layout_token);
	if (index < 0 || index >= MAX_STATS)
		gi.Error("%s: bad index %i", __FUNCTION__, index);

	index = ps_stats[index];
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		gi.Error("%s: bad configstring index %i", __FUNCTION__, index);

//	gi.DrawString(hudelem.x, hudelem.y, hudelem.xalign, layout_token);
}

static layoutOperations_t layout_ops[] =
{
	{"xl", HudElem_Layout_RectX},			// X pos [0+x]
	{"xr", HudElem_Layout_RectXRight},		// X pos from right [width-x]
	{"xv", HudElem_Layout_RectXMiddle},		// X middle [width/2+x]
	{"yt", HudElem_Layout_RectY},			// Y pos [0+y]
	{"yb", HudElem_Layout_RectYBottom},		// Y pos from right [height-y]
	{"yv", HudElem_Layout_RectYMiddle},		// Y middle [height/2+x]

	{"if", HudElem_Layout_Operator_IF},		// if X > 0 ..ops.. endif

	{"pic", HudElem_Layout_Pic},			
	{"img", HudElem_Layout_DrawImage},

	{"str", HudElem_Layout_DrawString},		// [xalign, string]
	{"cstr", HudElem_Layout_DrawStringFromCS} // [xalign, configstring index]
};
int num_layout_ops = sizeof(layout_ops) / sizeof(layoutOperations_t);

// =========================================

/*
================
CG_HUD_ExecuteLayoutString

Executes a very simple HUD program
================
*/
static void CG_HUD_ExecuteLayoutString(char* s)
{
	int		op;
	layoutOperations_t *lop;

	if (!s[0])
		return;

	layout_string = s;
	memset(&hudelem, 0, sizeof(hud_element_t));

	while (layout_string)
	{
	next:
		layout_token = COM_Parse(&layout_string);
		for (op = 0; op < num_layout_ops; op++)
		{
			lop = &layout_ops[op];
			if (!strcmp(layout_token, lop->name))
			{
				layout_token = COM_Parse(&layout_string);
				lop->func();
				goto next;
			}
		}
	}
}



/*
====================
CG_HUD_DrawCrosshair
====================
*/
static void CG_HUD_DrawCrosshair()
{
	int x, y;
	float scale;
	const float chSize[2] = { 20, 16 };

	if (!cg_crosshair->value || !bMenuActive)
		return;

	scale = cg_crosshair->value;

	x = loc.width / 2 - (chSize[0] / 2 * scale);
	y = loc.height / 2 - (chSize[1] / 2 * scale);
	//gi.DrawImage(x, y, 20 * chSize[0] * scale, chSize[1] * scale);
}


/*
==========
CG_DrawHUD
==========
*/
void CG_DrawHUD(void)
{
	// draw hud
	CG_HUD_ExecuteLayoutString(gi.GetConfigString(CS_HUD));

	// draw hud overlay (scoreboard, intermission, etc)
	if ((ps_stats[STAT_LAYOUTS] & 1) && ps_stats[STAT_LAYOUTS])
	{
		//CG_HUD_ExecuteLayoutString(cl_hud_layout_string);
	}

	// draw crosshair
	CG_HUD_DrawCrosshair();
}

#if 0

if (!strcmp(token, "num"))
{	// draw a number
	token = COM_Parse(&s);
	width = atoi(token);
	token = COM_Parse(&s);
	value = cl.frame.playerstate.stats[atoi(token)];
	SCR_DrawField(x, y, 0, width, value);
	continue;
}

if (!strcmp(token, "hnum"))
{	// health number
	int		color;

	width = 3;
	value = cl.frame.playerstate.stats[STAT_HEALTH];
	if (value > 25)
		color = 0;	// green
	else if (value > 0)
		color = (cl.frame.serverframe >> 2) & 1;		// flash
	else
		color = 1;

	if (cl.frame.playerstate.stats[STAT_FLASHES] & 1)
		re.DrawPic(x, y, "field_3");

	SCR_DrawField(x, y, color, width, value);
	continue;
}

if (!strcmp(token, "anum"))
{	// ammo number
	int		color;

	width = 3;
	value = cl.frame.playerstate.stats[STAT_AMMO];
	if (value > 5)
		color = 0;	// green
	else if (value >= 0)
		color = (cl.frame.serverframe >> 2) & 1;		// flash
	else
		continue;	// negative number = don't show

	if (cl.frame.playerstate.stats[STAT_FLASHES] & 4)
		re.DrawPic(x, y, "field_3");

	SCR_DrawField(x, y, color, width, value);
	continue;
}
#endif