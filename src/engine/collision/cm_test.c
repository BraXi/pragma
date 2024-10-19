/*
===========================================================================
PRAGMA Copyright (C) 2023-2024 BraXi.

This file is part of PRAGMA source code, a free open source game engine.
Quake II and Quake III Arena source code are Copyright (C) Id Software, Inc.
See the attached GNU General Public License v2 for more details.

Note: CModel implementation is mostly a direct copy of the one found in Quake III.
===========================================================================
*/

#include "cm_local.h"

/*
==================
CM_PointLeafnum_r
==================
*/
int CM_PointLeafnum_r( const vec3_t p, int num ) 
{
	float		d;
	cNode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = cm.nodes + num;
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

int CM_PointLeafnum( const vec3_t p ) 
{
	if ( !cm.numNodes ) {	// map not loaded
		return 0;
	}
	return CM_PointLeafnum_r (p, 0);
}


/*
======================================================================

LEAF LISTING

======================================================================
*/


void CM_StoreLeafs( leafList_t *ll, int nodenum ) 
{
	int		leafNum;

	leafNum = -1 - nodenum;

	// store the lastLeaf even if the list is overflowed
	if ( cm.leafs[ leafNum ].cluster != -1 ) 
	{
		ll->lastLeaf = leafNum;
	}

	if ( ll->count >= ll->maxcount)
	{
		ll->overflowed = true;
		return;
	}
	ll->list[ ll->count++ ] = leafNum;
}

void CM_StoreBrushes( leafList_t *ll, int nodenum ) 
{
	int			i, k;
	int			leafnum;
	int			brushnum;
	cLeaf_t		*leaf;
	cbrush_t	*b;

	leafnum = -1 - nodenum;

	leaf = &cm.leafs[leafnum];

	for ( k = 0 ; k < leaf->numLeafBrushes ; k++ ) 
	{
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];
		b = &cm.brushes[brushnum];
		
		if ( b->checkcount == cm.checkcount ) 
		{
			continue;	// already checked this brush in another leaf
		}

		b->checkcount = cm.checkcount;

		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( b->bounds[0][i] >= ll->bounds[1][i] || b->bounds[1][i] <= ll->bounds[0][i] ) 
			{
				break;
			}
		}
		
		if ( i != 3 ) 
		{
			continue;
		}

		if ( ll->count >= ll->maxcount) 
		{
			ll->overflowed = true;
			return;
		}

		((cbrush_t **)ll->list)[ ll->count++ ] = b;
	}

#if 0
	// store patches?
	for ( k = 0 ; k < leaf->numLeafSurfaces ; k++ ) 
	{
		patch = cm.surfaces[ cm.leafsurfaces[ leaf->firstleafsurface + k ] ];
		if ( !patch ) 
		{
			continue;
		}
	}
#endif
}

int BoxOnPlaneSideQ3(vec3_t emins, vec3_t emaxs, cplane_t* p)
{
	float	dist1, dist2;
	int		sides;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 1:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		break;
	case 2:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 3:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		break;
	case 4:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0] * emaxs[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emins[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0] * emins[0] + p->normal[1] * emins[1] + p->normal[2] * emins[2];
		dist2 = p->normal[0] * emaxs[0] + p->normal[1] * emaxs[1] + p->normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
void CM_BoxLeafnums_r( leafList_t *ll, int nodenum ) 
{
	cplane_t	*plane;
	cNode_t		*node;
	int			s;

	Com_Printf(__FUNCTION__"\n");
	while (1)
	{
		if (nodenum < 0) 
		{
			ll->storeLeafs( ll, nodenum );
			return;
		}
	
		node = &cm.nodes[nodenum];
		plane = node->plane;
		s = BoxOnPlaneSideQ3( ll->bounds[0], ll->bounds[1], plane );
		if (s == 1) 
		{
			nodenum = node->children[0];
		} 
		else if (s == 2) 
		{
			nodenum = node->children[1];
		}
		else 
		{
			// go down both
			CM_BoxLeafnums_r( ll, node->children[0] );
			nodenum = node->children[1];
		}

		Com_Printf("side: %i - nodenum %i\n", s, nodenum);

	}
}

/*
==================
CM_BoxLeafnums
==================
*/
int	CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *list, int listsize, int *lastLeaf) 
{
	leafList_t	ll;

	cm.checkcount++;

	VectorCopy( mins, ll.bounds[0] );
	VectorCopy( maxs, ll.bounds[1] );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = list;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf = 0;
	ll.overflowed = false;

	CM_BoxLeafnums_r( &ll, 0 );

	*lastLeaf = ll.lastLeaf;
	return ll.count;
}

/*
==================
CM_BoxBrushes
==================
*/
int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t **list, int listsize ) 
{
	leafList_t	ll;

	cm.checkcount++;

	VectorCopy( mins, ll.bounds[0] );
	VectorCopy( maxs, ll.bounds[1] );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = (void *)list;
	ll.storeLeafs = CM_StoreBrushes;
	ll.lastLeaf = 0;
	ll.overflowed = false;
	
	CM_BoxLeafnums_r( &ll, 0 );

	return ll.count;
}


//====================================================================


/*
==================
CM_PointContents
==================
*/
int CM_PointContents( const vec3_t p, clipHandle_t model ) 
{
	int			leafnum;
	int			i, k;
	int			brushnum;
	cLeaf_t		*leaf;
	cbrush_t	*b;
	int			contents;
	float		d;
	cmodel_t	*clipm;

	if (!cm.numNodes)
	{	
		// map not loaded
		return 0;
	}

	if ( model ) {
		clipm = CM_ClipHandleToModel( model );
		leaf = &clipm->leaf;
	} else {
		leafnum = CM_PointLeafnum_r (p, 0);
		leaf = &cm.leafs[leafnum];
	}

	contents = 0;
	for (k = 0; k<leaf->numLeafBrushes ; k++) 
	{
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];
		b = &cm.brushes[brushnum];

		// see if the point is in the brush
		for ( i = 0 ; i < b->numsides ; i++ ) 
		{
			d = DotProduct( p, b->sides[i].plane->normal );

// FIXME test for Cash
//			if ( d >= b->sides[i].plane->dist )
			if ( d > b->sides[i].plane->dist ) 
			{
				break;
			}
		}

		if ( i == b->numsides ) 
		{
			contents |= b->contents;
		}
	}

	return contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles) 
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if ( model != BOX_MODEL_HANDLE && (angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	return CM_PointContents( p_l, model );
}

/*
===============================================================================

PVS

===============================================================================
*/

byte *CM_ClusterPVS(int cluster) 
{
	if (cluster < 0 || cluster >= cm.numClusters || !cm.vised ) 
	{
		return cm.visibility;
	}

	return cm.visibility + cluster * cm.clusterBytes;
}
