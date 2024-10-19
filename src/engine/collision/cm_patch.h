/*
===========================================================================
PRAGMA Copyright (C) 2023-2024 BraXi.

This file is part of PRAGMA source code, a free open source game engine.
Quake II and Quake III Arena source code are Copyright (C) Id Software, Inc.
See the attached GNU General Public License v2 for more details.

Note: CModel implementation is mostly a direct copy of the one found in Quake III.
===========================================================================
*/

/*

cmodel_patch.c

This file does not reference any globals, and has these entry points:

	CM_ClearLevelPatches
	CM_GeneratePatchCollide
	CM_TraceThroughPatchCollide
	CM_PositionTestInPatchCollide
	CM_DrawDebugSurface


Issues for collision against curved surfaces:

	Surface edges need to be handled differently than surface planes
	Plane expansion causes raw surfaces to expand past expanded bounding box
	Position test of a volume against a surface is tricky.
	Position test of a point against a surface is not well defined, because the surface has no volume.
	Tracing leading edge points instead of volumes?
	Position test by tracing corner to corner? (8*7 traces -- ouch)
	coplanar edges
	triangulated patches
	degenerate patches

	- endcaps
	-  degenerate

WARNING: This may misbehave with meshes that have rows or columns that only
degenerate a few triangles. Completely degenerate rows and columns are handled properly.
*/

#ifndef _PRAGMA_CMODEL_PATCH_H_
#define _PRAGMA_CMODEL_PATCH_H_
#pragma once

#define	MAX_FACETS			1024
#define	MAX_PATCH_PLANES	2048
#define	MAX_GRID_SIZE		129

#define	SUBDIVIDE_DISTANCE	16	//4	// never more than this units away from curve
#define	PLANE_TRI_EPSILON	0.1
#define	WRAP_POINT_EPSILON	0.1
#define	POINT_EPSILON		0.1

#define	NORMAL_EPSILON		0.0001
#define	DIST_EPSILON		0.02

typedef struct 
{
	float	plane[4];
	int		signbits;		// signx + (signy<<1) + (signz<<2), used as lookup during collision
} patchPlane_t;

typedef struct 
{
	int			surfacePlane;
	int			numBorders;		// 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
	int			borderPlanes[4+6+16];
	int			borderInward[4+6+16];
	qboolean	borderNoAdjust[4+6+16];
} facet_t;

typedef struct patchCollide_s 
{
	vec3_t			bounds[2];
	int				numPlanes;			// surface planes plus edge planes
	patchPlane_t	*planes;
	int				numFacets;
	facet_t			*facets;
} patchCollide_t;

typedef struct 
{
	int			width;
	int			height;
	qboolean	wrapWidth;
	qboolean	wrapHeight;
	vec3_t		points[MAX_GRID_SIZE][MAX_GRID_SIZE];	// [width][height]
} cGrid_t;

struct patchCollide_s *CM_GeneratePatchCollide( int width, int height, vec3_t *points );

#endif /*_PRAGMA_CMODEL_PATCH_H_*/
