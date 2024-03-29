/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) LOOPED_ON LOOPED_OFF RELIABLE
Normal sounds play each time the target is used.  The reliable flag can be set for crucial sounds
Looped sounds are allways ATTN_STATIC / vol 1, and the use function toggles it on/off.
Multiple identical looping sounds will just increase volume without any speed cost.

LOOPED_ON 	speaker will play looping sound
LOOPED_OFF	speaker needs to be triggered to play a looping sound
RELIABLE	its guaranteed everyone on server will have this sound delivered

"message"	wav file to play

"volume"	0.0 to 1.0 (default 1)
"attenuation":	default 0 (normal)
		0 normal
		1 static
		2 diminish very rapidly with distance
		3 full volume the entire level
*/
void(entity user) target_speaker_use =
{
	float	chan;

	if (self.spawnflags & SF_SPEAKER_LOOPED_ON || self.spawnflags & SF_SPEAKER_LOOPED_OFF)
	{	
		// looping sound toggles
		if (self.loopsound)
			self.loopsound = 0;
		else
			self.loopsound = self.noise_index;
	}
	else
	{
		// normal sound
		if (self.spawnflags & SF_SPEAKER_RELIABLE)
			chan = CHAN_VOICE|CHAN_RELIABLE;
		else
			chan = CHAN_VOICE;
		
		// use a positioned_sound, because this entity won't normally be
		// sent to any clients because it is invisible
		playsound(self.origin, self, chan, self.message, self.volume, self.attenuation, 0);
	}
};

float() SP_target_speaker =
{
	if(!strlen(self.message))
	{
		print("target_speaker with no sound ('message' key) set at ", vtos(self.origin), "\n");
		return false;
	}

	// remap attenuation to engine defs
	if (self.attenuation == 0)
		self.attenuation = ATTN_NORM;
	else if (self.attenuation == 1)
		self.attenuation = ATTN_IDLE;
	else if (self.attenuation == 2)
		self.attenuation = ATTN_STATIC;
	else if (self.attenuation == 3)
		self.attenuation = ATTN_NONE;
	else
	{
		print("target_speaker at ", vtos(self.origin), " has attenuation set to unknown value ", ftos(self.attenuation), "\n");
		return false;
	}
	
	self.noise_index = precache_sound(self.message);		
	if(!self.noise_index)
	{
		print("target_speaker at ", vtos(self.origin), " has unknown sound ", self.message, "\n");
		return false;
	}
//	self.message = "";
	
	if (!self.volume)
		self.volume = 1.0;

	// check for prestarted looping sound
	if (self.spawnflags & SF_SPEAKER_LOOPED_ON)
		self.loopsound = self.noise_index;

	self.use = target_speaker_use;

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	linkentity(self);
	return true;
};