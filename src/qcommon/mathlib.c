#include "shared.h"

#define USE_SSE
#ifdef USE_SSE
#include <intrin.h>
#endif

vec3_t vec3_origin = { 0,0,0 };

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

/*
================
MakeNormalVectors
================
*/
void MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	// this rotate and negat guarantees a vector
	// not colinear with the original
	right[1] = -forward[0]; // stupid quake bug?
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

/*
================
RotatePointAroundVector
================
*/
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees)
{
	mat3x3_t	m;
	mat3x3_t	im;
	mat3x3_t	zrot;
	mat3x3_t	tmpmat;
	mat3x3_t	rot;
	int	i;
	vec3_t vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = cos(DEG2RAD(degrees));
	zrot[0][1] = sin(DEG2RAD(degrees));
	zrot[1][0] = -sin(DEG2RAD(degrees));
	zrot[1][1] = cos(DEG2RAD(degrees));

	R_ConcatRotations(m, zrot, tmpmat);
	R_ConcatRotations(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif

/*
================
AngleVectors
================
*/
void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

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
		// stupid quake bug here?
		right[0] = -sr * sp * cy + cr * sy;
		right[1] = -sr * sp * sy - cr * cy;
		right[2] = -sr * cp;
	}
	if (up)
	{
		up[0] = cr * sp * cy + sr * sy;
		up[1] = cr * sp * sy - sr * cy;
		up[2] = cr * cp;
	}
}

/*
================
ProjectPointOnPlane
================
*/
void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
PerpendicularVector

assumes scr is normalied
================
*/
void PerpendicularVector(vec3_t dst, const vec3_t src)
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}



/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations(mat3x3_t in1, mat3x3_t in2, mat3x3_t out)
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
}


/*
================
R_ConcatTransforms
================
*/
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
		in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
		in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
		in1[2][2] * in2[2][3] + in1[2][3];
}


//============================================================================


float Q_fabs(float f)
{
	int tmp = *(int*)&f;
	tmp &= 0x7FFFFFFF;
	return *(float*)&tmp;
}

#if defined _M_IX86 && !defined C_ONLY
#pragma warning (disable:4035)
__declspec(naked) long Q_ftol(float f)
{
	static int tmp;
	__asm fld dword ptr[esp + 4]
		__asm fistp tmp
	__asm mov eax, tmp
	__asm ret
}
#pragma warning (default:4035)
#endif

/*
===============
LerpAngle

===============
*/
float LerpAngle(float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}


float	anglemod(float a)
{
#if 0
	if (a >= 0)
		a -= 360 * (int)(a / 360);
	else
		a += 360 * (1 + (int)(-a / 360));
#endif
	a = (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
	return a;
}


// this is the slow, general version
int BoxOnPlaneSide2(vec3_t emins, vec3_t emaxs, struct cplane_s* p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i = 0; i < 3; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct(p->normal, corners[0]) - p->dist;
	dist2 = DotProduct(p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s* p)
{
	float	dist1, dist2;
	int		sides;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		assert(0);
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	assert(sides != 0);

	return sides;
}

void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i = 0; i < 3; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

int Vector2Compare(vec2_t v1, vec2_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1])
		return 0;

	return 1;
}

int VectorCompare(vec3_t v1, vec3_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
		return 0;

	return 1;
}

int Vector4Compare(vec4_t v1, vec4_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3])
		return 0;

	return 1;
}

vec_t VectorNormalize(vec3_t v)
{
	float	length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt(length);		// FIXME

	if (length)
	{
		ilength = 1 / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
	return length;
}

vec_t VectorNormalize2(vec3_t v, vec3_t out)
{
	float	length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt(length);		// FIXME

	if (length)
	{
		ilength = 1 / length;
		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	}

	return length;

}

void Vector2MA(vec2_t veca, float scale, vec2_t vecb, vec2_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
}

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

void Vector4MA(vec4_t veca, float scale, vec4_t vecb, vec4_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
	vecc[3] = veca[3] + scale * vecb[3];
}

vec_t _DotProduct(vec3_t v1, vec3_t v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

void _VectorCopy(vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

//double sqrt(double x);

vec_t Vector2Length(vec2_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i = 0; i < 2; i++)
		length += v[i] * v[i];
	length = sqrt(length);

	return length;
}

vec_t VectorLength(vec3_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i = 0; i < 3; i++)
		length += v[i] * v[i];
	length = sqrt(length);

	return length;
}

vec_t Vector4Length(vec4_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i = 0; i < 4; i++)
		length += v[i] * v[i];
	length = sqrt(length);

	return length;
}

void VectorInverse(vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void Vector2Scale(vec2_t in, vec_t scale, vec2_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
}

void VectorScale(vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void Vector4Scale(vec4_t in, vec_t scale, vec4_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
	out[3] = in[3] * scale;
}

int Q_log2(int val)
{
	int answer = 0;
	while (val >>= 1)
		answer++;
	return answer;
}

/*
=================
VectorAngles_Fixed

Stupid quake bug dismissed.
=================
*/
void VectorAngles_Fixed(const float* forward, float* result)
{
	float tmp, yaw, pitch;

	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 270.0f;
		else
			pitch = 90.0f;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180.0f / M_PI);

		if (yaw < 0.0f)
			yaw += 360.0f;

		tmp = sqrt( ((double)forward[0] * (double)forward[0]) + ((double)forward[1] * (double)forward[1]));
		pitch = (atan2(-forward[2], tmp) * 180.0f / M_PI);

		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	result[0] = pitch;
	result[1] = yaw;
	result[2] = 0;
}


/*
=================
VectorAngles
Code written by Spoike
=================
*/
void VectorAngles(const float* forward, const float* up, float* result)   //up may be NULL
{
	
	float    yaw, pitch, roll;

	if (forward[1] == 0 && forward[0] == 0)
	{
		if (forward[2] > 0)
		{
			pitch = -M_PI * 0.5;
			yaw = up ? atan2(-up[1], -up[0]) : 0;
		}
		else
		{
			pitch = M_PI * 0.5;
			yaw = up ? atan2(up[1], up[0]) : 0;
		}
		roll = 0;
	}
	else
	{
		yaw = atan2(forward[1], forward[0]);
		pitch = -atan2(forward[2], sqrt(forward[0] * forward[0] + forward[1] * forward[1]));

		if (up)
		{
			vec_t cp = cos(pitch), sp = sin(pitch);
			vec_t cy = cos(yaw), sy = sin(yaw);
			vec3_t tleft, tup;
			tleft[0] = -sy;
			tleft[1] = cy;
			tleft[2] = 0;
			tup[0] = sp * cy;
			tup[1] = sp * sy;
			tup[2] = cp;
			roll = -atan2(DotProduct(up, tleft), DotProduct(up, tup));
		}
		else
			roll = 0;
	}

	pitch *= 180 / M_PI;
	yaw *= 180 / M_PI;
	roll *= 180 / M_PI;

	if (pitch < 0)
		pitch += 360;
	if (yaw < 0)
		yaw += 360;
	if (roll < 0)
		roll += 360;

	result[0] = pitch;
	result[1] = yaw;
	result[2] = roll;
}


/*
=================
AxisToAngles
=================
*/
void AxisToAngles(vec3_t axis[3], vec3_t outAngles)
{
	float pitch, yaw, roll;

	pitch = asin(-axis[2][0]);
	if (cos(pitch) != 0)
	{
		yaw = atan2(axis[2][1] / cos(pitch), axis[2][2] / cos(pitch));
		roll = atan2(axis[1][0] / cos(pitch), axis[0][0] / cos(pitch));
	}
	else
	{
		roll = atan2(-axis[0][1], axis[1][1]);
		yaw = 0;
	}

	yaw = yaw * 180.0 / M_PI;
	pitch = pitch * 180.0 / M_PI;
	roll = roll * 180.0 / M_PI;

	outAngles[0] = pitch;
	outAngles[1] = yaw;
	outAngles[2] = roll;
}

/*
=================
AnglesToAxis (Q3)
=================
*/
void AnglesToAxis(vec3_t angles, vec3_t axis[3]) 
{
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors(angles, axis[0], right, axis[2]);
	VectorSubtract(vec3_origin, right, axis[1]);
}


/*
=================
AxisCopy (Q3)
=================
*/
void AxisCopy(vec3_t in[3], vec3_t out[3])
{
	VectorCopy(in[0], out[0]);
	VectorCopy(in[1], out[1]);
	VectorCopy(in[2], out[2]);
}

/*
=================
AxisClear (Q3)
=================
*/
void AxisClear(vec3_t axis[3])
{
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

/*
================
MatrixMultiply (Q3)
================
*/
void MatrixMultiply(mat3x3_t in1, mat3x3_t in2, mat3x3_t out)
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength(corner);
}

mat4_t mat4_identity =	{1, 0, 0, 0,
						 0, 1, 0, 0,
						 0, 0, 1, 0,
						 0, 0, 0, 1 };

/*
=================
Mat4MakeIdentity
=================
*/
void Mat4MakeIdentity(mat4_t mat)
{
	memset(mat, 0, sizeof(mat4_t));
	mat[0] = mat[5] = mat[10] = mat[15] = 1;
}

/*
=================
Mat4Perspective

Unlike glFrustum, this does not multiply by another matrix, it only creates one.
=================
*/
void Mat4Perspective(mat4_t mat, float l, float r, float b, float t, float znear, float zfar)
{
	memset(mat, 0, sizeof(mat4_t));
	mat[0] = (2 * znear) / (r - l);
	mat[5] = (2 * znear) / (t - b);
	mat[8] = (r + l) / (r - l);
	mat[9] = (t + b) / (t - b);
	mat[10] = -((zfar + znear) / (zfar - znear));
	mat[11] = -1;
	mat[14] = -((2 * zfar * znear) / (zfar - znear));
}

/*
=================
Mat4Ortho

Unlike glOrtho, this does not multiply by another matrix, it only creates one.
=================
*/
void Mat4Ortho(mat4_t mat, float l, float r, float b, float t, float znear, float zfar)
{
	memset(mat, 0, sizeof(mat4_t));
	mat[0] = 2 / (r - l);
	mat[5] = 2 / (t - b);
	mat[10] = -2 / (zfar - znear);
	mat[12] = -((r + l) / (r - l));
	mat[13] = -((t + b) / (t - b));
	mat[14] = -((zfar + znear) / (zfar - znear));
	mat[15] = 1;
}

/*
=================
Mat4Multiply
=================
*/
void Mat4Multiply(mat4_t left, mat4_t right)
{
	mat4_t t;
	memcpy(t, left, sizeof(t));
#ifdef USE_SSE
	//The same 4 columns are always used, so load them first.
	__m128 columns[4];
	__m128 rightvalue[4];
	__m128 res;
	columns[0] = _mm_loadu_ps(&t[0]);
	columns[1] = _mm_loadu_ps(&t[4]);
	columns[2] = _mm_loadu_ps(&t[8]);
	columns[3] = _mm_loadu_ps(&t[12]);

	//Each column will broadcast 4 values from the right matrix and multiply each column by it, 
	// and then sum up the resultant vectors to form a result column. 
	for (int i = 0; i < 4; i++)
	{
		rightvalue[0] = _mm_mul_ps(_mm_load_ps1(&right[i * 4 + 0]), columns[0]);
		rightvalue[1] = _mm_mul_ps(_mm_load_ps1(&right[i * 4 + 1]), columns[1]);
		rightvalue[2] = _mm_mul_ps(_mm_load_ps1(&right[i * 4 + 2]), columns[2]);
		rightvalue[3] = _mm_mul_ps(_mm_load_ps1(&right[i * 4 + 3]), columns[3]);

		//From some quick profiling this seems slightly faster than nesting all the adds. 
		res = _mm_add_ps(rightvalue[0], rightvalue[1]);
		res = _mm_add_ps(res, rightvalue[2]);
		res = _mm_add_ps(res, rightvalue[3]);

		_mm_storeu_ps(&left[i * 4], res);
	}

#else
	left[0] = t[0] * right[0] + t[4] * right[1] + t[8]  * right[2] + t[12] * right[3]; //i1 j1
	left[1] = t[1] * right[0] + t[5] * right[1] + t[9]  * right[2] + t[13] * right[3]; //i2 j1
	left[2] = t[2] * right[0] + t[6] * right[1] + t[10] * right[2] + t[14] * right[3]; //i3 j1
	left[3] = t[3] * right[0] + t[7] * right[1] + t[11] * right[2] + t[15] * right[3]; //14 j1

	left[4] = t[0] * right[4] + t[4] * right[5] + t[8]  * right[6] + t[12] * right[7]; //i1 j2
	left[5] = t[1] * right[4] + t[5] * right[5] + t[9]  * right[6] + t[13] * right[7]; //i2 j2
	left[6] = t[2] * right[4] + t[6] * right[5] + t[10] * right[6] + t[14] * right[7]; //i3 j2
	left[7] = t[3] * right[4] + t[7] * right[5] + t[11] * right[6] + t[15] * right[7]; //i4 j2

	left[8]  = t[0] * right[8] + t[4] * right[9] + t[8]  * right[10] + t[12] * right[11]; //i1 j3
	left[9]  = t[1] * right[8] + t[5] * right[9] + t[9]  * right[10] + t[13] * right[11]; //i2 j3
	left[10] = t[2] * right[8] + t[6] * right[9] + t[10] * right[10] + t[14] * right[11]; //i3 j3
	left[11] = t[3] * right[8] + t[7] * right[9] + t[11] * right[10] + t[15] * right[11]; //i4 j3

	left[12] = t[0] * right[12] + t[4] * right[13] + t[8]  * right[14] + t[12] * right[15]; //i1 j4
	left[13] = t[1] * right[12] + t[5] * right[13] + t[9]  * right[14] + t[13] * right[15]; //i2 j4
	left[14] = t[2] * right[12] + t[6] * right[13] + t[10] * right[14] + t[14] * right[15]; //i3 j4
	left[15] = t[3] * right[12] + t[7] * right[13] + t[11] * right[14] + t[15] * right[15]; //i4 j4
#endif
}

/*
=================
Mat4RotateAroundX
=================
*/
void Mat4RotateAroundX(mat4_t mat, float angle)
{
	mat4_t rotmat = {0};
	angle = DEG2RAD(angle);
	rotmat[0] = rotmat[15] = 1;
	rotmat[5] = cos(angle);
	rotmat[6] = sin(angle);
	rotmat[9] = -sin(angle);
	rotmat[10] = cos(angle);

	Mat4Multiply(mat, rotmat);
}

/*
=================
Mat4RotateAroundY
=================
*/
void Mat4RotateAroundY(mat4_t mat, float angle)
{
	mat4_t rotmat = { 0 };
	angle = DEG2RAD(angle);
	rotmat[5] = rotmat[15] = 1;
	rotmat[0] = cos(angle);
	rotmat[2] = -sin(angle);
	rotmat[8] = sin(angle);
	rotmat[10] = cos(angle);

	Mat4Multiply(mat, rotmat);
}

/*
=================
Mat4RotateAroundZ
=================
*/
void Mat4RotateAroundZ(mat4_t mat, float angle)
{
	mat4_t rotmat = { 0 };
	angle = DEG2RAD(angle);
	rotmat[10] = rotmat[15] = 1;
	rotmat[0] = cos(angle);
	rotmat[1] = sin(angle);
	rotmat[4] = -sin(angle);
	rotmat[5] = cos(angle);

	Mat4Multiply(mat, rotmat);
}

/*
=================
Mat4Rotate
=================
*/
void Mat4Rotate(mat4_t mat, float angle, float x, float y, float z)
{
	mat4_t rotmat = { 0 };
	rotmat[15] = 1;
	angle = DEG2RAD(angle);
	float c = cos(angle);
	float s = sin(angle);
	float xx = x * x;
	float xy = x * y;
	float xz = x * z;
	float yy = y * y;
	float yz = y * z;
	float zz = z * z;
	float xs = x * s;
	float ys = y * s;
	float zs = z * s;
	float oneminusc = (1 - c);

	float length = sqrt(x * x + y * y + z * z);
	//don't do anything on a null vector. 
	if (length == 0)
		return; 

	if (abs(length - 1) > 1e-4)
	{
		x /= length; y /= length; z /= length;
	}

	rotmat[0] = xx * oneminusc + c;
	rotmat[1] = xy * oneminusc + zs;
	rotmat[2] = xz * oneminusc - ys;

	rotmat[4] = xy * oneminusc - zs;
	rotmat[5] = yy * oneminusc + c;
	rotmat[6] = yz * oneminusc + xs;

	rotmat[8] = xz * oneminusc + ys;
	rotmat[9] = yz * oneminusc - xs;
	rotmat[10] = zz * oneminusc + c;

	Mat4Multiply(mat, rotmat);
}

/*
=================
Mat4Translate
=================
*/
void Mat4Translate(mat4_t mat, float x, float y, float z)
{
	mat4_t trans = { 0 };
	trans[0] = trans[5] = trans[10] = trans[15] = 1;
	trans[12] = x; trans[13] = y; trans[14] = z;

	Mat4Multiply(mat, trans);
}

/*
=================
Mat4Scale
=================
*/
void Mat4Scale(mat4_t mat, float x, float y, float z)
{
	mat4_t scale = { 0 };
	scale[15] = 1;
	scale[0] = x; scale[5] = y; scale[10] = z;

	Mat4Multiply(mat, scale);
}

//====================================================================================