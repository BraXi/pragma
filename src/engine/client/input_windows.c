/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// win_input.c -- windows 95 mouse code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "client.h"
#include "../../common/pragma_windows.h"

extern	unsigned	sys_msg_time;

cvar_t	*in_mouse;


qboolean	in_appactive;


/*
============================================================

  MOUSE CONTROL

============================================================
*/

// mouse variables
cvar_t	*m_filter;

qboolean	mlooking;

void IN_MLookDown (void) 
{ 
	mlooking = true; 
}


void IN_MLookUp (void) 
{
	mlooking = false;
	if (!cl_freelook->value && lookspring->value)
			IN_CenterView ();
}

static int mouse_buttons;
static int mouse_oldbuttonstate;
static int mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;
static POINT current_pos;


static int originalmouseparms[3];
static int newmouseparms[3] = { 0, 0, 1 };;

static qboolean bMouseActive;	// false when not focus app
static qboolean bRestoreMouseSPI;
static qboolean bMouseInitialized;
static qboolean	bMouseParamsValid;

static int window_center_x, window_center_y;
static int window_center_x_old, window_center_y_old;
static RECT window_rect;


/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse (void)
{
	int width, height;

	if (!bMouseInitialized)
	{
		return;
	}

	if (!in_mouse->value)
	{
		bMouseActive = false;
		return;
	}

	if (bMouseActive)
	{
		return;
	}

	bMouseActive = true;

	if (bMouseParamsValid)
		bRestoreMouseSPI = SystemParametersInfo(SPI_SETMOUSE, 0, newmouseparms, 0);

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(cl_hwnd, &window_rect);
	if (window_rect.left < 0)
		window_rect.left = 0;
	if (window_rect.top < 0)
		window_rect.top = 0;
	if (window_rect.right >= width)
		window_rect.right = width-1;
	if (window_rect.bottom >= height-1)
		window_rect.bottom = height-1;

	window_center_x = (window_rect.right + window_rect.left)/2;
	window_center_y = (window_rect.top + window_rect.bottom)/2;

	SetCursorPos (window_center_x, window_center_y);

	window_center_x_old = window_center_x;
	window_center_y_old = window_center_y;

	SetCapture ( cl_hwnd );
	ClipCursor (&window_rect);
	while (ShowCursor (FALSE) >= 0)
		;
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse (void)
{
	if (!bMouseInitialized)
	{
		return;
	}

	if (!bMouseActive)
	{
		return;
	}

	if (bRestoreMouseSPI)
	{
		SystemParametersInfo(SPI_SETMOUSE, 0, originalmouseparms, 0);
	}

	bMouseActive = false;

	ClipCursor (NULL);
	ReleaseCapture ();

	// evil..
	while (ShowCursor (TRUE) < 0)
		;
}



/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse (void)
{
	cvar_t		*cv;

	cv = Cvar_Get ("in_initmouse", "1", CVAR_NOSET, "Enable mouse.");
	if ( !cv->value ) 
		return; 

	bMouseInitialized = true;
	bMouseParamsValid = SystemParametersInfo (SPI_GETMOUSE, 0, originalmouseparms, 0);
	mouse_buttons = 3;
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent(int mstate)
{
	int		i;

	if (!bMouseInitialized)
		return;

// perform button actions
	for (i = 0 ; i < mouse_buttons; i++)
	{
		if ( (mstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, true, sys_msg_time);
		}

		if ( !(mstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
		{
				Key_Event (K_MOUSE1 + i, false, sys_msg_time);
		}
	}	
		
	mouse_oldbuttonstate = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	int		mx, my;

	if (!bMouseActive)
	{
		return;
	}

	// find mouse movement
	if (!GetCursorPos (&current_pos))
	{
		return;
	}

	mx = current_pos.x - window_center_x;
	my = current_pos.y - window_center_y;

#if 0
	if (!mx && !my)
		return;
#endif

	if (m_filter->value)
	{
		mouse_x = (mx + old_mouse_x) * 0.5;
		mouse_y = (my + old_mouse_y) * 0.5;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	mouse_x *= sensitivity->value;
	mouse_y *= sensitivity->value;

// add mouse X/Y movement to cmd
	if ( (in_strafe.state & 1) || (lookstrafe->value && mlooking ))
		cmd->sidemove += m_side->value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if ( (mlooking || cl_freelook->value) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch->value * mouse_y;
	}
	else
	{
		cmd->forwardmove -= m_forward->value * mouse_y;
	}

	// force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos (window_center_x, window_center_y);
}


/*
=========================================================================

VIEW CENTERING

=========================================================================
*/

cvar_t	*v_centermove;
cvar_t	*v_centerspeed;


/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	// mouse variables
	m_filter				= Cvar_Get ("m_filter",					"0",		0, NULL);
    in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE, "Enable mouse.");

	// centering
	v_centermove			= Cvar_Get ("v_centermove",				"0.15",		0, NULL);
	v_centerspeed			= Cvar_Get ("v_centerspeed",			"500",		0, NULL);

	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	IN_StartupMouse ();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active)
{
	in_appactive = active;
	bMouseActive = !active;		// force a new window check or turn off
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void)
{
	if (!bMouseInitialized)
		return;

	if (!in_mouse || !in_appactive)
	{
		IN_DeactivateMouse ();
		return;
	}

	if ( !cl.refresh_prepped || cls.key_dest == key_console || cls.key_dest == key_menu)
	{
		// temporarily deactivate if in fullscreen
		if (Cvar_VariableValue ("r_fullscreen") == 0)
		{
			IN_DeactivateMouse ();
			return;
		}
	}

	IN_ActivateMouse ();
}

/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove (cmd);
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
{
	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}



/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
}