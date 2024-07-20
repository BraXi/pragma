/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cmodel.c -- BSP model loading, collision handling

#include "qcommon.h"

qboolean bExtendedBSP = false;

typedef struct
{
	unsigned int qbism, vanilla, size_qbism, size_vanilla;
} bsp_limit_t;

static bsp_limit_t bspLimits[BSP_NUM_DATATYPES] =
{
	// QBISM BSP LIMIT,			VANILLA BSP LIMIT,		QBISM SIZE,			VANILLA SIZE
	{ MAX_MAP_MODELS_QBSP,		MAX_MAP_MODELS,			0,					sizeof(dbsp_model_t) },
	{ MAX_MAP_BRUSHES_QBSP,		MAX_MAP_BRUSHES,		0,					sizeof(dbrush_t) },
	{ MAX_MAP_ENTITIES_QBSP,	MAX_MAP_ENTITIES,		0,					0},						// unused
	{ MAX_MAP_ENTSTRING_QBSP,	MAX_MAP_ENTSTRING,		0,					0},
	{ MAX_MAP_TEXINFO_QBSP,		MAX_MAP_TEXINFO,		0,					sizeof(dbsp_texinfo_t) },	// refeered in cmod code below as surface info
	{ MAX_MAP_AREAS,			MAX_MAP_AREAS,			0,					sizeof(dbsp_area_t) },		// not extended, must increase network protocol
	{ MAX_MAP_AREAPORTALS,		MAX_MAP_AREAPORTALS,	0,					sizeof(dbsp_areaportal_t) },// not extended, must increase network protocol
	{ MAX_MAP_PLANES_QBSP,		MAX_MAP_PLANES,			0,					sizeof(dbsp_plane_t)},
	{ MAX_MAP_NODES_QBSP,		MAX_MAP_NODES,			sizeof(dbsp_node_ext_t), sizeof(dbsp_node_t)},		// dbsp_node_ext_t
	{ MAX_MAP_BRUSHSIDES_QBSP,	MAX_MAP_BRUSHSIDES,		sizeof(dbsp_brushside_ext_t), sizeof(dbsp_brushside_t) }, //dbsp_brushside_ext_t
	{ MAX_MAP_LEAFS_QBSP,		MAX_MAP_LEAFS,			sizeof(dbsp_leaf_ext_t), sizeof(dbsp_leaf_t) },		// dbsp_leaf_ext_t
	{ MAX_MAP_VERTS_QBSP,		MAX_MAP_VERTS,			0,					sizeof(dbsp_vertex_t) },
	{ MAX_MAP_FACES_QBSP,		MAX_MAP_FACES,			sizeof(dbsp_face_ext_t), sizeof(dbsp_face_t) },		// dbsp_face_ext_t
	{ MAX_MAP_LEAFFACES_QBSP,	MAX_MAP_LEAFFACES,		sizeof(unsigned int),sizeof(unsigned short) },
	{ MAX_MAP_LEAFBRUSHES_QBSP,	MAX_MAP_LEAFBRUSHES,	sizeof(unsigned int), sizeof(unsigned short) },
	{ MAX_MAP_PORTALS_QBSP,		MAX_MAP_PORTALS,		0, 0},										// unused
	{ MAX_MAP_EDGES_QBSP,		MAX_MAP_EDGES,			sizeof(dbsp_edge_ext_t), sizeof(dbsp_edge_t) },		// dbsp_edge_ext_t
	{ MAX_MAP_SURFEDGES_QBSP,	MAX_MAP_SURFEDGES,		0, sizeof(int)},
	{ MAX_MAP_LIGHTING_QBSP,	MAX_MAP_LIGHTING,		0, 0},
	{ MAX_MAP_VISIBILITY_QBSP,	MAX_MAP_VISIBILITY,		0, 0}
};

/*
=================
GetBSPLimit
=================
*/
unsigned int GetBSPLimit(bspDataType type)
{
	if (type >= BSP_NUM_DATATYPES || type < 0)
	{
		Com_Error(ERR_DROP, "%s wrong type %i\n", __FUNCTION__, type);
		return 0;
	}

	if (bExtendedBSP)
		return bspLimits[type].qbism;
	return bspLimits[type].vanilla;
}

/*
=================
GetBSPElementSize
=================
*/
unsigned int GetBSPElementSize(bspDataType type)
{
	if (type >= BSP_NUM_DATATYPES || type < 0)
	{
		Com_Error(ERR_DROP, "%s wrong type %i", __FUNCTION__, type);
		return 0;
	}

	if (bExtendedBSP && bspLimits[type].size_qbism > 0)
		return bspLimits[type].size_qbism;
	return bspLimits[type].size_vanilla;
}

/*
=================
CMod_ValidateBSPLump
=================
*/
static __inline void CMod_ValidateBSPLump(lump_t* l, bspDataType type, unsigned int* count, int minElemCount, const char* what, const char* func)
{
	unsigned int elemsize = GetBSPElementSize(type);

	*count = l->filelen / elemsize;

	if (l->filelen % elemsize)
		Com_Error(ERR_DROP, "%s: incorrect size of a %s lump", func, what);

	if (*count < minElemCount)
		Com_Error(ERR_DROP, "Map with no %s", what);

	if (*count >= GetBSPLimit(type))
		Com_Error(ERR_DROP, "Map has too many %s", what);
}


typedef struct
{
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
} cnode_t;

typedef struct
{
	cplane_t	*plane;
	mapsurface_t	*surface;
} cbrushside_t;

typedef struct
{
	int contents;
	int cluster;
	int area;
	unsigned int firstleafbrush;
	unsigned int numleafbrushes;
} cleaf_t;

typedef struct
{
	int contents;
	int numsides;
	int firstbrushside;
	unsigned int checkcount; // to avoid repeated testings
} cbrush_t;

typedef struct
{
	int numareaportals;
	int firstareaportal;
	int floodnum;			// if two areas have equal floodnums, they are connected
	int floodvalid;			// used to check if already flooded into
} carea_t;


/*
===============================================================================

	COLLISION MODEL DATA

===============================================================================
*/

static char map_name[MAX_QPATH];

static int map_numBrushSides;
static cbrushside_t map_brushSides[MAX_MAP_BRUSHSIDES_QBSP];

static int map_numSurfaceInfos;
static mapsurface_t map_surfaceInfos[MAX_MAP_TEXINFO_QBSP];

static int map_numPlanes;
static cplane_t map_planes[MAX_MAP_PLANES_QBSP + 6]; // extra for box hull

static int map_numNodes;
static cnode_t map_nodes[MAX_MAP_NODES_QBSP + 6]; // extra for box hull

static int map_numLeafClusters = 1; // allow leaf funcs to be called without a map
static int map_numLeafs = 1; // allow leaf funcs to be called without a map
static cleaf_t map_leafs[MAX_MAP_LEAFS_QBSP];

static int map_numLeafBrushes;
static unsigned int map_leafBrushes[MAX_MAP_LEAFBRUSHES_QBSP]; // was short for bsp38

static int map_numInlineModels;
static cmodel_t map_inlineModels[MAX_MAP_MODELS_QBSP];

static int map_numBrushes;
static cbrush_t map_brushes[MAX_MAP_BRUSHES_QBSP];

static int map_numEntityStringChars;
static char map_entityString[MAX_MAP_ENTSTRING_QBSP];

static int map_numVisibility;
static byte map_visibilityData[MAX_MAP_VISIBILITY_QBSP];
static dbsp_vis_t* map_vis = (dbsp_vis_t*)map_visibilityData;

static int map_numAreas = 1; // allow leaf funcs to be called without a map
static carea_t map_areas[MAX_MAP_AREAS];

static int map_numAreaPortals;
static dbsp_areaportal_t map_areaPortals[MAX_MAP_AREAPORTALS];
static qboolean map_openAreaPortalsList[MAX_MAP_AREAPORTALS];

// ============================================================================

static unsigned int checkcount = 0;
static int emptyleaf, solidleaf;
static mapsurface_t nullsurface = { 0 };
static int floodvalid;

static cvar_t *map_noareas; // debugging aid

static void CM_InitBoxHull();
static void CM_FloodAreaConnections();

int	c_pointcontents; // performance counters
int c_traces, c_brush_traces; // performance counters

static byte* cmod_base; // pointer to BSP data

/*
===============================================================================
					MAP LOADING
===============================================================================
*/

/*
=================
CMod_LoadSurfaceParams
=================
*/
static void CMod_LoadSurfaceParams(lump_t *l)
{
	dbsp_texinfo_t	*in;
	mapsurface_t	*out;
	unsigned int i, count;

	CMod_ValidateBSPLump(l, BSP_TEXINFO, &count, 1, "surface infos", __FUNCTION__);
	in = (void*)(cmod_base + l->fileofs);

	map_numSurfaceInfos = count;
	out = map_surfaceInfos;

	for (i = 0; i < count; i++, in++, out++)
	{
		strncpy(out->c.name, in->texture, sizeof(out->c.name) - 1);
		strncpy(out->rname, in->texture, sizeof(out->rname) - 1);
		out->c.flags = LittleLong(in->flags);
		out->c.value = LittleLong(in->value);
	}
}


/*
=================
CMod_LoadNodes
=================
*/
static void CMod_LoadNodes(lump_t *l)
{
	int			child;
	cnode_t		*out;
	unsigned int i, j, count;
	
	CMod_ValidateBSPLump(l, BSP_NODES, &count, 1, "nodes", __FUNCTION__);

	out = map_nodes;
	map_numNodes = count;

	if (bExtendedBSP) // qbism bsp
	{
		dbsp_node_ext_t* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, out++, in++)
		{
			out->plane = map_planes + LittleLong(in->planenum);
			for (j = 0; j < 2; j++)
			{
				child = LittleLong(in->children[j]);
				out->children[j] = child;
			}
		}
	}
	else
	{
		dbsp_node_t* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, out++, in++)
		{
			out->plane = map_planes + LittleLong(in->planenum);
			for (j = 0; j < 2; j++)
			{
				child = LittleLong(in->children[j]);
				out->children[j] = child;
			}
		}
	}
}

/*
=================
CMod_LoadBrushes
=================
*/
static void CMod_LoadBrushes(lump_t *l)
{
	dbrush_t* in;
	cbrush_t* out;
	unsigned int i, count;

	CMod_ValidateBSPLump(l, BSP_BRUSHES, &count, 1, "brushes", __FUNCTION__);

	out = map_brushes;
	map_numBrushes = count;

	in = (void*)(cmod_base + l->fileofs);
	for (i = 0; i < count; i++, out++, in++)
	{
		out->firstbrushside = LittleLong(in->firstside);
		out->numsides = LittleLong(in->numsides);
		out->contents = LittleLong(in->contents);
	}
}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs(lump_t *l)
{
	cleaf_t* out;
	unsigned int i, count;

	CMod_ValidateBSPLump(l, BSP_LEAFS, &count, 1, "leafs", __FUNCTION__);

	out = map_leafs;
	map_numLeafs = count;
	map_numLeafClusters = 0;

	if (bExtendedBSP) // qbism bsp
	{
		dbsp_leaf_ext_t* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			out->contents = LittleLong(in->contents);
			out->cluster = LittleLong(in->cluster);
			out->area = LittleLong(in->area);
			out->firstleafbrush = LittleLong(in->firstleafbrush);
			out->numleafbrushes = LittleLong(in->numleafbrushes);

			if (out->cluster >= map_numLeafClusters)
				map_numLeafClusters = out->cluster + 1;
		}
	}
	else
	{
		dbsp_leaf_t* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			out->contents = LittleLong(in->contents);
			out->cluster = LittleShort(in->cluster);
			out->area = LittleShort(in->area);
			out->firstleafbrush = LittleShort(in->firstleafbrush);
			out->numleafbrushes = LittleShort(in->numleafbrushes);

			if (out->cluster >= map_numLeafClusters)
				map_numLeafClusters = out->cluster + 1;
		}
	}

	if (map_leafs[0].contents != CONTENTS_SOLID)
		Com_Error(ERR_DROP, "Map leaf 0 is not CONTENTS_SOLID");

	solidleaf = 0;
	emptyleaf = -1;
	for (i = 1; i < map_numLeafs; i++)
	{
		if (!map_leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
		Com_Error(ERR_DROP, "Map does not have an empty leaf");
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes(lump_t* l)
{
	dbsp_plane_t* in;
	cplane_t* out;
	int		i, j, bits;
	unsigned int count;

	CMod_ValidateBSPLump(l, BSP_PLANES, &count, 1, "planes", __FUNCTION__);

	in = (void*)(cmod_base + l->fileofs);
	out = map_planes; // 12 extra for box hull
	map_numPlanes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = LittleLong(in->type);
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes(lump_t* l)
{
	unsigned int i, count;

	CMod_ValidateBSPLump(l, BSP_LEAFBRUSHES, &count, 1, "leaf brushes", __FUNCTION__);
	map_numLeafBrushes = count;

	if (bExtendedBSP)
	{
		unsigned int* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, in++)
			map_leafBrushes[i] = LittleLong(*in);
	}
	else
	{
		unsigned short* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, in++)
			map_leafBrushes[i] = (int)LittleShort(*in);
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides(lump_t* l)
{
	cbrushside_t* out;
	int		surfInfo, planeNum;
	unsigned int i, count;

	CMod_ValidateBSPLump(l, BSP_BRUSHSIDES, &count, 1, "brush sides", __FUNCTION__);

	out = map_brushSides;
	map_numBrushSides = count;

	if (bExtendedBSP) // Qbism BSP
	{
		dbsp_brushside_ext_t* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			planeNum = LittleLong(in->planenum);
			surfInfo = LittleLong(in->texinfo);

			if (surfInfo >= map_numSurfaceInfos)
				Com_Error(ERR_DROP, "Bad brushside surface info");

			if (planeNum >= map_numPlanes)
				Com_Error(ERR_DROP, "Bad brushside plane");

			out->plane = &map_planes[planeNum];
			out->surface = &map_surfaceInfos[surfInfo];
		}
	}
	else // Vanilla Q2 BSP
	{
		dbsp_brushside_t* in = (void*)(cmod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			planeNum = LittleShort(in->planenum);
			surfInfo = LittleShort(in->texinfo);

			if (surfInfo >= map_numSurfaceInfos)
				Com_Error(ERR_DROP, "Bad brushside surface info");

			if (planeNum >= map_numPlanes)
				Com_Error(ERR_DROP, "Bad brushside plane");

			out->plane = &map_planes[planeNum];
			out->surface = &map_surfaceInfos[surfInfo];
		}
	}
}

/*
=================
CMod_LoadInlineModels
=================
*/
static void CMod_LoadInlineModels(lump_t* l)
{
	dbsp_model_t* in;
	cmodel_t* out;
	unsigned int i, j, count;

	CMod_ValidateBSPLump(l, BSP_MODELS, &count, 1, "inline models", __FUNCTION__);
	in = (void*)(cmod_base + l->fileofs);

	map_numInlineModels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out = &map_inlineModels[i];

		for (j = 0; j < 3; j++)
		{
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		out->headnode = LittleLong(in->headnode);
	}
}


/*
=================
CMod_LoadAreas
=================
*/
static void CMod_LoadAreas(lump_t* l)
{
	carea_t* out;
	dbsp_area_t* in;
	unsigned int i, count;

	CMod_ValidateBSPLump(l, BSP_AREAS, &count, 1, "areas", __FUNCTION__);

	out = map_areas;
	map_numAreas = count;

	in = (void*)(cmod_base + l->fileofs);
	for (i = 0; i < count; i++, in++, out++)
	{
		out->numareaportals = LittleLong(in->numareaportals);
		out->firstareaportal = LittleLong(in->firstareaportal);
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

/*
=================
CMod_LoadAreaPortals
=================
*/
static void CMod_LoadAreaPortals(lump_t* l)
{
	dbsp_areaportal_t* out, * in;
	unsigned int i, count;

	CMod_ValidateBSPLump(l, BSP_AREAPORTALS, &count, 1, "area portals", __FUNCTION__);

	out = map_areaPortals;
	map_numAreaPortals = count;

	in = (void*)(cmod_base + l->fileofs);
	for (i = 0; i < count; i++, in++, out++)
	{
		out->portalnum = LittleLong(in->portalnum);
		out->otherarea = LittleLong(in->otherarea);
	}
}

/*
=================
CMod_LoadVisibility
=================
*/
static void CMod_LoadVisibility (lump_t *l)
{
	int i;

	if (l->filelen >= GetBSPLimit(BSP_VISIBILITY))
		Com_Error(ERR_DROP, "%s: Map has too large visibility info", __FUNCTION__);

	map_numVisibility = l->filelen;
	memcpy (map_visibilityData, cmod_base + l->fileofs, l->filelen);

	map_vis->numclusters = LittleLong (map_vis->numclusters);
	for (i = 0; i < map_vis->numclusters; i++)
	{
		map_vis->bitofs[i][0] = LittleLong (map_vis->bitofs[i][0]);
		map_vis->bitofs[i][1] = LittleLong (map_vis->bitofs[i][1]);
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString(lump_t *l)
{
	if (l->filelen >= GetBSPLimit(BSP_ENTSTRING))
	{
		Com_Error(ERR_DROP, "%s: Map has too large entity string", __FUNCTION__);
	}

	map_numEntityStringChars = l->filelen;
	memcpy (map_entityString, cmod_base + l->fileofs, l->filelen);
}

/*
==================
CM_FreeMap
==================
*/
void CM_FreeMap()
{
	// free old stuff
	map_name[0] = 0;

	map_numPlanes = 0;
	map_numNodes = 0;
	map_numLeafs = 0;
	map_numInlineModels = 0;
	map_numVisibility = 0;
	map_numEntityStringChars = 0;
	map_entityString[0] = 0;

}

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
cmodel_t *CM_LoadMap(char *name, qboolean clientload, unsigned *checksum)
{
	unsigned		*buf;
	int				i;
	dbsp_header_t		header;
	int				length;
	static unsigned	last_checksum;

	map_noareas = Cvar_Get ("cm_noareas", "0", 0, NULL);

	if ( !strcmp (map_name, name) && (clientload || !Cvar_VariableValue ("cm_flushmap")) )
	{
		*checksum = last_checksum;
		if (!clientload)
		{
			memset (map_openAreaPortalsList, 0, sizeof(map_openAreaPortalsList));
			CM_FloodAreaConnections ();
		}
		return &map_inlineModels[0]; // still have the right version
	}

	CM_FreeMap();

	// cinematic servers won't have anything at all
	if (!name || !name[0])
	{
		map_numLeafs = 1;
		map_numLeafClusters = 1;
		map_numAreas = 1;
		*checksum = 0;
		return &map_inlineModels[0];			
	}

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buf);
	if (!buf)
	{
		Com_Error(ERR_DROP, "Couldn't load BSP %s", name);
	}

	last_checksum = LittleLong (Com_BlockChecksum (buf, length));
	*checksum = last_checksum;

	header = *(dbsp_header_t *)buf;
	for (i=0 ; i<sizeof(dbsp_header_t)/4 ; i++)
		((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);

	bExtendedBSP = false;
	if (header.ident == QBISM_IDENT)
	{
		bExtendedBSP = true;
	}
	else if (header.ident != BSP_IDENT)
	{
		Com_Error(ERR_DROP, "CM_LoadMap: %s is not a BSP\n", __FUNCTION__, name);
	}

	if (header.version != BSP_VERSION)
		Com_Error(ERR_DROP, "%s: %s has wrong version number (%i should be %i)", __FUNCTION__, name, header.version, BSP_VERSION);

	cmod_base = (byte *)buf;

	// load into heap
	CMod_LoadSurfaceParams(&header.lumps[LUMP_TEXINFO]);
	CMod_LoadLeafs(&header.lumps[LUMP_LEAFS]);
	CMod_LoadLeafBrushes(&header.lumps[LUMP_LEAFBRUSHES]);
	CMod_LoadPlanes(&header.lumps[LUMP_PLANES]);
	CMod_LoadBrushes(&header.lumps[LUMP_BRUSHES]);
	CMod_LoadBrushSides(&header.lumps[LUMP_BRUSHSIDES]);
	CMod_LoadInlineModels(&header.lumps[LUMP_MODELS]);
	CMod_LoadNodes(&header.lumps[LUMP_NODES]);
	CMod_LoadAreas(&header.lumps[LUMP_AREAS]);
	CMod_LoadAreaPortals(&header.lumps[LUMP_AREAPORTALS]);
	CMod_LoadVisibility(&header.lumps[LUMP_VISIBILITY]);
	CMod_LoadEntityString(&header.lumps[LUMP_ENTITIES]);

	FS_FreeFile (buf);

	CM_InitBoxHull();

	// close all portals
	memset (map_openAreaPortalsList, 0, sizeof(map_openAreaPortalsList));
	CM_FloodAreaConnections ();

	strncpy (map_name, name, sizeof(map_name)-1);

	return &map_inlineModels[0];
}

/*
==================
CM_InlineModel

Returns the inline model by name
==================
*/
cmodel_t* CM_InlineModel(char* name)
{
	int		num;

	if (!name || name[0] != '*')
		Com_Error(ERR_DROP, "CM_InlineModel: bad name");
	num = atoi(name + 1);
	if (num < 1 || num >= map_numInlineModels)
		Com_Error(ERR_DROP, "CM_InlineModel: bad number %i, (numcmodels=%i)", num, map_numInlineModels);

	return &map_inlineModels[num];
}

/*
==================
CM_NumClusters
Returns the number of clusters
==================
*/
int	CM_NumClusters()
{
	return map_numLeafClusters;
}

/*
==================
CM_NumInlineModels
Returns the number of inline models
==================
*/
int	CM_NumInlineModels()
{
	return map_numInlineModels;
}

/*
==================
CM_EntityString
Returns the entity string for parsing, can be NULL
==================
*/
char* CM_EntityString()
{
	return map_entityString;
}

/*
==================
CM_LeafContents
Returns the contents of a leaf
==================
*/
int	CM_LeafContents(int leafnum)
{
	if (leafnum < 0 || leafnum >= map_numLeafs)
	{
		Com_Error(ERR_DROP, "%s: bad leaf number %i", __FUNCTION__, leafnum);
		return 0; // all these unnecessary returns are to make msvc happy
	}

	return map_leafs[leafnum].contents;
}

/*
==================
CM_LeafCluster
Returns the cluster of a leaf
==================
*/
int	CM_LeafCluster(int leafnum)
{
	if (leafnum < 0 || leafnum >= map_numLeafs)
	{
		Com_Error(ERR_DROP, "%s: bad leaf number %i", __FUNCTION__, leafnum);
		return 0;
	}
	return map_leafs[leafnum].cluster;
}

/*
==================
CM_LeafArea
Returns the area of a leaf
==================
*/
int	CM_LeafArea(int leafnum)
{
	if (leafnum < 0 || leafnum >= map_numLeafs)
	{
		Com_Error(ERR_DROP, "%s: bad leaf number %i", __FUNCTION__, leafnum);
		return 0;
	}
	return map_leafs[leafnum].area;
}

//=======================================================================

static cplane_t* box_planes;
static int		box_headnode;
static cbrush_t* box_brush;
static cleaf_t* box_leaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
static void CM_InitBoxHull()
{
	int			i;
	int			side;
	cnode_t		*c;
	cplane_t	*p;
	cbrushside_t	*s;

	box_headnode = map_numNodes;
	box_planes = &map_planes[map_numPlanes];
	if (map_numNodes + 6 > GetBSPLimit(BSP_NODES)
		|| map_numBrushes + 1 > GetBSPLimit(BSP_BRUSHES)
		|| map_numLeafBrushes + 1 > GetBSPLimit(BSP_LEAFBRUSHES)
		|| map_numBrushSides + 6 > GetBSPLimit(BSP_BRUSHSIDES)
		|| map_numPlanes + 12 > GetBSPLimit(BSP_PLANES))
		Com_Error(ERR_DROP, "%s: Not enough room for box tree\n", __FUNCTION__);

	box_brush = &map_brushes[map_numBrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = map_numBrushSides;
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &map_leafs[map_numLeafs];
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = map_numLeafBrushes;
	box_leaf->numleafbrushes = 1;

	map_leafBrushes[map_numLeafBrushes] = map_numBrushes;

	for (i = 0; i < 6; i++)
	{
		side = i&1;

		// brush sides
		s = &map_brushSides[map_numBrushSides+i];
		s->plane = map_planes + (map_numPlanes+i*2+side);
		s->surface = &nullsurface;

		// nodes
		c = &map_nodes[box_headnode+i];
		c->plane = map_planes + (map_numPlanes+i*2);
		c->children[side] = -1 - emptyleaf;
		if (i != 5)
			c->children[side^1] = box_headnode+i + 1;
		else
			c->children[side^1] = -1 - map_numLeafs;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}	
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_HeadnodeForBox(vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
CM_PointLeafnum_r
==================
*/
static int CM_PointLeafnum_r(vec3_t p, int num)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = map_nodes + num;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

/*
=============
CM_PointLeafnum

sound may call this without map loaded
=============
*/
int CM_PointLeafnum(vec3_t p)
{
	if (!map_numPlanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (p, 0);
}


static int		leaf_count, leaf_maxcount;
static int		*leaf_list;
static float	*leaf_mins, *leaf_maxs;
static int		leaf_topnode;

/*
=============
CM_BoxLeafnums_r

Fills in a list of all the leafs touched
=============
*/
static void CM_BoxLeafnums_r(int nodenum)
{
	cplane_t	*plane;
	cnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}
	
		node = &map_nodes[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (node->children[0]);
			nodenum = node->children[1];
		}

	}
}

/*
=============
CM_BoxLeafnums_headnode
=============
*/
static int CM_BoxLeafnums_headnode(vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

/*
=============
CM_BoxLeafnums
Fills in a list of all the leafs touched
=============
*/
int CM_BoxLeafnums(vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (mins, maxs, list, listsize, map_inlineModels[0].headnode, topnode);
}


/*
==================
CM_PointContents
Returns contents of inlinemodel at point
==================
*/
int CM_PointContents(vec3_t p, int headnode)
{
	int		l;

	if (!map_numNodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (p, headnode);

	return map_leafs[l].contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and rotating entities
==================
*/
int	CM_TransformedPointContents(vec3_t p, int headnode, vec3_t origin, vec3_t angles)
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;
	int			l;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (p_l, headnode);

	return map_leafs[l].contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON    (1 / 32.f)

static vec3_t	trace_start, trace_end;
static vec3_t	trace_mins, trace_maxs;
static vec3_t	trace_extents;
static trace_t	trace_trace;
static int		trace_contents;
static qboolean	trace_ispoint;		// optimized case

/*
================
CM_ClipBoxToBrush
================
*/
static void CM_ClipBoxToBrush(vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	cbrushside_t	*side, *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i = 0; i < brush->numsides; i++)
	{

		if ((brush->firstbrushside + i) >= GetBSPLimit(BSP_BRUSHSIDES))
		{
			Com_Error(ERR_DROP, "brushside > maxbrushsides");
		}

		side = &map_brushSides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection with the entire brush
		if (d1 > 0 && (d2 >= DIST_EPSILON || d2 >= d1)) // Paril: Q3A fix
			return;

		// if it doesn't cross the plane, the plane isn't relevent
		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = max(0.0f, (d1 - DIST_EPSILON) / (d1 - d2)); // Paril: from Q3A
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			// Paril: from Q3A
			f = min(1.0f, (d1 + DIST_EPSILON) / (d1 - d2));
			// Paril
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	
		// original point was inside brush
		trace->startsolid = true;
		if (!getout)
		{
			trace->allsolid = true;
			// Q2PRO: vanilla Q2 didn't set these two
			trace->fraction = 0;
			trace->contents = brush->contents;
		}
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1 && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = &(leadside->surface->c);
			trace->contents = brush->contents;
		}
	}
}

/*
================
CM_TestBoxInBrush
================
*/
static void CM_TestBoxInBrush(vec3_t mins, vec3_t maxs, vec3_t p1, trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i = 0; i < brush->numsides; i++)
	{
		side = &map_brushSides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial
		// general box case
		// push the plane out apropriately for mins/maxs
		// FIXME: use signbits into 8 way lookup for each mins/maxs

		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
static void CM_TraceToLeaf(int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafBrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
================
CM_TestInLeaf
================
*/
static void CM_TestInLeaf(int leafnum)
{
	int			k;
	int			brushnum;
	cleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafBrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

}


/*
==================
CM_RecursiveHullCheck
==================
*/
static void CM_RecursiveHullCheck(int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (-1-num);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = map_nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}


#if 0
CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
return;
#endif

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0f/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0f/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;
		
	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;
		
	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}



//======================================================================

/*
==================
CM_BoxTrace
==================
*/
trace_t CM_BoxTrace(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask)
{
	int		i;

	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &(nullsurface.c);

	if (!map_numNodes)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024];
		int		num_leafs;
		vec3_t	c1, c2;
		int		topnode;

		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		num_leafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<num_leafs ; i++)
		{
			CM_TestInLeaf (leafs[i]);
			if (trace_trace.allsolid)
				break;
		}
		VectorCopy (start, trace_trace.endpos);
		return trace_trace;
	}

	//
	// check for point special case
	//
	if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
		&& maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
	{
		trace_ispoint = true;
		VectorClear (trace_extents);
	}
	else
	{
		trace_ispoint = false;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	//
	// general sweeping through world
	//
	CM_RecursiveHullCheck (headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy (end, trace_trace.endpos);
	}
	else
	{
		for (i = 0; i < 3; i++)
			trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}
	return trace_trace;
}


#ifdef _WIN32
#pragma optimize( "", off ) //CM_TransformedBoxTrace
#endif

/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and rotating entities
==================
*/
trace_t CM_TransformedBoxTrace(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask, vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	qboolean	rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && 
	(angles[0] || angles[1] || angles[2]) )
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace (start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && trace.fraction != 1.0)
	{
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif



/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
static void CM_DecompressVis(byte *in, byte *out)
{
	int		c;
	byte	*out_p;
	int		row;

	row = (map_numLeafClusters+7)>>3;
	out_p = out;

	if (!in || !map_numVisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;		
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Com_DPrintf (DP_ALL, "warning: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

static byte pvsrow[MAX_MAP_LEAFS/8];
static byte phsrow[MAX_MAP_LEAFS/8];

/*
===================
CM_ClusterPVS
===================
*/
byte *CM_ClusterPVS(int cluster)
{
	if (cluster == -1)
		memset (pvsrow, 0, (map_numLeafClusters+7)>>3);
	else
		CM_DecompressVis (map_visibilityData + map_vis->bitofs[cluster][DVIS_PVS], pvsrow);
	return pvsrow;
}

/*
===================
CM_ClusterPHS
===================
*/
byte *CM_ClusterPHS(int cluster)
{
	if (cluster == -1)
		memset (phsrow, 0, (map_numLeafClusters+7)>>3);
	else
		CM_DecompressVis (map_visibilityData + map_vis->bitofs[cluster][DVIS_PHS], phsrow);
	return phsrow;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

/*
===================
FloodArea_r
===================
*/
static void FloodArea_r(carea_t *area, int floodnum)
{
	int		i;
	dbsp_areaportal_t	*p;

	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		Com_Error (ERR_DROP, "%s: reflooded", __FUNCTION__);
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	p = &map_areaPortals[area->firstareaportal];
	for (i=0 ; i<area->numareaportals ; i++, p++)
	{
		if (map_openAreaPortalsList[p->portalnum])
			FloodArea_r (&map_areas[p->otherarea], floodnum);
	}
}

/*
====================
CM_FloodAreaConnections
====================
*/
static void CM_FloodAreaConnections()
{
	int		i;
	carea_t	*area;
	int		floodnum;

	// all current floods are now invalid
	floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i= 1; i < map_numAreas; i++)
	{
		area = &map_areas[i];
		if (area->floodvalid == floodvalid)
			continue; // already flooded into
		floodnum++;
		FloodArea_r (area, floodnum);
	}

}

/*
===================
CM_SetAreaPortalState
===================
*/
void CM_SetAreaPortalState(int portalnum, qboolean open)
{
	if (portalnum > map_numAreaPortals)
		Com_Error (ERR_DROP, "areaportal > numareaportals");

	map_openAreaPortalsList[portalnum] = open;
	CM_FloodAreaConnections ();
}

/*
===================
CM_AreasConnected
===================
*/
qboolean CM_AreasConnected(int area1, int area2)
{
	if (map_noareas->value)
		return true;

	if (area1 > map_numAreas || area2 > map_numAreas)
		Com_Error (ERR_DROP, "area > numareas");

	if (map_areas[area1].floodnum == map_areas[area2].floodnum)
		return true;
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits(byte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (map_numAreas+7)>>3;

	if (map_noareas->value)
	{	
		// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		floodnum = map_areas[area].floodnum;
		for (i = 0; i< map_numAreas; i++)
		{
			if (map_areas[i].floodnum == floodnum || !area)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void CM_WritePortalState(FILE *f)
{
	fwrite (map_openAreaPortalsList, sizeof(map_openAreaPortalsList), 1, f);
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file and recalculates the area connections
===================
*/
void CM_ReadPortalState(FILE *f)
{
	FS_Read (map_openAreaPortalsList, sizeof(map_openAreaPortalsList), f);
	CM_FloodAreaConnections ();
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that is potentially visible
=============
*/
qboolean CM_HeadnodeVisible(int nodenum, byte *visbits)
{
	int		leafnum;
	int		cluster;
	cnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &map_nodes[nodenum];
	if (CM_HeadnodeVisible(node->children[0], visbits))
		return true;
	return CM_HeadnodeVisible(node->children[1], visbits);
}

