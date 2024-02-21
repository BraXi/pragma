/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

typedef vec_t rect_t[4];
typedef vec_t rgba_t[4];

typedef	int	fixed4_t;
typedef	int	fixed8_t;
typedef	int	fixed16_t;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

struct cplane_s;

extern vec3_t vec3_origin;

typedef struct
{
	vec3_t		origin;
	vec3_t		axis[3];
} orientation_t;

#define	nanmask (255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

// microsoft's fabs seems to be ungodly slow...
//float Q_fabs (float f);
//#define	fabs(f) Q_fabs(f)
#if !defined C_ONLY && !defined __linux__ && !defined __sgi
extern long Q_ftol(float f);
#else
#define Q_ftol( f ) ( long ) (f)
#endif

#define clamp(a,b,c)    ((a)<(b)?(a)=(b):(a)>(c)?(a)=(c):(a)) // Q2PRO

#define DotProduct(x,y)			(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define VectorCopy(a,b)			(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define VectorClear(a)			(a[0]=a[1]=a[2]=0)
#define VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

void MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up);
void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

// just in case you do't want to use the macros
vec_t _DotProduct(vec3_t v1, vec3_t v2);
void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy(vec3_t in, vec3_t out);

void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs);
int VectorCompare(vec3_t v1, vec3_t v2);
vec_t VectorLength(vec3_t v);
void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);
vec_t VectorNormalize(vec3_t v);		// returns vector length
vec_t VectorNormalize2(vec3_t v, vec3_t out);
void VectorInverse(vec3_t v);
void VectorScale(vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);

void VectorAngles(const float* forward, const float* up, float* result);
void AxisToAngles(vec3_t axis[3], vec3_t outAngles);
void AnglesToAxis(vec3_t angles, vec3_t axis[3]);
void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);

void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s* plane);
float anglemod(float a);
float LerpAngle(float a1, float a2, float frac);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void PerpendicularVector(vec3_t dst, const vec3_t src);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);