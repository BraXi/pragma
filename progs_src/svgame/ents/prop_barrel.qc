/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


/*QUAKED prop_barrel (1 .5 0) (-8 -8 0) (8 8 40) NO_PHYS NO_IGNITION NO_DEBRIS
model="models/barrels/barrel_red.md2"
Barrel that obeys physic, can be explosive and will use its targets when exploding.

NO_PHYS 	- object will not use physics
NO_IGNITION 	- barrel will not ignore when health is halved
NO_DEBRIS	- will not spawn debris

"takedamage"	set to 1 to make it explodable (default 0)
"health"	health points, will ignote when it drops by half (default 100)
"dmg"		maximum damage (default: 150)
"radius"	damage radius (default: 256)
"mass"		objects mass (default 300)
"model"		model to display (default models/barrels/barrel_red.md2)	
"mins"		change only when using diferent sized model (default '-8 -8 0')
"maxs"		change only when using diferent sized model (default '8 8 40')
*/

// spawn flags
float BARREL_SPAWNFLAG_NO_PHYS = 1;
float BARREL_SPAWNFLAG_NO_IGNITION = 2;
float BARREL_SPAWNFLAG_NO_DEBRIS = 4;

// default model and its animations
string prop_barrel_defaultmodel =  "models/barrel_red.md3";
$frame idle explo0 explo1 explo2 explo3 explo4 explo5 explo6 explo7 explo8

void(entity e) barrel_debris;

//void() barrel_explode0 = [$explo0, barrel_explode1] {};
//void() barrel_explode1 = [$explo1, barrel_explode2] {};
void() barrel_explode2 = [$explo2, barrel_explode3] {};
void() barrel_explode3 = [$explo3, barrel_explode4] { FX_Explosion( self.origin, "medium"); };
void() barrel_explode4 = [$explo4, barrel_explode5] { RadiusDamage(self, self.activator, self.dmg, self, self.radius, 0); };
void() barrel_explode5 = [$explo5, barrel_explode6] { FX_Explosion( self.origin + '0 0 20', "small");};
void() barrel_explode6 = [$explo6, barrel_explode7] { barrel_debris(self); };
void() barrel_explode7 = [$explo7, barrel_explode8] {};
void() barrel_explode8 = [$explo8, barrel_explode9] {};
void() barrel_explode9 = [$explo8, SUB_RemoveMe] {};

void(entity e) barrel_debris =
{
	if( e.spawnflags & BARREL_SPAWNFLAG_NO_DEBRIS )
		return;
	
	entity ent = spawn();
	ent.classname = "barrel_debris";		
	setmodel(ent, "models/barrel_debris.md3");
	setsize(ent, '-4 -4 0', '4 4 4');
	
	setorigin( ent, e.origin + '0 0 1');
	ent.movetype = MOVETYPE_TOSS;
	ent.mass = 50;	
	ent.solid = SOLID_NOT;	
	
	ent.think = SUB_RemoveMe;
	ent.nextthink = g_time + 5;
	
	linkentity(ent);
};

void(entity attacker, float damage, float method) prop_barrel_pain =
{
	if( attacker == self )
		return;
	self.activator = attacker; //last attacker
};

void(entity attacker, float damage, float method) prop_barrel_die =
{
//	self.effects = 0;
	self.takedamage = DAMAGE_NO;
	self.solid = SOLID_NOT; 
	
	SUB_UseTargets( self, self.activator );
	
	if(self.model == prop_barrel_defaultmodel)
	{
		barrel_explode2();
	}
	else
	{
		FX_Explosion(self.origin + '0 0 20', "medium");
		barrel_debris(self);
		self.think = SUB_RemoveMe;
		self.nextthink = g_time + (g_frameTime * 2);
		self.velocity = '0 0 40';
		RadiusDamage(self, self.activator, self.dmg, self, self.radius, 0);
	}
};

void() prop_barrel_think =
{
	if( !(self.spawnflags & BARREL_SPAWNFLAG_NO_IGNITION) )
	{
		if( self.health <= (self.max_health/2) )
		{
			self.loopsound = precache_sound("q1/fire1.wav");
			FX_Flame(self.origin + '0 0 37');
			DoDamage( self.activator, self.activator, self, 1, DAMAGE_NO_KNOCKBACK, 0 );
		}
	}
	self.nextthink = g_time + TIME_100_MS;
};

float() SP_prop_barrel = 
{
	if( strlen(self.model) == 0 )
		self.model = prop_barrel_defaultmodel;
	
	precache_model(self.model);	
	setmodel(self, self.model);
	
	if( self.mins == vec3_origin || self.maxs == vec3_origin )
		setsize(self, '-8 -8 0', '8 8 40');	
	
	self.solid = SOLID_BBOX;
	
	if( self.spawnflags & BARREL_SPAWNFLAG_NO_PHYS )
		self.movetype = MOVETYPE_NONE;
	else
		self.movetype = MOVETYPE_STEP;

	if( !self.radius )
		self.radius =  256;		
	if( !self.mass )
		self.mass = 300;	
	if( !self.dmg )
		self.dmg = 150;
	if( !self.health )
		self.health =  100;	
	self.max_health = self.health;	
	
	if( self.takedamage )
	{
		if( !(self.spawnflags & BARREL_SPAWNFLAG_NO_DEBRIS) )
			precache_model("models/barrel_debris.md3");
		
		self.takedamage = DAMAGE_YES;
		self.pain = prop_barrel_pain;
		self.die = prop_barrel_die;
	}
	
	self.think = prop_barrel_think;
	self.nextthink = g_time + (g_frameTime * 2);
	
	linkentity(self);
	return true;
};
