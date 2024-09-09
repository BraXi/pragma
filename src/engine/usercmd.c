/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../qcommon/qcommon.h"

void MSG_WriteDeltaUsercmd(sizebuf_t* buf, usercmd_t* from, usercmd_t* cmd)
{
	int		bits;

	//
	// send the movement message
	//
	bits = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

	MSG_WriteByte(buf, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteShort(buf, cmd->angles[0]);
	if (bits & CM_ANGLE2)
		MSG_WriteShort(buf, cmd->angles[1]);
	if (bits & CM_ANGLE3)
		MSG_WriteShort(buf, cmd->angles[2]);

	if (bits & CM_FORWARD)
		MSG_WriteShort(buf, cmd->forwardmove);
	if (bits & CM_SIDE)
		MSG_WriteShort(buf, cmd->sidemove);
	if (bits & CM_UP)
		MSG_WriteShort(buf, cmd->upmove);

	if (bits & CM_BUTTONS)
		MSG_WriteByte(buf, cmd->buttons);
	if (bits & CM_IMPULSE)
		MSG_WriteByte(buf, cmd->impulse);

	MSG_WriteByte(buf, cmd->msec);
}

void MSG_ReadDeltaUsercmd(sizebuf_t* msg_read, usercmd_t* from, usercmd_t* move)
{
	int bits;

	memcpy(move, from, sizeof(*move));

	bits = MSG_ReadByte(msg_read);

	// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadShort(msg_read);
	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadShort(msg_read);
	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadShort(msg_read);

	// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadShort(msg_read);
	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadShort(msg_read);
	if (bits & CM_UP)
		move->upmove = MSG_ReadShort(msg_read);

	// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadByte(msg_read);

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte(msg_read);

	// read time to run command
	move->msec = MSG_ReadByte(msg_read);
}