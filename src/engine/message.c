/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "pragma.h"

vec3_t	bytedirs[NUM_VERTEX_NORMALS] =
{
#include "../common/anorms.h"
};

//
// writing functions
//

void MSG_WriteChar(sizebuf_t* sb, int c)
{
	byte* buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
		Com_Error(ERR_FATAL, "MSG_WriteChar: range error");
#endif

	buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteByte(sizebuf_t* sb, int c)
{
	byte* buf;

#ifdef PARANOID
	if (c < 0 || c > 255)
		Com_Error(ERR_FATAL, "MSG_WriteByte: range error");
#endif

	buf = SZ_GetSpace(sb, 1);
	buf[0] = c;
}

void MSG_WriteShort(sizebuf_t* sb, int c)
{
	byte* buf;

#ifdef PARANOID
	if (c < ((short)0x8000) || c >(short)0x7fff)
		Com_Error(ERR_FATAL, "MSG_WriteShort: range error");
#endif

	buf = SZ_GetSpace(sb, 2);
	buf[0] = c & 0xff;
	buf[1] = c >> 8;
}

void MSG_WriteLong(sizebuf_t* sb, int c)
{
	byte* buf;

	buf = SZ_GetSpace(sb, 4);
	buf[0] = c & 0xff;
	buf[1] = (c >> 8) & 0xff;
	buf[2] = (c >> 16) & 0xff;
	buf[3] = c >> 24;
}

void MSG_WriteFloat(sizebuf_t* sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;


	dat.f = f;
	dat.l = LittleLong(dat.l);

	SZ_Write(sb, &dat.l, 4);
}

void MSG_WriteString(sizebuf_t* sb, const char* s)
{
	if (!s)
		SZ_Write(sb, "", 1);
	else
		SZ_Write(sb, (void*)s, (int)strlen(s) + 1);
}

void MSG_WriteCoord(sizebuf_t* sb, float f)
{
	MSG_WriteFloat(sb, f);
}

void MSG_WritePos(sizebuf_t* sb, vec3_t pos)
{
	MSG_WriteFloat(sb, pos[0]);
	MSG_WriteFloat(sb, pos[1]);
	MSG_WriteFloat(sb, pos[2]);
}

void MSG_WriteAngle(sizebuf_t* sb, float f)
{
	MSG_WriteByte(sb, (int)(f * 256 / 360) & 255);
}

void MSG_WriteAngle16(sizebuf_t* sb, float f)
{
	MSG_WriteShort(sb, ANGLE2SHORT(f));
}


void MSG_WriteDir(sizebuf_t* sb, vec3_t dir)
{
	int		i, best;
	float	d, bestd;

	if (!dir)
	{
		MSG_WriteByte(sb, 0);
		return;
	}

	bestd = 0;
	best = 0;
	for (i = 0; i < NUM_VERTEX_NORMALS; i++)
	{
		d = DotProduct(dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	MSG_WriteByte(sb, best);
}


void MSG_ReadDir(sizebuf_t* sb, vec3_t dir)
{
	int		b;

	b = MSG_ReadByte(sb);
	if (b >= NUM_VERTEX_NORMALS)
		Com_Error(ERR_DROP, "MSG_ReadDir: out of range");
	VectorCopy(bytedirs[b], dir);
}


//
// reading functions
//

void MSG_BeginReading(sizebuf_t* msg)
{
	msg->readcount = 0;
}

// returns -1 if no more characters are available
int MSG_ReadChar(sizebuf_t* msg_read)
{
	int	c;

	if (msg_read->readcount + 1 > msg_read->cursize)
		c = -1;
	else
		c = (signed char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;

	return c;
}

int MSG_ReadByte(sizebuf_t* msg_read)
{
	int	c;

	if (msg_read->readcount + 1 > msg_read->cursize)
		c = -1;
	else
		c = (unsigned char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;

	return c;
}

int MSG_ReadShort(sizebuf_t* msg_read)
{
	int	c;

	if (msg_read->readcount + 2 > msg_read->cursize)
		c = -1;
	else
		c = (short)(msg_read->data[msg_read->readcount]
			+ (msg_read->data[msg_read->readcount + 1] << 8));

	msg_read->readcount += 2;

	return c;
}

int MSG_ReadLong(sizebuf_t* msg_read)
{
	int	c;

	if (msg_read->readcount + 4 > msg_read->cursize)
		c = -1;
	else
		c = msg_read->data[msg_read->readcount]
		+ (msg_read->data[msg_read->readcount + 1] << 8)
		+ (msg_read->data[msg_read->readcount + 2] << 16)
		+ (msg_read->data[msg_read->readcount + 3] << 24);

	msg_read->readcount += 4;

	return c;
}

float MSG_ReadFloat(sizebuf_t* msg_read)
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;

	if (msg_read->readcount + 4 > msg_read->cursize)
		dat.f = -1;
	else
	{
		dat.b[0] = msg_read->data[msg_read->readcount];
		dat.b[1] = msg_read->data[msg_read->readcount + 1];
		dat.b[2] = msg_read->data[msg_read->readcount + 2];
		dat.b[3] = msg_read->data[msg_read->readcount + 3];
	}
	msg_read->readcount += 4;

	dat.l = LittleLong(dat.l);

	return dat.f;
}

char* MSG_ReadString(sizebuf_t* msg_read)
{
	static char	string[2048];
	int		l, c;

	l = 0;
	do
	{
		c = MSG_ReadChar(msg_read);
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string) - 1);

	string[l] = 0;

	return string;
}

char* MSG_ReadStringLine(sizebuf_t* msg_read)
{
	static char	string[2048];
	int		l, c;

	l = 0;
	do
	{
		c = MSG_ReadChar(msg_read);
		if (c == -1 || c == 0 || c == '\n')
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string) - 1);

	string[l] = 0;

	return string;
}

float MSG_ReadCoord(sizebuf_t* msg_read)
{
	return MSG_ReadFloat(msg_read);
}

void MSG_ReadPos(sizebuf_t* msg_read, vec3_t pos)
{
	pos[0] = MSG_ReadFloat(msg_read);
	pos[1] = MSG_ReadFloat(msg_read);
	pos[2] = MSG_ReadFloat(msg_read);
}

float MSG_ReadAngle(sizebuf_t* msg_read)
{
	return MSG_ReadChar(msg_read) * (360.0 / 256);
}

float MSG_ReadAngle16(sizebuf_t* msg_read)
{
	return SHORT2ANGLE(MSG_ReadShort(msg_read));
}

void MSG_ReadData(sizebuf_t* msg_read, void* data, int len)
{
	int		i;

	for (i = 0; i < len; i++)
		((byte*)data)[i] = MSG_ReadByte(msg_read);
}


int MSG_PackSolid32(const vec3_t mins, const vec3_t maxs)
{
	// Q2PRO code
	int x = maxs[0];
	int y = maxs[1];
	int zd = -mins[2];
	int zu = maxs[2] + 32;

	clamp(x, 1, 255);
	clamp(y, 1, 255);
	clamp(zd, 0, 255);
	clamp(zu, 0, 255);

	return MakeLittleLong(x, y, zd, zu);
}

void MSG_UnpackSolid32(int packedsolid, vec3_t mins, vec3_t maxs)
{
	// Q2PRO code
	int x = packedsolid & 255;
	int y = (packedsolid >> 8) & 255;
	int zd = (packedsolid >> 16) & 255;
	int zu = ((packedsolid >> 24) & 255) - 32;

	VectorSet(mins, -x, -y, -zd);
	VectorSet(maxs, x, y, zu);
}