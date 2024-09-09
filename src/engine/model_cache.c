/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*

com_model.c - general model cache
the general idea here is to load assets once and for good because both server and client share much data in common

*/

#include "../qcommon/qcommon.h"
#include "server/server.h"
#include "client/client.h" // same, for renderer

#include "model_cache.h"

//unsigned int com_registration_sequence;

static cached_model_t com_cachedModelsList[MAX_MODELS];
static unsigned int com_numCachedModels;



static void Com_LoadSkelModel(cached_model_t* mod, void* buffer, const int filelen); // .mod
static void Com_LoadAliasModel(cached_model_t* mod, void* buffer, const int filelen); // .md3


/*
=================
Mod_NumCachedModels
Returns the number of precached models
=================
*/
const int Mod_NumCachedModels()
{
	return com_numCachedModels;
}

/*
=================
Mod_HasRenderData
Returns true if the model has loaded additional data for renderer
=================
*/
qboolean Mod_HasRenderData(cached_model_t *pModel)
{
#ifdef _DEBUG
	if(pModel == NULL || !pModel)
		Com_Error(ERR_FATAL, "%s: !pModel\n", __FUNCTION__);
#endif
	return (pModel && pModel->pRenderData != NULL && pModel->renderDataSize > 0);
}


/*
=================
Mod_ForName
=================
*/
static cached_model_t* Mod_ForName(const cacheuser_t user, const char* name, const qboolean bMustLoad)
{
	cached_model_t* mod;
	unsigned* buf;
	int index, filelen;

	if (name[0] == (0 || '\0'))
		Com_Error(ERR_DROP, "%s: NULL name.\n", __FUNCTION__);

	// check if its been already precached
	for (index = 0, mod = com_cachedModelsList; index < Mod_NumCachedModels(); index++, mod++)
	{
		if (!mod->name[0])
			continue;

		if (!Q_stricmp((const char*)mod->name, name)) // braxi: made it case insensitive
		{
			//if (user > mod->user)
			//{
				// if the model is loaded by server and later renderer needs it too, bump the user
			//	mod->user = user;
			//}

			return mod;
		}
	}

	for (index = 0, mod = com_cachedModelsList; index < Mod_NumCachedModels(); index++, mod++)
	{
		if (!mod->name[0])
			break; // free spot
	}

	if (index == Mod_NumCachedModels())
	{
		if (Mod_NumCachedModels() >= MAX_MODELS)
		{
			Com_Error(ERR_DROP, "%s: too many model precaches.\n", __FUNCTION__, MAX_MODELS);
			return NULL;
		}
		com_numCachedModels++;
	}

	memset(mod, 0, sizeof(cached_model_t));

	strncpy(mod->name, name, sizeof(mod->name));
	mod->type = MOD_BAD;
	//mod->user = user;

	// inline models are grabbed only from BSP
	if (name[0] == '*') // *1, *2 ...
	{
		int bmodel = atoi(name + 1); 

#ifdef _DEBUG
		if (CM_NumInlineModels() == 0)
			Com_DPrintf(DP_ALL, "%s: Cannot cache inline models, no map loaded.\n", __FUNCTION__, bmodel);
#endif

		if (bmodel < 1 || bmodel >= CM_NumInlineModels())
		{
			// whe no world is loaded
			Com_Error(ERR_DROP, "%s: bad inline model number %i.\n", __FUNCTION__, bmodel);
		}
		mod->type = MOD_BRUSH;
		mod->brush = CM_InlineModel(name);
		return mod;
	}

	// all the others are loaded from file
	filelen = FS_LoadFile(mod->name, &buf);
	if (!buf)
	{
		if (bMustLoad)
			Com_Error(ERR_DROP, "%s: model '%s' not found.\n", __FUNCTION__, mod->name);
		else
			Com_Printf(PRINT_LOW, "%s: model '%s' not found.\n", __FUNCTION__, mod->name);

		memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}

	switch (LittleLong(*(unsigned*)buf))
	{
	case BSP_IDENT:
	case QBISM_IDENT:
		// this will be fun...

	case PMODEL_IDENT:
		break;

	case MD3_IDENT:
		break;

//	case SPRITE_IDENT:
//		break;
	default:
		Com_Error(ERR_DROP, "%s: '%s' is not a model.", mod->name);
		break;
	}

	FS_FreeFile(buf);

	return mod;
}

/*
=================
Mod_Register
=================
*/
cached_model_t* Mod_Register(const cacheuser_t user, const char* name, const qboolean bMustLoad)
{
	cached_model_t* mod;

	mod = Mod_ForName(user, name, bMustLoad);
	if (!mod || mod == NULL)
		return mod;

	//if (user > mod->user)
	//{
	//	mod->user = user;
	//}

	return mod;
}

/*
=================
Mod_Register
=================
*/
void Mod_Free(cached_model_t* pModel)
{

}


/*
=================
Mod_FreeRendererData
=================
*/
void Mod_FreeRendererData(cached_model_t* pModel)
{
	if (!pModel)
		return;

	if (Mod_HasRenderData(pModel))
	{
		//re.FreeModel(pModel);
		Hunk_Free(pModel->pRenderData);
		pModel->pRenderData = NULL;
	}
}

/*
=================
Mod_FreeUnused
=================
*/
void Mod_FreeUnused(const cacheuser_t user)
{
}

/*
=================
Mod_FreeUnused
=================
*/
void Mod_FreeAll()
{
}


/*
=================
AllocCachedModelData
=================
*/
static void AllocCachedModelData(cached_model_t *pModel, int maxSize, const char *str)
{
//#ifdef _DEBUG
	if (pModel == NULL || !pModel)
	{
		Com_Error(ERR_FATAL, "%s: !pModel\n", __FUNCTION__);
		return; // msvc
	}

	if (pModel && pModel->pData)
	{
		Com_Error(ERR_FATAL, "Called %s but %s has pData!\n", __FUNCTION__, pModel->name);
		return; // msvc
	}
//#endif

	pModel->pData = Hunk_Begin(maxSize, str);
	//pModel->dataSize = maxSize;
}


/*
=================
Mod_GetSkeletonPtr
Returns pointer to bone transformations for reference skeleton
=================
*/
mat4_t* Mod_GetInverseSkeletonMatrix(const cached_model_t* pModel)
{
	if (!pModel || pModel->type != MOD_NEWFORMAT || !pModel->skel)
		return NULL;

	if (!pModel->pInverseSkeleton)
		return NULL;

	return pModel->pInverseSkeleton;
}

/*
=================
Mod_GetBonesPtr
Returns pointer to bones array of a model.
=================
*/
pmodel_bone_t* Mod_GetBonesPtr(const cached_model_t* pModel)
{
	if (!pModel || pModel->type != MOD_NEWFORMAT || !pModel->skel)
		return NULL;
	return (pmodel_bone_t*)((byte*)pModel->skel + pModel->skel->ofs_bones);
}

/*
=================
Mod_GetSkeletonPtr
Returns pointer to bone transformations for reference skeleton
=================
*/
panim_bonetrans_t* Mod_GetSkeletonPtr(const cached_model_t* pModel)
{
	if (!pModel || pModel->type != MOD_NEWFORMAT || !pModel->skel)
		return NULL;
	return (panim_bonetrans_t*)((byte*)pModel->skel + pModel->skel->ofs_skeleton);
}


/*
=================
Mod_GetVertexesPtr
Returns pointer to vertex array.
=================
*/
pmodel_vertex_t* Mod_GetVertexesPtr(const cached_model_t* pModel)
{
	if (!pModel || pModel->type != MOD_NEWFORMAT || !pModel->skel)
		return NULL;
	return (pmodel_vertex_t*)((byte*)pModel->skel + pModel->skel->ofs_vertexes);
}

/*
=================
Mod_GetSurfacesPtr
Returns pointer to surfaces
=================
*/
void* Mod_GetSurfacesPtr(const cached_model_t* pModel)
{
	if (!pModel)
		return NULL;

	if (pModel->skel && pModel->type == MOD_NEWFORMAT)
		return (pmodel_surface_t*)((byte*)pModel->skel + pModel->skel->ofs_surfaces);
	else if (pModel->alias && pModel->type == MOD_ALIAS)
		return (md3Surface_t*)((byte*)pModel->alias + pModel->alias->ofsSurfaces);

	return NULL;
}

/*
=================
Mod_GetPartsPtr
Returns pointer to parts
=================
*/
void* Mod_GetPartsPtr(const cached_model_t* pModel)
{
	if (!pModel)
		return NULL;

	if(pModel->skel && pModel->type == MOD_NEWFORMAT)
		return (pmodel_part_t*)((byte*)pModel->skel + pModel->skel->ofs_parts);
	else if(pModel->alias && pModel->type == MOD_ALIAS) // not sure if this should be honestly here
		return (md3Surface_t*)((byte*)pModel->alias + pModel->alias->ofsSurfaces);

	return NULL;
}

/*
=================
Mod_GetType
=================
*/
modtype_t Mod_GetType(const cached_model_t* pModel)
{
	if (!pModel)
		return MOD_BAD;
	return pModel->type;
}

/*
=================
Mod_NumVerts
=================
*/
int Mod_NumVerts(const cached_model_t* pModel)
{
	int count = 0;

	if (pModel)
	{
		switch (pModel->type)
		{
		case MOD_NEWFORMAT:
			assert(pModel->skel);
			count = (int)pModel->skel->numVertexes;
			break;

		default:
			Com_Printf("todo %s for modtype %i\n", __FUNCTION__, pModel->type);
			break;
		}
	}

	return count;
}

/*
=================
Mod_NumFrames
=================
*/
int Mod_NumFrames(const cached_model_t* pModel)
{
	if (!pModel || pModel && pModel->type == MOD_BAD)
		return 0;
	return pModel->numFrames;
}

/*
=================
Mod_NumParts
=================
*/
int Mod_NumParts(const cached_model_t* pModel)
{
	if (!pModel || pModel && pModel->type == MOD_BAD)
		return 0;
	return pModel->numParts;
}


/*
=================
Mod_NumBones
=================
*/
int Mod_NumBones(const cached_model_t* pModel)
{
	if (!pModel || pModel && pModel->type == MOD_BAD)
		return 0;
	return pModel->numBones;
}

/*
=================
Mod_PartIndexForName

Returns the part index for name or -1 if not found.

md3s have single surface per part
pmodels have parts that consist of multiple surfaces
=================
*/
int Mod_PartIndexForName(const cached_model_t* pModel, const char* partName)
{
	md3Surface_t* surf;
	pmodel_part_t* part;
	int i, numparts;

	if (!pModel || pModel && pModel->type == MOD_BAD)
		return -1;

	numparts = Mod_NumParts(pModel);

	switch (Mod_GetType(pModel))
	{
	case MOD_NEWFORMAT:
		part = (pmodel_part_t*)Mod_GetPartsPtr(pModel);
		for (i = 0; i < numparts; i++, part++)
		{
			if (!strcmp(part->name, partName))
			{
				return i;
			}
		}
		break;

	case MOD_ALIAS:
		surf = (md3Surface_t*)Mod_GetSurfacesPtr(pModel);
		for (i = 0; i < numparts; i++, surf++)
		{
			if (!strcmp(surf->name, partName))
			{
				return i;
			}
		}
		break;

	default:
		// anything else (brushes and sprites) are single part, but -1 here
		return -1;
		break;
	}

	return -1;
}

/*
====================
Mod_GetCullRadius
====================
*/
float Mod_GetCullRadius(const cached_model_t* pModel)
{
	float radius = 32.0f;

	switch (Mod_GetType(pModel))
	{
	case MOD_ALIAS:
		assert(pModel->alias);
		md3Frame_t* frame = (md3Frame_t*)((byte*)pModel->alias + pModel->alias->ofsFrames);
		radius = frame->radius;
		break;

	default:
		Com_Printf("todo %s for modtype %i\n", __FUNCTION__, pModel->type);
		break;
	}
	return radius;
}

/*
=================
Com_LoadSkelModel
Loads the `.mod` model, pragma's own format.
=================
*/
static void Com_LoadSkelModel(cached_model_t* mod, void* buffer, const int filelen)
{
	pmodel_header_t* pHdr;
	pmodel_bone_t* bone;
	panim_bonetrans_t* trans;
	pmodel_vertex_t* vert;
	pmodel_surface_t* surf;
	pmodel_part_t* part;
	int  i, j;

	if (filelen <= sizeof(pmodel_header_t))
	{
		Com_Error(ERR_DROP, "%s: model '%s' has weird size.\n", __FUNCTION__, mod->name);
		return;
	}

	pHdr = (pmodel_header_t*)buffer;

	pHdr->ident = LittleLong(pHdr->ident);
	pHdr->version = LittleLong(pHdr->version);
	pHdr->flags = LittleLong(pHdr->flags);
	pHdr->numBones = LittleLong(pHdr->numBones);
	pHdr->numVertexes = LittleLong(pHdr->numVertexes);
	pHdr->numSurfaces = LittleLong(pHdr->numSurfaces);
	pHdr->numParts = LittleLong(pHdr->numParts);

	for (i = 0; i < 3; i++)
	{
		pHdr->mins[i] = LittleFloat(pHdr->mins[i]);
		pHdr->maxs[i] = LittleFloat(pHdr->maxs[i]);
	}

	// do a basic validation
	if (pHdr->ident != PMODEL_IDENT)
	{
		Com_Error(ERR_DROP, "'%s' is not an model.\n", mod->name);
		return;
	}

	if (pHdr->version != PMODEL_VERSION)
	{
		Com_Error(ERR_DROP, "Model '%s' has wrong version %i (should be %i).\n", mod->name, pHdr->version, PMODEL_VERSION);
		return;
	}

	if (pHdr->numBones < 1)
	{
		Com_Error(ERR_DROP, "Model '%s' has no bones.\n", mod->name);
		return;
	}

	if (pHdr->numVertexes < 3)
	{
		Com_Error(ERR_DROP, "Model '%s' has too few vertexs.\n", mod->name);
		return;
	}

	if (pHdr->numSurfaces < 1)
	{
		Com_Error(ERR_DROP, "Model '%s' has no surfaces.\n", mod->name);
		return;
	}

	if (pHdr->numParts < 1)
	{
		Com_Error(ERR_DROP, "Model '%s' has no parts.\n", mod->name);
		return;
	}

	pHdr->ofs_bones = sizeof(pmodel_header_t);
	pHdr->ofs_skeleton = (pHdr->ofs_bones + (sizeof(pmodel_bone_t) * pHdr->numBones));
	pHdr->ofs_vertexes = (pHdr->ofs_skeleton + (sizeof(panim_bonetrans_t) * pHdr->numBones));
	pHdr->ofs_surfaces = (pHdr->ofs_vertexes + (sizeof(pmodel_vertex_t) * pHdr->numVertexes));
	pHdr->ofs_parts = (pHdr->ofs_surfaces + (sizeof(pmodel_surface_t) * pHdr->numSurfaces));
	pHdr->ofs_end = (pHdr->ofs_parts + (sizeof(pmodel_part_t) * pHdr->numParts));

	if (pHdr->ofs_end > filelen)
	{
		Com_Error(ERR_DROP, "Model '%s' is corrupt.\n",  mod->name);
		return;
	}

	// allocate model
	AllocCachedModelData(mod, (filelen + 1), mod->name);	
	mod->skel = Hunk_Alloc(filelen);
	memcpy(mod->skel, buffer, filelen);
	mod->dataSize = Hunk_End();

	mod->type = MOD_NEWFORMAT;
	mod->numFrames = 1; // pragma models have binding pose
	mod->numParts = pHdr->numParts;
	mod->numBones = pHdr->numBones;

	// bones
	//assert(bone);
	bone = (pmodel_bone_t*)((byte*)mod->skel + pHdr->ofs_bones);
	for (i = 0; i < pHdr->numBones; i++, bone++)
	{
		bone->number = LittleLong(bone->number);
		bone->parentIndex = LittleLong(bone->parentIndex);

		for (j = 0; j < 3; j++)
		{
			bone->mins[j] = LittleLong(bone->mins[j]);
			bone->maxs[j] = LittleLong(bone->maxs[j]);
		}
	}

	// skeleton
	trans = Mod_GetSkeletonPtr(mod);
	//assert(trans);
	for (i = 0; i < pHdr->numBones; i++, trans++)
	{
		trans->bone = LittleLong(trans->bone);
		for (j = 0; j < 3; j++)
		{
			trans->origin[j] = LittleLong(trans->origin[j]);
			trans->rotation[j] = LittleLong(trans->rotation[j]);
		}
	}

	// vertexes
	vert = Mod_GetVertexesPtr(mod);
	//assert(vert);
	for (i = 0; i < pHdr->numVertexes; i++, vert++)
	{
		vert->boneId = LittleLong(vert->boneId);
		for (j = 0; j < 3; j++)
		{
			vert->xyz[j] = LittleFloat(vert->xyz[j]);
			vert->normal[j] = LittleFloat(vert->normal[j]);
		}

		vert->uv[0] = LittleFloat(vert->uv[0]);
		vert->uv[1] = -LittleFloat(vert->uv[1]); // FIXME: negated T, but its wrong TGA's fault!
	}

	// surfaces
	surf = (pmodel_surface_t*)Mod_GetSurfacesPtr(mod);
	//assert(surf);
	for (i = 0; i < pHdr->numSurfaces; i++, surf++)
	{
		surf->firstVert = LittleLong(surf->firstVert);
		surf->numVerts = LittleLong(surf->numVerts);
	}

	// parts
	part = (pmodel_part_t*)Mod_GetPartsPtr(mod);
	//assert(part);
	for (i = 0; i < pHdr->numParts; i++, part++)
	{
		part->firstSurf = LittleLong(part->firstSurf);
		part->numSurfs = LittleLong(part->numSurfs);
	}
}


/*
=================
Com_LoadAliasModel
Loads the `.md3` model.
=================
*/
void Com_LoadAliasModel(cached_model_t* mod, void* buffer, const int filelen)
{
	md3Header_t* pHdr;
	md3Frame_t* frame;
	md3Surface_t* surf;
	md3Triangle_t* tri;
	md3St_t* st;
	md3XyzNormal_t* xyz;
	md3Tag_t* tag;
	int	i, j;

	if (filelen <= sizeof(md3Header_t))
	{
		Com_Error(ERR_DROP, "Model '%s' has weird size.\n", mod->name);
		return;
	}

	pHdr = (md3Header_t*)buffer;

	pHdr->ident = LittleLong(pHdr->ident);
	pHdr->version = LittleLong(pHdr->version);
	pHdr->numFrames = LittleLong(pHdr->numFrames);
	pHdr->numTags = LittleLong(pHdr->numTags);
	pHdr->numSurfaces = LittleLong(pHdr->numSurfaces);
	pHdr->ofsFrames = LittleLong(pHdr->ofsFrames);
	pHdr->ofsTags = LittleLong(pHdr->ofsTags);
	pHdr->ofsSurfaces = LittleLong(pHdr->ofsSurfaces);
	pHdr->ofsEnd = LittleLong(pHdr->ofsEnd);

	if (pHdr->ident != MD3_IDENT)
	{
		Com_Error(ERR_DROP, "'%s' is not an model.\n", mod->name);
		return;
	}

	if (pHdr->version != MD3_VERSION)
	{
		Com_Error(ERR_DROP, "Model '%s' has wrong version %i (should be %i).\n", __FUNCTION__, mod->name, pHdr->version, MD3_VERSION);
		return;
	}

	if (filelen != pHdr->ofsEnd)
	{
		Com_Error(ERR_DROP, "Model '%s' is corrupt.\n",  mod->name);
		return;
	}

	// mod->type is set later on (we can still error at this point)
	mod->numFrames = pHdr->numFrames;
	mod->numParts = pHdr->numSurfaces;
	mod->numBones = 0; // alias models have no bones

	if (mod->numFrames < 1)
	{
		Com_Error(ERR_DROP, "Model '%s' has no frames.\n", mod->name);
		return;
	}

	// alloc model
	AllocCachedModelData(mod, (filelen + 1), mod->name);
	memcpy(mod->alias, buffer, filelen);
	mod->dataSize = Hunk_End();

	// swap all the frames
	frame = (md3Frame_t*)((byte*)mod->alias + mod->alias->ofsFrames);
	for (i = 0; i < mod->alias->numFrames; i++, frame++)
	{
		frame->radius = LittleFloat(frame->radius);
		for (j = 0; j < 3; j++) 
		{
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
		surf->ident = LittleLong(surf->ident);
		surf->flags = LittleLong(surf->flags);
		surf->numFrames = LittleLong(surf->numFrames);
		surf->numShaders = LittleLong(surf->numShaders);
		surf->numTriangles = LittleLong(surf->numTriangles);
		surf->ofsTriangles = LittleLong(surf->ofsTriangles);
		surf->numVerts = LittleLong(surf->numVerts);
		surf->ofsShaders = LittleLong(surf->ofsShaders);
		surf->ofsSt = LittleLong(surf->ofsSt);
		surf->ofsXyzNormals = LittleLong(surf->ofsXyzNormals);
		surf->ofsEnd = LittleLong(surf->ofsEnd);

		if (surf->numVerts > MD3_MAX_VERTS)
		{
			Com_Error(ERR_DROP, "Model '%s' has more too many vertexes in part '%s'.", mod->name, surf->name);
		}

		if (surf->numTriangles > MD3_MAX_TRIANGLES)
		{
			Com_Error(ERR_DROP, "Model '%s' has too many triangles in part '%s'.", mod->name, surf->name);
		}

		// lowercase the surface name so skin compares are faster
		_strlwr(surf->name);

		// strip off a trailing _1 or _2
		j = strlen(surf->name);
		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		// swap all the triangles
		tri = (md3Triangle_t*)((byte*)surf + surf->ofsTriangles);
		for (j = 0; j < surf->numTriangles; j++, tri++)
		{
			tri->indexes[0] = LittleLong(tri->indexes[0]);
			tri->indexes[1] = LittleLong(tri->indexes[1]);
			tri->indexes[2] = LittleLong(tri->indexes[2]);
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

	//mod->radius = MD3_ModelRadius(mod->index);
	//MD3_ModelBounds(mod->index, mod->mins, mod->maxs);
}