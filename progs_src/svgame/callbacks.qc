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

	if( self.health <= 0 )
	{
		setviewmodel(self, "");
		return;
		
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
void() HUD_SetStats;

void() ClientEndServerFrame =
{
	if(isplayer(self) == 0)
		return;	

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

void(entity attacker, entity inflictor, entity targ, float damage, float dmgflags, float method) DoDamage;
void(entity ent, vector fireAngles, vector offset, float wpn) W_Fire;

void() SpawnPlayer;
void() WeaponLogic;
void(float inButtons, float inImpulse, vector inMove, vector inAngles, float inLightLevel, float inMsecTime) ClientThink =
{
	if(isplayer(self) == 0)
		return;	
	
	
	//
	// let dead players respawn (fixme testing)
	//	
	if(self.health <= 0 && self.deadflag == DEAD_DEAD && inMove_z > 0)
	{
		SpawnPlayer();
		sprint(self, PRINT_HIGH, "respawned\n");
		return;
	}
	
	
	//
	// update button states, see button bits in shared.qc
	//
	self.oldbuttons = self.buttons;
	self.buttons = inButtons;
	self.latched_buttons |= self.buttons & ~self.oldbuttons;	
	
	
	//
	// perform player move simulation (bg_pmove.qc)
	//
	if( self.health > 0 )
	{
		if(self.movetype == MOVETYPE_NOCLIP)
			self.pm_type = PM_NOCLIP;
		else if(self.health <= -50)
			self.pm_type = PM_GIB;
		else
			self.pm_type = PM_NORMAL;
	}
	
	if(self.pm_type == PM_FREEZE)
		inAngles = self.v_angle;
	
	SV_PlayerMove(inMove, inAngles, inMsecTime);
	

	//
	// weapon think
	//
	
	WeaponLogic();

#if 0	
	if( self.health > 0 && (inButtons & BUTTON_ATTACK) && g_time >= self.wpn_lastShotTime+0.2 )
	{
		self.wpn_fireframe = g_frameNum;
		self.wpn_lastShotTime = g_time;
		self.animf = 0;
		self.goalframe = 2;
	
		if(self.model == "models/mutant.md3")
		{
			playsound(vec3_origin, self, CHAN_BODY, "mutant/Mutsght1.wav", 1, ATTN_NORM, 0);
			self.velocity_z += 400;
			return;
		}
	}
#endif
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
//	FX_Muzzleflash(self, FX_MUZZLEFLASH_LOGIN);	//this is lame
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