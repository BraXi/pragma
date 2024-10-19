/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _R_BSP_H_
#define _R_BSP_H_

#pragma once

#define	PLANE_NON_AXIAL	3
#define PlaneTypeForNormal(x) (x[0] == 1.0 ? PLANE_X : (x[1] == 1.0 ? PLANE_Y : (x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL) ) )

typedef enum
{
	WORLDSURF_BAD,				// none cannot be bad

	WORLDSURF_SKIP,				// ignore / hidden
	WORLDSURF_FACE,				// brush face
	WORLDSURF_MESH,				// misc_model
	WORLDSURF_BILLBOARD,		// billboard / flare
	WORLDSURF_PATCH,			// curve patch

	WORLDSURF_TYPES,
} worldSurfaceType_t;

typedef enum
{
	LIGHTMAP_LIGHTMAP = 0,		// (>= 0) means surface is properly light mapped
	LIGHTMAP_NONE = -1,			// material does not reference lightmap
	LIGHTMAP_WHITEIMAGE = -2,	// surface is fullbright
	LIGHTMAP_BY_VERTEX = -3,	// per vertex lighting
	LIGHTMAP_2D = -4			// material for 2D rendering
} worldLightMap_t;

typedef struct
{
	int					material;
	int					fogVolumeIndex;

	worldSurfaceType_t	surfaceType;
	void* data; // any of worldSurf_*t
} worldSurface_t; // new msurface_t

typedef struct
{
	// culling information
	vec3_t		mins, maxs;
	float		radius;

	// >= 0 : index to lightmaps, < 0 : vertex lit
	int			lightmap;

	// triangle definitions
	int			firstIndex;
	int			numIndexes;

	int			firstVert;
	int			numVerts;

	int* indices;
} worldSurf_Mesh_t;

typedef struct
{
	vec3_t		origin;
	vec3_t		normal;
	vec3_t		color;
} worldSurf_Billboard_t;


typedef struct
{
	cplane_t	plane;

	// >= 0 : index to lightmaps, < 0 : vertex lit
	int			lightmap;

	// triangle definitions (no normals at verts)
	int			firstVert;
	int			numVerts;
	int			firstIndex;
	int			numIndexes;

	int* indices;
} worldSurf_Face_t;


typedef struct worldNode_s
{
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	vec3_t		mins, maxs;		// for bounding box culling
	struct mnode_s* parent;

	// node specific
	cplane_t* plane;
	struct worldNode_s* children[2];

	// leaf specific
	int			cluster;
	int			area;

	worldSurface_t** firstmarksurface;
	int			nummarksurfaces;
} worldNode_t;

typedef struct
{
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	float		color[4];
} worldDrawVert_t;

typedef struct renderWorld_s
{
	char		name[MAX_QPATH]; // without .bsp and path
	qboolean	bLoaded;

	q3bsp_surfinfo_t* materials;
	int			numMaterials;

	worldDrawVert_t* drawVerts;
	int			numDrawVerts;

	GLuint* drawIndexes;
	int			numDrawIndexes;

	cplane_t* planes;
	int			numPlanes;

	worldSurface_t* surfaces; // New msurface_t !
	int			numSurfaces;

	worldSurface_t** marksurfaces; // New msurface_t !
	int			numMarkSurfaces;

	worldNode_t* nodes; // New mnode_t !
	int			numNodes;
	int			numDecisionNodes;

	int			numClusters;
	int			clusterBytes;

	bmodel_t* inlineModels;
	int			numBrushModels;

	byte* vis;
	byte* novis;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	byte* lightGridData;

	qboolean	bExternalLightmaps;
	int			numLightmaps;
	image_t* lightmaps[MAX_WORLD_LIGHTMAPS];

	unsigned int vbo_verts;
#if 0
	unsigned int vbo_indexes;
#endif
} renderWorld_t;


//
// r_bsp_load.c
//

void R_LoadWorld(model_t* mod, void* buffer);
void R_FreeWorld();


//
// r_bsp_draw.c
//

qboolean R_DrawQ3World();

#endif /*_R_BSP_H_*/