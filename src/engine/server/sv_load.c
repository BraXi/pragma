/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// sv_load.c - asset loading

#include "server.h"

qboolean ModelDef_LoadFile(const char* filename, modeldef_t* def);

static void SV_LoadMD3(svmodel_t* out, void* buffer);
static svmodel_t* SV_LoadModel(const char* name, qboolean crash);

static qboolean SV_FileExists(const char* name, qboolean crash);
static int SV_FindOrCreateAssetIndex(const char* name, int start, int max, const char* func);
/*
================
SV_ModelIndex
Find model index, load model if not present.
================
*/
int SV_ModelIndex(const char* name)
{
	int bmodelnum;

	if (!name)
		return 0;

	// handle inline models explictly
	if (name[0] == '*' && strlen(name) >= 2)
	{
		bmodelnum = atoi(name + 1);
		return (0 - bmodelnum);
	}

	return SV_FindOrCreateAssetIndex(name, CS_MODELS, MAX_MODELS, __FUNCTION__);
}

/*
================
SV_SoundIndex
Find sound index.
================
*/
int SV_SoundIndex(const char* name)
{
	return SV_FindOrCreateAssetIndex(name, CS_SOUNDS, MAX_SOUNDS, __FUNCTION__);
}

/*
================
SV_ImageIndex
Find image index.
================
*/
int SV_ImageIndex(const char* name)
{
	return SV_FindOrCreateAssetIndex(name, CS_IMAGES, MAX_IMAGES, __FUNCTION__);
}

/*
================
SV_ModelForNum
Returns server model for given index
Brush models will default to the NOMODEL
================
*/
svmodel_t* SV_ModelForNum(int index)
{
	svmodel_t* mod;

	if (SV_IsBrushModel(index))
	{
		return &sv.models[0];
	}

	if (index > sv.numModels || index < 0) 
	{
		Com_Error(ERR_DROP, "SV_ModelForNum: wrong index %i\n", index);
		return NULL; 
	}
	mod = &sv.models[index];

	return mod;
}

/*
================
SV_ModelIndexForName

returns model index for name
================
*/
int SV_ModelIndexForName(const char *name)
{
	svmodel_t* mod;
	int num;

	if (name[0] == '*')
	{
		num = atoi(name + 1);

		if (num >= CM_NumInlineModels() || num < 1)
		{
			SV_Error("%s: bad inline model %i\n", __FUNCTION__, num);
		}

		return (0 - num);
	}

	//model = &sv.models[0];
	for (mod = sv.models, num = 0; num < sv.numModels; num++, mod++)
	{
		if (!mod->name[0] || mod->type == MOD_BAD)
			continue;
		if (!strcmp(mod->name, name))
			return mod->modelindex;
	}

	return 0;
}

/*
================
SV_IsBrushModel
================
*/
qboolean SV_IsBrushModel(int modelindex)
{
	if (modelindex == MODELINDEX_WORLD)
		return true; // world

	if (modelindex < 0 && modelindex >= (0 - CM_NumInlineModels()))
		return true; // bmodels are indexed negative

	return false;
}

/*
================
SV_FindOrCreateAssetIndex

Used to load or find server assets (currently only loads only th important MD3 info).
================
*/
static int SV_FindOrCreateAssetIndex(const char* name, int start, int max, const char* func)
{
	static char fullname[MAX_QPATH];
	int		index;

	if (!name || !name[0])
		return 0;

	if (start == CS_MODELS && name[0] == '*')
	{
		SV_Error("Asset index for inline model!");
	}

	//
	//  return early if asset has been indexed
	//
	for (index = 1; index < max && sv.configstrings[start + index][0]; index++)
		if (!strcmp(sv.configstrings[start + index], name))
			return index;

	//
	// load asset
	//
	if (index == max)
		SV_Error("Hit limit of %i assets (%s)", max, func);

	//warn of late precaches or crash depending on sv_nolateloading
	if (sv.state == ss_game)
	{
		if (sv_nolateloading->value > 0)
			SV_Error("Precache '%s' too late.\n", name);
		else
			Com_Printf("WARNING: '%s' not precached (%s)\n", name, func);
	}

	if (start == CS_MODELS /*&& (name[0] != '*')*/)
	{
		if(SV_LoadModel(name, true) == NULL)
			return 0;
	}

	if (start == CS_SOUNDS)
	{
		Com_sprintf(fullname, sizeof(fullname), "sound/%s", name);
		if (SV_FileExists(fullname, false) == false)
			return 0;
	}
	
	if (start == CS_IMAGES)
	{
		Com_sprintf(fullname, sizeof(fullname), "gfx/%s.tga", name);
		if (SV_FileExists(fullname, false) == false)
			return 0;
	}
	
	// update configstring and send updates if necessary
	SV_SetConfigString(start + index, name);

	return index;
}

/*
=================
SV_FileExists
=================
*/
static qboolean SV_FileExists(const char* name, qboolean crash)
{
	int	fileLen;

	if (!name[0])
	{
		Com_Error(ERR_DROP, "%s: NULL name", __FUNCTION__);
		return false;
	}

	fileLen = FS_LoadFile(name, NULL);
	if (fileLen == -1)
	{
		if (crash)
			Com_Error(ERR_DROP, "'%s' not found", name);
		else
			Com_Printf("WARNING: '%s' not found\n", name);

		return false;
	}
	return true;
}

/*
=================
SV_LoadDefForModel

Loads the .def file for a model when present
=================
*/
static void SV_LoadDefForModel(svmodel_t* model)
{
	char defname[MAX_QPATH];
	int i;

	memset(&model->def, 0, sizeof(model->def));

	if (model->type != MOD_ALIAS)
		return;

	strcpy(defname, model->name);
	COM_StripExtension(model->name, defname);
	strcat(defname, ".def");

	// first anim is null
	strcat(model->def.anims[0].name, "*none*");
	model->def.anims[0].firstFrame = 0;
	model->def.anims[0].numFrames = 0;
	model->def.anims[0].lastFrame = 0;
	model->def.numAnimations = 1;

	if (ModelDef_LoadFile(defname, &model->def))
	{
		// def found, see if anims are trully correct
		for (i = 0; i < model->def.numAnimations; i++)
		{
			if (model->def.anims[i].lastFrame > model->numFrames)
			{
				Com_Error(ERR_DROP, "Model '%s' has %i frames but animation '%s' exceeds total number of frames\n", model->name, model->numFrames, model->def.anims[i].lastFrame);
			}
		}
	}
	else
	{
		// no def file
		strcat(model->def.anims[1].name, "*animation*");
		model->def.anims[1].firstFrame = 0;
		model->def.anims[1].numFrames = model->numFrames;
		model->def.anims[1].lastFrame = model->def.anims[1].numFrames;
		model->def.numAnimations = 2;
	}
}

/*
=================
SV_FreeModels
=================
*/
void SV_FreeModels()
{
	svmodel_t* mod;

	if(sv.numModels)
		Com_Printf("Freeing %i models (server)...\n", sv.numModels);

	for (int i = 0; i < MAX_MODELS; i++)
	{
		mod = &sv.models[i];
		if (mod->extradata)
		{
			Hunk_Free(mod->extradata);
		}
		memset(&sv.models[i], 0, sizeof(svmodel_t));
	}
	sv.numModels = 0;
}



/*
=================
SV_LoadModel
=================
*/
static svmodel_t* SV_LoadModel(const char* name, qboolean crash)
{
	svmodel_t	*model;
	unsigned	*buf;
	int			fileLen, i;

	crash = false;
	if (!name[0])
	{
		Com_Error(ERR_DROP, "SV_LoadModel: NULL name");
		return NULL;
	}

	if (sv.numModels == MAX_MODELS)
	{
		Com_Error(ERR_DROP, "SV_LoadModel: hit limit of %d models", MAX_MODELS);
		return NULL; // shut up compiler
	}


	// search the currently loaded models
	// if were coming from *Index its been already searched but be paranoid!
	for (i = 0, model = &sv.models[MODELINDEX_WORLD]; i < MAX_MODELS; i++, model++)
	{
		if (!model->name[0])
			continue;
		if (!strcmp(model->name, name))
			return model;
	}

	// find a free model slot spot
//	for (i = 0, model = &sv.models[MODELINDEX_WORLD]; i < MAX_MODELS; i++, model++)
//	{
//		if (model->type == MOD_BAD || !model->name[0])
//			break;	// free spot
//	}
	model = &sv.models[sv.numModels];
	model->modelindex = sv.numModels;
	
	//
	// load the file
	//
	strcpy(model->name, name);
	model->type = MOD_BAD;
	fileLen = FS_LoadFile(model->name, &buf);
	if (!buf)
	{
		if (crash)
			Com_Error(ERR_DROP, "model '%s' not found", model->name);
		memset(model->name, 0, sizeof(model->name));
		return NULL;
	}

	// call the apropriate loader
	switch (LittleLong(*(unsigned*)buf))
	{
	case MD3_IDENT:
		SV_LoadMD3(model, buf);
		break;
	case PMODEL_IDENT:
		FS_FreeFile(buf);
		Com_Error(ERR_FATAL, "unimplemented in %s", __FUNCTION__);
	default:
		break;
	}

	FS_FreeFile(buf);
	model->extradatasize = Hunk_End();

	if (model->type == MOD_BAD)
	{
		Com_Error(ERR_DROP, "Could not precache model '%s'\n", model->name);
		return NULL;
	}


	SV_LoadDefForModel(model);
	sv.numModels++;
	return model;
}

/*
=================
SV_LoadMD3
=================
*/
static void SV_LoadMD3(svmodel_t* mod, void* buffer)
{
	int				i, j;
	md3Header_t* in;
//	md3Frame_t* frame;
	md3Tag_t* tag;
	md3Surface_t* surf;

	in = (md3Header_t*)buffer;

	LittleLong(in->ident);
	LittleLong(in->version);
	LittleLong(in->numFrames);
	LittleLong(in->numTags);
	LittleLong(in->numSurfaces);
	LittleLong(in->ofsFrames);
	LittleLong(in->ofsTags);
	LittleLong(in->ofsSurfaces);
	LittleLong(in->ofsEnd);

	if (in->ident != MD3_IDENT)
	{
		Com_Printf("%s: %s is not a model\n", __FUNCTION__, mod->name);
		return;
	}
	if (in->version != MD3_VERSION)
	{
		Com_Printf("%s: '%s' has wrong version (%i should be %i)\n", __FUNCTION__, mod->name, in->version, MD3_VERSION);
		return;
	}

	if (in->numFrames < 1)
	{
		Com_Printf("%s: '%s' is corrupt (doesn't have a single frame)\n", __FUNCTION__, mod->name);
		return;
	}

	if (mod->extradata != NULL)
	{
		Com_Error(ERR_FATAL, "mod->extradata not NULL");
	}

	mod->extradata = Hunk_Begin(1024 * 32, "alias model (server)"); //32k should be sufficient?
	mod->alias = Hunk_Alloc(sizeof(alias_data_t));

	mod->numFrames = in->numFrames;
	mod->numTags = in->numTags;
	mod->numSurfaces = in->numSurfaces;

	if (in->numTags)
	{
		// this is tricky, if we were to copy frames straight from MD3 from file
		// we'd end up with duplicated tagnames * numframes * numtags

		// copy tag names
		memset(mod->alias->tagNames, 0, sizeof(mod->alias->tagNames));
		tag = (md3Tag_t*)((byte*)in + in->ofsTags);
		for (i = 0; i < in->numTags; i++, tag++)
		{
			if (!tag->name[0])
			{
				Com_Error(ERR_DROP, "%s: tag #%i in '%s' has empty name\n", __FUNCTION__, i, mod->name);
			}

			// lowercase the tag name so search compares are faster
			_strlwr(tag->name);
			memcpy(mod->alias->tagNames[i], tag->name, sizeof(tag->name));
		}

		// copy tags
		mod->alias->tagFrames = Hunk_Alloc(sizeof(orientation_t) * in->numTags * in->numFrames);

		tag = (md3Tag_t*)((byte*)in + in->ofsTags);
		for (i = 0; i < in->numTags * in->numFrames; i++, tag++)
		{
			for (j = 0; j < 3; j++)
			{
				tag->origin[j] = LittleFloat(tag->origin[j]);
				tag->axis[0][j] = LittleFloat(tag->axis[0][j]);
				tag->axis[1][j] = LittleFloat(tag->axis[1][j]);
				tag->axis[2][j] = LittleFloat(tag->axis[2][j]);
			}
			VectorCopy(tag->origin, mod->alias->tagFrames[i].origin);
			memcpy(mod->alias->tagFrames[i].axis, tag->axis, sizeof(tag->axis));
		}
	}

	// swap all the surfaces
	surf = (md3Surface_t*)((byte*)in + in->ofsSurfaces);
	for (i = 0; i < in->numSurfaces; i++)
	{
		LittleLong(surf->numFrames);
		LittleLong(surf->numShaders);
		LittleLong(surf->numTriangles);
		LittleLong(surf->numVerts);
		LittleLong(surf->ofsEnd);

		// don't really need these ifs here, but this will probably help a bit
		if (surf->numVerts > MD3_MAX_VERTS)
		{
			Com_Error(ERR_DROP, "SV_LoadMD3: %s has more than %i verts on a surface (%i)\n", mod->name, MD3_MAX_VERTS, surf->numVerts);
		}
		if (surf->numTriangles > MD3_MAX_TRIANGLES)
		{
			Com_Error(ERR_DROP, "SV_LoadMD3: %s has more than %i triangles on a surface (%i)\n", mod->name, MD3_MAX_TRIANGLES, surf->numTriangles);
		}

		// lowercase the surface name so skin compares are faster
		_strlwr(surf->name);

		// strip off a trailing _1 or _2
		j = (int)strlen(surf->name);
		if (j > 2 && surf->name[j - 2] == '_')
		{
			surf->name[j - 2] = 0;
		}

		memcpy(mod->alias->surfNames[i], surf->name, sizeof(surf->name));

		// find the next surface
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);

	}
	mod->type = MOD_ALIAS;
}


/*
=================
SV_ModelSurfIndexForName
=================
*/
int SV_ModelSurfIndexForName(int modelindex, const char* surfaceName)
{
	svmodel_t* mod;
	int index;

	mod = SV_ModelForNum(modelindex);
	if (!mod)
	{
		return -1;
	}

	if (mod->type == MOD_ALIAS)
	{
		if (mod->alias == NULL)
		{
			Com_Error(ERR_DROP, "%s: wrong alias model\n", __FUNCTION__);
			return -1; //msvc
		}
		
		for (index = 0; index < mod->numSurfaces; index++)
		{
			if (!Q_stricmp(mod->alias->surfNames[index], surfaceName))
			{
				return index;
			}
		}
	}
	else
	{
		Com_Error(ERR_DROP, "%s: model type unimplemented\n", __FUNCTION__);
	}
	return -1;
}


/*
=================
SV_TagIndexForName

returns index of a tag or -1 if not found
=================
*/
int SV_TagIndexForName(int modelindex, const char* tagName)
{
	svmodel_t* mod;
	int index;

	mod = SV_ModelForNum(modelindex);
	if (!mod)
	{
		Com_Error(ERR_DROP, "%s: wrong model for index %i\n", __FUNCTION__, modelindex);
		return -1; //doesn't get here
	}


	if (mod->type == MOD_ALIAS)
	{
		if (mod->alias == NULL)
		{
			Com_Error(ERR_DROP, "%s: MD3 but mod->mesh is NULL\n", __FUNCTION__);
			return -1; // msvc
		}
	
		int frame = 0; // will I ever need this here? probably no.
		orientation_t* tagdata = (orientation_t*)((byte*)mod->alias->tagFrames) + (frame * mod->numTags);
		for (index = 0; index < mod->numTags; index++, tagdata++)
		{
			if (!strcmp(mod->alias->tagNames[index], tagName))
			{
				return index; // found it
			}
		}
	}
	else
	{
		Com_Printf("%s: unimplemented model format\n", __FUNCTION__);
		return -1;
	}
	return -1;
}

/*
=================
SV_GetTag

returns orientation_t of a tag for a given frame or NULL if not found
=================
*/
orientation_t* SV_GetTag(int modelindex, int frame, const char* tagName)
{
	svmodel_t* mod;
	int index;

	mod = SV_ModelForNum(modelindex);
	if (!mod)
	{
		Com_Error(ERR_DROP, "%s: wrong model for index %i\n", __FUNCTION__, modelindex);
		return NULL;
	}

	// it is possible to have a bad frame while changing models, so don't error
	if (frame >= mod->numFrames)
		frame = mod->numFrames - 1;
	else if (frame < 0)
		frame = 0;

	if (mod->type == MOD_NEWFORMAT)
	{
		Com_Printf("unimplemented model format in %s\n", __FUNCTION__);
		return NULL;
	}
	else if (mod->type == MOD_ALIAS)
	{
		if (mod->alias == NULL)
		{
			Com_Error(ERR_DROP, "%s: MD3 but mod->mesh is NULL\n", __FUNCTION__);
			return NULL;
		}

		orientation_t* tagdata;
		tagdata = (orientation_t*)((byte*)mod->alias->tagFrames) + (frame * mod->numTags);
		for (index = 0; index < mod->numTags; index++, tagdata++)
		{
			if (!strcmp(mod->alias->tagNames[index], tagName))
			{
				return tagdata; // found it
			}
		}
	}
	return NULL;
}

/*
=================
SV_PositionTag

Returns the tag rotation and origin accounted by given origin and angles
=================
*/

void AngleVectors2(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float        angle;
	float        sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI * 2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
	}
	if (up)
	{
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
		up[2] = cr * cp;
	}
}


orientation_t out;
orientation_t* SV_PositionTag(vec3_t origin, vec3_t angles, int modelindex, int animframe, const char* tagName)
{
	orientation_t	*tag;
	orientation_t    parent;
	vec3_t			tempAxis[3];

	tag = SV_GetTag(modelindex, animframe, tagName);
	if (!tag)
		return NULL;

	AxisClear(parent.axis);
	VectorCopy(origin, parent.origin);
	AnglesToAxis(angles, parent.axis);

	AxisClear(out.axis);
	VectorCopy(parent.origin, out.origin);

//	AngleVectors2(angles, out.axis[0], out.axis[1], out.axis[2]);
//	AnglesToAxis(angles, out.axis);

	AxisClear(tempAxis);

	for (int i = 0; i < 3; i++)
	{
		VectorMA(out.origin, tag->origin[i], parent.axis[i], out.origin);
	}

	// translate origin
//	MatrixMultiply(tag->axis, parent.axis, out.axis); 

	// translate rotation and origin
	MatrixMultiply(out.axis, parent.axis, tempAxis);
	MatrixMultiply(tag->axis, tempAxis, out.axis);
	return &out;
}


/*
=================
SV_PositionTagOnEntity
=================
*/
orientation_t* SV_PositionTagOnEntity(gentity_t* ent, const char* tagName)
{
	orientation_t* tag;
	tag = SV_PositionTag(ent->v.origin, ent->v.angles, (int)ent->v.modelindex, (int)ent->v.animFrame, tagName);
	return tag;
}