/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================================
QUAKE3 MD3 MODELS
==============================================================================
*/

#include "r_local.h"

static void R_LerpMD3Frame(float lerp, int index, md3XyzNormal_t* oldVert, md3XyzNormal_t* vert, vec3_t outVert, vec3_t outNormal);
extern model_t* Mod_ForNum(int index);

extern int modfilelen; // for in Mod_LoadMD3

extern float	sinTable[FUNCTABLE_SIZE];
extern vec3_t	model_shadevector;
extern float	model_shadelight[3];


/*
=================
R_UploadMD3ToVertexBuffer

Upload md3 to vertex buffers, this only works with static models currently

TODO: This does currently generate one buffer per model surface group, for first animation frame only.
TODO: lods
=================
*/
static void R_UploadMD3ToVertexBuffer(model_t* mod, lod_t lod)
{
	md3Header_t		*pModel = NULL;
	md3Surface_t	*pSurface = NULL;
	md3St_t			*pTexCoord = NULL;
	md3Triangle_t	*pTriangle = NULL;
	md3XyzNormal_t	*pVert, *pOldVert;
	vec3_t			v, n; //vert and normal after lerp
	int				surf, tri, trivert, frame, oldframe;
	int numverts;

	pModel = mod->md3[lod];

	frame = oldframe = 0;

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

					R_LerpMD3Frame(0.0f, index, &pOldVert[index], &pVert[index], v, n); // 0 is not lerping at all, always "current" frame

					VectorCopy(v, mod->vb[surf]->verts[numverts].xyz);
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

	model = Mod_ForNum(handle);
	if (!model->md3[LOD_HIGH])
	{
		VectorClear(mins);
		VectorClear(maxs);
		return;
	}

	header = model->md3[LOD_HIGH];

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

	model = Mod_ForNum(handle);
	if (!model->md3[LOD_HIGH])
	{
		return 32.0f; // default
	}

	header = model->md3[LOD_HIGH];
	frame = (md3Frame_t*)((byte*)header + header->ofsFrames);
	return frame->radius;
}

/*
=================
Mod_LoadMD3

Loads Quake III MD3 model
=================
*/

#define	LL(x) x=LittleLong(x)
void Mod_LoadMD3(model_t* mod, void* buffer, lod_t lod)
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
	if(modfilelen != size)
	{
		ri.Error(ERR_DROP, "MOD_LoadMD3: '%s' has weird size (probably broken)\n", mod->name);
		return;
	}

	mod->extradatasize = size;
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
		ri.Error(ERR_DROP, "Mod_LoadMD3: %s has no frames\n", mod->name);
		return;
	}

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
			ri.Error(ERR_DROP, "Mod_LoadMD3: %s has more than %i verts on a surface (%i)", mod->name, MD3_MAX_VERTS, surf->numVerts);
		}
		if (surf->numTriangles > MD3_MAX_TRIANGLES)
		{
			ri.Error(ERR_DROP, "Mod_LoadMD3: %s has more than %i triangles on a surface (%i)", mod->name, MD3_MAX_TRIANGLES, surf->numTriangles);
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
			mod->images[j] = R_FindTexture(shader->name, it_model);
			shader->shaderIndex = mod->images[j]->texnum;
			if (mod->images[j] == r_notexture)
			{
				ri.Printf(PRINT_ALL, "Mod_LoadMD3: cannot load '%s' for model '%s'\n", shader->name, mod->name);
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

	mod->type = MOD_MD3;
	mod->numframes = mod->md3[lod]->numFrames;
	mod->radius = MD3_ModelRadius(mod->index);
	MD3_ModelBounds(mod->index, mod->mins, mod->maxs);

	R_UploadMD3ToVertexBuffer(mod, lod);
}


/*
================
MD3_GetTag
================
*/
//static md3Tag_t* MD3_GetTag(md3Header_t* mod, int frame, const char* tagName)
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

	if (!model->md3[0])
		return -1;

	md3Header_t *mod = model->md3[LOD_HIGH];
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

	if (!model->md3[LOD_HIGH])
	{
		AxisClear(tag->axis);
		VectorClear(tag->origin);
		return false;
	}

	start = MD3_GetTag(model->md3[LOD_HIGH], startFrame, tagIndex);
	end = MD3_GetTag(model->md3[LOD_HIGH], endFrame, tagIndex);

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
=================
R_LerpMD3Frame

Smoothly transitions vertices between two animation frames and also calculates normal
=================
*/
static void R_LerpMD3Frame(float lerp, int index, md3XyzNormal_t* oldVert, md3XyzNormal_t* vert, vec3_t outVert, vec3_t outNormal)
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

static void SetVBOClientState(vertexbuffer_t* vbo, qboolean enable)
{
	if (enable)
	{
		glEnableClientState(GL_VERTEX_ARRAY);

		if (vbo->flags & V_NORMAL)
			glEnableClientState(GL_NORMAL_ARRAY);

		if (vbo->flags & V_UV)
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		if (vbo->flags & V_COLOR)
			glEnableClientState(GL_COLOR_ARRAY);
	}
	else
	{
		glDisableClientState(GL_VERTEX_ARRAY);

		if (vbo->flags & V_NORMAL)
			glDisableClientState(GL_NORMAL_ARRAY);

		if (vbo->flags & V_UV)
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		if (vbo->flags & V_COLOR)
			glDisableClientState(GL_COLOR_ARRAY);
	}
}

/*
===============
DrawVertexBuffer
===============
*/
#define BUFFER_OFFSET(i) ((char*)NULL + (i))
void DrawVertexBuffer(rentity_t *ent, vertexbuffer_t* vbo, unsigned int startVert, unsigned int numVerts )
{
//	vbo->flags = V_UV | V_NORMAL;
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);

	int framesize = sizeof(glvert_t) * vbo->numVerts / ent->model->numframes;

	//void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);

	glEnableVertexAttribArray(0);
	//glBindAttribLocation(pCurrentProgram->programObject, 0, "inOldVertPos");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(0) + ent->oldframe * framesize);

	glEnableVertexAttribArray(1);
	//glBindAttribLocation(pCurrentProgram->programObject, 1, "inVertPos");
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(0) + ent->frame * framesize);

	glEnableVertexAttribArray(2);
	//glBindAttribLocation(pCurrentProgram->programObject, 2, "inOldNormal");
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(12) + ent->oldframe * framesize);

	glEnableVertexAttribArray(3);
	//glBindAttribLocation(pCurrentProgram->programObject, 3, "inNormal");
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(12) + ent->frame * framesize);

	glEnableVertexAttribArray(4);
	//glBindAttribLocation(pCurrentProgram->programObject, 4, "inTexCoord");
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(24) + ent->frame * framesize);

	// case one: we have index buffer
	if (vbo->numIndices && vbo->indices != NULL && vbo->indexBuf)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->indexBuf);

		//glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
		if (numVerts)
			glDrawRangeElements(GL_TRIANGLES, startVert, startVert + numVerts, numVerts, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
		else
			glDrawRangeElements(GL_TRIANGLES, 0, vbo->numVerts, vbo->numVerts, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else // case two: we don't have index buffer and for simple primitives like gui we usualy don't
	{
		if (!numVerts)
			glDrawArrays(GL_TRIANGLES, 0, vbo->numVerts);
		else
			glDrawArrays(GL_TRIANGLES, startVert, numVerts);
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


/*
=================
R_FastDrawMD3Model
=================
*/
static void R_FastDrawMD3Model(rentity_t* ent, md3Header_t* pModel, float lerp)
{
	md3Surface_t* pSurface = NULL;
	int				surf, surfverts;
	qboolean		anythingToDraw = false;

	R_ProgUniform1f(LOC_LERPFRAC, lerp);

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
			rperf.alias_fasttris += surfverts / 3;
		}

		anythingToDraw = true;

		R_BindTexture(r_speeds->value >= 3.0f ? r_notexture->texnum : ent->model->images[surf]->texnum);
		DrawVertexBuffer(ent, ent->model->vb[surf], 0, surfverts);

		pSurface = (md3Surface_t*)((byte*)pSurface + pSurface->ofsEnd);
	}

	if (r_speeds->value && anythingToDraw)
		rperf.alias_fastdraws++;
}


/*
=================
R_DrawMD3Model

Draws md3 model
=================
*/
vertexbuffer_t lerpVertsBuf[MD3_MAX_SURFACES];
static glvert_t lerpVerts[MD3_MAX_SURFACES][MD3_MAX_VERTS]; // this could be one list instead of MD3_MAX_SURFACES
static int lerpVertsCount[MD3_MAX_SURFACES];

void R_DrawMD3Model(rentity_t* ent, lod_t lod, float animlerp)
{
	md3Header_t		* pModel = NULL;
	md3Surface_t	* pSurface = NULL;
	md3St_t			* pTexCoord = NULL;
	md3Triangle_t	* pTriangle = NULL;
	md3XyzNormal_t	* pVert, * pOldVert;
	vec3_t			v, n; //vert and normal after lerp
	int				surf, tri, trivert;


	if (lod < 0 || lod >= NUM_LODS)
		ri.Error(ERR_DROP, "R_DrawMD3Model: '%s' wrong LOD num %i\n", ent->model->name, lod);

	pModel = ent->model->md3[lod];
	if (pModel == NULL)
	{
		Com_Printf("R_DrawMD3Model: '%s' has no LOD %i model\n", ent->model->name, lod);
		return;
	}

	R_BindProgram(GLPROG_ALIAS);
	if (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT)
		R_ProgUniform1f(LOC_PARM0, 1);
	else
		R_ProgUniform1f(LOC_PARM0, 0);

	R_ProgUniformVec3(LOC_SHADECOLOR, model_shadelight);
	R_ProgUniformVec3(LOC_SHADEVECTOR, model_shadevector);
	
	glColor4f(1, 1, 1, ent->alpha);


	if (ent->model->vb[0])
	{
		VectorSubtract(r_newrefdef.view.origin, ent->origin, v); // FIXME: doesn't account for FOV
		float dist = VectorLength(v);

		//if (ent->frame == ent->oldframe || dist >= r_nolerpdist->value || animlerp == 1.0f)
		{
			R_FastDrawMD3Model(ent, pModel, animlerp);
				return;
		}
	}

	//
	// lerp vertices on cpu
	//
	pSurface = (md3Surface_t*)((byte*)pModel + pModel->ofsSurfaces);
	for (surf = 0; surf < pModel->numSurfaces; surf++)
	{
		lerpVertsCount[surf] = 0;

		if ((ent->hiddenPartsBits & (1 << surf)))
		{
			pSurface = (md3Surface_t*)((byte*)pSurface + pSurface->ofsEnd);
			continue; // surface is hidden
		}

		pTexCoord = (md3St_t*)((byte*)pSurface + pSurface->ofsSt);
		pTriangle = (md3Triangle_t*)((byte*)pSurface + pSurface->ofsTriangles);

		pOldVert = (short*)((byte*)pSurface + pSurface->ofsXyzNormals) + (ent->oldframe * pSurface->numVerts * 4);
		pVert = (short*)((byte*)pSurface + pSurface->ofsXyzNormals) + (ent->frame * pSurface->numVerts * 4);

		for (tri = 0; tri < pSurface->numTriangles; tri++, pTriangle++)
		{
			for (trivert = 0; trivert < 3; trivert++)
			{
				int index = pTriangle->indexes[trivert];

				R_LerpMD3Frame(animlerp, index, &pOldVert[index], &pVert[index], v, n); // 1 is not lerping at all, always "current" frame
				VectorCopy(v, lerpVerts[surf][lerpVertsCount[surf]].xyz);
				VectorCopy(n, lerpVerts[surf][lerpVertsCount[surf]].normal);
				lerpVerts[surf][lerpVertsCount[surf]].st[0] = pTexCoord[index].st[0];
				lerpVerts[surf][lerpVertsCount[surf]].st[1] = pTexCoord[index].st[1];		
				lerpVertsCount[surf]++;
			}
		}

		if (r_speeds->value)
		{
			rperf.alias_tris += lerpVertsCount[surf] / 3;
			rperf.alias_lerpverts += lerpVertsCount[surf];
		}

		R_UpdateVertexBuffer(&lerpVertsBuf[surf], lerpVerts[surf], lerpVertsCount[surf], (V_UV | V_NORMAL));
		
		R_BindTexture(ent->model->images[surf]->texnum);
		R_DrawVertexBuffer(&lerpVertsBuf[surf], 0, 0);

		pSurface = (md3Surface_t*)((byte*)pSurface + pSurface->ofsEnd);
	}

	if (r_speeds->value)
	{
		for (surf = 0; surf < pModel->numSurfaces; surf++)
		{
			if (lerpVertsCount[surf] > 0)
			{
				rperf.alias_slowdraws++;
				break;
			}
		}
	}
	
}

#if 0
void R_DrawMD3Model(rentity_t* ent, lod_t lod, float animlerp)
{
	md3Header_t* model;
	md3Surface_t* surface = 0;
	md3Shader_t* md3Shader = 0;
	md3Frame_t* md3Frame = 0;
	image_t* shader = NULL;
	md3St_t* texcoord;
	md3XyzNormal_t* vert, * oldVert; //interp
	md3Triangle_t* triangle;
	int				i, j, k;

	vec3_t v, n; //vert and normal after lerp

	if (lod < 0 || lod >= NUM_LODS)
		ri.Error(ERR_DROP, "R_DrawMD3Model: '%s' wrong LOD num %i\n", ent->model->name, lod);

	model = ent->model->md3[lod];
	if (model == NULL)
	{
		Com_Printf("R_DrawMD3Model: '%s' has no LOD %i model\n", ent->model->name, lod);
		return;
	}

	R_BindProgram(GLPROG_ALIAS);
	if (r_fullbright->value || pCurrentRefEnt->renderfx & RF_FULLBRIGHT)
		R_ProgUniform1f(LOC_PARM0, 1);
	else
		R_ProgUniform1f(LOC_PARM0, 0);

	R_ProgUniformVec3(LOC_SHADECOLOR, model_shadelight);
	R_ProgUniformVec3(LOC_SHADEVECTOR, model_shadevector);

	glColor4f(1, 1, 1, ent->alpha);


	//
	// whereas possible, try to render optimized mesh
	//
	if (r_fast->value && ent->model->vb[0]) // && ent->oldframe == 0 && ent->frame == 0)
	{
		for (i = 0; i < model->numSurfaces; i++)
		{
			int surfverts = ent->model->vb[i]->numVerts / model->numFrames;

			surface = (md3Surface_t*)((byte*)model + model->ofsSurfaces);
			if ((ent->hiddenPartsBits & (1 << i)))
			{
				surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
				continue; // surface is hidden
			}

			if (r_speeds->value)
				rperf.alias_tris += surfverts / 3;

			R_BindTexture(ent->model->images[i]->texnum);
			R_DrawVertexBuffer(ent->model->vb[i], ent->frame * surfverts, surfverts);

			surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
		}
		return;
	}

	//
	// use old immediate mode rendering
	//
	surface = (md3Surface_t*)((byte*)model + model->ofsSurfaces);
	for (i = 0; i < model->numSurfaces; i++)
	{
		if ((ent->hiddenPartsBits & (1 << i)))
		{
			surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
			continue; // surface is hidden
		}

		if (surface->numShaders > 0)
		{
			//bind texture
			md3Shader = (md3Shader_t*)((byte*)surface + surface->ofsShaders);
			R_BindTexture(md3Shader->shaderIndex);
		}

		// grab tris, texcoords and verts
		texcoord = (md3St_t*)((byte*)surface + surface->ofsSt);
		triangle = (md3Triangle_t*)((byte*)surface + surface->ofsTriangles);

		oldVert = (short*)((byte*)surface + surface->ofsXyzNormals) + (ent->oldframe * surface->numVerts * 4); // current keyframe verts
		vert = (short*)((byte*)surface + surface->ofsXyzNormals) + (ent->frame * surface->numVerts * 4); // next keyframe verts

		if (r_speeds->value)
			rperf.alias_tris += surface->numTriangles;

		// for each triangle in surface
		glBegin(GL_TRIANGLES);
		for (j = 0; j < surface->numTriangles; j++, triangle++)
		{
			//for each vert in triangle
			for (k = 0; k < 3; k++)
			{
				int index = triangle->indexes[k];

				R_LerpMD3Frame(animlerp, index, &oldVert[index], &vert[index], v, n);

				glTexCoord2f(texcoord[index].st[0], texcoord[index].st[1]);
				glNormal3fv(n);
				glVertex3fv(v);
			}
		}
		glEnd();

		surface = (md3Surface_t*)((byte*)surface + surface->ofsEnd);
	}
}
#endif


qboolean R_LerpTag(orientation_t* tag, struct model_t* model, int startFrame, int endFrame, float frac, int tagIndex)
{
	return MD3_LerpTag(tag, model, startFrame, endFrame, frac, tagIndex);
}


#if 0
static void MD3_Normals(md3Header_t* pModel, float yangle)
{
	md3Surface_t* surf;
	md3XyzNormal_t* xyz;

	float			angle;
	float			lat, lng;
	float			angleCos, angleSin;
	vec3_t			temp;
	vec3_t			normal;

	angle = yangle / 180 * M_PI;
	angleCos = cos(angle);
	angleSin = sin(angle);

	surf = (md3Surface_t*)((byte*)pModel + pModel->ofsSurfaces);
	for (int i = 0; i < pModel->numSurfaces; i++)
	{
		xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);

		for (int j = 0; j < surf->numVerts; j++, xyz++)
		{
			// decode the lat/lng normal to a 3 float normal
			lat = (xyz->normal >> 8) & 0xff;
			lng = (xyz->normal & 0xff);
			lat *= M_PI / 128;
			lng *= M_PI / 128;

			temp[0] = cos(lat) * sin(lng);
			temp[1] = sin(lat) * sin(lng);
			temp[2] = cos(lng);

			// rotate the normal
			normal[0] = temp[0] * angleCos - temp[1] * angleSin;
			normal[1] = temp[0] * angleSin + temp[1] * angleCos;
			normal[2] = temp[2];

//			printf("yawangle %f = surf %i tri %i normal [%f %f %f]\n", yangle, i, j, normal[0], normal[1], normal[2]);
		}
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}
}
#endif