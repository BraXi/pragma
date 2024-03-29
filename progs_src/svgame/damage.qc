/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// damage.qc
/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/

float g_TeamPlay;

float(entity e1, entity e2) OnSameTeam =
{
	if(!g_TeamPlay)
		return false;	
	
	if(isplayer(e1) && isplayer(e2))
		if(e1.team == e2.team)
			return true;
		
	return false;	
};

float(entity targ, entity inflictor) CanDamage =
{
	local vector	dest;


	// brushmodels need special checking because their origin is 0,0,0
	if (targ.movetype == MOVETYPE_PUSH)
	{
		dest = SUB_GetCenter(targ);
		
		//void(vector p1, vector v1, vector v2, vector p2, entity e, float c)
		trace(inflictor.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if(trace_fraction == 1.0)
			return true;
		if(trace_ent == targ)
			return true;
		return false;
	}
	
	trace(inflictor.origin, vec3_origin, vec3_origin, targ.origin, inflictor, MASK_SOLID);
	if(trace_fraction == 1.0)
		return true;

	dest = targ.origin;
	dest = dest + '15 15 0';
	
	trace(inflictor.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if(trace_fraction == 1.0)
		return true;

	dest = targ.origin;
	dest = dest + '15 -15 0';
	
	trace(inflictor.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if(trace_fraction == 1.0)
		return true;

	dest = targ.origin;
	dest = dest + '-15 15 0';
	
	trace(inflictor.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if(trace_fraction == 1.0)
		return true;

	dest = targ.origin;
	dest = dest - '15 15 0';
	
	trace(inflictor.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if(trace_fraction == 1.0)
		return true;

	return false;
};

void(float type, vector origin, vector normal, float damage) SpawnDamage =
{
	if (damage > 255)
		damage = 255;
	
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(type);
//	MSG_WriteByte(damage);
	MSG_WritePos(origin);
	MSG_WriteDir(normal);
	MSG_Multicast(origin, MULTICAST_PVS);
};


/*
============
DoDamage

attacker 	- entity that does damage (ex. player)
inflictor 	- entity that dealt damage (ex. grenade)
targ 		- the victim (ex. moonster)

damage 		- how much damage to deal
dmgflags	- damage flags (see below)
method		- DMG_EXPLOSIVE etc

dmgflags
============
*/

void(entity attacker, entity inflictor, entity targ, float damage, float dmgflags, float method) DoDamage =
{
	vector	dir;
	float	take;
	float	fx;
	entity	oldSelf;
	
	if( !targ.takedamage )
		return;
	
	// handle players diferently
	if(isplayer(targ))
	{	
		if(!(dmgflags & DAMAGE_NO_PROTECTION))
		{
			if(targ.flags & FL_GODMODE)
				return; // god mode cheat
			if(targ.movetype == MOVETYPE_NOCLIP)
				return;	// noclip cheat
		}
		//return;
	}
	
	// own explosions do half of damage to self
//	if( method == DMG_EXPLOSIVE && attacker == targ )
//		damage = damage * 0.5;
	
	if(damage < 1)
		damage = 1;	

	if( method == DMG_BULLET )
		fx = TE_BULLET_SPARKS;
	else if( method == DMG_ENERGY )
		fx = TE_LASER_SPARKS;
	else if( method == DMG_LAVA )
		fx = TE_FLAME;
	else
		fx = TE_SPARKS;
	
//	SpawnDamage(fx, targ.origin, vec3_origin, damage);
	
	dir = targ.origin - inflictor.origin;
//	dir = inflictor.origin - targ.origin;
	
	float knockback;
	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;
	
	knockback = 1;
	if (!(dmgflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ.movetype != MOVETYPE_NONE) && (targ.movetype != MOVETYPE_BOUNCE) && (targ.movetype != MOVETYPE_PUSH) && (targ.movetype != MOVETYPE_STOP))
		{
			vector	kvel;
			float	mass2;

			if (targ.mass < 50)
				mass2 = 50;
			else
				mass2 = targ.mass;
			kvel = dir * 200.0;
			kvel = kvel * (knockback / mass2);
			
			targ.velocity = targ.velocity + kvel;
			//print("push velocity: ", vtos(targ.velocity), "\n");
		}
	}
	
	take = rint(damage); // no stupid floats
	
	targ.health = targ.health - take;

	oldSelf = self;
	self = targ;	
	if(targ.health > 0)
	{		
		if(targ.pain)
			targ.pain(attacker, take, method); // .void(entity attacker, float damage, float method) pain;		
	}
	else if(targ.health < 0 && targ.deadflag == DEAD_NO)
	{
		targ.deadflag = DEAD_DEAD;
		if(targ.die)	
			targ.die(attacker, take, method); //.void(entity attacker, float damage, float method) die;	
	}
	self = oldSelf;
	
//	centerprint(getent(1), "dodamage: health ", ftos(targ.health) );
};

/*
============
RadiusDamage
============
*/
void(entity inflictor, entity attacker, float damage, entity ignore, float radius, int mod) RadiusDamage =
{
	float	points;
	entity  ent;
	vector	v;
	vector	dir;
	vector pos;
	float dist;
	
	pos = SUB_GetCenter(inflictor);
	
	ent = world;
	while((ent = findradius(ent, inflictor.origin, radius)) != world)
	{
		if(ent == ignore)
			continue;		
		if(!ent.takedamage)
			continue;
		
		v = ent.origin + '0 0 10';
		dist = vectorlength(v - pos);
		
		points = damage - (0.5 * dist);	
		
		if(ent == attacker)
			points = points * 0.5;	
				
		points = rint(points);	
		if(points > 0)
		{
			if(CanDamage (ent, inflictor))
			{
				dir = ent.origin - inflictor.origin;
				DoDamage(attacker, inflictor, ent, points, DAMAGE_NO_PROTECTION, DMG_EXPLOSIVE);
				//dprint4( ftos(points), " radius damage to: ", ent.classname, "\n");
				dprint( "RadiusDamage: ", ftos(points), " dmg at dist ", ftos(dist), " to ", ent.classname, "\n");
			}
		}
	}
};
