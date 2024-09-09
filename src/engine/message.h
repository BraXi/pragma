/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================

MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors.

==============================================================
*/

#pragma once

#ifndef _PRAGMA_MESSAGE_H_
#define _PRAGMA_MESSAGE_H_

// TODO: move MSG_WriteDeltaEntity out of here?

void	MSG_WriteDeltaEntity(entity_state_t* from, entity_state_t* to, sizebuf_t* msg, qboolean force, qboolean newentity);

void	MSG_WriteChar(sizebuf_t* sb, int c);
void	MSG_WriteByte(sizebuf_t* sb, int c);
void	MSG_WriteShort(sizebuf_t* sb, int c);
void	MSG_WriteLong(sizebuf_t* sb, int c);
void	MSG_WriteFloat(sizebuf_t* sb, float f);
void	MSG_WriteString(sizebuf_t* sb, char* s);
void	MSG_WriteCoord(sizebuf_t* sb, float f);
void	MSG_WritePos(sizebuf_t* sb, vec3_t pos);
void	MSG_WriteAngle(sizebuf_t* sb, float f);
void	MSG_WriteAngle16(sizebuf_t* sb, float f);
void	MSG_WriteDir(sizebuf_t* sb, vec3_t vector);

void	MSG_BeginReading(sizebuf_t* sb);

int		MSG_ReadChar(sizebuf_t* sb);
int		MSG_ReadByte(sizebuf_t* sb);
int		MSG_ReadShort(sizebuf_t* sb);
int		MSG_ReadLong(sizebuf_t* sb);
float	MSG_ReadFloat(sizebuf_t* sb);
char	*MSG_ReadString(sizebuf_t* sb);
char	*MSG_ReadStringLine(sizebuf_t* sb);
float	MSG_ReadCoord(sizebuf_t* sb);
void	MSG_ReadPos(sizebuf_t* sb, vec3_t pos);
float	MSG_ReadAngle(sizebuf_t* sb);
float	MSG_ReadAngle16(sizebuf_t* sb);
void	MSG_ReadDir(sizebuf_t* sb, vec3_t vector);
void	MSG_ReadData(sizebuf_t* sb, void* buffer, int size);


#if PROTOCOL_FLOAT_COORDS == 1
	int MSG_PackSolid32(const vec3_t mins, const vec3_t maxs);
	void MSG_UnpackSolid32(int packedsolid, vec3_t mins, vec3_t maxs);
#else
	void MSG_UnpackSolid16(int packedsolid, vec3_t bmins, vec3_t bmaxs);
#endif

#endif /*_PRAGMA_MESSAGE_H_*/
