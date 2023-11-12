/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

float developer, developer_script; // cvars for debugging

// TODO move these
.float noise_index;
.float groundEntityNum;
.float		mass;

//
// 'worldspawn' entity
//
.string		sky;			// skybox textures to use
.vector		skyaxis;		// vector axis for rotating sky
.float		skyrotate;		// speed of rotation in degrees/second

//
// weapons
//

.float		weapon;			// current weapon index to g_weapons array
//.float		wpn_spreadScale;	// increases with every shot

//
// damage and pain handling
//
.float		deadflag;		// DEAD_NO, DEAD_DYING, DEAD_YES
.float		takedamage;		// does entity take damage? DAMAGE_NO, DAMAGE_YES, DAMAGE_AIM
.float		max_health;		// health should be never higher than this 
.float		armor;			// number of armor points for protection


.void(entity attacker, float damage, float method) pain;
.void(entity attacker, float damage, float method) die;



// 'target_speaker'
.float		volume;
.float		attenuation;
.float		noise_index;

// 'trigger_hurt': deal dmg damage points
.float		dmg;

// 'func_barrel': damage radius
.float		radius;

// 'light' entity: index to lightstyle
// 'func_areaportal': toggle between states
.float		style; 

// 'trigger_counter': when this reaches 0 fire all of its targets
// 'func_spawner': spawn this number of entities
.float		count;

//
// 'light' entity
//
.vector		color; 		// light color
.float		light; 		// light radius

//
// used by SUB_UseTargets solely
//
.entity		activator;		// entity that's going to usetargets after a delay
.string		target;			// enities with targetname of this will be used
.string		targetname;		// 'target' name used for targets and killtargets
.string		killtarget;		// entities with targetname matching this killtarget will be removed


// function to call when entity is being used (triggered)
// self = entity being used, other = activator
.void(entity a) 	use; 


// SUB_UseTargets: targets will be fired after this time
// 'func_spawner': wait this time before spawning next entity
.float 		delay;


//SUB_UseTargets: play this sound to the activator when firing targets
.string 	snd;

//'trigger_counter': play this sound when counting
.string		snd2;


// SUB_UseTargets: message to print to user/activator
// 'worldspawn': name of the level
// 'func_spawner': spawn entities with this classname
.string 	message;