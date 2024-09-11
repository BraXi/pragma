
#ifndef  _SMD_H_

typedef enum { SMD_MODEL, SMD_ANIMATION } smdtype_t; // What was loaded from file and what we expect to load

typedef struct smd_bone_s
{
	int index;
	int parent;		// parent bone index index, -1 == root bone
	char name[MAX_QPATH];
} smd_bone_t;

typedef struct smd_vert_s
{
	vec3_t	xyz;
	vec3_t	normal;
	vec2_t	uv;
	int	bone;
} smd_vert_t;

typedef struct smd_bonetransform_s
{
	int bone;
	vec3_t position;
	vec3_t rotation;
} smd_bonetransform_t;

typedef struct smd_surface_s
{
	int firstvert;
	int numverts;			// firstvert + numverts = triangles
	char texture[MAX_QPATH];
} smd_surface_t;

typedef struct smddata_s
{
	char name[MAX_QPATH];
	smdtype_t type;

	// data loaded or derived from .smd file
	int numbones;
	int numframes;
	int numverts;
	int numsurfaces;

	vec3_t mins, maxs;
	int radius;

	smd_bone_t* vBones[SMDL_MAX_BONES]; //SMD_NODES
	smd_bonetransform_t* vBoneTransforms[SMDL_MAX_BONES * SMDL_MAX_FRAMES]; //SMD_SKELETON
	smd_vert_t* vVerts[SMDL_MAX_VERTS]; //SMD_TRIANGLES
	smd_surface_t* vSurfaces[SMDL_MAX_SURFACES]; // built for each texture
} smddata_t;

#endif // ! _SMD_H_
