#include "r_local.h"

extern image_t* font_current;

image_t* R_RegisterPic(char* name);

typedef enum
{
	XALIGN_NONE = 0,
	XALIGN_LEFT = 0,
	XALIGN_RIGHT = 1,
	XALIGN_CENTER = 2
} UI_AlignX;

void R_AdjustToVirtualScreenSize(float* x, float* y)
{
	float    xscale;
	float    yscale;

	xscale = vid.width / 800.0;
	yscale = vid.height / 600.0;

	if (x) *x *= xscale;
	if (y) *y *= yscale;
}


void R_DrawSingleChar(float x, float y, float w, float h, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	GL_Bind(font_current->texnum);

	qglBegin(GL_QUADS);
	qglTexCoord2f(fcol, frow);
	qglVertex2f(x, y);
	qglTexCoord2f(fcol + size, frow);
	qglVertex2f(x + w, y);
	qglTexCoord2f(fcol + size, frow + size);
	qglVertex2f(x + w, y + h);
	qglTexCoord2f(fcol, frow + size);
	qglVertex2f(x, y + h);
	qglEnd();
}


void R_DrawString(char* string, float x, float y, float fontSize, int alignx, rgba_t color)
{
	float CHAR_SIZEX = 8 * fontSize;
	float CHAR_SIZEY = 8 * fontSize;
	int ofs_x = 0;

	if (!string)
		return;

	R_AdjustToVirtualScreenSize(&x, &y); // pos
	R_AdjustToVirtualScreenSize(&CHAR_SIZEX, &CHAR_SIZEY); // pos

	// align text
	int strX = (strlen(string) * CHAR_SIZEX);
	if (alignx == XALIGN_CENTER)
		ofs_x -= (strX / 2);
	else if (alignx == XALIGN_RIGHT)
		ofs_x -= ((strlen(string) * CHAR_SIZEX));

	// draw string
	while (*string)
	{
		R_DrawSingleChar(ofs_x + x, y, CHAR_SIZEX, CHAR_SIZEY, *string);
		x += CHAR_SIZEX;
		string++;
	}
}


void R_DrawStretchedImage(rect_t pos, rgba_t color, char* pic)
{
	image_t* gl;

	gl = R_RegisterPic(pic);
	if (!gl)
	{
		ri.Con_Printf(PRINT_ALL, "R_DrawStretchedImage: no %s\n", pic);
		return;
	}

	rect_t rect;
	
	rect[0] = pos[0];
	rect[1] = pos[1];
	rect[2] = pos[2];
	rect[3] = pos[3];

	R_AdjustToVirtualScreenSize(&rect[0], &rect[1]); // pos
	R_AdjustToVirtualScreenSize(&rect[2], &rect[3]); // w/h


	qglEnable(GL_BLEND);
	//qglBlendFunc(GL_ONE, GL_ONE);;
	qglAlphaFunc(GL_GREATER, 0.05);
	qglColor4f(color[0], color[1], color[2], color[3]);

	GL_Bind(gl->texnum);
	qglBegin(GL_QUADS);
	{
		qglTexCoord2f(gl->sl, gl->tl);
		qglVertex2f(rect[0], rect[1]);
		qglTexCoord2f(gl->sh, gl->tl);
		qglVertex2f(rect[0] + rect[2], rect[1]);
		qglTexCoord2f(gl->sh, gl->th);
		qglVertex2f(rect[0] + rect[2], rect[1] + rect[3]);
		qglTexCoord2f(gl->sl, gl->th);
		qglVertex2f(rect[0], rect[1] + rect[3]);
	}
	qglEnd();

	qglDisable(GL_BLEND);
	qglAlphaFunc(GL_GREATER, 0.666);
	qglColor4f(1, 1, 1, 1);

}

void R_DrawFill(rect_t pos, rgba_t color)
{
	rect_t rect;
	rect[0] = pos[0];
	rect[1] = pos[1];
	rect[2] = pos[2];
	rect[3] = pos[3];

	R_AdjustToVirtualScreenSize(&rect[0], &rect[1]); // pos
	R_AdjustToVirtualScreenSize(&rect[2], &rect[3]); // w/h

	qglEnable(GL_BLEND);
	qglDisable(GL_TEXTURE_2D);

	qglAlphaFunc(GL_GREATER, 0.05);
	qglColor4f(color[0], color[1], color[2], color[3]);

	qglBegin(GL_QUADS);
	{
		qglVertex2f(rect[0], rect[1]);
		qglVertex2f(rect[0] + rect[2], rect[1]);
		qglVertex2f(rect[0] + rect[2], rect[1] + rect[3]);
		qglVertex2f(rect[0], rect[1] + rect[3]);
	}
	qglEnd();

	qglDisable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);

	qglAlphaFunc(GL_GREATER, 0.666);
	qglColor4f(1, 1, 1, 1);
}