/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _mod_skel_h_
#define _mod_skel_h_

/*
skeletal model and animation format

[smdl_header]
[smdl_bone * smdl_header.numbones]
[smdl_seq * smdl_header.numbones * smdl_header.numframes]
[smdl_group * smdl_header.numgroups]
[smdl_vert_t * smdl_header.numverts]
*/

#define SMDL_IDENT		(('F'<<24)+('S'<<16)+('X'<<8)+'B') // little-endian "BXSF"
#define SMDL_VERSION	1

#define SMDL_MAX_BONES	64 // all the limits can be easily changed
#define SMDL_MAX_FRAMES	512
#define SMDL_MAX_GROUPS	8
#define SMDL_MAX_VERTS	4096

typedef struct smdl_tri_s
{
	float	xyz[3];
	float	normal[3];
	float	uv[2];
	int		bone;		// parent index -- smdl_bone_t[bone]
} smdl_vert_t;

typedef struct smdl_group_s
{
	char texture[MAX_QPATH];
	int firstvert;		// smdl_vert_t[firstvert]
	int numverts;		// always numverts % 3
	int texnum;			// for engine use
	int flags;			// for engine use
} smdl_surf_t;

typedef struct smdl_seq_s
{
	int bone;
	float pos[3];
	float rot[3];
} smdl_seq_t;

typedef struct smdl_bone_s
{
	char name[MAX_QPATH];
	int index;
	int parent;
} smdl_bone_t;

typedef struct smdl_header_s
{
	int ident;		// SMDL_IDENT
	int version;	// SMDL_VERSION

	int numbones;	// each lod must have the same
	int numframes;
	int numverts;
	int numsurfaces;

	int playrate;	// frames per second

	float mins[3];	// local bbox of model verts or entire animaton
	float maxs[3];

	int ofs_bones;
	int ofs_seq;
	int ofs_groups;
	int ofs_verts;
} smdl_header_t;


typedef struct smdl_data_s
{
	smdl_header_t	hdr;
} smdl_data_t;

#endif /*_mod_skel_h_*/