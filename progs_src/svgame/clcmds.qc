/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// clcmds.qc - client commands

void(float cheat) ClientCmd_Cheat;

entity testmodel;
void() testmodel_rotate = 
{
//	self.avelocity = '0 90 0';

	self.animFrame ++;
	if( self.animFrame > 125 )
		self.animFrame = 62;
	self.nextthink = g_time + TIME_100_MS;
	self.think = testmodel_rotate;	
	
};
void() ClientCmd_TestModel =
{
	if(!testmodel)
	{
		testmodel = spawn();
		setmodel(testmodel, "models/mutant.md3");
		testmodel.think = testmodel_rotate;
		testmodel.nextthink = g_time + TIME_100_MS;
		self.animFrame = 62;
	}
	setorigin(testmodel, self.origin + '0 0 0' );
	
	testmodel.renderFlags = RF_FULLBRIGHT | RF_COLOR | RF_SCALE; //| RF_TRANSLUCENT;
	testmodel.renderColor = '0 1 0';
	testmodel.renderScale = 3;
	testmodel.renderAlpha = 0.8;	
	testmodel.angles = self.angles - '0 180 0';
	testmodel.angles_x = testmodel.angles_z = 0;
	testmodel.origin_z = testmodel.origin_z + (24 * (testmodel.renderScale-1));
};

void(entity ent, int newweapon) SetWeapon;
void() ClientCmd_SetWeapon =
{
	float newweapon = stof(argv(1));
	SetWeapon(self, newweapon);

};

float() ParseClientCommand =
{
	local string cmd;

	cmd = argv(0);

	if(g_intermissionTime)
		return 0;

	if( cmd == "respawn" ) 
	{
		if( self.health <= 0 )
		{
			SpawnPlayer();
			sprint(self, PRINT_HIGH, "respawned\n");
		}
	}

	if( cmd == "mdl" ) /* cheat */
	{
		ClientCmd_TestModel();
	}
	else if( cmd == "weapon" ) /* cheat */
	{
		ClientCmd_SetWeapon();
	}
	
	if( cmd == "god" ) /* cheat */
	{
		ClientCmd_Cheat(0);
	}
	else if( cmd == "noclip" ) /* cheat */
	{
		ClientCmd_Cheat(1);
	}
	else if( cmd == "notarget" ) /* cheat */
	{
		ClientCmd_Cheat(2);
	}
	else if( cmd == "give" ) /* cheat */
	{
		ClientCmd_Cheat(3);
	}	
	else
	{ 
		return 0;
	}
	
	return 1;
};

// ========================================================== 
// client commands
// ========================================================== 

void(float cheat) ClientCmd_Cheat =
{
	local string msg;
	
	if(cvar("sv_cheats") < 1)
	{
		sprint(self, PRINT_HIGH, "Cheats not enabled.\n");
		return;
	}
	
	msg = "unknown cheat command\n";
	
	if( cheat == 0 )
	{
		//GOD MODE
		self.flags ^= FL_GODMODE;
		if(!(self.flags & FL_GODMODE))
			msg = "godmode OFF\n";
		else
			msg = "godmode ON\n";		 
	}
	else if( cheat == 1 ) 
	{
		// NOCLIP
		if(self.movetype == MOVETYPE_NOCLIP)
		{
			self.movetype = MOVETYPE_WALK;
			msg = "noclip OFF\n";
		}
		else
		{
			self.movetype = MOVETYPE_NOCLIP;
			msg = "noclip ON\n";
		}		
	}
	else if( cheat == 2 ) 
	{
		// NOTARGET
		self.flags ^= FL_NOTARGET;
		if(!(self.flags & FL_NOTARGET))
			msg = "notarget OFF\n";
		else
			msg = "notarget ON\n";		
	}
	else if( cheat == 3 ) 
	{
		// GIVE
		//error("implement 'give' cheat\n" );
	}	
	sprint( self, PRINT_HIGH, msg );	
//	self.cheatsUsed = self.cheatsUsed + 1; // mark cheater :)
};
