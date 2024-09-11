/*
prtool, part of pragma
Copyright (C) 2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "prtool.h"

typedef enum
{
	ASSET_RAWFILE, // anything
	ASSET_MODEL, // .md3, .smd, .mod, .anm + images
	ASSET_GUI, // .gui
	ASSET_SHADER, // .fp + .vp
	ASSET_BSP, // .bsp + textures
	ASSET_TEXTURE, // .tga
	ASSET_IMAGE, // .tga
	ASSET_FONT, // .bff
	NUM_ASSET_TYPES
} AssetType;

const char* assetTypeNames[] = { "rawfile", "model", "gui", "shader", "world", "texture", "image", "font" };

typedef struct xasset_s
{
	char name[56];
	AssetType type;
	bool isok;
	model_t* model;
} xasset_t;

static char pakname[MAXPATH]; // PAK to create
static dpackfile_t pakFiles[MAX_FILES_IN_PACK], *pPakFile;
static FILE* pakFileHandle;
static int numFilesInPak;
static unsigned int pakFilesSize;
static xasset_t assetList[MAX_FILES_IN_PACK];
static int numAssetsTotal = 0;
static int numAssetsByType[NUM_ASSET_TYPES];
static int numMissingFiles = 0;
static char missingFileNames[MAX_FILES_IN_PACK][56];

static xasset_t *AssetForName(const char* name, const AssetType type)
{
	int i;
	xasset_t *asset;

	// name cannot be too long
	if (strlen(name) >= (MAX_FILENAME_IN_PACK-1))
	{
		Com_Warning("Asset name \"%s\" is too long", name);
		return NULL;
	}

	// find asset in list
	for (i = 0; i < numAssetsTotal; i++)
	{
		asset = &assetList[i];
		if (asset->name[0] == 0)
			continue;

		if (!strcmp(asset->name, name))
			return asset;
	}

	// check limits
	if (numAssetsTotal >= MAX_FILES_IN_PACK)
	{
		Com_Error("Too many files");
		//return NULL;
	}

	// create new entry
	asset = &assetList[numAssetsTotal];
	memset(asset, 0, sizeof(xasset_s));

	strncpy(asset->name, name, sizeof(asset->name));
	asset->type = type;

	numAssetsTotal++;
	numAssetsByType[asset->type] ++;

	asset->isok = (bool)Com_FileExists("%s/%s", g_devdir, asset->name);
	if(!asset->isok)
	{
		Com_Warning("missing %s \"%s\".", assetTypeNames[asset->type], asset->name);

		strncpy(missingFileNames[numMissingFiles], asset->name, sizeof(missingFileNames[0]));
		numMissingFiles++;
	}

	return asset;
}


static char tempfilename[MAXPATH];

static void AddRawFile()
{
	xasset_t* asset = AssetForName(Com_GetArg(1), ASSET_RAWFILE);
}

static void AddModel()
{
	// adds model and its textures
	xasset_t* asset_mod, *asset_tex;

	asset_mod = AssetForName(Com_GetArg(1), ASSET_MODEL);
	if (!asset_mod)
		return;

	if (!asset_mod->model)
	{
		snprintf(tempfilename, sizeof(tempfilename), "%s/%s", g_devdir, Com_GetArg(1));
		asset_mod->model = LoadModel(Com_GetArg(1), tempfilename);
		if (asset_mod->model && asset_mod->model->type != MOD_BAD)
		{
			for (int i = 0; i < asset_mod->model->numTextures; i++)
			{
				asset_tex = AssetForName(asset_mod->model->textureNames[i], ASSET_IMAGE);
			}
		}
		else
		{
			asset_mod->isok = false;
		}
	}
}

static void AddGUI()
{
	snprintf(tempfilename, sizeof(tempfilename), "guis/%s.gui", Com_GetArg(1));
	xasset_t *asset = AssetForName(tempfilename, ASSET_GUI);
}

static void AddShader()
{
	xasset_t* asset1, *asset2;

	snprintf(tempfilename, sizeof(tempfilename), "shaders/%s.fp", Com_GetArg(1));
	asset1 = AssetForName(tempfilename, ASSET_SHADER);
	if (!asset1 || !asset1->isok)
		return;

	snprintf(tempfilename, sizeof(tempfilename), "shaders/%s.vp", Com_GetArg(1));
	asset2 = AssetForName(tempfilename, ASSET_SHADER);
	if (!asset2 || !asset2->isok)
	{
		asset1->isok = false; // invalidate FB because were missing VP anyway
		return;
	}
}

static bool AddTexturesUsedInBSP(const char* filename)
{
	dbsp_header_t header;
	FILE* f;
	lump_t* lump;
	dbsp_texinfo_t* pTexInfos;
	int filelen, i, numTexInfos;

	char texturename[MAX_QPATH];

	if (strlen(filename) >= (MAX_QPATH - 1))
	{
		Com_Warning("BSP %s has too long name.", filename);
		return false;
	}

	f = Com_OpenReadFile(filename, false);
	if (!f)
	{
		Com_Warning("BSP %s is missing.", filename);
		return false;
	}

	filelen = Com_FileLength(f);
	if (filelen < sizeof(dbsp_header_t))
	{
		fclose(f);
		Com_Warning("BSP %s is too small in size.", filename);
		return false;
	}

	Com_SafeRead(f, &header, sizeof(dbsp_header_t));
	//fread(&header, sizeof(dbsp_header_t), 1, f);

	header.ident = Com_EndianLong(header.ident);
	if (header.ident != BSP_IDENT && header.ident != QBISM_IDENT)
	{
		fclose(f);
		Com_Warning("%s is not a BSP (and it's not QBISM BSP either).");
		return false;
	}

	header.version = Com_EndianLong(header.version);
	if (header.version != BSP_VERSION)
	{
		fclose(f);
		Com_Warning("BSP %s has wrong version %i (should be %i).", filename, header.version, BSP_VERSION);
		return false;
	}

	//for (i = 0, lump = &header.lumps[HEADER_LUMPS]; i < HEADER_LUMPS; i++, lump++)
	//{
		lump = &header.lumps[LUMP_TEXINFO];
		lump->filelen = Com_EndianLong(lump->filelen);
		lump->fileofs = Com_EndianLong(lump->fileofs);
	//}

	//lump = &header.lumps[LUMP_TEXINFO];
	if (lump->filelen % sizeof(dbsp_texinfo_t))
	{
		fclose(f);
		Com_Warning("BSP %s has weird TEXINFO lump size", filename);
		return false;
	}

	numTexInfos = (lump->filelen / sizeof(dbsp_texinfo_t));

	if (numTexInfos <= 0)
	{
		fclose(f);
		Com_Warning("BSP %s has no TEXINFOs (which is really weird)", filename);
		return false;
	}

	pTexInfos = (dbsp_texinfo_t*)Com_SafeMalloc(sizeof(dbsp_texinfo_t) * numTexInfos, __FUNCTION__);

	fseek(f, lump->fileofs, SEEK_SET);
	Com_SafeRead(f, pTexInfos, sizeof(dbsp_texinfo_t) * numTexInfos);
	//fread(pTexInfos, sizeof(dbsp_texinfo_t), numTexInfos, f);

	xasset_t* asset;
	for (i = 0; i < numTexInfos; i++)
	{
		snprintf(texturename, sizeof(texturename), "textures/%s.tga", pTexInfos[i].texture);
		asset = AssetForName(texturename, ASSET_TEXTURE);
	}
	free(pTexInfos);
	fclose(f);
	return true;
}

static void AddBSP()
{
	xasset_t* asset_bsp;

	snprintf(tempfilename, sizeof(tempfilename), "%s/%s", g_devdir, Com_GetArg(1));
	asset_bsp = AssetForName(Com_GetArg(1), ASSET_BSP);
	//if (!asset_bsp || !asset_bsp->isok)
	//	return false;

	AddTexturesUsedInBSP(tempfilename);
}

static void AddTexture()
{
	snprintf(tempfilename, sizeof(tempfilename), "textures/%s.tga", Com_GetArg(1));
	xasset_t* asset = AssetForName(tempfilename, ASSET_TEXTURE);
}

static void AddImage()
{
	xasset_t* asset = AssetForName(Com_GetArg(1), ASSET_IMAGE);
}

static void AddFont()
{
	snprintf(tempfilename, sizeof(tempfilename), "fonts/%s.bff", Com_GetArg(1));
	xasset_t* asset = AssetForName(tempfilename, ASSET_FONT);
}

static void AddSkyTextures()
{
	xasset_t* sky[6];
	static const char* sky_tex_prefix[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
	for (int i = 0; i < 6; i++)
	{
		snprintf(tempfilename, sizeof(tempfilename), "textures/sky/%s_%s.tga", Com_GetArg(1), sky_tex_prefix[i]);
		sky[i] = AssetForName(tempfilename, ASSET_TEXTURE);
	}
}

static command_t commands[] =
{
	{"file", AddRawFile, 2},
	{"model", AddModel, 2},
	{"gui", AddGUI, 2},
	{"shader", AddShader, 2},
	{"world", AddBSP, 2},
	{"texture", AddTexture, 2},
	{"image", AddImage, 2},
	{"font", AddFont, 2},
	{"sky", AddSkyTextures, 2}
};
static const int numCommands = sizeof(commands) / sizeof(command_t);

/*
=================
Pak_ParseFileList
Parses file list and builds an asset list of files that will be copied over to a pak
=================
*/
int Pak_ParseFileList(const char* fileName)
{
	static char line[MAXLINELEN];
	FILE* f = NULL;
	int i, line_num = 0;
	char *key;
	int numEntries = 0;

	f = Com_OpenReadFile(fileName, true);

	if (!f)
		return 0;

	Com_Printf("Parsing asset list: %s ...\n\n", fileName);

	while (fgets(line, sizeof(line), f) != NULL)
	{
		line_num++;
		Com_Tokenize(line);

		if (Com_NumArgs() > 2)
		{
			Com_Warning("[line %i] more than two arguments, skipping entire line, whitespace by accident?", line_num);
			continue;
		}

		if (Com_NumArgs() != 2)
		{
			//Com_Warning("skipped line %i: %s", line_num, line);
			continue;
		}

		key = Com_GetArg(0);

		if (!key || key && key[0] == 0)
			continue;

		if (!strcmp("$pakname", key))
		{
			if(pakname[0] != 0)
				Com_Error("[line %i] redefinition of $pakname", line_num);

			strncpy(pakname, Com_GetArg(1), sizeof(pakname));
		}
		else if(key[0] == '$')
		{
			for (i = 0; i < numCommands; i++)
			{
				if (!strcmp(commands[i].name, key+1))
				{
					numEntries++;
					commands[i].pFunc();
					break;
				}
			}
		}
	}

	fclose(f);

	Com_Printf("\n%i entries totaling to %i assets.\n\n", numEntries, numAssetsTotal);
	return 1;
}


/*
=================
PackFile
Write a single file into a pak
=================
*/
static void PackFile(char* src, char* name)
{
	FILE *in;
	long fileLen;
	byte* buf = NULL;

	if (numFilesInPak >= MAX_FILES_IN_PACK)
		Com_Error("More than %i files in a pack", MAX_FILES_IN_PACK);

	in = Com_OpenReadFile(src, true);

	fileLen = Com_FileLength(in);
	pakFilesSize += fileLen;

	pPakFile->filepos = Com_EndianLong( ftell(pakFileHandle) );
	pPakFile->filelen = Com_EndianLong(fileLen);
	strncpy(pPakFile->name, name, sizeof(pPakFile->name));

	if(g_verbose)
		Com_Printf(" %56s : %10i bytes\n", pPakFile->name, fileLen);

	buf = (byte*)Com_SafeMalloc((size_t)(fileLen + (size_t)1), __FUNCTION__);

	Com_SafeRead(in, buf, fileLen);
	Com_SafeWrite(pakFileHandle, buf, fileLen);

	free(buf);
	fclose(in);

	numFilesInPak++;
	pPakFile++;
}

/*
=================
PackFile
Write files into a pak
=================
*/
static void Pak_WritePak()
{
	char destfile[MAXPATH];
	char srcfile[MAXPATH];
	dpackheader_t header;

	if (numAssetsTotal == 0)
	{
		Com_Error("Nothing to pack!");
	}

	sprintf(destfile, "%s/%s.pak", g_gamedir, pakname);
	pakFileHandle = Com_OpenWriteFile(destfile, true);
	Com_Printf("Writing package \"%s\" to %s ...\n", pakname, destfile);

	// write dummy header
	memset(&header, 0, sizeof(dpackheader_t));
	Com_SafeWrite(pakFileHandle, &header, sizeof(header));

	pPakFile = pakFiles;

	for (int type = ASSET_RAWFILE; type < NUM_ASSET_TYPES; type++)
	{
		if (numAssetsByType[type] <= 0)
			continue;

		//Com_Printf("Writing %i %s%s...\n", numAssetsByType[type], assetTypeNames[type], (numAssetsByType[type] > 1 ? "s" : ""));
		for (int idx = 0; idx < numAssetsTotal; idx++)
		{
			if (!assetList[idx].isok || assetList[idx].type != type)
				continue;

			sprintf(srcfile, "./%s/%s", g_devdir, assetList[idx].name);

			if (g_verbose)
				Com_Printf("%8s ", assetTypeNames[assetList[idx].type]);

			PackFile(srcfile, assetList[idx].name);
		}
	}

	// write real header
	int dirlen = (int)((byte*)pPakFile - (byte*)pakFiles);
	header.ident = Com_EndianLong(IDPAKHEADER);
	header.dirofs = Com_EndianLong(ftell(pakFileHandle));
	header.dirlen = Com_EndianLong(dirlen);

	// write directory tree
	Com_SafeWrite(pakFileHandle, pakFiles, header.dirlen);
	fseek(pakFileHandle, 0, SEEK_SET);
	Com_SafeWrite(pakFileHandle, &header, sizeof(header));
	fclose(pakFileHandle);


	Com_Printf("Done writing %s - %i files in total (%i KB).\n\n", destfile, numFilesInPak, (pakFilesSize / 1024));
	
	for (int i = 0; i < NUM_ASSET_TYPES; i++)
	{
		if (numAssetsByType[i] == 0)
			continue;

		Com_HappyPrintf("   %4i %s%s\n", numAssetsByType[i], assetTypeNames[i], (numAssetsByType[i] > 1 ? "s" : ""));
	}

	if (numMissingFiles > 0)
	{
		Com_Printf("\n****** %i %s missing %s ******\n", numMissingFiles, numMissingFiles == 1 ? "file is" : "files are", g_verbose == true ? "" : "(see warnings above)");

		if (g_verbose)
		{
			for (int i = 0; i < numMissingFiles; i++)
			{
				Com_Printf("   %s\n", missingFileNames[i]);
			}
		}
	}
}

void Opt_CreatePack()
{
	char fileName[MAXPATH];

	assert(sizeof(dpackfile_t) == 64);

	numFilesInPak = 0;
	numAssetsTotal = 0;
	pakFilesSize = 0;
	memset(&assetList, 0, sizeof(assetList));
	memset(pakname, 0, sizeof(pakname));
	memset(numAssetsByType, 0, sizeof(numAssetsByType));
	

	sprintf(fileName, "%s/%s/%s", g_devdir, PAKLIST_DIR, g_controlFileName);

	Pak_ParseFileList(fileName);
	Pak_WritePak();
}