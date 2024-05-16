/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_skel.c -- skeletal models and animations

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

#define RD_MAX_SMDL_HUNKSIZE	0x400000 //r_model.c

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
Mod_LoadSMDL_Binary
=================
*/
static void Mod_LoadSMDL_Binary(model_t* mod, void* buffer, lod_t lod)
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
		ri.Error(ERR_DROP, "Error parsing %s at line %i: expected %i tokens but found %i.\n", pLoadModel->name, parse_line_num, numtokens, COM_TokenNumArgs());
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

	ri.Error(ERR_DROP, "Error parsing %s at line %i: %s\n", pLoadModel->name, parse_line_num, msg);
}

/*
=================
Mod_LoadSkelModel_Text
=================
*/
void Mod_LoadSkelModel_Text(model_t* mod, void* buffer, lod_t lod)
{
	smdl_header_t	*hdr;
	smdl_seq_t* seq = NULL;

	int	parse_position = 0;
	int parsePoint = SMD_HEADER; // what are we parsing now
	int seq_numbones; // when parsing sequences this must match hdr->numbones

	mod->extradata = Hunk_Begin(1024 * 1024 * 2, "smdl");
	hdr = Hunk_Alloc(sizeof(smdl_header_t));
	mod->smdl[lod] = hdr;

	smdl_surf_t surf;
	memset(&surf, 0, sizeof(surf));

	char* data = buffer;
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
			}
			if (Q_stricmp("1", COM_TokenGetArg(1)))
			{
				Mod_ParseErrorLn("model is wrong version (%s should be %i).", COM_TokenGetArg(1), SMDL_VERSION);
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
					Mod_ParseErrorLn("redefinition of nodes.");

				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numbones <= 0)
					Mod_ParseErrorLn("model must have at least a single bone.");

				parsePoint = SMD_SKELETON;
				continue;
			}

			Mod_ExpectNumArgs(3); // <int index> <string name> <int parent_index>

			smdl_bone_t *bone = Hunk_Alloc(sizeof(smdl_bone_t));

			bone->index = atoi(COM_TokenGetArg(0));
			strncpy(bone->name, COM_TokenGetArg(1), MAX_QPATH);
			bone->parent = atoi(COM_TokenGetArg(2));

			if (bone->index > hdr->numbones)
				Mod_ParseErrorLn("expected bone index %i but found %i.", hdr->numbones, bone->index);

			if (hdr->numbones == 0 && bone->parent != -1)
				Mod_ParseErrorLn("root bone (%s) must have no parent.", bone->name);


			mod->bones[hdr->numbones] = bone;
			hdr->numbones++;
		}

		//
		// SKELETON - REFERENCE POSE OR ANIMATION
		//
		else if (parsePoint == SMD_SKELETON)
		{
			if (!Q_stricmp("skeleton", COM_TokenGetArg(0)))
			{
				if(hdr->numframes > 0)
					Mod_ParseErrorLn("redefinition of skeleton.");
				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numframes > 0 && hdr->numbones != seq_numbones)
					Mod_ParseErrorLn("sequence %i has %i bones but should have %i.", hdr->numframes, seq_numbones, hdr->numbones);

				parsePoint = SMD_TRIANGLES;
				continue;
			}

			if (!Q_stricmp("time", COM_TokenGetArg(0)))
			{
				Mod_ExpectNumArgs(2);

				if (atoi(COM_TokenGetArg(1)) != hdr->numframes)
				{
					Mod_ParseErrorLn("expected frame %i but found %s.", hdr->numframes, COM_TokenGetArg(1));
				}

				if (hdr->numframes > 0 && hdr->numbones != seq_numbones)
					Mod_ParseErrorLn("frame %i has %i bones but should %i.", hdr->numframes, seq_numbones, hdr->numbones);

				seq = Hunk_Alloc(sizeof(smdl_seq_t) * hdr->numbones);
				mod->seqences[hdr->numframes] = seq;
				hdr->numframes++;
				seq_numbones = 0;
				continue;
			}

			Mod_ExpectNumArgs(7); // <int bone_index> <vector3 position> <vector3 rotation>

			if (!seq)
			{
				Mod_ParseErrorLn("!seq");
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
				Mod_ParseErrorLn("wrong bone index %i.", s->bone);

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
				if (hdr->numverts > 0)
					Mod_ParseErrorLn("redefinition of triangles.");
				continue;
			}

			if (!Q_stricmp("end", COM_TokenGetArg(0)))
			{
				if (hdr->numverts <= 0)
					Mod_ParseErrorLn("model group has no verts.");

				//if ((hdr->numverts & 3))
				//	Mod_ParseErrorLn("model has weird verticle count (%i).", hdr->numverts);

				parsePoint = SMD_END;
				continue;
			}

			if (COM_TokenNumArgs() == 1)
			{
				//
				// texture name, if this is diferent than the previous name we have to create a group
				//
				char* tex = COM_TokenGetArg(0);

				if (Q_stricmp(surf.texture, tex))
				{
					surf.numverts = hdr->numverts - surf.firstvert;
					//groups[hdr->numgroups] *= group;
					surf.firstvert = hdr->numverts;
					strncpy(surf.texture, tex, MAX_QPATH);
					ri.Printf(PRINT_LOW, "new surface %i for texture '%s'.\n", hdr->numsurfaces, surf.texture);
					hdr->numsurfaces++;

					continue;
				}
				continue;
			}

			Mod_ExpectNumArgs(9); // <int bone_index> <vector3 position> <vector3 normal> <vector2 texcoords> (optional <vector3 bone weights>)

			smdl_vert_t vert;

			vert.bone = atoi(COM_TokenGetArg(0));

			vert.xyz[0] = atof(COM_TokenGetArg(1));
			vert.xyz[1] = atof(COM_TokenGetArg(2));
			vert.xyz[2] = atof(COM_TokenGetArg(3));

			vert.normal[0] = atof(COM_TokenGetArg(4));
			vert.normal[1] = atof(COM_TokenGetArg(5));
			vert.normal[2] = atof(COM_TokenGetArg(6));

			vert.uv[0] = atof(COM_TokenGetArg(7));
			vert.uv[1] = atof(COM_TokenGetArg(8));

			//if (COM_TokenNumArgs() == 12) // with bone weights
			//{
			//}

			hdr->numverts++;
		}
	}

	for (int i = 0; i < hdr->numbones; i++)
	{
		ri.Printf(PRINT_LOW, "bone %i '%s', parent of %i\n", mod->bones[i]->index, mod->bones[i]->name, mod->bones[i]->parent);
	}

	for (int i = 0; i < hdr->numframes; i++)
	{
		smdl_seq_t* s = mod->seqences[i];
		ri.Printf(PRINT_LOW, "-- frame %i --\n", i);
		for (int j = 0; j < hdr->numbones; j++, s++)
		{
			
			ri.Printf(PRINT_LOW, "bone %i at (%f %f %f) (%f %f %f)\n", s->bone, s->pos[0], s->pos[1], s->pos[2], s->rot[0], s->rot[1], s->rot[2]);
		}
	}
	ri.Printf(PRINT_LOW, "%s: %i surfaces (%i tris total), %i frames, %i bones\n", mod->name, hdr->numsurfaces, hdr->numverts / 3, hdr->numframes, hdr->numbones);
	mod->type = MOD_SKEL;
}
/*
=================
Mod_LoadSkelModel
=================
*/
void Mod_LoadSkelModel(model_t* mod, void* buffer, lod_t lod)
{
	char	*hdr = buffer;

	mod->type = MOD_BAD;

	//if (modelFileLength > 7)
	//{	
		//if (hdr[0] == 'v' && hdr[1] == 'e' && hdr[2] == 'r' && hdr[3] == 's' && hdr[4] == 'i' && hdr[5] == 'o' && hdr[5] == 'n') // this is ugly
		//{
			Mod_LoadSkelModel_Text(mod, buffer, lod);
		//	return;
		//}
	//}
}


/*
=================
Mod_LoadAnimation
=================
*/
void Mod_LoadAnimation(model_t* mod, void* buffer, lod_t lod)
{

}

/*
=================
Mod_LoadAnimation
=================
*/
void R_DrawSkelModel(rentity_t* ent, lod_t lod, float animlerp)
{
	qboolean		anythingToDraw = false;

	if (lod < 0 || lod >= NUM_LODS)
		ri.Error(ERR_DROP, "R_DrawMD3Model: '%s' wrong LOD num %i\n", ent->model->name, lod);

//	pModel = NULL;
//	if (pModel == NULL)
//	{
//		Com_Printf("R_DrawMD3Model: '%s' has no LOD %i model\n", ent->model->name, lod);
//		return;
//	}

//	if (!ent->model->vb[0])
//		return; // not uploaded to gpu
}