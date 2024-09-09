/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "pragma.h"

void SZ_Init(sizebuf_t* buf, byte* data, const int length)
{
	memset(buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

void SZ_Clear(sizebuf_t* buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void* SZ_GetSpace(sizebuf_t* buf, const int length)
{
	void* data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Com_Error(ERR_FATAL, "SZ_GetSpace: overflow without allowoverflow set");

		if (length > buf->maxsize)
			Com_Error(ERR_FATAL, "SZ_GetSpace: %i is > full buffer size", length);

		Com_Printf("SZ_GetSpace: overflow\n");
		SZ_Clear(buf);
		buf->overflowed = true;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write(sizebuf_t* buf, void* data, const int length)
{
	memcpy(SZ_GetSpace(buf, length), data, length);
}

void SZ_Print(sizebuf_t* buf, char* data)
{
	int		len;

	len = (int)strlen(data) + 1;

	if (buf->cursize)
	{
		if (buf->data[buf->cursize - 1])
			memcpy((byte*)SZ_GetSpace(buf, len), data, len); // no trailing 0
		else
			memcpy((byte*)SZ_GetSpace(buf, len - 1) - 1, data, len); // write over trailing 0
	}
	else
		memcpy((byte*)SZ_GetSpace(buf, len), data, len);
}
