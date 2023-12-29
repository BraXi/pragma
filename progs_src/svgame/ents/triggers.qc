/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// triggers.qc

/*
trigger_multiple
trigger_once
trigger_relay
trigger_counter
trigger_always
trigger_push
trigger_gravity
trigger_hurt
trigger_actorjump / trigger_monsterjump
*/

//
//spawnflags
//

//trigger_multiple
float TRIG_SPAWNFLAG_ACTOR = 1;		// actors can activte trigger
float TRIG_SPAWNFLAG_NOT_PLAYER = 2;	// players can not activate trigger
float TRIG_SPAWNFLAG_TRIGGERED = 4;		// start disabled

//trigger_counter
float TRIG_SPAWNFLAG_NO_MESSAGE = 1; 
float TRIG_SPAWNFLAG_SND_WHEN_COMPLETED = 2; 

//trigger_hurt
float TRIG_SPAWNFLAG_START_OFF = 1;
float TRIG_SPAWNFLAG_TOGGLE = 2;
float TRIG_SPAWNFLAG_SILENT = 4;
float TRIG_SPAWNFLAG_NO_PROTECTION = 8;
float TRIG_SPAWNFLAG_SLOW = 16;

.vector movedir;
.float wait;
.float delay;
.float speed;
.float fly_sound_debounce_time;
.float timestamp;
.float jumpheight;
.float height;


void( vector v1, vector v2 ) G_SetMovedir = {};


/*
InitTriggerVectors

For each trigger, find it's target info_notnull to calculate movedir vector.
*/
void() InitTriggerVectors = 
{
	entity ent, trig, targ;
//	vector dir;
	
	for( ent = getent(1); ent != world; ent = nextent(ent) )
	{
		resetrunaway(); // in worst case this loops ents^2 times so don't let scripts run out of instructions
		
// TODO: need to add builtin for this		
//		if( !issubstr(ent.classname, "trigger_" )
//			continue; // not a trigger

		if( ent.classname == "trigger_hurt" || ent.classname == "trigger_counter" )
			continue; // not the triggers we want
		
		trig = ent; 
		
		// setting movedir in radiant has higher priority and we also want triggers with targets
		if( trig.movedir != vec3_origin || IsStringEmpty( trig.target ) )
			continue;  
			
		// find trigger's targets
		for( targ = getent(1); targ != world; targ = nextent(targ) )
		{
			resetrunaway();
			if( trig.target == targ.targetname ) //target found
			{
				if( targ.classname == "info_notnull" ) // calculate trigger's movedir
				{
					//dir = vectoangles( trig.origin - targ.origin );
					//dir = normalize( dir );
					//trig.movedir = dir;
					
					trig.movedir = normalize( trig.origin - targ.origin );
					break;
				}
			}
		}
	}
};

/*
InitTrigger

Initialize entity to become a trigger
*/
void() InitTrigger =
{
	self.solid = SOLID_TRIGGER;
	self.movetype = MOVETYPE_NONE;
	self.svflags = SVF_NOCLIENT;
	
	setmodel(self, self.model);
	
	if( strlen(self.snd) > 0 )
	{
		if( precache_sound(self.snd) == 0 )
			dprint( self.classname, " at ", vtos(self.origin), " has snd set to '", self.snd, "' but cannot load file\n" );
	}
};

// the wait time has passed, so set back up for another activation
void() trig_multiple_wait =
{
	self.nextthink = 0;
};

// the trigger was just activated
// self.activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void() trig_multiple =
{
	if (self.nextthink)
		return;		// already been triggered

	SUB_UseTargets(self, self.activator);

	if (self.wait > 0)	
	{
		self.think = trig_multiple_wait;
		self.nextthink = g_time + self.wait;
	}
	else
	{	// we can't just remove (self) here, because this is a touch function called while looping through area links...
		self.touch = null_touch;
		self.nextthink = g_time + FRAMETIME;
		self.think = SUB_RemoveMe;
	}
};

void(entity user) trig_multiple_use =
{
	self.activator = other;
	trig_multiple();
};

void(float planeDist, vector planeNormal, float surfaceFlags) trig_multiple_touch =
{
	if( isplayer(other) && (self.spawnflags & TRIG_SPAWNFLAG_NOT_PLAYER) )
		return;
	else if( other.svflags & SVF_MONSTER && !(self.spawnflags & TRIG_SPAWNFLAG_ACTOR) )
		return;

	self.activator = other;
	trig_multiple();
};


/*QUAKED trigger_multiple (.5 .5 .5) ? ACTOR NOT_PLAYER TRIGGERED
Variable sized repeatable trigger.  Must be targeted at one or more entities.

If ACTOR, AI actors can activate.
if NOT_PLAYER, players can NOT activate.
If TRIGGERED, this trigger must be triggered before it is live.

If "delay" is set, the trigger waits some time after activating before firing.

"wait" : seconds between triggerings. (.2 default)
"message" : string to be displayed when triggered
"snd" : sound to play when triggered
*/
void(entity activator) trigger_enable =
{
	self.use = trig_multiple_use;
	self.solid = SOLID_TRIGGER;
	linkentity(self);
};

float() SP_trigger_multiple =
{	
	if( !self.wait )
		self.wait = (g_frameTime * 2);
	
	self.touch = trig_multiple_touch;
	InitTrigger();

	if(self.spawnflags & TRIG_SPAWNFLAG_TRIGGERED)
	{
		self.solid = SOLID_NOT;
		self.use = trigger_enable;
	}
	else
	{
		self.solid = SOLID_TRIGGER;
		self.use = trig_multiple_use;
	}

	setmodel(self, self.model);
	linkentity(self);
	return true;
};

/*QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED
Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching "targetname".

If TRIGGERED, this trigger must be triggered before it is live.
If "delay" is set, the trigger waits some time after activating before firing.

"message" : string to be displayed when triggered
*/
float() SP_trigger_once =
{
	self.wait = -1;
	SP_trigger_multiple();
	return true;
};

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
*/
void(entity activator) trigger_relay_use =
{
	SUB_UseTargets(self, other);
};

float() SP_trigger_relay =
{
	self.use = trigger_relay_use;
//	linkentity(self); // this never broadcasts so don't link
	return true;
};


/*QUAKED trigger_counter (.5 .5 .5) ? NO_MESSAGE SND_WHEN_COMPLETED
Acts as an intermediary for an action that takes multiple inputs.

If NO_MESSAGE is NOT set, t will print "1 more.. " etc when triggered.
If SND_WHEN_COMPLETED is set, it will play .snd only when sequence is completed.

After the counter has been triggered "count" times (default 2), it will display message, fire all of it's targets and remove itself.

"count" = How many times it needs to be triggered (default 2)
"snd" = sound to play when triggered (default "misc/talk1.wav")
"message" = (default "Sequence completed!")
*/
void(entity user) trigger_counter_use =
{
	if (self.count == 0)
		return;
		
	self.count = self.count - 1;

	if (self.count)
	{
		if (!(self.spawnflags & TRIG_SPAWNFLAG_NO_MESSAGE))
		{
			centerprint(other, ftos(self.count), " more to go...");		
			playsound(vec3_origin, other, CHAN_AUTO, self.snd, 1, ATTN_NORM, 0);
		}
		return;
	}
	
	if (!(self.spawnflags & TRIG_SPAWNFLAG_NO_MESSAGE))
	{
		centerprint(other, "Sequence completed!");
		playsound(vec3_origin, other, CHAN_AUTO, self.snd, 1, ATTN_NORM, 0);
	}
	self.activator = other;
	trig_multiple();
}

float() SP_trigger_counter =
{
	self.wait = -1;
	
	if( !self.count )
		self.count = 2;
	
	self.use = trigger_counter_use;
	
	if( IsStringEmpty(self.snd) || IsStringEmpty(self.snd2) )
	{
		// set default sound
		self.snd = "misc/talk1.wav";
		self.snd2 = "misc/talk1.wav";		
		precache_sound(self.snd2);
	}
	
	return true;
};


/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
float() SP_trigger_always =
{
	// we must have some delay to make sure our use targets are present
	if (self.delay < (g_frameTime * 2))
		self.delay = (g_frameTime * 2);
	SUB_UseTargets(self, self);
	return true;
};


/*QUAKED trigger_gravity (.5 .5 .5) ?
Changes the touching entites gravity to
the value of "gravity".  1.0 is standard
gravity for the level.
*/
void(float planeDist, vector planeNormal, float surfaceFlags) trigger_gravity_touch 
{
	other.gravity = self.gravity;
};

float() SP_trigger_gravity =
{
	if (self.gravity == 0)
	{
		dprint("trigger_gravity without gravity set at ", vtos(self.origin), "\n");
		return false;
	}

	InitTrigger();
	self.touch = trigger_gravity_touch;
	return true;
};



/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.

It does dmg points of damage each server frame, plays "snd"  when hurting.

If START_OFF, must be targeted to turn on
If TOGGLE, switch between on and off state
If SILENT, supresses playing the sound
If SLOW, changes the damage rate to once per second
If NO_PROTECTION, *nothing* stops the damage

"dmg" : damage dealt every server frame (default 10, whole numbers only)
"snd" : hurt sound (default "world/electro.wav")
*/
void(entity activator) hurt_use =
{
	if (self.solid == SOLID_NOT)
		self.solid = SOLID_TRIGGER;
	else
		self.solid = SOLID_NOT;
	
	linkentity(self);

	if (!(self.spawnflags & TRIG_SPAWNFLAG_TOGGLE))
		self.use = null_use;
};


void(float planeDist, vector planeNormal, float surfaceFlags) hurt_touch =
{
	local float dflags;

	if( !other.takedamage || self.timestamp > g_time)
		return;

	if(self.spawnflags & TRIG_SPAWNFLAG_SLOW)
		self.timestamp = g_time + 1;
	else
		self.timestamp = g_time + FRAMETIME;

	if(!(self.spawnflags & TRIG_SPAWNFLAG_SILENT))
	{
		if ((g_frameNum % 10) == 0)
			playsound(vec3_origin, other, CHAN_AUTO, self.snd, 1, ATTN_NORM, 0);
	}

	if (self.spawnflags & TRIG_SPAWNFLAG_NO_PROTECTION)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;
	
//	T_Damage (other, self, self, vec3_origin, other.s.origin, vec3_origin, self.dmg, self.dmg, dflags, MOD_TRIGGER_HURT); //fixme
};

void() SP_trigger_hurt =
{
	InitTrigger();

	self.noise_index = precache_sound("world/electro.wav");
	self.touch = hurt_touch;

	if(!self.dmg)
		self.dmg = 10;

	if (self.spawnflags & TRIG_SPAWNFLAG_START_OFF)
		self.solid = SOLID_NOT;
	else
		self.solid = SOLID_TRIGGER;

	if (self.spawnflags & TRIG_SPAWNFLAG_TOGGLE)
		self.use = hurt_use;

	linkentity(self);
};


/*QUAKED trigger_actorjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards
*/
void(float planeDist, vector planeNormal, float surfaceFlags) trigger_monsterjump_touch =
{
	if(other.flags & (FL_FLY | FL_SWIM))
		return;
	if(other.svflags & SVF_DEADMONSTER)
		return;
	if(!(other.svflags & SVF_MONSTER))
		return;

	// set XY even if not on ground, so the jump will clear lips
	other.velocity[0] = self.movedir[0] * self.speed;
	other.velocity[1] = self.movedir[1] * self.speed;
	
	if (other.groundEntityNum == -1)
		return;
	
	other.groundEntityNum = -1;
	other.velocity_z = self.movedir_z;
};

float() SP_trigger_actorjump =
{
	self.classname = "trigger_actorjump";
	if(!self.speed)
		self.speed = 200;
	
	if(!self.jumpheight)
		self.jumpheight = 200;
	
	if(self.angles_y == 0)
		self.angles_y = 360;
	
	InitTrigger();
	self.touch = trigger_monsterjump_touch;
	self.movedir_z = self.jumpheight;
	return true;
};

float() SP_trigger_monsterjump = /*this is only for compatibility so pragma shuts up about no spawn function for entity*/
{
	return SP_trigger_actorjump();
};