/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_mesh.c: triangle model functions

#include "r_local.h"

// globals for model lighting
#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] =
{
#include "anorms.h"
};

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anormtab.h"
;

float	*model_shadedots = r_avertexnormal_dots[0];
vec3_t	model_shadevector;
float	model_shadelight[3];


qboolean R_MD2_CullModel(vec3_t bbox[8], centity_t* e);

/*
=================
R_EntityShouldRender

decides whenever entity is visible and should be drawn
=================
*/
qboolean R_EntityShouldRender(centity_t* ent)
{
	vec3_t	bbox[8];

	if (ent->alpha <= 0.0f && (ent->flags & RF_TRANSLUCENT))
		return false; // completly transparent

	if (ent->model)
	{
		if (!(ent->flags & RF_WEAPONMODEL))
		{
			if (ent->model->type == MOD_MD2 && R_MD2_CullModel(bbox, ent))
				return false;
			else if (ent->model->type == MOD_MD3)
				return true; //todo

			return true;
		}

	}

	// if it's a weapon model in "center" we don't draw it
	if (ent->flags & RF_WEAPONMODEL)
	{
		if (r_lefthand->value == 2)
			return false;
	}

	return true;
}

/*
=================
R_SetEntityShadeLight

get lighting information for centity
=================
*/
void R_SetEntityShadeLight(centity_t* ent)
{
	float	scale;
	float	min;
	float	an;
	int		i;

	if (ent->flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE))
	{
		// special case for godmode
		if ((ent->flags & RF_SHELL_RED) && (ent->flags & RF_SHELL_BLUE) && (ent->flags & RF_SHELL_GREEN))
		{
			for (i = 0; i < 3; i++)
				model_shadelight[i] = 1.0;
		}
		else if (ent->flags & (RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE))
		{
			VectorClear(model_shadelight);

			if (ent->flags & RF_SHELL_RED)
			{
				model_shadelight[0] = 1.0;
				if (ent->flags & (RF_SHELL_BLUE | RF_SHELL_DOUBLE))
					model_shadelight[2] = 1.0;
			}
			else if (ent->flags & RF_SHELL_BLUE)
			{
				if (ent->flags & RF_SHELL_DOUBLE)
				{
					model_shadelight[1] = 1.0;
					model_shadelight[2] = 1.0;
				}
				else
				{
					model_shadelight[2] = 1.0;
				}
			}
			else if (ent->flags & RF_SHELL_DOUBLE)
			{
				model_shadelight[0] = 0.9;
				model_shadelight[1] = 0.7;
			}
		}
		else if (ent->flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN))
		{
			VectorClear(model_shadelight);
			// PMM - new colors
			if (ent->flags & RF_SHELL_HALF_DAM)
			{
				model_shadelight[0] = 0.56;
				model_shadelight[1] = 0.59;
				model_shadelight[2] = 0.45;
			}
			if (ent->flags & RF_SHELL_GREEN)
			{
				model_shadelight[1] = 1.0;
			}
		}
	}
	else if (currententity->flags & RF_FULLBRIGHT || r_fullbright->value)
	{
		for (i = 0; i < 3; i++)
			model_shadelight[i] = 1.0;
	}
	else
	{
		R_LightPoint(currententity->origin, model_shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if (ent->flags & RF_WEAPONMODEL)
		{
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (model_shadelight[0] > model_shadelight[1])
			{
				if (model_shadelight[0] > model_shadelight[2])
					r_lightlevel->value = 150 * model_shadelight[0];
				else
					r_lightlevel->value = 150 * model_shadelight[2];
			}
			else
			{
				if (model_shadelight[1] > model_shadelight[2])
					r_lightlevel->value = 150 * model_shadelight[1];
				else
					r_lightlevel->value = 150 * model_shadelight[2];
			}

		}

		if (r_monolightmap->string[0] != '0')
		{
			float s = model_shadelight[0];

			if (s < model_shadelight[1])
				s = model_shadelight[1];
			if (s < model_shadelight[2])
				s = model_shadelight[2];

			model_shadelight[0] = s;
			model_shadelight[1] = s;
			model_shadelight[2] = s;
		}
	}

	if (ent->flags & RF_MINLIGHT)
	{
		for (i = 0; i < 3; i++)
			if (model_shadelight[i] > 0.1)
				break;
		if (i == 3)
		{
			model_shadelight[0] = 0.1;
			model_shadelight[1] = 0.1;
			model_shadelight[2] = 0.1;
		}
	}

	if (ent->flags & RF_GLOW)
	{
		scale = 0.1 * sin(r_newrefdef.time * 7);
		for (i = 0; i < 3; i++)
		{
			min = model_shadelight[i] * 0.8;
			model_shadelight[i] += scale;
			if (model_shadelight[i] < min)
				model_shadelight[i] = min;
		}
	}

	// ir goggles color override
	if (r_newrefdef.rdflags & RDF_IRGOGGLES && ent->flags & RF_IR_VISIBLE)
	{
		model_shadelight[0] = 1.0;
		model_shadelight[1] = 0.0;
		model_shadelight[2] = 0.0;
	}

	model_shadedots = r_avertexnormal_dots[((int)(ent->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	// this is uttery shit
	an = ent->angles[1] / 180 * M_PI;
	model_shadevector[0] = cos(-an);
	model_shadevector[1] = sin(-an);
	model_shadevector[2] = 1;
	VectorNormalize(model_shadevector);
}



void R_EmitWireBox(vec3_t mins, vec3_t maxs)
{
	qglBegin(GL_QUAD_STRIP);
	qglVertex3f(mins[0], mins[1], mins[2]);
	qglVertex3f(mins[0], mins[1], maxs[2]);
	qglVertex3f(maxs[0], mins[1], mins[2]);
	qglVertex3f(maxs[0], mins[1], maxs[2]);
	qglVertex3f(maxs[0], maxs[1], mins[2]);
	qglVertex3f(maxs[0], maxs[1], maxs[2]);
	qglVertex3f(mins[0], maxs[1], mins[2]);
	qglVertex3f(mins[0], maxs[1], maxs[2]);
	qglVertex3f(mins[0], mins[1], mins[2]);
	qglVertex3f(mins[0], mins[1], maxs[2]);
	qglEnd();
}

void R_EmitWirePoint(vec3_t origin)
{
	int size = 8;

	qglBegin(GL_LINES);
	qglVertex3f(origin[0] - size, origin[1], origin[2]);
	qglVertex3f(origin[0] + size, origin[1], origin[2]);
	qglVertex3f(origin[0], origin[1] - size, origin[2]);
	qglVertex3f(origin[0], origin[1] + size, origin[2]);
	qglVertex3f(origin[0], origin[1], origin[2] - size);
	qglVertex3f(origin[0], origin[1], origin[2] + size);
	qglEnd();
}


void R_DrawBBox(vec3_t mins, vec3_t maxs)
{
	qglDisable(GL_DEPTH_TEST);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	qglDisable(GL_TEXTURE_2D);
	qglDisable(GL_CULL_FACE);
	qglColor3f(1, 1, 1);

	R_EmitWireBox(mins, maxs);

	qglColor3f(1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
	qglEnable(GL_CULL_FACE);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglEnable(GL_DEPTH_TEST);
}
