/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_download.c 

#include "client.h"

extern	cvar_t* allow_download;
extern	cvar_t* allow_download_models;
extern	cvar_t* allow_download_sounds;
extern	cvar_t* allow_download_maps;

int precache_check; // for autodownload of precache items
int precache_spawncount;
int precache_tex;
int precache_model_skin;

byte* precache_model; // used for skin checking in alias models

//#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
//#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
//#define TEXTURE_CNT (ENV_CNT+7) // BRAXI -- 7 was 13 (removed pcx)


static qboolean CL_DownloadFiles(float allowDownload, int assetType, int maxAssetCount, char* fileDir, char* fileExt, int nextType)
{
	char fn[MAX_OSPATH];

	if (precache_check >= assetType && precache_check < assetType + maxAssetCount)
	{
		if (!allowDownload)
		{
			precache_check = nextType;
			return false;
		}

		if (precache_check == assetType)
			precache_check++; // zero is blank

		while (precache_check < assetType + maxAssetCount && cl.configstrings[precache_check][0])
		{
			if (cl.configstrings[precache_check][0] == '*' || cl.configstrings[precache_check][0] == '#')
			{
				precache_check++;
				continue;
			}
			Com_sprintf(fn, sizeof(fn), "%s/%s%s", fileDir, cl.configstrings[precache_check++], fileExt);
			Com_Printf("%s: %s\n", __FUNCTION__, fn); // braxi -- dev
			if (!CL_CheckOrDownloadFile(fn))
				return true; // started a download
		}
		precache_check = nextType;
	}
	return false;
}



static void CL_DownloadModelsWithSkins(float allowDownload, int nextType)
{
#if 0
	md2Header_t* pheader;

	if (precache_check >= CS_MODELS && precache_check < CS_MODELS + MAX_MODELS)
	{
		if (!allowDownload)
		{
			precache_check = nextType;
			return;
		}

		while (precache_check < CS_MODELS + MAX_MODELS && cl.configstrings[precache_check][0])
		{
			if (cl.configstrings[precache_check][0] == '*' || cl.configstrings[precache_check][0] == '#')
			{
				precache_check++;
				continue;
			}
			if (precache_model_skin == 0)
			{
				if (!CL_CheckOrDownloadFile(cl.configstrings[precache_check]))
				{
//					Com_Printf("%s: model %s\n", __FUNCTION__, cl.configstrings[precache_check]); // braxi -- dev
					precache_model_skin = 1;
					return; // started a download
				}
				precache_model_skin = 1;
			}

			// checking for skins in the model
			if (!precache_model)
			{
				FS_LoadFile(cl.configstrings[precache_check], (void**)&precache_model);
				if (!precache_model)
				{
					precache_model_skin = 0;
					precache_check++;
					continue; // couldn't load it
				}
				if (LittleLong(*(unsigned*)precache_model) != MD2_IDENT)
				{
					// not an alias model
					FS_FreeFile(precache_model);
					precache_model = 0;
					precache_model_skin = 0;
					precache_check++;
					continue;
				}
				pheader = (md2Header_t*)precache_model;
				if (LittleLong(pheader->version) != MD2_VERSION)
				{
					precache_check++;
					precache_model_skin = 0;
					continue; // couldn't load it
				}

				pheader = (md2Header_t*)precache_model;

				while (precache_model_skin - 1 < LittleLong(pheader->num_skins))
				{
					char* skinname = (char*)precache_model + LittleLong(pheader->ofs_skins) + (precache_model_skin - 1) * MD2_MAX_SKINNAME;

					Com_Printf("%s: %s - %s\n", __FUNCTION__, cl.configstrings[precache_check], skinname); // braxi -- dev
					if (!CL_CheckOrDownloadFile(skinname));
					{
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}
				if (precache_model)
				{
					FS_FreeFile(precache_model);
					precache_model = 0;
				}
				precache_model_skin = 0;
				precache_check++;
			}
		}
		precache_check = nextType;
	}
#endif
}

static void CL_DownloadSkyImages(float allowDownload, int nextType)
{
#if 0
	const char* env_suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };
	char fn[MAX_OSPATH];
	int n;

	if (precache_check > ENV_CNT && precache_check < nextType)
	{
		if (allowDownload)
		{
			while (precache_check < TEXTURE_CNT)
			{
				n = precache_check++ - ENV_CNT - 1;
				Com_sprintf(fn, sizeof(fn), "env/%s_%s.tga", cl.configstrings[CS_SKY], env_suf[n]);

				Com_Printf("%s: %s\n", __FUNCTION__, fn); // braxi -- dev
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = nextType;
	}
#endif
}

static void CL_DownloadMapTextures(float allowDownload, char* fileDir, char* fileExt, int nextType)
{
#if 0
	char fn[MAX_OSPATH];
	if (precache_check == TEXTURE_CNT)
	{
		precache_check = TEXTURE_CNT + 1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT + 1)
	{
		// from qcommon/cmodel.c
		extern int			numtexinfo;
		extern mapsurface_t	map_surfaces[];

		if (allowDownload)
		{
			while (precache_tex < numtexinfo)
			{
				sprintf(fn, "%s/%s.%s", fileDir, map_surfaces[precache_tex++].rname, fileExt);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT + 999;
	}
#endif
}



void CL_RequestNextDownload(void)
{
	unsigned	map_checksum;
	unsigned	cgprogs_checksum;
	char* mapFileName;

	if (cls.state != ca_connected)
		return;

#if 0
	if (!allow_download->value && precache_check < ENV_CNT)
		precache_check = ENV_CNT;

	mapFileName = cl.configstrings[CS_MODELS + 1];

	precache_check = ENV_CNT;

	// download map
	if (precache_check == CS_MODELS)
	{
		precache_check = CS_MODELS + 2; // 0 isn't used
		if (allow_download_maps->value)
			if (!CL_CheckOrDownloadFile(mapFileName))
				return; // started a download
	}

	// download all .md2 models and all of their skins
	CL_DownloadModelsWithSkins(allow_download_models->value, CS_SOUNDS);

	// download all sounds
	if (CL_DownloadFiles(allow_download_sounds->value, CS_SOUNDS, MAX_SOUNDS, "sounds", ""/*".wav"*/, CS_IMAGES))
		return;

	// download all images
	if (CL_DownloadFiles(allow_download->value, CS_IMAGES, MAX_IMAGES, "pics", ".pcx", ENV_CNT))
		return;

	// perform BSP checksum to detect cheater maps
	if (precache_check == ENV_CNT)
	{
		precache_check = ENV_CNT + 1;


	}
#endif

	mapFileName = cl.configstrings[CS_MODELS + 1];
	CM_LoadMap(mapFileName, true, &map_checksum);

	if (map_checksum != atoi(cl.configstrings[CS_CHECKSUM_MAP]))
	{
		Com_Error(ERR_DROP, "Local map version differs from server: %i != '%s'\n", map_checksum, cl.configstrings[CS_CHECKSUM_MAP]);
		return;
	}

	cgprogs_checksum = Scr_GetProgsCRC(VM_CLGAME);
	if ((int)cgprogs_checksum != atoi(cl.configstrings[CS_CHECKSUM_CGPROGS]))
	{
		Com_Error(ERR_DROP, "Client game differs from server: %i != '%s'\n", cgprogs_checksum, cl.configstrings[CS_CHECKSUM_CGPROGS]);
		return;
	}

//	CL_DownloadSkyImages((allow_download->value && allow_download_maps->value), TEXTURE_CNT);
//	CL_DownloadMapTextures((allow_download->value && allow_download_maps->value), "textures", "wal", TEXTURE_CNT + 999);

	CL_RegisterSounds();
	CL_PrepRefresh();

	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message, va("begin %i\n", precache_spawncount));

}


/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f(void)
{
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2) 
	{
		unsigned	map_checksum;		// for detecting cheater maps

		CM_LoadMap(cl.configstrings[CS_MODELS + 1], true, &map_checksum);
		CL_RegisterSounds();
		CL_PrepRefresh();
		return;
	}

	precache_check = CS_MODELS;
	precache_spawncount = atoi(Cmd_Argv(1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload();
}

//=============================================================================

void CL_DownloadFileName(char* dest, int destlen, char* fn)
{
// braxi -- removed player skins
//	if (strncmp(fn, "players", 7) == 0)
//		Com_sprintf(dest, destlen, "%s/%s", BASEDIRNAME, fn);
//	else
		Com_sprintf(dest, destlen, "%s/%s", FS_Gamedir(), fn);
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean	CL_CheckOrDownloadFile(char* filename)
{
	FILE* fp;
	char	name[MAX_OSPATH];

	if (strstr(filename, ".."))
	{
		Com_Printf("Refusing to download a path with ..\n");
		return true;
	}

	if (FS_LoadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		return true;
	}

	strcpy(cls.downloadname, filename);

	// download to a temp name, and only rename to the real name 
	// when done, so if interrupted a runt file wont be left
	COM_StripExtension(cls.downloadname, cls.downloadtempname);
	strcat(cls.downloadtempname, ".tmp");

	//ZOID
	// check to see if we already have a tmp for this file, if so, try to resume
	// open the file if not opened yet
	CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

	//	FS_CreatePath (name);

	fp = fopen(name, "r+b");
	if (fp) 
	{ // it exists
		int len;
		fseek(fp, 0, SEEK_END);
		len = ftell(fp);

		cls.download = fp;

		// give the server an offset to start the download
		Com_Printf("Resuming %s\n", cls.downloadname);
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, va("download %s %i", cls.downloadname, len));
	}
	else {
		Com_Printf("Downloading %s\n", cls.downloadname);
		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		MSG_WriteString(&cls.netchan.message, va("download %s", cls.downloadname));
	}

	cls.downloadnumber++;

	return false;
}

/*
===============
CL_Download_f

Request a download from the server
===============
*/
void	CL_Download_f(void)
{
	char filename[MAX_OSPATH];

	if (Cmd_Argc() != 2) 
	{
		Com_Printf("Usage: download <filename>\n");
		return;
	}

	Com_sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr(filename, ".."))
	{
		Com_Printf("Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile(filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf("File %s already exists.\n", filename);
		return;
	}

	strcpy(cls.downloadname, filename);
	Com_Printf("Downloading %s\n", cls.downloadname);

	// download to a temp name, and only rename to the real 
	// name when done, so if interrupted a runt file wont be left
	COM_StripExtension(cls.downloadname, cls.downloadtempname);
	strcat(cls.downloadtempname, ".tmp");

	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
	MSG_WriteString(&cls.netchan.message, va("download %s", cls.downloadname));

	cls.downloadnumber++;
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload(void)
{
	int		size, percent;
	char	name[MAX_OSPATH];
	int		r;

	// read the data
	size = MSG_ReadShort(&net_message);
	percent = MSG_ReadByte(&net_message);
	if (size == -1)
	{
		Com_Printf("Server does not have this file.\n");
		if (cls.download)
		{
			// if here, we tried to resume a file but the server said no
			fclose(cls.download);
			cls.download = NULL;
		}
		CL_RequestNextDownload();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

		FS_CreatePath(name);

		cls.download = fopen(name, "wb");
		if (!cls.download)
		{
			net_message.readcount += size;
			Com_Printf("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload();
			return;
		}
	}

	fwrite(net_message.data + net_message.readcount, 1, size, cls.download);
	net_message.readcount += size;

	if (percent != 100)
	{
		// request next block
		cls.downloadpercent = percent;

		MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
		SZ_Print(&cls.netchan.message, "nextdl");
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

		//		Com_Printf ("100%%\n");

		fclose(cls.download);

		// rename the temp file to it's final name
		CL_DownloadFileName(oldn, sizeof(oldn), cls.downloadtempname);
		CL_DownloadFileName(newn, sizeof(newn), cls.downloadname);
		r = rename(oldn, newn);
		if (r)
			Com_Printf("failed to rename.\n");

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed

		CL_RequestNextDownload();
	}
}