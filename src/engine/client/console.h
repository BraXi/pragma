/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cl_console.h

#ifndef _PRAGMA_CL_CONSOLE_H_
#define _PRAGMA_CL_CONSOLE_H_

#pragma once

#define	NUM_CON_TIMES 8
#define CON_TEXTSIZE 32768

typedef struct
{
	qboolean	initialized;

	char	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int		ormask;			// high bit mask for colored characters

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	cursorspeed;

	int		vislines;

	float	times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
} console_t;

extern console_t con;

void Con_CheckResize (void);
void Con_Init (void);
void Con_DrawConsole (float frac);
void Con_Print (char *txt);
//void Con_CenteredPrint (char *text);
void Con_Clear_f (void);
void Con_DrawNotify (void);
void Con_ClearNotify (void);
void Con_ToggleConsole_f (void);

#endif /*_PRAGMA_CL_CONSOLE_H_*/
