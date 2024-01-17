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
// Main windowed and fullscreen graphics interface module. This module
// is used for OpenGL rendering version of the Quake refresh engine.

#include <assert.h>
#include <float.h>

#include "..\client\client.h"
#include "winquake.h"

// Structure containing functions exported from refresh DLL
refexport_t	re;

cvar_t *win_noalttab;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // message that will be supported by the OS 
#endif

static UINT MSH_MOUSEWHEEL;

// Console variables that we need to access from this module
cvar_t		*r_gamma;
cvar_t		*r_renderer;			// Name of Refresh DLL loaded
cvar_t		*r_xpos;			// X coordinate of window position
cvar_t		*r_ypos;			// Y coordinate of window position
cvar_t		*r_fullscreen;

// Global variables used internally by this module
viddef_t	viddef;				// global video state; used by other modules
HINSTANCE	reflib_library;		// Handle to refresh DLL 
qboolean	reflib_active = 0;

HWND        cl_hwnd;            // Main window handle for life of program

#define VID_NUM_MODES ( sizeof( r_modes ) / sizeof( r_modes[0] ) )

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static qboolean s_alttab_disabled;

extern	unsigned	sys_msg_time;

/*
** WIN32 helper functions
*/
static void WIN_DisableAltTab( void )
{
	if ( s_alttab_disabled )
		return;

	RegisterHotKey( 0, 0, MOD_ALT, VK_TAB );
	RegisterHotKey( 0, 1, MOD_ALT, VK_RETURN );
	s_alttab_disabled = true;
}

static void WIN_EnableAltTab( void )
{
	if ( s_alttab_disabled )
	{
		UnregisterHotKey( 0, 0 );
		UnregisterHotKey( 0, 1 );

		s_alttab_disabled = false;
	}
}

/*
==========================================================================

DLL GLUE

==========================================================================
*/

#define	MAXPRINTMSG	4096
void VID_Printf (int print_level, char *fmt, ...)
{
		va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg),fmt,argptr);
	va_end (argptr);

	printf("%s", msg);

	if (print_level == PRINT_ALL)
	{
		Com_Printf ("%s", msg);
	}
	else if ( print_level == PRINT_DEVELOPER )
	{
		Com_DPrintf (DP_REND,"%s", msg);
	}
	else if ( print_level == PRINT_ALERT )
	{
		MessageBox( 0, msg, "PRINT_ALERT", MB_ICONWARNING );
		OutputDebugString( msg );
	}
}

void VID_Error (int err_level, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	static qboolean	inupdate;
	
	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);

	Com_Error (err_level,"%s", msg);
}

//==========================================================================

byte        scantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6', 
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0 
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i', 
	'o',    'p',    '[',    ']',    13 ,    K_CTRL,'a',  's',      // 1 
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';', 
	'\'' ,    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2 
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*', 
	K_ALT,' ',   0  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0  , K_HOME, 
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
}; 

/*
=======
MapKey

Map from windows to quake keynums
=======
*/
int MapKey (int key)
{
	int result;
	int modified = ( key >> 16 ) & 255;
	qboolean is_extended = false;

	if ( modified > 127)
		return 0;

	if ( key & ( 1 << 24 ) )
		is_extended = true;

	result = scantokey[modified];

	if ( !is_extended )
	{
		switch ( result )
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			return result;
		}
	}
	else
	{
		switch ( result )
		{
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
		return result;
	}
}

void AppActivate(BOOL fActive, BOOL minimize)
{
	Minimized = minimize;

	Key_ClearStates();

	// we don't want to act like we're active if we're minimized
	if (fActive && !Minimized)
		ActiveApp = true;
	else
		ActiveApp = false;

	// minimize/restore mouse-capture on demand
	if (!ActiveApp)
	{
		IN_Activate (false);
		S_Activate (false);

		if ( win_noalttab->value )
		{
			WIN_EnableAltTab();
		}
	}
	else
	{
		IN_Activate (true);
		S_Activate (true);
		if ( win_noalttab->value )
		{
			WIN_DisableAltTab();
		}
	}
}

/*
====================
MainWndProc

main window procedure
====================
*/
LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( uMsg == MSH_MOUSEWHEEL )
	{
		if ( ( ( int ) wParam ) > 0 )
		{
			Key_Event( K_MWHEELUP, true, sys_msg_time );
			Key_Event( K_MWHEELUP, false, sys_msg_time );
		}
		else
		{
			Key_Event( K_MWHEELDOWN, true, sys_msg_time );
			Key_Event( K_MWHEELDOWN, false, sys_msg_time );
		}
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		/*
		** this chunk of code theoretically only works under NT4 and Win98
		** since this message doesn't exist under Win95
		*/
		if ( ( short ) HIWORD( wParam ) > 0 )
		{
			Key_Event( K_MWHEELUP, true, sys_msg_time );
			Key_Event( K_MWHEELUP, false, sys_msg_time );
		}
		else
		{
			Key_Event( K_MWHEELDOWN, true, sys_msg_time );
			Key_Event( K_MWHEELDOWN, false, sys_msg_time );
		}
		break;

	case WM_HOTKEY:
		return 0;

	case WM_CREATE:
		cl_hwnd = hWnd;

		MSH_MOUSEWHEEL = RegisterWindowMessage("MSWHEEL_ROLLMSG"); 
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_PAINT:
		SCR_DirtyScreen ();	// force entire screen to update next frame
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_DESTROY:
		// let sound and input know about this?
		cl_hwnd = NULL;
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_ACTIVATE:
		{
			int	fActive, fMinimized;

			// KJB: Watch this for problems in fullscreen modes with Alt-tabbing.
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);

			AppActivate( fActive != WA_INACTIVE, fMinimized);

			if ( reflib_active )
				re.AppActivate( !( fActive == WA_INACTIVE ) );
		}
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

	case WM_MOVE:
		{
			int		xPos, yPos;
			RECT r;
			int		style;

			if (!r_fullscreen->value)
			{
				xPos = (short) LOWORD(lParam);    // horizontal position 
				yPos = (short) HIWORD(lParam);    // vertical position 

				r.left   = 0;
				r.top    = 0;
				r.right  = 1;
				r.bottom = 1;

				style = GetWindowLong( hWnd, GWL_STYLE );
				AdjustWindowRect( &r, style, FALSE );

				Cvar_SetValue( "r_xpos", xPos + r.left);
				Cvar_SetValue( "r_ypos", yPos + r.top);
				r_xpos->modified = false;
				r_ypos->modified = false;
				if (ActiveApp)
					IN_Activate (true);
			}
		}
        return DefWindowProc (hWnd, uMsg, wParam, lParam);

// this is complicated because Win32 seems to pack multiple mouse events into
// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		{
			int	temp;

			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			IN_MouseEvent (temp);
		}
		break;

	case WM_SYSCOMMAND:
		if ( wParam == SC_SCREENSAVE )
			return 0;
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
	case WM_SYSKEYDOWN:
		if ( wParam == 13 )
		{
			if ( r_fullscreen )
			{
				Cvar_SetValue( "r_fullscreen", !r_fullscreen->value );
			}
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		Key_Event( MapKey( lParam ), true, sys_msg_time);
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key_Event( MapKey( lParam ), false, sys_msg_time);
		break;

	default:	// pass all unhandled messages to DefWindowProc
        return DefWindowProc (hWnd, uMsg, wParam, lParam);
    }

    /* return 0 if handled message, 1 if not */
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh DLL. We do this
simply by setting the modified flag for the vid_ref variable, which will
cause the entire video mode and refresh DLL to be reset on the next frame.
============
*/
void VID_Restart_f (void)
{
	r_renderer->modified = true;
}

void VID_Front_f( void )
{
	SetWindowLong( cl_hwnd, GWL_EXSTYLE, WS_EX_TOPMOST );
	SetForegroundWindow( cl_hwnd );
}

/*
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
	const char *description;
	int         width, height;
	int         mode;
} vidmode_t;

vidmode_t r_modes[] =
{
	{ "Mode 0: 1280x720",  1280, 720,   0 },
	{ "Mode 1: 1366x768",  1366, 768,   1 },
	{ "Mode 2: 1920x1080", 1920, 1080,   2 },
	{ "Mode 3: 640x480",   640, 480,   3 },
	{ "Mode 4: 800x600",   800, 600,   4 },
	{ "Mode 5: 960x720",   960, 720,   5 },
	{ "Mode 6: 1024x768",  1024, 768,  6 },
	{ "Mode 7: 1152x864",  1152, 864,  7 },
	{ "Mode 8: 1280x960",  1280, 960, 8 },
	{ "Mode 9: 1600x1200", 1600, 1200, 9 }
};

qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
	if ( mode < 0 || mode >= VID_NUM_MODES )
		return false;

	*width  = r_modes[mode].width;
	*height = r_modes[mode].height;

	return true;
}

/*
** VID_UpdateWindowPosAndSize
*/
void VID_UpdateWindowPosAndSize( int x, int y )
{
	RECT r;
	int		style;
	int		w, h;

	r.left   = 0;
	r.top    = 0;
	r.right  = viddef.width;
	r.bottom = viddef.height;

	style = GetWindowLong( cl_hwnd, GWL_STYLE );
	AdjustWindowRect( &r, style, FALSE );

	w = r.right - r.left;
	h = r.bottom - r.top;

	MoveWindow(cl_hwnd, x, y, w, h, TRUE);
//	MoveWindow( cl_hwnd, r_xpos->value, r_ypos->value, w, h, TRUE );
}

/*
** VID_NewWindow
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;

	cl.force_refdef = true;		// can't use a paused refdef
}

void VID_FreeReflib (void)
{
	if ( !FreeLibrary( reflib_library ) )
		Com_Error( ERR_FATAL, "Reflib FreeLibrary failed" );
	memset (&re, 0, sizeof(re));
	reflib_library = NULL;
	reflib_active  = false;
}

/*
==============
VID_LoadRefresh
==============
*/
qboolean VID_LoadRefresh( char *name )
{
	refimport_t	ri;
	GetRefAPI_t	GetRefAPI;
	
	if ( reflib_active )
	{
		re.Shutdown();
		VID_FreeReflib ();
	}

	Com_Printf( "------- Loading %s -------\n", name );

	if ( ( reflib_library = LoadLibrary( name ) ) == 0 )
	{
		Com_Printf( "LoadLibrary(\"%s\") failed\n", name );

		return false;
	}

	ri.Cmd_AddCommand = Cmd_AddCommand;
	ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
	ri.Cmd_Argc = Cmd_Argc;
	ri.Cmd_Argv = Cmd_Argv;
	ri.Cmd_ExecuteText = Cbuf_ExecuteText;
	ri.Con_Printf = VID_Printf;
	ri.Sys_Error = VID_Error;
	ri.FS_LoadFile = FS_LoadFile;
	ri.FS_FreeFile = FS_FreeFile;
	ri.FS_Gamedir = FS_Gamedir;
	ri.Cvar_Get = Cvar_Get;
	ri.Cvar_Set = Cvar_Set;
	ri.Cvar_SetValue = Cvar_SetValue;
	ri.Vid_GetModeInfo = VID_GetModeInfo;
	ri.Vid_MenuInit = VID_MenuInit;
	ri.Vid_NewWindow = VID_NewWindow;

	GetRefAPI = (GetRefAPI_t)GetProcAddress(reflib_library, "GetRefAPI");
	if ( (GetRefAPI) == 0 )
		Com_Error( ERR_FATAL, "GetProcAddress failed on %s", name );

	re = GetRefAPI(ri);
	if (re.api_version != API_VERSION)
	{
		VID_FreeReflib ();
		Com_Error (ERR_FATAL, "%s has incompatible api_version", name);
	}

	if ( re.Init( global_hInstance, MainWndProc ) == -1 )
	{
		re.Shutdown();
		VID_FreeReflib ();
		return false;
	}

	Com_Printf( "------------------------------------\n");
	reflib_active = true;


	vidref_val = VIDREF_OTHER;
	if(r_renderer)
	{
		if(!strcmp (r_renderer->string, DEFAULT_RENDERER))
			vidref_val = VIDREF_GL;
	}


	return true;
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	char name[100];

	if ( win_noalttab->modified )
	{
		if ( win_noalttab->value )
		{
			WIN_DisableAltTab();
		}
		else
		{
			WIN_EnableAltTab();
		}
		win_noalttab->modified = false;
	}

	if ( r_renderer->modified )
	{
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds();
	}
	while (r_renderer->modified)
	{
		/*
		** refresh has changed
		*/
		r_renderer->modified = false;
		r_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		Com_sprintf(name, sizeof(name), "renderer_%s.dll", r_renderer->string );
		if ( !VID_LoadRefresh( name ) )
		{
			if ( strcmp (r_renderer->string, DEFAULT_RENDERER) == 0 )
				Com_Error (ERR_FATAL, "Couldn't start %s renderer!", r_renderer->string);
			Cvar_Set( "r_renderer", DEFAULT_RENDERER);

			/*
			** drop the console if we fail to load a refresh
			*/
			if ( cls.key_dest != key_console )
			{
				Con_ToggleConsole_f();
			}
		}
		cls.disable_screen = false;
	}

	/*
	** update our window position
	*/
	if ( r_xpos->modified || r_ypos->modified )
	{
		if (!r_fullscreen->value)
			VID_UpdateWindowPosAndSize( r_xpos->value, r_ypos->value );

		r_xpos->modified = false;
		r_ypos->modified = false;
	}
}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Create the video variables so we know how to start the graphics drivers */
	r_renderer = Cvar_Get ("r_renderer", DEFAULT_RENDERER, CVAR_ARCHIVE);
	r_xpos = Cvar_Get ("r_xpos", "20", CVAR_ARCHIVE);
	r_ypos = Cvar_Get ("r_ypos", "20", CVAR_ARCHIVE);
	r_fullscreen = Cvar_Get ("r_fullscreen", "0", CVAR_ARCHIVE);
	r_gamma = Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
	win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );

	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);
	Cmd_AddCommand ("vid_front", VID_Front_f);
		
	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if ( reflib_active )
	{
		re.Shutdown ();
		VID_FreeReflib ();
	}
}


