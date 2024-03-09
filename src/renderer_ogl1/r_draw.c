/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_draw.c

#include "r_local.h"

enum
{
	FONT_SMALL,
//	FONT_CONSOLE,
//	FONT_CONSOLE_BOLD,
	NUM_FONTS
};

extern vertexbuffer_t vb_gui;
extern int guiVertCount;
extern  glvert_t guiVerts[1024];

extern void ClearVertexBuffer();
extern void PushVert(float x, float y, float z);
extern void SetTexCoords(float s, float t);
extern void SetNormal(float x, float y, float z);
extern void SetColor(float r, float g, float b);

static image_t* font_textures[NUM_FONTS];
image_t* font_current;

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	// load fonts
	font_textures[FONT_SMALL] = R_FindTexture ("gfx/fonts/q2.tga", it_gui);
	if (font_textures[FONT_SMALL] == r_texture_missing)
	{
		ri.Error(ERR_FATAL, "failed to load default font gfx/fonts/q2.tga\n");
		return;
	}

//	font_textures[FONT_CONSOLE] = R_FindTexture("pics/font_console.tga", it_gui);

	for (unsigned int i = 0; i < NUM_FONTS; i++)
	{
//		if (font_textures[i] == r_texture_missing)
//		{
//			font_textures[i] = font_textures[FONT_SMALL]; //fixup
//		}

		R_BindTexture(font_textures[i]->texnum);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	R_BindTexture(0);

	font_current = font_textures[FONT_SMALL];
}



/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);
	PushVert(x + 8, y, 0);
	SetTexCoords(fcol + size, frow);
	PushVert(x + 8, y + 8, 0);
	SetTexCoords(fcol + size, frow + size);
	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);
	PushVert(x + 8, y + 8, 0);
	SetTexCoords(fcol + size, frow + size);
	PushVert(x, y + 8, 0);
	SetTexCoords(fcol, frow + size);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_BindTexture(font_current->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}

/*
=============
Draw_FindPic
=============
*/
image_t	*R_RegisterPic (char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "gfx/%s.tga", name);
		gl = R_FindTexture (fullname, it_gui);
	}
	else
		gl = R_FindTexture (name+1, it_gui);

	return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, char *pic)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (int x, int y, int w, int h, char *pic)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		ri.Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(x + w, y, 0);
	SetTexCoords(gl->sh, gl->tl);
	PushVert(x + w, y + h, 0);
	SetTexCoords(gl->sh, gl->th);
	PushVert(x, y, 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(x + w, y + h, 0);
	SetTexCoords(gl->sh, gl->th);
	PushVert(x, y + h, 0);
	SetTexCoords(gl->sl, gl->th);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_BindTexture(gl->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, char *pic)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		ri.Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(x + gl->width, y, 0);
	SetTexCoords(gl->sh, gl->tl);
	PushVert(x + gl->width, y + gl->height, 0);
	SetTexCoords(gl->sh, gl->th);	
	PushVert(x, y, 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(x + gl->width, y + gl->height, 0);
	SetTexCoords(gl->sh, gl->th);
	PushVert(x, y + gl->height, 0);
	SetTexCoords(gl->sl, gl->th);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_BindTexture(gl->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h, char *pic)
{
	image_t	*image;

	image = R_RegisterPic (pic);
	if (!image)
	{
		ri.Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(x / 64.0f, y / 64.0f);	
	PushVert(x + w, y, 0);
	SetTexCoords((x + w) / 64.0f, y / 64.0f);
	PushVert(x + w, y + h, 0);
	SetTexCoords((x + w) / 64.0f, (y + h) / 64.0f);	
	PushVert(x, y, 0);
	SetTexCoords(x / 64.0f, y / 64.0f);
	PushVert(x + w, y + h, 0);
	SetTexCoords((x + w) / 64.0f, (y + h) / 64.0f);
	PushVert(x, y + h, 0);
	SetTexCoords(x / 64.0f, (y + h) / 64.0f);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_BindTexture(image->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h)
{
	ClearVertexBuffer();
	PushVert(x, y, 0);
	PushVert(x + w, y, 0);
	PushVert(x + w, y + h, 0);
	PushVert(x, y, 0);
	PushVert(x + w, y + h, 0);
	PushVert(x, y + h, 0);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, 0);
	R_BindTexture(r_texture_white->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (float *rgba)
{
	ClearVertexBuffer();
	PushVert(0, 0, 0);
	PushVert(vid.width, 0, 0);
	PushVert(vid.width, vid.height, 0);
	PushVert(0, 0, 0);
	PushVert(vid.width, vid.height, 0);
	PushVert(0, vid.height, 0);
	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, 0);

	R_Blend(true);
	R_BindTexture(r_texture_white->texnum);
	R_ProgUniform4f(LOC_COLOR4, rgba[0], rgba[1], rgba[2], rgba[3]);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
	R_Blend(false);

}


//====================================================================


/*
=============
Draw_StretchRaw
=============
*/

void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{
#if 0

	glTexImage2D (GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBegin(GL_TRIANGLES);
	{
		glTexCoord2f(0, 0); glVertex2f(x, y);
		glTexCoord2f(1, 0); glVertex2f(x + w, y);
		glTexCoord2f(1, t); glVertex2f(x + w, y + h);

		glTexCoord2f(0, 0); glVertex2f(x, y);
		glTexCoord2f(1, t); glVertex2f(x + w, y + h);
		glTexCoord2f(0, t); glVertex2f(x, y + h);
	}
	glEnd();
#endif
}

