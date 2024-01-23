#include "client.h"

/*
==============================================================

LIGHT STYLE MANAGEMENT

==============================================================
*/

typedef struct
{
	int		length;
	float	value[3];		
	float	map[MAX_QPATH];
} clightstyle_t;

static clightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
static int				lastofs;

/*
================
CL_ClearLightStyles
================
*/
void CL_ClearLightStyles(void)
{
	memset(cl_lightstyle, 0, sizeof(cl_lightstyle));
	lastofs = -1;
}

/*
================
CL_RunLightStyles

Advance light animation
================
*/
void CL_RunLightStyles(void)
{
	int		ofs;
	int		i;
	clightstyle_t* ls;

	ofs = cl.time / 100;
	if (ofs == lastofs)
		return;
	lastofs = ofs;

	for (i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++)
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
CL_SetLightStyleFromCS

sets lightstyle from configstring
================
*/
void CL_SetLightStyleFromCS(int i)
{
	char* s;
	int		j, k;

	s = cl.configstrings[i + CS_LIGHTS];

	j = (int)strlen(s);
	if (j >= MAX_QPATH)
		Com_Error(ERR_DROP, "lightstyle %i has too long length %i", i + (CS_LIGHTS), j);

	cl_lightstyle[i].length = j;

	for (k = 0; k < j; k++)
		cl_lightstyle[i].map[k] = (float)(s[k] - 'a') / (float)('m' - 'a');
}

/*
================
CL_AddLightStyles
================
*/
void CL_AddLightStyles(void)
{
	int		i;
	clightstyle_t* ls;

	for (i = 0, ls = cl_lightstyle; i < MAX_LIGHTSTYLES; i++, ls++)
		V_AddLightStyle(i, ls->value[0], ls->value[1], ls->value[2]);
}
