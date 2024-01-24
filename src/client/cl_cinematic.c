/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "client.h"


typedef struct
{
	int		width;
	int		height;
	byte	*pic;
} cinematics_t;

cinematics_t	cin;


//=============================================================

/*
==================
SCR_StopCinematic
==================
*/
void SCR_StopCinematic (void)
{
	cl.cinematictime = 0;	// done
}

/*
====================
SCR_FinishCinematic

Called when either the cinematic completes, or it is aborted
====================
*/
void SCR_FinishCinematic (void)
{
	// tell the server to advance to the next map / cinematic
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, va("nextserver %i\n", cl.servercount));
}

/*
==================
SCR_ReadNextFrame
==================
*/
byte *SCR_ReadNextFrame (void)
{
	cl.cinematicframe++;
	return NULL;
}


/*
==================
SCR_RunCinematic
==================
*/
void SCR_RunCinematic()
{
	unsigned int frame;
	cl.cinematictime = 0; //bonk.

	if (cl.cinematictime <= 0)
	{
		SCR_StopCinematic ();
		return;
	}

	if (cl.cinematicframe == -1)
		return;		// static image

	if (cls.key_dest != key_game)
	{	// pause if menu or console is up
		cl.cinematictime = cls.realtime - cl.cinematicframe*1000/14;
		return;
	}

	frame = (cls.realtime - cl.cinematictime) * 14 / 1000;
	if (frame <= cl.cinematicframe)
		return;

	if (frame > cl.cinematicframe+1)
	{
		Com_Printf ("Dropped frame: %i > %i\n", frame, cl.cinematicframe+1);
		cl.cinematictime = cls.realtime - cl.cinematicframe*1000/14;
	}
}

/*
==================
SCR_DrawCinematic

Returns true if a cinematic is active, meaning the view rendering should be skipped
==================
*/
qboolean SCR_DrawCinematic (void)
{
	if (cl.cinematictime <= 0)
	{
		return false;
	}

	if (cls.key_dest == key_menu)
	{	// blank screen and pause if menu is up
		return true;
	}

	if (!cin.pic)
		return true;

	re.DrawStretchRaw(0, 0, viddef.width, viddef.height, cin.width, cin.height, cin.pic);
	return true;
}

/*
==================
SCR_PlayCinematic
==================
*/
void SCR_PlayCinematic (char *arg)
{
	char	name[MAX_OSPATH], *dot;

	cl.cinematicframe = 0;

	dot = strstr (arg, ".");

	if (dot && !strcmp (dot, ".tga"))
	{	
		Com_sprintf (name, sizeof(name), "gfx/cin/%s", arg);
		cl.cinematicframe = -1;
		cl.cinematictime = 1;
		SCR_EndLoadingPlaque ();
		cls.state = ca_active;

		if (!cin.pic)
		{
			Com_Printf ("%s not found.\n", name);
			cl.cinematictime = 0;
		}
		return;
	}

}
