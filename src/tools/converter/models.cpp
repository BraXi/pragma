
#include "converter.h"

#include "../../qcommon/mod_md3.h"


static void Mod_LoadMD3(model_t* outModel, void* data);

model_t* Mod_LoadModel(const char *modelname, const char *filename)
{
	long fileLen;
	void* pData = NULL;
	model_t* pModel = NULL;

	fileLen = Com_LoadFile(filename, &pData, false);
	if (!pData)
	{
		return NULL;
	}

	if (strlen(modelname) >= (MAX_QPATH - 1))
	{
		Com_Warning("model %s has too long name");
		return NULL;
	}

	pModel = (model_t*)Com_SafeMalloc(sizeof(model_s), __FUNCTION__);
	
	strncpy(pModel->name, modelname, sizeof(pModel->name));
	pModel->type = MOD_BAD;
	pModel->numTextures = 0;
	pModel->pData = pData;
	pModel->dataSize = fileLen;

	// call the apropriate loader
	switch (Com_EndianLong(*(unsigned*)pData))
	{
	case MD3_IDENT:
		Mod_LoadMD3(pModel, pData);
		break;
	}

	if (pModel->type == MOD_BAD)
	{
		Com_Warning("\"%s\" is not a model.", filename);
		free(pData);
		//free(pModel);
		return NULL;
	}

	return pModel;
}


static void Mod_LoadMD3(model_t* outModel, void* pModelData)
{
	md3Header_t* mod;
	md3Frame_t* frame;
	md3Tag_t* tag;
	md3Surface_t* surf;
	md3Shader_t* shader;
	int	i, j;

	mod = (md3Header_t*)pModelData;

	if (outModel->dataSize < sizeof(md3Header_t))
	{
		Com_Warning("Model \"%s\" file is too small.", outModel->name);
		return;
	}

	Com_EndianLong(mod->ident);
	Com_EndianLong(mod->version);
	Com_EndianLong(mod->numFrames);
	Com_EndianLong(mod->numTags);
	Com_EndianLong(mod->numSurfaces);
	Com_EndianLong(mod->ofsFrames);
	Com_EndianLong(mod->ofsTags);
	Com_EndianLong(mod->ofsSurfaces);
	Com_EndianLong(mod->ofsEnd);

	if (mod->ident != MD3_IDENT)
	{
		Com_Warning("Model \"%s\" is not MD3.", outModel->name);
		return;
	}
	if (mod->version != MD3_VERSION)
	{
		Com_Warning("Model \"%s\" has wrong version %i (should be %i).", outModel->name, mod->version, MD3_VERSION);
		return;
	}

	if (mod->numFrames < 1)
	{
		Com_Warning("Model \"%s\" has no frames.", outModel->name);
		return;
	}

	// frames
	if ((mod->ofsFrames + (sizeof(md3Frame_t) * mod->numFrames)) >= outModel->dataSize)
	{
		Com_Warning("Model \"%s\" has corrupt frames.", outModel->name);
		return;
	}

	frame = (md3Frame_t*)((byte*)mod + mod->ofsFrames);
	for (i = 0; i < mod->numFrames; i++, frame++)
	{
		frame->radius = Com_EndianFloat(frame->radius);
		for (j = 0; j < 3; j++)
		{
			frame->bounds[0][j] = Com_EndianFloat(frame->bounds[0][j]);
			frame->bounds[1][j] = Com_EndianFloat(frame->bounds[1][j]);
			frame->localOrigin[j] = Com_EndianFloat(frame->localOrigin[j]);
		}
	}

	// tags
	if ((mod->ofsTags + (sizeof(md3Tag_t) * mod->numTags)) >= outModel->dataSize)
	{
		Com_Warning("Model \"%s\" has corrupt tags.", outModel->name);
		return;
	}
	tag = (md3Tag_t*)((byte*)mod + mod->ofsTags);
	for (i = 0; i < mod->numTags * mod->numFrames; i++, tag++)
	{
		for (j = 0; j < 3; j++)
		{
			tag->origin[j] = Com_EndianFloat(tag->origin[j]);
			tag->axis[0][j] = Com_EndianFloat(tag->axis[0][j]);
			tag->axis[1][j] = Com_EndianFloat(tag->axis[1][j]);
			tag->axis[2][j] = Com_EndianFloat(tag->axis[2][j]);
		}
		_strlwr(tag->name);
	}

	// surfaces
	if ((mod->ofsSurfaces + (sizeof(md3Surface_t) * mod->numSurfaces)) >= outModel->dataSize)
	{
		Com_Warning("Model \"%s\" has corrupt surfaces.", outModel->name);
		return;
	}

	surf = (md3Surface_t*)((byte*)mod + mod->ofsSurfaces);
	for (i = 0; i < mod->numSurfaces; i++)
	{
		Com_EndianLong(surf->ident);
		Com_EndianLong(surf->flags);
		Com_EndianLong(surf->numFrames);
		Com_EndianLong(surf->numShaders);
		Com_EndianLong(surf->numTriangles);
		Com_EndianLong(surf->ofsTriangles);
		Com_EndianLong(surf->numVerts);
		Com_EndianLong(surf->ofsShaders);
		Com_EndianLong(surf->ofsSt);
		Com_EndianLong(surf->ofsXyzNormals);
		Com_EndianLong(surf->ofsEnd);

		//_strlwr(surf->name);

		if (surf->numVerts > MD3_MAX_VERTS)
			Com_Warning("Model %s has surface \"%s\" with %i vertexes which is over soft limit of %i.", mod->name, surf->name, surf->numVerts, MD3_MAX_VERTS);

		if (surf->numTriangles > MD3_MAX_TRIANGLES)
			Com_Warning("Model %s has surface \"%s\" with %i triangles which is over soft limit of %i.", mod->name, surf->name, surf->numTriangles, MD3_MAX_TRIANGLES);

		// strip off a trailing _1 or _2
		j = (int)strlen(surf->name);
		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// shaders
		shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
		for (j = 0; j < surf->numShaders; j++, shader++)
		{
			// pragma hack, textures starting with $ are generated in C code
			if (shader->name[0] == '$')
				continue;

			if (outModel->numTextures >= 32)
			{
				Com_Error("increase max model textures.");
			}
			strncpy(outModel->textureNames[outModel->numTextures], shader->name, sizeof(outModel->textureNames[0]));
			outModel->numTextures++;
		}

#if 0
		int k;
		md3Triangle_t* tri;
		md3St_t* st;
		md3XyzNormal_t* xyz;
		// swap all the triangles
		tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
		for (j = 0; j < surf->numTriangles; j++, tri++)
		{
			for (k = 0; k < 3; k++)
				Com_EndianLong(tri->indexes[k]);
		}

		// swap all the ST
		st = (md3St_t*)((byte*)surf + surf->ofsSt);
		for (j = 0; j < surf->numVerts; j++, st++)
		{
			for (k = 0; k < 2; k++)
				st->st[k] = Com_EndianFloat(st->st[k]);
		}

		// swap all the XyzNormals
		xyz = (md3XyzNormal_t*)((byte*)surf + surf->ofsXyzNormals);
		for (j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++)
		{
			for (k = 0; k < 3; k++)
				xyz->xyz[k] = Com_EndianShort(xyz->xyz[k]);
			xyz->normal = Com_EndianShort(xyz->normal);
		}
#endif
		// find the next surface
		if ((mod->ofsSurfaces + (sizeof(md3Surface_t) * i) + surf->ofsEnd) > outModel->dataSize)
		{
			Com_Warning("Model \"%s\" has corrupt surface.", outModel->name);
			return;
		}

		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}

	outModel->type = MOD_MD3;
	return;
}


