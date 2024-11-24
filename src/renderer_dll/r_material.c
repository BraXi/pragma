#include "r_local.h"

#define MAX_MATERIALS	(MAX_WORLD_SHADERS + 1024)

material_t r_defaultMaterial;

material_t* r_materials = NULL;
static int r_numMaterials = 0;

/*
=================
R_FindMaterial
=================
*/
material_t* R_FindMaterial(const char* name, const worldLightMap_t lightmap)
{
	int i;
	material_t* mat;

	if (!name || name[0] == 0)
	{
		return &r_defaultMaterial;
	}

	// see if material is already loaded
	mat = r_materials;
	for (i = 0; i < r_numMaterials; i++, mat++)
	{
		if (!Q_stricmp(name, mat->name))
		{
			return mat;
		}
	}

	// try to load material
	return &r_defaultMaterial;
}

/*
=================
R_WorldMaterialForNum
=================
*/
material_t* R_WorldMaterialForNum(const int materialNum, const worldLightMap_t lightmap)
{
	material_t* material;
	const char* name;

	if (materialNum < 0 || materialNum >= r_world->numMaterials)
	{
		ri.Error(ERR_DROP, "%s: bad num %i", materialNum);
	}

	name = r_world->materials[materialNum].name;
	material = R_FindMaterial(name, lightmap);
	return NULL;
}

extern int registration_sequence;
/*
=================
R_InitMaterials
=================
*/
void R_InitMaterials()
{
	size_t size;
	char temp[MAX_QPATH];
	image_t* tex;
	material_t* mat;

	size = sizeof(material_t) * MAX_MATERIALS;

	// allocate space for materials
	if (!r_materials)
	{
		r_materials = ri.MemAlloc(size);
		Com_Printf("Reserved %i kb of memory for materials.\n", size/1024);
	}

	r_numMaterials = 0;

#if 0
	// create default material
	memset(&r_defaultMaterial, 0, sizeof(r_defaultMaterial));
	Com_sprintf(r_defaultMaterial.name, sizeof(r_defaultMaterial.name), "$default");
	r_defaultMaterial.diffuse_id = r_texture_missing->texnum;
	r_materials[0] = r_defaultMaterial;
	r_numMaterials++;
#endif

	for (int i = 0; i < r_world->numMaterials; i++)
	{
		mat = &r_materials[r_numMaterials];
		
		Com_sprintf(temp, sizeof(temp), "%s.tga", r_world->materials[i].name);

		tex = R_FindTexture(temp, it_texture, true);
		tex->registration_sequence = registration_sequence;
		mat->diffuse_id = tex->texnum;

		r_numMaterials++;
	}
}