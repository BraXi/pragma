/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#pragma once

/*
==============================================================

CRC

This is a 16 bit, non-reflected CRC using the polynomial 0x1021
and the initial and final xor values shown below..
.  
In other words, the CCITT standard CRC used by XMODEM.

==============================================================
*/



#ifndef _PRAGMA_CRC_H_
#define _PRAGMA_CRC_H_

void CRC_Init(unsigned short* crcvalue);
void CRC_ProcessByte(unsigned short* crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);
unsigned short CRC_Block(byte* start, int count);
unsigned short CRC_ChecksumFile(const char* name, qboolean fatal);

#endif /*_PRAGMA_CRC_H_*/