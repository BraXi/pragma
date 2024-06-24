/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')

typedef struct
{
	char	name[56];
	int32_t		filepos, filelen;
} dpackfile_t;

typedef struct
{
	int32_t		ident;		// == IDPAKHEADER
	int32_t		dirofs;
	int32_t		dirlen;
} dpackheader_t;

#define	MAX_FILES_IN_PACK	4096


/*
========================================================================

 MODELS

========================================================================
*/

typedef enum { LOD_HIGH, LOD_MEDIUM, LOD_LOW, NUM_LODS } lod_t;

#include "mod_smdl.h"
#include "mod_md3.h" // md3s are cheap to animate but consume much more memory

/*
==============================================================================

  .BSP file format

==============================================================================
*/

// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024

typedef enum
{
	BSP_MODELS, BSP_BRUSHES, BSP_ENTITIES, BSP_ENTSTRING, BSP_TEXINFO,
	BSP_AREAS, BSP_AREAPORTALS, BSP_PLANES, BSP_NODES, BSP_BRUSHSIDES,
	BSP_LEAFS, BSP_VERTS, BSP_FACES, BSP_LEAFFACES, BSP_LEAFBRUSHES,
	BSP_PORTALS, BSP_EDGES, BSP_SURFEDGES, BSP_LIGHTING, BSP_VISIBILITY, BSP_NUM_DATATYPES
} bspDataType;

//
// REGULAR QUAKE2 BSP FORMAT
//
#define BSP_IDENT		(('P'<<24)+('S'<<16)+('B'<<8)+'I') // little-endian "IBSP"
#define BSP_VERSION	38

// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by 16 bit short limits
#define	MAX_MAP_MODELS		1024
#define	MAX_MAP_BRUSHES		8192
#define	MAX_MAP_ENTITIES	2048
#define	MAX_MAP_ENTSTRING	0x40000
#define	MAX_MAP_TEXINFO		8192

#define	MAX_MAP_AREAS		256		// the same in qbism
#define	MAX_MAP_AREAPORTALS	1024	// the same in qbism
#define	MAX_MAP_PLANES		65536
#define	MAX_MAP_NODES		65536
#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
#define	MAX_MAP_LEAFFACES	65536
#define	MAX_MAP_LEAFBRUSHES 65536
#define	MAX_MAP_PORTALS		65536
#define	MAX_MAP_EDGES		128000
#define	MAX_MAP_SURFEDGES	256000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_VISIBILITY	0x100000


//
// QBISM EXTENDED QUAKE2 BSP FORMAT
//
#define QBISM_IDENT				 ('Q' | ('B' << 8) | ('S' << 16) | ('P' << 24))

#define MAX_MAP_MODELS_QBSP      131072
#define MAX_MAP_BRUSHES_QBSP     1048576
#define MAX_MAP_ENTITIES_QBSP    131072
#define MAX_MAP_ENTSTRING_QBSP   13631488
#define MAX_MAP_TEXINFO_QBSP     1048576
#define MAX_MAP_PLANES_QBSP      1048576
#define MAX_MAP_NODES_QBSP       1048576
#define MAX_MAP_LEAFS_QBSP       1048576
#define MAX_MAP_VERTS_QBSP       4194304
#define MAX_MAP_FACES_QBSP       1048576
#define MAX_MAP_LEAFFACES_QBSP   1048576
#define MAX_MAP_LEAFBRUSHES_QBSP 1048576
#define MAX_MAP_EDGES_QBSP       1048576
#define MAX_MAP_BRUSHSIDES_QBSP  4194304
#define MAX_MAP_PORTALS_QBSP     1048576
#define MAX_MAP_SURFEDGES_QBSP   4194304
#define MAX_MAP_LIGHTING_QBSP    54525952
#define MAX_MAP_VISIBILITY_QBSP  0x8000000

//=============================================================================

// BSPX extensions to BSP
#define BSPX_IDENT		(('X'<<24)+('P'<<16)+('S'<<8)+'B') // little-endian "BSPX"

typedef struct
{
	uint16_t	width;
	uint16_t	height;
	int32_t		lightofs;		// start of numstyles (from face struct) * (width * height) samples
	float	vecs[2][4];		// this is a world -> lightmap space transformation matrix
} bspx_decoupledlm_t;

typedef struct
{
	char	name[24];	// up to 23 chars, zero-padded
	int32_t		fileofs;	// from file start
	int32_t		filelen;
} bspx_lump_t;

typedef struct
{
	int32_t ident;		// BSPX_IDENT
	int32_t numlumps;	// bspx_lump_t[numlumps]
} bspx_header_t;

//=============================================================================

typedef struct
{
	int32_t		fileofs, filelen;
} lump_t;

// standard lumps
#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16
#define	LUMP_AREAS			17
#define	LUMP_AREAPORTALS	18
#define	HEADER_LUMPS		19

typedef struct
{
	int32_t			ident;
	int32_t			version;
	lump_t		lumps[HEADER_LUMPS];
} dbsp_header_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int32_t		headnode;
	int32_t		firstface, numfaces;	// inlineModels just draw faces
										// without walking the bsp tree
} dbsp_model_t;


typedef struct
{
	float	point[3];
} dbsp_vertex_t;


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are always opposites

typedef struct
{
	float	normal[3];
	float	dist;
	int32_t	type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dbsp_plane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_NONE			-1
#define	CONTENTS_EMPTY			0		// air
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4		// drawn but not solid
#define	CONTENTS_LAVA			8		// liquid volume
#define	CONTENTS_SLIME			16		// liquid volume
#define	CONTENTS_WATER			32		// liquid volume
#define	CONTENTS_MIST			64		// drawn but not solid
#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL		0x8000	// func_areaportal

#define	CONTENTS_PLAYERCLIP		0x10000 // everything but players can pass through
#define	CONTENTS_MONSTERCLIP	0x20000 // everything but actors can pass through

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000	// can walk through but shots are blocked
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000	// player can climb it
#define	CONTENTS_PLAYER			0x40000000	// player, should never be on a brush, only in game

#define	SURF_LIGHT		0x1		// value will hold the light strength
#define	SURF_SLICK		0x2		// effects game physics
#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10	// 33% alpha
#define	SURF_TRANS66	0x20	// 66% alpha
#define	SURF_FLOWING	0x40	// scroll towards S coord
#define	SURF_NODRAW		0x80	// don't bother referencing the texture
#define	SURF_HINT		0x100	// make a primary bsp splitter
#define	SURF_SKIP		0x200	// completely ignore, allowing non-closed brushes

// ericw_tools additional surface flags
#define SURF_ALPHATEST	(1 << 25) // alpha test flag
#define SURF_N64_UV		(1 << 28) // N64 UV and surface flag hack
#define SURF_SCROLLX	(1 << 29) // slow x scroll
#define SURF_SCROLLY	(1 << 30) // slow y scroll
#define SURF_SCROLLFLIP	(1 << 31) // flip scroll directon


typedef struct
{
	int32_t		planenum;
	int32_t		children[2];	// negative numbers are -(leafs+1), not nodes
	int16_t		mins[3];		// for frustom culling
	int16_t		maxs[3];
	uint16_t	firstface;
	uint16_t	numfaces;	// counting both sides
} dbsp_node_t;

typedef struct
{
	int32_t			planenum;
	int32_t			children[2];	// negative numbers are -(leafs+1), not nodes
	float		mins[3];		// for frustom culling
	float		maxs[3];
	uint32_t firstface;
	uint32_t numfaces; // counting both sides
} dbsp_node_ext_t;	// QBISM BSP


typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int32_t		flags;			// miptex flags + overrides
	int32_t		value;			// light emission, etc
	char		texture[32];	// texture name (textures/*.tga)
	int32_t		nexttexinfo;	// for animations, -1 = end of chain
} dbsp_texinfo_t;


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	uint16_t	v[2];		// vertex numbers
} dbsp_edge_t;

typedef struct
{
	uint32_t v[2]; // vertex numbers
} dbsp_edge_ext_t; // QBISM BSP


#define	MAX_LIGHTMAPS_PER_SURFACE	4 // this is also max lightstyles for surf
typedef struct
{
	uint16_t	planenum;
	int16_t		side;

	int32_t		firstedge;		// we must support > 64k edges
	int16_t		numedges;
	int16_t		texinfo;

// lighting info
	byte		styles[MAX_LIGHTMAPS_PER_SURFACE];
	int32_t			lightofs;		// start of [numstyles*surfsize] samples
} dbsp_face_t;

typedef struct
{
	uint32_t		planenum;
	int32_t			side;

	int32_t			firstedge; // we must support > 64k edges
	int32_t			numedges;
	int32_t			texinfo;

	// lighting info
	byte		styles[MAX_LIGHTMAPS_PER_SURFACE];
	int32_t			lightofs; // start of [numstyles*surfsize] samples
} dbsp_face_ext_t; // QBISM BSP


typedef struct
{
	int32_t			contents;			// OR of all brushes (not needed?)

	int16_t			cluster;
	int16_t			area;

	int16_t			mins[3];			// for frustum culling
	int16_t			maxs[3];

	uint16_t	firstleafface;
	uint16_t	numleaffaces;

	uint16_t	firstleafbrush;
	uint16_t	numleafbrushes;
} dbsp_leaf_t;

typedef struct
{
	int32_t			contents; // OR of all brushes (not needed?)

	int32_t			cluster;
	int32_t			area;

	float			mins[3]; // for frustum culling
	float			maxs[3];

	uint32_t	firstleafface;
	uint32_t	numleaffaces;

	uint32_t	firstleafbrush;
	uint32_t	numleafbrushes;
} dbsp_leaf_ext_t; // QBISM BSP


typedef struct
{
	uint16_t	planenum;		// facing out of the leaf
	int16_t		texinfo;
} dbsp_brushside_t;

typedef struct
{
	uint32_t	planenum; // facing out of the leaf
	uint32_t				texinfo;
} dbsp_brushside_ext_t; // QBISM BSP

typedef struct
{
	int32_t			firstside;
	int32_t			numsides;
	int32_t			contents;
} dbrush_t;

//#define	ANGLE_UP	-1 // braxi -- unused
//#define	ANGLE_DOWN	-2 // braxi -- unused


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
#define	DVIS_NUM	2

typedef struct
{
	int32_t			numclusters;
	int32_t			bitofs[8][DVIS_NUM];	// bitofs[numclusters][DVIS_NUM]
} dbsp_vis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct
{
	int32_t		portalnum;
	int32_t		otherarea;
} dbsp_areaportal_t;

typedef struct
{
	int32_t		numareaportals;
	int32_t		firstareaportal;
} dbsp_area_t;
