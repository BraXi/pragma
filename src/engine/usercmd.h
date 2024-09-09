/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================

User Commands

==============================================================
*/

#pragma once

#ifndef _PRAGMA_USERCMD_H_
#define _PRAGMA_USERCMD_H_


void MSG_WriteDeltaUsercmd(sizebuf_t* buf, usercmd_t* from, usercmd_t* cmd);
void MSG_ReadDeltaUsercmd(sizebuf_t* msg_read, usercmd_t* from, usercmd_t* move);


#endif /*_PRAGMA_USERCMD_H_*/