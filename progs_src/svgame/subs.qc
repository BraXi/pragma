/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// subs.qc - subroutines

void() SUB_Null = {};
void() SUB_RemoveMe = { remove(self); };

float() CheckSpawnFlags; // <-- spawn.qc
void() KillBox = { dprint("KillBox()\n"); };

void(entity user, entity who) SUB_UseTargets;

// null stubs for callbacks
void() null_prethink = {};
void() null_think = {};
void() null_blocked = {};
void(float planeDist, vector planeNormal, float surfaceFlags) null_touch = {};
void(entity activator) null_use = {};
float(float mod, float damage, float dmgflags) null_dmgmod = { return damage; };

void(entity attacker, float damage, float method) null_pain = {};
void(entity attacker, float damage, float method) null_die = {};

float(string str) IsStringEmpty =
{
	float len = strlen(str);
//	dprint("strempty: ", str, " - ", ftos(len), "\n");
	if( len == 0 )
		return true;
	if(!str)
		return true;
	if(str == string_null)
		return true;
	if(str == "")
		return true;	
	if(str == " ")
		return true;
	
	return false;
};

/*
==============================
SUB_GetCenter

brushmodels have their origin at 0,0,0 so calculate their origin as a centre of bbox
==============================
*/
vector(entity ent) SUB_GetCenter =
{
	vector pos;
	
	if( self.mins == vec3_origin || self.maxs == vec3_origin )
		return self.origin;
	
	pos = ent.absmin + ent.absmax;
	pos = pos * 0.5;
	return pos;
};

void() think_delayed_usetargets =
{
	SUB_UseTargets(self, self.activator);
	remove(self);
};

/*
==============================
SUB_UseTargets

the global "activator" should be set to the entity that initiated the firing.

If self.delay is set, entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

centerprints any self.message to the activator.

Search for (string)targetname in all entities that match (string)self.target and call their .use function

==============================
*/
void(entity user, entity who) SUB_UseTargets =
{
	entity temp, temp2;
	entity ent;

	// check for a delay and create a temp entity to fire at a later time
	if (user.delay > 0)
	{
		if(!who || who == world)
			dprint("WARNING: Delayed SUB_UseTargets with no activator\n");	
		
		temp = spawn();
		temp.classname = "temp_delayed_use";
		temp.nextthink = g_time + user.delay;
		temp.think = think_delayed_usetargets;
		temp.activator = who;
		temp.message = user.message;
		temp.target = user.target;
		temp.killtarget = user.killtarget;	
		return;
	}
		
	// print message
	if( IsStringEmpty(user.message) == false && isplayer(who))
	{
		// message is set and we're player
		centerprint( who, user.message );
		
		if(IsStringEmpty(user.snd) == false)
			playsound(vec3_origin, who, CHAN_AUTO, user.snd, 1, ATTN_NORM, 0);
		else if(user.snd != "none")
			playsound(vec3_origin, who, CHAN_AUTO, "misc/talk1.wav", 1, ATTN_NORM, 0);
	}


	// remove all killtargets
	if(IsStringEmpty(user.killtarget) == false)
	{	
		for( ent = getent(1); ent != world; ent = nextent(ent) )
		{
			if( ent.targetname == user.killtarget )
			{	
				if(ent == user)
					dprint( "WARNING: Entity ", ent.classname, " at ", vtos(ent.origin), " killtriggered itself!\n");				
				remove(ent);
			}
		}
	}


	//fire targets
	if(IsStringEmpty(user.target) == false)
	{	
		for( ent = getent(1); ent != world; ent = nextent(ent) )
		{
			if(ent.classname == "func_areaportal" && (user.classname == "func_door" || user.classname == "func_door_rotating"))
				continue; // doors fire area portals in a specific way

			if( ent.targetname != user.target)
				continue;
			
			if(ent == user)
			{
				dprint( "WARNING: Entity ", ent.classname, " at ", vtos(ent.origin), " tried to use itself\n");
				continue;
			}
			
			if(ent.use && ent.use != null_use)
			{
				temp = self;
				temp2 = other;
				
				self = ent;
				other = who;
				ent.use(other);
				
				self = temp;
				other = temp2;
			}
			
//			if(!entinuse(ent))
//			{
//				dprintf5( "WARNING: Entity ", ent.classname, " at ", vtos(ent.origin), " was removed while using targets\n");
//				return;
//			}
		}
	}
};
