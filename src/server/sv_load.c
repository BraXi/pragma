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
// sv_load.c - asset loading

#include "server.h"

static void SV_LoadMD3(svmodel_t* out, void* buffer);
static void SV_LoadSP2(svmodel_t* out, void* buffer);

static svmodel_t* SV_LoadModel(char* name, qboolean crash);
static int SV_FindOrCreateAssetIndex(char* name, int start, int max, const char* func);


/*
================
SV_ModelIndex
Find model index, load model if not present.
================
*/
int SV_ModelIndex(char* name)
{
	return SV_FindOrCreateAssetIndex(name, CS_MODELS, MAX_MODELS, __FUNCTION__);
}

/*
================
SV_SoundIndex
Find sound index.
================
*/
int SV_SoundIndex(char* name)
{
	return SV_FindOrCreateAssetIndex(name, CS_SOUNDS, MAX_SOUNDS, __FUNCTION__);
}

/*
================
SV_ImageIndex
Find image index.
================
*/
int SV_ImageIndex(char* name)
{
	return SV_FindOrCreateAssetIndex(name, CS_IMAGES, MAX_IMAGES, __FUNCTION__);
}

/*
================
SV_ModelForNum

Returns svmodel for index
================
*/
svmodel_t* SV_ModelForNum(unsigned int index)
{
	svmodel_t* mod;

	if (index < 0 || index > sv.num_models) 
	{
		Com_Error(ERR_DROP, "SV_ModelForNum: index %i out of range [0,%i]\n", index, sv.num_models);
		return NULL; // silence warning
	}

	mod = &sv.models[index];
	return mod;
}

/*
================
SV_ModelForName

Returns svmodel
================
*/
svmodel_t* SV_ModelForName(char *name)
{
	svmodel_t* model;
	model = &sv.models[0];
	for (int i = 0; i < sv.num_models; i++, model++)
	{
		if (!model->name[0] || model->type == MOD_BAD)
			continue;
		if (!strcmp(model->name, name))
			return model;
	}
	return NULL;
}

/*
================
SV_FindOrCreateAssetIndex

Used to load or find server assets (currently only loads only th important MD3 info).
================
*/
static int SV_FindOrCreateAssetIndex(char* name, int start, int max, const char* func)
{
	int		index;

	if (!name || !name[0])
		return 0;

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
		Com_Error(ERR_DROP, "%s: hit limit of %i assets", func, max);

	//warn of late precaches or crash depending on sv_nolateloading
	if (sv.state == ss_game)
	{
		if (sv_nolateloading->value > 0)
			Com_Error(ERR_DROP, "%s: tried to precache '%s' but the server is running\n", func, name);
		else
			Com_Printf("WARNING: %s: late loading of '%s', consider precaching it in main()\n",func,  name);
	}

	if(start == CS_MODELS /*&& (name[0] != '*')*/)
		SV_LoadModel(name, true);

	// update configstring
	strncpy(sv.configstrings[start + index], name, sizeof(sv.configstrings[index]));

	if (sv.state != ss_loading)
	{	// send the update to everyone
		SZ_Clear(&sv.multicast);
		MSG_WriteChar(&sv.multicast, SVC_CONFIGSTRING);
		MSG_WriteShort(&sv.multicast, start + index);
		MSG_WriteString(&sv.multicast, name);
		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}

	return index;
}

/*
=================
SV_LoadModel
=================
*/
static svmodel_t* SV_LoadModel(char* name, qboolean crash)
{
	svmodel_t* model;
	unsigned* buf;
	int			fileLen;
	int			i;

	if (!name[0])
	{
		Com_Error(ERR_DROP, "SV_LoadModel: NULL name");
		return NULL;
	}

	//
	// search the currently loaded models
	// if were coming from *Index its been already searched but be paranoid!
	//
	for (i = 0, model = &sv.models[i]; i < MAX_MODELS; i++, model++)
	{
		if (!model->name[0])
			continue;
		if (!strcmp(model->name, name))
			return model;
	}

	//
	// find a free model slot spot
	//
	for (i = 0, model = &sv.models[i]; i < MAX_MODELS; i++, model++)
	{
		if (/*model->type == MOD_BAD ||*/ !model->name[0])
			break;	// free spot
	}

	sv.num_models++;
	if (sv.num_models == MAX_MODELS)
	{
		Com_Error(ERR_DROP, "SV_LoadModel: hit limit of %d models", MAX_MODELS);
		return NULL; // shut up compiler
	}

	crash = false;
	//
	// load the file
	//
	strcpy(model->name, name);
	model->type = MOD_BAD;
	fileLen = FS_LoadFile(model->name, &buf);
	if (!buf)
	{
		if (crash)
			Com_Error(ERR_DROP, "SV_LoadModel: '%s' not found", model->name);
		memset(model->name, 0, sizeof(model->name));
		return NULL;
	}

	// call the apropriate loader
	switch (LittleLong(*(unsigned*)buf))
	{
	case MD3_IDENT:
		SV_LoadMD3(model, buf);
		break;
	case SP2_IDENT:
		SV_LoadSP2(model, buf);
		break;
	default:
		Com_Error(ERR_DROP, "SV_LoadModel: '%s' is not a model", model->name);
	}

	FS_FreeFile(buf);

	if (model->type == MOD_BAD)
	{
		Com_Error(ERR_DROP, "SV_LoadModel: bad model '%s'", model->name);
		return NULL;
	}

	return model;
}

/*
=================
SV_LoadMD3
=================
*/
static void SV_LoadMD3(svmodel_t* out, void* buffer)
{
	int				i, j;
	md3Header_t* in;
//	md3Frame_t* frame;
	md3Tag_t* tag;

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
		Com_Printf("SV_LoadMD3: %s is not a model\n", out->name);
		return;
	}
	if (in->version != MD3_VERSION)
	{
		Com_Printf("SV_LoadMD3: '%s' has wrong version (%i should be %i)\n", out->name, in->version, MD3_VERSION);
		return;
	}

	if (in->numFrames < 1)
	{
		Com_Printf("SV_LoadMD3: '%s' has no frames\n", out->name);
		return;
	}

	out->numFrames = in->numFrames;
	out->numTags = in->numTags;
	out->numSurfaces = out->numSurfaces;

	if (in->numTags)
	{
		// this is tricky, if we were to copy frames straight from MD3 from file
		// we'd end up with duplicated tagnames * numframes * numtags

		// copy tag names
		memset(out->tagNames, 0, sizeof(out->tagNames));
		tag = (md3Tag_t*)((byte*)in + in->ofsTags);
		for (i = 0; i < in->numTags; i++, tag++)
		{
			if (!tag->name[0])
			{
				Com_Error(ERR_DROP, "SV_LoadMD3: tag #%i in '%s' has empty name\n", i, out->name);
			}

			// lowercase the tag name so search compares are faster
			_strlwr(tag->name);
			memcpy(out->tagNames[i], tag->name, sizeof(tag->name));
		}

		// copy tags
		out->tagFrames = Z_TagMalloc(sizeof(orientation_t) * in->numTags * in->numFrames, TAG_SERVER_MODELDATA);

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
			VectorCopy(tag->origin, out->tagFrames[i].origin);
			memcpy(out->tagFrames[i].axis, tag->axis, sizeof(tag->axis));
		}
	}

	out->type = MOD_MD3;
}


/*
=================
SV_LoadSP2
=================
*/
static void SV_LoadSP2(svmodel_t* out, void* buffer)
{
	sp2Header_t* in;

	in = (sp2Header_t*)buffer;

	in->ident = LittleLong(in->ident);
	in->version = LittleLong(in->version);
	in->numframes = LittleLong(in->numframes);

	if( in->version != SP2_VERSION)
		Com_Error(ERR_DROP, "SV_LoadSP2: '%s' is wrong version %i", out->name, in->version);

	if (in->numframes > 32)
		Com_Error(ERR_DROP, "SV_LoadSP2: '%s' has too many frames (%i > %i)", out->name, in->numframes, 32);

	out->numFrames = in->numframes;
	out->numSurfaces = 1;
	out->type = MOD_SPRITE;
}


/*
=================
SV_GetTag

returns orientation_t of a tag for a given frame or NULL if not found
=================
*/
orientation_t* SV_GetTag(int modelindex, int frame, char* tagName)
{
	svmodel_t* mod;
	orientation_t* tagdata;
	int index;

	mod = SV_ModelForNum(modelindex);
	if (!mod || mod->type != MOD_MD3)
	{
		Com_Error(ERR_DROP, "SV_GetTag: wrong model for index %i\n", modelindex);
		return NULL;
	}

	// it is possible to have a bad frame while changing models, so don't error
	if (frame >= mod->numFrames)
		frame = mod->numFrames - 1;
	else if (frame < 0)
		frame = 0;

	tagdata = (orientation_t*)((byte*)mod->tagFrames) + (frame * mod->numTags);
	for (index = 0; index < mod->numTags; index++, tagdata++)
	{
		if (!strcmp(mod->tagNames[index], tagName))
		{
			return tagdata; // found it
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
orientation_t* SV_PositionTag(vec3_t origin, vec3_t angles, int modelindex, int animframe, char* tagName)
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
orientation_t* SV_PositionTagOnEntity(gentity_t* ent, char* tagName)
{
	orientation_t* tag;
	tag = SV_PositionTag(ent->v.origin, ent->v.angles, (int)ent->v.modelindex, (int)ent->v.animFrame, tagName);
	return tag;
}