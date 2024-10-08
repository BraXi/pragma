/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================================

	QUAKE3 MD3 ALIAS MODELS

==============================================================================
*/

#include "r_local.h"

extern model_t* R_ModelForNum(int index);
extern int modelFileLength; // for in Mod_LoadAliasMD3
extern vec3_t	model_shadevector;
extern float	model_shadelight[3];

extern qboolean r_pendingflip;

#ifndef LL
#define	LL(x) x=LittleLong(x)
#endif

#ifndef BUFFER_OFFSET
#define BUFFER_OFFSET(i) ((char*)NULL + (i))
#endif

#define FUNCTABLE_SIZE 1024
#define FUNCTABLE_MASK (FUNCTABLE_SIZE-1)
static float sinTable[FUNCTABLE_SIZE];
static qboolean bSinTableGenerated = false;

/*
=================
R_GenerateSinTableForAliasModels
This is only used to upload md3 vertices to gpu.
=================
*/
static void R_GenerateSinTableForAliasModels()
{
	if (bSinTableGenerated)
		return;

	for (int i = 0; i < FUNCTABLE_SIZE; i++)
		sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));

	bSinTableGenerated = true;
}

/*
=================
R_LerpAliasFrame
Smoothly transitions vertices between two animation frames and also calculates normal
This is only used to upload vertices to gpu.
=================
*/
static void R_LerpAliasFrame(float lerp, int index, md3XyzNormal_t* oldVert, md3XyzNormal_t* vert, vec3_t outVert, vec3_t outNormal)
{
	int lat, lng;
	vec3_t p1, p2, n1, n2;

	// linear interpolation between the current and next vertex positions
	VectorCopy(oldVert->xyz, p1);
	VectorCopy(vert->xyz, p2);

	outVert[0] = (p1[0] + lerp * (p2[0] - p1[0]));
	outVert[1] = (p1[1] + lerp * (p2[1] - p1[1]));
	outVert[2] = (p1[2] + lerp * (p2[2] - p1[2]));

	// scale verticles to qu
	outVert[0] = (outVert[0] / 64);
	outVert[1] = (outVert[1] / 64);
	outVert[2] = (outVert[2] / 64);

	// retrieve and linear interpolate normal
	lat = (oldVert->normal >> 8) & 0xff;
	lng = (oldVert->normal & 0xff);
	lat *= (FUNCTABLE_SIZE / 256);
	lng *= (FUNCTABLE_SIZE / 256);

	n1[0] = sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * sinTable[lng];
	n1[1] = sinTable[lat] * sinTable[lng];
	n1[2] = sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

	lat = (vert->normal >> 8) & 0xff;
	lng = (vert->normal & 0xff);
	lat *= (FUNCTABLE_SIZE / 256);
	lng *= (FUNCTABLE_SIZE / 256);

	n2[0] = sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] * sinTable[lng];
	n2[1] = sinTable[lat] * sinTable[lng];
	n2[2] = sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

	outNormal[0] = (n1[0] + lerp * (n2[0] - n1[0]));
	outNormal[1] = (n1[1] + lerp * (n2[1] - n1[1]));
	outNormal[2] = (n1[2] + lerp * (n2[2] - n1[2]));
}

/*
=================
R_UploadAliasModelTris
=================
*/
static void R_UploadAliasModelTris(model_t* mod)
{
	md3Header_t		*pModel = NULL;
	md3Surface_t	*pSurface = NULL;
	md3St_t			*pTexCoord = NULL;
	md3Triangle_t	*pTriangle = NULL;
	md3XyzNormal_t	*pVert, *pOldVert;
	vec3_t			v, n; //vert and normal after lerp
	int				surf, tri, trivert, frame, oldframe;
	int numverts;

	pModel = mod->alias;
	frame = oldframe = 0;

	R_GenerateSinTableForAliasModels();

	pSurface = (md3Surface_t*)((byte*)pModel + pModel->ofsSurfaces);
	for (surf = 0; surf < pModel->numSurfaces; surf++)
	{	
		mod->vb[surf] = R_AllocVertexBuffer((V_UV | V_NORMAL), ((pSurface->numTriangles * 3) * pModel->numFrames), 0);
		
		numverts = 0;
		for (frame = 0; frame < pModel->numFrames; frame++)
		{
			oldframe = frame;

			pTexCoord = (md3St_t*)((byte*)pSurface + pSurface->ofsSt);
			pTriangle = (md3Triangle_t*)((byte*)pSurface + pSurface->ofsTriangles);
			pOldVert = (short*)((byte*)pSurface + pSurface->ofsXyzNormals) + (oldframe * pSurface->numVerts * 4); // current keyframe verts
			pVert = (short*)((byte*)pSurface + pSurface->ofsXyzNormals) + (frame * pSurface->numVerts * 4); // next keyframe verts

			for (tri = 0; tri < pSurface->numTriangles; tri++, pTriangle++)
			{
				for (trivert = 0; trivert < 3; trivert++)
				{
					int index = pTriangle->indexes[trivert];

					R_LerpAliasFrame(1.0f, index, &pOldVert[index], &pVert[index], v, n);

					VectorCopy(v, mod->vb[surf]->verts[numverts].xyz);
					VectorNormalize(n);

					VectorCopy(n, mod->vb[surf]->verts[numverts].normal);
					mod->vb[surf]->verts[numverts].st[0] = pTexCoord[index].st[0];
					mod->vb[surf]->verts[numverts].st[1] = pTexCoord[index].st[1];
					numverts++;
				}
			}
		}
		R_UpdateVertexBuffer(mod->vb[surf], mod->vb[surf]->verts, numverts, (V_UV | V_NORMAL));
		pSurface = (md3Surface_t*)((byte*)pSurface + pSurface->ofsEnd);
	}
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

	model = R_ModelForNum(handle);
	if (!model->alias)
	{
		VectorClear(mins);
		VectorClear(maxs);
		return;
	}

	header = model->alias;

	frame = (md3Frame_t*)((byte*)header + header->ofsFrames);

	VectorCopy(frame->bounds[0], mins);
	VectorCopy(frame->bounds[1], maxs);
}

/*
====================
MD3_ModelRadius
====================
*/
static float MD3_ModelRadius(int handle)
{
	model_t* model;
	md3Header_t* header;
	md3Frame_t* frame;

	model = R_ModelForNum(handle);
	if (!model->alias)
	{
		return 32.0f; // default
	}

	header = model->alias;
	frame = (md3Frame_t*)((byte*)header + header->ofsFrames);
	return frame->radius;
}

/*
=================
Mod_LoadAliasMD3

Loads Quake III MD3 model
=================
*/
void Mod_LoadAliasMD3(model_t* mod, void* buffer)
{
	int				i, j;
	md3Header_t		* pinmodel;
	md3Frame_t		* frame;
	md3Surface_t	* surf;
	md3Shader_t		* shader;
	md3Triangle_t	* tri;
	md3St_t			* st;
	md3XyzNormal_t	* xyz;
	md3Tag_t		* tag;
	int				version;
	int				size;

	mod->type = MOD_BAD;

	pinmodel = (md3Header_t*)buffer;
	version = LittleLong(pinmodel->version);
	if (version != MD3_VERSION)
	{
		ri.Error(ERR_DROP, "MOD_LoadMD3: '%s' has wrong version (%i should be %i)\n", mod->name, version, MD3_VERSION);
		return;
	}

	size = LittleLong(pinmodel->ofsEnd);
	if(modelFileLength != size)
	{
		ri.Error(ERR_DROP, "MOD_LoadMD3: '%s' has weird size (probably broken)\n", mod->name);
		return;
	}

	mod->extradatasize = size;
	mod->alias = Hunk_Alloc(size);

	memcpy(mod->alias, buffer, LittleLong(pinmodel->ofsEnd));

	LL(mod->alias->ident);
	LL(mod->alias->version);
	LL(mod->alias->numFrames);
	LL(mod->alias->numTags);
	LL(mod->alias->numSurfaces);
	LL(mod->alias->ofsFrames);
	LL(mod->alias->ofsTags);
	LL(mod->alias->ofsSurfaces);
	LL(mod->alias->ofsEnd);

	if (mod->alias->numFrames < 1)
	{
		ri.Error(ERR_DROP, "Mod_LoadAliasMD3: %s has no frames\n", mod->name);
		return;
	}

	// swap all the frames
	frame = (md3Frame_t*)((byte*)mod->alias + mod->alias->ofsFrames);
	for (i = 0; i < mod->alias->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(frame->radius);
		for (j = 0; j < 3; j++) {
			frame->bounds[0][j] = LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j] = LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j] = LittleFloat(frame->localOrigin[j]);
		}
	}

	// swap all the tags
	tag = (md3Tag_t*)((byte*)mod->alias + mod->alias->ofsTags);
	for (i = 0; i < mod->alias->numTags * mod->alias->numFrames; i++, tag++)
	{
		for (j = 0; j < 3; j++) 
		{
			tag->origin[j] = LittleFloat(tag->origin[j]);
			tag->axis[0][j] = LittleFloat(tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(tag->axis[2][j]);
		}	
		_strlwr(tag->name); // lowercase the tag name so search compares are faster
	}

	// swap all the surfaces
	surf = (md3Surface_t*)((byte*)mod->alias + mod->alias->ofsSurfaces);
	for (i = 0; i < mod->alias->numSurfaces; i++)
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
			ri.Error(ERR_DROP, "Mod_LoadAliasMD3: %s has more than %i verts on a surface (%i)", mod->name, MD3_MAX_VERTS, surf->numVerts);
		}
		if (surf->numTriangles > MD3_MAX_TRIANGLES)
		{
			ri.Error(ERR_DROP, "Mod_LoadAliasMD3: %s has more than %i triangles on a surface (%i)", mod->name, MD3_MAX_TRIANGLES, surf->numTriangles);
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
			mod->images[j] = R_FindTexture(shader->name, it_model, true);
			shader->shaderIndex = mod->images[j]->texnum;
			if (mod->images[j] == r_texture_missing)
			{
				ri.Printf(PRINT_ALL, "Mod_LoadAliasMD3: cannot load '%s' for model '%s'\n", shader->name, mod->name);
			}

		}

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

		// find the next surface
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}

	mod->type = MOD_ALIAS;
	mod->numframes = mod->alias->numFrames;
	mod->radius = MD3_ModelRadius(mod->index);
	MD3_ModelBounds(mod->index, mod->mins, mod->maxs);

	R_UploadAliasModelTris(mod);
}

/*
================
MD3_GetTag
================
*/
static md3Tag_t* MD3_GetTag(md3Header_t* mod, int frame, int tagIndex)
{
	md3Tag_t* tag;
	int			i;

	if (frame >= mod->numFrames)
	{
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}
	else if (frame < 0)
	{
		frame = 0;
	}

	tag = (md3Tag_t*)((byte*)mod + mod->ofsTags) + frame * mod->numTags;
	for (i = 0; i < mod->numTags; i++, tag++)
	{
		if (i == tagIndex)
		{
			return tag;	// found it
		}
	}
	return NULL;
}

/*
================
R_TagIndexForName
================
*/
int R_TagIndexForName(struct model_s *model, const char* tagName)
{
	md3Tag_t* tag;
	int			i;

	if (!model->alias)
		return -1;

	md3Header_t *mod = model->alias;
	tag = (md3Tag_t*)((byte*)mod + mod->ofsTags);
	for (i = 0; i < mod->numTags; i++, tag++)
	{
		if (!strcmp(tag->name, tagName))
		{
			return i;	// found it
		}
	}
	return -1;
}

/*
================
MD3_LerpTag
================
*/
static qboolean MD3_LerpTag(orientation_t* tag, model_t* model, int startFrame, int endFrame, float frac, int tagIndex)
{
	md3Tag_t* start, * end;
	float		frontLerp, backLerp;
	int			i;

	if (!model->alias)
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	start = MD3_GetTag(model->alias, startFrame, tagIndex);
	end = MD3_GetTag(model->alias, endFrame, tagIndex);

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
===============
DrawVertexBuffer

modified version of R_DrawVertexBuffer to account model animation
===============
*/
static void DrawVertexBuffer(rentity_t *ent, vertexbuffer_t* vbo, unsigned int startVert, unsigned int numVerts )
{
	int framesize;

	framesize = sizeof(glvert_t)* vbo->numVerts / ent->model->numframes;

	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);

	glEnableVertexAttribArray(0);
	glBindAttribLocation(pCurrentProgram->programObject, 0, "inOldVertPos");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(0) + ent->oldframe * framesize);

	glEnableVertexAttribArray(1);
	glBindAttribLocation(pCurrentProgram->programObject, 1, "inVertPos");
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(0) + ent->frame * framesize);

	glEnableVertexAttribArray(2);
	glBindAttribLocation(pCurrentProgram->programObject, 2, "inOldNormal");
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(12) + ent->oldframe * framesize);

	glEnableVertexAttribArray(3);
	glBindAttribLocation(pCurrentProgram->programObject, 3, "inNormal");
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(12) + ent->frame * framesize);

	glEnableVertexAttribArray(4);
	glBindAttribLocation(pCurrentProgram->programObject, 4, "inTexCoord");
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(24) + ent->frame * framesize);

	if (!numVerts)
		glDrawArrays(GL_TRIANGLES, 0, vbo->numVerts);
	else
		glDrawArrays(GL_TRIANGLES, startVert, numVerts);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


/*
=================
R_DrawAliasModel
Draws md3 model
=================
*/
void R_DrawAliasModel(rentity_t* ent, float animlerp)
{
	md3Header_t		*pModel = NULL;
	md3Surface_t	*pSurface = NULL;
	int				surf, surfverts;
	qboolean		anythingToDraw = false;

	pModel = ent->model->alias;
	if (pModel == NULL)
	{
		Com_Printf("R_DrawAliasModel: '%s' has no model\n", ent->model->name);
		return;
	}

	if (!ent->model->vb[0])
		return; // not uploaded to gpu

	R_BindProgram(GLPROG_ALIAS);

	R_ProgUniformVec3(LOC_AMBIENT_COLOR, model_shadelight);
	R_ProgUniformVec3(LOC_AMBIENT_DIR, model_shadevector);
	R_ProgUniform1f(LOC_LERPFRAC, animlerp);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_local_matrix);
	if (r_pendingflip)
	{
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT)
		R_ProgUniform1f(LOC_PARM0, 1);
	else
		R_ProgUniform1f(LOC_PARM0, 0);

	if (ent->renderfx & RF_TRANSLUCENT)
		R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, ent->alpha);
	else
		R_ProgUniform4f(LOC_COLOR4, 1, 1, 1, 1);

	if (ent->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(true);

	//glDisable(GL_CULL_FACE);
	for (surf = 0; surf < pModel->numSurfaces; surf++)
	{
		surfverts = ent->model->vb[surf]->numVerts / pModel->numFrames;

		pSurface = (md3Surface_t*)((byte*)pModel + pModel->ofsSurfaces);
		if ((ent->hiddenPartsBits & (1 << surf)))
		{
			pSurface = (md3Surface_t*)((byte*)pSurface + pSurface->ofsEnd);
			continue; // surface is hidden
		}

		if (r_speeds->value)
		{
			rperf.alias_tris += surfverts / 3;
		}

		anythingToDraw = true;

		R_MultiTextureBind(TMU_DIFFUSE, ent->model->images[surf]->texnum);
		DrawVertexBuffer(ent, ent->model->vb[surf], 0, surfverts);

		pSurface = (md3Surface_t*)((byte*)pSurface + pSurface->ofsEnd);
	}

	//glEnable(GL_CULL_FACE);
	if (r_pendingflip)
	{
		Mat4Scale(r_projection_matrix, -1, 1, 1);
		R_ProgUniformMatrix4fv(LOC_PROJECTION, 1, r_projection_matrix);
	}

	if (ent->renderfx & RF_VIEW_MODEL)
		R_SendDynamicLightsToCurrentProgram(false);

	if (r_speeds->value && anythingToDraw)
		rperf.alias_drawcalls++;
}

qboolean R_LerpTag(orientation_t* tag, struct model_s* model, int startFrame, int endFrame, float frac, int tagIndex)
{
	return MD3_LerpTag(tag, model, startFrame, endFrame, frac, tagIndex);
}