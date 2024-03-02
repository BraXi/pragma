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
	font_textures[FONT_SMALL] = GL_FindImage ("gfx/fonts/q2.tga", it_gui);
	if (font_textures[FONT_SMALL] == r_notexture)
	{
		ri.Error(ERR_FATAL, "failed to load default font gfx/fonts/q2.tga\n");
		return;
	}

//	font_textures[FONT_CONSOLE] = GL_FindImage("pics/font_console.tga", it_gui);

	for (unsigned int i = 0; i < NUM_FONTS; i++)
	{
//		if (font_textures[i] == r_notexture)
//		{
//			font_textures[i] = font_textures[FONT_SMALL]; //fixup
//		}

		GL_Bind(font_textures[i]->texnum);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	GL_Bind(0);

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

	GL_Bind (font_current->texnum);

	glBegin(GL_TRIANGLES);
	{
		glTexCoord2f(fcol, frow);
		glVertex2f(x, y);
		glTexCoord2f(fcol + size, frow);
		glVertex2f(x + 8, y);
		glTexCoord2f(fcol + size, frow + size);
		glVertex2f(x + 8, y + 8);

		glTexCoord2f(fcol, frow);
		glVertex2f(x, y);
		glTexCoord2f(fcol + size, frow + size);
		glVertex2f(x + 8, y + 8);
		glTexCoord2f(fcol, frow + size);
		glVertex2f(x, y + 8);
	}
	glEnd();
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
		gl = GL_FindImage (fullname, it_gui);
	}
	else
		gl = GL_FindImage (name+1, it_gui);

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

	GL_Bind (gl->texnum);
	glBegin(GL_TRIANGLES);
	{
		glTexCoord2f(gl->sl, gl->tl);
		glVertex2f(x, y);
		glTexCoord2f(gl->sh, gl->tl);
		glVertex2f(x + w, y);
		glTexCoord2f(gl->sh, gl->th);
		glVertex2f(x + w, y + h);

		glTexCoord2f(gl->sl, gl->tl);
		glVertex2f(x, y);
		glTexCoord2f(gl->sh, gl->th);
		glVertex2f(x + w, y + h);
		glTexCoord2f(gl->sl, gl->th);
		glVertex2f(x, y + h);
	}
	glEnd();

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

	GL_Bind (gl->texnum);
	glBegin(GL_TRIANGLES);
	{
		glTexCoord2f(gl->sl, gl->tl);	glVertex2f(x, y);
		glTexCoord2f(gl->sh, gl->tl);	glVertex2f(x + gl->width, y);
		glTexCoord2f(gl->sh, gl->th);	glVertex2f(x + gl->width, y + gl->height);

		glTexCoord2f(gl->sl, gl->tl);	glVertex2f(x, y);
		glTexCoord2f(gl->sh, gl->th);	glVertex2f(x + gl->width, y + gl->height);
		glTexCoord2f(gl->sl, gl->th);	glVertex2f(x, y + gl->height);
	}
	glEnd();
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

	GL_Bind (image->texnum);
	glBegin(GL_TRIANGLES);
	{
		glTexCoord2f(x / 64.0f, y / 64.0f);				glVertex2f(x, y);
		glTexCoord2f((x + w) / 64.0f, y / 64.0f);		glVertex2f(x + w, y);
		glTexCoord2f((x + w) / 64.0f, (y + h) / 64.0f);	glVertex2f(x + w, y + h);

		glTexCoord2f(x / 64.0f, y / 64.0f);				glVertex2f(x, y);
		glTexCoord2f((x + w) / 64.0f, (y + h) / 64.0f);	glVertex2f(x + w, y + h);
		glTexCoord2f(x / 64.0f, (y + h) / 64.0f);		glVertex2f(x, y + h);
	}
	glEnd();
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h)
{
	glDisable (GL_TEXTURE_2D);

	glBegin(GL_TRIANGLES);
	{
		glVertex2f(x, y);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);

		glVertex2f(x, y);
		glVertex2f(x + w, y + h);
		glVertex2f(x, y + h);
	}
	glEnd();

	glEnable (GL_TEXTURE_2D);
}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (float *rgba)
{
	R_Blend(true);
	glDisable (GL_TEXTURE_2D);
	glAlphaFunc(GL_GREATER, 0.1);
	glColor4fv(rgba);

	glBegin(GL_TRIANGLES);
	{
		glVertex2f(0, 0);
		glVertex2f(vid.width, 0);
		glVertex2f(vid.width, vid.height);

		glVertex2f(0, 0);
		glVertex2f(vid.width, vid.height);
		glVertex2f(0, vid.height);
	}
	glEnd();

	glColor4f(1,1,1,1);
	glAlphaFunc(GL_GREATER, 0.666);
	glEnable (GL_TEXTURE_2D);
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
	unsigned	image32[256*256];

	int			i, j, trows;
	byte		*source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	GL_Bind (0);

	if (rows<=256)
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows/256.0;
		trows = 256;
	}
	t = rows*hscale / 256;


	unsigned *dest;
	for (i=0 ; i<trows ; i++)
	{
		row = (int)(i*hscale);
		if (row > rows)
			break;
		source = data + cols*row;
		dest = &image32[i*256];
		fracstep = cols*0x10000/256;
		frac = fracstep >> 1;
		for (j=0 ; j<256 ; j++)
		{
			dest[j] = r_rawpalette[source[frac>>16]];
			frac += fracstep;
		}
	}

	glTexImage2D (GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	

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

