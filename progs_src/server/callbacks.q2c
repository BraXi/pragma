/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// callbacks.qc - functions called by engine

/*
ClientBeginServerFrame 	-- before physics are run
ClientEndServerFrame 	-- after all physics are run
ClientBegin 			-- finished connecting and ready to play
ClientConnect 			-- connecting to server
ClientDisconnect 		-- disconnected from server
ClientCommand 			-- server receives console command from client
*/


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/

.float wpn_lastShotTime;
.float wpn_fireframe;
.float animf;
.float goalframe;

void() ClientBeginServerFrame =
{
	if(isplayer(self) == 0)
		return;
	
	//self.animf ++; //= self.animf +1;
	//if( self.animf > 3 )
	//	self.animf = 0;
	
	//if( self.wpn_lastShotTime >= g_time )
	{
		if( self.animf < self.goalframe )
			self.animf++;
	}


	if( self.health <= 0 )
	{
		setviewmodel(self, "");
		return;
		
	}
	if(self.model == "models/mutant.md3")
	{
	}
	else
	{
		setviewmodel(self, "models/v_ak47.md3");
		setviewmodelparms(self, self.animf, vec_null, vec_null);
	}
};

/*
==============
ClientEndServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void() Player_ViewEffects;

void(entity inflictor, entity attacker, float damage, entity ignore, float radius, int mod) RadiusDamage;
void() HUD_SetStats;

void() ClientEndServerFrame =
{
	if(isplayer(self) == 0)
		return;	
	
	// pmove has been already performed and applied
	
	//RadiusDamage(self, self, 100, self, 96, 0);
	Player_ViewEffects();
	HUD_SetStats();
	
	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if( self.deadflag == DEAD_NO )
	{
		if (self.v_angle_x > 180)
			self.angles_x = (-360 + self.v_angle_x) / 3;
		else
			self.angles_x = self.v_angle_x / 3;	
		self.angles_y = self.v_angle_y; //+90;
		self.angles_z = 0;
	}
	
//	self.angles[ROLL] = SV_CalcRoll(ent->v.angles, ent->v.velocity) * 4;	
};

/*
============
ClientThink
This will be called once for each client frame, 
which will usually be a couple times for each server frame.

inButtons - BUTTON_ flags
inAngles - client view side angles
inMove = [forwardmove, sidemove, upmove]
============
*/
#define	BUTTON_ATTACK	1
#define	BUTTON_USE	2
#define	BUTTON_ANY	128

void(entity attacker, entity inflictor, entity targ, float damage, float dmgflags, float method) DoDamage;
void(entity who, vector start, vector end, float weaponnum) FireBullet;
void(entity ent, vector fireAngles, vector offset, float wpn) W_Fire;

void(float inButtons, float inImpulse, vector inMove, vector inAngles, float inLightLevel, float inMsecTime) ClientThink =
{
	
	if(isplayer(self) == 0)
		return;	
	
	/* 
	note to self: 
	client is not aware of its bbox size, thus pmove predictions fail
	make clientinfo transmit bbox size so pmove can become more flexible
	
	*/
	// pmove
	if( self.health > 0 )
	{
		if(self.movetype == MOVETYPE_NOCLIP)
			self.pm_type = PM_SPECTATOR;
		else if(self.health <= -50)
			self.pm_type = PM_GIB;
		else
			self.pm_type = PM_NORMAL;
	}
	
	//self.pm_type = PM_FREEZE;
	if(self.pm_type == PM_FREEZE)
		inAngles = self.v_angle;
	
	setviewoffset(self, '0 0 24');	
	setsize(self, '-16 -16 -24', '16 16 32');
	self.viewheight = 22;
	pmove(self, inMove, true);
			
	if( self.health > 0 && (inButtons & BUTTON_ATTACK) && g_time >= self.wpn_lastShotTime+0.2 )
	{
		self.wpn_fireframe = g_frameNum;
		self.wpn_lastShotTime = g_time;
		self.animf = 0;
		self.goalframe = 2;
	
		if(self.model == "models/monsters/mutant/tris.md2")
		{
			playsound(vec3_origin, self, CHAN_BODY, "mutant/Mutsght1.wav", 1, ATTN_NORM, 0);
			self.velocity_z += 400;
			return;
		}
		
		vector o;
		o_z = self.viewheight;
		W_Fire(self, self.v_angle, o, 1);
	}
};


/*
============
ClientBegin

called when a client has finished connecting, and is ready to be placed into the game.
This will happen every level load and thus also when server changes map.
============
*/
void() SpawnPlayer;
void() RestoreClientPers;
void() HUD_SetInitialStats;
void() ClientBegin =
{
	self.classname = "player";
	bprint(2, "new client connected\n");	
	
	RestoreClientPers();
	SpawnPlayer();
	HUD_SetInitialStats();	
	FX_Muzzleflash(self, FX_MUZZLEFLASH_LOGIN);	
	ClientEndServerFrame();	
};

/*
============
ClientConnect

Called when a player begins connecting to the server.
Server progs can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but loadgames will.
============
*/
float() ClientConnect =
{
	return true;
};

/*
============
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void() ClientDisconnect =
{
	bprint(0, "client disconnected\n");
	FX_Muzzleflash(self, FX_MUZZLEFLASH_LOGOUT);
};

/*
==============
ClientCommand

This is called when client sends command to server
The progs can issue argc() / argv() commands to get the rest of the parameters
Note that engine commands like say, say_team.. are in engine and ClientCommand is called if no builtin command exist.
Return true if progs handle given command, false usually means we shuld print to chat
==============
*/
float() ParseClientCommand; // clcmds.qc
float() ClientCommand =
{
	return ParseClientCommand();
};