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
cvar_t		*scr_drawall;

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
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");

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
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_ClearNotify ();
}


void SCR_DrawCenterString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = viddef.height*0.35;
	else
		y = 48;

	do	
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (viddef.width - l*8)/2;
		SCR_AddDirtyPoint (x, y);
		for (j=0 ; j<l ; j++, x+=8)
		{
			re.DrawSingleChar (x, y, start[j]);	
			if (!remaining--)
				return;
		}
		SCR_AddDirtyPoint (x, y+8);
			
		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_centertime_off -= cls.frametime;
	
	if (scr_centertime_off <= 0)
		return;

	re.SetColor(1, 1, 1, 1);
	SCR_DrawCenterString ();
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
	scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);
	scr_conspeed = Cvar_Get ("scr_conspeed", "3", 0);
	scr_showpause = Cvar_Get ("scr_showpause", "1", 0);
	scr_centertime = Cvar_Get ("scr_centertime", "2.5", 0);
	scr_printspeed = Cvar_Get ("scr_printspeed", "8", 0);

	scr_drawall = Cvar_Get ("scr_drawall", "0", 0);

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
	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged < CMD_BACKUP-1)
		return;

	re.DrawImage(scr_vrect.x+64, scr_vrect.y, "code/net");
}

/*
==============
SCR_DrawPause
==============
*/
void SCR_DrawPause (void)
{
	if (!scr_showpause->value)		// turn off for screenshots
		return;

	if (!cl_paused->value)
		return;

	static rgba_t col_white = { 1.0f, 1.0f, 1.0f, 1.0f };
	re.DrawString("[Game Paused]", 400, 10, 1.0, XALIGN_CENTER, col_white);
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
//	int		w, h;
	rgba_t rect_fullscreen;
	rgba_t color;
		
	if (!scr_draw_loading)
		return;

	scr_draw_loading = false;

	rect_fullscreen[0] = rect_fullscreen[1] = 0;
	rect_fullscreen[2] = 800;
	rect_fullscreen[3] = 600;

	VectorSet(color, 0, 0, 0);
	color[3] = 0.5;
	re.NewDrawFill(rect_fullscreen, color);

	VectorSet(color, 1, 1, 1);
	color[3] = 1;
	re.DrawString("loading", 400, 280, 2, 2, color);
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
	re.SetColor(0.1, 0.1, 0.1, 1);
	re.DrawFill(0, 0, viddef.width, viddef.height);

	// weeeewwwy temporarry
	rgba_t c = { 1,1,1,1 };
	re.DrawString("Entering Game", 400, 170, 3, 2, c); //240

	// line
	float rect[4] = { 220, 205, 360, 5 };
	rgba_t c2 = { 0.8, 0.8, 0.8, 1 };
	re.NewDrawFill(rect, c2);

	// server address
	re.DrawString(cls.servername, 400, 225, 1.8, 2, c);

	// display mod name when game is not BASEDIRNAME
	char* mod = Cvar_VariableString("gamedir");
	if (Q_stricmp(mod, BASEDIRNAME))
		re.DrawString(va("Mod: %s", mod), 400, 300, 2, 2, c);

	// map name
	char* mapname = "";
	if (cl.configstrings[CS_MODELS + 1][0])
		mapname = cl.configstrings[CS_MODELS + 1];
	re.DrawString(va("Loading %s...", mapname), 400, 330, 2, 2, c);

	// cheats
	if (CL_CheatsAllowed())
	{
		VectorSet(c, 0.8, 0.2, 0);
		re.DrawString("- CHEATS ENABLED -", 400, 410, 2, 2, c);
	}
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
	
//	if (cls.state == ca_disconnected || cls.state == ca_connecting)
//	{	// forced full screen console
//		Con_DrawConsole (1.0);
//		return;
//	}

#if 1
	if (  cls.state >= ca_connecting && cls.state != ca_active) // || !cl.refresh_prepped)
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

	if (cls.state == ca_disconnected)
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

	if ( cls.state != ca_active )
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

	if (scr_drawall->value)
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

static void SetStringHighBit2(char* s)
{
	while (*s)
		*s++ |= 128;
}

extern void UI_DrawString(int x, int y, UI_AlignX alignx, char* string);
void DrawDownloadNotify()
{
	// fixme -- redo all of this in new UI
	int		w, h;
	char	dlstring[MAX_OSPATH+16];

	if (!cls.download)
		return;

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
}

/*
==================
SCR_DrawFPS

Draws current FPS
==================
*/
extern int frame_time;
extern cvar_t* cl_showfps;
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

	if (fps >= 1000)
		VectorSet(color, 0.3, 0.8, 0);

	if ((int)cl_showfps->value == 1 && mapname[0] && cls.state == ca_active)
		re.DrawString(va("%i FPS (%i ms) on %s", fps, frame_time, cl.configstrings[CS_MODELS + 1]), 795, 5, 0.7, 1, color);
	else
		re.DrawString(va("%i FPS (%i ms)", fps, frame_time), 795, 5, 0.7, 1, color);

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
		re.DrawString("loading", 400, 280, 2, 2, color);
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
		CG_DrawGUI();

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
	float col[4] = { 1,0.4,0.5,1};
	re.DrawString(va("pragma %s prealpha build %s", PRAGMA_VERSION, PRAGMA_TIMESTAMP), 795, 590, 0.7, 1, col);
	SCR_DrawFPS();
#endif

	re.EndFrame();
}
