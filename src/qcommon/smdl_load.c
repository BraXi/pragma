/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// smdl_load.c -- skeletal model and animation loading shared with other modules

/*
general notes

'models'/ - smd models directory
`modelanims/` is for smd animations

`modelskins/` is the directory from where textures load
`modeldefs/` is where model configs are stored
*/

#include "qcommon.h"

enum SMD_Def
{
	SMD_NONE,
	SMD_HEADER,
	SMD_NODES,
	SMD_SKELETON,
	SMD_TRIANGLES,
	SMD_END
};

static char parse_line[256];
static int parse_line_len, parse_line_num = 1;

static int loadFileLength = -1;
static char *loadFileName = NULL;

static smdl_vert_t* temp_verts = NULL;


/*
=================
SMD_ClipBoneRotation
clip bone rotaton between -pi <= rot < pi
=================
*/
static void SMD_ClipBoneRotation(vec3_t bonerot)
{
	
	for (int i = 0; i < 3; i++) 
	{

		while (bonerot[i] >= M_PI)
			bonerot[i] -= M_PI * 2;
		while (bonerot[i] < -M_PI)
			bonerot[i] += M_PI * 2;
	}
}

/*
=================
SMD_CleanUp
An error during parsing has happened so clean up, note that out should be freed by user
=================
*/
static void SMD_CleanUp()
{
	loadFileLength = -1;
	parse_line_len = 0;
	parse_line_num = 1;

	if (temp_verts != NULL)
	{
		free(temp_verts);
		temp_verts = NULL;
	}
}


/*
=================
ParseSMD_ExpectNumArgs
=================
*/
static void ParseSMD_ExpectNumArgs(int numtokens)
{
	if (numtokens != COM_TokenNumArgs())
	{
		Com_Printf("Error parsing %s at line %i: expected %i tokens but found %i.\n", loadFileName, parse_line_num, numtokens, COM_TokenNumArgs());
		SMD_CleanUp();
	}
}

/*
=================
SMD_ParseError
=================
*/
static void SMD_ParseError(char* fmt, ...)
{
	va_list			argptr;
	static char		msg[1024];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	Com_Printf("Error loading '%s': %s\n", loadFileName, msg);
	SMD_CleanUp();
}

/*
=================
SMD_ParseError
=================
*/
static void SMD_ParseErrorAtLine(char* fmt, ...)
{
	va_list			argptr;
	static char		msg[1024];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	Com_Printf("Error parsing model %s at line %i: %s\n", loadFileName, parse_line_num, msg);
}

/*
=================
Com_LoadAnimOrModel

Parses SMD file and returns true on success.
smd_data_t *out contains model or animation data dependng what loadType was

Most stupid parse errors are forgiven, you must allocate *out with Hunk_Alloc and pass the file *buffer before use
=================
*/
qboolean Com_LoadAnimOrModel(SMDL_Type loadType, smdl_data_t* out, char *name, int fileLength, void *buffer)
{
	int	parse_position = 0;
	int parsePoint = SMD_HEADER;
	int seq_numbones; // when parsing sequences this must match hdr->numbones
	char currentTexture[MAX_QPATH];
	char* data;

	smdl_header_t* hdr = NULL;
	smdl_seq_t* seq = NULL;
	smdl_surf_t* surface = NULL;
	smdl_vert_t* vert = NULL;

	if (!name || !name[0]) // returns down here are to shut up msvc...
	{
		Com_Error(ERR_FATAL, "%s called with NULL name", __FUNCTION__);
		return false;
	}

	if (!out)
	{
		Com_Error(ERR_FATAL, "%s called for `%s` but out is NULL", __FUNCTION__, name);
		return false;
	}

	if (!buffer || !fileLength)
	{
		Com_Error(ERR_FATAL, "%s called for `%s` but buffer or fileLength is NULL", __FUNCTION__, name);
		return false;
	}

	if (loadType != SMDL_MODEL && loadType != SMDL_MODEL_NO_TRIS && loadType != SMDL_ANIMATION)
	{
		Com_Error(ERR_FATAL, "%s with weird loadType", __FUNCTION__);
		return false;
	}

	loadFileName = name;
	loadFileLength = fileLength;

	if (loadType == SMDL_MODEL)
	{
		temp_verts = calloc(SMDL_MAX_VERTS, sizeof(smdl_vert_t));
		if (!temp_verts)
		{
			Com_Error(ERR_FATAL, "%s failed to allocate temp_verts", __FUNCTION__, name); // gib moar rrraaammm
			return false;
		}
		vert = temp_verts;
	}

	out->type = SMDL_BAD;

	hdr = &out->hdr;

	memset(currentTexture, 0, sizeof(currentTexture));

	data = buffer;
	while (parse_position != loadFileLength)
	{
		// read line by line
		memset(parse_line, 0, sizeof(parse_line));
		parse_line_len = 0;

		if (data[parse_position] == '\n' || data[parse_position] == 0)
		{
			parse_position++;
			parse_line_num++;
			continue;
		}

		parse_line_len = 0;
		while (data[parse_position] != '\n' && data[parse_position] != '\0')
		{
			if (parse_line_len == 254)
			{
				SMD_ParseErrorAtLine("line is too long.");
			}

			parse_line[parse_line_len] = data[parse_position];
			parse_line_len++; parse_position++;
		}
		parse_line[parse_line_len] = '\0';

		// ...and tokenize it
		COM_TokenizeString(parse_line);

		//	ri.Printf(PRINT_LOW, "line %i argc=%i : ", lineNum,  COM_TokenNumArgs());
		//	for (int j = 0; j < COM_TokenNumArgs(); j++)
		//	ri.Printf(PRINT_LOW, "'%s' ", COM_TokenGetArg(j));
		//ri.Printf(PRINT_LOW, ".\n");

		//
		// VERSION
		//
		if (parsePoint == SMD_HEADER)
		{
			ParseSMD_ExpectNumArgs(2);

			if (Q_stricmp("version", COM_TokenGetArg(0)))
			{
				SMD_ParseErrorAtLine("expected 'version' but found '%s'.", COM_TokenGetArg(0));
				return false;
			}
			if (Q_stricmp("1", COM_TokenGetArg(1)))
			{
				SMD_ParseErrorAtLine("model is wrong version (%s should be %i).", COM_TokenGetArg(1), SMDL_VERSION);
				return false;
			}
			parsePoint = SMD_NODES;
			continue;
		}

		//
		// NODES == BONES
		//
		else if (parsePoint == SMD_NODES)
		{
			if (!Q_stricmp("nodes", COM_TokenGetArg(0)))
			{
				if (hdr->numbones > 0)
				{
					SMD_ParseErrorAtLine("redefinition of nodes.");
					return false;
				}

				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numbones <= 0)
				{
					SMD_ParseErrorAtLine("model must have at least a single bone.");
					return false;
				}

				parsePoint = SMD_SKELETON;
				continue;
			}

			ParseSMD_ExpectNumArgs(3); // <int index> <string name> <int parent_index>

			if (hdr->numbones == SMDL_MAX_BONES)
			{
				SMD_ParseErrorAtLine("too many bones (%i max).", SMDL_MAX_BONES);
				return false;
			}

			smdl_bone_t* bone = Hunk_Alloc(sizeof(smdl_bone_t));

			bone->index = atoi(COM_TokenGetArg(0));
			strncpy(bone->name, COM_TokenGetArg(1), MAX_QPATH);
			bone->parent = atoi(COM_TokenGetArg(2));

			if (bone->index > hdr->numbones)
			{
				SMD_ParseErrorAtLine("expected bone index %i but found %i.", hdr->numbones, bone->index);
				return false;
			}

			if (hdr->numbones == 0 && bone->parent != -1)
			{
				SMD_ParseErrorAtLine("root bone (%s) must have no parent.", bone->name);
				return false;
			}


			out->bones[hdr->numbones] = bone;
			hdr->numbones++;
		}

		//
		// SKELETON - REFERENCE POSE OR ANIMATION
		//
		else if (parsePoint == SMD_SKELETON)
		{
			if (!Q_stricmp("skeleton", COM_TokenGetArg(0)))
			{
				if (hdr->numframes > 0)
				{
					SMD_ParseErrorAtLine("redefinition of skeleton.");
				}
				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numframes > 0 && hdr->numbones != seq_numbones)
				{
					SMD_ParseError("sequence %i has %i bones but should have %i.", hdr->numframes, seq_numbones, hdr->numbones);
					return false;
				}

				if (loadType == SMDL_ANIMATION)
				{
					parsePoint = SMD_END;
				}
				else
				{
					parsePoint = SMD_TRIANGLES;
				}

				continue;
			}

			if (!Q_stricmp("time", COM_TokenGetArg(0)))
			{
				ParseSMD_ExpectNumArgs(2);

				if (loadType == SMDL_MODEL && hdr->numframes > 0)
				{
					SMD_ParseErrorAtLine("model has more than one frame.");
					return false;
				}

				if (hdr->numframes == SMDL_MAX_FRAMES)
				{
					SMD_ParseErrorAtLine("too many animation frames (%i max).", SMDL_MAX_FRAMES);
					return false;
				}

				if (atoi(COM_TokenGetArg(1)) != hdr->numframes)
				{
					SMD_ParseErrorAtLine("expected frame %i but found %s.", hdr->numframes, COM_TokenGetArg(1));
					return false;
				}

				if (hdr->numframes > 0 && hdr->numbones != seq_numbones)
				{
					SMD_ParseErrorAtLine("frame %i has %i bones but should %i.", hdr->numframes, seq_numbones, hdr->numbones);
					return false;
				}

				seq = Hunk_Alloc(sizeof(smdl_seq_t) * hdr->numbones);
				out->seqences[hdr->numframes] = seq;
				hdr->numframes++;
				seq_numbones = 0;
				continue;
			}

			ParseSMD_ExpectNumArgs(7); // <int bone_index> <vector3 position> <vector3 rotation>

			if (!seq)
			{
				SMD_ParseErrorAtLine("!seq");
				return false;
			}

			smdl_seq_t* s = seq;

			s->bone = atoi(COM_TokenGetArg(0));

			s->pos[0] = atof(COM_TokenGetArg(1));
			s->pos[1] = atof(COM_TokenGetArg(2));
			s->pos[2] = atof(COM_TokenGetArg(3));

			s->rot[0] = atof(COM_TokenGetArg(4));
			s->rot[1] = atof(COM_TokenGetArg(5));
			s->rot[2] = atof(COM_TokenGetArg(6));

			SMD_ClipBoneRotation(s->rot);

			if (s->bone > hdr->numbones || s->bone < 0)
			{
				SMD_ParseErrorAtLine("wrong bone index %i.", s->bone);
				return false;
			}

			seq++;
			seq_numbones++;
		}

		//
		// TRIANGLES
		//
		else if (parsePoint == SMD_TRIANGLES)
		{
			if (!Q_stricmp("triangles", COM_TokenGetArg(0)))
			{
				if (loadType == SMDL_ANIMATION)
				{
					SMD_ParseErrorAtLine("animation cannot have triangles.");
					return false;
				}

				if (hdr->numverts > 0)
				{
					SMD_ParseErrorAtLine("redefinition of triangles.");
					return false;
				}
				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numverts <= 0)
				{
					SMD_ParseError("model has no verts.");
					return false;
				}

				parsePoint = SMD_END;
				break;
			}

			if (COM_TokenNumArgs() == 1)
			{
				//
				// texture name, if this is diferent than the previous name we have to create a group
				//
				char* newtexture = COM_TokenGetArg(0);

				if (Q_stricmp(currentTexture, newtexture))
				{
					out->surfaces[hdr->numsurfaces] = Hunk_Alloc(sizeof(smdl_surf_t));
					surface = out->surfaces[hdr->numsurfaces];

					strncpy(surface->name, va("surf_%i", hdr->numsurfaces), MAX_QPATH);

					strncpy(surface->texture, newtexture, MAX_QPATH);
					strncpy(currentTexture, newtexture, MAX_QPATH);

					if (hdr->numsurfaces > 0)
					{
						if (!out->surfaces[hdr->numsurfaces - 1]->numverts)
						{
							SMD_ParseErrorAtLine("surface has no verts.");
							return false;
						}

						if (out->surfaces[hdr->numsurfaces - 1]->numverts < 3)
						{
							SMD_ParseErrorAtLine("surface has less than 3 verts.");
							return false;
						}
						surface->firstvert = (out->surfaces[hdr->numsurfaces - 1]->numverts + 1);
					}
					else
					{
						surface->firstvert = 0;
					}

					//ri.Printf(PRINT_LOW, "new surface %i for texture '%s'.\n", hdr->numsurfaces, surface->texture);
					hdr->numsurfaces++;
				}
				continue;
			}

			if (!surface)
			{
				SMD_ParseError("!surface");
				return false;
			}

			ParseSMD_ExpectNumArgs(9); // <int bone_index> <vector3 position> <vector3 normal> <vector2 texcoords> (optional <vector3 bone weights>)

			if (hdr->numverts == SMDL_MAX_VERTS)
			{
				SMD_ParseErrorAtLine("Model has more than %i triangles.", SMDL_MAX_TRIS);
				return false;
			}

			if (loadType == SMDL_MODEL)
			{
				vert->bone = atoi(COM_TokenGetArg(0));

				vert->xyz[0] = atof(COM_TokenGetArg(1));
				vert->xyz[1] = atof(COM_TokenGetArg(2));
				vert->xyz[2] = atof(COM_TokenGetArg(3));

				vert->normal[0] = atof(COM_TokenGetArg(4));
				vert->normal[1] = atof(COM_TokenGetArg(5));
				vert->normal[2] = atof(COM_TokenGetArg(6));

				vert->uv[0] = atof(COM_TokenGetArg(7));
				vert->uv[1] = -atof(COM_TokenGetArg(8)); // must negate uv

				AddPointToBounds(vert->xyz, hdr->mins, hdr->maxs);
				vert++;
			}
			else if (loadType == SMDL_MODEL_NO_TRIS) // only calculate bounds
			{
				vec3_t xyz;

				xyz[0] = atof(COM_TokenGetArg(1));
				xyz[1] = atof(COM_TokenGetArg(2));
				xyz[2] = atof(COM_TokenGetArg(3));
				AddPointToBounds(xyz, hdr->mins, hdr->maxs);
			}
			else
			{
				Com_Error(ERR_FATAL, "no way.\n");
			}

			//if (COM_TokenNumArgs() == 12) // with bone weights
			//{
			//}

			surface->numverts++;
			hdr->numverts++;
		}
	}

	if (loadType == SMDL_MODEL)
	{
		unsigned int vertsize = (sizeof(smdl_vert_t) * hdr->numverts);
		out->verts = Hunk_Alloc(vertsize);
		memcpy(out->verts, temp_verts, vertsize);
		free(temp_verts);

		hdr->boundingradius = RadiusFromBounds(hdr->mins, hdr->maxs);

		//Com_DPrintf(DP_ALL, "Mins/Maxs: [%f %f %f] [%f %f %f]\n", hdr->mins[0], hdr->mins[1], hdr->mins[2], hdr->maxs[0], hdr->maxs[1], hdr->maxs[2]);
		Com_DPrintf(DP_ALL, "Loaded model with %i surfaces, %i verts (%i tris total), %i bones.\n", hdr->numsurfaces, hdr->numverts, hdr->numverts / 3, hdr->numbones);
	}
	else if (loadType == SMDL_ANIMATION)
	{
		Com_DPrintf(DP_ALL, "Loaded animation with %i frames, %i bones.\n", hdr->numframes, hdr->numbones);
	}

	out->type = loadType;
	//strncpy(out->name, COM_SkipPath(pLoadModel->name), MAX_QPATH);

	return true;
}




/*
=================
Com_WriteBinaryModel
Writes an optimized binary format for faster loading
=================
*/
static void Com_WriteBinaryModel(smdl_data_t* mod)
{
}

/*
=================
Com_WriteBinaryAnimation
Writes an optimized binary format for faster loading
=================
*/
static void Com_WriteBinaryAnimation(smdl_data_t* mod)
{
}



// 


#define MAX_ANIMATIONS 128
static smdl_anim_t animsArray[MAX_ANIMATIONS];
static unsigned int animsCount = 0;



/*
=================
Com_AnimationForName
=================
*/
smdl_anim_t* Com_AnimationForName(char* name, qboolean crash)
{
	smdl_anim_t* anim;
	unsigned* buf;
	int			i, fileLen;
	qboolean	loaded;
	char		filename[MAX_QPATH];

	if (!name[0])
		Com_Error(ERR_DROP, "%s: NULL name", __FUNCTION__);

	for (i = 0, anim = animsArray; i < animsCount; i++, anim++)
	{
		if (!anim->name[0])
			continue;

		if (!strcmp(anim->name, name))
			return anim;
	}

	for (i = 0, anim = animsArray; i < animsCount; i++, anim++)
	{
		if (!anim->name[0])
			break;
	}

	if (i == animsCount)
	{
		if (animsCount == MAX_ANIMATIONS)
			Com_Error(ERR_DROP, "%s: hit limit of %d animations", __FUNCTION__, MAX_ANIMATIONS);
		animsCount++;
	}

	Com_sprintf(filename, sizeof(filename), "modelanims/%s.smd", name);

	fileLen = FS_LoadFile(filename, &buf);
	if (!buf)
	{
		if (crash)
			Com_Error(ERR_DROP, "%s: animation `%s` not found.\n", __FUNCTION__, anim->name);

		memset(anim->name, 0, sizeof(anim->name));
		return NULL;
	}

	anim->extradata = Hunk_Begin(1024 * 256, "animation"); // 256kb should be more than plenty?
	loaded = Com_LoadAnimOrModel(SMDL_MODEL, &anim->data, name, fileLen, buf);

	FS_FreeFile(buf);

	if (!loaded)
	{
		Hunk_Free(anim->extradata);
		memset(&anim, 0, sizeof(anim));

		if (crash)
			Com_Error(ERR_DROP, "%s: failed to load animation `%s`.\n", __FUNCTION__, anim->name);

		return NULL;
	}

	anim->data.hdr.playrate = SANIM_FPS;

	anim->extradatasize = Hunk_End();
	strncpy(anim->name, name, MAX_QPATH);

	return anim;
}
