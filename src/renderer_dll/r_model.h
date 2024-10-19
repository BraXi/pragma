/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


typedef struct polyvert_s
{
	vec3_t	pos;
	float	alpha;
	float	texCoord[2];
	float	lmTexCoord[2];  // lightmap texture coordinate (sometimes unused)
	vec3_t	normal;
} polyvert_t;

typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	struct	glpoly_s	*chain;
	int		numverts;
	int		flags;					// for SURF_UNDERWATER (not needed anymore?)
	polyvert_t	verts[4]; // variable sized
} poly_t;


//===================================================================

typedef struct // q3 bmodel
{
	vec3_t		mins, maxs;		// for culling
	void* firstSurface;
	int			numSurfaces;
} bmodel_t;

typedef struct model_s
{
	char		name[MAX_QPATH];
	int			index;		// index to model array
	int			registration_sequence;
	modtype_t	type;

	int			numframes;
	int			flags;

	// volume occupied by the model graphics	
	vec3_t		mins, maxs;
	float		radius;


	bmodel_t	*bmodel;

	//
	// MOD_ALIAS & MOD_NEWFORMAT
	//
	int			cullDist;	// don't draw if camera is farther than this

	// MOD_NEWFORMAT
	pmodel_header_t* newmod;
	mat4_t *inverseBoneMatrix;

	// MOD_ALIAS
	md3Header_t* alias;	
	image_t* images[MD3_MAX_SURFACES]; // MD3_MAX_SHADERS ??

	// common for all models
	vertexbuffer_t* vb[MD3_MAX_SURFACES];
	
	int			extradatasize;
	void		*extradata;
} model_t;

//============================================================================

void	R_InitModels();
void	R_FreeAllModels();
void	R_FreeModel(model_t* mod);

model_t *R_ModelForName (const char *name, qboolean crash);

void	Cmd_modellist_f (void);

//void	*Hunk_Begin (const int maxsize, const char *name);
void	*Hunk_Alloc (int size);
int		Hunk_End (void);
void	Hunk_Free (void *base);


