/*
prtool, part of pragma
Copyright (C) 2024 BraXi.

Contains small portions of Quake 2 Engine source code,
which is Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "prtool.h"

typedef enum { SMD_ERROR, SMD_HEADER, SMD_NODES, SMD_SKELETON, SMD_TRIANGLES, SMD_END } smdpoint_t; // What is parsed right now

static int numTransforms = 0;
static smddata_t* pRawSMD = NULL; // ptr to smddata thats being loaded
static int line_num; // curent line
static smdpoint_t parsePoint; // what is being parsed
static char currentTexture[MAX_QPATH]; // if texture != currentTexture, create new surface
static int numBonesInFrame; // this must match smddata->numbones every frame
static smd_surface_t* pSurface = NULL; // surface we're building

/*
=================
FreeSMD
Frees all smddata_t
=================
*/
void FreeSMD(smddata_t* pData)
{
	int i;

	if (!pData)
	{
		Com_Warning("Tried to free() after free(), nasty");
		return;
	}

	if (pData->numbones)
	{
		for (i = 0; i < pData->numbones; i++)
			free(pData->vBones[i]);
	}

	if (pData->numframes) // ha, wrong
	{
		for (i = 0; i < pData->numframes* pData->numbones; i++)
			free(pData->vBoneTransforms[i]);
	}

	if (pData->numverts)
	{
		for (i = 0; i < pData->numverts; i++)
			free(pData->vVerts[i]);
	}

	if (pData->numsurfaces)
	{
		for (i = 0; i < pData->numsurfaces; i++)
			free(pData->vSurfaces[i]);
	}

	free(pData);
	//pRawSMD = NULL;
}

/*
=================
SMD_ParserExpectArgs
=================
*/
static void SMD_ParserExpectArgs(int numargs)
{
	if (numargs != Com_NumArgs())
	{
		Com_Error2("[%s:%i]: expected %i tokens but found %i.", pRawSMD->name, line_num, numargs, Com_NumArgs());
		parsePoint = SMD_ERROR;
		FreeSMD(pRawSMD);
		pRawSMD = NULL;
	}
}


/*
=================
SMD_ParseErrorAtLine
=================
*/
static void SMD_ParseErrorAtLine(const char* text, ...)
{
	va_list			argptr;
	static char		msg[MAXPRINTMSG];

	va_start(argptr, text);
	vsnprintf(msg, sizeof(msg), text, argptr);
	va_end(argptr);

	Com_Error2("[%s:%i]: %s", pRawSMD->name, line_num, msg);

	parsePoint = SMD_ERROR;
	FreeSMD(pRawSMD);
	pRawSMD = NULL;
}


/*
=================
SMD_ParseHeader
=================
*/
static void SMD_ParseHeader()
{
	SMD_ParserExpectArgs(2);

	
	if (Com_stricmp("version", Com_GetArg(0)))
	{
		SMD_ParseErrorAtLine("Expected \"version\" but found \"%s\".", Com_GetArg(0));
		return;
	}

	if (Com_stricmp("1", Com_GetArg(1)))
	{
		SMD_ParseErrorAtLine("SMD is wrong version %s (should be 1).", Com_GetArg(1));
		return;
	}

	parsePoint = SMD_NODES;
}


/*
=================
SMD_ParseNodes
=================
*/
static void SMD_ParseNodes()
{
	smd_bone_t* pNewBone = NULL;
	int index;
	char *name;
	int parent;

	// "nodes" begins parsing of nodes
	if (!Com_stricmp("nodes", Com_GetArg(0)))
	{
		if (pRawSMD->numbones > 0)
		{
			SMD_ParseErrorAtLine("Found redefinition of nodes (which is bad).");
		}
		return;
	}

	// "end" marks the end of nodes, "skeleton" should be parsed next
	if (!Com_stricmp("end", Com_GetArg(0)))
	{
		if (pRawSMD->numbones <= 0)
		{
			SMD_ParseErrorAtLine("Model has no bones.");
			return;
		}
		parsePoint = SMD_SKELETON;
		return;
	}

	//
	// read nodes (bones) as follows: <int bone index> <string bone name> <int parent bone index>
	//
	SMD_ParserExpectArgs(3); 

	index = atoi(Com_GetArg(0));
	name = Com_GetArg(1);
	parent = atoi(Com_GetArg(2));

	if (pRawSMD->numbones == SMDL_MAX_BONES)
	{
		SMD_ParseErrorAtLine("Too many bones (max is %i).", SMDL_MAX_BONES);
		return;
	}

	if (strlen(name) >= 32) //if (strlen(name) >= MAX_QPATH)
	{
		SMD_ParseErrorAtLine("Bone %i (%s) has name longer than %i characters.", pRawSMD->numbones, name, 32);
		return;
	}

	if (index > pRawSMD->numbones)
	{
		SMD_ParseErrorAtLine("Expected bone index %i but found %i.", pRawSMD->numbones, index);
		return;
	}

	if (pRawSMD->numbones == 0 && parent != -1)
	{
		SMD_ParseErrorAtLine("Bone (%s) has parent but is a root bone.", name);
		return;
	}


	//
	// Allocate a new bone
	//
	pNewBone = (smd_bone_t*)Com_SafeMalloc(sizeof(smdl_bone_t), __FUNCTION__);
	
	pNewBone->index = index;
	pNewBone->parent = parent;

	strncpy(pNewBone->name, name, MAX_QPATH);
	_strlwr(pNewBone->name); // lowercase bone name

	pRawSMD->vBones[pRawSMD->numbones] = pNewBone;
	//pRawSMD->vBones.push_back(pNewBone);
	pRawSMD->numbones++;
}

/*
=================
SMD_ParseSkeleton
=================
*/
void SMD_ParseSkeleton()
{
	int thisFrameNum, boneIndex;
	vec3_t bone_pos, bone_rot;
	smd_bonetransform_t* pNewTransform = NULL;

	// "skeleton" begins key frames
	if (!Com_stricmp("skeleton", Com_GetArg(0)))
	{
		if (pRawSMD->numframes > 0)
		{
			SMD_ParseErrorAtLine("Redefinition of skeleton.");
		}
		numTransforms = 0;
		return;
	}

	// "end" marks the end of skeleton (frames), depending if we're dealing  
	// with model or animation we expect "triangles" or end of file next
	if (!Com_stricmp("end", Com_GetArg(0)))
	{
		if (pRawSMD->numframes > 0 && pRawSMD->numbones != numBonesInFrame)
		{
			SMD_ParseErrorAtLine("Frame %i has %i bones but should have %i bones.", pRawSMD->numframes, numBonesInFrame, pRawSMD->numbones);
			return;
		}

		// go to end if this is the animation otherwise read vertexes
		parsePoint = (pRawSMD->type == SMD_ANIMATION) ? SMD_END : SMD_TRIANGLES;
		return;
	}

	// "time <number>" marks a beginning of animation frame (time 0 is reference skeleton pose for models)
	if (!Com_stricmp("time", Com_GetArg(0)))
	{
		SMD_ParserExpectArgs(2);
		thisFrameNum = atoi(Com_GetArg(1));

		// if this is a model we only want first frame which is base skeleton
		if (pRawSMD->type == SMDL_MODEL && pRawSMD->numframes > 0)
		{
			SMD_ParseErrorAtLine("Model can only have one frame referencing base skeleton.");
			return;
		}

		if (pRawSMD->numframes >= SMDL_MAX_FRAMES)
		{
			SMD_ParseErrorAtLine("Too many animation frames (max is %i).", SMDL_MAX_FRAMES);
			return;
		}

		if (thisFrameNum != pRawSMD->numframes)
		{
			SMD_ParseErrorAtLine("Expected frame %i but found %i.", pRawSMD->numframes, thisFrameNum);
			return;
		}

		if (pRawSMD->numframes > 0 && pRawSMD->numbones != numBonesInFrame)
		{
			SMD_ParseErrorAtLine("Frame %i influences %i bones, but should %i.", pRawSMD->numframes, numBonesInFrame, pRawSMD->numbones);
			return;
		}

		pRawSMD->numframes++;
		numBonesInFrame = 0;
		return;
	}

	// read bones transformations
	SMD_ParserExpectArgs(7); // <int bone_index> <vector3 position> <vector3 rotation>

	boneIndex = atoi(Com_GetArg(0)); // bone that's influenced
	bone_pos[0] = (float)atof(Com_GetArg(1));
	bone_pos[1] = (float)atof(Com_GetArg(2));
	bone_pos[2] = (float)atof(Com_GetArg(3));
	bone_rot[0] = (float)atof(Com_GetArg(4));
	bone_rot[1] = (float)atof(Com_GetArg(5));
	bone_rot[2] = (float)atof(Com_GetArg(6));

	if (boneIndex > pRawSMD->numbones || boneIndex < 0)
	{
		SMD_ParseErrorAtLine("Wrong bone index %i.", boneIndex);
		return;
	}

	//
	// Allocate a new bone transformation
	//
	pNewTransform = (smd_bonetransform_t*)Com_SafeMalloc(sizeof(smd_bonetransform_s), __FUNCTION__);
	pNewTransform->bone = boneIndex;
	for (int i = 0; i < 3; i++)
	{
		pNewTransform->position[i] = bone_pos[i];
		pNewTransform->rotation[i] = bone_rot[i];
	}
	pRawSMD->vBoneTransforms[numTransforms] = pNewTransform;
	numTransforms++;
	numBonesInFrame++;
}

/*
=================
SMD_ParseTriangles
=================
*/
static void SMD_ParseTriangles()
{
	smd_surface_t *pNewSurface, *pPrevSurf;
	smd_vert_t *pNewVert;
	char *texture;

	// "triangles" begin vertexes
	if (!Com_stricmp("triangles", Com_GetArg(0)))
	{
		if (pRawSMD->type == SMDL_ANIMATION)
		{
			SMD_ParseErrorAtLine("This SMD has triangles, and we wanted an animation.");
			return;
		}

		if (pRawSMD->numverts > 0)
		{
			SMD_ParseErrorAtLine("Redefinition of triangles.");
			return;
		}
		return;
	}

	// "end" marks an end of vertexes and we're done reading entire SMD
	if (!Com_stricmp("end", Com_GetArg(0)))
	{
		if (pRawSMD->numverts <= 0)
		{
			SMD_ParseErrorAtLine("Model has no vertexes.");
			return;
		}

		// go to the end
		parsePoint = SMD_END;
		return;
	}

	// this is very likely texture name
	if (Com_NumArgs() == 1)
	{
		texture = Com_GetArg(0);

		if (Com_stricmp(currentTexture, texture))
		{
			// This texture is diferent than previous - allocate a new surface

			pNewSurface = (smd_surface_t*)Com_SafeMalloc(sizeof(smd_surface_s), __FUNCTION__);	
			pSurface = pNewSurface;
			pRawSMD->vSurfaces[pRawSMD->numsurfaces] = pNewSurface;

			strncpy(pSurface->texture, texture, MAX_QPATH);
			_strlwr(pSurface->texture); // lowercase texture name for linux
			strncpy(currentTexture, pSurface->texture, MAX_QPATH);

			if (pRawSMD->numsurfaces > 0)
			{
				pPrevSurf = pRawSMD->vSurfaces[pRawSMD->numsurfaces - 1];
				if (!pPrevSurf->numverts)
				{
					SMD_ParseErrorAtLine("Surface %i has no vertexes.", pRawSMD->numsurfaces - 1);
					return;
				}

				if (pPrevSurf->numverts < 3)
				{
					SMD_ParseErrorAtLine("Surface %i has less than 3 vertexes (needs to have at least 3).", pRawSMD->numsurfaces - 1);
					return;
				}
				pSurface->firstvert = (pPrevSurf->firstvert + pPrevSurf->numverts + 1);
			}
			else
			{
				// this is the first surface, and the upper if() is not :P
				pSurface->firstvert = 0;
			}

			//if(g_verbose)
			//Com_Printf("Surface %i uses texture \"%s\".\n", pRawSMD->numsurfaces, pSurface->texture);

			pRawSMD->numsurfaces++;
		}
		return;
	}

	if (!pSurface)
	{
		Com_Error("pSurface == NULL"); // just in case, we can peacefuly crash i guess
		return;
	}

	// read vertex
	// <int bone_index> <vector3 position> <vector3 normal> <vector2 texcoords>
	SMD_ParserExpectArgs(9); 

	if (pRawSMD->numverts >= SMDL_MAX_VERTS)
	{
		SMD_ParseErrorAtLine("Model has more than %i triangles.", SMDL_MAX_TRIS);
		return;
	}


	// Allocate a new vertex
	pNewVert = (smd_vert_t*)Com_SafeMalloc(sizeof(smd_vert_s), __FUNCTION__);

	pNewVert->bone = atoi(Com_GetArg(0));
	if (pNewVert->bone >= pRawSMD->numbones || pNewVert->bone < 0)
	{
		SMD_ParseErrorAtLine("A vertex references bone that does not exist (%i).", pNewVert->bone);
	}

	pNewVert->xyz[0] = (float)atof(Com_GetArg(1));
	pNewVert->xyz[1] = (float)atof(Com_GetArg(2));
	pNewVert->xyz[2] = (float)atof(Com_GetArg(3));
	pNewVert->normal[0] = (float)atof(Com_GetArg(4));
	pNewVert->normal[1] = (float)atof(Com_GetArg(5));
	pNewVert->normal[2] = (float)atof(Com_GetArg(6));
	pNewVert->uv[0] = (float)atof(Com_GetArg(7));
	pNewVert->uv[1] = (float)atof(Com_GetArg(8));

	// FIXME: need AddPointToBounds in tools
	//AddPointToBounds(pNewVert->xyz, pRawSMD->mins, pRawSMD->maxs);

	pRawSMD->vVerts[pRawSMD->numverts] = pNewVert;
	pSurface->numverts++;
	pRawSMD->numverts++;
}


/*
=================
InitSMDParser
=================
*/
static void InitSMDParser()
{
	//pRawSMD = NULL;
	line_num = 0;
	parsePoint = SMD_HEADER;
	memset(currentTexture, 0, sizeof(currentTexture));
	numBonesInFrame = 0;
	pSurface = NULL;
	numTransforms = 0;
}

/*
=================
SmdTypeAsStr
=================
*/
static const char* SmdTypeAsStr(const smdtype_t type)
{
	if (type == SMD_MODEL)
		return "model";
	return "animation";
}

/*
=================
LoadSMD
Loads SMD source file and expect it to be either animation or model, and return raw data on success
=================
*/
smddata_t *LoadSMD(const smdtype_t expect, const char *name, const char* filename)
{
	FILE* f;
	smddata_t* data;
	static char line[MAXLINELEN];

	f = Com_OpenReadFile(filename, false);
	if (!f)
	{
		//Com_Warning("Couldn't open: %s", filename);
		return NULL;
	}

	InitSMDParser();

	data = (smddata_t*)Com_SafeMalloc(sizeof(smddata_t), __FUNCTION__);
	data->type = expect;
	strncpy(data->name, name, sizeof(data->name));
	pRawSMD = data;

	//Com_Printf("Loading %s: %s\n", SmdTypeAsStr(data->type), name);

	while (fgets(line, sizeof(line), f) != NULL)
	{
		line_num++;

		Com_Tokenize(line);
		//if (Com_NumArgs() == 0)
		//{
		//	continue;
		//}

		//printf("line %i: '%s'\n", line_num, line);
		switch (parsePoint)
		{
		case SMD_ERROR:
			Com_Warning("Failed to load %s %s due to errors.", SmdTypeAsStr(expect), name);
			//FreeSMD(data);
			data = pRawSMD = NULL;
			goto done_reading;
			break;

		case SMD_HEADER:
			SMD_ParseHeader();
			break;

		case SMD_NODES:
			SMD_ParseNodes();
			break;

		case SMD_SKELETON:
			SMD_ParseSkeleton();
			break;

		case SMD_TRIANGLES:
			SMD_ParseTriangles();
			break;

		case SMD_END:
			// FIXME: need RadiusFromBounds in tools
			//pRawSMD->radius = RadiusFromBounds(pRawSMD->mins, pRawSMD->maxs);
			goto done_reading;
			break;
		}
	}

done_reading:

	//if (data)
	//	Com_Printf("... loaded %s: %s\n", SmdTypeAsStr(data->type), name);
	fclose(f);
	return data;
}