/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
==============================================================================

   .BSP file format

==============================================================================
*/


#ifndef _PRAGMA_BSP_H_
#define _PRAGMA_BSP_H_

#define BSP_IDENT				(('P'<<24)+('S'<<16)+('B'<<8)+'B')
#define BSP_VERSION			1


// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define	MAX_WORLD_MODELS		1024
#define	MAX_WORLD_BRUSHES		32768
#define	MAX_WORLD_ENTITIES		2048
#define	MAX_WORLD_ENTSTRING		262144
#define	MAX_WORLD_SHADERS		1024

#define	MAX_WORLD_AREAS			256	// MAX_WORLD_AREA_BYTES in q_shared must match!
#define	MAX_WORLD_FOGS			256
#define	MAX_WORLD_PLANES		131072
#define	MAX_WORLD_NODES			131072
#define	MAX_WORLD_BRUSHSIDES	131072
#define	MAX_WORLD_LEAFS			131072
#define	MAX_WORLD_LEAFFACES		131072
#define	MAX_WORLD_LEAFBRUSHES	262144
#define	MAX_WORLD_PORTALS		131072
#define	MAX_WORLD_LIGHTING		8388608
#define	MAX_WORLD_LIGHTGRID		8388608
#define	MAX_WORLD_VISIBILITY	8388608

#define	MAX_WORLD_DRAW_SURFS	131072
#define	MAX_WORLD_DRAW_VERTS	524288
#define	MAX_WORLD_DRAW_INDEXES	524288

#define MAX_WORLD_LIGHTMAPS		256

#define	WORLD_LIGHTMAP_WIDTH	1024
#define WORLD_LIGHTMAP_HEIGHT	WORLD_LIGHTMAP_WIDTH
#define WORLD_LIGHTMAP_SIZE		WORLD_LIGHTMAP_WIDTH

#define WORLD_MAX_COORD			( 128*1024 )
#define WORLD_MIN_COORD			( -128*1024 )
#define WORLD_SIZE				( WORLD_MAX_COORD - WORLD_MIN_COORD )

#define MAX_PATCH_SIZE			32
#define MAX_FACE_POINTS			128 // limit from q3map light.c, not quake3 engine

//=============================================================================

typedef struct 
{
	int32_t	fileofs, filelen;
} lump_t;

#define	BSPLUMP_ENTITIES		0
#define	BSPLUMP_MATERIALS		1
#define	BSPLUMP_PLANES			2
#define	BSPLUMP_NODES			3
#define	BSPLUMP_LEAFS			4
#define	BSPLUMP_LEAFSURFACES	5
#define	BSPLUMP_LEAFBRUSHES		6
#define	BSPLUMP_MODELS			7
#define	BSPLUMP_BRUSHES			8
#define	BSPLUMP_BRUSHSIDES		9
#define	BSPLUMP_DRAWVERTS		10
#define	BSPLUMP_DRAWINDEXES		11
#define	BSPLUMP_FOGS			12
#define	BSPLUMP_SURFACES		13
#define	BSPLUMP_LIGHTMAPS		14
#define	BSPLUMP_LIGHTGRID		15
#define	BSPLUMP_VISIBILITY		16
#define	BSP_LUMPS				17

typedef struct 
{
	int32_t		ident;
	int32_t		version;
	lump_t		lumps[BSP_LUMPS];
} bsp_header_t;


typedef struct 
{
	float		mins[3], maxs[3];
	int32_t		firstSurface, numSurfaces;
	int32_t		firstBrush, numBrushes;
} bsp_model_t;

typedef struct 
{
	char		name[MAX_QPATH];
	int32_t		surfaceFlags;
	int32_t		contentFlags;
} bsp_surfinfo_t;

// planes x^1 is allways the opposite of plane x

typedef struct 
{
	float		normal[3];
	float		dist;
} bsp_plane_t;

typedef struct 
{
	int32_t		planeNum;
	int32_t		children[2];	// negative numbers are -(leafs+1), not nodes
	int32_t		mins[3];		// for frustom culling
	int32_t		maxs[3];
} bsp_node_t;

typedef struct 
{
	int32_t		cluster;			// -1 = opaque cluster (do I still store these?)
	int32_t		area;

	int32_t		mins[3];			// for frustum culling
	int32_t		maxs[3];

	int32_t		firstLeafSurface;
	int32_t		numLeafSurfaces;

	int32_t		firstLeafBrush;
	int32_t		numLeafBrushes;
} bsp_leaf_t;

typedef struct 
{
	int32_t		planeNum;			// positive plane side faces out of the leaf
	int32_t		materialNum;
} bsp_brushside_t;

typedef struct 
{
	int32_t		firstSide;
	int32_t		numSides;
	int32_t		materialNum;		// the shader that determines the contents flags
} bsp_brush_t;

typedef struct 
{
	char		material[MAX_QPATH];
	int32_t		brushNum;
	int32_t		visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} bsp_fog_t;

typedef struct 
{
	vec3_t		xyz;
	float		st[2]; // diffuse texcoords
	float		lightmap[2]; // lightmap texcoords
	vec3_t		normal;
	byte		color[4]; // must extract first
} bsp_drawvert_t;

typedef enum 
{
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE,
	MST_FOLIAGE
} bsp_surfacetype_t;

typedef struct 
{
	int32_t		materialNum;
	int32_t		fogNum;
	int32_t		surfaceType; // bsp_surfacetype_t

	int32_t		firstVert;
	int32_t		numVerts;

	int32_t		firstIndex;
	int32_t		numIndexes;

	int32_t		lightmapNum;
	int32_t		lightmapX, lightmapY;
	int32_t		lightmapWidth, lightmapHeight;

	vec3_t		lightmapOrigin;
	vec3_t		lightmapVecs[3];	// for patches, [0] and [1] are lodbounds

	int32_t		patchWidth;
	int32_t		patchHeight;
} bsp_surface_t;


//
// Content flags
//
#define	CONTENTS_NODE			-1			// differentiate node from leafs

#define	CONTENTS_SOLID		1			// an eye is never valid in a solid
#define	CONTENTS_LAVA			8			// treat as lava
#define	CONTENTS_SLIME		16			// treat as slime
#define	CONTENTS_WATER		32			// treat as water
#define	CONTENTS_FOG			64			// fog volume

#define CONTENTS_NOTTEAM1		0x0080
#define CONTENTS_NOTTEAM2		0x0100
#define CONTENTS_NOBOTCLIP	0x0200		//

#define	CONTENTS_AREAPORTAL	0x8000

#define	CONTENTS_PLAYERCLIP	0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

#define	CONTENTS_TELEPORTER	0x40000		// bot specific contents type
#define	CONTENTS_JUMPPAD		0x80000		// bot specific contents type
#define CONTENTS_CLUSTERPORTAL 0x100000	// bot specific contents type
#define CONTENTS_DONOTENTER	0x200000	// bot specific contents type
#define CONTENTS_BOTCLIP		0x400000	// bot specific contents type
#define CONTENTS_MOVER		0x800000	// bot specific contents type

#define	CONTENTS_ORIGIN		0x1000000	// removed before bsping an entity

#define	CONTENTS_BODY			0x2000000	// should never be on a brush, only in game
#define	CONTENTS_CORPSE		0x4000000
#define	CONTENTS_DETAIL		0x8000000	// brushes not used for the bsp
#define	CONTENTS_STRUCTURAL	0x10000000	// brushes used for the bsp
#define	CONTENTS_TRANSLUCENT	0x20000000	// don't consume surface fragments inside
#define	CONTENTS_TRIGGER		0x40000000
#define	CONTENTS_NODROP		0x80000000	// don't leave bodies or items (death fog, lava)



//
// Surface flags
//
#define	SURF_NODAMAGE			0x1			// never give falling damage
#define	SURF_SLICK				0x2			// effects game physics
#define	SURF_SKY				0x4			// lighting from environment map
#define	SURF_LADDER				0x8			// effects game physics
#define	SURF_NOIMPACT			0x10		// don't make missile explosions
#define	SURF_NOMARKS			0x20		// don't leave missile marks
#define	SURF_FLESH				0x40		// make flesh sounds and effects
#define	SURF_NODRAW				0x80		// don't generate a drawsurface at all
#define	SURF_HINT				0x100		// make a primary bsp splitter
#define	SURF_SKIP				0x200		// completely ignore, allowing non-closed brushes
#define	SURF_NOLIGHTMAP			0x400		// surface doesn't need a lightmap
#define	SURF_POINTLIGHT			0x800		// generate lighting info at vertexes
#define	SURF_METALSTEPS			0x1000		// clanking footsteps
#define	SURF_NOSTEPS			0x2000		// no footstep sounds
#define	SURF_NONSOLID			0x4000		// don't collide against curves with this set
#define SURF_LIGHTFILTER		0x8000		// act as a light filter during q3map -light
#define	SURF_ALPHASHADOW		0x10000		// do per-pixel light shadow casting in q3map
#define	SURF_NODLIGHT			0x20000		// don't dlight even if solid (solid lava, skies)
#define SURF_DUST				0x40000		// leave a dust trail when walking on this surface


#endif /*_PRAGMA_BSP_H_*/