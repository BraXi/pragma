/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_win.h

#include "../qcommon/qcommon.h"

#ifdef DEDICATED_ONLY
	#include <windows.h>
#else
	#include "winquake.h"
	#include "resource.h"
#endif

#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#ifndef DEDICATED_ONLY
int			ActiveApp;
qboolean	Minimized;
#endif /*DEDICATED_ONLY*/

static HANDLE		hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;

#define	MAX_NUM_ARGVS	128
int		argc;
char	*argv[MAX_NUM_ARGVS];

/*
===============================================================================

SYSTEM IO

===============================================================================
*/


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

#ifndef DEDICATED_ONLY
	CL_Shutdown ();
#endif

	Qcommon_Shutdown ();

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

#ifdef DEDICATED_ONLY
	printf("Sys_Error: %s\n", text);
#else
	MessageBox(NULL, text, "Error", 0 /* MB_OK */ );
#endif

	exit (1);
}

void Sys_Quit (void)
{
	timeEndPeriod( 1 );

#ifndef DEDICATED_ONLY
	CL_Shutdown();
#endif

	Qcommon_Shutdown ();

#ifndef DEDICATED_ONLY
	if (dedicated && dedicated->value)
		FreeConsole ();
#endif

	exit (0);
}


//================================================================



//================================================================

#pragma warning( disable : 4996 )
/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

	timeBeginPeriod( 1 );

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Sys_Error ("pragma requires windows version 4 or greater");
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("pragma doesn't run on Win32s");

	if (dedicated->value)
	{
#ifndef DEDICATED_ONLY
		if (!AllocConsole ())
			Sys_Error ("Couldn't create dedicated server console");
#endif
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	}
}


static char	console_text[256];
static int	console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD	recs[1024];
	int		ch;
	DWORD dummy, numread, numevents;

	if (!dedicated || !dedicated->value)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;
			
				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);	
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}

						break;

				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	DWORD	dummy;
	char	text[256];

#ifndef DEDICATED_ONLY
	if (!dedicated || !dedicated->value)
		return;
#endif

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_msg_time = msg.time;
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// grab frame time 
	sys_frame_time = timeGetTime();	// FIXME: should this be at start?
}



/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) 
			{
				data = malloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
#ifndef DEDICATED_ONLY
void Sys_AppActivate (void)
{
	ShowWindow ( cl_hwnd, SW_RESTORE);
	SetForegroundWindow ( cl_hwnd );
}
#endif
/*
==================
ParseCommandLine
==================
*/
#ifndef DEDICATED_ONLY
void ParseCommandLine (LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

}
#endif

#ifdef DEDICATED_ONLY
/*
==================
main

entry point for dedicated server
==================
*/
int main(int inargc, char** inargv)
{
	int		time, oldtime, newtime;

	Qcommon_Init(inargc, inargv);
	oldtime = Sys_Milliseconds();

	/* main window message loop */
	while (1)
	{
		Sleep(1);

		do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while (time < 1);

#ifndef _M_X64
		_controlfp(_PC_24, _MCW_PC);
#endif

		Qcommon_Frame(time);

		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
#else

/*
==================
WinMain

==================
*/
HINSTANCE	global_hInstance;
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG		msg;
	int		time, oldtime, newtime;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
		return 0;

	global_hInstance = hInstance;
	ParseCommandLine(lpCmdLine);

	Qcommon_Init(argc, argv);
	oldtime = Sys_Milliseconds();

	if (Cvar_VariableValue("debugcon"))
	{
		HWND consoleHandle;
		AllocConsole();
		freopen("conin$", "r", stdin);
		freopen("conout$", "w", stdout);
		freopen("conout$", "w", stderr);
		consoleHandle = GetConsoleWindow();
		MoveWindow(consoleHandle, 1, 1, 680, 480, 1);
		printf("[debugcon enabled]\n");
	}

	/* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->value))
		{
			Sleep(1);
		}

		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
				Com_Quit();
			sys_msg_time = msg.time;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while (time < 1);

		_controlfp(_PC_24, _MCW_PC);
		Qcommon_Frame(time);

		oldtime = newtime;
	}

	// never gets here
	return TRUE;
}
#endif