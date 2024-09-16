/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// input.h -- external (non-keyboard) input devices

#ifndef _PRAGMA_INPUT_H_
#define _PRAGMA_INPUT_H_

void IN_Init (void);

void IN_Shutdown (void);

void IN_Commands (void);
// oportunity for devices to stick commands on the script buffer

void IN_Frame (void);

void IN_Move (usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd

void IN_Activate (qboolean active);

#endif /*_PRAGMA_INPUT_H_*/
