/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"

extern void CL_LogoutEffect(vec3_t org, int type);
extern void CL_ItemRespawnParticles(vec3_t org);

/*
==============================================================
MUZZLE FLASHES
==============================================================
*/

muzzleflash_t cl_muzzleflashes[FX_WEAPON_MUZZLEFLASHES] =
{
	//	forward,	right,	up,		light radius,	light color,		scale,	volume,	sound
		{44.0f,		-4.8f,	-3.5f,	200,			{1, 0.9, 0.7},		1.4f,	1.0f,	"weapons/deagle/shot.wav"}, 	// FX_MUZZLEFLASH_PISTOL
		{66.0f,		-4.8f,	-3.5f,	260,			{1, 1, 0.7},		1.0f,	1.0f,	"weapons/ak47/shot.wav"},	// FX_MUZZLEFLASH_RIFLE
		{66.0f,		-4.8f,	-3.5f,	200,			{1, 1, 0.7},		1.0f,	1.0f,	NULL}	// FX_MUZZLEFLASH_SHOTGUN
};


/*
==============
CL_ParseMuzzleFlash

svc_muzzleflash [entnum] [muzzleflashfx]
==============
*/
void CL_ParseMuzzleFlash(void)
{
	vec3_t		v_fwd, v_right, v_up;
	cdlight_t* dlight;
	int			entity_num, effectNum;
	ccentity_t* cent;

	entity_num = MSG_ReadShort(&net_message);
	if (entity_num < 1 || entity_num >= MAX_GENTITIES)
		Com_Error(ERR_DROP, "CL_ParseMuzzleFlash: bad entity");

	cent = &cl_entities[entity_num];
	effectNum = MSG_ReadByte(&net_message);

	dlight = CL_AllocDynamicLight(entity_num);
	VectorCopy(cent->current.origin, dlight->origin);

	if (cl.playernum + 1 == entity_num) // this is our local player
	{
		cl.muzzleflash_time = cl.time + 90;

		cl.muzzleflash = effectNum;
		if (effectNum >= FX_WEAPON_MUZZLEFLASHES || effectNum < 0)
			cl.muzzleflash = FX_MUZZLEFLASH_PISTOL;

		cl.muzzleflash_frame++;
		if (cl.muzzleflash_frame > 9)
			cl.muzzleflash_frame = 0;
	}


	AngleVectors(cent->current.angles, v_fwd, v_right, v_up);
	VectorMA(dlight->origin, 12, v_fwd, dlight->origin);
	VectorMA(dlight->origin, 16, v_right, dlight->origin);

	if (effectNum >= 0 && effectNum < FX_WEAPON_MUZZLEFLASHES)
	{
		muzzleflash_t* mz = &cl_muzzleflashes[effectNum];

		dlight->minlight = 32;
		dlight->die = cl.time + 140;

		VectorCopy(mz->dlight_color, dlight->color);
		dlight->radius = mz->dlight_radius + (rand() & 31);

		if (mz->sound != NULL)
			S_StartSound(NULL, entity_num, CHAN_WEAPON, S_RegisterSound(mz->sound), mz->volume, ATTN_NORM, 0); // should precache sound
	}
	else
	{
		dlight->minlight = 32;
		dlight->die = cl.time;

		switch (effectNum)
		{
		case FX_MUZZLEFLASH_LOGIN:

			dlight->die = cl.time + 1.0;
			S_StartSound(NULL, entity_num, CHAN_WEAPON, S_RegisterSound("effects/login.wav"), 1, ATTN_NORM, 0);
			CL_LogoutEffect(cent->current.origin, effectNum);
			break;
		case FX_MUZZLEFLASH_LOGOUT:
			VectorSet(dlight->color, 1, 0, 0);
			dlight->die = cl.time + 1.0;
			S_StartSound(NULL, entity_num, CHAN_WEAPON, S_RegisterSound("effects/logout.wav"), 1, ATTN_NORM, 0);
			CL_LogoutEffect(cent->current.origin, effectNum);
			break;
		case FX_MUZZLEFLASH_RESPAWN:
			VectorSet(dlight->color, 1, 1, 0);
			dlight->die = cl.time + 1.0;
			S_StartSound(NULL, entity_num, CHAN_WEAPON, S_RegisterSound("effects/respawn.wav"), 1, ATTN_NORM, 0);
			CL_LogoutEffect(cent->current.origin, effectNum);
			break;
		default:
			break;
		}
	}
}


void CL_AddMuzzleFlash()
{

}