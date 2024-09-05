#include "prtool.h"

typedef enum { ASSET_BAD, ASSET_MODEL, ASSET_ANIMATION, ASSET_TYPES } assettype_t;

static int numAssetsByType[ASSET_TYPES];
static const char *assetTypeNames[ASSET_TYPES] = { "bad", "model", "animation" };

typedef struct sourcedata_s
{
	char name[MAX_QPATH];
	char filename[MAXPATH];
	smddata_t* pData;
} sourcedata_t;

typedef struct assetdef_s
{
	assettype_t type;

	char name[MAX_QPATH];
	char outName[MAX_QPATH];

	int numBones;
	int numVertexes;
	int numSurfaces;
	int numParts;

	std::vector<sourcedata_t*> vSources;
	panim_event_t* pEvents;

	unsigned int flags;
	int fps;
} assetdef_t;

#define MODELS_DIR "models"
#define MODEL_EXT "mod"
#define ANIMATIONS_DIR "modelanims"
#define ANIMATION_EXT "anm"

#define MIN_FPS 10
#define MAX_FPS 60

extern smddata_t* LoadSMD(const smdtype_t expect, const char* name, const char* filename);

static FILE* qcFileHandle = NULL;
static char qcName[MAXPATH];

#define MAX_ASSETS 1024
static assetdef_t assets[MAX_ASSETS];
static int numAssets = 0;
static int numConverted = 0;
static assetdef_t* pAsset = assets;
static int qc_line;

static void Cmd_Model();		// $model name
static void Cmd_Animation();	// $animation name
static void Cmd_AddMesh();		// $mesh partname sourcefile.smd
static void Cmd_Source();		// $source file.ext
static void Cmd_Event();		// $event framenumber "event string"
static void Cmd_FPS();			// $fps integer
static void Cmd_HitBox();		// $hitbox bonename hitname minsx minsy minsz maxsx maxsy maxsz
//static void Cmd_Frames();		// $frames firstframe lastframe

static command_t commands[] =
{
	// name, function, numargs
	{"model", Cmd_Model, 1},
	{"animation", Cmd_Animation, 1},
	{"mesh", Cmd_AddMesh, 2},
	{"source", Cmd_Source, 1},
	{"event", Cmd_Event, 2},
	{"hitbox", Cmd_Event, 8},
	{"fps", Cmd_FPS, 1}//,
	//{"frames", Cmd_Frames, 2}
};

static const int numCommands = sizeof(commands) / sizeof(command_t); // number of commands

/*
=================
InitAssetDef
Initializes assetdef_t to defaults and increments pAsset ptr
=================
*/
static void InitAssetDef(const assettype_t type, const char *name)
{
	if (!pAsset || pAsset == NULL)
	{
		Com_Error("pAsset == NULL");
		return;
	}

	if (numAssets >= MAX_ASSETS)
	{
		Com_Error("Increase MAX_ASSETS.");
		return;
	}

	if (numAssets && pAsset)
		pAsset++;

	memset(pAsset, 0, sizeof(assetdef_s));

	pAsset->type = type;
	strncpy(pAsset->name, name, sizeof(pAsset->name));
	//pAsset->flags = 0;
	//pAsset->scale[0] = pAsset->scale[1] = pAsset->scale[2] = 1.0f;
	pAsset->fps = MIN_FPS;

	numAssets++;

	numAssetsByType[type]++;
}

/*
=================
Cmd_Model
$model name
=================
*/
static void Cmd_Model()
{
	InitAssetDef(ASSET_MODEL, Com_GetArg(1));

	snprintf(pAsset->outName, sizeof(pAsset->outName), "%s/%s.%s", MODELS_DIR, pAsset->name, MODEL_EXT);

	Com_HappyPrintf("\n------------ Model '%s' ------------\n", Com_GetArg(1));
	Com_Printf("Destination: %s\n", pAsset->outName);
}

/*
=================
Cmd_Animation
$animation name
=================
*/
static void Cmd_Animation()
{
	InitAssetDef(ASSET_ANIMATION, Com_GetArg(1));

	snprintf(pAsset->outName, sizeof(pAsset->outName), "%s/%s.%s", ANIMATIONS_DIR, pAsset->name, ANIMATION_EXT);

	Com_HappyPrintf("\n------------ Animation '%s' ------------\n", Com_GetArg(1));
	Com_Printf("Destination: %s\n", pAsset->outName);
}

/*
=================
Cmd_AddMesh
$mesh partname sourcefile.smd
=================
*/
static void Cmd_AddMesh()
{
	char filename[MAXPATH];
	char *name, *srcfile;
	unsigned int i;
	sourcedata_t* srcdata;

	if (!numAssets)
		return;

	if (pAsset->type != ASSET_MODEL)
	{
		Com_Error("[line %i] %s can only be set for models.", qc_line, Com_GetArg(0));
		return;
	}

	name = Com_GetArg(1);
	srcfile = Com_GetArg(2);
	for (i = 0; i < pAsset->vSources.size(); i++)
	{
		// do not allow the same source file included multiple times
		if (!Com_stricmp(pAsset->vSources[i]->filename, srcfile))
		{
			Com_Warning("[line %i] ignoring duplicate source file \"%s\" in model %s.", qc_line, srcfile, pAsset->name);
			return;
		}

		// do not allow the same part names
		if (!Com_stricmp(pAsset->vSources[i]->name, name))
		{
			Com_Warning("[line %i] ignoring duplicate part \"%s\" in model %s", qc_line, pAsset->vSources[i]->name, pAsset->name);
			return;
		}
	}

	// check if the file exists
	snprintf(filename, sizeof(filename), "%s/%s", g_devdir, srcfile);

	if (!Com_FileExists(filename))
	{
		Com_Warning("Ignoring part %i \"%s\" which references missing file: %s", pAsset->vSources.size(), name, filename);
		return;
	}

	srcdata = (sourcedata_s*)Com_SafeMalloc(sizeof(sourcedata_s), __FUNCTION__);
	strncpy(srcdata->name, name, sizeof(srcdata->name));
	strncpy(srcdata->filename, srcfile, sizeof(srcdata->filename));

	// TODO: move to ConvertAsset
	srcdata->pData = LoadSMD(SMD_MODEL, filename, filename);
	if (!srcdata->pData)
	{
		Com_Error2("Cannot load %s for mesh %i \"%s\".", filename, pAsset->vSources.size(), name);
		//free(srcdata);
		return;
	}

	Com_Printf("Part \"%s\" : %s (%i surface%s, %i vertexes, %i triangles)\n", /*pAsset->vSources.size(),*/ name, filename, srcdata->pData->numsurfaces, srcdata->pData->numsurfaces == 1 ? "" : "s", srcdata->pData->numverts, (srcdata->pData->numverts / 3));
	for (i = 0; i < (unsigned int)srcdata->pData->numsurfaces; i++)
	{
		Com_Printf("   ... %s:%i : \"%s\".\n", srcdata->name, i, srcdata->pData->vSurfaces[i]->texture);
	}

	if (!pAsset->vSources.size())
	{
		// first part defines how many bones the model will have
		pAsset->numBones = srcdata->pData->numbones;
	}

	pAsset->vSources.push_back(srcdata);

	pAsset->numVertexes += srcdata->pData->numverts;
	pAsset->numSurfaces += srcdata->pData->numsurfaces;
	pAsset->numParts = (uint32_t)pAsset->vSources.size();
}


/*
=================
Cmd_Source
$source sourcefile.smd
=================
*/
static void Cmd_Source()
{
	char filename[MAXPATH];
	char *srcfile;
	sourcedata_t* srcdata;

	if (pAsset->type != ASSET_ANIMATION)
		return;

	if (pAsset->vSources.size())
	{
		Com_Warning("[line %i] skipping redefinition of %s.", qc_line, Com_GetArg(0));
		return;
	}

	srcfile = Com_GetArg(1);
	snprintf(filename, sizeof(filename), "%s/%s", g_devdir, srcfile);

	if (!Com_FileExists(filename))
	{
		Com_Error("Source file %s does not exist.", filename);
		return;
	}

	srcdata = (sourcedata_s*)Com_SafeMalloc(sizeof(sourcedata_s), __FUNCTION__);
	strncpy(srcdata->name, pAsset->name, sizeof(srcdata->name));
	strncpy(srcdata->filename, srcfile, sizeof(srcdata->filename));

	// TODO: move to ConvertAsset
	srcdata->pData = LoadSMD(SMD_ANIMATION, filename, filename);
	if (!srcdata->pData)
	{
		Com_Error2("Cannot load animation \"%s\".", filename);
		//free(srcdata);
		return;
	}

	Com_Printf("   ... %i frame%s\n", srcdata->pData->numframes, srcdata->pData->numframes == 1 ? "" : "s");
	Com_Printf("   ... %i bones referenced\n", srcdata->pData->numbones);

	pAsset->vSources.push_back(srcdata);
}

/*
=================
Cmd_Event
$event framenumber "event string"
=================
*/
static void Cmd_Event()
{
	int framenum;
	int numframes;
	char* eventstr;

	if (pAsset->type != ASSET_ANIMATION)
	{
		Com_Error("[line %i] %s can only be set for animations.", qc_line, Com_GetArg(0));
		return;
	}

	if (!pAsset->vSources.size())
	{
		Com_Error("[line %i] %s must be after $source.", qc_line, Com_GetArg(0));
		return;
	}

	framenum = atoi(Com_GetArg(1));
	eventstr = Com_GetArg(2);

	numframes = pAsset->vSources[0]->pData->numframes;

	if (framenum < 0 || framenum >= numframes)
	{
		Com_Warning("[line %i] %s for a frame (%i) which doesn't exist.", qc_line, Com_GetArg(0), framenum);
		return;
	}

	if (strlen(eventstr) >= PMOD_MAX_EVENTSTRING)
	{
		Com_Warning("[line %i] $%s has too long event string.", qc_line, Com_GetArg(0));
	}

	if (pAsset->pEvents == NULL)
	{
		pAsset->pEvents = (panim_event_t*)Com_SafeMalloc(numframes * sizeof(panim_event_s), __FUNCTION__);
	}


	strncpy(pAsset->pEvents[framenum].str, eventstr, sizeof(char)*PMOD_MAX_EVENTSTRING);
	Com_Printf("   ... event at %i : '%s'\n", framenum, pAsset->pEvents[framenum].str);
}

/*
=================
Cmd_HitBox
// $hitbox bonename hitname minsx minsy minsz maxsx maxsy maxsz
=================
*/
static void Cmd_HitBox()
{
	int numbones;
	char* bonename;
	char* hitname;
	vec3_t mins, maxs;

	if (pAsset->type != ASSET_MODEL)
	{
		Com_Error("[line %i] %s can only be set for models.", qc_line, Com_GetArg(0));
		return;
	}

	if (!pAsset->vSources.size())
	{
		Com_Error("[line %i] %s must be after $source.", qc_line, Com_GetArg(0));
		return;
	}

	bonename = Com_GetArg(1);
	hitname = Com_GetArg(2);

	numbones = pAsset->vSources[0]->pData->numbones;

}


/*
=================
Cmd_FPS
$fps integer between min/max fps
=================
*/
static void Cmd_FPS()
{
	int fps;

	if (pAsset->type != ASSET_ANIMATION)
	{
		Com_Error("[line %i] $%s can only be set for animations.", qc_line, Com_GetArg(0));
		return;
	}

	fps = atoi(Com_GetArg(1));

	if (fps < MIN_FPS || fps > MAX_FPS)
		Com_Warning("[line %i] ignoring weird $%s, defaulted to [1.0 1.0 1.0].",  qc_line, Com_GetArg(0));	
	else
	{
		pAsset->fps = fps;
		Com_Printf("   ... %i frames per second\n", pAsset->fps);
	}
}

/*
=================
LoadAssetsFromControlFile
Loads asset entries from control file
=================
*/
static void LoadAssetsFromControlFile(const char* fileName)
{
	static char line[MAXLINELEN];
	int i;
	char* key;

	qcFileHandle = Com_OpenReadFile(fileName, false);

	if (!qcFileHandle)
	{
		Com_Error("Cannot open control file: %s\n", fileName);
		return;
	}

	Com_Printf("Control file: %s\n", fileName);

	qc_line = 0;
	while (fgets(line, sizeof(line), qcFileHandle) != NULL)
	{
		qc_line++;
		Com_Tokenize(line);

		if (Com_NumArgs() < 1)
		{
		//	//Com_Warning("[line %i] less than two arguments, skipping entire line.", qc_line);
			continue;
		}

		key = Com_GetArg(0);

		if (key[0] == '$')
		{
			for (i = 0; i < numCommands; i++)
			{
				if (!Com_stricmp(commands[i].name, key + 1) && Com_NumArgs() >= commands[i].numargs+1)
				{
					commands[i].pFunc();
					break;
				}
			}
		}
	}
	fclose(qcFileHandle);
}


#define SAFE_FREE(x) \
	if (x) { \
		free(x); \
		x = NULL; }


/*
=================
WriteModel
=================
*/
static void WriteModel(assetdef_t* def)
{
	char filename[MAXPATH];
	FILE* f;
	smddata_t* pData = NULL;
	long elementsize, outsize = 0;
	int i, j, k, nextfirst;
	
	pmodel_header_t header;
	pmodel_bone_t bone;
	panim_bonetrans_t skel;
	pmodel_vertex_t vert;
	pmodel_surface_t surf;
	pmodel_part_t part;

	snprintf(filename, sizeof(filename), "%s/models/%s.%s", g_devdir, def->name, MODEL_EXT);
	f = Com_OpenWriteFile(filename, false);
	if (!f)
	{
		Com_Warning("Cannot open %s for writing.", filename);
		return;
	}

	// write header
	memset(&header, 0, sizeof(pmodel_header_s));
	header.ident = Com_EndianLong(PMODEL_IDENT);
	header.version = Com_EndianLong(PMODEL_VERSION);
	//header.mins = { 0 };
	//header.maxs = { 0 };
	header.flags = Com_EndianLong(def->flags);
	header.numBones = Com_EndianLong(def->numBones);
	header.numVertexes = Com_EndianLong(def->numVertexes);
	header.numSurfaces = Com_EndianLong(def->numSurfaces);
	header.numParts = Com_EndianLong(def->numParts);

	Com_SafeWrite(f, &header, sizeof(header));
	outsize += sizeof(header);


	// use first part of a model for bones and skeleton, all remaining parts match them
	pData = def->vSources[0]->pData; // MAIN PART
	elementsize = sizeof(pmodel_bone_t);
	for (i = 0; i < pData->numbones; i++)
	{
		smd_bone_t* srcbone = pData->vBones[i];

		memset(&bone, 0, elementsize);
		bone.number = Com_EndianLong(srcbone->index);
		bone.parentIndex = Com_EndianLong(srcbone->parent);
		strncpy(bone.name, srcbone->name, sizeof(bone.name));

		//bone.partname[PMOD_MAX_HITPARTNAME]; // head, arm_upper, torso_lower, hand_left etc..

		for (j = 0; j < 3; j++)
		{
			bone.mins[j] = Com_EndianFloat(bone.mins[j]);
			bone.maxs[j] = Com_EndianFloat(bone.maxs[j]);
		}

		Com_SafeWrite(f, &bone, elementsize);
		outsize += elementsize;
	}

	// write skeleton
	elementsize = sizeof(panim_bonetrans_t);
	for (i = 0; i < pData->numbones; i++)
	{
		smd_bonetransform_t* srcskel = pData->vBoneTransforms[i];

		memset(&skel, 0, elementsize);
		skel.bone = Com_EndianLong(srcskel->bone);

		for (j = 0; j < 3; j++)
		{
			skel.origin[j] = Com_EndianFloat(srcskel->position[j]);
			skel.rotation[j] = Com_EndianFloat(srcskel->rotation[j]);
		}

		Com_SafeWrite(f, &skel, elementsize);
		outsize += elementsize;
	}

	// write vertexes from all parts
	elementsize = sizeof(pmodel_vertex_t);
	for (i = 0; i < (int)def->vSources.size(); i++)
	{
		pData = def->vSources[i]->pData;

		for (j = 0; j < pData->numverts; j++) // for each vert in part
		{
			smd_vert_t* srcvert = pData->vVerts[j];
			memset(&vert, 0, elementsize);

			for (k = 0; k < 3; k++)
			{
				vert.xyz[k] = Com_EndianFloat(srcvert->xyz[k]);
				vert.normal[k] = Com_EndianFloat(srcvert->normal[k]);
			}

			vert.uv[0] = Com_EndianFloat(srcvert->uv[0]);
			vert.uv[1] = Com_EndianFloat(srcvert->uv[1]);

			vert.boneId = Com_EndianLong(srcvert->bone);

			Com_SafeWrite(f, &vert, elementsize);
			outsize += elementsize;
		}
	}

	// write surfaces from all parts, gotta recalculate firstvert indexes
	nextfirst = 0;
	elementsize = sizeof(pmodel_surface_t);
	for (i = 0; i < (int)def->vSources.size(); i++)
	{
		pData = def->vSources[i]->pData;

		for (j = 0; j < pData->numsurfaces; j++) 
		{		
			smd_surface_t* srcsurf = pData->vSurfaces[j];
			memset(&surf, 0, elementsize);
			surf.firstVert = Com_EndianLong(nextfirst + srcsurf->firstvert);
			surf.numVerts = Com_EndianLong(srcsurf->numverts);
			strncpy(surf.material, srcsurf->texture, sizeof(surf.material));

			nextfirst += srcsurf->numverts;

			Com_SafeWrite(f, &surf, elementsize);
			outsize += elementsize;		
		}
	}

	// write parts
	nextfirst = 0;
	elementsize = sizeof(pmodel_part_t);
	for (i = 0; i < (int)def->vSources.size(); i++)
	{
		pData = def->vSources[i]->pData;
		memset(&part, 0, elementsize);

		//char name[PMOD_MAX_SURFNAME];
		strncpy(part.name, def->vSources[i]->name, sizeof(part.name));
		part.firstSurf = Com_EndianLong(nextfirst);
		part.numSurfs = Com_EndianLong(pData->numsurfaces);
		
		nextfirst += pData->numsurfaces;

		Com_SafeWrite(f, &part, elementsize);
		outsize += elementsize;
	}

	Com_Printf("   ... written '%s' (%i bytes).\n", filename, outsize);
	fclose(f);
}


/*
=================
WriteAnimation
=================
*/
static void WriteAnimation(assetdef_t* def)
{
	char filename[MAXPATH];
	int i, j;
	FILE* f;
	long outsize = 0;

	smddata_t* pData;
	panim_header_t header;
	panim_bone_t bone;
	panim_bonetrans_t trans;
	//panim_event_t ev;

	if (!def->vSources.size())
	{	
		Com_Warning("No source file loaded for asset '%s'.\n", def->name);
		return;
	}
	
	pData = def->vSources[0]->pData; // anims have a single source

	snprintf(filename, sizeof(filename), "%s/modelanims/%s.%s", g_devdir, def->name, ANIMATION_EXT);
	f = Com_OpenWriteFile(filename, false);
	if (!f)
	{
		if (pData)
			free(pData);
		Com_Warning("Cannot open %s for writing.", def->outName);
		return;
	}

	// write header
	memset(&header, 0, sizeof(panim_header_s));
	header.ident = Com_EndianLong(PANIM_IDENT);
	header.version = Com_EndianLong(PANIM_VERSION);
	header.flags = Com_EndianLong(def->flags);
	header.rate = Com_EndianLong(def->fps);
	header.numBones = Com_EndianLong(pData->numbones);
	header.numFrames = Com_EndianLong(pData->numframes);

	Com_SafeWrite(f, &header, sizeof(header));

	// write bones
	for (i = 0; i < pData->numbones; i++)
	{
		smd_bone_t* srcbone = pData->vBones[i];

		memset(&bone, 0, sizeof(panim_bone_s));
		strncpy(bone.name, srcbone->name, sizeof(bone.name));
		bone.number = Com_EndianLong(srcbone->index);
		bone.parentIndex = Com_EndianLong(srcbone->parent);

		Com_SafeWrite(f, &bone, sizeof(bone));
		outsize += sizeof(panim_bone_s);	
	}

	// write transformations
	for (i = 0; i < pData->numbones * pData->numframes; i++)
	{
		smd_bonetransform_t* srctrans = pData->vBoneTransforms[i];
		memset(&trans, 0, sizeof(panim_bonetrans_s));

		trans.bone = Com_EndianLong(srctrans->bone);
		for (j = 0; j < 3; j++)
		{
			trans.origin[j] = Com_EndianFloat(srctrans->position[j]);
			trans.rotation[j] = Com_EndianFloat(srctrans->rotation[j]);
		}

		outsize += sizeof(panim_bonetrans_t);
		Com_SafeWrite(f, &trans, sizeof(trans));
	}

	// write events
	if (def->pEvents != NULL)
	{	
		for (i = 0; i < pData->numframes; i++)
		{
			Com_SafeWrite(f, &def->pEvents[i], sizeof(panim_event_t));
			outsize += sizeof(panim_event_t);
		}
	}

	Com_Printf("   ... written '%s' (%i bytes).\n", filename, outsize);
	fclose(f);
}

/*
=================
ConvertAsset
=================
*/
static void ConvertAsset(assetdef_t *def)
{
	if (def->type == ASSET_BAD)
	{
		Com_Error("assetdef->type == ASSET_BAD");
		return;
	}

	//Com_Printf("Converting '%s' of type '%s'...\n", def->name, assetTypeNames[def->type]);

	Com_Printf("Converting '");
	Com_HappyPrintf("%s", def->name);
	Com_Printf("' of type '");
	Com_HappyPrintf("%s", assetTypeNames[def->type]);
	Com_Printf("'...\n");

	switch (def->type)
	{
	case ASSET_MODEL:
		WriteModel(def);
		break;

	case ASSET_ANIMATION:
		WriteAnimation(def);
		break;
	}
}

/*
=================
Opt_ConvertAsset
=================
*/
void Opt_ConvertAsset()
{
	int i;
	assetdef_t* def;

	memset(qcName, 0, sizeof(qcName));
	sprintf(qcName, "%s/%s", g_devdir, g_controlFileName);

	memset(numAssetsByType, 0, sizeof(numAssetsByType));

	Com_Printf("==== Convert All ====\n");

	LoadAssetsFromControlFile(qcName);

	if (numAssets == 0)
	{
		Com_Error("Nothing to convert!");
	}

	Com_HappyPrintf("\n%i assets to convert.\n\n", numAssets);

	def = assets;
	for (i = 0; i < numAssets; i++, def++)
	{
		ConvertAsset(def);
		numConverted++;
	}

	Com_Printf("\n");

	for (int i = 0; i < ASSET_TYPES; i++)
	{
		if (numAssetsByType[i] == 0)
			continue;

		Com_HappyPrintf("   %4i %s%s\n", numAssetsByType[i], assetTypeNames[i], (numAssetsByType[i] > 1 ? "s" : ""));
	}

	Com_Exit(0);
}