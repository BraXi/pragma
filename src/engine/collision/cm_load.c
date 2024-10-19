/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cmodel.c -- model loading

#include "cm_local.h"

#define	MAX_PATCH_VERTS		1024

// to allow boxes to be treated as brush models, we allocate
// some extra indexes along with those needed by the map
#define	BOX_BRUSHES		1
#define	BOX_SIDES		6
#define	BOX_LEAFS		2
#define	BOX_PLANES		12

#define	LL(x) x=LittleLong(x)

clipMap_t	cm;
int			c_pointcontents;
int			c_traces, c_brush_traces, c_patch_traces;


byte		*cmod_base;

cvar_t		*cm_noAreas;
cvar_t		*cm_noCurves;
cvar_t		*cm_playerCurveClip;

cmodel_t	box_model;
cplane_t	*box_planes;
cbrush_t	*box_brush;


void CM_InitBoxHull();
void CM_FloodAreaConnections();


/*
===============================================================================

					MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadShaders
=================
*/
static void CMod_LoadShaders(lump_t *l) 
{
	q3bsp_surfinfo_t *in, *out;
	int i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) 
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}

	count = l->filelen / sizeof(*in);
	if (count < 1) 
	{
		Com_Error (ERR_DROP, "Map with no materials");
	}
	
	cm.shaders = Hunk_Alloc( count * sizeof( *cm.shaders ) );
	cm.numShaders = count;

	memcpy( cm.shaders, in, count * sizeof( *cm.shaders ) );

	out = cm.shaders;
	for ( i=0 ; i<count ; i++, in++, out++ ) {
		out->contentFlags = LittleLong( out->contentFlags );
		out->surfaceFlags = LittleLong( out->surfaceFlags );
	}
}


/*
=================
CMod_LoadSubmodels
=================
*/
static void CMod_LoadSubmodels(lump_t *l) 
{
	q3bsp_model_t *in;
	cmodel_t *out;
	int i, j, count;
	int *indexes;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}
	
	count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		Com_Error(ERR_DROP, "Map with no inline models");
	}

	cm.cmodels = Hunk_Alloc( count * sizeof( *cm.cmodels ) );
	cm.numSubModels = count;

	if ( count > MAX_WORLD_MODELS ) 
	{
		Com_Error( ERR_DROP, "MAX_SUBMODELS exceeded" );
	}

	for (i = 0; i<count ; i++, in++, out++)
	{
		out = &cm.cmodels[i];

		for (j = 0; j < 3; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
		}

		if ( i == 0 ) 
		{
			continue;	// world model doesn't need other info
		}

		// make a "leaf" just to hold the model's brushes and surfaces
		out->leaf.numLeafBrushes = LittleLong( in->numBrushes );
		indexes = Hunk_Alloc( out->leaf.numLeafBrushes * 4 );
		out->leaf.firstLeafBrush = indexes - cm.leafbrushes;

		for (j = 0; j < out->leaf.numLeafBrushes; j++) 
		{
			indexes[j] = LittleLong(in->firstBrush) + j;
		}

		out->leaf.numLeafSurfaces = LittleLong(in->numSurfaces);	
		indexes = Hunk_Alloc( out->leaf.numLeafSurfaces * 4 );
		out->leaf.firstLeafSurface = indexes - cm.leafsurfaces;

		for (j = 0; j < out->leaf.numLeafSurfaces; j++) 
		{
			indexes[j] = LittleLong(in->firstSurface) + j;
		}
	}
}


/*
=================
CMod_LoadNodes
=================
*/
static void CMod_LoadNodes(lump_t *l) 
{
	q3bsp_node_t *in;
	cNode_t *out;
	int child;
	int i, j, count;
	
	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}
	
	count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		Com_Error(ERR_DROP, "Map has no nodes");
	}

	cm.nodes = Hunk_Alloc( count * sizeof( *cm.nodes ) );
	cm.numNodes = count;

	out = cm.nodes;

	for (i = 0; i < count; i++, out++, in++)
	{
		out->plane = cm.planes + LittleLong( in->planeNum );
		for (j = 0; j < 2; j++)
		{
			child = LittleLong(in->children[j]);
			out->children[j] = child;
		}
	}

}

/*
=================
CM_BoundBrush
=================
*/
static void CM_BoundBrush(cbrush_t *b) 
{
	b->bounds[0][0] = -b->sides[0].plane->dist;
	b->bounds[1][0] = b->sides[1].plane->dist;

	b->bounds[0][1] = -b->sides[2].plane->dist;
	b->bounds[1][1] = b->sides[3].plane->dist;

	b->bounds[0][2] = -b->sides[4].plane->dist;
	b->bounds[1][2] = b->sides[5].plane->dist;
}


/*
=================
CMod_LoadBrushes
=================
*/
static void CMod_LoadBrushes(lump_t *l) 
{
	q3bsp_brush_t *in;
	cbrush_t *out;
	int i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in)) 
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}

	count = l->filelen / sizeof(*in);
	cm.brushes = Hunk_Alloc( ( BOX_BRUSHES + count ) * sizeof( *cm.brushes ) );
	cm.numBrushes = count;
	out = cm.brushes;

	for (i = 0; i<count ; i++, out++, in++) 
	{
		out->sides = cm.brushsides + LittleLong(in->firstSide);
		out->numsides = LittleLong(in->numSides);

		out->shaderNum = LittleLong( in->materialNum );
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) 
		{
			Com_Error( ERR_DROP, __FUNCTION__": bad materialNum : % i", out->shaderNum );
		}

		out->contents = cm.shaders[out->shaderNum].contentFlags;

		CM_BoundBrush(out);
	}

}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs(lump_t *l)
{
	q3bsp_leaf_t *in;
	cLeaf_t *out;
	int i, count;
	
	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}

	count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		Com_Error(ERR_DROP, "Map with no leafs");
	}

	cm.leafs = Hunk_Alloc( ( BOX_LEAFS + count ) * sizeof( *cm.leafs ) );
	cm.numLeafs = count;
	out = cm.leafs;	

	for (i = 0; i<count ; i++, in++, out++)
	{
		out->cluster = LittleLong(in->cluster);
		out->area = LittleLong(in->area);
		out->firstLeafBrush = LittleLong(in->firstLeafBrush);
		out->numLeafBrushes = LittleLong(in->numLeafBrushes);
		out->firstLeafSurface = LittleLong(in->firstLeafSurface);
		out->numLeafSurfaces = LittleLong(in->numLeafSurfaces);

		if (out->cluster >= cm.numClusters)
		{
			cm.numClusters = out->cluster + 1;
		}

		if (out->area >= cm.numAreas)
		{
			cm.numAreas = out->area + 1;
		}
	}

	cm.areas = Hunk_Alloc( cm.numAreas * sizeof( *cm.areas ) );
	cm.areaPortals = Hunk_Alloc( cm.numAreas * cm.numAreas * sizeof( *cm.areaPortals ) );
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes(lump_t *l)
{
	q3bsp_plane_t *in;
	cplane_t *out;
	int count, bits, i, j;
	
	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}

	count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		Com_Error(ERR_DROP, "Map with no planes");
	}

	cm.planes = Hunk_Alloc( ( BOX_PLANES + count ) * sizeof( *cm.planes ) );
	cm.numPlanes = count;
	out = cm.planes;	

	for (i = 0; i < count; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat(in->dist);
		out->type = PlaneTypeForNormal( out->normal );
		out->signbits = bits;
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes(lump_t *l)
{
	int			i;
	int			*out;
	int		 	*in;
	int			count;
	
	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}

	count = l->filelen / sizeof(*in);

	cm.leafbrushes = Hunk_Alloc( (count + BOX_BRUSHES) * sizeof( *cm.leafbrushes ) );
	cm.numLeafBrushes = count;

	out = cm.leafbrushes;

	for (i = 0; i < count; i++, in++, out++) 
	{
		*out = LittleLong(*in);
	}
}

/*
=================
CMod_LoadLeafSurfaces
=================
*/
static void CMod_LoadLeafSurfaces(lump_t *l)
{
	int *in, *out;
	int i, count;
	
	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny lump size");
	}
	count = l->filelen / sizeof(*in);

	cm.leafsurfaces = Hunk_Alloc( count * sizeof( *cm.leafsurfaces ) );
	cm.numLeafSurfaces = count;

	out = cm.leafsurfaces;

	for (i = 0; i < count; i++, in++, out++) 
	{
		*out = LittleLong (*in);
	}
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides(lump_t *l)
{
	q3bsp_brushside_t *in;
	cbrushside_t *out;
	int i, count, num;

	in = (void *)(cmod_base + l->fileofs);
	if ( l->filelen % sizeof(*in) ) 
	{
		Com_Error(ERR_DROP, __FUNCTION__": Funny lump size");
	}

	count = l->filelen / sizeof(*in);

	cm.brushsides = Hunk_Alloc( ( BOX_SIDES + count ) * sizeof( *cm.brushsides ) );
	cm.numBrushSides = count;

	out = cm.brushsides;	

	for (i = 0; i < count; i++, in++, out++) 
	{
		num = LittleLong(in->planeNum);
		out->plane = &cm.planes[num];

		out->shaderNum = LittleLong(in->materialNum);
		if ( out->shaderNum < 0 || out->shaderNum >= cm.numShaders ) 
		{
			Com_Error( ERR_DROP, __FUNCTION__": Bad material index: %i", out->shaderNum );
		}

		out->surfaceFlags = cm.shaders[out->shaderNum].surfaceFlags;
	}
}


/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString( lump_t *l ) 
{
	if(l->filelen <= 0)
	{
		Com_Error(ERR_DROP, __FUNCTION__": Map with no entities");
	}

	cm.entityString = Hunk_Alloc( l->filelen );
	cm.numEntityChars = l->filelen;
	memcpy (cm.entityString, cmod_base + l->fileofs, l->filelen);
}

/*
=================
CMod_LoadVisibility
=================
*/
#define	VIS_HEADER	8
static void CMod_LoadVisibility( lump_t *l ) 
{
	int		len;
	byte	*buf;

    len = l->filelen;
	if ( !len )
	{
		cm.clusterBytes = ( cm.numClusters + 31 ) & ~31;
		cm.visibility = Hunk_Alloc( cm.clusterBytes );
		memset( cm.visibility, 255, cm.clusterBytes );
		return;
	}
	buf = cmod_base + l->fileofs;

	cm.vised = true;
	cm.visibility = Hunk_Alloc( len );

	cm.numClusters = LittleLong( ((int *)buf)[0] );
	cm.clusterBytes = LittleLong( ((int *)buf)[1] );

	memcpy(cm.visibility, buf + VIS_HEADER, len - VIS_HEADER );
}

//==================================================================


/*
=================
CMod_LoadPatches
=================
*/
static void CMod_LoadPatches(lump_t *surfs, lump_t *verts) 
{
	q3bsp_surface_t* in;
	q3bsp_drawVert_t *dv, *dv_p;
	cPatch_t *patch;
	int c, i, j, count;
	int width, height;
	int shaderNum;
	vec3_t points[MAX_PATCH_VERTS]; // MOVE OFF STACK?

	in = (void *)(cmod_base + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny surfaces lump size");
	}

	cm.numSurfaces = count = surfs->filelen / sizeof(*in);
	cm.surfaces = Hunk_Alloc( cm.numSurfaces * sizeof( cm.surfaces[0] ) );

	dv = (void *)(cmod_base + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
	{
		Com_Error(ERR_DROP, __FUNCTION__": funny drawverts lump size");
	}

	// scan through all the surfaces, but only load patches, not planar faces
	for (i = 0 ; i < count ; i++, in++) 
	{
		if ( LittleLong( in->surfaceType ) != MST_PATCH ) 
		{
			continue;  // ignore other surfaces
		}

		// FIXME: check for non-colliding patches

		cm.surfaces[ i ] = patch = Hunk_Alloc( sizeof( *patch ) );

		// load the full drawverts onto the stack
		width = LittleLong( in->patchWidth );
		height = LittleLong( in->patchHeight );

		c = width * height;
		if ( c > MAX_PATCH_VERTS ) 
		{
			Com_Error( ERR_DROP, __FUNCTION__": MAX_PATCH_VERTS" );
		}

		dv_p = dv + LittleLong( in->firstVert );
		for ( j = 0 ; j < c ; j++, dv_p++ ) 
		{
			points[j][0] = LittleFloat( dv_p->xyz[0] );
			points[j][1] = LittleFloat( dv_p->xyz[1] );
			points[j][2] = LittleFloat( dv_p->xyz[2] );
		}

		shaderNum = LittleLong( in->materialNum );
		patch->contents = cm.shaders[shaderNum].contentFlags;
		patch->surfaceFlags = cm.shaders[shaderNum].surfaceFlags;

		// create the internal facet structure
		patch->pc = CM_GeneratePatchCollide( width, height, points );
	}
}

//==================================================================

unsigned CM_LumpChecksum(lump_t *lump) 
{
	return LittleLong (Com_BlockChecksum (cmod_base + lump->fileofs, lump->filelen));
}

unsigned CM_Checksum(q3bsp_header_t *header) 
{
	unsigned checksums[16];
	checksums[0] = CM_LumpChecksum(&header->lumps[Q3LUMP_SHADERS]);
	checksums[1] = CM_LumpChecksum(&header->lumps[Q3LUMP_LEAFS]);
	checksums[2] = CM_LumpChecksum(&header->lumps[Q3LUMP_LEAFBRUSHES]);
	checksums[3] = CM_LumpChecksum(&header->lumps[Q3LUMP_LEAFSURFACES]);
	checksums[4] = CM_LumpChecksum(&header->lumps[Q3LUMP_PLANES]);
	checksums[5] = CM_LumpChecksum(&header->lumps[Q3LUMP_BRUSHSIDES]);
	checksums[6] = CM_LumpChecksum(&header->lumps[Q3LUMP_BRUSHES]);
	checksums[7] = CM_LumpChecksum(&header->lumps[Q3LUMP_MODELS]);
	checksums[8] = CM_LumpChecksum(&header->lumps[Q3LUMP_NODES]);
	checksums[9] = CM_LumpChecksum(&header->lumps[Q3LUMP_SURFACES]);
	checksums[10] = CM_LumpChecksum(&header->lumps[Q3LUMP_DRAWVERTS]);

	return LittleLong(Com_BlockChecksum(checksums, 11 * 4));
}


byte* cmodel_base = NULL;
int cmodel_size;
/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
void CM_LoadMap( const char *name, qboolean clientload, int *checksum ) 
{
	q3bsp_header_t header;
	int *buf;
	int i, length;
	static unsigned	last_checksum;

	if ( !name || !name[0] ) 
	{
		Com_Error( ERR_DROP, __FUNCTION__": NULL name" );
	}
	

	cm_noAreas = Cvar_Get("cm_noAreas", "0", CVAR_CHEAT, NULL);
	cm_noCurves = Cvar_Get("cm_noCurves", "0", CVAR_CHEAT, NULL);
	cm_playerCurveClip = Cvar_Get("cm_playerCurveClip", "1", CVAR_ARCHIVE|CVAR_CHEAT, NULL);
	
	Com_Printf( __FUNCTION__"( %s, %i )\n", name, clientload );

	if ( !strcmp( cm.name, name ) && clientload ) 
	{
		*checksum = last_checksum;
		return;
	}

	// free old stuff
	memset( &cm, 0, sizeof( cm ) );
	CM_ClearLevelPatches();

	if (cmodel_base)
	{
		Hunk_Free(cmodel_base);
		cmodel_base = NULL;
		cmodel_size = 0;
	}
	
	if ( !name[0] ) 
	{
		cm.numLeafs = 1;
		cm.numClusters = 1;
		cm.numAreas = 1;
		cm.cmodels = Hunk_Alloc( sizeof( *cm.cmodels ) );
		*checksum = 0;
		return;
	}

	//
	// load the file
	//
	length = FS_LoadFile(name, (void**)&buf);
	if ( !buf ) 
	{
		Com_Error (ERR_DROP, "Couldn't load %s", name);
	}

	last_checksum = LittleLong (Com_BlockChecksum (buf, length));
	*checksum = last_checksum;

	header = *(q3bsp_header_t*)buf;
	for (i = 0; i<sizeof(q3bsp_header_t)/4; i++) 
	{
		((int *)&header)[i] = LittleLong ( ((int *)&header)[i]);
	}

	if (header.ident != WORLD_IDENT)
	{
		Com_Error(ERR_DROP, "CM_LoadMap: %s is not a map", name);
	}

	if ( header.version != WORLD_VERSION )
	{
		Com_Error (ERR_DROP, "CM_LoadMap: %s has wrong version number (%i should be %i)", name, header.version, WORLD_VERSION);
	}

	cmod_base = (byte *)buf;

	cmod_base = Hunk_Begin(1024 * 1024 * 8, "CModel");

	// load into heap
	CMod_LoadShaders( &header.lumps[Q3LUMP_SHADERS] );
	CMod_LoadLeafs (&header.lumps[Q3LUMP_LEAFS]);
	CMod_LoadLeafBrushes (&header.lumps[Q3LUMP_LEAFBRUSHES]);
	CMod_LoadLeafSurfaces (&header.lumps[Q3LUMP_LEAFSURFACES]);
	CMod_LoadPlanes (&header.lumps[Q3LUMP_PLANES]);
	CMod_LoadBrushSides (&header.lumps[Q3LUMP_BRUSHSIDES]);
	CMod_LoadBrushes (&header.lumps[Q3LUMP_BRUSHES]);
	CMod_LoadSubmodels (&header.lumps[Q3LUMP_MODELS]);
	CMod_LoadNodes (&header.lumps[Q3LUMP_NODES]);
	CMod_LoadEntityString (&header.lumps[Q3LUMP_ENTITIES]);
	CMod_LoadVisibility( &header.lumps[Q3LUMP_VISIBILITY] );
	CMod_LoadPatches( &header.lumps[Q3LUMP_SURFACES], &header.lumps[Q3LUMP_DRAWVERTS] );

	cmodel_size = Hunk_End();

	// we are NOT freeing the file, because it is cached for the ref
	FS_FreeFile (buf);

	CM_InitBoxHull();
	CM_FloodAreaConnections();

	// allow this to be cached if it is loaded by the server
	if ( !clientload ) 
	{
		strncpy( cm.name, name, sizeof(cm.name) );
	}
}

/*
==================
CM_ClearMap
==================
*/
void CM_ClearMap() 
{
	memset( &cm, 0, sizeof( cm ) );
	CM_ClearLevelPatches();
}

/*
==================
CM_ClipHandleToModel
==================
*/
cmodel_t *CM_ClipHandleToModel( clipHandle_t handle ) 
{
	if ( handle < 0 ) 
	{
		Com_Error( ERR_DROP, __FUNCTION__": bad handle %i", handle );
	}
	if ( handle < cm.numSubModels ) 
	{
		return &cm.cmodels[handle];
	}
	if ( handle == BOX_MODEL_HANDLE ) 
	{
		return &box_model;
	}
	if ( handle < MAX_SUBMODELS ) 
	{
		Com_Error( ERR_DROP, __FUNCTION__": bad handle %i < %i < %i", cm.numSubModels, handle, MAX_SUBMODELS );
	}

	Com_Error( ERR_DROP, __FUNCTION__": bad handle %i", handle + MAX_SUBMODELS );
	return NULL;

}

/*
==================
CM_EntityString
==================
*/
char* CM_EntityString()
{
	return cm.entityString;
}

/*
==================
CM_NumClusters
==================
*/
int CM_NumClusters()
{
	return cm.numClusters;
}

/*
==================
CM_NumInlineModels
==================
*/
int CM_NumInlineModels()
{
	return cm.numSubModels;
}

/*
==================
CM_InlineModel
==================
*/
clipHandle_t CM_InlineModel(int index) 
{
	if ( index < 0 || index >= CM_NumInlineModels())
	{
		Com_Error (ERR_DROP, __FUNCTION__": bad number");
	}
	return index;
}

/*
==================
CM_LeafCluster
==================
*/
int CM_LeafCluster(int leafnum) 
{
	if (leafnum < 0 || leafnum >= cm.numLeafs) 
	{
		Com_Error (ERR_DROP, __FUNCTION__": bad number");
	}
	return cm.leafs[leafnum].cluster;
}

/*
==================
CM_LeafCCM_LeafArealuster
==================
*/
int CM_LeafArea(int leafnum) 
{
	if ( leafnum < 0 || leafnum >= cm.numLeafs ) 
	{
		Com_Error (ERR_DROP, __FUNCTION__": bad number");
	}
	return cm.leafs[leafnum].area;
}

//=======================================================================

void SetPlaneSignbits(cplane_t* out) 
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
	cplane_t	*p;
	cbrushside_t	*s;

	box_planes = &cm.planes[cm.numPlanes];

	box_brush = &cm.brushes[cm.numBrushes];
	box_brush->numsides = 6;
	box_brush->sides = cm.brushsides + cm.numBrushSides;
	box_brush->contents = Q3CONTENTS_BODY;

	box_model.leaf.numLeafBrushes = 1;
//	box_model.leaf.firstLeafBrush = cm.numBrushes;
	box_model.leaf.firstLeafBrush = cm.numLeafBrushes;
	cm.leafbrushes[cm.numLeafBrushes] = cm.numBrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &cm.brushsides[cm.numBrushSides+i];
		s->plane = 	cm.planes + (cm.numPlanes+i*2+side);
		s->surfaceFlags = 0;

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

		SetPlaneSignbits( p );
	}	
}

/*
===================
CM_TempBoxModel

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
Capsules are handled differently though.
===================
*/
clipHandle_t CM_TempBoxModel(const vec3_t mins, const vec3_t maxs, int capsule) 
{

	VectorCopy( mins, box_model.mins );
	VectorCopy( maxs, box_model.maxs );

	if ( capsule ) 
	{
		return CAPSULE_MODEL_HANDLE;
	}

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

	VectorCopy(mins, box_brush->bounds[0]);
	VectorCopy(maxs, box_brush->bounds[1]);

	return BOX_MODEL_HANDLE;
}

/*
===================
CM_ModelBounds
===================
*/
void CM_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs) 
{
	cmodel_t *cmod;

	cmod = CM_ClipHandleToModel(model);
	VectorCopy(cmod->mins, mins);
	VectorCopy(cmod->maxs, maxs);
}


