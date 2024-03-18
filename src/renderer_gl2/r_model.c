/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_model.c -- model loading and caching

#include "r_local.h"

model_t	*loadmodel;
int		modfilelen;

void Mod_LoadSP2 (model_t *mod, void *buffer);
void Mod_LoadBSP (model_t *mod, void *buffer);
extern void Mod_LoadMD3 (model_t *mod, void *buffer, lod_t lod);

byte	mod_novis[MAX_MAP_LEAFS_QBSP/8];

#ifdef PROTOCOL_EXTENDED_ASSETS
	#define	MAX_MOD_KNOWN	1024
#else
	#define	MAX_MOD_KNOWN	512
#endif
model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

// the inline * models from the current map are kept seperate
model_t	mod_inline[MAX_MOD_KNOWN];

int		registration_sequence;


qboolean bExtendedBSP = false;
/*
** Mod_ForNum
*/
model_t* Mod_ForNum(int index) 
{
	model_t* mod;

	// out of range gets the defualt model
	if (index < 1 || index >= mod_numknown)
	{
		return &mod_known[0];
	}

	mod = &mod_known[index];

	return mod;
}


/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mnode_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!model)
		ri.Error (ERR_DROP, "%s: no model", __FUNCTION__);
	if (!model->nodes)
		ri.Error(ERR_DROP, "%s: model without nodes", __FUNCTION__);

	node = model->nodes;
	while (1)
	{
		if (node->contents != -1)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS_QBSP/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->vis->numclusters+7)>>3;	
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;		
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
	
	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_DecompressVis ( (byte *)model->vis + model->vis->bitofs[cluster][DVIS_PVS], model);
}


//===============================================================================

/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f (void)
{
	int		i;
	model_t	*mod;
	int		total;

	static char *mtypes[4] = { "BAD", "BSP", "SPRITE", "MD3" };

	total = 0;
	ri.Printf (PRINT_ALL,"Loaded models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		ri.Printf (PRINT_ALL, "%i: %s '%s' [%d frames, %d bytes]\n", i, mtypes[mod->type], mod->name, mod->numframes, mod->extradatasize);
		total += mod->extradatasize;
	}
	ri.Printf (PRINT_ALL, "\nTotal resident: %i\n", total);
	ri.Printf(PRINT_ALL, "Total %i out of %i models in use\n\n", i, MAX_MOD_KNOWN);
}

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
}



/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
	unsigned *buf;
	int		i;
	
	if (!name[0])
		ri.Error (ERR_DROP, "Mod_ForName: NULL name");
		
	//
	// inline models are grabbed only from worldmodel
	//
	if (name[0] == '*')
	{
		i = atoi(name+1);
		if (i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels)
			ri.Error (ERR_DROP, "bad inline model number");
		return &mod_inline[i];
	}

	//
	// search the currently loaded models
	//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (!strcmp (mod->name, name) )
			return mod;
	}
	
	//
	// find a free model slot spot
	//
	for (i = 0, mod=mod_known ; i< mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			break;	// free spot
	}

	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			ri.Error (ERR_DROP, "Mod_ForName: hit limit of %d models", MAX_MOD_KNOWN);
		mod_numknown++;
	}

	strcpy (mod->name, name);
	mod->index = i;

	//
	// load the file
	//
	modfilelen = ri.LoadFile(mod->name, &buf);
	if (!buf)
	{
		if (crash)
			ri.Error (ERR_DROP, "Mod_ForName: %s not found", mod->name);
		memset (mod->name, 0, sizeof(mod->name));
		return NULL;
	}
	
	loadmodel = mod;

	//
	// fill it in
	//


	// call the apropriate loader
	
	switch (LittleLong(*(unsigned *)buf))
	{
	case MD3_IDENT: /* Quake3 .md3 model */
		loadmodel->extradata = Hunk_Begin(0x400000); // is 4MB enuff?
		Mod_LoadMD3(mod, buf, LOD_HIGH);
		break;
		
	case SP2_IDENT: /* Quake2 .sp2 sprite */
		loadmodel->extradata = Hunk_Begin (0x10000);
		Mod_LoadSP2(mod, buf);
		break;
	
	case BSP_IDENT: /* Quake2 .bsp v38*/
		bExtendedBSP = false;
		loadmodel->extradata = Hunk_Begin (0x1000000);
		Mod_LoadBSP(mod, buf);
		break;

	case QBISM_IDENT: /* Qbism extended BSP */
		bExtendedBSP = true;
		loadmodel->extradata = Hunk_Begin(0x1000000*6);
		Mod_LoadBSP(mod, buf);
		break;

	default:
		ri.Error (ERR_DROP,"Mod_ForName: file %s is not a vaild model", mod->name);
		break;
	}

	loadmodel->extradatasize = Hunk_End();

	ri.FreeFile (buf);

	return mod;
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

byte	*mod_base;

/*
=================
CMod_ValidateBSPLump
=================
*/
static __inline void CMod_ValidateBSPLump(lump_t* l, bspDataType type, unsigned int* count, int minElemCount, const char* what, const char* func)
{
	unsigned int elemsize = ri.GetBSPElementSize(type, bExtendedBSP);
	unsigned int limit = ri.GetBSPLimit(type, bExtendedBSP);

	if (elemsize > 0)
		*count = l->filelen / elemsize;
	else
		*count = l->filelen;

	if (elemsize != 0 && (l->filelen % elemsize))
		ri.Error(ERR_DROP, "%s: incorrect size of a BSP %s lump", func, what);

	if (minElemCount > 0 && *count < minElemCount)
		ri.Error(ERR_DROP, "Map with no %s", what);

	if (*count >= limit)
		ri.Error(ERR_DROP, "Map has too many %s (%i), max is %s", what, count, limit);
}

/*
=================
Mod_LoadLightMaps
=================
*/
void Mod_LoadLightMaps(lump_t *l)
{
	int count;
	if (!l->filelen)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	CMod_ValidateBSPLump(l, BSP_LIGHTING, &count, 0, "lightmaps", __FUNCTION__);

	loadmodel->lightdata = Hunk_Alloc(l->filelen);	
	memcpy (loadmodel->lightdata, mod_base + l->fileofs, l->filelen);
}


/*
=================
Mod_LoadVisibility
=================
*/
static void Mod_LoadVisibility(lump_t *l)
{
	int		i, count;

	if (!l->filelen)
	{
		loadmodel->vis = NULL;
		return;
	}

	CMod_ValidateBSPLump(l, BSP_VISIBILITY, &count, 0, "visibility", __FUNCTION__);

	loadmodel->vis = Hunk_Alloc(l->filelen);	
	memcpy(loadmodel->vis, mod_base + l->fileofs, l->filelen);

	loadmodel->vis->numclusters = LittleLong (loadmodel->vis->numclusters);
	for (i = 0; i < loadmodel->vis->numclusters; i++)
	{
		loadmodel->vis->bitofs[i][0] = LittleLong (loadmodel->vis->bitofs[i][0]);
		loadmodel->vis->bitofs[i][1] = LittleLong (loadmodel->vis->bitofs[i][1]);
	}
}


/*
=================
Mod_LoadVerts
=================
*/
static void Mod_LoadVerts(lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	CMod_ValidateBSPLump(l, BSP_VERTS, &count, 4, "verts", __FUNCTION__);

	out = Hunk_Alloc(count * sizeof(*out));
	in = (void*)(mod_base + l->fileofs);

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}
}


/*
=================
Mod_LoadInlineModels
=================
*/
static void Mod_LoadInlineModels(lump_t *l)
{
	dmodel_t	*in;
	mmodel_t	*out;
	int			i, j, count;

	CMod_ValidateBSPLump(l, BSP_MODELS, &count, 1, "inline models", __FUNCTION__);

	in = (void*)(mod_base + l->fileofs);
	out = Hunk_Alloc(count * sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{	
			// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat(in->mins[j]) - 1;
			out->maxs[j] = LittleFloat(in->maxs[j]) + 1;
			out->origin[j] = LittleFloat(in->origin[j]);
		}
		out->radius = RadiusFromBounds(out->mins, out->maxs);
		out->headnode = LittleLong(in->headnode);
		out->firstface = LittleLong(in->firstface);
		out->numfaces = LittleLong(in->numfaces);
	}
}

/*
=================
Mod_LoadEdges
=================
*/
static void Mod_LoadEdges(lump_t *l)
{
	medge_t *out;
	int 	i, count;

	CMod_ValidateBSPLump(l, BSP_EDGES, &count, 1, "edges", __FUNCTION__);

	out = Hunk_Alloc((count + 1) * sizeof(*out));	
	
	loadmodel->edges = out;
	loadmodel->numedges = count;

	if (bExtendedBSP)
	{
		dedge_ext_t* in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			out->v[0] = (unsigned int)LittleLong(in->v[0]);
			out->v[1] = (unsigned int)LittleLong(in->v[1]);
		}
	}
	else
	{
		dedge_t* in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			out->v[0] = (unsigned short)LittleShort(in->v[0]);
			out->v[1] = (unsigned short)LittleShort(in->v[1]);
		}
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
static void Mod_LoadTexinfo(lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out, *step;
	int 	i, j, count;
	char	name[MAX_QPATH];
	int		next;

	CMod_ValidateBSPLump(l, BSP_TEXINFO, &count, 1, "textures", __FUNCTION__);

	in = (void *)(mod_base + l->fileofs);
	out = Hunk_Alloc(count*sizeof(*out));	

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j = 0; j < 8; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);

		out->flags = LittleLong (in->flags);
		next = LittleLong (in->nexttexinfo);
		if (next > 0)
			out->next = loadmodel->texinfo + next;
		else
		    out->next = NULL;

		// load up texture
		Com_sprintf(name, sizeof(name), "textures/%s.tga", in->texture);
		out->image = R_FindTexture(name, it_texture, true);
		if (!out->image)
		{
			ri.Printf(PRINT_ALL, "%s: couldn't load '%s'\n", __FUNCTION__, name);
			out->image = r_texture_missing;
		}

	}

	// count animation frames
	// todo - update for materials
	for (i = 0; i< count; i++)
	{
		out = &loadmodel->texinfo[i];
		out->numframes = 1;
		for (step = out->next ; step && step != out ; step=step->next)
			out->numframes++;
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
static void CalcSurfaceExtents(msurface_t *s)
{
	float	mins[2], maxs[2];
	long double val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		
		for (j = 0; j < 2; j++)
		{
			val = (long double)v->position[0] * (long double)tex->vecs[j][0] +
				(long double)v->position[1] * (long double)tex->vecs[j][1] +
				(long double)v->position[2] * (long double)tex->vecs[j][2] +
				(long double)tex->vecs[j][3];

			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i = 0; i < 2; i++)
	{	
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256 */ )
//			ri.Error (ERR_DROP, "Bad surface extents");
	}
}


void GL_BuildPolygonFromSurface(msurface_t *fa);
void GL_CreateSurfaceLightmap (msurface_t *surf);
void GL_EndBuildingLightmaps(void);
void GL_BeginBuildingLightmaps(model_t *m);

/*
=================
Mod_LoadFaces
=================
*/
static void Mod_LoadFaces(lump_t *l)
{
	msurface_t 	*out;
	int			i, count, surfnum, side;
	unsigned int	planenum;
	int			ti;

	CMod_ValidateBSPLump(l, BSP_FACES, &count, 1, "faces", __FUNCTION__);
	
	out = Hunk_Alloc(count*sizeof(*out));	

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	GL_BeginBuildingLightmaps (loadmodel);

	if (bExtendedBSP)
	{
		dface_ext_t* in = (void*)(mod_base + l->fileofs);
		for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
		{
			out->firstedge = LittleLong(in->firstedge);
			out->numedges = LittleLong(in->numedges);
			out->flags = 0;
			out->polys = NULL;

			planenum = LittleLong(in->planenum);
			side = LittleLong(in->side);
			if (side)
				out->flags |= SURF_PLANEBACK;

			out->plane = loadmodel->planes + planenum;

			ti = LittleLong(in->texinfo);
			if (ti < 0 || ti >= loadmodel->numtexinfo)
				ri.Error(ERR_DROP, "%s: bad texture info number", __FUNCTION__);
			out->texinfo = loadmodel->texinfo + ti;

			CalcSurfaceExtents(out);

			// lighting info
			for (i = 0; i < MAXLIGHTMAPS; i++)
				out->styles[i] = in->styles[i];

			i = LittleLong(in->lightofs);
			if (i == -1)
				out->samples = NULL;
			else
				out->samples = loadmodel->lightdata + i;

			// set the drawing flags
			if (out->texinfo->flags & SURF_WARP)
			{
				out->flags |= SURF_DRAWTURB;
				for (i = 0; i < 2; i++)
				{
					out->extents[i] = 16384;
					out->texturemins[i] = -8192;
				}
				GL_SubdivideSurface(out);	// cut up polygon for warps
			}

			// create lightmaps and polygons
			if (!(out->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
				GL_CreateSurfaceLightmap(out);

			if (!(out->texinfo->flags & SURF_WARP))
				GL_BuildPolygonFromSurface(out);
		}

	}
	else
	{
		dface_t* in = (void*)(mod_base + l->fileofs);
		for (surfnum = 0; surfnum < count; surfnum++, in++, out++)
		{
			out->firstedge = LittleLong(in->firstedge);
			out->numedges = LittleShort(in->numedges);
			out->flags = 0;
			out->polys = NULL;

			planenum = LittleShort(in->planenum);
			side = LittleShort(in->side);
			if (side)
				out->flags |= SURF_PLANEBACK;

			out->plane = loadmodel->planes + planenum;

			ti = LittleShort(in->texinfo);
			if (ti < 0 || ti >= loadmodel->numtexinfo)
				ri.Error(ERR_DROP, "%s: bad texture info number", __FUNCTION__);
			out->texinfo = loadmodel->texinfo + ti;

			CalcSurfaceExtents(out);

			// lighting info
			for (i = 0; i < MAXLIGHTMAPS; i++)
				out->styles[i] = in->styles[i];

			i = LittleLong(in->lightofs);
			if (i == -1)
				out->samples = NULL;
			else
				out->samples = loadmodel->lightdata + i;

			// set the drawing flags
			if (out->texinfo->flags & SURF_WARP)
			{
				out->flags |= SURF_DRAWTURB;
				for (i = 0; i < 2; i++)
				{
					out->extents[i] = 16384;
					out->texturemins[i] = -8192;
				}
				GL_SubdivideSurface(out);	// cut up polygon for warps
			}

			// create lightmaps and polygons
			if (!(out->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
				GL_CreateSurfaceLightmap(out);

			if (!(out->texinfo->flags & SURF_WARP))
				GL_BuildPolygonFromSurface(out);
		}
	}


	GL_EndBuildingLightmaps ();
}


/*
=================
Mod_SetParent
=================
*/
static void Mod_SetParent(mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent(node->children[0], node);
	Mod_SetParent(node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	mnode_t 	*out;

	CMod_ValidateBSPLump(l, BSP_NODES, &count, 1, "nodes", __FUNCTION__);
	out = Hunk_Alloc( count*sizeof(*out) );	
	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	if (bExtendedBSP)
	{
		dnode_ext_t* in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			for (j = 0; j < 3; j++)
			{
				out->mins[j] = LittleFloat(in->mins[j]);
				out->maxs[j] = LittleFloat(in->maxs[j]);
			}

			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = (unsigned int)LittleLong(in->firstface);
			out->numsurfaces = (unsigned int)LittleLong(in->numfaces);
			out->contents = -1;	// differentiate from leafs

			for (j = 0; j < 2; j++)
			{
				p = LittleLong(in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	else
	{
		dnode_t* in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			for (j = 0; j < 3; j++)
			{
				out->mins[j] = LittleShort(in->mins[j]);
				out->maxs[j] = LittleShort(in->maxs[j]);
			}

			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = LittleShort(in->firstface);
			out->numsurfaces = LittleShort(in->numfaces);
			out->contents = -1;	// differentiate from leafs

			for (j = 0; j < 2; j++)
			{
				p = LittleLong(in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t*)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (lump_t *l)
{
	mleaf_t 	*out;
	int			i, j, count;
//	glpoly_t	*poly;

	CMod_ValidateBSPLump(l, BSP_LEAFS, &count, 1, "leafs", __FUNCTION__);
	
	out = Hunk_Alloc(count*sizeof(*out));	

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	if (bExtendedBSP)
	{
		dleaf_ext_t* in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			for (j = 0; j < 3; j++)
			{
				out->mins[j] = LittleFloat(in->mins[j]);
				out->maxs[j] = LittleFloat(in->maxs[j]);
			}

			out->contents = LittleLong(in->contents);
			out->cluster = LittleLong(in->cluster);
			out->area = LittleLong(in->area);

			out->firstmarksurface = loadmodel->marksurfaces + (unsigned int)LittleLong(in->firstleafface);
			out->nummarksurfaces = (unsigned int)LittleLong(in->numleaffaces);
		}
	}
	else
	{
		dleaf_t* in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++, in++, out++)
		{
			for (j = 0; j < 3; j++)
			{
				out->mins[j] = LittleShort(in->mins[j]);
				out->maxs[j] = LittleShort(in->maxs[j]);
			}

			out->contents = LittleLong(in->contents);
			out->cluster = LittleShort(in->cluster);
			out->area = LittleShort(in->area);

			out->firstmarksurface = loadmodel->marksurfaces + LittleShort(in->firstleafface);
			out->nummarksurfaces = LittleShort(in->numleaffaces);
		}
	}
		
#if 0
		// for (currently not used) underwater warp
		if ( out->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA) )
		{
			for (j = 0; j < out->nummarksurfaces; j++)
			{
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				for (poly = out->firstmarksurface[j]->polys; poly; poly = poly->next)
					poly->flags |= SURF_UNDERWATER;
			}
		}
#endif	
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces(lump_t *l)
{	
	int		i, j, count;
	msurface_t **out;
	
	CMod_ValidateBSPLump(l, BSP_LEAFFACES, &count, 1, "leaf faces", __FUNCTION__);
	out = Hunk_Alloc(count*sizeof(*out));	

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	if (bExtendedBSP)
	{
		unsigned int *in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++)
		{
			j = LittleLong(in[i]);
			if (j < 0 || j >= loadmodel->numsurfaces)
				ri.Error(ERR_DROP, "%s: bad surface number", __FUNCTION__);
			out[i] = loadmodel->surfaces + j;
		}
	}
	else
	{
		short* in = (void*)(mod_base + l->fileofs);
		for (i = 0; i < count; i++)
		{
			j = LittleShort(in[i]);
			if (j < 0 || j >= loadmodel->numsurfaces)
				ri.Error(ERR_DROP, "%s: bad surface number", __FUNCTION__);
			out[i] = loadmodel->surfaces + j;
		}
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (lump_t *l)
{	
	int		i, count;
	int		*in, *out;
	
	CMod_ValidateBSPLump(l, BSP_SURFEDGES, &count, 1, "surface edges", __FUNCTION__);
	in = (void *)(mod_base + l->fileofs);
	out = Hunk_Alloc ( count*sizeof(*out));	

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for (i = 0; i < count; i++)
		out[i] = LittleLong(in[i]);
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (lump_t *l)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error (ERR_DROP, "Mod_LoadPlanes: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*2*sizeof(*out));	
	
	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}
}

/*
=================
Mod_LoadBSP
=================
*/
void Mod_LoadBSP(model_t *mod, void *buffer)
{
	int			i;
	dheader_t	*header;
	mmodel_t 	*bm;
	
	loadmodel->type = MOD_BRUSH;
	if (loadmodel != mod_known)
		ri.Error (ERR_DROP, "Mod_LoadBSP: Loaded a brush model after the world");

	header = (dheader_t *)buffer;

	i = LittleLong (header->version);
	if (i != BSP_VERSION)
		ri.Error (ERR_DROP, "Mod_LoadBSP: %s is wrong version", mod->name);

// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	pCurrentModel = loadmodel;

// load into heap
	Mod_LoadVerts (&header->lumps[LUMP_VERTEXES]);
	Mod_LoadEdges (&header->lumps[LUMP_EDGES]);
	Mod_LoadSurfedges (&header->lumps[LUMP_SURFEDGES]);
	Mod_LoadLightMaps (&header->lumps[LUMP_LIGHTING]);
	Mod_LoadPlanes (&header->lumps[LUMP_PLANES]);
	Mod_LoadTexinfo (&header->lumps[LUMP_TEXINFO]);
	Mod_LoadFaces (&header->lumps[LUMP_FACES]);
	Mod_LoadMarksurfaces (&header->lumps[LUMP_LEAFFACES]);
	Mod_LoadVisibility (&header->lumps[LUMP_VISIBILITY]);
	Mod_LoadLeafs (&header->lumps[LUMP_LEAFS]);
	Mod_LoadNodes (&header->lumps[LUMP_NODES]);
	Mod_LoadInlineModels (&header->lumps[LUMP_MODELS]);
	mod->numframes = 2;		// regular and alternate animation
	
//
// set up the submodels
//
	for (i = 0; i < mod->numsubmodels; i++)
	{
		model_t	*starmod;

		bm = &mod->submodels[i];
		starmod = &mod_inline[i];

		*starmod = *loadmodel;
		
		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;

		if (starmod->firstnode >= loadmodel->numnodes)
			ri.Error (ERR_DROP, "Mod_LoadBSP: Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;
	
		if (i == 0)
			*loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}
}

/*
==============================================================================

QUAKE2 SP2 MODELS

==============================================================================
*/

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSP2 (model_t *mod, void *buffer)
{
	sp2Header_t	*sprin, *sprout;
	int			i;

	sprin = (sp2Header_t *)buffer;
	sprout = Hunk_Alloc (modfilelen);

	sprout->ident = LittleLong (sprin->ident);
	sprout->version = LittleLong (sprin->version);
	sprout->numframes = LittleLong (sprin->numframes);

	if (sprout->version != SP2_VERSION)
		ri.Error (ERR_DROP, "%s has wrong version number (%i should be %i)",
				 mod->name, sprout->version, SP2_VERSION);

	if (sprout->numframes > 32)
		ri.Error (ERR_DROP, "%s has too many frames (%i > %i)",
				 mod->name, sprout->numframes, 32);

	// byte swap everything
	for (i=0 ; i<sprout->numframes ; i++)
	{
		sprout->frames[i].width = LittleLong (sprin->frames[i].width);
		sprout->frames[i].height = LittleLong (sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong (sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong (sprin->frames[i].origin_y);
		memcpy (sprout->frames[i].name, sprin->frames[i].name, MAX_QPATH);
		mod->images[i] = R_FindTexture (sprout->frames[i].name, it_sprite, true);
	}

	mod->type = MOD_SPRITE;
}

//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration (char *model)
{
	char	fullname[MAX_QPATH];
	cvar_t	*flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;		// force markleafs

	Com_sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = ri.Cvar_Get ("flushmap", "0", 0);
	if ( strcmp(mod_known[0].name, fullname) || flushmap->value)
		Mod_Free (&mod_known[0]);
	r_worldmodel = Mod_ForName(fullname, true);

	r_viewcluster = -1;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
struct model_s *R_RegisterModel (char *name)
{
	model_t	*mod;
	int		i, j;
	sp2Header_t		*sp2Header;
	md3Header_t		*md3Header;

	md3Surface_t* surf;
	md3Shader_t* shader;

	mod = Mod_ForName(name, false);
	if (mod)
	{
		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if (mod->type == MOD_SPRITE)
		{
			sp2Header = (sp2Header_t *)mod->extradata;
			mod->numframes = sp2Header->numframes;
			for(i = 0; i< sp2Header->numframes; i++)
				mod->images[i] = R_FindTexture (sp2Header->frames[i].name, it_sprite, true);
		}
		else if (mod->type == MOD_MD3)
		{
			md3Header = mod->md3[LOD_HIGH];
			mod->numframes = md3Header->numFrames;
			surf = (md3Surface_t*)((byte*)md3Header + md3Header->ofsSurfaces);
			for (i = 0; i < md3Header->numSurfaces; i++)
			{
				shader = (md3Shader_t*)((byte*)surf + surf->ofsShaders);
				for (j = 0; j < surf->numShaders; j++, shader++)
				{
					mod->images[i+j] = R_FindTexture(shader->name, it_model, true);
					shader->shaderIndex = mod->images[i]->texnum;
				}
				surf = (md3Surface_t*)((byte*)surf + surf->ofsEnd);
			}
		}
		else if (mod->type == MOD_BRUSH)
		{
			for (i=0 ; i<mod->numtexinfo; i++)
				mod->texinfo[i].image->registration_sequence = registration_sequence;
		}
	}
	return mod;
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration (void)
{
	int		i;
	model_t	*mod;

	for (i=0, mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence != registration_sequence)
		{	// don't need this model
			Mod_Free (mod);
		}
	}

	R_FreeUnusedTextures ();
}


//=============================================================================


/*
================
Mod_Free
================
*/
void Mod_Free(model_t* mod)
{
	for (int i = 0; i < MD3_MAX_SURFACES; i++)
	{
		R_FreeVertexBuffer(mod->vb[i]);
	}

	if (mod->extradata)
		Hunk_Free(mod->extradata);

#if 0
	if (mod->type == MOD_MD3)
	{
		for (int i = 0; i < NUM_LODS; i++)
			if (mod->md3[i] != NULL)
			{
				printf("how is this not null\n");
				mod->md3[i] = NULL;
			}
	}
#endif
	memset (mod, 0, sizeof(*mod));
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll (void)
{
	int		i;

	for (i=0 ; i<mod_numknown ; i++)
	{
		if (mod_known[i].extradatasize)
			Mod_Free (&mod_known[i]);
	}
}
