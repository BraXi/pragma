/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================

cvar_public.h - console variables

Public interface shared with all pragma modules.

Nothing outside the Cvar_*() functions should modify cvar_s!

==============================================================
*/

#pragma once

#ifndef _PRAGMA_CVAR_PUBLIC_H_
#define _PRAGMA_CVAR_PUBLIC_H_

// cvar flags

#define	CVAR_ARCHIVE	1	// set to cause it to be saved to vars.rc
#define	CVAR_USERINFO	2	// added to userinfo  when changed
#define	CVAR_SERVERINFO	4	// added to serverinfo when changed
#define	CVAR_NOSET		8	// don't allow change from console at all, but can be set from the command line
#define	CVAR_LATCH		16	// save changes until server restart
#define	CVAR_CHEAT		32	// set only when cheats are enabled, unused yet


typedef struct cvar_s
{
	char* name;
	char* description; // can be NULL
	char* string;
	char* latched_string;	// for CVAR_LATCH vars
	int			flags;
	qboolean	modified;	// set each time the cvar is changed
	float		value;
	struct cvar_s* next;
} cvar_t;

#endif /*_PRAGMA_CVAR_PUBLIC_H_*/