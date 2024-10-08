/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cl_screen.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  cinematics
  full screen image for quit and victory

  end of unit intermissions

  */

#include "client.h"

float		scr_con_current;	// aproaches scr_conlines at scr_conspeed
float		scr_conlines;		// 0.0 to 1.0 lines of console to display

qboolean	scr_initialized;		// ready to draw

int			scr_draw_loading;

vrect_t		scr_vrect;		// position of render window on screen

cvar_t		*scr_viewsize;
cvar_t		*scr_conspeed;
cvar_t		*scr_centertime;
cvar_t		*scr_showpause;
cvar_t		*scr_printspeed;

extern void CL_DrawGraphOnScreen();
extern void CL_InitGraph();

typedef struct
{
	int		x1, y1, x2, y2;
} dirty_t;

dirty_t		scr_dirty, scr_old_dirty[2];

void SCR_TimeRefresh_f (void);
void SCR_Loading_f (void);



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	char	*s;
	char	line[64];
	int		i, j, l;

	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.time;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;
		s++;
	}

	// echo it to the console
	Com_Printf("====================\n");
	s = str;
	do	
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (s[l] == '\n' || !s[l])
				break;

		for (i=0 ; i<(40-l)/2 ; i++)
			line[i] = ' ';

		for (j=0 ; j<l ; j++)
		{
			line[i++] = s[j];
		}

		line[i] = '\n';
		line[i+1] = 0;

		Com_Printf ("%s", line);

		while (*s && *s != '\n')
			s++;

		if (!*s)
			break;
		s++;		// skip the \n
	} while (1);
	Com_Printf("====================\n");
	Con_ClearNotify ();
}


void SCR_CheckDrawCenterString (void)
{
	int x, y;
	int font = 0;
	float scale = 0.35;
	vec4_t color = { 0.05f, 0.05f, 0.05f, 1.0f };

	scr_centertime_off -= cls.frametime;
	
	if (scr_centertime_off <= 0)
		return;

	x = viddef.width / 2;
	y = viddef.height / 6;

	char* text = scr_centerstring;
	while (*text && *text == '\n')
	{
		text++;
	}

	int width = re.GetTextWidth(font, text) * scale; // text width in pixels
	int charheight = re.GetFontHeight(font) * scale; // text height in pixels

	re.SetColor(1,1,1, 0.85);
	re.DrawFill(x - (width / 2) - 24, y - 4, width + 48, charheight + 8);

	re.NewDrawString(x, y, XALIGN_CENTER, font, scale, color, text);
	//y += re.GetFontHeight(font) * scale;

	//SCR_DrawCenterString ();
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void)
{
	int		size;

	// bound viewsize
	if (scr_viewsize->value < 40)
		Cvar_Set ("viewsize","40");
	if (scr_viewsize->value > 100)
		Cvar_Set ("viewsize","100");

	size = scr_viewsize->value;

	scr_vrect.width = viddef.width*size/100;
	scr_vrect.width &= ~7;

	scr_vrect.height = viddef.height*size/100;
	scr_vrect.height &= ~1;

	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (viddef.height - scr_vrect.height)/2;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f(void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value+10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f(void)
{
	Cvar_SetValue ("viewsize",scr_viewsize->value-10);
}

/*
=================
SCR_Sky_f

Set a specific sky, rotation speed and sky color
=================
*/
void SCR_Sky_f (void)
{
	float	rotate;
	vec3_t	axis, color;

	if (!CL_CheatsAllowed())
	{
		Com_Printf("'%s' - cheats not allowed\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sky <skyname> <rotate> <axis x y z> <color r g b>\n");
		return;
	}
	if (Cmd_Argc() > 2)
		rotate = atof(Cmd_Argv(2));
	else
		rotate = 0;

	if (Cmd_Argc() >= 6)
	{
		axis[0] = atof(Cmd_Argv(3));
		axis[1] = atof(Cmd_Argv(4));
		axis[2] = atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	if (Cmd_Argc() >= 9)
	{
		color[0] = atof(Cmd_Argv(6));
		color[1] = atof(Cmd_Argv(7));
		color[2] = atof(Cmd_Argv(8));
	}
	else
	{
		color[0] = color[1] = color[2] = 1;
	}
	re.SetSky (Cmd_Argv(1), rotate, axis, color);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE, NULL);
	scr_conspeed = Cvar_Get ("scr_conspeed", "3", 0, NULL);
	scr_showpause = Cvar_Get ("scr_showpause", "1", 0, NULL);
	scr_centertime = Cvar_Get ("scr_centertime", "2.5", 0, NULL);
	scr_printspeed = Cvar_Get ("scr_printspeed", "8", 0, NULL);

	CL_InitGraph();

//
// register our commands
//
	Cmd_AddCommand ("timerefresh",SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading",SCR_Loading_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);
	Cmd_AddCommand ("sky",SCR_Sky_f);

	scr_initialized = true;
}


/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	static rgba_t col_red = { 0.8f, 0.1f, 0.2f, 1.0f };
	vec2_t xy;
	static const int fontid = 0; // FIXME

	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged < CMD_BACKUP-1)
		return;

	xy[0] = scr_vrect.x + (scr_vrect.width / 2.0f);
	xy[1] = scr_vrect.y + scr_vrect.height - ((re.GetFontHeight(0) * 0.5f) * 5);

	re.NewDrawString(xy[0], xy[1], XALIGN_CENTER, fontid, 0.5f, col_red, "Connection Problem?");

	xy[1] += (re.GetFontHeight(0) * 0.5f);

	re.NewDrawString(xy[0], xy[1], XALIGN_CENTER, fontid, 0.33f, col_red, "No response from server.");
}

/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause(void)
{
	static rgba_t col_white = { 1.0f, 1.0f, 1.0f, 1.0f };
	vec2_t xy;
	static const int fontid = 0; // FIXME

	if (!scr_showpause->value)
		return; // turn off for screenshots

	if (!cl_paused->value)
		return;

	xy[0] = scr_vrect.x + (scr_vrect.width / 2.0f);
	xy[1] = scr_vrect.y + (scr_vrect.height / 2.0f) - ((re.GetFontHeight(0) * 0.5f) / 2);

	re.NewDrawString(xy[0], xy[1], XALIGN_CENTER, fontid, 0.5f, col_white, "Game Paused");
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
//	int		w, h;
	vec4_t rect;
	int fontid = 0; // fixme
	rgba_t rect_fullscreen;
	rgba_t color;
		
	if (!scr_draw_loading)
		return;

	scr_draw_loading = false;
	
	Vector4Set(rect, scr_vrect.x, scr_vrect.y, scr_vrect.width, scr_vrect.height);
	Vector4Set(color, 0.0f, 0.0f, 0.0f, 0.7f);
	re.NewDrawFill(rect_fullscreen, color);

	rect[0] = scr_vrect.x + (scr_vrect.width / 2.0f);
	rect[1] = scr_vrect.y + (scr_vrect.height / 2.0f) - ((re.GetFontHeight(0) * 0.5f));
	Vector4Set(color, 0.84f, 0.8f, 0.8f, 1.0f);

	re.NewDrawString(rect[0], rect[1], XALIGN_CENTER, fontid, 0.5f, color, "You're safe for now...");

	rect[1] += ((re.GetFontHeight(0) * 0.6f));
	re.NewDrawString(rect[0], rect[1], XALIGN_CENTER, fontid, 0.3f, color, "Entering next chapter.");
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void SCR_RunConsole (void)
{
// decide on the height of the console
	if (cls.key_dest == key_console)
		scr_conlines = 0.5;		// half screen
	else
		scr_conlines = 0;				// none visible
	
	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*cls.frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*cls.frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

}

static void SCR_DrawLoadingScreen()
{
	//TODO: MOVE TO GUI QC !!!
	static const int fontid = 0;
	float fontscale;
	vec4_t rect;
	rgba_t color;
	float fontheight;
	char* str;

	rgba_t col_white = { 1,1,1, 1.0f };


	// background
	Vector4Set(rect, scr_vrect.x, scr_vrect.y, scr_vrect.width, scr_vrect.height);
	Vector4Set(color, 0.08f, 0, 0.02, 1.0f);
	re.NewDrawFill(rect, color);

	// connecting to...
	fontscale = 0.35f;
	fontheight = re.GetFontHeight(fontid) * fontscale;
	str = va("Connecting to %s...", cls.servername);
	Vector4Set(rect, scr_vrect.x + 10.0f, 12.0f, 0, 0);
	re.NewDrawString(rect[0], rect[1], XALIGN_LEFT, fontid, fontscale, col_white, str);

	// level name
	rect[1] += fontheight;
	fontscale = 0.6f;
	fontheight = re.GetFontHeight(fontid) * fontscale;
	str = cl.configstrings[CS_MODELS+1];
	re.NewDrawString(rect[0], rect[1], XALIGN_LEFT, fontid, fontscale, col_white, str);

	// top line
	rect[0] = scr_vrect.x;
	rect[1] = scr_vrect.y + 80.0;
	rect[2] = scr_vrect.width;
	rect[3] = 4.0f;

	Vector4Set(color, 0.7f, 0.7f, 0.7f, 0.5f);
	re.NewDrawFill(rect, color);
	//
	//(*DrawStretchedImage)(rect_t rect, rgba_t color, char* pic);
	Vector4Set(rect, scr_vrect.x, rect[1]+4.0f, scr_vrect.width, scr_vrect.height-168);
	re.DrawStretchedImage(rect, col_white, "levelshots/test");

	// bottom line
	rect[0] = scr_vrect.x;
	rect[1] = scr_vrect.y + scr_vrect.height - 80.0;
	rect[2] = scr_vrect.width;
	rect[3] = 4.0f;

	Vector4Set(color, 0.7f, 0.7f, 0.7f, 0.5f);
	re.NewDrawFill(rect, color);

	if (CL_CheatsAllowed())
	{
		fontscale = 0.4f;
		fontheight = re.GetFontHeight(fontid) * fontscale;
		str = "cheats allowed | developer mode";

		Vector4Set(rect, scr_vrect.x + (scr_vrect.width / 2), rect[1] + 20, 0, 0);
		re.NewDrawString(rect[0], rect[1], XALIGN_CENTER, fontid, fontscale, col_white, str);
	}

	// players
	fontscale = 0.7f;
	fontheight = re.GetFontHeight(fontid) * fontscale;
	str = va("Survival");
	Vector4Set(rect, scr_vrect.x + scr_vrect.width / 2, (scr_vrect.y + 10), 0, 0);

	Vector4Set(color, 0.75f, 0.05, 0, 0.75f);
	re.NewDrawString(rect[0], rect[1], XALIGN_CENTER, fontid, fontscale, color, str);

	// players
	rect[1] += fontheight - 5;
	fontscale = 0.3f;
	fontheight = re.GetFontHeight(fontid) * fontscale;
	str = va("Survive waves of infected and escape");

	//Vector4Set(color, 0.8f, 0.8, 0.8, 0.8f);
	re.NewDrawString(rect[0], rect[1], XALIGN_CENTER, fontid, fontscale, color, str);



#if 0
	// line
	float rect[4] = { 220, 205, 360, 5 };
	rgba_t c2 = { 0.8, 0.8, 0.8, 1 };
	re.NewDrawFill(rect, c2);

	// server address
	re._DrawString(cls.servername, 400, 225, 1.8, 2, c);

	// display mod name when game is not BASEDIRNAME
	char* mod = Cvar_VariableString("gamedir");
	if (Q_stricmp(mod, BASEDIRNAME))
		re._DrawString(va("Mod: %s", mod), 400, 300, 2, 2, c);

	// map name
	char* mapname = "";
	if (cl.configstrings[CS_MODELS + 1][0])
		mapname = cl.configstrings[CS_MODELS + 1];
	re._DrawString(va("Loading %s...", mapname), 400, 330, 2, 2, c);

	// cheats
	if (CL_CheatsAllowed())
	{
		VectorSet(c, 0.8, 0.2, 0);
		re._DrawString("- CHEATS ENABLED -", 400, 410, 2, 2, c);
	}

#endif
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	Con_CheckResize ();

	re.SetColor(1, 1, 1, 1);
	
//	if (cls.state == CS_DISCONNECTED || cls.state == CS_CONNECTING)
//	{	// forced full screen console
//		Con_DrawConsole (1.0);
//		return;
//	}

#if 1
	if (  cls.state >= CS_CONNECTING && cls.state != CS_ACTIVE) // || !cl.refresh_prepped)
	{	
		// connected, but can't render

#if 0
		Con_DrawConsole(0.5);
		re.DrawFill (0, viddef.height/2, viddef.width, viddef.height/2, 0);
#else
		SCR_DrawLoadingScreen();
#endif
//		return;
	}
#endif
	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current);
	}
	else
	{
		if (cls.key_dest == key_game || cls.key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}

//=============================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque(void)
{
	S_StopAllSounds();
	cl.sound_prepped = false;		// don't play ambients

	if (cls.disable_screen)
		return;

//	if (developer->value)
//		return; //not needed, printing to remote console

	if (cls.state == CS_DISCONNECTED)
		return;	

	// close console
	if (cls.key_dest == key_console)
	{
		cls.key_dest = key_game;
		return;
	}

	if (cl.cinematictime > 0)
		scr_draw_loading = 2;	// clear to black first
	else
		scr_draw_loading = 1;

	SCR_UpdateScreen ();
	cls.disable_screen = Sys_Milliseconds ();
	cls.disable_servercount = cl.servercount;
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	cls.disable_screen = 0;
	Con_ClearNotify ();
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f (void)
{
	SCR_BeginLoadingPlaque ();
}

/*
================
SCR_TimeRefresh_f
================
*/
int entitycmpfnc( const rentity_t *a, const rentity_t *b )
{
	/*
	** all other models are sorted by model
	*/
	{
		return ( ( int ) a->model - ( int ) b->model );
	}
}

void SCR_TimeRefresh_f (void)
{
	int		i;
	int		start, stop;
	float	time;

	if ( cls.state != CS_ACTIVE )
		return;

	start = Sys_Milliseconds ();

	if (Cmd_Argc() == 2)
	{	// run without page flipping
		re.BeginFrame( 0 );
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.view.angles[1] = i/128.0*360.0;
			re.RenderFrame (&cl.refdef, false);
		}
		re.EndFrame();
	}
	else
	{
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.view.angles[1] = i/128.0*360.0;

			re.BeginFrame( 0 );
			re.RenderFrame (&cl.refdef, false);
			re.EndFrame();
		}
	}

	stop = Sys_Milliseconds ();
	time = (stop-start)/1000.0;
	Com_Printf ("%f seconds (%f fps)\n", time, 128/time);
}

/*
=================
SCR_AddDirtyPoint
=================
*/
void SCR_AddDirtyPoint (int x, int y)
{
	if (x < scr_dirty.x1)
		scr_dirty.x1 = x;
	if (x > scr_dirty.x2)
		scr_dirty.x2 = x;
	if (y < scr_dirty.y1)
		scr_dirty.y1 = y;
	if (y > scr_dirty.y2)
		scr_dirty.y2 = y;
}

void SCR_DirtyScreen (void)
{
	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width-1, viddef.height-1);
}

/*
==============
SCR_TileClear

Clear any parts of the tiled background that were drawn on last frame
==============
*/
void SCR_TileClear (void)
{
	int		i;
	int		top, bottom, left, right;
	dirty_t	clear;

	SCR_DirtyScreen ();	// for power vr or broken page flippers...

	if (scr_con_current == 1.0)
		return;		// full screen console
	if (scr_viewsize->value == 100)
		return;		// full screen rendering
	if (cl.cinematictime > 0)
		return;		// full screen cinematic

	// erase rect will be the union of the past three frames
	// so tripple buffering works properly
	clear = scr_dirty;
	for (i=0 ; i<2 ; i++)
	{
		if (scr_old_dirty[i].x1 < clear.x1)
			clear.x1 = scr_old_dirty[i].x1;
		if (scr_old_dirty[i].x2 > clear.x2)
			clear.x2 = scr_old_dirty[i].x2;
		if (scr_old_dirty[i].y1 < clear.y1)
			clear.y1 = scr_old_dirty[i].y1;
		if (scr_old_dirty[i].y2 > clear.y2)
			clear.y2 = scr_old_dirty[i].y2;
	}

	scr_old_dirty[1] = scr_old_dirty[0];
	scr_old_dirty[0] = scr_dirty;

	scr_dirty.x1 = 9999;
	scr_dirty.x2 = -9999;
	scr_dirty.y1 = 9999;
	scr_dirty.y2 = -9999;

	// don't bother with anything convered by the console)
	top = scr_con_current*viddef.height;
	if (top >= clear.y1)
		clear.y1 = top;

	if (clear.y2 <= clear.y1)
		return;		// nothing disturbed

	top = scr_vrect.y;
	bottom = top + scr_vrect.height-1;
	left = scr_vrect.x;
	right = left + scr_vrect.width-1;

	if (clear.y1 < top)
	{	// clear above view screen
		i = clear.y2 < top-1 ? clear.y2 : top-1;
		re.DrawTileClear (clear.x1 , clear.y1,
			clear.x2 - clear.x1 + 1, i - clear.y1+1, "backtile");
		clear.y1 = top;
	}
	if (clear.y2 > bottom)
	{	// clear below view screen
		i = clear.y1 > bottom+1 ? clear.y1 : bottom+1;
		re.DrawTileClear (clear.x1, i,
			clear.x2-clear.x1+1, clear.y2-i+1, "backtile");
		clear.y2 = bottom;
	}
	if (clear.x1 < left)
	{	// clear left of view screen
		i = clear.x2 < left-1 ? clear.x2 : left-1;
		re.DrawTileClear (clear.x1, clear.y1,
			i-clear.x1+1, clear.y2 - clear.y1 + 1, "backtile");
		clear.x1 = left;
	}
	if (clear.x2 > right)
	{	// clear left of view screen
		i = clear.x1 > right+1 ? clear.x1 : right+1;
		re.DrawTileClear (i, clear.y1,
			clear.x2-i+1, clear.y2 - clear.y1 + 1, "backtile");
		clear.x2 = right;
	}

}

//=======================================================

void DrawDownloadNotify()
{
	// fixme -- redo all of this in new UI
	if (!cls.download)
		return;

#if 0
	extern void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);
	Com_sprintf(dlstring, sizeof(dlstring), "%s [%02d%%]", cls.downloadname, cls.downloadpercent);
	SetStringHighBit2(dlstring);

	w = viddef.width / 2;
	h = (viddef.height / 2) - 100;

	re.DrawStretchImage(0, 0, viddef.width, viddef.height, "backtile");

	UI_DrawString(w, h, XALIGN_CENTER, va("Connecting to: %s", cls.servername));

	UI_DrawString(w, h+=48, XALIGN_CENTER,  "DOWNLOADING CUSTOM CONTENT...");
	UI_DrawString(w, h += 12, XALIGN_CENTER,"===============================");

	UI_DrawString(w, h += 36, XALIGN_CENTER, dlstring);
//	UI_DrawString(w, h += 96, XALIGN_CENTER, "press [esc] to cancel");
#endif
}

/*
==================
SCR_DrawFPS

Draws current FPS
==================
*/
extern int frame_time;
extern cvar_t* cl_showfps, *cl_maxfps;
static void SCR_DrawFPS()
{
	float color[4];
	int fps;
	char* mapname;

	if (!cl_showfps->value)
		return;

	mapname = cl.configstrings[CS_MODELS + 1];

	if (frame_time <= 0)
		frame_time = 1;

	fps = 1000 / frame_time;

	re.SetColor(1, 1, 1, 1);
	VectorSet(color, 1, 1, 1);
	color[3] = 1;


	if (fps > cl_maxfps->value)
		fps = cl_maxfps->value;

	if (fps >= 1000)
		VectorSet(color, 0.3, 0.8, 0);

	if ((int)cl_showfps->value == 1 && mapname[0] && cls.state == CS_ACTIVE)
		re.NewDrawString(viddef.width - 5, 4, 2, 0, 0.25f, color, va("%i FPS (%i ms) on %s", fps, frame_time, cl.configstrings[CS_MODELS + 1]));
	else
		re.NewDrawString(viddef.width - 5, 4, 2, 0, 0.25f, color, va("%i FPS (%i ms)", fps, frame_time));

}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	float color[4];

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (Sys_Milliseconds() - cls.disable_screen > 120000)
		{
			cls.disable_screen = 0;
			Com_Printf ("Loading plaque timed out.\n");
		}
		return;
	}

	if (!scr_initialized || !con.initialized)
		return;				// not initialized yet

	re.BeginFrame( 0 );

	if (scr_draw_loading == 2) //loading plaque over black screen
	{	
		re.RenderFrame(&cl.refdef, true); // only gui
		scr_draw_loading = false;
		VectorSet(color, 1, 1, 1);
		color[3] = 1;
		re._DrawString("loading", 400, 280, 2, 2, color);
	} 
	else if (cl.cinematictime > 0) // in cinematic, handle menus and console specially
	{
		re.RenderFrame(&cl.refdef, true); // only gui
		if (cls.key_dest == key_menu)
		{
			UI_Draw();
		}
		else if (cls.key_dest == key_console)
		{
			SCR_DrawConsole ();
		}
		else
		{
			SCR_DrawCinematic();
		}
	}
	else 
	{
		// do 3D refresh drawing, and then update the screen
		SCR_CalcVrect ();

		// clear any dirty part of the background
		SCR_TileClear ();

		//
		// render world
		//
		V_RenderView(0);

		//
		// gui rendering at this point
		//

		re.SetColor(1, 1, 1, 1);
		//CG_DrawGUI();

		re.SetColor(1, 1, 1, 1);
		UI_Draw();

		SCR_CheckDrawCenterString();

		CL_DrawGraphOnScreen();

		SCR_DrawPause ();

		SCR_DrawConsole ();

		DrawDownloadNotify();

		SCR_DrawLoading ();

		//devtools
		SCR_DrawNet();

	}

#if 1
	float col[4] = { 0.9,1.0,0.90,0.6};
	re.NewDrawString(viddef.width - 5 , viddef.height - (re.GetFontHeight(0)*0.25), 2, 0, 0.25f, col, va("PRAGMA %s PREALPHA BUILD %s", PRAGMA_VERSION, PRAGMA_TIMESTAMP));
	SCR_DrawFPS();
#endif

	re.EndFrame();
}
