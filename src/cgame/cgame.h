/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

extern void CL_ShutdownClientGame();
extern void CL_InitClientGame();

extern void CG_Frame(float frametime, int time, float realtime);
extern void CG_DrawGUI();

extern qboolean CG_CanDrawCall();

extern void CG_ServerCommand();

#define AF_NOLERP 1
#define AF_LOOPING 2

typedef struct
{
	int		soundindex;
	int		effects;
} anim_event_t;

typedef struct
{
	char			name[MAX_QPATH];

	int				startframe;
	int				numframes;
	int				rate;

	int				flags;

	int				numevents;
	anim_event_t	*events; // [numevents]
} anim_t;

typedef struct
{
	unsigned int starttime;	//sv.time when animation started

	int		startframe;
	int		numframes;
	int		rate;

	int		flags;
} animstate_t;