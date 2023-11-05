/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

#include "cg_local.h"

/*
==============
CL_ParseMuzzleFlash
==============
*/

#if 0
void CL_ParseMuzzleFlash(void)
{
	vec3_t		fv, rv;
	cdlight_t* dl;
	int			i, weapon;
	ccentity_t* pl;
	int			silenced;
	float		volume;
	char		soundname[64];

	i = MSG_ReadShort(&net_message);
	if (i < 1 || i >= MAX_GENTITIES)
		Com_Error(ERR_DROP, "CL_ParseMuzzleFlash: bad entity");

	weapon = MSG_ReadByte(&net_message);
	silenced = weapon & MZ_SILENCED;
	weapon &= ~MZ_SILENCED;

	pl = &cl_entities[i];

	dl = CL_AllocDlight(i);
	VectorCopy(pl->current.origin, dl->origin);
	AngleVectors(pl->current.angles, fv, rv, NULL);
	VectorMA(dl->origin, 18, fv, dl->origin);
	VectorMA(dl->origin, 16, rv, dl->origin);
	if (silenced)
		dl->radius = 100 + (rand() & 31);
	else
		dl->radius = 200 + (rand() & 31);
	dl->minlight = 32;
	dl->die = cl.time; // + 0.1;

	if (silenced)
		volume = 0.2;
	else
		volume = 1;

	switch (weapon)
	{
	case MZ_BLASTER:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_BLUEHYPERBLASTER:
		dl->color[0] = 0; dl->color[1] = 0; dl->color[2] = 1;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_HYPERBLASTER:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_MACHINEGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case MZ_SHOTGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_SSHOTGUN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_CHAINGUN1:
		dl->radius = 200 + (rand() & 31);
		dl->color[0] = 1; dl->color[1] = 0.25; dl->color[2] = 0;
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case MZ_CHAINGUN2:
		dl->radius = 225 + (rand() & 31);
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0;
		dl->die = cl.time + 0.1;	// long delay
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.05);
		break;
	case MZ_CHAINGUN3:
		dl->radius = 250 + (rand() & 31);
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 0.1;	// long delay
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.033);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.066);
		break;
	case MZ_RAILGUN:
		dl->color[0] = 0.5; dl->color[1] = 0.5; dl->color[2] = 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_ROCKET:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0.2;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_GRENADE:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound(NULL, i, CHAN_AUTO, S_RegisterSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_BFG:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
		break;

	case MZ_LOGIN:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
	case MZ_LOGOUT:
		dl->color[0] = 1; dl->color[1] = 0; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
	case MZ_RESPAWN:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect(pl->current.origin, weapon);
		break;
		// RAFAEL
	case MZ_PHALANX:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0.5;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
		break;
		// RAFAEL
	case MZ_IONRIPPER:
		dl->color[0] = 1; dl->color[1] = 0.5; dl->color[2] = 0.5;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/rippfire.wav"), volume, ATTN_NORM, 0);
		break;

		// ======================
		// PGM
	case MZ_ETF_RIFLE:
		dl->color[0] = 0.9; dl->color[1] = 0.7; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_SHOTGUN2:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_HEATBEAM:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 100;
		//		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/bfg__l1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_BLASTER2:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 0;
		// FIXME - different sound for blaster2 ??
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_TRACKER:
		// negative flashes handled the same in gl/soft until CL_AddDLights
		dl->color[0] = -1; dl->color[1] = -1; dl->color[2] = -1;
		S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_NUKE1:
		dl->color[0] = 1; dl->color[1] = 0; dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case MZ_NUKE2:
		dl->color[0] = 1; dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case MZ_NUKE4:
		dl->color[0] = 0; dl->color[1] = 0; dl->color[2] = 1;
		dl->die = cl.time + 100;
		break;
	case MZ_NUKE8:
		dl->color[0] = 0; dl->color[1] = 1; dl->color[2] = 1;
		dl->die = cl.time + 100;
		break;
		// PGM
		// ======================
	}
}

#endif