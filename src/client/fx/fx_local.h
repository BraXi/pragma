/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
*[PlayFX functions]
*		||
*		||-> creates fx runner
*		||
*		\/
*	fx_runner_t : FX RUNNER
*	^	|-> created at runtime, replaces temp entities
*	|	|-> holds all the basic data such as origin, angles, startTime, etc...
*	|	|-> can be [optionaly] attached to entity [and] tag
*	|	|-> can be single shot, looping, paused, reversed...
*	|	|-> is loaded from disk
*	|	|-> THIS DOES "PLAY" EFFECT EVEN IF FXDEF IS CULLED
*	|	|
*	|	|> fxdef_t : EFFECT DEFINITION
*	|		|-> holds structured segments (all effect elements)
*	|		|-> mins and maxs for culling
*	|		|-> it is never modified by FXRUNNER
*	|		|
*	|		| [note: each segment below can be configured to spawn with a delay]
*	|		| [note2: each segment can be culled based on distance to viewer]
*	|		|
*	|		|-> PARTICLE SEGMENTS * numParticleSegments [fx_particle_def_t]
*	|		|		|-> each segment spawns number of cparticle_t
*	|		|		|-> can vary in size, velocity, direction, texture, etc...
*	|		|		|-> each cparticle_t inherits initial [randomized] data from parent
*	^		|		|-> can [optionaly] collide with world
*	|_______|______<| can spawn sibling effects on events such as impact
*	^		|
*	|		|-> DLIGHT SEGMENTS * numLightsSegments
*	|		|		|> each spawned cdlight_t inherits initial [randomized] data from parent
*	^		|
*	|		|-> PROP SEGMENTS * numPropSegments
*	|		|		|-> each spawned centity_t inherits initial [randomized] data from parent
*	|		|		|-> can [optionaly] collide with world
*	|		|		|-> explosions and bullet impacts can [optionaly] collide and wake it up
*	|_______|______<| can spawn sibling effects on events such as impact
*			|
*			|-> SOUND SEGMENTS * numSoundSegments
*			|		|-> can play any sound
*			|
*			|-> GAME EVENTS
*			|		|-> can be used to tint screen, create an earthquake...
*			|		|-> or call event function from QCPROGS (and/or C?)
*			|
*	[..............]
*/

#define FX_MAX_PARTICLES 1024
#define FX_MAX_EFFECTS 256			// max loaded effects at a time, cannot be blindly increased due to net protocol encoding as a byte
#define FX_MAX_ACTIVE_EFFECTS 256	// number of fxrunners at max
#define FX_MAX_SEGMENTS_PER_TYPE 8	

typedef enum
{
	PT_DEFAULT,
	PT_SPRITE
} part_type_t;

enum
{
	FX_PARTICLE,
	FX_LIGHT,
	FX_MODEL,
	FX_SOUND
//	FX_DECAL,
};


// ================================================================ //
// EFFECT DEFINITION
// ================================================================ //

typedef struct fx_particle_def_t
{
	float	lifetime;			// (lifetime*1000)
	float	delay;				// create after cl.time+(delay*1000)

	float	fadeColorAfterTime;	// start transitioning to color2 after this time
	float	scaleAfterTime;		// start transitioning to size2 after this time

	int		count;
	int		addRandomCount;		// (flags & FXF_RANDOM_COUNT)

	vec3_t	origin;
	vec3_t	randomDistrubution;

	vec3_t	velocity;
	vec3_t	addRandomVelocity;	// (flags & FXF_RANDOM_VELOCITY)

	int		gravity;
	int		addRandomGravity;	// (flags & FXF_RANDOM_GRAVITY)

	vec4_t	color;
	vec4_t	color2;

	float	size[2];
	float	size2[2];
} fx_particle_def_t;


typedef struct fx_dlight_s
{
	float	lifetime;			// (lifetime*1000)
	float	delay;				// create after cl.time+(delay*1000)

	vec3_t	origin;
	vec3_t	color;

	float	radius;
	float	randomradius;
	float	endradius;
}fx_dlight_t;

typedef struct fxdef_s
{
	char			name[MAX_QPATH];

	vec3_t			mins, maxs;

	float			length; // in seconds

	int				numPartSegments;
	fx_particle_def_t	* part[FX_MAX_SEGMENTS_PER_TYPE];

	int				numDLightSegments;
	fx_dlight_t		*dlight[FX_MAX_SEGMENTS_PER_TYPE];

} fxdef_t;


typedef struct fx_runner_s
{
	qboolean	inuse;

	int		startTime;
	int		endTime;

	vec3_t	origin;
	vec3_t	dir;

	int		entity;			// index to cl_entities[]
	int		tag;			// index to tag on entity's model

	int		effectIndex;
} fx_runner_t;

// ================================================================ //


typedef struct fx_particle_s
{
	struct fx_particle_s* next;

	int			effectIndex;

	unsigned int	starttime;
	unsigned int	lifetime;

	vec3_t		origin;
	vec3_t		velocity;
	float		gravity;
	rgba_t		rgba;

	struct image_s* mat;
} fx_particle_t;

typedef struct fxsysstate_s
{
	// effect definitions
	fxdef_t				*fxDefs[FX_MAX_EFFECTS];
	int					numLoadedEffects;

	// active effects
	fx_runner_t			fxRunners[FX_MAX_ACTIVE_EFFECTS];
	int					numActiveFX;

	// particles managment
	fx_particle_t		particles[FX_MAX_PARTICLES];
	fx_particle_t		*active_particles, *free_particles;
	int					numParticlesInUse;
} fxsysstate_t;

extern fxsysstate_t fxSys;

int CL_FindEffect(char* name);
qboolean FX_LoadFromFile(char* name);