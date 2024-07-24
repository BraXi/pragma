/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _mod_skel_h_
#define _mod_skel_h_


// IF THIS IS CHANGED RENDERER MUST BE RECOMPILED

/*
========================================================================

triangle model and skeletal animation format -- .smd

structs also match optimized binary file

[smdl_header]
[smdl_bone * smdl_header.numbones]
[smdl_seq * smdl_header.numbones * smdl_header.numframes]
[smdl_group * smdl_header.numgroups]
[smdl_vert * smdl_header.numverts]

========================================================================
*/

#define SANIM_IDENT		(('Q'<<24)+('E'<<16)+('S'<<8)+'B') // little-endian "BSEQ"
#define SANIM_VERSION	1

#define SANIM_FPS		10 // default

#define SMDL_IDENT		(('D'<<24)+('O'<<16)+('M'<<8)+'B') // little-endian "BMOD"
#define SMDL_VERSION	1

#define SMDL_MAX_BONES		128 // all the limits can be easily changed
#define SMDL_MAX_FRAMES		512 // a single animation can have, a model will have only one

#define SMDL_MAX_SURFACES	8 // also max textures if each surface has diferent
#define SMDL_MAX_TRIS		32768 // for entire model
#define SMDL_MAX_VERTS		(SMDL_MAX_TRIS*3)

typedef struct smdl_vert_s
{
	float	xyz[3];
	float	normal[3];
	float	uv[2];
	int		bone;		// bone index -- smdl_bone_t[bone]
} smdl_vert_t;

typedef struct smdl_group_s
{
	char name[32];
	char texture[MAX_QPATH];
	int firstvert;			// smdl_vert_t[firstvert]
	int numverts;			// smdl_vert_t[firstvert+numverts] always enough to form triangles
	unsigned int texnum;	// *for engine use*
	unsigned int flags;		// *for engine use*
} smdl_surf_t;

// animation frame == smdl_seq_t * header.numbones
typedef struct smdl_seq_s 
{
	int bone;
	float pos[3];
	float rot[3];
	quat_t	q;
} smdl_seq_t;

typedef struct smdl_bone_s
{
	char name[MAX_QPATH];
	int index;
	int parent;		// parent bone index, -1 == root bone
} smdl_bone_t;

typedef struct smdl_header_s
{
	int ident;		// SMDL_IDENT
	int version;	// SMDL_VERSION

	int numbones;	// ??each lod must have the same??
	int numframes;	// a model will only have one, reference pose
	int numverts;
	int numsurfaces;

	int playrate;	// frames per second

	float mins[3];	// local bounds
	float maxs[3];
	int boundingradius; // radius from bounds
} smdl_header_t;

typedef enum // NOTE: DO NOT CHANGE ORDER 
{
	SMDL_BAD,
	SMDL_ANIMATION,
	SMDL_MODEL,
	SMDL_MODEL_NO_TRIS // because server doesn't need tris
} SMDL_Type;

typedef struct smdl_data_s
{
	SMDL_Type		type;

	smdl_header_t	hdr;

	smdl_bone_t* bones[SMDL_MAX_BONES];
	smdl_seq_t* seqences[SMDL_MAX_FRAMES];

	// a model will have inverted bind pose matrix
	mat4_t		invBindPoseMat[SMDL_MAX_BONES];

	// animations don't have these
	smdl_surf_t* surfaces[SMDL_MAX_SURFACES];
	smdl_vert_t* verts;

} smdl_data_t;

typedef struct smdl_anim_s
{
	char			name[MAX_QPATH];
	smdl_data_t		data;

	int				extradatasize;
	void			*extradata;
} smdl_anim_t;

#endif /*_mod_skel_h_*/