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

/*
==============================================================================
QUAKE3 MD3 MODELS
==============================================================================
*/

#include "r_local.h"

extern model_t* Mod_ForNum(int index);
extern qboolean R_EntityShouldRender(centity_t* ent);

extern float	*model_shadedots;
extern vec3_t	model_shadevector;
extern float	model_shadelight[3];

#define	LL(x) x=LittleLong(x)

#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)


void Mod_LoadMD3(model_t* mod, void* buffer, lod_t lod)
{
	int				i, j;
	md3Header_t* pinmodel;
	md3Frame_t* frame;
	md3Surface_t* surf;
	md3Shader_t* shader;
	md3Triangle_t* tri;
	md3St_t* st;
	md3XyzNormal_t* xyz;
	md3Tag_t* tag;
	int					version;
	int					size;

	mod->type = MOD_BAD;

	pinmodel = (md3Header_t*)buffer;
	version = LittleLong(pinmodel->version);
	if (version != MD3_VERSION)
	{
		ri.Con_Printf(PRINT_ALL, "MOD_LoadMD3: %s has wrong version (%i should be %i)\n", mod->name, version, MD3_VERSION);
		return;
	}

	size = LittleLong(pinmodel->ofsEnd);
	mod->extradatasize = size;

	//	mod->dataSize += size;
	mod->md3[lod] = Hunk_Alloc(size);

	memcpy(mod->md3[lod], buffer, LittleLong(pinmodel->ofsEnd));

	LL(mod->md3[lod]->ident);
	LL(mod->md3[lod]->version);
	LL(mod->md3[lod]->numFrames);
	LL(mod->md3[lod]->numTags);
	LL(mod->md3[lod]->numSurfaces);
	LL(mod->md3[lod]->ofsFrames);
	LL(mod->md3[lod]->ofsTags);
	LL(mod->md3[lod]->ofsSurfaces);
	LL(mod->md3[lod]->ofsEnd);

	if (mod->md3[lod]->numFrames < 1)
	{
		ri.Con_Printf(PRINT_ALL, "MOD_LoadMD3: %s has no frames\n", mod->name);
		return;
	}

	mod->type = MOD_MD3;

	ri.Con_Printf(PRINT_LOW, "\n=================================\n");
	ri.Con_Printf(PRINT_LOW, "Mod_LoadMD3: %s\n\n", mod->name);

	// swap all the frames
	frame = (md3Frame_t*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsFrames);
	for (i = 0; i < mod->md3[lod]->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(frame->radius);
		for (j = 0; j < 3; j++) {
			frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
		}
	}

	// swap all the tags
	tag = (md3Tag_t*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsTags);
	for (i = 0; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames; i++, tag++)
	{
		for (j = 0; j < 3; j++) {
			tag->origin[j] = LittleFloat(tag->origin[j]);
			tag->axis[0][j] = LittleFloat(tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(tag->axis[2][j]);
		}
	}

	// swap all the surfaces
	surf = (md3Surface_t*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsSurfaces);
	for (i = 0; i < mod->md3[lod]->numSurfaces; i++)
	{

		LL(surf->ident);
		LL(surf->flags);
		LL(surf->numFrames);
		LL(surf->numShaders);
		LL(surf->numTriangles);
		LL(surf->ofsTriangles);
		LL(surf->numVerts);
		LL(surf->ofsShaders);
		LL(surf->ofsSt);
		LL(surf->ofsXyzNormals);
		LL(surf->ofsEnd);

		if (surf->numVerts > MD3_MAX_VERTS)
		{
			ri.Sys_Error(ERR_DROP, "Mod_LoadMD3: %s has more than %i verts on a surface (%i)", mod->name, MD3_MAX_VERTS, surf->numVerts);
		}
		if (surf->numTriangles * 3 > SHADER_MAX_INDEXES)
		{
			ri.Sys_Error(ERR_DROP, "Mod_LoadMD3: %s has more than %i triangles on a surface (%i)", mod->name, SHADER_MAX_INDEXES / 3, surf->numTriangles);
		}

		// lowercase the surface name so skin compares are faster
		_strlwr(surf->name);

		// strip off a trailing _1 or _2
		j = strlen(surf->name);
		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// register the shaders
		shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
		for (j = 0; j < surf->numShaders; j++, shader++)
		{
			ri.Con_Printf(PRINT_ALL, "image %i: %s\n", j, shader->name);

			mod->skins[j] = GL_FindImage(shader->name, it_model); //R_RegisterSkin(shader->name);
			shader->shaderIndex = mod->skins[j]->texnum;
			if (mod->skins[j] == r_notexture)
			{
				ri.Con_Printf(PRINT_ALL, "Mod_LoadMD3: cannot load '%s' for model '%s'\n", shader->name, mod->name);
			}

		}
		ri.Con_Printf(PRINT_LOW, "\n");

		// swap all the triangles
		tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
		for (j = 0; j < surf->numTriangles; j++, tri++)
		{
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
		st = (md3St_t*)((byte*)surf + surf->ofsSt);
		for (j = 0; j < surf->numVerts; j++, st++)
		{
			st->st[0] = LittleFloat(st->st[0]);
			st->st[1] = LittleFloat(st->st[1]);
		}

		// swap all the XyzNormals
		xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
		for (j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++)
		{
			xyz->xyz[0] = LittleShort(xyz->xyz[0]);
			xyz->xyz[1] = LittleShort(xyz->xyz[1]);
			xyz->xyz[2] = LittleShort(xyz->xyz[2]);

			xyz->normal = LittleShort(xyz->normal);
		}

		ri.Con_Printf(PRINT_LOW, "Surface %i:  %i frames, %i images, %i triangles, %i verts\n", i, surf->numFrames, surf->numShaders, surf->numTriangles, surf->numVerts);

		// find the next surface
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);

	}

	ri.Con_Printf(PRINT_LOW, "\n");
	ri.Con_Printf(PRINT_LOW, "numFrames:   %i\n", mod->md3[lod]->numFrames);
	ri.Con_Printf(PRINT_LOW, "numTags:     %i\n", mod->md3[lod]->numTags);
	ri.Con_Printf(PRINT_LOW, "numSurfaces: %i\n", mod->md3[lod]->numSurfaces);

	ri.Con_Printf(PRINT_LOW, "=================================\n");
}


/*
================
MD3_GetTag
================
*/
static md3Tag_t* MD3_GetTag(md3Header_t* mod, int frame, const char* tagName)
{
	md3Tag_t* tag;
	int			i;

	if (frame >= mod->numFrames)
	{
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t*)((byte*)mod + mod->ofsTags) + frame * mod->numTags;
	for (i = 0; i < mod->numTags; i++, tag++)
	{
		if (!strcmp(tag->name, tagName))
		{
			return tag;	// found it
		}
	}

	return NULL;
}

void AxisClear(vec3_t axis[3])
{
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

/*
================
MD3_LerpTag
================
*/
static qboolean MD3_LerpTag(orientation_t* tag, int handle, int startFrame, int endFrame, float frac, const char* tagName)
{
	md3Tag_t* start, * end;
	float		frontLerp, backLerp;
	model_t* model;
	int			i;

	model = Mod_ForNum(handle);
	if (!model->md3[0])
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	start = MD3_GetTag(model->md3[0], startFrame, tagName);
	end = MD3_GetTag(model->md3[0], endFrame, tagName);

	if (!start || !end)
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for (i = 0; i < 3; i++)
	{
		tag->origin[i] = start->origin[i] * backLerp + end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp + end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp + end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp + end->axis[2][i] * frontLerp;
	}
	VectorNormalize(tag->axis[0]);
	VectorNormalize(tag->axis[1]);
	VectorNormalize(tag->axis[2]);
	return true;
}


/*
====================
MD3_ModelBounds
====================
*/
static void MD3_ModelBounds(int handle, vec3_t mins, vec3_t maxs)
{
	model_t* model;
	md3Header_t* header;
	md3Frame_t* frame;

	model = Mod_ForNum(handle);
	if (!model->md3[0])
	{
		VectorClear(mins);
		VectorClear(maxs);
		return;
	}

	header = model->md3[0];

	frame = (md3Frame_t*)((byte*)header + header->ofsFrames);

	VectorCopy(frame->bounds[0], mins);
	VectorCopy(frame->bounds[1], maxs);
}


/*
=================
R_DrawMD3Model
=================
*/
#define RF_WRAP_FRAMES 4096
extern float	r_avertexnormal_dots[16][256];

void R_SetEntityShadeLight(centity_t* ent);

void R_DrawMD3Model(centity_t* ent)
{
	md3Header_t		*model;
	md3Surface_t	*surface = 0;
	md3Shader_t		*md3Shader = 0;
	image_t			*shader = NULL;
	int				i, j;

	model = ent->model->md3[LOD_HIGH]; // todo: pick proper lod
	if ((ent->frame >= model->numFrames) || (ent->frame < 0) ) 
	{
		ri.Con_Printf(PRINT_DEVELOPER, "R_DrawMD3Model: no such frame %d in '%s'\n", ent->frame, ent->model->name);
		ent->frame = 0;
		ent->oldframe = 0;
	}

	//
	// check if we're visible
	//
	if(!R_EntityShouldRender(ent))
		return;

	//
	// setup lighting
	//
	R_SetEntityShadeLight(ent);

	//
	// move and rotate
	//
	qglPushMatrix();
	ent->angles[PITCH] = -ent->angles[PITCH];
	R_RotateForEntity(ent);
	ent->angles[PITCH] = -ent->angles[PITCH];


	//
	// draw
	//
	qglShadeModel(GL_SMOOTH);
	GL_TexEnv(GL_MODULATE);

	if (ent->flags & RF_TRANSLUCENT)
	{
		qglEnable(GL_BLEND);
	}

	if (!r_lerpmodels->value)
		ent->backlerp = 0;

	surface = (md3Surface_t*)((byte*)model + model->ofsSurfaces);
	for (i = 0; i < model->numSurfaces; i++)
	{
		if (surface->numShaders > 0) 
		{
			md3Shader = (md3Shader_t*)((byte*)surface + surface->ofsShaders);
			GL_Bind(md3Shader->shaderIndex);
		}

		md3St_t* texcoord = (md3St_t*)((byte*)surface + surface->ofsSt);
		md3XyzNormal_t* vert = (md3XyzNormal_t*)((byte*)surface + surface->ofsXyzNormals);
		md3Triangle_t* triangle = (md3Triangle_t*)((byte*)surface + surface->ofsTriangles);

//		ri.Con_Printf(PRINT_ALL, "shadelight %f.3 %f.3 %f.3\n", model_shadelight[0], model_shadelight[1], model_shadelight[2]);

		for (j = 0; j < surface->numTriangles; j++, triangle++)
		{
			qglBegin(GL_TRIANGLES);

			for (int k = 0; k < 3; k++)
			{
				// apply ligiting
				float l = model_shadedots[vert[triangle->indexes[k]].normal];
				qglColor4f(l*model_shadelight[0], l*model_shadelight[1], l*model_shadelight[2], 1.0f);
				
				qglTexCoord2f(texcoord[triangle->indexes[k]].st[0], texcoord[triangle->indexes[k]].st[1]);		
				qglVertex3f( (GLfloat)vert[triangle->indexes[k]].xyz[0]/32, (GLfloat)vert[triangle->indexes[k]].xyz[1]/32, (GLfloat)vert[triangle->indexes[k]].xyz[2]/32 );
			}
			qglEnd();
		}
		surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
//		ri.Con_Printf(PRINT_ALL, "R_DrawMD3Model:%d '%s'\n", ent->frame, ent->model->name);
	}
	qglPopMatrix();

	if ((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F))
	{
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		qglCullFace(GL_FRONT);
	}

	if (currententity->flags & RF_TRANSLUCENT)
	{
		qglDisable(GL_BLEND);
	}

	GL_TexEnv(GL_REPLACE);
	qglShadeModel(GL_FLAT);

}
