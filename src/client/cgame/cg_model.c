/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"

orientation_t out;
#if 0
int CG_GetTag(clentity_t* ent, int modelSlot, char *tagname)
{
	return 0;
}

void CG_GetTagOrigin(clentity_t *ent, int tagIndex, vec3_t outOrigin)
{
	char* tagName;
	orientation_t* tag;


	if (!model || model->type != MOD_MD3 || !model->numTags)
	{
		//	Scr_RunError(Scr_GetString(ent->v.model));
		Scr_ReturnVector(ent->v.origin);
		return;
	}

	tag = SV_PositionTagOnEntity(ent, tagName);
	if (!tag)
	{
		Scr_RunError("gettagorigin(): model '%s' nas no tag '%s'\n", model->name, tagName);
		return;
	}

	Scr_ReturnVector(tag->origin);
}



/*
=================
SV_GetTag

returns orientation_t of a tag for a given frame or NULL if not found
=================
*/
orientation_t* SV_GetTag(int modelindex, int frame, char* tagName)
{
	svmodel_t* mod;
	orientation_t* tagdata;
	int index;

	mod = SV_ModelForNum(modelindex);
	if (!mod || mod->type != MOD_MD3)
	{
		Com_Error(ERR_DROP, "SV_GetTag: wrong model for index %i\n", modelindex);
		return NULL;
	}

	// it is possible to have a bad frame while changing models, so don't error
	if (frame >= mod->numFrames)
		frame = mod->numFrames - 1;
	else if (frame < 0)
		frame = 0;

	tagdata = (orientation_t*)((byte*)mod->tagFrames) + (frame * mod->numTags);
	for (index = 0; index < mod->numTags; index++, tagdata++)
	{
		if (!strcmp(mod->tagNames[index], tagName))
		{
			return tagdata; // found it
		}
	}
	return NULL;
}

static void AngleVectors2(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float        angle;
	float        sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI * 2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
	}
	if (up)
	{
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
		up[2] = cr * cp;
	}
}

/*
=================
SV_PositionTag

Returns the tag rotation and origin accounted by given origin and angles
=================
*/

orientation_t* CG_PositionTag(vec3_t origin, vec3_t angles, int modelindex, int animframe, float lerpfrac, int tagIndex)
{
	orientation_t* tag;
	orientation_t    parent;
	vec3_t			tempAxis[3];

	tag = CG_GetTag(modelindex, animframe, lerpfrac, tagIndex);
	if (!tag)
		return NULL;

	AxisClear(parent.axis);
	VectorCopy(origin, parent.origin);
	AnglesToAxis(angles, parent.axis);

	AxisClear(out.axis);
	VectorCopy(parent.origin, out.origin);

	AxisClear(tempAxis);
	for (int i = 0; i < 3; i++)
	{
		VectorMA(out.origin, tag->origin[i], parent.axis[i], out.origin);
	}

	// translate rotation and origin
	MatrixMultiply(out.axis, parent.axis, tempAxis);
	MatrixMultiply(tag->axis, tempAxis, out.axis);
	return &out;
}


/*
=================
CG_PositionTagOnEntity
=================
*/
orientation_t* CG_PositionTagOnEntity(clentity_t* ent, int modelindex, int tagIndex)
{
	orientation_t* tag;
	tag = CG_PositionTag(ent->current.origin, ent->current.angles, modelindex, ent->current.frame, tagIndex);
	return tag;
}
#endif