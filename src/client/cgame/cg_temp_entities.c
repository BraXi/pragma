/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cl_tent.c -- client side temporary entities

#include "../client.h"
#include "cg_local.h"

void CG_PartFX_Explosion(vec3_t org);

typedef void (*te_cmd) (void);
typedef struct
{
	int		tev;
	te_cmd	func;

}tempEntityDef_t;

static unsigned int TE_Type;
static vec3_t pos, dir, color;

/*
=================
TempEnt_Flame
TE_FLAME
=================
*/
static void TempEnt_Flame(void)
{
	MSG_ReadPos(&net_message, pos);
	CG_PartFX_Flame(NULL, pos);
}

/*
=================
TempEnt_Blood
TE_BLOOD
=================
*/
static void TempEnt_Blood(void) 
{
	static vec3_t blood_rgb = { 0.607, 0.121, 0.000 };

	MSG_ReadPos(&net_message, pos);
	MSG_ReadDir(&net_message, dir);
	CG_PartFX_Generic(pos, dir, blood_rgb, 60);
}

/*
=================
TempEnt_GunShotOrBulletSparks
TE_GUNSHOT & TE_BULLET_SPARKS
particles + sound
=================
*/
static void TempEnt_GunShotOrBulletSparks(void)
{
	int r = rand() & 10;

	MSG_ReadPos(&net_message, pos);
	MSG_ReadDir(&net_message, dir);

	if (TE_Type == TE_GUNSHOT)
	{
		VectorSet(color, 0.6, 0.5, 0.5);
		CG_PartFX_Generic(pos, dir, color, 40);
	}
	else
	{
		VectorSet(color, 1.000000, 0.670588, 0.027451);
		CG_PartFX_Generic(pos, dir, color, 6);
	}

	// impact sound
	if (r < 3)
		S_StartSound(pos, 0, 0, cgMedia.sfx_ricochet[r], 1, ATTN_NORM, 0);
}



/*
=================
TempEnt_Splash
TE_SPLASH
particles + sound
=================
*/
static void TempEnt_Splash(void)
{
	int c, cnt;
	static vec3_t splash_color[] =
	{
		{0.0, 0.0, 0.0},					// SPLASH_UNKNOWN
		{1.000000, 0.670588, 0.027451},		// SPLASH_SPARKS
		{0.466667, 0.482353, 0.811765},		// SPLASH_BLUE_WATER
		{0.482353, 0.372549, 0.294118},		// SPLASH_BROWN_WATER
		{0.000000, 1.000000, 0.000000},		// SPLASH_SLIME
		{1.000000, 0.670588,0.027451},		// SPLASH_LAVA
		{0.607843, 0.121569, 0.000000}		// SPLASH_BLOOD
	};

	cnt = MSG_ReadByte(&net_message); // count
	MSG_ReadPos(&net_message, pos); // pos
	MSG_ReadDir(&net_message, dir); // dir
	c = MSG_ReadByte(&net_message); // color

	if (c > 6)
		VectorCopy(splash_color[SPLASH_UNKNOWN], color);
	else
		VectorCopy(splash_color[c], color);

	CG_PartFX_Generic(pos, dir, color, cnt);

#if 0
	if (c == SPLASH_SPARKS)
	{
		S_StartSound(pos, 0, 0, cgMedia.sfx_splash[rand() & 3], 1, ATTN_NORM, 0);
	}
#endif
}


/*
=================
TempEnt_Explosion
TE_EXPLOSION

light + particles + sound
=================
*/

static void TempEnt_Explosion(void)
{
	cdlight_t* dlight;

	MSG_ReadPos(&net_message, pos);

	CG_PartFX_Explosion(pos);

	dlight = CG_AllocDynamicLight(0);

	VectorCopy(dlight->origin, pos);
	dlight->radius = 200 + (rand() % 75);
	dlight->decay = 16;
	dlight->die = cl.time + 1100;
	VectorSet(dlight->color, 1.0f, 0.67f, 0.027f);
//	CL_NewDynamicLight(0, pos[0], pos[1], pos[2] + 8, r, 1000);

	S_StartSound(pos, 0, 0, cgMedia.sfx_explosion[rand() & 3], 1, ATTN_NORM, 0);
}
 
/*	TE_FLAME,
	TE_GUNSHOT,
	TE_BULLET_SPARKS,
	TE_BLOOD,
	TE_EXPLOSION,
	TE_SPLASH,
	TE_COUNT
*/
tempEntityDef_t tempEntityDefs[] = // must match temp_event_t
{
	{TE_FLAME, TempEnt_Flame},
	{TE_GUNSHOT, TempEnt_GunShotOrBulletSparks},
	{TE_BULLET_SPARKS, TempEnt_GunShotOrBulletSparks},
	{TE_BLOOD, TempEnt_Blood},
	{TE_EXPLOSION, TempEnt_Splash},
	{TE_SPLASH, TempEnt_Splash}
};


/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
}



/*
=================
CG_ParseTempEntityCommand
=================
*/
void CG_ParseTempEntityCommand (void)
{
	TE_Type = MSG_ReadByte (&net_message);

	if (TE_Type > TE_COUNT)
	{
		Com_Error(ERR_DROP, "CG_ParseTempEntityCommand: bad temp entity %i\n", TE_Type);
		return;
	}
	tempEntityDefs[TE_Type].func();
}

/*
=================
CG_AddTempEntities
=================
*/
void CG_AddTempEntities (void)
{
}
