/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "fx_local.h"

fxsysstate_t fxSys;

static cvar_t* fx_show;


static int FX_GetNumLoadedEffects() // FIXME
{
	return fxSys.numLoadedEffects;
}

static int FX_GetNumTags() // FIXME
{
	return 1;
}


// void CL_NewDynamicLight(int key, float x, float y, float z, float radius, float time)

/*
=================
CL_FindEffect
=================
*/
int CL_FindEffect(char* name)
{
	for (int i = 0; i < FX_GetNumLoadedEffects(); i++)
	{
		if (!fxSys.fxDefs[i] || !fxSys.fxDefs[i]->name[0])
			continue;

		if (!Q_stricmp(name, fxSys.fxDefs[i]->name))
			return i;
	}
	return -1;
}


static void Cmd_LoadFX_f(void)
{
	char* name = Cmd_Argv(1);
	if (Cmd_Argc() != 2)
	{
		Com_Printf("loadfx <filename> : load effect (fx/<filename>.fx)\n");
		return;
	}

	FX_LoadFromFile(Cmd_Argv(1));
}

static void Cmd_PlayFX_f(void)
{
	char* name = Cmd_Argv(1);
	if (Cmd_Argc() != 2)
	{
		Com_Printf("playfx <filename> x y z: play effect at xyz\n");
		return;
	}
}

/*
=================
FX_ClearParticles
=================
*/
void FX_ClearParticles()
{
	int		i;

	memset(fxSys.particles, 0, sizeof(fxSys.particles));
	fxSys.free_particles = &fxSys.particles[0];
	fxSys.active_particles = NULL;

	for (i = 0; i < FX_MAX_PARTICLES; i++)
	{
		fxSys.particles[i].next = &fxSys.particles[i + 1];
	}

	fxSys.particles[FX_MAX_PARTICLES - 1].next = NULL;
}





static void FX_FreeEffects()
{
	Z_FreeTags(TAG_FX);
	memset(&fxSys.fxDefs, 0, sizeof(fxSys.fxDefs));
	fxSys.numLoadedEffects = 0;

	for (int i = 0; i < FX_MAX_ACTIVE_EFFECTS; i++)
	{

	}
}

/*
=================
CL_InitEffects
=================
*/
void CL_InitEffects()
{
	fx_show  = Cvar_Get("fx_show", "1", CVAR_CHEAT);

	FX_ClearParticles();

	if (fxSys.numLoadedEffects > 0)
	{
		Z_FreeTags(TAG_FX);
	}

	memset(&fxSys, 0, sizeof(fxSys));

	Com_Printf("... max particles  : %i\n", FX_MAX_PARTICLES);
	Com_Printf("... max fx runners : %i\n", FX_MAX_ACTIVE_EFFECTS);
	Com_Printf("... max effects    : %i\n", FX_MAX_EFFECTS);

	Cmd_AddCommand("loadfx", Cmd_LoadFX_f);
	Cmd_AddCommand("playfx", Cmd_PlayFX_f);

	Com_Printf("Initialized FX system.\n");
}	


/*
=================
CL_ShutdownEffects
=================
*/
void CL_ShutdownEffects()
{
	FX_FreeEffects();

	Cmd_RemoveCommand("loadfx");
	Cmd_RemoveCommand("removefx");

}
