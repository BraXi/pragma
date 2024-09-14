/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "../client.h"
#include "cg_local.h"

/*
==============================================================

LIGHT STYLE MANAGEMENT

==============================================================
*/

static CLightStyle_t	cg_lightstyles[MAX_LIGHTSTYLES];
static int				lastofs;

/*
================
CG_ClearLightStyles
================
*/
void CG_ClearLightStyles()
{
	memset(cg_lightstyles, 0, sizeof(cg_lightstyles));
	lastofs = -1;
}

/*
================
CG_RunLightStyles

Advance light animation
================
*/
void CG_RunLightStyles(void)
{
	int		ofs;
	int		i;
	CLightStyle_t* ls;

	ofs = cl.time / 100;
	if (ofs == lastofs)
		return;
	lastofs = ofs;

	for (i = 0, ls = cg_lightstyles; i < MAX_LIGHTSTYLES; i++, ls++)
	{
		if (!ls->length)
		{
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}
		if (ls->length == 1)
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		else
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[ofs % ls->length];
	}
}

/*
================
CG_LightStyleFromConfigString

sets lightstyle from configstring
================
*/
void CG_LightStyleFromConfigString(int index)
{
	char	*config;
	size_t	len;
	int		k;

	if (index < 0)
	{
		Com_Error(ERR_DROP, "%s: index %i < 0", __FUNCTION__, index);
		return; //msvc
	}

	if (index >= MAX_LIGHTSTYLES)
	{
		Com_Error(ERR_DROP, "%s: index %i >= MAX_LIGHTSTYLES", __FUNCTION__, index);
		return; //msvc
	}

	config = cl.configstrings[index + CS_LIGHTS];

	len = (int)strlen(config);
	if (len >= MAX_QPATH)
		Com_Error(ERR_DROP, "%s: style %i is too long", __FUNCTION__, index);

	cg_lightstyles[index].length = len;

	for (k = 0; k < len; k++)
		cg_lightstyles[index].map[k] = (float)(config[k] - 'a') / (float)('m' - 'a');
}

/*
================
CG_AddLightStyles
================
*/
void CG_AddLightStyles(void)
{
	int		i;
	CLightStyle_t* ls;

	for (i = 0, ls = cg_lightstyles; i < MAX_LIGHTSTYLES; i++, ls++)
		V_AddLightStyle(i, ls->value[0], ls->value[1], ls->value[2]);
}


