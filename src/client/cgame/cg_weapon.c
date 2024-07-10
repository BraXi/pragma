/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"

void CG_AddViewMuzzleFlash(rentity_t* refent, player_state_t* ps);
void CG_AddViewFlashLight(rentity_t* refent, player_state_t* ps);

/*
==============
CG_AddViewWeapon
==============
*/
void CG_AddViewWeapon(player_state_t* ps, player_state_t* ops)
{
	rentity_t	viewmodel;
	int			i;

	// allow the gun to be completely removed but don't cull flashlight
	if (!cl_drawviewmodel->value)
	{
		CG_AddViewFlashLight(NULL, ps);
		return;
	}

	memset(&viewmodel, 0, sizeof(viewmodel));

	if (gun_model && CL_CheatsAllowed())
		viewmodel.model = gun_model; // development tool
	else
		viewmodel.model = cl.model_draw[ps->viewmodel[0]];

	if (!viewmodel.model)
		return;

	// set up position
	for (i = 0; i < 3; i++)
	{
		viewmodel.origin[i] = cl.refdef.view.origin[i] + ops->viewmodel_offset[i]
			+ cl.lerpfrac * (ps->viewmodel_offset[i] - ops->viewmodel_offset[i]);
		viewmodel.angles[i] = cl.refdef.view.angles[i] + LerpAngle(ops->viewmodel_angles[i],
			ps->viewmodel_angles[i], cl.lerpfrac);
	}

	if (gun_frame && CL_CheatsAllowed())
	{
		// development tool
		viewmodel.frame = gun_frame;
		viewmodel.oldframe = gun_frame;
		viewmodel.backlerp = 1.0 - cl.lerpfrac;
	}
	else
	{
		viewmodel.frame = ps->viewmodel_frame;

		// just changed weapons, don't lerp from oldframe and remove muzzleflash if present
		if (ps->viewmodel[0] != ops->viewmodel[0])
		{
			cl.muzzleflash_time = 0;
			viewmodel.oldframe = viewmodel.frame;
			viewmodel.backlerp = 1;
		}
		else
		{
			viewmodel.oldframe = ops->viewmodel_frame;
			viewmodel.backlerp = 1.0 - cl.lerpfrac;
		}
	}

	viewmodel.animbacklerp = viewmodel.backlerp;
	VectorCopy(viewmodel.origin, viewmodel.oldorigin);
	viewmodel.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_VIEW_MODEL;
	
	V_AddEntity(&viewmodel);

	AnglesToAxis(viewmodel.angles, viewmodel.axis); // this is necessary to attach other entities to the model

	CG_AddViewMuzzleFlash(&viewmodel, ps);
	CG_AddViewFlashLight(&viewmodel, ps);

	// second viewmodel is arms so just copy all params to it
	if (ps->viewmodel[1])
	{
		viewmodel.model = cl.model_draw[ps->viewmodel[1]];
		V_AddEntity(&viewmodel);
	}
}