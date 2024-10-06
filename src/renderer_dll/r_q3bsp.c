/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_q3bsp.c -- because q2bsp is fucking annoying

/*
notes:

cplane_t in q3 is the same
msurface_t = worldSurface_t
mnode_t = worldNode_t

misc_models in maps are turned into direct geometry by q3map == srfTriangles_t
*/

#include "r_local.h"

extern model_t* pLoadModel;
extern int modelFileLength;
extern byte* mod_base;
extern model_t r_inlineModels[MAX_WORLD_MODELS];

typedef enum 
{
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
	SF_MD4,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct worldSurface_s // new msurface_t
{
	int					viewCount;	// if == tr.viewCount, already added
	struct shader_s		*material;
	int					fogIndex;

	surfaceType_t		*data;			// any of srf*_t
} worldSurface_t;

typedef struct srfGridMesh_s 
{
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits[2];

	// culling information
	vec3_t			meshBounds[2];
	vec3_t			localOrigin;
	float			meshRadius;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;
	q3bsp_drawVert_t	verts[1];		// variable sized
} srfGridMesh_t;

typedef struct
{
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits[2];

	// culling information
	vec3_t			bounds[2];
	vec3_t			localOrigin;
	float			radius;

	// triangle definitions
	int				numIndexes;
	int				*indexes;

	int				numVerts;
	q3bsp_drawVert_t* verts;
} srfTriangles_t;

typedef struct srfFlare_s 
{
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
} srfFlare_t;

#define	VERTEXSIZE	8 // xyz normal uv
typedef struct 
{
	surfaceType_t	surfaceType;
	cplane_t	plane;

	// dynamic lighting information
	int			dlightBits[2];

	// triangle definitions (no normals at points)
	int			numPoints;
	int			numIndices;
	int			ofsIndices;
	float		points[1][VERTEXSIZE];	// variable sized
										// there is a variable length list of indices here also
} srfSurfaceFace_t;

#define	CONTENTS_NODE		-1
typedef struct worldNode_s // new mnode
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

typedef struct renderWorld_s
{
	char		name[MAX_QPATH]; // without .bsp and path
	qboolean	bLoaded;

	q3bsp_shader_t* shaders;
	int			numShaders;

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

	bmodel_t	*bmodels;
	int			numBrushModels;

	byte		*vis;
	byte		*novis;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	byte		*lightGridData;

	int			numLightmaps;
	image_t		*lightmaps[Q3BSP_MAX_LIGHTMAPS];
} renderWorld_t;

static renderWorld_t world;

#define	PLANE_NON_AXIAL	3
#define PlaneTypeForNormal(x) (x[0] == 1.0 ? PLANE_X : (x[1] == 1.0 ? PLANE_Y : (x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL) ) )

/*
=================
CheckLumpSize
=================
*/
static void CheckLumpSize(const lump_t* lump, size_t element_size, const char* func_name)
{
	if (lump->filelen % element_size)
	{
		ri.Error(ERR_DROP, "%s: funny lump size in %s", func_name, world.name);
	}
}

/*
=================
R_LoadShaders
=================
*/
static void R_LoadShaders(const lump_t* lump)
{
	int		i, count;
	q3bsp_shader_t *in, *out;

	in = (void*)(mod_base + lump->fileofs);
	CheckLumpSize(lump, sizeof(*in), __FUNCTION__);

	count = lump->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	world.shaders = out;
	world.numShaders = count;

	memcpy(out, in, count * sizeof(*out));

	for (i = 0; i < count; i++) 
	{
		out[i].surfaceFlags = LittleLong(out[i].surfaceFlags);
		out[i].contentFlags = LittleLong(out[i].contentFlags);
	}
}

/*
===============
R_ColorShiftLightingBytes
===============
*/
static void R_ColorShiftLightingBytes(byte in[4], byte out[4])
{
	int	r, g, b;
	int max;

	r = in[0];
	g = in[1];
	b = in[2];

	// normalize by color instead of saturating to white
	if ((r | g | b) > 255) 
	{
		max = r > g ? r : g;
		max = max > b ? max : b;
		r = r * 255 / max;
		g = g * 255 / max;
		b = b * 255 / max;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = in[3];
}

/*
===============
R_LoadLightmaps
===============
*/
static void R_LoadLightmaps(const lump_t* lump)
{
	byte* buf, * buf_p;
	int			len;
	byte		pixelData[Q3BSP_LIGHTMAP_WIDTH * Q3BSP_LIGHTMAP_HEIGHT * 4];
	int			i, j;
	int			numPixels;

	len = lump->filelen;
	if (!len)
	{
		world.numLightmaps = 0;
		return; // no lighting data
	}

	buf = mod_base + lump->fileofs;

	numPixels = Q3BSP_LIGHTMAP_WIDTH * Q3BSP_LIGHTMAP_HEIGHT; // Q3BSP_LIGHTMAP_SIZE * Q3BSP_LIGHTMAP_SIZE

	// create all the lightmaps
	world.numLightmaps = len / (numPixels * 3);
	for (i = 0; i < world.numLightmaps; i++)
	{
		// expand the 24 bit on-disk to 32 bit
		buf_p = buf + i * numPixels * 3;

		for (j = 0; j < numPixels; j++)
		{
			R_ColorShiftLightingBytes(&buf_p[j * 3], &pixelData[j * 4]);
			pixelData[j * 4 + 3] = 255;
		}

		world.lightmaps[i] = R_LoadTexture(va("$lightmap_%d", i), (byte*)pixelData, Q3BSP_LIGHTMAP_WIDTH, Q3BSP_LIGHTMAP_HEIGHT, it_texture, 32);

		// R_LoadTexture leaves texture bound, so make sure it clamps
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		R_BindTexture(0);
	}
}

/*
=================
R_LoadPlanes
=================
*/
static void R_LoadPlanes(const lump_t* lump)
{
	q3bsp_plane_t* in;
	int			i, j;
	int			count;
	int			bits;
	cplane_t	*out;

	in = (void*)(mod_base + lump->fileofs);
	CheckLumpSize(lump, sizeof(*in), __FUNCTION__);

	count = lump->filelen / sizeof(*in);
	out = Hunk_Alloc(count * 2 * sizeof(*out));

	world.planes = out;
	world.numPlanes = count;

	for (i = 0; i < count; i++, in++, out++) 
	{
		bits = 0;
		for (j = 0; j < 3; j++) 
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0) 
			{
				bits |= 1 << j;
			}
		}

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal(out->normal);
		out->signbits = bits;
	}
}

/*
=================
R_LoadFogs
=================
*/
static void R_LoadFogs(const lump_t* fogLump, const lump_t* brushLump, const lump_t* brushSidesLump)
{
}

/*
===============
ParseMesh
===============
*/
static void ParseMesh(q3bsp_surface_t* ds, q3bsp_drawVert_t* verts, worldSurface_t* surf) 
{
	srfGridMesh_t* grid;
	int				i, j;
	int				width, height, numPoints;
	q3bsp_drawVert_t points[MAX_PATCH_SIZE * MAX_PATCH_SIZE];
	int				lightmapNum;
	vec3_t			bounds[2];
	vec3_t			tmpVec;
	static surfaceType_t	skipData = SF_SKIP;

	if (1) // FIXME: Q3
		return;

	lightmapNum = LittleLong(ds->lightmapNum);

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader value
	surf->material = NULL; // = ShaderForShaderNum(ds->shaderNum, lightmapNum);


	// we may have a nodraw surface, because they might still need to be around for movement clipping
	i = LittleLong(ds->shaderNum);
	if (i >= world.numShaders || i < 0)
	{
		ri.Error(ERR_DROP, "Wrong material index %i\n", i);
	}

	if (world.shaders[i].surfaceFlags & Q3SURF_NODRAW) 
	{
		surf->data = &skipData;
		return;
	}

	width = LittleLong(ds->patchWidth);
	height = LittleLong(ds->patchHeight);

	verts += LittleLong(ds->firstVert);
	numPoints = width * height;
	for (i = 0; i < numPoints; i++) 
	{
		for (j = 0; j < 3; j++) 
		{
			points[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			points[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		for (j = 0; j < 2; j++) 
		{
			points[i].st[j] = LittleFloat(verts[i].st[j]);
			points[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}
		R_ColorShiftLightingBytes(verts[i].color, points[i].color);
	}

	// pre-tesseleate
	//grid = R_SubdividePatchToGrid(width, height, points); // FIXME: Q3
	surf->data = (surfaceType_t*)grid;

	// copy the level of detail origin, which is the center
	// of the group of all curves that must subdivide the same
	// to avoid cracking
	for (i = 0; i < 3; i++) 
	{
		bounds[0][i] = LittleFloat(ds->lightmapVecs[0][i]);
		bounds[1][i] = LittleFloat(ds->lightmapVecs[1][i]);
	}

	VectorAdd(bounds[0], bounds[1], bounds[1]);
	VectorScale(bounds[1], 0.5f, grid->lodOrigin);
	VectorSubtract(bounds[0], grid->lodOrigin, tmpVec);
	grid->lodRadius = VectorLength(tmpVec);
}

/*
=================
ParseTriSurf
=================
*/
static void ParseTriSurf(q3bsp_surface_t* ds, q3bsp_drawVert_t* verts, worldSurface_t* surf, int* indexes) 
{
	srfTriangles_t* tri;
	int				i, j;
	int				numVerts, numIndexes;

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader
	// FIXME: Q3
	surf->material = NULL; // ShaderForShaderNum(ds->shaderNum, LIGHTMAP_BY_VERTEX);

	numVerts = LittleLong(ds->numVerts);
	numIndexes = LittleLong(ds->numIndexes);

	tri = Hunk_Alloc(sizeof(*tri) + numVerts * sizeof(tri->verts[0]) + numIndexes * sizeof(tri->indexes[0]));
	tri->surfaceType = SF_TRIANGLES;
	tri->numVerts = numVerts;
	tri->numIndexes = numIndexes;
	tri->verts = (q3bsp_drawVert_t*)(tri + 1);
	tri->indexes = (int*)(tri->verts + tri->numVerts);

	surf->data = (surfaceType_t*)tri;

	// copy vertexes
	ClearBounds(tri->bounds[0], tri->bounds[1]);
	verts += LittleLong(ds->firstVert);
	for (i = 0; i < numVerts; i++) 
	{
		for (j = 0; j < 3; j++) 
		{
			tri->verts[i].xyz[j] = LittleFloat(verts[i].xyz[j]);
			tri->verts[i].normal[j] = LittleFloat(verts[i].normal[j]);
		}
		AddPointToBounds(tri->verts[i].xyz, tri->bounds[0], tri->bounds[1]);
		for (j = 0; j < 2; j++) 
		{
			tri->verts[i].st[j] = LittleFloat(verts[i].st[j]);
			tri->verts[i].lightmap[j] = LittleFloat(verts[i].lightmap[j]);
		}

		R_ColorShiftLightingBytes(verts[i].color, tri->verts[i].color);
	}

	// copy indexes
	indexes += LittleLong(ds->firstIndex);
	for (i = 0; i < numIndexes; i++) 
	{
		tri->indexes[i] = LittleLong(indexes[i]);
		if (tri->indexes[i] < 0 || tri->indexes[i] >= numVerts) 
		{
			ri.Error(ERR_DROP, "Bad index in triangle surface");
		}
	}
}

/*
===============
SetPlaneSignbits
===============
*/
static void SetPlaneSignbits(cplane_t* out) 
{
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j = 0; j < 3; j++) 
	{
		if (out->normal[j] < 0) 
		{
			bits |= 1 << j;
		}
	}
	out->signbits = bits;
}

/*
===============
ParseFace
===============
*/
static void ParseFace(q3bsp_surface_t* ds, q3bsp_drawVert_t* verts, worldSurface_t* surf, int* indexes) 
{
	int			i, j;
	srfSurfaceFace_t* cv;
	int			numPoints, numIndexes;
	int			lightmapNum;
	int			sfaceSize, ofsIndexes;

	lightmapNum = LittleLong(ds->lightmapNum);

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader value
	// FIXME: Q3
	surf->material = NULL; // ShaderForShaderNum(ds->shaderNum, lightmapNum);

	numPoints = LittleLong(ds->numVerts);
	if (numPoints > MAX_FACE_POINTS) 
	{
		ri.Printf(PRINT_ALL, "WARNING: MAX_FACE_POINTS exceeded: %i\n", numPoints);
		numPoints = MAX_FACE_POINTS;
		//surf->material = r_default_brush_material;
	}

	numIndexes = LittleLong(ds->numIndexes);

	// create the srfSurfaceFace_t
	sfaceSize = (int)&((srfSurfaceFace_t*)0)->points[numPoints];
	ofsIndexes = sfaceSize;
	sfaceSize += sizeof(int) * numIndexes;

	cv = Hunk_Alloc(sfaceSize);
	cv->surfaceType = SF_FACE;
	cv->numPoints = numPoints;
	cv->numIndices = numIndexes;
	cv->ofsIndices = ofsIndexes;

	verts += LittleLong(ds->firstVert);
	for (i = 0; i < numPoints; i++) {
		for (j = 0; j < 3; j++) {
			cv->points[i][j] = LittleFloat(verts[i].xyz[j]);
		}
		for (j = 0; j < 2; j++) {
			cv->points[i][3 + j] = LittleFloat(verts[i].st[j]);
			cv->points[i][5 + j] = LittleFloat(verts[i].lightmap[j]);
		}
		R_ColorShiftLightingBytes(verts[i].color, (byte*)&cv->points[i][7]);
	}

	indexes += LittleLong(ds->firstIndex);
	for (i = 0; i < numIndexes; i++) {
		((int*)((byte*)cv + cv->ofsIndices))[i] = LittleLong(indexes[i]);
	}

	// take the plane information from the lightmap vector
	for (i = 0; i < 3; i++) {
		cv->plane.normal[i] = LittleFloat(ds->lightmapVecs[2][i]);
	}
	cv->plane.dist = DotProduct(cv->points[0], cv->plane.normal);
	SetPlaneSignbits(&cv->plane);
	cv->plane.type = PlaneTypeForNormal(cv->plane.normal);

	surf->data = (void*)cv;
}


/*
===============
ParseFlare
===============
*/
static void ParseFlare(q3bsp_surface_t* ds, q3bsp_drawVert_t* verts, worldSurface_t* surf, int* indexes) 
{
	srfFlare_t* flare;
	int				i;

	// get fog volume
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	// get shader
	// FIXME: Q3
	surf->material = NULL; // ShaderForShaderNum(ds->shaderNum, LIGHTMAP_BY_VERTEX);

	flare = Hunk_Alloc(sizeof(*flare));
	flare->surfaceType = SF_FLARE;

	surf->data = (void*)flare;

	for (i = 0; i < 3; i++) 
	{
		flare->origin[i] = LittleFloat(ds->lightmapOrigin[i]);
		flare->color[i] = LittleFloat(ds->lightmapVecs[0][i]);
		flare->normal[i] = LittleFloat(ds->lightmapVecs[2][i]);
	}
}


/*
=================
R_LoadSurfaces
=================
*/
static void R_LoadSurfaces(const lump_t* surfsLump, const lump_t* vertsLump, const lump_t* indexLump)
{
	q3bsp_surface_t* in;
	q3bsp_drawVert_t* dv;

	worldSurface_t* out;

	int* indexes;
	int			count;
	int			numFaces, numMeshes, numTriSurfs, numFlares;
	int			i;

	numFaces = 0;
	numMeshes = 0;
	numTriSurfs = 0;
	numFlares = 0;

	in = (void*)(mod_base + surfsLump->fileofs);
	CheckLumpSize(surfsLump, sizeof(*in), __FUNCTION__);
	count = surfsLump->filelen / sizeof(*in);
	
	dv = (void*)(mod_base + vertsLump->fileofs);
	CheckLumpSize(vertsLump, sizeof(*dv), __FUNCTION__);

	indexes = (void*)(mod_base + indexLump->fileofs);
	CheckLumpSize(indexLump, sizeof(*indexes), __FUNCTION__);
	out = Hunk_Alloc(count * sizeof(*out));

	world.surfaces = out;
	world.numSurfaces = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		switch (LittleLong(in->surfaceType))
		{
		case MST_PATCH:
			ParseMesh(in, dv, out);
			numMeshes++;
			break;

		case MST_TRIANGLE_SOUP:
			ParseTriSurf(in, dv, out, indexes);
			numTriSurfs++;
			break;

		case MST_PLANAR:
			ParseFace(in, dv, out, indexes);
			numFaces++;
			break;

		case MST_FLARE:
			ParseFlare(in, dv, out, indexes);
			numFlares++;
			break;
		default:
			ri.Error(ERR_DROP, "Bad surface type");
		}
	}

	ri.Printf(PRINT_ALL, "... loaded %d faces, %i meshes, %i trisurfs, %i flares\n", numFaces, numMeshes, numTriSurfs, numFlares);
}

/*
=================
R_LoadMarkSurfaces
=================
*/
static void R_LoadMarkSurfaces(const lump_t* lump)
{
	int		i, count;
	int32_t	*in;
	worldSurface_t** out;

	in = (void*)(mod_base + lump->fileofs);
	CheckLumpSize(lump, sizeof(*in), __FUNCTION__);

	count = lump->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	world.marksurfaces = out;
	world.numMarkSurfaces = count;

	for (i = 0; i < count; i++)
	{
		out[i] = world.surfaces + LittleLong(in[i]);
	}
}

/*
=================
R_SetParent
=================
*/
static void R_SetParent(mnode_t* node, mnode_t* parent)
{
	node->parent = parent;
	if (node->contents != CONTENTS_NODE)
		return;

	R_SetParent(node->children[0], node);
	R_SetParent(node->children[1], node);
}

/*
=================
R_LoadNodesAndLeafs
=================
*/
static void R_LoadNodesAndLeafs(const lump_t* nodeLump, const lump_t* leafLump)
{
	q3bsp_node_t* in;
	q3bsp_leaf_t* inLeaf;
	worldNode_t* out;
	int			i, j, p;
	int			numNodes, numLeafs;

	in = (void*)(mod_base + nodeLump->fileofs);
	CheckLumpSize(nodeLump, sizeof(q3bsp_node_t), __FUNCTION__);
	CheckLumpSize(leafLump, sizeof(q3bsp_leaf_t), __FUNCTION__);

	numNodes = nodeLump->filelen / sizeof(q3bsp_node_t);
	numLeafs = leafLump->filelen / sizeof(q3bsp_leaf_t);

	out = Hunk_Alloc((numNodes + numLeafs) * sizeof(*out));

	world.nodes = out;
	world.numNodes = numNodes + numLeafs;
	world.numDecisionNodes = numNodes;

	// load nodes
	for (i = 0; i < numNodes; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleLong(in->mins[j]);
			out->maxs[j] = LittleLong(in->maxs[j]);
		}

		p = LittleLong(in->planeNum);
		out->plane = world.planes + p;

		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j = 0; j < 2; j++)
		{
			p = LittleLong(in->children[j]);
			if (p >= 0)
				out->children[j] = world.nodes + p;
			else
				out->children[j] = world.nodes + numNodes + (-1 - p);
		}
	}

	// load leafs
	inLeaf = (void*)(mod_base + leafLump->fileofs);
	for (i = 0; i < numLeafs; i++, inLeaf++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->mins[j] = LittleLong(inLeaf->mins[j]);
			out->maxs[j] = LittleLong(inLeaf->maxs[j]);
		}

		out->cluster = LittleLong(inLeaf->cluster);
		out->area = LittleLong(inLeaf->area);

		if (out->cluster >= world.numClusters) 
		{
			world.numClusters = out->cluster + 1;
		}

		out->firstmarksurface = (world.marksurfaces + LittleLong(inLeaf->firstLeafSurface));
		out->nummarksurfaces = LittleLong(inLeaf->numLeafSurfaces);
	}

	// chain decendants
	R_SetParent(world.nodes, NULL);
}

/*
=================
R_LoadInlineModels
=================
*/
static void R_LoadInlineModels(const lump_t* lump)
{
	q3bsp_model_t* in;
	bmodel_t	*out;
	model_t		*model;
	int			i, j, count;

	in = (void*)(mod_base + lump->fileofs);
	CheckLumpSize(lump, sizeof(*in), __FUNCTION__);
	count = lump->filelen / sizeof(*in);

	world.bmodels = out = Hunk_Alloc(count * sizeof(*out));

	for (i = 0; i < count; i++, in++, out++) 
	{
		model = &r_inlineModels[i];

		model->type = MOD_Q3BRUSH;
		model->bmodel = out;
		Com_sprintf(model->name, sizeof(model->name), "*%d", i);

		for (j = 0; j < 3; j++) 
		{
			out->bounds[0][j] = LittleFloat(in->mins[j]);
			out->bounds[1][j] = LittleFloat(in->maxs[j]);
		}

		out->firstSurface = world.surfaces + LittleLong(in->firstSurface);
		out->numSurfaces = LittleLong(in->numSurfaces);
	}
}

/*
=================
R_LoadVisibility
=================
*/
static void R_LoadVisibility(const lump_t* lump)
{
	int		len;
	byte	*buf;

	len = (world.numClusters + 63) & ~63;

	world.novis = Hunk_Alloc(len);
	memset(world.novis, 0xff, len);

	len = lump->filelen;
	if (!len) 
	{
		return;
	}

	buf = mod_base + lump->fileofs;

	world.numClusters = LittleLong(((int*)buf)[0]);
	world.clusterBytes = LittleLong(((int*)buf)[1]);

	world.vis = Hunk_Alloc(len - 8);
	memcpy(world.vis, buf + 8, len - 8);
}

/*
=================
R_ParseEntities
=================
*/
static void R_ParseEntities(const lump_t* lump)
{
	char	*token, *p;
	int		numEntities;
	char	key[64], value[256];
	renderWorld_t* w;

	w = &world;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	p = (void*)(mod_base + lump->fileofs);

	numEntities = 0;
	while (1)
	{
		// parse the opening brace	
		token = COM_Parse(&p);

		if (!p)
			break;

		if (token[0] != '{')
		{
			ri.Error(ERR_FATAL, "%s: Found '%s' when expecting {\n", __FUNCTION__, token);
		}

		// go through all the dictionary pairs
		while (1)
		{
			token = COM_Parse(&p); // parse key

			if (!token)
			{
				ri.Error(ERR_DROP, "%s: EOF without }\n", __FUNCTION__);
				return;
			}

			if (token[0] == '}')
			{
				break;
			}

			strncpy(key, token, sizeof(key));

			token = COM_Parse(&p); // parse value
			if (!token)
			{
				ri.Error(ERR_DROP, "%s: no key\n", __FUNCTION__);
				return;
			}

			if (token[0] == '}')
			{
				ri.Error(ERR_DROP, "%s: } without data\n", __FUNCTION__);
			}

			strncpy(value, token, sizeof(value));

			if (numEntities == 0) // worldspawn!
			{
				// check for a different grid size
				if (!Q_stricmp(key, "gridsize")) 
				{
					sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2]);
					continue;
				}
			}
		}

		numEntities++;
	}
	ri.Printf(PRINT_ALL, "... %i entities.\n", __FUNCTION__, numEntities);
}

/*
=================
R_LoadLightGrid
=================
*/
static void R_LoadLightGrid(const lump_t* lump)
{
	int		i;
	vec3_t	maxs;
	int		numGridPoints;
	renderWorld_t *w;
	float	*wMins, *wMaxs;

	w = &world;

	w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

	wMins = w->bmodels[0].bounds[0];
	wMaxs = w->bmodels[0].bounds[1];

	for (i = 0; i < 3; i++) 
	{
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil(wMins[i] / w->lightGridSize[i]);
		maxs[i] = w->lightGridSize[i] * floor(wMaxs[i] / w->lightGridSize[i]);
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i]) / w->lightGridSize[i] + 1;
	}

	numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if (lump->filelen != numGridPoints * 8) 
	{
		ri.Printf(PRINT_ALL, "WARNING: light grid mismatch\n");
		w->lightGridData = NULL;
		return;
	}

	w->lightGridData = Hunk_Alloc(lump->filelen);
	memcpy(w->lightGridData, (void*)(mod_base + lump->fileofs), lump->filelen);

#if 0 // don't deal. - braxi.
	// deal with overbright bits
	for (i = 0; i < numGridPoints; i++) 
	{
		R_ColorShiftLightingBytes(&w->lightGridData[i * 8], &w->lightGridData[i * 8]);
		R_ColorShiftLightingBytes(&w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3]);
	}
#endif
}


/*
=================
R_LoadWorld
=================
*/
void R_LoadWorld(model_t* mod, void* buffer)
{
	int		i;
	q3bsp_header_t* header;

	if (world.bLoaded)
	{
		return;
	}

	world.bLoaded = false;

	header = (q3bsp_header_t*)buffer;
	i = LittleLong(header->version);
	if (i != Q3BSP_VERSION)
	{
		ri.Error(ERR_DROP, "R_LoadWorld: %s has wrong version number (%i should be %i)", mod->name, i, Q3BSP_VERSION);
	}

	// swap all the lumps
	mod_base = (byte*)header;
	for (i = 0; i < sizeof(q3bsp_header_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	Com_sprintf(world.name, sizeof(world.name), "%s", COM_SkipPath(mod->name));

	mod->type = MOD_Q3BRUSH;
	mod->numframes = 1;
	r_pCurrentModel = pLoadModel;

	R_LoadShaders(&header->lumps[Q3LUMP_SHADERS]);
	R_LoadLightmaps(&header->lumps[Q3LUMP_LIGHTMAPS]);
	R_LoadPlanes(&header->lumps[Q3LUMP_PLANES]);
	R_LoadFogs(&header->lumps[Q3LUMP_FOGS], &header->lumps[Q3LUMP_BRUSHES], &header->lumps[Q3LUMP_BRUSHSIDES]);
	R_LoadSurfaces(&header->lumps[Q3LUMP_SURFACES], &header->lumps[Q3LUMP_DRAWVERTS], &header->lumps[Q3LUMP_DRAWINDEXES]);
	R_LoadMarkSurfaces(&header->lumps[Q3LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs(&header->lumps[Q3LUMP_NODES], &header->lumps[Q3LUMP_LEAFS]);
	R_LoadInlineModels(&header->lumps[Q3LUMP_MODELS]);
	R_LoadVisibility(&header->lumps[Q3LUMP_VISIBILITY]);
	R_ParseEntities(&header->lumps[Q3LUMP_ENTITIES]);
	R_LoadLightGrid(&header->lumps[Q3LUMP_LIGHTGRID]);

	world.bLoaded = true;
	mod->type = MOD_Q3BRUSH;

	ri.Printf(PRINT_ALL, "Loaded Q3 BSP: %s\n", world.name);
}
