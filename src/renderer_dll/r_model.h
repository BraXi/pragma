/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#ifndef _R_MODEL_H_
#define _R_MODEL_H_

#pragma once

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


//===================================================================

typedef struct // brush models are parts of world model
{
	// for culling
	vec3_t	mins, maxs;
	float	radius;

	int		firstSurface; // index to world surfaces
	int		numSurfaces;
} bmodel_t;

typedef struct model_s
{
	char		name[MAX_QPATH];
	modtype_t	type;
	int			registration_sequence;
	int			index;

	int			numframes;

	// flags -- unused
	//int		flags;

	// for culling	
	vec3_t		mins, maxs;
	float		radius;
	int			drawDistance;

	// MOD_BRUSH
	bmodel_t	*bmodel;

	// MOD_NEWFORMAT
	pmodel_header_t* newmod;
	mat4_t *inverseBoneMatrix;

	// MOD_ALIAS
	md3Header_t* alias;	
	image_t* images[MD3_MAX_SURFACES];

	vertexbuffer_t* vb[MD3_MAX_SURFACES];
	
	int			extradatasize;
	void		*extradata;
} model_t;

//============================================================================

void R_InitModels();
void R_FreeAllModels();
void R_FreeModel(model_t* mod);

model_t *R_ModelForName(const char *name, qboolean crash);

void Cmd_modellist_f(void);

//void	*Hunk_Begin (const int maxsize, const char *name);
void *Hunk_Alloc(int size);
int Hunk_End(void);
void Hunk_Free(void *base);


#endif /*_R_MODEL_H_*/