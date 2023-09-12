/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#include "../server/server.h"

void Scr_DefineBuiltin(void (*function)(void), pb_t type, qboolean devmode, char* qcstring);


/*
===============
PF_rint
float rint(float)
===============
*/
void PF_rint(void)
{
	float	f;

	if (Scr_NumArgs() != 1)
	{
		Scr_RunError("called with %i arguments\n", Scr_NumArgs());
		return;
	}

	f = Scr_GetParmFloat(0);
	if (f > 0)
		Scr_ReturnFloat((int)(f + 0.5));
	else
		Scr_ReturnFloat((int)(f - 0.5));
}

/*
===============
PF_floor
float floor(float)
===============
*/
void PF_floor(void)
{
	float val = floor( Scr_GetParmFloat(0) );
	Scr_ReturnFloat(val);
}

/*
===============
PF_ceil
float ceil(float)
===============
*/
void PF_ceil(void)
{
	float val = ceil(Scr_GetParmFloat(0));
	Scr_ReturnFloat(val);
}

/*
===============
PF_sin
float sin(float)
===============
*/
void PF_sin(void)
{
	float val = sin(Scr_GetParmFloat(0));
	Scr_ReturnFloat(val);
}

/*
===============
PF_cos
float cos(float)
===============
*/
void PF_cos(void)
{
	float val = cos(Scr_GetParmFloat(0));
	Scr_ReturnFloat(val);
}

/*
===============
PF_ceil
float sqrt(float)
===============
*/
void PF_sqrt(void)
{
	float val = sqrt(Scr_GetParmFloat(0));
	Scr_ReturnFloat(val);
}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void PF_normalize(void)
{
	float	*invec;
	vec3_t	newvalue;
	float	newv;

	invec = Scr_GetParmVector(0);

	newv = invec[0] * invec[0] + invec[1] * invec[1] + invec[2] * invec[2];
	newv = sqrt(newv);

	if (newv == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		newv = 1 / newv;
		newvalue[0] = invec[0] * newv;
		newvalue[1] = invec[1] * newv;
		newvalue[2] = invec[2] * newv;
	}

	Scr_ReturnVector(newvalue);
}

/*
=================
PF_vlen

float vlen(vector)
=================
*/
void PF_vlen(void)
{
	float	*invec;
	float	retval;

	invec = Scr_GetParmVector(0);

	retval = invec[0] * invec[0] + invec[1] * invec[1] + invec[2] * invec[2];
	retval = sqrt(retval);

	Scr_ReturnFloat(retval);
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void PF_vectoyaw(void)
{
	float	*invec;
	float	yaw;

	invec = Scr_GetParmVector(0);

	if (invec[1] == 0 && invec[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int)(atan2(invec[1], invec[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	Scr_ReturnFloat(yaw);
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void PF_vectoangles(void)
{
	float	*invec;
	float	forward;
	float	yaw, pitch;
	vec3_t	out;

	invec = Scr_GetParmVector(0);

	if (invec[1] == 0 && invec[0] == 0)
	{
		yaw = 0;
		if (invec[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int)(atan2(invec[1], invec[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt(invec[0] * invec[0] + invec[1] * invec[1]);
		pitch = (int)(atan2(invec[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	
	out[0] = pitch;
	out[1] = yaw;
	out[2] = 0;
	Scr_ReturnVector(out);
}

/*
=================
PF_random

Returns a number from 0 <= num < 1

float random()
=================
*/
void PF_random(void)
{
	float		num;
	num = (rand() & 0x7fff) / ((float)0x7fff);
	Scr_ReturnFloat(num);
}

/*
==============
PF_anglevectors

Writes new values for v_forward, v_up, and v_right based on angles
anglevectors(vector)
==============
*/
void PF_anglevectors(void)
{
	AngleVectors(Scr_GetParmVector(0), Scr_GetGlobals()->v_forward, Scr_GetGlobals()->v_right, Scr_GetGlobals()->v_up);
}

static char* PF_rint_str = "float(float val) rint";
static char* PF_floor_str = "float(float val) floor";
static char* PF_ceil_str = "float(float val) ceil";
static char* PF_sin_str = "float(float val) sin";
static char* PF_cos_str = "float(float val) cos";
static char* PF_sqrt_str = "float(float val) sqrt";
static char* PF_normalize_str = "vector(vector in) normalize";
static char* PF_vlen_str = "float(vector in) vlen";
static char* PF_vectoyaw_str = "float(vector in) vectoyaw";
static char* PF_vectoangles_str = "float(vector in) vectoangles";
static char* PF_random_str = "float() random";
static char* PF_anglevectors_str = "void(vector v1) anglevectors";

/*
=================
Scr_InitMathBuiltins

Register mathlib builtins which can be shared by both client and server progs
=================
*/
void Scr_InitMathBuiltins()
{
	Scr_DefineBuiltin(PF_rint, PF_BOTH, false, PF_rint_str);
	Scr_DefineBuiltin(PF_floor, PF_BOTH, false, PF_floor_str);
	Scr_DefineBuiltin(PF_ceil, PF_BOTH, false, PF_ceil_str);
	Scr_DefineBuiltin(PF_sin, PF_BOTH, false, PF_sin_str);
	Scr_DefineBuiltin(PF_cos, PF_BOTH, false, PF_cos_str);
	Scr_DefineBuiltin(PF_sqrt, PF_BOTH, false, PF_sqrt_str);

	Scr_DefineBuiltin(PF_normalize, PF_BOTH, false, PF_normalize_str);
	Scr_DefineBuiltin(PF_vlen, PF_BOTH, false, PF_vlen_str);
	Scr_DefineBuiltin(PF_vectoyaw, PF_BOTH, false, PF_vectoyaw_str);
	Scr_DefineBuiltin(PF_vectoangles, PF_BOTH, false, PF_vectoangles_str);

	Scr_DefineBuiltin(PF_random, PF_BOTH, false, PF_random_str);

	Scr_DefineBuiltin(PF_anglevectors, PF_BOTH, false, PF_anglevectors_str); //fixme
}