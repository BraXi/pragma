/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// console.c

#include "client.h"

console_t	con;

cvar_t		*con_notifytime;
float con_fontscale = 0.25;
float con_charheight = 8;

#define		MAXCMDLINE	256
extern	char	key_lines[32][MAXCMDLINE];
extern	int		edit_line;
extern	int		key_linepos;
		
//static int con_charsize = 8;
static int con_font = 0;

static vec4_t col_white = { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t col_white2 = { 0.8f, 0.8f, 0.8f, 1.0f };
static vec4_t col_red = { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t col_green = { 0.0f, 1.0f, 0.0f, 1.0f };
static vec4_t col_blue = { 0.0f, 0.0f, 1.0f, 1.0f };
static vec4_t col_yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
static vec4_t col_orange = { 1.0f, 0.59f, 0.0f, 1.0f };

// re.DrawString(float x, float y, int alignx, int charSize, int fontId, vec4_t color, const char* str);

void DrawString (int x, int y, char *s)
{
	//re.DrawString(x, y, 0, con_charsize->value, con_font, col_white, s);

	re.NewDrawString(x, y, con_font, 0, con_fontscale, col_white, s);
}



void Key_ClearTyping (void)
{
	key_lines[edit_line][1] = 0;	// clear any typing
	key_linepos = 1;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	SCR_EndLoadingPlaque ();	// get rid of loading plaque

	if (cl.attractloop)
	{
		Cbuf_AddText ("killserver\n");
		return;
	}

	Key_ClearTyping ();
	Con_ClearNotify ();

	if (cls.key_dest == key_console)
	{
		if(UI_NumOpenedGuis() > 0 && cls.state < CS_ACTIVE)
			cls.key_dest = key_menu;
		else if (cls.state >= CS_ACTIVE && UI_NumOpenedGuis() == 0)
			cls.key_dest = key_game;
		else
			cls.key_dest = key_console;

//		Cvar_Set ("paused", "0"); // braxi: console no longer pauses the game
	}
	else
	{
		cls.key_dest = key_console;	
	}
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f (void)
{
	Key_ClearTyping ();

	if (cls.key_dest == key_console)
	{
		if (cls.state == CS_ACTIVE)
		{
			UI_CloseAllGuis();
			cls.key_dest = key_game;
		}
	}
	else
		cls.key_dest = key_console;
	
	Con_ClearNotify ();
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	memset (con.text, ' ', CON_TEXTSIZE);
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x;
	char	*line;
	FILE	*f;
	char	buffer[1024];
	char	name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_sprintf (name, sizeof(name), "%s/%s.txt", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("Dumped console text to %s.\n", name);
	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		strncpy (buffer, line, con.linewidth);
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		fprintf (f, "%s\n", buffer);
	}

	fclose (f);
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;
	
	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con.times[i] = 0;
}

						
/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	char	tbuf[CON_TEXTSIZE];
	
	//width = (viddef.width >> 3) - 2;
	width = viddef.width/con_charheight;

	if (width == con.linewidth)
		return;

	if(re.GetFontHeight)
		con_charheight = re.GetFontHeight(con_font) * con_fontscale;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 38;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		memset (con.text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width; //viddef.width/con_charsize->value
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		memcpy (tbuf, con.text, CON_TEXTSIZE);
		memset (con.text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void)
{
	con_notifytime = Cvar_Get ("con_notifytime", "4", 0);

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);


	con.linewidth = -1;
	Con_CheckResize();

	Com_Printf("Console initialized.\n");
	con.initialized = true;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	memset (&con.text[(con.current%con.totallines)*con.linewidth], ' ', con.linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;

	if (!con.initialized)
		return;

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
		mask = 0;


	while ( (c = *txt) != NULL )
	{
	// count word length
		for (l=0 ; l< con.linewidth ; l++)
			if ( txt[l] <= ' ')
				break;

	// word wrap
		if (l != con.linewidth && (con.x + l > con.linewidth) )
			con.x = 0;

		txt++;

		if (cr)
		{
			con.current--;
			cr = false;
		}

		
		if (!con.x)
		{
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con.current >= 0)
				con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}

		switch (c)
		{
		case '\n':
			con.x = 0;
			break;

		case '\r':
			con.x = 0;
			cr = 1;
			break;

		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = c | mask | con.ormask;
			con.x++;
			if (con.x >= con.linewidth)
				con.x = 0;
			break;
		}
		
	}
}


/*
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint (char *text)
{
	int		l;
	char	buffer[1024];

	l = strlen(text);
	l = (con.linewidth-l)/2;
	if (l < 0)
		l = 0;
	memset (buffer, ' ', l);
	strcpy (buffer+l, text);
	strcat (buffer, "\n");
	Con_Print (buffer);
}

/*
==============================================================================

DRAWING

==============================================================================
*/

static void Con_ShowSuggestions(char* text)
{
	cvar_t* cvars[48];
	int		found = 0;
	int		i, x, y = 10, w = 0;

	if (key_linepos < 2)
		return;


	cvar_t* var;
	for (var = cvar_vars; var; var = var->next)
	{
		if (Q_strncasecmp(text, var->name, key_linepos - 1))
			continue;

		cvars[found] = var;
		found++;
	
		i = re.GetTextWidth(con_font, va("%s %s", var->name, var->string)) * con_fontscale;
		//i = (strlen(var->name) + strlen(var->string)+1) * con_charsize->value;
		if (i > w)
			w = i;

		if (found == 48)
			break;
	}

	if (found)
	{
		//re.DrawString(10, con.vislines + y, 0, con_charsize->value, con_font, col_orange, va("Listing %i matching cvars:", found));
		x = 12;
		y = con.vislines + con_charheight;

		//re.SetColor(0, 0, 0, 0.4);
		//re.DrawFill(x, y, w, con_charheight * found);

		for (i = 0; i < found; i++)
		{
			if(i%2)
				re.NewDrawString(x, y, 0, con_font, con_fontscale, col_yellow, va("%s %s", cvars[i]->name, cvars[i]->string));
			else
				re.NewDrawString(x, y, 0, con_font, con_fontscale, col_orange, va("%s %s", cvars[i]->name, cvars[i]->string));
			y += con_charheight;

			if (y >= viddef.height - con_charheight*2)
			{
				x += w + 12;
				y = con.vislines + con_charheight;
			}
		}
	}
}

/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (void)
{
	int		y;
	int		i;
	char	*text;

	if (cls.key_dest == key_menu)
		return;

	if (cls.key_dest != key_console && cls.state == CS_ACTIVE)
		return;		// don't draw anything (always draw if not active)

	text = key_lines[edit_line];
	
	// add the cursor frame
	if (((int)(cls.realtime >> 8) & 1))
		text[key_linepos] = '_';
	
	// fill out remainder with spaces
	for (i=key_linepos+1 ; i< con.linewidth ; i++)
		text[i] = ' ';
		
	//	prestep if horizontally scrolling
	if (key_linepos >= con.linewidth)
		text += 1 + key_linepos - con.linewidth;
		
	// draw it
	y = con.vislines-16;
	re.NewDrawString(10, y, 0, con_font, con_fontscale, col_orange, text); // input line
	Con_ShowSuggestions(text + 1);

	// remove cursor
	key_lines[edit_line][key_linepos] = 0;
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	char	*text;
	int		i;
	int		time;
	char	*s;
	int		skip;
	static char str[MAXCMDLINE];
	memset(str, 0, sizeof(str));

	if (cls.state != CS_ACTIVE)
		return;

	v = viddef.height - ((viddef.height / 5));
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;
		
		for (x = 0; x < con.linewidth; x++)
			str[x] = text[x];
		re.NewDrawString(10, v, 0, con_font, con_fontscale, col_white, str);
		v += con_charheight;
	}


	if (cls.key_dest == key_message)
	{
		if (chat_team)
		{
			DrawString (10, v, "Say Team:");
			skip = 11;
		}
		else
		{
			DrawString (10, v, "Say:");
			skip = 5;
		}

		s = chat_buffer;
		if (chat_bufferlen > (viddef.width>>3)-(skip+1))
			s += chat_bufferlen - ((viddef.width>>3)-(skip+1));

		x = 0;
		if(((cls.realtime >> 8) & 1))
			re.NewDrawString((x + skip) << 3, v, 0, con_font, con_fontscale, col_white, va("%s_",s));
		else
			re.NewDrawString((x + skip) << 3, v, 0, con_font, con_fontscale, col_white, s);
		v += con_charheight;
	}
	
	if (v)
	{
		SCR_AddDirtyPoint (0,0);
		SCR_AddDirtyPoint (viddef.width-1, v);
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole (float frac)
{
	int				i, x, y;
	int				rows;
	char			*text;
	int				row;
	int				lines;
	char			version[64];
	char tb[256];

#if 0
	if (con_charsize->modified)
	{
		if (con_charsize->value > 32.0f)
			Cvar_ForceSet("con_charsize", "32");
		else if (con_charsize->value < 8.0f)
			Cvar_ForceSet("con_charsize", "8");
		con_charsize->modified = false;
	}
#endif

	Con_CheckResize();
	lines = viddef.height * frac;
	if (lines <= 0)
		return;

	if (lines > viddef.height)
		lines = viddef.height;


	// draw the background
	re.SetColor(0,0,0,0.8);
	re.DrawFill(0, -viddef.height + lines, viddef.width, viddef.height);
	re.SetColor(1, 1, 1, 1);


	SCR_AddDirtyPoint (0,0);
	SCR_AddDirtyPoint (viddef.width-1,lines-1);

	Com_sprintf (version, sizeof(version), "PRAGMA %s (%s)", PRAGMA_VERSION, PRAGMA_TIMESTAMP);
	re.NewDrawString(viddef.width - 5 , lines - 14, 2, con_font, con_fontscale, col_orange, version);

	// draw the text
	con.vislines = lines;
	
	rows = (lines-22)>>3;		// rows of text to draw

	y = lines - 30;

	// draw from the bottom up
	if (con.display != con.current)
	{
		// draw arrows to show the buffer is backscrolled
		for (x=0 ; x<con.linewidth ; x+=4)
			re.NewDrawString((x + 1) * 16, y, 0, con_font, con_fontscale, col_white, "^");
		y -= con_charheight;
		rows--;
	}
	
	row = con.display;
	for (i=0 ; i<rows ; i++, y -= con_charheight, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines)
			break;		// past scrollback wrap point
			
		text = con.text + (row % con.totallines) * con.linewidth;

		memset(tb, 0, sizeof(tb));
		for (x = 0; x < con.linewidth; x++)
			tb[x] = text[x];

		re.NewDrawString(10, y, 0, con_font, con_fontscale, col_white2, tb);
	}


// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}


