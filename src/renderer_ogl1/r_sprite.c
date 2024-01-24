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

	psprite = (sp2Header_t*)currentmodel->extradata;

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

	qglColor4f(1, 1, 1, currententity->alpha);

	GL_Bind(currentmodel->skins[ent->frame]->texnum);
	GL_TexEnv(GL_MODULATE);

//	if (ent->alpha == 1.0)
//		qglEnable(GL_ALPHA_TEST);
//	else
//		qglDisable(GL_ALPHA_TEST);
	R_AlphaTest(ent->alpha == 1.0);

	qglBegin(GL_QUADS);

	qglTexCoord2f(0, 1);
	VectorMA(vec3_origin, -frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(0, 0);
	VectorMA(vec3_origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(1, 0);
	VectorMA(vec3_origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(1, 1);
	VectorMA(vec3_origin, -frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	qglVertex3fv(point);

	qglEnd();

	R_AlphaTest(false);
}