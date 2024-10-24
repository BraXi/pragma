/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_model.c -- model loading and caching

#include "r_local.h"

int		registration_sequence; // increased with each level load

model_t	*pLoadModel;		// ptr to model which is being loaded
int		modelFileLength;	// length of loaded model file

#define RD_MAX_PMOD_HUNKSIZE	0x400000 // 4 MB
#define RD_MAX_MD3_HUNKSIZE		0x400000 // 4 MB
#define RD_MAX_BSP_HUNKSIZE		0x1000000 // 16 MB

#define	RD_MAX_MODELS	1024
static model_t	r_models[RD_MAX_MODELS];
static int		r_models_count;
model_t			r_inlineModels[RD_MAX_MODELS]; // the inline "*" brush models from the current map are kept seperate

void Mod_LoadAliasMD3(model_t* mod, void* buffer);
void R_LoadNewModel(model_t* mod, void* buffer);
void Mod_LoadNewModelTextures(model_t* mod);;

/*
=================
R_ModelForNum

Returns the model_t for index
=================
*/
model_t* R_ModelForNum(int index) 
{
	model_t* mod;

	// out of range gets the default model
	if (index < 0 || index >= r_models_count)
	{
		return r_defaultmodel; // NULL
	}

	mod = &r_models[index];

	return mod;
}

/*
==================
R_ModelForName

Loads in a model for the given name
==================
*/
model_t* R_ModelForName(const char* name, qboolean crash)
{
	model_t* mod;
	unsigned* buf;
	int		i;

	if (!name[0])
		ri.Error(ERR_DROP, "%s: called with NULL name.\n", __FUNCTION__);

	//
	// inline models are grabbed only from worldmodel
	//
	if (name[0] == '*')
	{
		i = atoi(name + 1);
	//	if (i < 1 || !r_worldmodel || i >= r_worldmodel->numInlineModels) // FIXME@Q3BSP
	//		ri.Error(ERR_DROP, "%s: bad inline model number %i.\n", __FUNCTION__, i);
		return &r_inlineModels[i];
	}

	//
	// search the currently loaded models
	//
	for (i = 0, mod = r_models; i < r_models_count; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (!strcmp(mod->name, name))
			return mod;
	}

	//
	// find a free model slot spot
	//
	for (i = 0, mod = r_models; i < r_models_count; i++, mod++)
	{
		if (!mod->name[0])
			break;	// free spot
	}

	if (i == r_models_count)
	{
		if (r_models_count == RD_MAX_MODELS)
			ri.Error(ERR_DROP, "%s: hit limit of %d models.\n", __FUNCTION__, RD_MAX_MODELS);
		r_models_count++;
	}

	strcpy(mod->name, name);
	mod->index = i;

	//
	// load the file
	//
	modelFileLength = ri.LoadFile(mod->name, &buf);
	if (!buf)
	{
		if (crash)
			ri.Error(ERR_DROP, "%s: %s not found.\n", __FUNCTION__, mod->name);
		else
			ri.Printf(PRINT_LOW, "%s: %s not found.\n", __FUNCTION__, mod->name);

		memset(mod->name, 0, sizeof(mod->name));
		return NULL;
	}

	pLoadModel = mod;

	//
	// call the apropriate loader
	//
	switch (LittleLong(*(unsigned*)buf))
	{
	case PMODEL_IDENT:
		pLoadModel->extradata = Hunk_Begin(RD_MAX_PMOD_HUNKSIZE, "Model (Renderer)");
		R_LoadNewModel(mod, buf);
		break;

	case MD3_IDENT: /* MD3 */
		pLoadModel->extradata = Hunk_Begin(RD_MAX_MD3_HUNKSIZE, "Alias Model (Renderer)");
		Mod_LoadAliasMD3(mod, buf);
		break;
	default:
			ri.Error(ERR_DROP, "R_ModelForName:%s is not a model", mod->name);
		break;
	}

	pLoadModel = NULL;
	ri.FreeFile(buf);

	return mod;
}


/*
================
R_FreeModel

frees a model_t
================
*/
void R_FreeModel(model_t* mod)
{
	for (int i = 0; i < MD3_MAX_SURFACES; i++)
	{
		if (mod->vb[i])
		{
			R_FreeVertexBuffer(mod->vb[i]);
			mod->vb[i] = NULL;
		}
	}

	if (mod->extradata)
	{
		Hunk_Free(mod->extradata);
	}

	memset(mod, 0, sizeof(*mod));
}

/*
================
R_FreeAllModels

Frees all the models
================
*/
void R_FreeAllModels()
{
	int		i;
	for (i = 0; i < r_models_count; i++)
	{
		if (r_models[i].extradatasize)
			R_FreeModel(&r_models[i]);
	}
}

/*
================
R_FreeUnusedModels

Frees models that are no longer needed
================
*/
static void R_FreeUnusedModels()
{
	int		i;
	model_t* mod;

	for (i = 0, mod = r_models; i < r_models_count; i++, mod++)
	{
		if (!mod->name[0])
			continue;

		if (mod->registration_sequence != registration_sequence)
		{
			// don't need this model anymore
			R_FreeModel(mod); 
		}
	}
}

//=============================================================================

static void R_TouchAliasModel(model_t* mod)
{
	md3Header_t* md3Header;
	md3Surface_t* surf;
	md3Shader_t* shader;

	int	i, j, nt = 0;

	if (!mod || !mod->alias)
	{
		return;
	}

	md3Header = mod->alias;
	mod->numframes = md3Header->numFrames;

	surf = (md3Surface_t*)((byte*)md3Header + md3Header->ofsSurfaces);
	for (i = 0; i < md3Header->numSurfaces; i++)
	{
		shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
		for (j = 0; j < surf->numShaders; j++, shader++)
		{
			mod->images[nt] = R_FindTexture(shader->name, it_model, true);

			if (mod->images[nt] == NULL)
				mod->images[nt] = r_texture_missing;

			// mark surfaces with transparent textures
			if (mod->images[nt]->has_alpha)
				surf->flags = MSF_TRANSPARENT;

			shader->shaderIndex = mod->images[nt]->texnum;
			nt++;
		}
		surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
	}
}

static void R_TouchNewModel(model_t* mod)
{
	if (!mod || !mod->newmod)
	{
		return;
	}
	Mod_LoadNewModelTextures(mod);
}


/*
================
R_RegisterModel

Loads a model and associated textures
bumps the registration_sequence for model and textures they use
================
*/
struct model_s* R_RegisterModel(const char* name)
{
	model_t* mod;

	mod = R_ModelForName(name, false);
	if (!mod || mod == NULL)
		return mod;

	mod->registration_sequence = registration_sequence;

	switch (mod->type)
	{
	case MOD_BRUSH:
		// intentionaly empty
		break;

	case MOD_ALIAS:
		R_TouchAliasModel(mod);
		break;

	case MOD_NEWFORMAT:
		R_TouchNewModel(mod);
		break;

	default:
		ri.Error(ERR_DROP, __FUNCTION__": Model %s has bad type %i\n", mod->name, mod->type);
		break;
	}

	return mod;
}

/*
================
R_BeginRegistration
Loads the world BSP model and bumps registration sequence so the (old) unused assets can be freed after registration is complete.
================
*/
void R_BeginRegistration(const char *worldName)
{
	registration_sequence ++;

	R_LoadWorld(worldName);

	// load the default model
	r_defaultmodel = R_ModelForName("models/dev/xyz.md3", true);
	r_defaultmodel->registration_sequence = registration_sequence;
}

/*
================
R_EndRegistration
Frees all assets that don't have matching registration_sequence/
================
*/
void R_EndRegistration(void)
{
	R_FreeUnusedModels();
	R_FreeUnusedTextures();
}

/*
================
Cmd_modellist_f
================
*/
void Cmd_modellist_f(void)
{
	int		i;
	model_t* mod;
	int		total;

	static char *mods[] = { "BAD", "BSP", "ALIAS", "MESH"};

	total = 0;
	ri.Printf(PRINT_ALL, "Loaded models:\n");
	for (i = 0, mod = r_models; i < r_models_count; i++, mod++)
	{
		if (!mod->name[0])
			continue;

		if (mod->type == MOD_BRUSH)
			ri.Printf(PRINT_ALL, "%i: %s '%s' [%d kb]\n", i, mods[mod->type], mod->name, mod->extradatasize/1024);
		else if (mod->type == MOD_NEWFORMAT)
			ri.Printf(PRINT_ALL, "%i: %s '%s' [%d kb]\n", i, mods[mod->type], mod->name, mod->extradatasize / 1024);
		else
			ri.Printf(PRINT_ALL, "%i: %s '%s' [%d frames, %d kb]\n", i, mods[mod->type], mod->name, mod->numframes, mod->extradatasize/1024);
		total += mod->extradatasize;
	}
	ri.Printf(PRINT_ALL, "\nTotal resident: %i kb\n", total / 1024);
	ri.Printf(PRINT_ALL, "Total %i out of %i models in use\n", i, RD_MAX_MODELS);
}

/*
===============
R_InitModels
===============
*/
void R_InitModels()
{
	ri.AddCommand("modellist", Cmd_modellist_f);
}