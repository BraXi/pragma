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
#define MD3_MAX_LODS		NUM_LODS // was 4, but we want NUM_LODS in pragma
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface (???)
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	8		// per model, braxi -- was 32, changed to 8 to fit into network's hidePartBits
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
	int		ident;

	char	name[MD3_MAX_NAME];	// polyset name

	int		flags;

	int		numFrames;			// all surfaces in a model should have the same
	int		numShaders;			// all surfaces in a model should have the same
	int		numVerts;
	int		numTriangles;

	int		ofsTriangles;

	int		ofsShaders;			// offset from start of md3Surface_t
	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct
{
	char			name[MAX_QPATH];
	int				shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct
{
	int			indexes[3];
} md3Triangle_t;

typedef struct
{
	float		st[2];
} md3St_t;

typedef struct
{
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct
{
	int			ident;				// == MD3_IDENT
	int			version;			// == MD3_VERSION

	char		name[MAX_QPATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
} md3Header_t;

#endif /*_mod_md3_h_*/