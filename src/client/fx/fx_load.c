/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "fx_local.h"

/*
==============================================================

EFFECT LOADING

==============================================================
*/

static char* parseFileName;

static parsefield_t fields_fx_particle[] =
{
	{"lifetime", F_FLOAT, FOFS(fx_particle_def_t,lifetime)},

	{"fadeaftertime", F_FLOAT, FOFS(fx_particle_def_t,fadeColorAfterTime)},
	{"scaleaftertime", F_FLOAT, FOFS(fx_particle_def_t,scaleAfterTime)},

	{"delay", F_FLOAT, FOFS(fx_particle_def_t,delay)},		
	
	{"count", F_INT, FOFS(fx_particle_def_t,count)},			// if !(flags & FXF_RANDOM_COUNT)
	{"addrandomcount", F_INT, FOFS(fx_particle_def_t,addRandomCount)},
	
	{"origin", F_VECTOR3, FOFS(fx_particle_def_t,origin)},
	{"randomdistrubution", F_VECTOR3, FOFS(fx_particle_def_t,randomDistrubution)},

	{"velocity", F_VECTOR3, FOFS(fx_particle_def_t,velocity)},	// if !(flags & FXF_RANDOM_VELOCITY)
	{"addrandomvelocity", F_FLOAT, FOFS(fx_particle_def_t,addRandomVelocity)},		// if !(flags & FXF_RANDOM_GRAVITY)
	
	{"gravity", F_FLOAT, FOFS(fx_particle_def_t,gravity)},
	{"addrandomgravity", F_FLOAT, FOFS(fx_particle_def_t,addRandomGravity)},

	{"color", F_VECTOR4, FOFS(fx_particle_def_t,color)},
	{"color2", F_VECTOR4, FOFS(fx_particle_def_t,color2)},

	{"size", F_VECTOR2, FOFS(fx_particle_def_t,size)},
	{"size2", F_VECTOR2, FOFS(fx_particle_def_t,size2)}
};

static parsefield_t fields_fx_dlight[] =
{
	{"origin", F_VECTOR3, FOFS(fx_dlight_t,origin)},
	{"color", F_VECTOR3, FOFS(fx_dlight_t,color)},
	{"radius", F_FLOAT, FOFS(fx_dlight_t,radius)}
};


/*
===============
FX_CreatePartSegment
===============
*/
static void FX_CreatePartSegment(fxdef_t* owner)
{
	if (owner->numPartSegments >= FX_MAX_SEGMENTS_PER_TYPE)
	{
		Com_Error(ERR_DROP, "hit max particle segments in effect '%s'", owner->name);
		return;
	}

	fx_particle_def_t* part = Z_TagMalloc(sizeof(fx_particle_def_t), TAG_FX);

	VectorSet(part->color, 1.0f, 1.0f, 1.0f);
	part->size[0] = part->size[1] = 1.0;
	part->size2[0] = part->size2[1] = 1.0;

	owner->part[owner->numPartSegments] = part;
	owner->numPartSegments++;
}

/*
===============
FX_CreateDynamicLightSegment
===============
*/
static void FX_CreateDynamicLightSegment(fxdef_t* owner)
{
	if (owner->numDLightSegments >= FX_MAX_SEGMENTS_PER_TYPE)
	{
		Com_Error(ERR_DROP, "hit max dlight segments in effect '%s'", owner->name);
		return;
	}

	fx_dlight_t* dlight = Z_TagMalloc(sizeof(fx_dlight_t), TAG_FX);

	VectorSet(dlight->color, 1.0f, 1.0f, 1.0f);
	dlight->radius = 64;
//	dlight->lifeTime = 1000;

	owner->dlight[owner->numDLightSegments] = dlight;
	owner->numDLightSegments++;
}

/*
===============
FX_ParseFile
===============
*/
static void FX_ParseFile(char* data, fxdef_t* def)
{
	char* token;
	char	key[256];
	char	value[256];
	int		keyvalpairnum;
	parsefield_t* fields;

	// check header
	token = COM_Parse(&data);
	if (!token || Q_stricmp("PRAGMA_EFFECT", token))
		Com_Error(ERR_DROP, "'%s' is not a vaild effect file", parseFileName, token);

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
				if (!Q_stricmp("PARTICLE", key))
				{
					fields = fields_fx_particle;
					FX_CreatePartSegment(def);
				}
				else if (!Q_stricmp("DYNAMIC_LIGHT", key))
				{
					fields = fields_fx_dlight;
					FX_CreateDynamicLightSegment(def);

				}
				else
				{
					fields = NULL;
					Com_Error(ERR_DROP, "%s: expected PARTICLE or DYNAMIC_LIGHT", parseFileName);
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

			if (fields == fields_fx_particle)
				COM_ParseField(key, value, def->part[def->numPartSegments - 1], fields);
			else if (fields == fields_fx_dlight)
				COM_ParseField(key, value, def->dlight[def->numDLightSegments - 1], fields);
		}
	}
}


/*
===============
FX_LoadFromFile
===============
*/
qboolean FX_LoadFromFile(char* name)
{
	unsigned int	len;
	char			* data = NULL;
	char			filename[MAX_OSPATH];
	int				fxindex;
	fxdef_t			*fx;
	
	if (fxSys.numLoadedEffects >= FX_MAX_EFFECTS)
	{
		Com_Error(ERR_DROP, "reached limit of %i loaded effects\n");
		return false;
	}

	// se if its already loaded
	fxindex = CL_FindEffect(name);
	if (fxindex == -1)
		return -1;

	// load file
	Com_sprintf(filename, sizeof(filename), "fx/%s.efx", name);
	len = FS_LoadTextFile(filename, (void**)&data);
	if (!len || len == -1)
	{
		return -1;
	}

	fx = Z_TagMalloc(sizeof(fxdef_t), TAG_FX);
	strncpy(fx->name, name, sizeof(fx->name));

	// parse it
	parseFileName = filename;
	FX_ParseFile(data, fx);
	FS_FreeFile(data);

	fxSys.fxDefs[fxSys.numLoadedEffects] = fx;
	fxSys.numLoadedEffects++;

	Com_Printf("...loaded FX: %s\n", filename);
	return fxSys.numLoadedEffects-1;
}

