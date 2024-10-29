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
	LIGHTMAP_LIGHTMAP = 0,		// a value greater or equal to zero means index to r_world->lightmaps[]
	LIGHTMAP_NONE = -1,			// material does not reference lightmap
	LIGHTMAP_WHITEIMAGE = -2,	// use white texture (fullbright)
	LIGHTMAP_BY_VERTEX = -3,	// use vertex colors for lighting
	LIGHTMAP_2D = -4,			// ???

	LIGHTMAP_MAX = MAX_WORLD_LIGHTMAPS
} worldLightMap_t;

typedef struct
{
	vec3_t		origin;
	vec3_t		normal;
	vec3_t		color;
} worldSurf_Billboard_t;

typedef struct
{
	cplane_t	plane;
} worldSurf_Face_t;

typedef struct
{
	worldSurfaceType_t	surfaceType;

	// index to r_world->materials[]
	int			material_id;

	// index to r_world->lightmaps[], see worldLightMap_t for details
	int			lightmap_id;

	// currently unused
	int			fogvolume_id;

	// culling information
	vec3_t		mins, maxs;
	float		radius;

	// index to r_world->drawVerts[]
	int			firstVert;

	// number of vertexes
	int			numVerts;

	// number of draw indexes
	int			numIndexes;

	// dynmicaly allocated draw indexes pointing to r_world->drawVerts[]
	int			*drawIndexes;

	// additional worldSurf_*t data depending on surfaceType
	void		*data; 

} worldSurface_t; // new msurface_t



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
	int			hunksize;

	vec3_t		sunDirection;

	bsp_surfinfo_t* materials;
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
	int			numInlineModels;

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

extern renderWorld_t *r_world;

//
// r_bsp_load.c
//

void R_FreeWorld();
void R_LoadWorld(const char *bsp_name);



//
// r_bsp_draw.c
//

qboolean R_DrawQ3World();

#endif /*_R_BSP_H_*/