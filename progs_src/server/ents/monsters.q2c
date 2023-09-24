/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// ai/monsters_temp.qc -- this is a placehorder to spawn q2 monsters as props

//.void(entity ent, entity attacker, float damage, float method) pain;
//.void(entity ent, entity attacker, float damage, float method) die;


float(string mdlname) CreateTestDummy = 
{
	if( !precache_model(mdlname) )
	return false;
	
	setmodel(self, mdlname);
	linkentity(self);
	return true;
};

float() SP_monster_berserk = { return CreateTestDummy("models/monsters/berserk/tris.md2"); };
float() SP_monster_gladiator = { return CreateTestDummy("models/monsters/gladiatr/tris.md2"); };
float() SP_monster_gunner = { return CreateTestDummy("models/monsters/gunner/tris.md2"); };
float() SP_monster_infantry = { return CreateTestDummy("models/monsters/infantry/tris.md2"); };
float() SP_monster_soldier_light = { return CreateTestDummy("models/monsters/soldier/tris.md2"); };
float() SP_monster_soldier = { return CreateTestDummy("models/monsters/soldier/tris.md2"); };
float() SP_monster_soldier_ss = { return CreateTestDummy("models/monsters/soldier/tris.md2"); };
float() SP_monster_tank = { return CreateTestDummy("models/monsters/tank/tris.md2"); };
float() SP_monster_medic = { return CreateTestDummy("models/monsters/medic/tris.md2"); };
float() SP_monster_flipper = { return CreateTestDummy("models/monsters/flipper/tris.md2"); };
float() SP_monster_chick = { return CreateTestDummy("models/monsters/bitch/tris.md2"); };
float() SP_monster_parasite = { return CreateTestDummy("models/monsters/parasite/tris.md2"); };
float() SP_monster_flyer = { return CreateTestDummy("models/monsters/flyer/tris.md2"); };
float() SP_monster_brain = { return CreateTestDummy("models/monsters/brain/tris.md2"); };
float() SP_monster_floater = { return CreateTestDummy("models/monsters/float/tris.md2"); };
float() SP_monster_hover = { return CreateTestDummy("models/monsters/hover/tris.md2"); };
float() SP_monster_mutant = { return CreateTestDummy("models/monsters/mutant/tris.md2"); };
float() SP_monster_supertank = { return CreateTestDummy("models/monsters/boss1/tris.md2"); };
float() SP_monster_boss2 = { return CreateTestDummy("models/monsters/boss2/tris.md2"); };
float() SP_monster_jorg = { return CreateTestDummy("models/monsters/boss3/jorg/tris.md2"); };
