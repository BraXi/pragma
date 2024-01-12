
#include "../client/client.h"

static char* parseFileName = "";

#define DEFAULT_ANIM_RATE 10

typedef struct animation_info_s
{
	char		*name;
	int			firstFrame;
	int			lastFrame;
	int			numFrames;
	int			rate;
	qboolean	looping;
} animation_info_t;

typedef struct event_info_s
{
	char	*name;
	char	*action;

	char	*animation;
	int		frame;
} event_info_t;

typedef struct skin_info_s
{
	char	*name;
	int		*replace;
} skin_info_t;

typedef struct modeldef_s
{
	int		numAnimations;
	int		numEvents;
	int		numSkins;

	animation_info_t	*anims[32];
	event_info_t		*events[64];
	skin_info_t			*skins[4];
} modeldef_t;

#define	FOFS(type,x) (int)&(((type *)0)->x)

typedef enum
{
	F_INT,
	F_FLOAT,
	F_STRING,
	F_BOOLEAN,
	F_VECTOR3,
	F_VECTOR4,
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char* name;
	fieldtype_t	type;
	int		ofs;
} parsefield_t;

static parsefield_t fields_anim[] =
{
	/* animation */
	{"name", F_STRING, FOFS(animation_info_t,name)},
	{"firstFrame", F_INT, FOFS(animation_info_t,firstFrame)},
	{"lastFrame", F_INT, FOFS(animation_info_t,lastFrame)},
	{"numFrames", F_INT, FOFS(animation_info_t,numFrames)},
	{"rate", F_INT, FOFS(animation_info_t,rate)},
	{"looping", F_BOOLEAN, FOFS(animation_info_t,looping)},
};

static parsefield_t fields_event[] =
{
	/* events */
	{"name", F_STRING, FOFS(event_info_t,name)},
	{"animation", F_STRING, FOFS(event_info_t,name)},
	{"frame", F_INT, FOFS(event_info_t,frame)},
	{"action", F_STRING, FOFS(event_info_t,action)}
};

static parsefield_t fields_skin[] =
{
	/* events */
	{"name", F_STRING, FOFS(skin_info_t,name)},
	{"replace", F_STRING, FOFS(skin_info_t,replace)},
};

char* DEF_NewString(char* string)
{
	char	*newstring = NULL, *new_p = NULL;
	int		pos, length;

	length = strlen(string) + 1;
	newstring = Z_Malloc(length * sizeof(char));

	if (!newstring)
	{
		Com_Error(ERR_DROP, "`%s: malloc failed\n", __FUNCTION__);
		return NULL; // msvc crap
	}

	memset(newstring, 0, length * sizeof(char));

	new_p = newstring;

	for (pos = 0; pos < length; pos++)
	{
		if (string[pos] == '\\' && pos < length - 1)
		{
			pos++;
			if (string[pos] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[pos];
	}

	return newstring;
}

void DEF_ParseField(char* key, char* value, byte *ptr, parsefield_t* f)
{
	float	vec[4];

//	Com_Printf("DEF_ParseField: '%s' - '%s'\n", key, value);
	for (; f->name; f++)
	{
		if (!Q_stricmp(f->name, key))
		{	
			// found it
			switch (f->type)
			{
			case F_INT:
				*(int*)(ptr + f->ofs) = atoi(value);
				break;

			case F_FLOAT:
				*(float*)(ptr + f->ofs) = atof(value);
				break;

			case F_BOOLEAN:
				if(!Q_stricmp(value, "true"))
					*(int*)(ptr + f->ofs) = true;
				else if (!Q_stricmp(value, "false"))
					*(int*)(ptr + f->ofs) = false;
				else
					*(int*)(ptr + f->ofs) = (atoi(value) > 0 ? true : false);
				break;

			case F_STRING:
				*(char**)(ptr + f->ofs) = DEF_NewString(value);
				break;

			case F_VECTOR3:
				sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float*)(ptr + f->ofs))[0] = vec[0];
				((float*)(ptr + f->ofs))[1] = vec[1];
				((float*)(ptr + f->ofs))[2] = vec[2];
				break;

			case F_VECTOR4:
				sscanf(value, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
				((float*)(ptr + f->ofs))[0] = vec[0];
				((float*)(ptr + f->ofs))[1] = vec[1];
				((float*)(ptr + f->ofs))[2] = vec[2];
				((float*)(ptr + f->ofs))[3] = vec[3];
				break;

			case F_IGNORE:
				break;
			}
			return;
		}
	}
	Com_Printf("WARNING: %s has unknown field `%s`\n", parseFileName, key);
}

void DEF_ParseTokens(char* data, modeldef_t* def)
{
	char	*token;
	char	key[256];
	char	value[256];
	int		keyvalpairnum;
	parsefield_t * fields;

	// check header
	token = COM_Parse(&data);
	if (!token || Q_stricmp("MODELDEFINITION", token))
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
				if (!Q_stricmp("ANIMATION", key))
				{
					fields = fields_anim;
					def->anims[def->numAnimations] = Z_Malloc(sizeof(animation_info_t));
					def->numAnimations++;
				}
				else if (!Q_stricmp("EVENT", key))
				{
					fields = fields_event;
					def->events[def->numEvents] = Z_Malloc(sizeof(event_info_t));
					def->numEvents++;
				}
				else if (!Q_stricmp("SKIN", key))
				{
					fields = fields_event;
					def->skins[def->numSkins] = Z_Malloc(sizeof(skin_info_t));
					def->numSkins++;
				}
				else
				{
					fields = NULL;
					Com_Error(ERR_DROP, "%s: expected ANIMATION, SKIN or EVENT", parseFileName);
				}

				continue;
			}

			// parse value	
			token = COM_Parse(&data);
			if (!data)
				Com_Error(ERR_DROP, "%s: EOF without closing brace", parseFileName);

			if (token[0] == '}')
				Com_Error(ERR_DROP, "%s: closing brace without data", parseFileName);

			strncpy(value, token, sizeof(value) - 1);

			if(fields == fields_anim)
				DEF_ParseField(key, value, def->anims[def->numAnimations-1], fields);
			else if (fields == fields_event)
				DEF_ParseField(key, value, def->events[def->numEvents - 1], fields);
			else if (fields == fields_skin)
				DEF_ParseField(key, value, def->anims[def->numSkins - 1], fields);
		}
	}
}


static void DEF_ValidateAnimations(modeldef_t* def)
{
	animation_info_t* anim;
	int i, j;

	for (i = 0; i < def->numAnimations; i++)
	{
		anim = def->anims[i];

		// animation must have a name
		if (!anim->name || !anim->name[0])
			Com_Error(ERR_DROP, "%s: animation %i has no name\n", parseFileName, i);

		// animations cannot have the same names
		for (j = 0; j < def->numAnimations; j++)
		{
			if (j != i && !Q_stricmp(anim->name, def->anims[j]->name))
				Com_Error(ERR_DROP, "%s: animation '%s' declared more than once\n", parseFileName, anim->name);
		}

		// animation must have frames
		if (anim->lastFrame <= 0 && anim->numFrames <= 0)
			Com_Error(ERR_DROP, "%s: animation '%s' has no frames\n", parseFileName, anim->name);

		// lastFrame, numFrames - only one can be set
		if (anim->lastFrame > 0 && anim->numFrames > 0)
			Com_Error(ERR_DROP, "%s: animation '%s' has both lastFrame(%i) and numFrames(%i) set\n", parseFileName, anim->name, anim->lastFrame, anim->numFrames);

		if (anim->numFrames > 0)
			anim->lastFrame = anim->firstFrame + anim->numFrames;
		else
			anim->numFrames = anim->lastFrame - anim->firstFrame;

		// first frame cannot be greater than last frame
		if (anim->firstFrame > anim->lastFrame)
			Com_Error(ERR_DROP, "%s: animation '%s' has lastFrame(%i) greater than firstFrame(%i)\n", parseFileName, anim->name, anim->lastFrame, anim->firstFrame);


		if (anim->rate <= 0)
			anim->rate = DEFAULT_ANIM_RATE;

	}
}

qboolean DEF_LoadFile(char* filename, modeldef_t* def)
{
	int		len;
	byte	*raw;
	char	*text;
	char	fname[MAX_OSPATH];

	//
	// load file
	//
	len = strlen(filename);
	//if (l > 4 && !strcmp(level + l - 4, ".cin"))

		//Com_sprintf(filename, sizeof(filename), "defs/%s", arg);

	len = FS_LoadFile(filename, (void**)&raw);
	if (!len || len == -1)
	{
		Com_Printf("%s: couldn't load '%s'\n", __FUNCTION__, filename);
		return false;
	}

	// NULL terminate the file
	text = Z_Malloc(len + 1);
	memcpy(text, raw, len);
	text[len] = 0;
	FS_FreeFile(raw); // free file now

	parseFileName = filename;

	//
	// parse definition file
	//
	DEF_ParseTokens(text, def);
	Z_Free(text);

	//
	// check for errors and validate
	//
	DEF_ValidateAnimations(def);

	Com_Printf("%s - %i animations, %i events and %i skins.\n", filename, def->numAnimations, def->numEvents, def->numSkins);

	return true;
}


void CL_Test_f(void)
{
#if 0
	modeldef_t* def = Z_Malloc(sizeof(modeldef_t));

	DEF_LoadFile("defs/test.def", def);
#endif

}