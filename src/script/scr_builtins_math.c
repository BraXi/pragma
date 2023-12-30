/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
#include "../qcommon/qcommon.h"
#include "script_internals.h"

#define scr_random()	( ( rand() & 0x7fff ) / ( (float)0x7fff ) )
#define scr_crandom()	( 2.0 * ( scr_random() - 0.5 ) )

/*
===============
PF_fabs
float fabs(float)
===============
*/
void PF_fabs(void)
{
	float	f;

	if (Scr_NumArgs() != 1)
	{
		Scr_RunError("called with %i arguments\n", Scr_NumArgs());
		return;
	}

	f = Scr_GetParmFloat(0);

	Scr_ReturnFloat( fabs(f) );
}

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
	//num = (rand() & 0x7fff) / ((float)0x7fff);
	num = scr_random();
	Scr_ReturnFloat(num);
}

/*
=================
PF_crandom
Returns a number between -1.0..1.0
float crandom()
=================
*/
void PF_crandom(void)
{
	float		num;
	//num = (rand() & 0x7fff) / ((float)0x7fff);
	num = scr_crandom();
	Scr_ReturnFloat(num);
}


/*
=================
PF_randomint

Returns a integer number between 0 and num

float randomint(float num)
=================
*/
void PF_randomint(void)
{
	float	out;
	int		in;

	in = (int)Scr_GetParmFloat(0);
	if (in <= 0)
	{
		Scr_RunError("randomint(%i) must be called with number greater than 0\n", in);
		return;
	}

	out = (rand() % in);
	Scr_ReturnFloat(out);
}

/*
==============
PF_anglevectors

Writes new values for v_forward, v_up, and v_right based on angles
anglevectors(vector)
==============
*/
extern void PFSV_AngleVectors(void);
extern void PFCG_AngleVectors(void);
void PF_anglevectors(void)
{
//	AngleVectors(Scr_GetParmVector(0), sv.script_globals->v_forward, sv.script_globals->v_right, sv.script_globals->v_up);
	switch (active_qcvm->progsType)
	{
	case VM_SVGAME:
		PFSV_AngleVectors();
		break;
	case VM_CLGAME:
		PFCG_AngleVectors();
		break;
	default:
		Scr_RunError("PF_anglevectors not implemented for this qcvm");
		break;
	}
}

/*
==============
PF_crossproduct
vector cross = crossproduct(vector v1, vector v2)
==============
*/
void PF_crossproduct(void)
{
	vec3_t cross;
	CrossProduct(Scr_GetParmVector(0), Scr_GetParmVector(1), cross);
	Scr_ReturnVector(cross);
}

/*
==============
PF_dotproduct
float dot = dotproduct(vector v1, vector v2)
==============
*/
void PF_dotproduct(void)
{
	float dot = _DotProduct(Scr_GetParmVector(0), Scr_GetParmVector(1));
	Scr_ReturnFloat(dot);
}

/*
==============
PF_anglemod
returns angle in range 0-360
float angle = anglemod(float a)
==============
*/
void PF_anglemod(void)
{
	float out;
	out = anglemod(Scr_GetParmFloat(0));
	Scr_ReturnFloat(out);
}

/*
==============
PF_lerpangle

float smoothed_angle = lerpangle(float a2, float a1, float frac)
==============
*/
void PF_lerpangle(void)
{
	float out;
	out = LerpAngle(Scr_GetParmFloat(0), Scr_GetParmFloat(1), Scr_GetParmFloat(2));
	Scr_ReturnFloat(out);
}


/*
=================
Scr_InitMathBuiltins

Register mathlib builtins which can be shared by all QCVMs
=================
*/
void Scr_InitMathBuiltins()
{
	Scr_DefineBuiltin(PF_fabs, PF_ALL, "fabs", "float(float val)");
	Scr_DefineBuiltin(PF_rint, PF_ALL, "rint", "float(float val)");
	Scr_DefineBuiltin(PF_floor, PF_ALL, "floor", "float(float val)");
	Scr_DefineBuiltin(PF_ceil, PF_ALL, "ceil", "float(float val)");
	Scr_DefineBuiltin(PF_sin, PF_ALL, "sin", "float(float val)");
	Scr_DefineBuiltin(PF_cos, PF_ALL, "cos", "float(float val)");
	Scr_DefineBuiltin(PF_sqrt, PF_ALL, "sqrt", "float(float val)");

	Scr_DefineBuiltin(PF_normalize, PF_ALL, "normalize", "vector(vector in)");
	Scr_DefineBuiltin(PF_vlen, PF_ALL, "vectorlength", "float(vector in)");
	Scr_DefineBuiltin(PF_vectoyaw, PF_ALL, "vectoyaw", "float(vector in)");
	Scr_DefineBuiltin(PF_vectoangles, PF_ALL, "vectoangles", "vector(vector in)");

	Scr_DefineBuiltin(PF_random, PF_ALL, "random", "float()");
//	Scr_DefineBuiltin(PF_crandom, PF_ALL, false, "float() crandom");
	Scr_DefineBuiltin(PF_randomint, PF_ALL, "randomint", "float(float n)");

	Scr_DefineBuiltin(PF_anglevectors, PF_ALL, "anglevectors", "void(vector v1)");
	Scr_DefineBuiltin(PF_crossproduct, PF_ALL, "crossproduct", "vector(vector v1, vector v2)");
	Scr_DefineBuiltin(PF_dotproduct, PF_ALL, "dotproduct", "float(vector v1, vector v2)");
	Scr_DefineBuiltin(PF_anglemod, PF_ALL, "anglemod", "float(float a)");
	Scr_DefineBuiltin(PF_lerpangle, PF_ALL, "lerpangle", "float(float a2, float a1, float frac)");
}