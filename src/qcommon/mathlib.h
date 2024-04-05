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

typedef vec_t mat3x3_t[3][3];
//Column-major matrix, intended for OpenGL. 
typedef vec_t mat4_t[16];
extern mat4_t mat4_identity;

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

#define Dot2Product(x,y)		(x[0]*y[0]+x[1]*y[1])
#define DotProduct(x,y)			(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define Dot4Product(x,y)		(x[0]*y[0]+x[1]*y[1]+x[2]*y[2]+x[3]*y[3])

#define VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define VectorCopy(a,b)			(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define VectorClear(a)			(a[0]=a[1]=a[2]=0)
#define VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

#define Vector2Subtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1])
#define Vector2Add(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1])
#define Vector2Copy(a,b)		(b[0]=a[0],b[1]=a[1])
#define Vector2Clear(a)			(a[0]=a[1]=0)
#define Vector2Negate(a,b)		(b[0]=-a[0],b[1]=-a[1])
#define Vector2Set(v, x, y)		(v[0]=(x), v[1]=(y))

#define Vector4Subtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2],c[3]=a[3]-b[3])
#define Vector4Add(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2],c[3]=a[3]+b[3])
#define Vector4Copy(a,b)		(b[0]=a[0],b[1]=a[1],b[2]=a[2],b[3]=a[3])
#define Vector4Clear(a)			(a[0]=a[1]=a[2]=a[3]=0)
#define Vector4Negate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2],b[3]=-a[3])
#define Vector4Set(v, x,y,z,w)	(v[0]=(x), v[1]=(y), v[2]=(z), v[3]=(w))

void Vector2MA(vec2_t veca, float scale, vec2_t vecb, vec2_t vecc);
void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);
void Vector4MA(vec4_t veca, float scale, vec4_t vecb, vec4_t vecc);

void Vector2Scale(vec2_t in, vec_t scale, vec2_t out);
void VectorScale(vec3_t in, vec_t scale, vec3_t out);
void Vector4Scale(vec4_t in, vec_t scale, vec4_t out);

int Vector2Compare(vec2_t v1, vec2_t v2);
int VectorCompare(vec3_t v1, vec3_t v2);
int Vector4Compare(vec4_t v1, vec4_t v2);

vec_t Vector2Length(vec2_t v);
vec_t VectorLength(vec3_t v);
vec_t Vector4Length(vec4_t v);

// just in case you do't want to use the macros
vec_t _DotProduct(vec3_t v1, vec3_t v2);
void _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy(vec3_t in, vec3_t out);

void MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up);

void ClearBounds(vec3_t mins, vec3_t maxs);
void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs);

void CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);
vec_t VectorNormalize(vec3_t v);		// returns vector length
vec_t VectorNormalize2(vec3_t v, vec3_t out);
void VectorInverse(vec3_t v);

int Q_log2(int val);

void VectorAngles(const float* forward, const float* up, float* result);
void AxisToAngles(vec3_t axis[3], vec3_t outAngles);
void AnglesToAxis(vec3_t angles, vec3_t axis[3]);
void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);
void MatrixMultiply(mat3x3_t in1, mat3x3_t in2, mat3x3_t out);

void R_ConcatRotations(mat3x3_t in1, mat3x3_t in2, mat3x3_t out);
void R_ConcatTransforms(float in1[3][4], float in2[3][4], float out[3][4]);

void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s* plane);
float anglemod(float a);
float LerpAngle(float a1, float a2, float frac);

void Mat4MakeIdentity(mat4_t mat);
void Mat4Perspective(mat4_t mat, float l, float r, float b, float t, float znear, float zfar);
//Does left x right, results in left. 
void Mat4Multiply(mat4_t left, mat4_t right);
//Rotates are performed by multiplying a resultant matrix against mat.
//All rotates in the rendering code are rotations around cardinal axes, so only these are supported ATM.
void Mat4RotateAroundX(mat4_t mat, float angle);
void Mat4RotateAroundY(mat4_t mat, float angle);
void Mat4RotateAroundZ(mat4_t mat, float angle);
//Multiplies mat by a translation matrix composed of xyz.
void Mat4Translate(mat4_t mat, float x, float y, float z);
//Multiplies mat by a scale matrix composed of xyz.
void Mat4Scale(mat4_t mat, float x, float y, float z);

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

float RadiusFromBounds(vec3_t mins, vec3_t maxs);