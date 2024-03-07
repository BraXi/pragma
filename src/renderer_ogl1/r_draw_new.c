/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

extern image_t* font_current;

image_t* R_RegisterPic(char* name);

vertexbuffer_t vb_gui;

int guiVertCount;
glvert_t guiVerts[1024];

void ClearVertexBuffer()
{
	guiVertCount = 0;
}

void PushVert(float x, float y, float z)
{
	VectorSet(guiVerts[guiVertCount].xyz, x, y, z);
	guiVertCount++;
}

void SetTexCoords(float s, float t)
{
	guiVerts[guiVertCount - 1].st[0] = s;
	guiVerts[guiVertCount - 1].st[1] = t;
}

void SetNormal(float x, float y, float z)
{
	VectorSet(guiVerts[guiVertCount - 1].normal, x, y, z);
}

void SetColor(float r, float g, float b)
{
	VectorSet(guiVerts[guiVertCount - 1].rgb, r, g, b);
}


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

	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + w, y, 0);
	SetTexCoords(fcol + size, frow);

	PushVert(x + w, y + h, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + w, y + h, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x, y + h, 0);
	SetTexCoords(fcol, frow + size);
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

	CHAR_SIZEX = CHAR_SIZEY = (int)CHAR_SIZEX; // hack so text isnt blurry

	// align text
	int strX = (strlen(string) * CHAR_SIZEX);
	if (alignx == XALIGN_CENTER)
		ofs_x -= (strX / 2);
	else if (alignx == XALIGN_RIGHT)
		ofs_x -= ((strlen(string) * CHAR_SIZEX));

	ClearVertexBuffer();
	while (*string)
	{
		R_DrawSingleChar(ofs_x + x, y, CHAR_SIZEX, CHAR_SIZEY, *string);
		x += CHAR_SIZEX;
		string++;
	}

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);

	glColor4fv(color);
	R_BindTexture(font_current->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}

void R_DrawSingleChar3D(float x, float y, float z, float fontSize, int num)
{
	int row, col;
	float frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return; // space

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	PushVert(x + (-fontSize / 2), -fontSize / 2, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + (fontSize / 2), -fontSize / 2, 0);
	SetTexCoords(fcol + size, frow);

	PushVert(x + (fontSize / 2), fontSize / 2, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x + (-fontSize / 2), -fontSize / 2, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + (fontSize / 2), fontSize / 2, 0);
	SetTexCoords(fcol + size, frow + size);
	
	PushVert(x + (-fontSize / 2), fontSize / 2, 0);
	SetTexCoords(fcol, frow + size);
}

void R_DrawString3D(char* string, vec3_t pos, float fontSize, int alignx, vec3_t color)
{
	// this does support more than the code uses, leaving for future use
	float CHAR_SIZEX = 8 * fontSize;
	float CHAR_SIZEY = 8 * fontSize;
	int ofs_x = 0;

	if (!string || !string[0] | !strlen(string))
		return;

	alignx = XALIGN_CENTER;
	// align text
	int strX = (strlen(string) * CHAR_SIZEX);
	if (alignx == XALIGN_CENTER)
		ofs_x -= (strX / 2);
	else if (alignx == XALIGN_RIGHT)
		ofs_x -= ((strlen(string) * CHAR_SIZEX));

	glPushMatrix();
	glTranslatef(pos[0], pos[1], pos[2]);
	glRotatef(-90, 1.0f, 0.0f, 0.0f); 
	glRotatef((-r_newrefdef.view.angles[1])+90, 0.0f, 1.0f, 0.0f); //rotate to always face the camera
	glRotatef((-r_newrefdef.view.angles[0]), 1.0f, 0.0f, 0.0f);

	// draw string
	ClearVertexBuffer();
	while (*string)
	{
		R_DrawSingleChar3D(ofs_x, pos[1], pos[2], CHAR_SIZEX, *string);
		ofs_x += CHAR_SIZEX;
		string++;
	}

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	glColor3fv(color);
	R_BindTexture(font_current->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);

	glPopMatrix();
}

void R_DrawStretchedImage(rect_t pos, rgba_t color, char* pic)
{
	image_t* gl;

	gl = R_RegisterPic(pic);
	if (!gl)
	{
		ri.Printf(PRINT_ALL, "R_DrawStretchedImage: no %s\n", pic);
		return;
	}

	rect_t rect;
	
	rect[0] = pos[0];
	rect[1] = pos[1];
	rect[2] = pos[2];
	rect[3] = pos[3];

	R_AdjustToVirtualScreenSize(&rect[0], &rect[1]); // pos
	R_AdjustToVirtualScreenSize(&rect[2], &rect[3]); // w/h

	ClearVertexBuffer();

	PushVert(rect[0], rect[1], 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(rect[0] + rect[2], rect[1], 0);
	SetTexCoords(gl->sh, gl->tl);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	SetTexCoords(gl->sh, gl->th);
	
	PushVert(rect[0], rect[1], 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	SetTexCoords(gl->sh, gl->th);
	PushVert(rect[0], rect[1] + rect[3], 0);
	SetTexCoords(gl->sl, gl->th);

	R_Blend(true);
//	glAlphaFunc(GL_GREATER, 0.05);
	glColor4f(color[0], color[1], color[2], color[3]);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_BindTexture(gl->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);

	R_Blend(false);
//	glAlphaFunc(GL_GREATER, 0.1);
	glColor4f(1, 1, 1, 1);

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

	ClearVertexBuffer();
	PushVert(rect[0], rect[1], 0);
	PushVert(rect[0] + rect[2], rect[1], 0);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	PushVert(rect[0], rect[1], 0);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	PushVert(rect[0], rect[1] + rect[3], 0);

	R_Blend(true);
	glDisable(GL_TEXTURE_2D);

//	glAlphaFunc(GL_GREATER, 0.05);
	glColor4f(color[0], color[1], color[2], color[3]);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, 0);
	R_DrawVertexBuffer(&vb_gui, 0, 0);

	R_Blend(false);
	glEnable(GL_TEXTURE_2D);

//	glAlphaFunc(GL_GREATER, 0.1);
	glColor4f(1, 1, 1, 1);
}