/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _mod_md3_h_
#define _mod_md3_h_

/*
========================================================================

.MD3 triangle model file format

MOD_ALIAS is MD3

========================================================================
*/

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per model
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	8		// per model
#define MD3_MAX_TAGS		16		// per frame
#define MD3_MAX_NAME		64		// max tag and skin names len, this was MAX_QPATH

#define	MD3_XYZ_SCALE		(1.0/64) // vertex scales

typedef struct md3Frame_s
{
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct md3Tag_s
{
	char		name[MD3_MAX_NAME];	// tag name
	vec3_t		origin;
	vec3_t		axis[3];
} md3Tag_t;


typedef struct
{
	int32_t	ident;

	char	name[MD3_MAX_NAME];	// polyset name

	int32_t		flags;

	int32_t		numFrames;			// all surfaces in a model should have the same
	int32_t		numShaders;			// all surfaces in a model should have the same
	int32_t		numVerts;
	int32_t		numTriangles;

	int32_t		ofsTriangles;

	int32_t		ofsShaders;			// offset from start of md3Surface_t
	int32_t		ofsSt;				// texture coords are common for all frames
	int32_t		ofsXyzNormals;		// numVerts * numFrames

	int32_t		ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct
{
	char			name[MAX_QPATH];
	int32_t			shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct
{
	int32_t			indexes[3];
} md3Triangle_t;

typedef struct
{
	float		st[2];
} md3St_t;

typedef struct
{
	int16_t	xyz[3];
	int16_t	normal;
} md3XyzNormal_t;

typedef struct
{
	int32_t			ident;				// == MD3_IDENT
	int32_t			version;			// == MD3_VERSION

	char		name[MAX_QPATH];	// model name

	int32_t			flags;

	int32_t			numFrames;
	int32_t			numTags;
	int32_t			numSurfaces;

	int32_t			numSkins;

	int32_t			ofsFrames;			// offset for first frame
	int32_t			ofsTags;			// numFrames * numTags
	int32_t			ofsSurfaces;		// first surface, others follow

	int32_t			ofsEnd;				// end of file
} md3Header_t;

#endif /*_mod_md3_h_*/