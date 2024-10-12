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

cplane_t in Q3 is the same in Q2
q2 msurface_t = worldSurface_t
q2 mnode_t = worldNode_t

misc_models in maps are turned into geometry by q3map == worldSurf_Mesh_t
*/

#include "r_local.h"

extern model_t* pLoadModel;
extern int modelFileLength;
extern byte* mod_base;
extern model_t r_inlineModels[MAX_WORLD_MODELS];

typedef struct
{
	char		name[MAX_QPATH];

	image_t		*diffuse;
	image_t		*lightmap;
} material_t;

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

typedef struct 
{
	struct material_t	*material;
	int					fogVolumeIndex;

	worldSurfaceType_t	surfaceType;
	void				*data; // any of worldSurf_*t
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

typedef struct renderWorld_s
{
	char		name[MAX_QPATH]; // without .bsp and path
	qboolean	bLoaded;

	q3bsp_material_t* materials;
	int			numMaterials;

	q3bsp_drawVert_t* drawVerts;
	int			numDrawVerts;

	GLuint			*drawIndexes;
	int			numDrawIndexes;

	cplane_t	*planes;
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


	unsigned int vbo_verts;
#if 0
	unsigned int vbo_indexes;
#endif
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
R_LoadWorldMaterials
=================
*/
static void R_LoadWorldMaterials(const lump_t* lump)
{
	int		i, count;
	q3bsp_material_t *in, *out;

	in = (void*)(mod_base + lump->fileofs);
	CheckLumpSize(lump, sizeof(*in), __FUNCTION__);

	count = lump->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	world.materials = out;
	world.numMaterials = count;

	memcpy(out, in, count * sizeof(*out));

	for (i = 0; i < count; i++) 
	{
		out[i].surfaceFlags = LittleLong(out[i].surfaceFlags);
		out[i].contentFlags = LittleLong(out[i].contentFlags);
	}

	ri.Printf(PRINT_ALL, "... %i materials\n", world.numMaterials);
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

	
	world.numLightmaps = len / (numPixels * 3);

	if (world.numLightmaps >= Q3BSP_MAX_LIGHTMAPS)
	{
		ri.Error(ERR_DROP, "Too many lightmaps");
		return;
	}

	// create all the lightmaps
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

		// make sure lightmap is set to clamp to edges (R_LoadTexture leaves texture bound)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		R_BindTexture(0);
	}

	ri.Printf(PRINT_ALL, "... %i light maps (%i kb)\n", world.numLightmaps, (numPixels * 4 * world.numLightmaps) / 1024);
}

/*
=================
R_LoadWorldPlanes
=================
*/
static void R_LoadWorldPlanes(const lump_t* lump)
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
=================
R_LoadDrawVerts
=================
*/
static void R_LoadDrawVerts(const lump_t* lump)
{
	q3bsp_drawVert_t *in, *out;
	int			i, j;
	int			count;

	in = (void*)(mod_base + lump->fileofs);
	CheckLumpSize(lump, sizeof(*in), __FUNCTION__);

	count = lump->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	world.drawVerts = out;
	world.numDrawVerts = count;

	for (i = 0; i < count; i++)
	{
		for (j = 0; j < 3; j++)
		{
			out[i].xyz[j] = LittleFloat(in[i].xyz[j]);
			out[i].normal[j] = LittleFloat(in[i].normal[j]);
		}

		for (j = 0; j < 2; j++)
		{
			out[i].st[j] = LittleFloat(in[i].st[j]);
			out[i].lightmap[j] = LittleFloat(in[i].lightmap[j]);
		}

		//out[i].lightmap[1] = -out[i].lightmap[1];

		for (j = 0; j < 4; j++)
			out[i].color[j] = in[i].color[j];	

		R_ColorShiftLightingBytes(out[i].color, in[i].color);
	}
}

typedef enum
{
	LIGHTMAP_LIGHTMAP = 0,		// (>= 0) means surface is properly light mapped
	LIGHTMAP_NONE = -1,			// material does not reference lightmap
	LIGHTMAP_WHITEIMAGE = -2,	// surface is fullbright
	LIGHTMAP_BY_VERTEX = -3,	// per vertex lighting
	LIGHTMAP_2D = -4			// material for 2D rendering
} WorldLightMapType;


void* R_WorldMaterialForNum(const int num, const WorldLightMapType lightmap)
{
	return NULL;
}

typedef struct
{
	int32_t val;
} drawindex_t;
/*
=================
R_LoadDrawIndexes
=================
*/
static void R_LoadDrawIndexes(const lump_t* lump)
{
	drawindex_t* in;
	GLuint *out;
	int i, count;

	in = (void*)(mod_base + lump->fileofs);
	CheckLumpSize(lump, sizeof(*in), __FUNCTION__);

	count = lump->filelen / sizeof(int);
	out = Hunk_Alloc(count * sizeof(int));

	world.drawIndexes = out;
	world.numDrawIndexes = count;

	for (i = 0; i < count; i++ )
	{
		out[i] = LittleLong(in[i].val);

		int y = out[i];
		
		ri.Printf(PRINT_ALL, "%d,", y);
		if (out[i] < 0 || out[i] >= world.numDrawVerts)
		{
			ri.Printf(0, "Bad draw index");
		}
	}
}

/*
=================
ParseMeshSurface
misc_models etc..
=================
*/
static void ParseMeshSurface(const q3bsp_surface_t* bspSurf, worldSurface_t* worldSurf) 
{
	worldSurf_Mesh_t *mesh;
	q3bsp_drawVert_t *pVertex;
	int i;

	mesh = Hunk_Alloc(sizeof(*mesh));
	
	mesh->lightmap = LittleLong(bspSurf->lightmapNum);
	mesh->firstIndex = LittleLong(bspSurf->firstIndex);
	mesh->numIndexes = LittleLong(bspSurf->numIndexes);
	mesh->firstVert = LittleLong(bspSurf->firstVert);
	mesh->numVerts = LittleLong(bspSurf->numVerts);

	// Fix indices to be relative to firstVert
	for (i = 0; i < mesh->numIndexes; i++)
	{
		world.drawIndexes[mesh->firstIndex + i] += mesh->firstVert;
	}

	// calculate bounding box for world surface
	pVertex = world.drawVerts + mesh->firstVert;

	ClearBounds(mesh->mins, mesh->maxs);
	for (i = 0; i < mesh->numVerts; i++, pVertex++)
	{
		AddPointToBounds(pVertex->xyz, mesh->mins, mesh->maxs);
	}

	worldSurf->material = R_WorldMaterialForNum(bspSurf->materialNum, LIGHTMAP_BY_VERTEX);
	worldSurf->surfaceType = WORLDSURF_MESH;
	worldSurf->fogVolumeIndex = LittleLong(bspSurf->fogNum) + 1;
	worldSurf->data = (void*)mesh;
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
ParseBrushFace
===============
*/
static void ParseBrushFace(const q3bsp_surface_t* bspSurf, worldSurface_t* worldSurf) 
{
	worldSurf_Face_t* face;
	int i;

	face = Hunk_Alloc(sizeof(*face));

	face->lightmap = LittleLong(bspSurf->lightmapNum);
	face->firstVert = LittleLong(bspSurf->firstVert);
	face->numVerts = LittleLong(bspSurf->numVerts);
	face->firstIndex = LittleLong(bspSurf->firstIndex);
	face->numIndexes = LittleLong(bspSurf->numIndexes);

	// Fix indices to be relative to firstVert
	for (i = 0; i < face->numIndexes; i++)
	{
		world.drawIndexes[face->firstIndex + i] += face->firstVert;
	}

	// take the plane information from the lightmap vector
	for (i = 0; i < 3; i++) 
	{
		face->plane.normal[i] = LittleFloat(bspSurf->lightmapVecs[2][i]);
	}

	SetPlaneSignbits(&face->plane);
	face->plane.dist = DotProduct(world.drawVerts[ world.drawIndexes[face->firstIndex] ].xyz, face->plane.normal);
	face->plane.type = PlaneTypeForNormal(face->plane.normal);

	worldSurf->surfaceType = WORLDSURF_FACE;
	worldSurf->fogVolumeIndex = LittleLong(bspSurf->fogNum) + 1;
	worldSurf->material = R_WorldMaterialForNum(bspSurf->materialNum, LittleLong(bspSurf->lightmapNum));
	worldSurf->data = (void*)face;
}


/*
===============
ParseBillboard
===============
*/
static void ParseBillboard(const q3bsp_surface_t* bspSurf, worldSurface_t* worldSurf) 
{
	worldSurf_Billboard_t* billboard;
	int i;

	billboard = Hunk_Alloc(sizeof(*billboard));

	for (i = 0; i < 3; i++) 
	{
		billboard->origin[i] = LittleFloat(bspSurf->lightmapOrigin[i]);
		billboard->color[i] = LittleFloat(bspSurf->lightmapVecs[0][i]);
		billboard->normal[i] = LittleFloat(bspSurf->lightmapVecs[2][i]);
	}

	worldSurf->surfaceType = WORLDSURF_BILLBOARD;
	worldSurf->fogVolumeIndex = LittleLong(bspSurf->fogNum) + 1;
	worldSurf->material = R_WorldMaterialForNum(bspSurf->materialNum, LIGHTMAP_BY_VERTEX);
	worldSurf->data = (void*)billboard;
}


/*
=================
R_LoadSurfaces
=================
*/
static void R_LoadSurfaces(const lump_t* surfsLump, const lump_t* vertsLump, const lump_t* indexLump)
{
	q3bsp_surface_t *in;
	worldSurface_t *out;
	int count;
	int numFaces, numPatchMeshes, numMeshes, numBillboards;
	int i;

	numFaces = 0;
	numPatchMeshes = 0;
	numMeshes = 0;
	numBillboards = 0;

	in = (void*)(mod_base + surfsLump->fileofs);
	CheckLumpSize(surfsLump, sizeof(*in), __FUNCTION__);

	count = surfsLump->filelen / sizeof(*in);
	out = Hunk_Alloc(count * sizeof(*out));

	world.surfaces = out;
	world.numSurfaces = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		switch (LittleLong(in->surfaceType))
		{
		case MST_PLANAR: /* brush sides */
			ParseBrushFace(in, out);
			numFaces++;
			break;

		case MST_PATCH: /* curve patches */
			//ParseMesh(in, dv, out);
			numPatchMeshes++;
			break;

		case MST_TRIANGLE_SOUP: /* meshes - misc_models */
			ParseMeshSurface(in, out);
			numMeshes++;
			break;

		case MST_FLARE: /* billboards */
			ParseBillboard(in, out);
			numBillboards++;
			break;
		default:
			ri.Error(ERR_DROP, "Bad surface type %i", in->surfaceType);
		}
	}

	ri.Printf(PRINT_ALL, "... loaded %d faces, %i patches, %i meshes, %i billboards\n", numFaces, numPatchMeshes, numMeshes, numBillboards);
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
	if (node->contents != Q3CONTENTS_NODE)
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

		out->contents = Q3CONTENTS_NODE;	// differentiate from leafs

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
			out->mins[j] = LittleFloat(in->mins[j]);
			out->maxs[j] = LittleFloat(in->maxs[j]);
		}

		out->firstSurface = world.surfaces + LittleLong(in->firstSurface);
		out->numSurfaces = LittleLong(in->numSurfaces);
	}

	if (count > 1)
		ri.Printf(PRINT_ALL, "... %i inline models\n");
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
		ri.Printf(PRINT_ALL, "... %s has no visibility data\n", world.name);
		return;
	}

	buf = mod_base + lump->fileofs;

	world.numClusters = LittleLong(((int*)buf)[0]);
	world.clusterBytes = LittleLong(((int*)buf)[1]);

	world.vis = Hunk_Alloc(len - 8);
	memcpy(world.vis, buf + 8, len - 8);

	ri.Printf(PRINT_ALL, "... %i kb of visibility data\n", len-8/1024);
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
	ri.Printf(PRINT_ALL, "... parsed %i entities\n", numEntities);
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

	wMins = w->bmodels[0].mins;
	wMaxs = w->bmodels[0].maxs;

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


	// deal with overbright bits
	for (i = 0; i < numGridPoints; i++) 
	{
		R_ColorShiftLightingBytes(&w->lightGridData[i * 8], &w->lightGridData[i * 8]);
		R_ColorShiftLightingBytes(&w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3]);
	}

	ri.Printf(PRINT_ALL, "... %i light grid points (gridsize %ix%ix%i)\n", numGridPoints, (int)w->lightGridSize[0], (int)w->lightGridSize[1], (int)w->lightGridSize[2]);
}

/*
=================
R_CreateWorldVBO
=================
*/
static qboolean R_CreateWorldVBO()
{
	GLint upload_size = 0;
	GLint total_size = 0;

	// upload vertices
	upload_size = 0;
	glGenBuffers(1, &world.vbo_verts);

	glBindBuffer(GL_ARRAY_BUFFER, world.vbo_verts);
	glBufferData(GL_ARRAY_BUFFER, (world.numDrawVerts * sizeof(world.drawVerts[0])), &world.drawVerts[0].xyz[0], GL_STATIC_DRAW);
	
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &upload_size);
	if (upload_size != (world.numDrawVerts * sizeof(world.drawVerts[0])))
	{
		ri.Printf(PRINT_ALL, "Failed to create vertex buffer for %s\n", world.name);
		return false;
	}
	total_size += upload_size;
	glBindBuffer(GL_ARRAY_BUFFER, 0);

#if 0
	// upload indices
	upload_size = 0;
	glGenBuffers(1, &world.vbo_indexes);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, world.vbo_indexes);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (world.numDrawIndexes * sizeof(int)), world.drawIndexes, GL_STATIC_DRAW);

	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &upload_size);
	if (upload_size != (world.numDrawIndexes * sizeof(int)))
	{
		ri.Printf(PRINT_ALL, "Failed to create index buffer for %s\n", world.name);
		return false;
	}
	total_size += upload_size;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	ri.Printf(PRINT_ALL, "... created VBOs for world model with %i vertexes and %i indices (%i kb)\n", world.numDrawVerts, world.numDrawIndexes, total_size / 1024);
#else

	ri.Printf(PRINT_ALL, "... created VBO for world model with %i vertexes (%i kb)\n", world.numDrawVerts, total_size / 1024);
#endif

	return true;
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

	R_LoadWorldMaterials(&header->lumps[Q3LUMP_SHADERS]);
	R_LoadLightmaps(&header->lumps[Q3LUMP_LIGHTMAPS]);
	R_LoadWorldPlanes(&header->lumps[Q3LUMP_PLANES]);
	R_LoadFogs(&header->lumps[Q3LUMP_FOGS], &header->lumps[Q3LUMP_BRUSHES], &header->lumps[Q3LUMP_BRUSHSIDES]);
	R_LoadDrawVerts(&header->lumps[Q3LUMP_DRAWVERTS]);
	R_LoadDrawIndexes(&header->lumps[Q3LUMP_DRAWINDEXES]);
	R_LoadSurfaces(&header->lumps[Q3LUMP_SURFACES], &header->lumps[Q3LUMP_DRAWVERTS], &header->lumps[Q3LUMP_DRAWINDEXES]);
	R_LoadMarkSurfaces(&header->lumps[Q3LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs(&header->lumps[Q3LUMP_NODES], &header->lumps[Q3LUMP_LEAFS]);
	R_LoadInlineModels(&header->lumps[Q3LUMP_MODELS]);
	R_LoadVisibility(&header->lumps[Q3LUMP_VISIBILITY]);
	R_ParseEntities(&header->lumps[Q3LUMP_ENTITIES]);
	R_LoadLightGrid(&header->lumps[Q3LUMP_LIGHTGRID]);

	if (R_CreateWorldVBO() == false)
	{
		// should freak out here
	}

	world.bLoaded = true;
	mod->type = MOD_Q3BRUSH;

	ri.Printf(PRINT_ALL, "Succesfuly loaded Quake3 BSP %s\n", world.name);
}



qboolean R_DrawQ3World()
{
	int i;
	worldSurface_t* surf;
	worldSurf_Face_t* face;
	worldSurf_Mesh_t* mesh;

	if (!world.bLoaded)
		return false;

	glFrontFace(GL_CW);

	glBindBuffer(GL_ARRAY_BUFFER, world.vbo_verts);

	R_BindProgram(GLPROG_Q3WORLD);

	//Mat4MakeIdentity(r_worldent.modelMatrix);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, r_worldent.modelMatrix);

	R_MultiTextureBind(TMU_DIFFUSE, r_texture_white->texnum);
	R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);

	glEnableVertexAttribArray(0);
	glBindAttribLocation(pCurrentProgram->programObject, 0, "inVertPos");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(q3bsp_drawVert_t), (void*)0);

	glEnableVertexAttribArray(1);
	glBindAttribLocation(pCurrentProgram->programObject, 1, "inTexCoord");
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(q3bsp_drawVert_t), (void*)offsetof(q3bsp_drawVert_t, st));

	glEnableVertexAttribArray(2);
	glBindAttribLocation(pCurrentProgram->programObject, 2, "inLightMapCoord");
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(q3bsp_drawVert_t), (void*)offsetof(q3bsp_drawVert_t, lightmap));

	glEnableVertexAttribArray(3);
	glBindAttribLocation(pCurrentProgram->programObject, 3, "inNormal");
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(q3bsp_drawVert_t), (void*)offsetof(q3bsp_drawVert_t, normal));

	glDisable(GL_CULL_FACE);
	surf = world.surfaces;
	for (i = 0; i < world.numSurfaces; i++, surf++)
	{
		if (surf->surfaceType == WORLDSURF_FACE)
		{
			face = (worldSurf_Face_t*)surf->data;

			if (face->lightmap >= 0)
				R_MultiTextureBind(TMU_LIGHTMAP, world.lightmaps[face->lightmap]->texnum);
			else
				continue;

				//R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);

			glDrawArrays(GL_TRIANGLE_FAN, face->firstVert, face->numVerts);
		}
		else if (surf->surfaceType == WORLDSURF_MESH)
		{
			mesh = (worldSurf_Mesh_t*)surf->data;

			R_MultiTextureBind(TMU_DIFFUSE, r_texture_missing->texnum);

			if (mesh->lightmap >= 0)
				R_MultiTextureBind(TMU_LIGHTMAP, world.lightmaps[mesh->lightmap]->texnum);
			else
				R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);
			
			//void glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices);
			glDrawElements(GL_TRIANGLES, mesh->numIndexes, GL_UNSIGNED_INT, &world.drawIndexes[mesh->firstIndex]);
		}
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	R_UnbindProgram();

	return true;
}