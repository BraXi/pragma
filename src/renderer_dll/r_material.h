#ifndef _R_MATERIAL_H_
#define _R_MATERIAL_H_

typedef enum
{
	CULL_BACK,
	CULL_FRONT,
	CULL_NONE
} rCullFace_t;

typedef struct
{
	char			name[MAX_QPATH];
	qboolean		bLoaded; // true if the material has loaded without errors, if false use default material

	unsigned int	contents;
	unsigned int	surfaceFlags;

	// diffuse map
	char			diffuse[MAX_QPATH];
	int				diffuse_id; // index to images

	rCullFace_t		cull;
} material_t;


material_t* R_FindMaterial(const char* name, const worldLightMap_t lightmap);

material_t* R_WorldMaterialForNum(const int materialNum, const worldLightMap_t lightmap);

void R_InitMaterials();

#endif /*_R_MATERIAL_H_*/