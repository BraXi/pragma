/*
pragma
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// navigation.c - implementation of A* pathfinding alghoritm 
// https://en.wikipedia.org/wiki/A*_search_algorithm

#include "qcommon.h"

//#define DEBUG_PATHFINDING 1

void Nav_Init();
int Nav_AddPathNode(float x, float y, float z);
void Nav_AddPathNodeLink(int nodeId, int linkTo);
int Nav_GetNearestNode(vec3_t origin);
int Nav_SearchPath(int startWaypoint, int goalWaypoint);

#define NO_WAYPOINT -1
#define MAX_WAYPOINTS 1024
#define MAX_WAYPOINT_LINKS 8
#define TAG_NAV_NODES 1231

#define NAVFL_DISABLED 1		// node is temporarily disabled (ex. doors are locked)

typedef struct waypoint_s
{
	int wpIdx;

	vec3_t origin;
//	int flags;

	int linkCount;
	int links[MAX_WAYPOINT_LINKS];
} waypoint_t;

typedef struct pathnode_s pathnode_t;
typedef struct pathnode_s
{
	int wpIdx;	// index to nav.waypoints[]
	float g;	// distance between the current node and the start node
	float f;	// total cost of the node
	float h;	// estimated distance from the current node to the end node (heuristic)
	pathnode_t* parent;
} pathnode_t;


typedef struct nav_s
{
	waypoint_t* waypoints;
	int waypoints_count;

	int openNodesSize;
	int closedNodesSize;
} nav_t;

static nav_t nav;

static pathnode_t null_pathnode;
static pathnode_t *openNodes[MAX_WAYPOINTS];
static pathnode_t *closedNodes[MAX_WAYPOINTS];

/*
=================
distance3d

returns the distance between two vectors
=================
*/
static float distance3d(vec3_t p1, vec3_t p2)
{
	vec3_t temp;
	VectorSubtract(p1, p2, temp);
	return VectorLength(temp);
}

/*
=================
distance3dsquared

returns the squared distance between two vectors
=================
*/
static float distance3dsquared(vec3_t p1, vec3_t p2)
{
	float dx = p1[0] - p2[0];
	float dy = p1[1] - p2[1];
	float dz = p1[2] - p2[2];
	return dx * dx + dy * dy + dz * dz;
}


/*
=================
Nav_IsInitialized
=================
*/
static qboolean Nav_IsInitialized()
{
	if (nav.waypoints == NULL)
		return false;
	return true;
}

/*
=================
Nav_GetNodesCount
=================
*/
int Nav_GetNodesCount()
{
	return nav.waypoints_count;
}


/*
=================
Nav_GetNodeLinkCount
=================
*/
int Nav_GetNodeLinkCount(int node)
{
	if (!Nav_IsInitialized())
		return 0;

	if (node < 0 || node >= nav.waypoints_count)
		return 0;

	return nav.waypoints[node].linkCount;
}

/*
=================
Nav_GetNodeLink
=================
*/
int Nav_GetNodeLink(int node, int link)
{
	if (!Nav_GetNodeLinkCount(node))
		return NO_WAYPOINT;

	if (link < 0 || link >= nav.waypoints[node].linkCount)
		return NO_WAYPOINT;

	return nav.waypoints[node].links[link];
}

/*
=================
Nav_Init

initialize naviagation system
=================
*/
void Nav_Init()
{
	if (Nav_IsInitialized())
	{
		Z_Free(nav.waypoints);
		nav.waypoints = NULL;
	}

	null_pathnode.wpIdx = NO_WAYPOINT;

	memset(openNodes, 0, sizeof(openNodes));
	memset(closedNodes, 0, sizeof(closedNodes));
	memset(&nav, 0, sizeof(nav));

	nav.waypoints_count = 0;
	nav.waypoints = Z_Malloc(MAX_WAYPOINTS * sizeof(waypoint_t));

	for (int i = 0; i < MAX_WAYPOINTS; i++)
	{
		nav.waypoints[i].wpIdx = NO_WAYPOINT;
		for (int j = 0; j < MAX_WAYPOINT_LINKS; j++)
			nav.waypoints[i].links[j] = NO_WAYPOINT;
	}
	Com_Printf("Nav_Init: allocated space for %d waypoints\n", MAX_WAYPOINTS);
}

/*
=================
Nav_Shutdown
=================
*/
void Nav_Shutdown()
{
	if (!Nav_IsInitialized())
		return;

	memset(openNodes, 0, sizeof(openNodes));
	memset(closedNodes, 0, sizeof(closedNodes));
	memset(&nav, 0, sizeof(nav));

	nav.waypoints_count = 0;

	if (nav.waypoints != NULL)
	{
		Z_Free(nav.waypoints);
		nav.waypoints = NULL;
	}

	Z_FreeTags(TAG_NAV_NODES);
}

/*
=================
Nav_AddPathNode

Inserts pathnode at given position, returns index to waypoint
=================
*/
int Nav_AddPathNode(float x, float y, float z)
{
	if (nav.waypoints_count >= MAX_WAYPOINTS || nav.waypoints_count < 0)
		return NO_WAYPOINT;

	nav.waypoints[nav.waypoints_count].wpIdx = nav.waypoints_count;
	nav.waypoints[nav.waypoints_count].origin[0] = x;
	nav.waypoints[nav.waypoints_count].origin[1] = y;
	nav.waypoints[nav.waypoints_count].origin[2] = z;
//	VectorCopy(nav.waypoints[nav.waypoints_count].origin, pos);

	nav.waypoints_count++;
	return (nav.waypoints_count - 1);
}

/*
=================
Nav_AddPathNodeLink

Connect waypoints
=================
*/
void Nav_AddPathNodeLink(int nodeId, int linkTo)
{
	if (nodeId >= MAX_WAYPOINTS || nodeId < 0)
		return;

	if (nav.waypoints[nodeId].linkCount >= MAX_WAYPOINT_LINKS)
		return;

	nodeId = nav.waypoints_count - 1;
	nav.waypoints[nodeId].links[nav.waypoints[nodeId].linkCount] = linkTo;
	nav.waypoints[nodeId].linkCount++;
}

/*
=================
 Nav_GetNodePos

returns wnode origin
=================
*/
void Nav_GetNodePos(int num, float *x, float *y, float *z)
{
	if (!Nav_IsInitialized() || num >= Nav_GetNodesCount() || num < 0)
	{
		*x = *y = *z = 0;
		return;
	}

	*x = nav.waypoints[num].origin[0];
	*y = nav.waypoints[num].origin[1];
	*z = nav.waypoints[num].origin[2];
}

/*
=================
Nav_GetNearestNode

returns index to waypoint that is closest to origin, or -1 if not found
=================
*/
int Nav_GetNearestNode(vec3_t origin)
{
	int nearestNode, nearestDist, i;
	float dist;

	nearestNode = NO_WAYPOINT;
	nearestDist = 9999999999;

	if (!Nav_IsInitialized() || !Nav_GetNodesCount())
		return nearestNode;


	// loop through all waypoints
	for (i = 0; i < nav.waypoints_count; i++)
	{
		// get the squared distance
		dist = distance3dsquared(origin, nav.waypoints[i].origin);

		// check if the distance is closer than the currently closest
		if (dist < nearestDist)
		{
			// memorize the current waypoint
			nearestDist = dist;
			nearestNode = i;
		}
	}	
	return nearestNode; // return the ID of the closest waypoint
}



/*
=================
Nav_DebugDrawNodes
=================
*/
#ifdef DEBUG_PATHFINDING
extern void SV_AddDebugLine(vec3_t p1, vec3_t p2, vec3_t color, float thickness, float drawtime, qboolean depthtested);
extern void SV_AddDebugPoint(vec3_t pos, vec3_t color, float thickness, float drawtime, qboolean depthtested);
extern void SV_AddDebugBox(vec3_t pos, vec3_t p1, vec3_t p2, vec3_t color, float thickness, float drawtime, qboolean depthtested);
extern void SV_AddDebugString(vec3_t pos, vec3_t color, float fontSize, float drawtime, qboolean depthtested, char* text);
#endif
static void Nav_DebugDrawNodes()
{
#ifdef DEBUG_PATHFINDING
	vec3_t p;
	vec3_t oldp;
	int i;


	vec3_t mins = { -16, 16, 0 };
	vec3_t maxs = { 16, 16, 56 };

	//memset(str, sizeof(str), 0);
	for (i = 0; i < nav.waypoints_count; i++)
	{
		vec3_t c = { 1,1,0 };
		SV_AddDebugPoint(nav.waypoints[i].origin, c, 2, 0.1, true);

		SV_AddDebugString(nav.waypoints[i].origin, c, 1.5, 0.1, false, va("node %i", i));

		for (int j = 0; j < nav.waypoints[i].linkCount; j++)
		{
			int chidx = nav.waypoints[i].links[j];

			vec3_t c = { 0,0,0.6 };
			SV_AddDebugLine(nav.waypoints[i].origin, nav.waypoints[chidx].origin, c, 1, 0.1, false);
		}
	}

	for (i = 0; i < nav.openNodesSize; i++)
	{
		vec3_t c = { 0,1,0 };
		VectorCopy(nav.waypoints[openNodes[i]->wpIdx].origin, p);
		p[2] += 48;
		SV_AddDebugLine(nav.waypoints[openNodes[i]->wpIdx].origin, p,c, 4, 0.1, false);
	}

	for (i = 0; i < nav.closedNodesSize; i++)
	{
		vec3_t c = { 1,0,0 };
//		if (i > 0)
//			SV_AddDebugLine(nav.waypoints[closedNodes[i].wpIdx].origin, oldp, c, 2, 0.1, false);

		VectorCopy(nav.waypoints[closedNodes[i]->wpIdx].origin, p);
		p[2] += 48;

		SV_AddDebugLine(nav.waypoints[closedNodes[i]->wpIdx].origin, p, c, 4, 0.1, false);
		VectorCopy(nav.waypoints[closedNodes[i]->wpIdx].origin, oldp);
	}

	printf("======\n");
	for (i = 0; i < nav.openNodesSize; i++)
		printf("openNodes[%i] = %i\n", i, openNodes[i]->wpIdx);

	for (i = 0; i < nav.closedNodesSize; i++)
		printf("closedNodes[%i] = %i\n", i, closedNodes[i]->wpIdx);
	printf("======\n");
#endif
}


/*
=================
Nav_NodeExistsInList
=================
*/
static qboolean Nav_NodeExistsInList(pathnode_t* nodes[], int num, int listSize)
{
	for (int i = 0; i < listSize; i++)
	{
		if (nodes[i]->wpIdx == num)
			return true;
	}
	return false;
}

/*
=================
Nav_AllocNode
=================
*/
static pathnode_t* Nav_AllocNode()
{
	pathnode_t* n = Z_TagMalloc(sizeof(pathnode_t), TAG_NAV_NODES);
	n->wpIdx = NO_WAYPOINT;
	return  n;
}

/*
=================
Nav_SearchPath

Returns the next node on a path from one waypoint to another, 
which will be index to nav.waypoints, returns -1 if there's no route.

startWaypoint - index to the waypoint we're currently at
goalWaypoint - index to the waypoint we'd like to go

=================
*/
int Nav_SearchPath(int startWaypoint, int goalWaypoint) 
{
	pathnode_t *startnode;
	int bestNode, highestPriority;

	if (!Nav_IsInitialized() || !Nav_GetNodesCount())
		return NO_WAYPOINT; // for maps without pathnodes

	if (startWaypoint < 0)
	{
		Com_Printf("%s: startWaypoint < 0\n", __FUNCTION__); 
		//Com_Error(ERR_DROP, "%s: startWaypoint < 0\n", __FUNCTION__);
		return NO_WAYPOINT;
	}

	if (goalWaypoint >= nav.waypoints_count)
	{
		Com_Printf("%s: goalWaypoint %i >= %i nav.waypoints_count\n", __FUNCTION__, goalWaypoint, nav.waypoints_count); 
		//Com_Error(ERR_DROP, "%s: goalWaypoint %i >= %i nav.waypoints_count\n", __FUNCTION__, goalWaypoint, nav.waypoints_count);
		return NO_WAYPOINT;
	}

#if 1
	Z_FreeTags(TAG_NAV_NODES);
	// is this too paranoid?
	memset(openNodes, 0, sizeof(openNodes));
	memset(closedNodes, 0, sizeof(closedNodes));
#endif

	nav.openNodesSize = 0;
	nav.closedNodesSize = 0;

	// create start node and add it to openNodes list
	startnode = Nav_AllocNode();
	startnode->g = 0;
	startnode->h = distance3d(nav.waypoints[startWaypoint].origin, nav.waypoints[goalWaypoint].origin);
	startnode->f = startnode->g + startnode->h;
	startnode->wpIdx = startWaypoint;
	startnode->parent = &null_pathnode;
	openNodes[nav.openNodesSize++] = startnode; 

	while(nav.openNodesSize > 0)
	{
		// remove node n with lowest f from openNodes
		bestNode = NO_WAYPOINT;
		highestPriority = 9999999999;
		pathnode_t *n = openNodes[0];

		for (int i = 0; i < nav.openNodesSize; i++) 
		{
			if (openNodes[i]->f < highestPriority) 
			{
				bestNode = i;
				highestPriority = openNodes[i]->f;
			}
		}

		if (bestNode != NO_WAYPOINT)
		{
			// remove node from openNodes
			n = openNodes[bestNode];
			//openNodes[bestNode]->wpIdx = NO_WAYPOINT;
			for (int i = bestNode; i < nav.openNodesSize - 1; i++) 
			{
				openNodes[i] = openNodes[i + 1];
			}
			nav.openNodesSize--;
		}
		else 
		{
			Nav_DebugDrawNodes();
			return NO_WAYPOINT;
		}

		// if n is goal node, build path and return success
		if (n->wpIdx == goalWaypoint) 
		{
			pathnode_t *x = n;
			for (int j = 0; j < 1000; j++) // TODO - isn't 1000 to little? 
			{
				pathnode_t* parent = x->parent;
				if (parent == NULL || parent->parent == NULL || parent->parent->wpIdx == NO_WAYPOINT)
				{
					Nav_DebugDrawNodes();
//					printf("goal node!\n");
					return x->wpIdx;
				}
				x = parent;
			}
			Nav_DebugDrawNodes();
			return NO_WAYPOINT;
		}

		// for all neightbors (nc) of node n
		for (int i = 0; i < nav.waypoints[n->wpIdx].linkCount; i++)
		{
			int newg = n->g + distance3d(nav.waypoints[n->wpIdx].origin, nav.waypoints[nav.waypoints[n->wpIdx].links[i]].origin);

			if (Nav_NodeExistsInList(openNodes, nav.waypoints[n->wpIdx].links[i], nav.openNodesSize)) 
			{
				// skip if nc exists in openNodes and nc.g <= newg
				pathnode_t *nc = Nav_AllocNode(); 
				for (int p = 0; p < nav.openNodesSize; p++) 
				{
					if (openNodes[p]->wpIdx == nav.waypoints[n->wpIdx].links[i]) 
					{
						nc = openNodes[p];
						break;
					}
				}

				if (nc->g <= newg) 
				{
					continue;
				}
			}
			else if (Nav_NodeExistsInList(closedNodes, nav.waypoints[n->wpIdx].links[i], nav.closedNodesSize)) 
			{
				// skip if nc is in closedNodes and nc.g <= newg
				pathnode_t *nc = Nav_AllocNode();
				for (int p = 0; p < nav.closedNodesSize; p++) 
				{
					if (closedNodes[p]->wpIdx == nav.waypoints[n->wpIdx].links[i]) 
					{
						nc = closedNodes[p];
						break;
					}
				}

				if (nc->g <= newg) 
				{
					continue;
				}
			}

			pathnode_t *nc = Nav_AllocNode();
			nc->parent = n;
			nc->g = newg;
			nc->h = distance3d(nav.waypoints[nav.waypoints[n->wpIdx].links[i]].origin, nav.waypoints[goalWaypoint].origin);
			nc->f = nc->g + nc->h;
			nc->wpIdx = nav.waypoints[n->wpIdx].links[i];

			// if nc is in closedNodes, remove it from that list
			if (Nav_NodeExistsInList(closedNodes, nc->wpIdx, nav.closedNodesSize)) 
			{
				qboolean deleted = false;
				for (int p = 0; p < nav.closedNodesSize; p++) 
				{
					if (closedNodes[p]->wpIdx == nc->wpIdx) 
					{
						closedNodes[p]->wpIdx = NO_WAYPOINT;
						for (int x = p; x < nav.closedNodesSize - 1; x++)
							closedNodes[x] = closedNodes[x + 1];

						deleted = true;
						break;
					}

					if (deleted)
						break;
				}
				nav.closedNodesSize--;
			}

			// if nc doesn't exist in openNodes, add it to that list
			if (!Nav_NodeExistsInList(openNodes, nc->wpIdx, nav.openNodesSize)) 
			{
				openNodes[nav.openNodesSize++] = nc;
			}
		}

		// we're done with all links, push n onto closedNodes
		if (!Nav_NodeExistsInList(closedNodes, n->wpIdx, nav.closedNodesSize)) 
		{
			closedNodes[nav.closedNodesSize++] = n;
		}
	}
	Nav_DebugDrawNodes();
	return NO_WAYPOINT;
}

#if 0
/*
PRAGMA_NAVIGATION
$pathnodes_count NUM_WAYPOINTS
$pathnode INDEX POS_X POS_Y POS_Z NUM_LINKS INDEX_LINK1 INDEX_LINK2 ...
$pathnode INDEX POS_X POS_Y POS_Z NUM_LINKS INDEX_LINK1 INDEX_LINK2 ...
$pathnode INDEX POS_X POS_Y POS_Z NUM_LINKS INDEX_LINK1 INDEX_LINK2 ...
...
*/
void Nav_LoadWaypoints(char* mapname)
{
	char	fname[MAX_OSPATH];
	char	line[256];
	int		linenum = 0;
	int len;
	char* token;
	waypoint_t* wp;

	FILE* file;

	memset(nav.waypoints, 0, sizeof(nav.waypoints));
	nav.waypoints_count = 0;
	Com_sprintf(fname, sizeof(fname), "%s/nav/%s.nav", FS_Gamedir(), mapname);

	len = FS_FOpenFile(fname, &file);
	if (!len || len == -1)
	{
		Com_Printf("Missing navigation file: %s\n", fname);
		return;
	}

	linenum = 0;
	while (fgets(line, sizeof(line), file) != NULL)
	{
		linenum++;

		COM_TokenizeString(line);
		if (!COM_TokenNumArgs())
			continue;

		if (linenum == 1)
		{
			//expect: PRAGMA_NAVIGATION
			if (!Nav_ExpectDef("PRAGMA_NAVIGATION", 1))
			{
				FS_FCloseFile(file);
				Com_Error(ERR_DROP, "error parsing `%s` in line %i\n", fname);
				return;
			}
			continue;
		}
		else if (linenum == 2)
		{
			//expect: $pathnodes_count nav.waypoints_count
			if (!Nav_ExpectDef("$pathnodes_count", 2))
			{
				FS_FCloseFile(file);
				Com_Error(ERR_DROP, "error parsing `%s` in line %i\n", fname);
				return;
			}
			nav.waypoints_count = atoi(COM_TokenGetArg(1));
			continue;
		}

		//expect: $pathnode INDEX X Y Z NUM_LINKS
		if (!Nav_ExpectDef("$pathnode", 6))
		{
			FS_FCloseFile(file);
			Com_Error(ERR_DROP, "error parsing `%s` in line %i\n", fname);
			return;
		}
	}
}
#endif