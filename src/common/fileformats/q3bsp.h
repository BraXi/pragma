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

  Quake2 BSP
  QBISM BSP

  BSPX DECOUPLED_LM

  CONTENT FLAGS
  SURFACE FLAGS

==============================================================================
*/


#ifndef _PRAGMA_Q3BSP_H_
#define _PRAGMA_Q3BSP_H_

#define Q3BSP_IDENT	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
#define Q3BSP_VERSION	46


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

#define	Q3BSP_LIGHTMAP_WIDTH	128
#define	Q3BSP_LIGHTMAP_HEIGHT	128
#define Q3BSP_LIGHTMAP_SIZE		128
#define Q3BSP_MAX_LIGHTMAPS		256

#define Q3BSP_MAX_WORLD_COORD	( 128*1024 )
#define Q3BSP_MIN_WORLD_COORD	( -128*1024 )
#define Q3BSP_WORLD_SIZE		( Q3BSP_MAX_WORLD_COORD - Q3BSP_MIN_WORLD_COORD )

//=============================================================================

#ifndef _PRAGMA_BSP_H_
typedef struct 
{
	int32_t	fileofs, filelen;
} lump_t;
#endif

#define	Q3LUMP_ENTITIES			0
#define	Q3LUMP_SHADERS			1
#define	Q3LUMP_PLANES			2
#define	Q3LUMP_NODES			3
#define	Q3LUMP_LEAFS			4
#define	Q3LUMP_LEAFSURFACES		5
#define	Q3LUMP_LEAFBRUSHES		6
#define	Q3LUMP_MODELS			7
#define	Q3LUMP_BRUSHES			8
#define	Q3LUMP_BRUSHSIDES		9
#define	Q3LUMP_DRAWVERTS		10
#define	Q3LUMP_DRAWINDEXES		11
#define	Q3LUMP_FOGS				12
#define	Q3LUMP_SURFACES			13
#define	Q3LUMP_LIGHTMAPS		14
#define	Q3LUMP_LIGHTGRID		15
#define	Q3LUMP_VISIBILITY		16
#define	Q3_LUMPS				17

typedef struct 
{
	int32_t		ident;
	int32_t		version;

	lump_t		lumps[Q3_LUMPS];
} q3bsp_header_t;


typedef struct 
{
	float		mins[3], maxs[3];
	int32_t		firstSurface, numSurfaces;
	int32_t		firstBrush, numBrushes;
} q3bsp_model_t;

typedef struct 
{
	char		shader[MAX_QPATH];
	int32_t		surfaceFlags;
	int32_t		contentFlags;
} q3bsp_shader_t;

// planes x^1 is allways the opposite of plane x

typedef struct 
{
	float		normal[3];
	float		dist;
} q3bsp_plane_t;

typedef struct 
{
	int32_t		planeNum;
	int32_t		children[2];	// negative numbers are -(leafs+1), not nodes
	int32_t		mins[3];		// for frustom culling
	int32_t		maxs[3];
} q3bsp_node_t;

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
} q3bsp_leaf_t;

typedef struct 
{
	int32_t		planeNum;			// positive plane side faces out of the leaf
	int32_t		shaderNum;
} q3bsp_brushside_t;

typedef struct 
{
	int32_t		firstSide;
	int32_t		numSides;
	int32_t		shaderNum;		// the shader that determines the contents flags
} q3bsp_brush_t;

typedef struct 
{
	char		shader[MAX_QPATH];
	int32_t		brushNum;
	int32_t		visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} q3bsp_fog_t;

typedef struct 
{
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
} q3bsp_drawVert_t;

typedef enum 
{
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE
} q3bsp_mapSurfaceType_t;

typedef struct 
{
	int32_t		shaderNum;
	int32_t		fogNum;
	int32_t		surfaceType;

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
} q3bsp_surface_t;

#endif /*_PRAGMA_Q3BSP_H_*/