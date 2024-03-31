/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_world.c -- NEW WORLD RENDERING CODE

#include <assert.h>
#include "r_local.h"

extern vec3_t		modelorg;		// relative to viewpoint
extern msurface_t	*r_alpha_surfaces;

extern cvar_t* r_fastworld;

extern image_t* R_World_TextureAnimation(mtexinfo_t* tex);

/*
=================
R_NEW_DrawInlineBModel
=================
*/

void R_DrawInlineBModel_NEW()
{
}


/*
================
R_DrawWorld_TextureChains_NEW
================
*/
void R_DrawWorld_TextureChains_NEW()
{
	int		i;
	msurface_t* s;
	image_t* image;

	rperf.visible_textures = 0;

	R_BindProgram(GLPROG_WORLD);

	// lightmapped surfaces
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence || !image->texturechain)
			continue;

		rperf.visible_textures++;

		for (s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
			{ 
				// draw all lightmapped surfs
			}
		}
	}

	// unlit and/or fullbright surfaces
	for (i = 0, image = r_textures; i < r_textures_count; i++, image++)
	{
		if (!image->registration_sequence)
			continue;

		s = image->texturechain;
		if (!s)
			continue;

		for (; s; s = s->texturechain)
		{
			if (s->flags & SURF_DRAWTURB)
			{
				// draw surf
			}
		}

		if (!r_showtris->value)
			image->texturechain = NULL;
	}
	R_UnbindProgram();
}

/*
================
R_World_DrawAlphaSurfaces_NEW
================
*/
void R_World_DrawAlphaSurfaces_NEW()
{
	msurface_t* surf;

	glLoadMatrixf(r_world_matrix);

	R_Blend(true);
	R_BindProgram(GLPROG_WORLD);
	for (surf = r_alpha_surfaces; surf; surf = surf->texturechain)
	{
		if (surf->flags & SURF_DRAWTURB)
		{
			// draw unlit water surf chain
		}
		else
		{
			// draw unlit surf
		}
	}
	R_Blend(false);
	R_UnbindProgram();

	r_alpha_surfaces = NULL;
}



void R_DrawWorld_NEW()
{

}