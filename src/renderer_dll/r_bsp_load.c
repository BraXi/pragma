/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_bsp_load.c -- bsp loading

/*
notes:

cplane_t in Q3 is the same in Q2
q2 msurface_t = worldSurface_t
q2 mnode_t = worldNode_t

misc_models in maps are turned into geometry by q3map == worldSurf_Mesh_t
*/

#include "r_local.h"

//#define ALLOW_EXTERNAL_LIGHTMAPS	// allow loading external lightmaps from TGA images

extern model_t* pLoadModel;
extern int modelFileLength;
extern byte* mod_base;
extern model_t r_inlineModels[MAX_WORLD_MODELS];

renderWorld_t world;

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
	q3bsp_surfinfo_t *in, *out;

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

		ri.Printf(PRINT_ALL, "    '%s'\n", out[i].name);
	}

	ri.Printf(PRINT_ALL, "... %i materials\n", world.numMaterials);
}

/*
===============
R_CopyColorNoAlpha
===============
*/
static void R_CopyColorNoTrans(byte in[4], byte out[4])
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
}

#ifdef ALLOW_EXTERNAL_LIGHTMAPS 
/*
===============
R_TryLoadExternalLightmaps
===============
*/
static void R_TryLoadExternalLightmaps()
{
	q3bsp_header_t *hdr;
	q3bsp_surface_t *surf;
	image_t *lightmap;
	int numReferencedLightmaps;
	int i, count, lmnum;
	char filename[MAX_QPATH];

	hdr = (q3bsp_header_t*)mod_base;
	surf = (void*)(mod_base + hdr->lumps[Q3LUMP_SURFACES].fileofs);
	count = hdr->lumps[Q3LUMP_SURFACES].filelen / sizeof(*surf);

	// find the highest lightmap index in surfaces to know the number of lightmaps we need
	numReferencedLightmaps = -1;
	for (i = 0; i < count; i++, surf++)
	{
		lmnum = LittleLong(surf->lightmapNum);
		if (lmnum >= 0 && lmnum > numReferencedLightmaps)
		{
			numReferencedLightmaps = lmnum;
		}
	}

	// ... and try loading them
	for (count = 0, i = 0; i < numReferencedLightmaps+1; i++)
	{
		Com_sprintf(filename, sizeof(filename), "maps/%s/lm_%04d.tga", world.name, i);
		lightmap = R_FindTexture(filename, it_texture, true);
		if (lightmap && lightmap != r_texture_missing)
		{
			count++;
			world.lightmaps[i] = lightmap;
		}
		else
		{
			ri.Error(ERR_DROP, "missing lightmap %i for map %s", i, world.name);
		}
	}

	if (!count)
		return;

	world.numLightmaps = count;

	world.bExternalLightmaps = true;
	ri.Printf(PRINT_ALL, "... %i external light maps\n", world.numLightmaps);
}
#endif

/*
===============
R_LoadLightmaps
===============
*/
static void R_LoadLightmaps(const lump_t* lump)
{
	byte* buf, * buf_p;
	int			len;
	byte		*tempPixels;
	int			i, j;
	int			numPixels;

	world.bExternalLightmaps = false;

	len = lump->filelen;
	if (!len)
	{
		
		world.numLightmaps = 0;

#ifdef ALLOW_EXTERNAL_LIGHTMAPS
		// No lightmap data in BSP but maybe there are external lightmaps
		R_TryLoadExternalLightmaps();
#endif
		return; 
	}

	buf = mod_base + lump->fileofs;

	numPixels = WORLD_LIGHTMAP_WIDTH * WORLD_LIGHTMAP_WIDTH; // Q3BSP_LIGHTMAP_SIZE * Q3BSP_LIGHTMAP_SIZE

	world.numLightmaps = len / (numPixels * 3);

	if (world.numLightmaps >= MAX_WORLD_LIGHTMAPS)
	{
		ri.Error(ERR_DROP, "Too many lightmaps");
		return;
	}

	tempPixels = malloc(numPixels * 4);
	if (!tempPixels)
	{
		ri.Error(ERR_DROP, "malloc for lightmap failed.");
		return;
	}

	memset(tempPixels, 255, numPixels * 4);

	// create all the lightmaps
	for (i = 0; i < world.numLightmaps; i++)
	{
		// expand the 24 bit on-disk to 32 bit
		buf_p = buf + i * numPixels * 3;

		for (j = 0; j < numPixels; j++)
		{
			R_CopyColorNoTrans(&buf_p[j * 3], &tempPixels[j * 4]);
			tempPixels[j * 4 + 3] = 255;
		}

		world.lightmaps[i] = R_LoadTexture(va("$lightmap_%d", i), (byte*)tempPixels, WORLD_LIGHTMAP_WIDTH, WORLD_LIGHTMAP_HEIGHT, it_texture, 32);

		// make sure lightmap is set to clamp to edges (R_LoadTexture leaves texture bound)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		R_BindTexture(0);
	}

	free(tempPixels);
	ri.Printf(PRINT_ALL, "... %i light map%s (%i kb)\n", world.numLightmaps, (world.numLightmaps > 0) ? "s" : "", (numPixels * 4 * world.numLightmaps) / 1024);
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

int *di_changed;
/*
=================
R_LoadDrawVerts
=================
*/
static void R_LoadDrawVerts(const lump_t* lump)
{
	q3bsp_drawVert_t *in;
	worldDrawVert_t *out;
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

		// convert color from bytes to floats
		for (j = 0; j < 4; j++)
		{
			out[i].color[j] = (in[i].color[j] / 255.0);
		}
	}
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

	di_changed = calloc(world.numDrawIndexes, sizeof(int));

	for (i = 0; i < count; i++ )
	{
		out[i] = LittleLong(in[i].val);
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

	face->indices = Hunk_Alloc(face->numIndexes * sizeof(int));
	for (i = 0; i < face->numIndexes; i++)
	{
		face->indices[i] = world.drawIndexes[face->firstIndex + i] + face->firstVert;
		//world.drawIndexes[face->firstIndex + i] += face->firstVert;
		int index = world.drawIndexes[face->firstIndex + i];
	
		if (index < 0 || index >= world.numDrawVerts)
		{
			ri.Error(ERR_DROP, "Bad brush draw index %i", index);
		}
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
	worldSurf->material = LittleLong(bspSurf->materialNum);
	worldSurf->fogVolumeIndex = LittleLong(bspSurf->fogNum) + 1;
	//worldSurf->material = R_WorldMaterialForNum(bspSurf->materialNum, LittleLong(bspSurf->lightmapNum));
	worldSurf->data = (void*)face;
}

/*
=================
ParseMeshSurface
misc_models etc..
=================
*/
static void ParseMeshSurface(const q3bsp_surface_t* bspSurf, worldSurface_t* worldSurf)
{
	worldSurf_Mesh_t* mesh;
	worldDrawVert_t* pVertex;
	int i;

	mesh = Hunk_Alloc(sizeof(*mesh));

	mesh->lightmap = LittleLong(bspSurf->lightmapNum);
	mesh->firstIndex = LittleLong(bspSurf->firstIndex);
	mesh->numIndexes = LittleLong(bspSurf->numIndexes);
	mesh->firstVert = LittleLong(bspSurf->firstVert);
	mesh->numVerts = LittleLong(bspSurf->numVerts);

	mesh->indices = Hunk_Alloc(mesh->numIndexes * sizeof(int));
	for (i = 0; i < mesh->numIndexes; i++)
	{
		mesh->indices[i] = world.drawIndexes[mesh->firstIndex + i] + mesh->firstVert;
		//world.drawIndexes[mesh->firstIndex + i] += mesh->firstVert;
		int index = world.drawIndexes[mesh->firstIndex + i];
		if (index < 0 || index > world.numDrawVerts)
		{
			ri.Error(ERR_DROP, "Bad mesh draw index %i", index);
		}
	}

	// calculate bounding box for world surface
	pVertex = world.drawVerts + mesh->firstVert;

	ClearBounds(mesh->mins, mesh->maxs);
	for (i = 0; i < mesh->numVerts; i++, pVertex++)
	{
		AddPointToBounds(pVertex->xyz, mesh->mins, mesh->maxs);
	}

	//worldSurf->material = R_WorldMaterialForNum(bspSurf->materialNum, LIGHTMAP_BY_VERTEX);
	worldSurf->material = LittleLong(bspSurf->materialNum);
	worldSurf->surfaceType = WORLDSURF_MESH;
	worldSurf->fogVolumeIndex = LittleLong(bspSurf->fogNum) + 1;
	worldSurf->data = (void*)mesh;
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
	//->material = R_WorldMaterialForNum(bspSurf->materialNum, LIGHTMAP_BY_VERTEX);
	worldSurf->data = (void*)billboard;
}


/*
=================
R_LoadWorldSurfaces
=================
*/
static void R_LoadWorldSurfaces(const lump_t* surfsLump, const lump_t* vertsLump, const lump_t* indexLump)
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
			//ParseMeshSurface(in, out);
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

	world.inlineModels = out = Hunk_Alloc(count * sizeof(*out));

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
	ri.Printf(PRINT_ALL, "... %i entities in a map\n", numEntities);
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

	wMins = w->inlineModels[0].mins;
	wMaxs = w->inlineModels[0].maxs;

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
		R_CopyColorNoTrans(&w->lightGridData[i * 8], &w->lightGridData[i * 8]);
		R_CopyColorNoTrans(&w->lightGridData[i * 8 + 3], &w->lightGridData[i * 8 + 3]);
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
	char tempname[MAX_QPATH];

	if (world.bLoaded)
	{
		return;
	}

	world.bLoaded = false;

	header = (q3bsp_header_t*)buffer;
	i = LittleLong(header->version);
	if (i != WORLD_VERSION)
	{
		ri.Error(ERR_DROP, "R_LoadWorld: %s has wrong version number (%i should be %i)", mod->name, i, WORLD_VERSION);
	}

	// swap all the lumps
	mod_base = (byte*)header;
	for (i = 0; i < sizeof(q3bsp_header_t) / 4; i++)
	{
		((int*)header)[i] = LittleLong(((int*)header)[i]);
	}

	COM_StripExtension(COM_SkipPath(mod->name), tempname);
	Com_sprintf(world.name, sizeof(world.name), "%s", tempname);

	mod->type = MOD_Q3BRUSH;
	mod->numframes = 1;
	r_pCurrentModel = pLoadModel;

	ri.Printf(PRINT_ALL, "----- %s(%s) -----\n", __FUNCTION__, world.name);

	R_LoadWorldMaterials(&header->lumps[Q3LUMP_SHADERS]);
	R_LoadLightmaps(&header->lumps[Q3LUMP_LIGHTMAPS]);
	R_LoadWorldPlanes(&header->lumps[Q3LUMP_PLANES]);
	R_LoadFogs(&header->lumps[Q3LUMP_FOGS], &header->lumps[Q3LUMP_BRUSHES], &header->lumps[Q3LUMP_BRUSHSIDES]);
	R_LoadDrawVerts(&header->lumps[Q3LUMP_DRAWVERTS]);
	R_LoadDrawIndexes(&header->lumps[Q3LUMP_DRAWINDEXES]);
	R_LoadWorldSurfaces(&header->lumps[Q3LUMP_SURFACES], &header->lumps[Q3LUMP_DRAWVERTS], &header->lumps[Q3LUMP_DRAWINDEXES]);
	R_LoadMarkSurfaces(&header->lumps[Q3LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs(&header->lumps[Q3LUMP_NODES], &header->lumps[Q3LUMP_LEAFS]);
	R_LoadInlineModels(&header->lumps[Q3LUMP_MODELS]);
	R_LoadVisibility(&header->lumps[Q3LUMP_VISIBILITY]);
	R_ParseEntities(&header->lumps[Q3LUMP_ENTITIES]);
	R_LoadLightGrid(&header->lumps[Q3LUMP_LIGHTGRID]);

	R_InitMaterials();

	if (R_CreateWorldVBO() == false)
	{
		// should freak out here
	}

	world.bLoaded = true;
	mod->type = MOD_Q3BRUSH;

	ri.Printf(PRINT_ALL, "----------\n");
}


/*
=================
R_FreeWorld
=================
*/
void R_FreeWorld()
{

}