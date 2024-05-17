/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_smdl_load.c -- skeletal model and animation loading

/*
general notes

'models'/ - md3,smd,spr models directory
`modelskins/` is the directory from where textures load
`modelanims/` is the directory for sequences
`modeldefs/` is where model configs are stored
*/

#include "r_local.h"

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

extern int modelFileLength;
extern model_t* pLoadModel;

void R_UploadSkelModel(model_t* mod); // r_smdl.c

/*
=================
Mod_WriteOptimizedModel
Writes an optimized binary format for faster loading
=================
*/
static void Mod_WriteOptimizedModel(model_t* mod)
{
}

/*
=================
Mod_WriteOptimizedAnimation
Writes an optimized binary format for faster loading
=================
*/
static void Mod_WriteOptimizedAnimation(model_t* mod)
{
}


/*
=================
Mod_ExpectNumArgs
=================
*/
static void Mod_ExpectNumArgs(int numtokens)
{
	if(numtokens != COM_TokenNumArgs())
		ri.Printf(PRINT_LOW, "Error parsing %s at line %i: expected %i tokens but found %i.\n", pLoadModel->name, parse_line_num, numtokens, COM_TokenNumArgs());
}

/*
=================
Mod_ParseError
=================
*/
static void Mod_ParseError(char* fmt, ...)
{
	va_list			argptr;
	static char		msg[1024];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	ri.Printf(PRINT_LOW, "Error loading '%s': %s\n", pLoadModel->name,  msg);
}

/*
=================
Mod_ParseError
=================
*/
static void Mod_ParseErrorLn(char* fmt, ...)
{
	va_list			argptr;
	static char		msg[1024];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	ri.Printf(PRINT_LOW, "Error parsing model %s at line %i: %s\n", pLoadModel->name, parse_line_num, msg);
}

/*
=================
Mod_LoadSMD

Pares both SMD models and animations
=================
*/
qboolean Mod_LoadSMD(smdl_data_t *out, void* buffer, SMDL_Type loadType)
{
	int	parse_position = 0;
	int parsePoint = SMD_HEADER;
	int seq_numbones; // when parsing sequences this must match hdr->numbones
	char currentTexture[MAX_QPATH];
	char* data;

	smdl_header_t	*hdr = NULL;
	smdl_seq_t		*seq = NULL;
	smdl_surf_t		*surface = NULL;
	smdl_vert_t		*vert = NULL;

	smdl_vert_t		*temp_verts;

	temp_verts = calloc((SMDL_MAX_TRIS * 3) * SMDL_MAX_SURFACES, sizeof(smdl_vert_t));
	if (!temp_verts)
	{
		Mod_ParseError("!temp_verts");
		return false;
	}
	vert = temp_verts;

	out->type = SMDL_BAD;

	hdr = &out->hdr;

	memset(currentTexture, 0, sizeof(currentTexture));

	data = buffer;
	while (parse_position != modelFileLength)
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
				Mod_ParseErrorLn("line is too long.");
			}

			parse_line[parse_line_len] = data[parse_position];
			parse_line_len++; parse_position++;
		}
		parse_line[parse_line_len] = '\0';


		// ...and tokenize it
		COM_TokenizeString(parse_line);
		
//		ri.Printf(PRINT_LOW, "line %i argc=%i : ", lineNum,  COM_TokenNumArgs());
//		for (int j = 0; j < COM_TokenNumArgs(); j++)
//			ri.Printf(PRINT_LOW, "'%s' ", COM_TokenGetArg(j));
//		ri.Printf(PRINT_LOW, ".\n");

		//
		// VERSION
		//
		if (parsePoint == SMD_HEADER)
		{
			Mod_ExpectNumArgs(2);

			if (Q_stricmp("version", COM_TokenGetArg(0)))
			{
				Mod_ParseErrorLn("expected 'version' but found '%s'.", COM_TokenGetArg(0));
				return false;
			}
			if (Q_stricmp("1", COM_TokenGetArg(1)))
			{
				Mod_ParseErrorLn("model is wrong version (%s should be %i).", COM_TokenGetArg(1), SMDL_VERSION);
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
					Mod_ParseErrorLn("redefinition of nodes.");
					return false;
				}

				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numbones <= 0)
				{
					Mod_ParseErrorLn("model must have at least a single bone.");
					return false;
				}

				parsePoint = SMD_SKELETON;
				continue;
			}

			Mod_ExpectNumArgs(3); // <int index> <string name> <int parent_index>

			if (hdr->numbones == SMDL_MAX_BONES)
			{
				Mod_ParseErrorLn("too many bones (%i max).", SMDL_MAX_BONES);
				return false;
			}

			smdl_bone_t *bone = Hunk_Alloc(sizeof(smdl_bone_t));

			bone->index = atoi(COM_TokenGetArg(0));
			strncpy(bone->name, COM_TokenGetArg(1), MAX_QPATH);
			bone->parent = atoi(COM_TokenGetArg(2));

			if (bone->index > hdr->numbones)
			{
				Mod_ParseErrorLn("expected bone index %i but found %i.", hdr->numbones, bone->index);
				return false;
			}

			if (hdr->numbones == 0 && bone->parent != -1)
			{
				Mod_ParseErrorLn("root bone (%s) must have no parent.", bone->name);
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
					Mod_ParseErrorLn("redefinition of skeleton.");
				}
				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numframes > 0 && hdr->numbones != seq_numbones)
				{
					Mod_ParseError("sequence %i has %i bones but should have %i.", hdr->numframes, seq_numbones, hdr->numbones);
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
				Mod_ExpectNumArgs(2);

				if(loadType == SMDL_MODEL && hdr->numframes > 0)
				{
					Mod_ParseErrorLn("model has more than one frame.");
					return false;
				}

				if (hdr->numframes == SMDL_MAX_FRAMES)
				{
					Mod_ParseErrorLn("too many animation frames (%i max).", SMDL_MAX_FRAMES);
					return false;
				}

				if (atoi(COM_TokenGetArg(1)) != hdr->numframes)
				{
					Mod_ParseErrorLn("expected frame %i but found %s.", hdr->numframes, COM_TokenGetArg(1));
					return false;
				}

				if (hdr->numframes > 0 && hdr->numbones != seq_numbones)
				{
					Mod_ParseErrorLn("frame %i has %i bones but should %i.", hdr->numframes, seq_numbones, hdr->numbones);
					return false;
				}

				seq = Hunk_Alloc(sizeof(smdl_seq_t) * hdr->numbones);
				out->seqences[hdr->numframes] = seq;
				hdr->numframes++;
				seq_numbones = 0;
				continue;
			}

			Mod_ExpectNumArgs(7); // <int bone_index> <vector3 position> <vector3 rotation>

			if (!seq)
			{
				Mod_ParseErrorLn("!seq");
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

			if (s->bone > hdr->numbones || s->bone < 0)
			{
				Mod_ParseErrorLn("wrong bone index %i.", s->bone);
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
					Mod_ParseErrorLn("animation cannot have triangles.");
					return false;
				}

				if (hdr->numverts > 0)
				{
					Mod_ParseErrorLn("redefinition of triangles.");
					return false;
				}
				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numverts <= 0)
				{
					Mod_ParseError("model group has no verts.");
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

					strncpy(surface->texture, newtexture, MAX_QPATH);
					strncpy(currentTexture, newtexture, MAX_QPATH);

					if (hdr->numsurfaces > 0)
					{
						if (!out->surfaces[hdr->numsurfaces-1]->numverts)
						{
							Mod_ParseErrorLn("surface has no verts.");
							return false;
						}

						if (out->surfaces[hdr->numsurfaces - 1]->numverts < 3)
						{
							Mod_ParseErrorLn("surface has less than 3 verts.");
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
				Mod_ParseError("!surface");
				return false;
			}

			Mod_ExpectNumArgs(9); // <int bone_index> <vector3 position> <vector3 normal> <vector2 texcoords> (optional <vector3 bone weights>)

			if (hdr->numverts == SMDL_MAX_VERTS)
			{
				Mod_ParseErrorLn("Model has more than %i triangles.", SMDL_MAX_TRIS);
				return false;
			}

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

			//if (COM_TokenNumArgs() == 12) // with bone weights
			//{
			//}

			vert++;
			surface->numverts++;
			hdr->numverts++;
		}
	}

	out->verts = Hunk_Alloc(sizeof(smdl_vert_t) * hdr->numverts);
	memcpy(out->verts, temp_verts, sizeof(smdl_vert_t)* hdr->numverts);
	free(temp_verts);

	//strncpy(out->name, COM_SkipPath(pLoadModel->name), MAX_QPATH);
	out->type = loadType;

	if (loadType == SMDL_MODEL)
	{
		//ri.Printf(PRINT_LOW, "Mins/Maxs: [%f %f %f] [%f %f %f]\n", hdr->mins[0], hdr->mins[1], hdr->mins[2], hdr->maxs[0], hdr->maxs[1], hdr->maxs[2]);
		ri.Printf(PRINT_LOW, "Loaded model with %i surfaces, %i verts (%i tris total), %i bones.\n", hdr->numsurfaces, hdr->numverts, hdr->numverts / 3, hdr->numbones);
	}
	else if (loadType == SMDL_ANIMATION)
	{
		ri.Printf(PRINT_LOW, "Loaded animation with %i frames, %i bones.\n",  hdr->numframes, hdr->numbones);
	}

	return true;
}

/*
=================
Mod_LoadSMDL_Binary
=================
*/
static void Mod_LoadSMDL_Binary(model_t* mod, void* buffer, lod_t lod)
{
}


void R_UploadSkelModel(model_t* mod);

/*
=================
Mod_LoadSkelModel
=================
*/
void Mod_LoadSkelModel(model_t* mod, void* buffer, lod_t lod)
{
	qboolean loaded = false;
	smdl_surf_t* surf;
	char texturename[MAX_QPATH];

	mod->extradata = Hunk_Begin(1024*1024, "skeletal model");

	mod->smdl = Hunk_Alloc(sizeof(smdl_data_t));

	loaded = Mod_LoadSMD(mod->smdl, buffer, SMDL_MODEL);

	if (!loaded)
	{
		Hunk_Free(mod->extradata);
		mod->extradata = NULL;
		ri.Printf(PRINT_LOW, "Loading model '%s' failed.\n", mod->name);
		return;
	}

	surf = mod->smdl->surfaces[0];
	for (int i = 0; i < mod->smdl->hdr.numsurfaces; i++)
	{
		if (surf->texture[0] == '$')
		{
			mod->images[i] = R_FindTexture(surf->texture, it_model, true);
		}
		else
		{
			Com_sprintf(texturename, sizeof(texturename), "modelskins/%s", surf->texture);
			mod->images[i] = R_FindTexture(texturename, it_model, true);
		}

		if (!mod->images[i])
			mod->images[i] = r_texture_missing;

		surf->texnum = mod->images[i]->texnum;
	}


	VectorCopy(mod->smdl->hdr.mins, mod->mins);
	VectorCopy(mod->smdl->hdr.maxs, mod->maxs);
	mod->radius = RadiusFromBounds(mod->mins, mod->maxs);

	R_UploadSkelModel(mod);

	mod->type = MOD_SKEL;
}

#define MAX_ANIMATIONS 128
static smdl_anim_t animsArray[MAX_ANIMATIONS];
static unsigned int animsCount = 0;

/*
=================
AnimationForName
=================
*/
smdl_anim_t* AnimationForName(char *name, qboolean crash)
{
	smdl_anim_t	*anim;
	unsigned	*buf;
	int			i, fileLen;
	qboolean	loaded;
	char		filename[MAX_QPATH];

	if (!name[0])
		ri.Error(ERR_DROP, "%s: NULL name", __FUNCTION__);

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
			ri.Error(ERR_DROP, "%s: hit limit of %d animations", __FUNCTION__, MAX_ANIMATIONS);
		animsCount++;
	}

	Com_sprintf(filename, sizeof(filename), "modelanims/%s.smd", name);

	fileLen = ri.LoadFile(filename, &buf);
	if (!buf)
	{
		if (crash)
			ri.Error(ERR_DROP, "%s: %s not found", __FUNCTION__, anim->name);

		memset(anim->name, 0, sizeof(anim->name));
		return NULL;
	}

	anim->extradata = Hunk_Begin(1024 * 512, "animation");

	loaded = Mod_LoadSMD(&anim->data, buf, SMDL_ANIMATION);

	if (!loaded)
	{
		ri.Printf(PRINT_LOW, "Loading animation '%s' failed.\n", anim->name);
		Hunk_Free(anim->extradata);
		memset(&anim, 0, sizeof(anim));
		return NULL;
	}

	anim->extradatasize = Hunk_End();
	strncpy(anim->name, name, MAX_QPATH);

	return anim;
}