/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "qcommon.h"

static char* parseFileName = "";


static parsefield_t fields_anim[] =
{
	/* animation */
//	{"animation", F_STRING, FOFS(animation_t,name)},
	{"first", F_INT, FOFS(animation_t,firstFrame)},
//	{"lastFrame", F_INT, FOFS(animation_t,lastFrame)},
	{"frames", F_INT, FOFS(animation_t,numFrames)},
	{"fps", F_INT, FOFS(animation_t,lerpTime)},
	{"looping", F_BOOLEAN, FOFS(animation_t,looping)},
};


static void ModelDef_Parse(char* data, modeldef_t* def)
{
	char	*token;
	char	key[256];
	char	value[256];
	int		keyvalpairnum;
	parsefield_t * fields;

	// check header
	token = COM_Parse(&data);
	if (!token || Q_stricmp("PRAGMA_MODEL_DEF_v1", token))
		Com_Error(ERR_DROP, "'%s' is not a vaild model definition file", parseFileName, token);

	while (1)
	{
		// parse the opening brace	
		token = COM_Parse(&data);
		if (!data)
			break;

		if (token[0] != '{')
			Com_Error(ERR_DROP, "%s: found '%s' when expecting '{'", parseFileName, token);

		// go through all the key value pairs inside of { }
		fields = NULL;
		keyvalpairnum = 0;
		while (1)
		{
			// parse key
			token = COM_Parse(&data);
			if (token[0] == '}')
				break;
			if (!data)
				Com_Error(ERR_DROP, "%s: EOF without closing brace", parseFileName, __FUNCTION__);

			strncpy(key, token, sizeof(key) - 1);

			keyvalpairnum++;
			if (keyvalpairnum == 1)
			{
//				Com_Printf("type: '%s'\n", key);
				if (!Q_stricmp("animation", key))
				{
					fields = fields_anim;
					memset(&def->anims[def->numAnimations], 0, sizeof(def->anims[0]));
					//def->anims[def->numAnimations] = Z_Malloc(sizeof(animation_t));
					def->numAnimations++;
//					continue;
				}
				else if (!Q_stricmp("SKIN", key))
				{
//					fields = fields_event;
//					def->skins[def->numSkins] = Z_Malloc(sizeof(skin_info_t));
					def->numSkins++;
					continue;
				}
				else
				{
					fields = NULL;
					Com_Error(ERR_DROP, "%s: expected ANIMATION, or SKIN", parseFileName);
				}
			}

			// parse value	
			token = COM_Parse(&data);
			if (!data)
				Com_Error(ERR_DROP, "%s: EOF without closing brace", parseFileName);

			if (token[0] == '}')
				Com_Error(ERR_DROP, "%s: closing brace without data", parseFileName);

			strncpy(value, token, sizeof(value) - 1);

			if (fields == fields_anim && keyvalpairnum == 1)
			{
				// haaaack
				strncpy(def->anims[def->numAnimations-1].name, token, sizeof(def->anims[0].name) - 1);
				continue;
			}

			if(fields == fields_anim)
				COM_ParseField(key, value, &def->anims[def->numAnimations - 1], fields);
//			else if (fields == fields_skin)
//				COM_ParseField(key, value, def->skins[def->numSkins - 1], fields);
		}
	}
}


static void ModelDef_ValidateAnimations(modeldef_t* def)
{
	animation_t* anim;
	int i, j;

	// anim0 is null
	for (i = 1; i < def->numAnimations; i++)
	{
		anim = &def->anims[i];
		// animation must have a name
		if (!anim->name || !anim->name[0])
			Com_Error(ERR_DROP, "%s: animation %i has no name\n", parseFileName, i);

		// animations cannot have the same names
		for (j = 0; j < def->numAnimations; j++)
		{
			if (j != i && !Q_stricmp(anim->name, def->anims[j].name))
				Com_Error(ERR_DROP, "%s: animation '%s' declared more than once\n", parseFileName, anim->name);
		}

		// animation must have frames
		if (anim->lastFrame <= 0 && anim->numFrames <= 0)
			Com_Error(ERR_DROP, "%s: animation '%s' has no frames\n", parseFileName, anim->name);

		// lastFrame, numFrames - only one can be set
		if (anim->lastFrame > 0 && anim->numFrames > 0)
			Com_Error(ERR_DROP, "%s: animation '%s' has both lastFrame(%i) and numFrames(%i) set\n", parseFileName, anim->name, anim->lastFrame, anim->numFrames);

		anim->lastFrame = anim->firstFrame + anim->numFrames;

		// first frame cannot be greater than last frame
		if (anim->firstFrame > anim->lastFrame)
			Com_Error(ERR_DROP, "%s: animation '%s' has lastFrame(%i) greater than firstFrame(%i)\n", parseFileName, anim->name, anim->lastFrame, anim->firstFrame);

		if (anim->lerpTime <= 0)
			anim->lerpTime = (1000 / DEFAULT_ANIM_RATE);
		else
			anim->lerpTime = (1000 / anim->lerpTime);

//		Com_Printf("%i - '%s' start %i, end %i, frames %i\n", i, anim->name, anim->firstFrame, anim->lastFrame, anim->numFrames);
	}
}

qboolean ModelDef_LoadFile(char* filename, modeldef_t* def)
{
	int		len;
	char	*data = NULL;

	len = FS_LoadTextFile(filename, (void**)&data);
	if (!len || len == -1)
		return false;
	if (data == NULL)
		return false;

	parseFileName = filename;
	ModelDef_Parse(data, def);
	FS_FreeFile(data);

	ModelDef_ValidateAnimations(def);

//	Com_Printf("%s - %i animations, %i skins.\n", filename, def->numAnimations, def->numSkins);
	return true;
}


animation_t *ModelDef_AnimForName(modeldef_t* def, char* animName)
{
	int i;
	for (i = 0; i < def->numAnimations; i++)
	{
		if (!def->anims[i].name || !def->anims[i].name[0])
			continue;

		if (!Q_stricmp(def->anims[i].name, animName))
			return &def->anims[i];
	}
	return NULL;
}
