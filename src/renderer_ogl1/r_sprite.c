/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================================
QUAKE2 SPRITES
==============================================================================
*/

#include "r_local.h"

void R_DrawSprite(rentity_t* ent)
{
	vec3_t	point;
	sp2Frame_t* frame;
	float* up, * right;
	sp2Header_t* psprite;

	psprite = (sp2Header_t*)pCurrentModel->extradata;

	// advance frame
	ent->frame %= psprite->numframes;

	frame = &psprite->frames[ent->frame];

#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		vec3_t		v_forward, v_right, v_up;

		AngleVectors(currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = vup;
		right = vright;
	}

	glColor4f(1, 1, 1, pCurrentRefEnt->alpha);

	GL_Bind(pCurrentModel->skins[ent->frame]->texnum);
	GL_TexEnv(GL_MODULATE);

//	if (ent->alpha == 1.0)
//		glEnable(GL_ALPHA_TEST);
//	else
//		glDisable(GL_ALPHA_TEST);
	R_AlphaTest(ent->alpha == 1.0);

	glBegin(GL_QUADS);

	glTexCoord2f(0, 1);
	VectorMA(vec3_origin, -frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	glVertex3fv(point);

	glTexCoord2f(0, 0);
	VectorMA(vec3_origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	glVertex3fv(point);

	glTexCoord2f(1, 0);
	VectorMA(vec3_origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	glVertex3fv(point);

	glTexCoord2f(1, 1);
	VectorMA(vec3_origin, -frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	glVertex3fv(point);

	glEnd();

	R_AlphaTest(false);
}