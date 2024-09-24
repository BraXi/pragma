/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================

PROTOCOL

Communications protocols between clients and servers

the svc_strings[] array in cl_parse.c should mirror svc_ops_e enum

==============================================================
*/

#pragma once

#ifndef _PRAGMA_PROTOCOL_H_
#define _PRAGMA_PROTOCOL_H_

#define PROTOCOL_REVISION	5
#define	PROTOCOL_VERSION	('B'+'X'+PROTOCOL_REVISION)


// copies of entity_state_t to keep buffered
#define	UPDATE_BACKUP	16	

// must be power of two
#define	UPDATE_MASK		(UPDATE_BACKUP-1)

//
// Server to Client
//
enum svc_ops_e
{
	SVC_BAD,

	SVC_MUZZLEFLASH,
	SVC_TEMP_ENTITY,
	SVC_CGCMD,					// handled in cgame progs: [byte cmd] ....

	SVC_PLAYFX,					// [byte] effect_index, [pos] origin, [dir] direction, [byte] looping 
	SVC_PLAYFXONENT,			// [byte] effect_index, [short] entity_num, [byte] tag_id, [byte] looping 

	SVC_NOP,

	SVC_DISCONNECT,
	SVC_RECONNECT,

	SVC_SOUND,					// <see code>
	SVC_STOPSOUND,				// [byte] entity index

	SVC_PRINT,					// [byte] id [string] null terminated string

	SVC_STUFFTEXT,				// [string] stuffed into client's console buffer, should be \n terminated

	SVC_SERVERDATA,				// [long] protocol ...
	SVC_CONFIGSTRING,			// [short] [string]
	SVC_SPAWNBASELINE,

	SVC_CENTERPRINT,			// [string] to put in center of the screen

	SVC_DOWNLOAD,				// [short] size [size bytes]

	SVC_PLAYERINFO,				// variable

	SVC_PACKET_ENTITIES,		// [...]
	SVC_DELTA_PACKET_ENTITIES,	// [...]

	SVC_FRAME
};


//
// Client to Server
//
enum clc_ops_e
{
	clc_bad,
	clc_nop,
	clc_move,				// [[usercmd_t]
	clc_userinfo,			// [[userinfo string]
	clc_stringcmd			// [string] message
};


//
// Player State Update Flags - plyer_state_t communication
//
#define	PS_M_TYPE			(1<<0)
#define	PS_M_ORIGIN			(1<<1)
#define	PS_M_VELOCITY		(1<<2)
#define	PS_M_TIME			(1<<3)
#define	PS_M_FLAGS			(1<<4)
#define	PS_M_GRAVITY		(1<<5)
#define	PS_M_DELTA_ANGLES	(1<<6)

#define	PS_VIEWOFFSET		(1<<7)
#define	PS_VIEWANGLES		(1<<8)
#define	PS_KICKANGLES		(1<<9)
#define	PS_EFFECTS			(1<<10)
#define	PS_FOV				(1<<11)
#define	PS_VIEWMODEL_INDEX	(1<<12)
#define	PS_VIEWMODEL_PARAMS	(1<<13)
#define	PS_RDFLAGS			(1<<14)

#define PS_EXTRABYTES		(1<<15) // reki -- 28-12-23 added this so we don't shoot ourselves in the foot later, looking for more bytes
#define	PS_M_BBOX_SIZE		(1<<16) // braxi -- added bbox size, maybe make it byte?
#define PS_M_FLAGSLONG		(1<<17) // reki -- 28-12-23 if our mod is using a lot of pmove flags, make sure they're networked

#define	PS_FX_BLEND			(1<<0)
#define	PS_FX_BLUR			(1<<1)
#define	PS_FX_CONTRAST		(1<<2)
#define	PS_FX_GRAYSCALE		(1<<3)
#define	PS_FX_INVERSE		(1<<4)
#define	PS_FX_INTENSITY		(1<<5)
#define	PS_FX_NOISE			(1<<6)


//==============================================

// user_cmd_t communication

// ms and light always sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE2 	(1<<1)
#define	CM_ANGLE3 	(1<<2)
#define	CM_FORWARD	(1<<3)
#define	CM_SIDE		(1<<4)
#define	CM_UP		(1<<5)
#define	CM_BUTTONS	(1<<6)
#define	CM_IMPULSE	(1<<7)

//==============================================

// a sound without an ent or pos will be a local only sound
#define	SND_VOLUME		(1<<0)		// a byte
#define	SND_ATTENUATION	(1<<1)		// a byte
#define	SND_POS			(1<<2)		// three coordinates
#define	SND_ENT			(1<<3)		// a short 0-2: channel, 3-12: entity
#define	SND_OFFSET		(1<<4)		// a byte, msec offset from frame start
#define	SND_INDEX_16	(1<<5)		// soundnum is > 255

#define DEFAULT_SOUND_PACKET_VOLUME	1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//==============================================

//
// entity_state_t communication
// 
// _8 = sent as byte
// _16 = sent as short
// try to pack the common update flags into the first byte
#define	U_ETYPE				(1<<0)		// entity type
#define	U_ORIGIN_XY			(1<<1)		// current origin
#define	U_ANGLE_X			(1<<2)		// current angles
#define	U_ANGLE_Y			(1<<3)		// current angles
#define	U_ANIMFRAME_8		(1<<4)		// anim frame is 0-255
#define	U_EVENT_8			(1<<5)		// byte
#define	U_REMOVE			(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS_1		(1<<7)		// -- read one additional byte --

// second byte
#define	U_NUMBER_16			(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN_Z			(1<<9)
#define	U_ANGLE_Z			(1<<10)
#define	U_MODELINDEX_16		(1<<11)		// modelindex
#define U_RENDERFLAGS_8		(1<<12)		// fullbright, etc
#define U_FREE_FLAG			(1<<13)		
#define	U_EFFECTS_8			(1<<14)		// effects - EF_ flags
#define	U_MOREBITS_2		(1<<15)		// -- read one additional byte --

// third byte
#define	U_SKIN_8			(1<<16)		// TODO: index to entry in .skin file
#define	U_ANIMFRAME_16		(1<<17)		// animframe greater than 255
#define	U_RENDERFLAGS_16	(1<<18)		// 8 + 16 = 32B
#define	U_EFFECTS_16		(1<<19)		// effects - EF_ flags, 8 + 16 = 32
#define	U_ATTACHMENT_1		(1<<20)		// model attached to a tag
#define	U_ATTACHMENT_2		(1<<21)		// model attached to a tag
#define	U_ATTACHMENT_3		(1<<22)		// model attached to a tag
#define	U_MOREBITS_3		(1<<23)		// -- read one additional byte --

// fourth byte
#define	U_OLDORIGIN			(1<<24)		// id: FIXME: get rid of this, braxi: WHY?
#define	U_ANIMATION			(1<<25)
#define	U_LOOPSOUND			(1<<26)		// byte/short, index to sounds
#define	U_PACKEDSOLID		(1<<27)
#define	U_RENDERSCALE		(1<<28)
#define	U_RENDERALPHA		(1<<29)
#define	U_RENDERCOLOR		(1<<30)

#define	U_FREE4				(1<<31)


#endif /*_PRAGMA_PROTOCOL_H_*/
