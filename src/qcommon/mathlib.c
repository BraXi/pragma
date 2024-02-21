#include "shared.h"

vec3_t vec3_origin = { 0,0,0 };

#define DEG2RAD( a ) ( a * M_PI ) / 180.0F

void MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	// this rotate and negat guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees)
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
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
** assumes "src" is normalized
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
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3])
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
#if !id386 || defined __linux__ 
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
#else
#pragma warning( disable: 4035 )

__declspec(naked) int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s* p)
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx

		cmp bops_initialized, 1
		je  initialized
		mov bops_initialized, 1

		mov Ljmptab[0 * 4], offset Lcase0
		mov Ljmptab[1 * 4], offset Lcase1
		mov Ljmptab[2 * 4], offset Lcase2
		mov Ljmptab[3 * 4], offset Lcase3
		mov Ljmptab[4 * 4], offset Lcase4
		mov Ljmptab[5 * 4], offset Lcase5
		mov Ljmptab[6 * 4], offset Lcase6
		mov Ljmptab[7 * 4], offset Lcase7

		initialized :

		mov edx, ds : dword ptr[4 + 12 + esp]
			mov ecx, ds : dword ptr[4 + 4 + esp]
			xor eax, eax
			mov ebx, ds : dword ptr[4 + 8 + esp]
			mov al, ds : byte ptr[17 + edx]
			cmp al, 8
			jge Lerror
			fld ds : dword ptr[0 + edx]
			fld st(0)
			jmp dword ptr[Ljmptab + eax * 4]
			Lcase0 :
			fmul ds : dword ptr[ebx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ebx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			jmp LSetSides
			Lcase1 :
		fmul ds : dword ptr[ecx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ebx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			jmp LSetSides
			Lcase2 :
		fmul ds : dword ptr[ebx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ecx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			jmp LSetSides
			Lcase3 :
		fmul ds : dword ptr[ecx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ecx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			jmp LSetSides
			Lcase4 :
		fmul ds : dword ptr[ebx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ebx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			jmp LSetSides
			Lcase5 :
		fmul ds : dword ptr[ecx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ebx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			jmp LSetSides
			Lcase6 :
		fmul ds : dword ptr[ebx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ecx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ecx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			jmp LSetSides
			Lcase7 :
		fmul ds : dword ptr[ecx]
			fld ds : dword ptr[0 + 4 + edx]
			fxch st(2)
			fmul ds : dword ptr[ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[4 + ecx]
			fld ds : dword ptr[0 + 8 + edx]
			fxch st(2)
			fmul ds : dword ptr[4 + ebx]
			fxch st(2)
			fld st(0)
			fmul ds : dword ptr[8 + ecx]
			fxch st(5)
			faddp st(3), st(0)
			fmul ds : dword ptr[8 + ebx]
			fxch st(1)
			faddp st(3), st(0)
			fxch st(3)
			faddp st(2), st(0)
			LSetSides :
			faddp st(2), st(0)
			fcomp ds : dword ptr[12 + edx]
			xor ecx, ecx
			fnstsw ax
			fcomp ds : dword ptr[12 + edx]
			and ah, 1
			xor ah, 1
			add cl, ah
			fnstsw ax
			and ah, 1
			add ah, ah
			add cl, ah
			pop ebx
			mov eax, ecx
			ret
			Lerror :
		int 3
	}
}
#pragma warning( default: 4035 )
#endif

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


int VectorCompare(vec3_t v1, vec3_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
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

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
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

double sqrt(double x);

vec_t VectorLength(vec3_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i = 0; i < 3; i++)
		length += v[i] * v[i];
	length = sqrt(length);		// FIXME

	return length;
}

void VectorInverse(vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale(vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
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

	outAngles[0] = 0;
	outAngles[1] = pitch;
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
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3])
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

//====================================================================================