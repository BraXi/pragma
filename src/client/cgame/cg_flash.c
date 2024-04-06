/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cg_muzzleflash.c - muzzleflash effect

#include "../client.h"
#include "cg_local.h"

void PositionRotatedEntityOnTag(rentity_t* entity, rentity_t* parent, int parentModel, int tagIndex);

static muzzleflash_t cl_muzzleflashes[FX_WEAPON_MUZZLEFLASHES] =
{
		//light radius,	light color,		scale,	volume,	sound
		{300,			{1, 0.9, 0.7},		1.4f,	0.7f,	"weapons/deagle/shot.wav"}, 	// FX_MUZZLEFLASH_PISTOL
		{360,			{1, 0.9, 0.8},		1.0f,	0.6f,	"weapons/ak47/shot.wav"},	// FX_MUZZLEFLASH_RIFLE
		{200,			{1, 1, 0.7},		1.0f,	1.0f,	NULL}	// FX_MUZZLEFLASH_SHOTGUN
};

/*
==============
CG_ParseMuzzleFlashMessage

svc_muzzleflash [entnum] [muzzleflashfx]
==============
*/
void CG_ParseMuzzleFlashMessage(void)
{
	vec3_t		v_fwd, v_right, v_up;
	cdlight_t* dlight;
	int			entity_num, effectNum;
	clentity_t* cent;

	entity_num = MSG_ReadShort(&net_message);
	if (entity_num < 1 || entity_num >= MAX_GENTITIES)
		Com_Error(ERR_DROP, "CL_ParseMuzzleFlash: bad entity");

	cent = &cl_entities[entity_num];
	effectNum = MSG_ReadByte(&net_message);

	dlight = CG_AllocDynamicLight(entity_num);
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
			S_StartSound(NULL, entity_num, CHAN_WEAPON, S_RegisterSound(mz->sound), mz->volume, ATTN_NORM, 0); // todo: precache sound
	}
}

extern cvar_t* cl_hand;
extern cvar_t* cl_testlights;
/*
==============
CG_AddViewMuzzleFlash

For now we have no model for it, and ignore cl_hand
==============
*/
void CG_AddViewFlashLight(rentity_t* refent, player_state_t* ps)
{
	rentity_t ent;
//	vec3_t out_angles;
	int tag;

	if ( !refent->model || !ps->stats[STAT_HEALTH] || cl_testlights->value)
		return;

	tag = re.TagIndexForName(refent->model, "tag_flash");
	if (tag == -1)
		return;

	memset(&ent, 0, sizeof(ent));

	AxisClear(ent.axis);
	PositionRotatedEntityOnTag(&ent, refent, ps->viewmodel_index, tag);
//	VectorAngles_Fixed(ent.axis[0], out_angles);


	V_AddSpotLight(ent.origin, ent.axis[0], 900, -0.95, 1.0f, 0.85f, 0.7f);
}

/*
==============
CG_AddViewMuzzleFlash
==============
*/
void CG_AddViewMuzzleFlash(rentity_t* refent, player_state_t* ps)
{
	rentity_t ent;
	vec3_t out_angles;
	int tag;

	if (!refent->model || cl.muzzleflash >= FX_WEAPON_MUZZLEFLASHES)
		return;

	if (cl.time > cl.muzzleflash_time)
		return;

	tag = re.TagIndexForName(refent->model, "tag_flash");
	if (tag == -1)
		return;

	memset(&ent, 0, sizeof(ent));
	AxisClear(ent.axis);
	PositionRotatedEntityOnTag(&ent, refent, ps->viewmodel_index, tag);
	VectorAngles_Fixed(ent.axis[0], out_angles);

	VectorCopy(out_angles, ent.angles);

	ent.model = cgMedia.mod_v_muzzleflash;
	ent.frame = ent.oldframe = cl.muzzleflash_frame;
	ent.renderfx = RF_FULLBRIGHT | RF_DEPTHHACK | RF_VIEW_MODEL | RF_TRANSLUCENT | RF_SCALE;
	ent.alpha = 0.7; // todo: fix the goddamn model
	ent.scale = cl_muzzleflashes[cl.muzzleflash].flashscale;

	V_AddEntity(&ent);
}
