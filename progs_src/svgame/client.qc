/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


enum
{
	AP_LOW = 0,
	AP_MEDIUM,
	AP_COMBAT,
	AP_DEATH,
	AP_INSTANT
};

float ANIMTREE_NONE = 0;
float ANIMTREE_HUMAN = 1;
float ANIMTREE_MUTANT = 2;


struct playerAnims
{
	// [ start, end, notlooping ]
	vector idle; //idle  stand anim
	vector walk;
	
	vector jump;
	vector attack;	
	
	vector run_front;
	
	vector crouch_idle;
	vector crouch_walk;
	vector crouch_attack;
	vector crouch_pain;
	vector crouch_death;

	vector pain1;
	vector pain2;
	vector pain3;
	
	vector death1;
	vector death2;
	vector death3;
	
	// sounds
	string snd_jump;
	
	string snd_pain1;
	string snd_pain2;
	string snd_pain3;
	
	string snd_death1;
	string snd_death2;
	string snd_death3;
};

playerAnims animTree[3];



.vector playerAnim; 		// current player anim
.float playerAnimTree;
.float playerAnimFrame; 	// current animation frame
.float animPriority;

void(string n) safe_precache_sound =
{
	if(strlen(n))
		precache_sound(n);
};

void(float atr) precacheAnimTree =
{	
	safe_precache_sound(animTree[atr].snd_jump);
	
	safe_precache_sound(animTree[atr].snd_pain1);
	safe_precache_sound(animTree[atr].snd_pain2);
	safe_precache_sound(animTree[atr].snd_pain3);
	
	safe_precache_sound(animTree[atr].snd_death1);
	safe_precache_sound(animTree[atr].snd_death2);
	safe_precache_sound(animTree[atr].snd_death3);
};

void() initHumanAnimTree = 
{	
	animTree[ANIMTREE_HUMAN].idle = '0 39 0';
	animTree[ANIMTREE_HUMAN].walk = vec3_origin;
	
	animTree[ANIMTREE_HUMAN].jump = '66 71 1';
	animTree[ANIMTREE_HUMAN].attack = '46 53 0';
	
	animTree[ANIMTREE_HUMAN].run_front = '40 45 0';
	
	animTree[ANIMTREE_HUMAN].crouch_idle = '135 153 0';
	animTree[ANIMTREE_HUMAN].crouch_walk = '154 159 0';
	animTree[ANIMTREE_HUMAN].crouch_attack = '160 168 0';
	animTree[ANIMTREE_HUMAN].crouch_pain = '169 172 0';
	animTree[ANIMTREE_HUMAN].crouch_death = '173 177 0';

	animTree[ANIMTREE_HUMAN].pain1 = '54 57 1';
	animTree[ANIMTREE_HUMAN].pain2 = '58 61 1';
	animTree[ANIMTREE_HUMAN].pain3 = '62 65 1';
	
	animTree[ANIMTREE_HUMAN].death1 = '178 183 1';
	animTree[ANIMTREE_HUMAN].death2 = '184 189 1';
	animTree[ANIMTREE_HUMAN].death3 = '190 197 1';	
	
	// sounds
	animTree[ANIMTREE_HUMAN].snd_jump = "player/female/jump1.wav";
	
	animTree[ANIMTREE_HUMAN].snd_pain1 = "player/female/pain100_1.wav";
	animTree[ANIMTREE_HUMAN].snd_pain2 = "player/female/pain75_2.wav";
	animTree[ANIMTREE_HUMAN].snd_pain3 = "player/female/pain50_2.wav";
	
	animTree[ANIMTREE_HUMAN].snd_death1 = "player/female/jump1.wav";
	animTree[ANIMTREE_HUMAN].snd_death2 = "player/female/jump1.wav";
	animTree[ANIMTREE_HUMAN].snd_death3 = "player/female/jump1.wav";
	
	precacheAnimTree(ANIMTREE_HUMAN);
};

void() initMonsterAnimTree = 
{	
	animTree[ANIMTREE_MUTANT].idle = '62 125 0';
	animTree[ANIMTREE_MUTANT].walk = '126 148 0';	
	animTree[ANIMTREE_MUTANT].run_front = '56 61 0';
	
	animTree[ANIMTREE_MUTANT].jump = '144 148 1';
	animTree[ANIMTREE_MUTANT].attack = '5 14 0';
//	animTree[ANIMTREE_MUTANT].attack = '0 14 0'; // jumping attack
	
	animTree[ANIMTREE_MUTANT].pain1 = '34 38 1';
	animTree[ANIMTREE_MUTANT].pain2 = '39 44 1';
	animTree[ANIMTREE_MUTANT].pain3 = '45 55 1'; // protective stance?
	
	animTree[ANIMTREE_MUTANT].death1 = '15 23 1';
	animTree[ANIMTREE_MUTANT].death2 = '24 33 1';
	animTree[ANIMTREE_MUTANT].death3 = '24 33 1'; // copy of death2
	
	// sounds
	animTree[ANIMTREE_MUTANT].snd_jump = "gunner/sight1.wav";
	animTree[ANIMTREE_MUTANT].snd_pain1 = "mutant/Mutpain1.wav";
	animTree[ANIMTREE_MUTANT].snd_pain2 = "mutant/Mutpain2.wav";
	animTree[ANIMTREE_MUTANT].snd_pain3 = animTree[ANIMTREE_MUTANT].snd_pain3;
	
	animTree[ANIMTREE_MUTANT].snd_death1 = "mutant/Mutdeth1.wav";
	animTree[ANIMTREE_MUTANT].snd_death2 = animTree[ANIMTREE_MUTANT].snd_death1;
	animTree[ANIMTREE_MUTANT].snd_death3 = animTree[ANIMTREE_MUTANT].snd_death1;
	
	precacheAnimTree(ANIMTREE_MUTANT);
};

void() initAnimTrees = 
{	
	initHumanAnimTree();
	initMonsterAnimTree();
};


/*
=======================================
Player_PlayBodyAnim
=======================================
*/
void() Player_PlayBodyAnimation =
{
	if( self.playerAnim_x == 0 && self.playerAnim_y == 0 )
	{
		self.animFrame = self.playerAnimFrame = 0; // no anim playing
		return;
	}	
	
	// '_z' means animation is not looping
	if( self.playerAnim_z && self.playerAnim_y == self.animFrame )
		return;
		
	if( self.playerAnim_y == self.animFrame )
	{
		self.animPriority = 0;
		//self.playerAnimFrame = self.playerAnim_y;
		//return;
	}
		
	self.playerAnimFrame = self.playerAnimFrame + 1;
	if( self.playerAnimFrame > self.playerAnim_y )
	{
//		dprint("anim ", GetAnimName(self.playerAnim), " ended\n");		
		self.playerAnimFrame = self.playerAnim_y;
		self.playerAnim = '0 0 0';
		self.animPriority = 0;
	}	
	self.animFrame = self.playerAnimFrame;
};

void(vector newanim, float priority, float reset) Player_SetAnimation =
{
	if( newanim == self.playerAnim && reset == false )
		return;
	
	if( self.animPriority != priority )
	{
		if( self.animPriority > priority )
			return;
	}
	
	self.animPriority = priority;
	self.playerAnim = newanim;	 
	self.animFrame = self.playerAnimFrame = self.playerAnim_x;
};

/*
=======================================
Player_ViewEffects
=======================================
*/
void() Player_ViewEffects =
{
	local float	contents;
	local vector	vieworg;
	
	if( self.deadflag == DEAD_NO )
	{
		#if 1
		if(self.velocity_z > 20)
		{
			if( self.health <= 0 )
			{
				Player_SetAnimation(animTree[self.playerAnimTree].death2, AP_DEATH, false);
			}
			else
			{
				Player_SetAnimation(animTree[self.playerAnimTree].jump, AP_MEDIUM, false);
			}
		}
		else
		{
			if( self.velocity_x == 0 && self.velocity_y == 0 )
			{
				Player_SetAnimation(animTree[self.playerAnimTree].idle, AP_MEDIUM, false);
			}	
			else
			{
				Player_SetAnimation(animTree[self.playerAnimTree].run_front, AP_MEDIUM, false);
			}		
		}
		#endif
	}

	Player_PlayBodyAnimation();
	
	vieworg = self.origin + getviewoffset(self);
	contents = pointcontents(vieworg); // add for contents	

// not used in gl renderer :v
//	if (contents & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER) )
//		self.rdflags |= RDF_UNDERWATER;
//	else
//		self.rdflags &= ~RDF_UNDERWATER;
	
	if (contents & (/*CONTENTS_SOLID|*/CONTENTS_LAVA))
		setviewblend(self, '1.0 0.3 0.0', 0.6);
	else if (contents & CONTENTS_SLIME)
		setviewblend(self, '0.0 0.1 0.05', 0.6);
	else if (contents & CONTENTS_WATER)
		setviewblend(self, '0.5 0.3 0.2', 0.4);
	else
		setviewblend(self, vec_null, 0);
	
	if(self.model == "models/mutant.md3")
		setviewblend(self, '0.8 0.2 0.0', 0.35);	
};


void(entity attacker, float damage, float method) player_pain =
{	
	if( self.playerAnimTree == ANIMTREE_MUTANT && self.health <= self.max_health/2 )
		self.skinnum = 1;
	
	// play pain sound
	if( random() > 0.5 )
		playsound(vec3_origin, self, CHAN_BODY, animTree[self.playerAnimTree].snd_pain1, 1, ATTN_NORM, 0);
	else
		playsound(vec3_origin, self, CHAN_BODY, animTree[self.playerAnimTree].snd_pain2, 1, ATTN_NORM, 0);
		
	Player_SetAnimation(animTree[self.playerAnimTree].pain2, AP_MEDIUM, false);
};

void(entity attacker, float damage, float method) player_die =
{	
	self.deadflag = DEAD_DEAD;
	self.solid = SOLID_NOT;
	self.movetype = MOVETYPE_NONE;
	
	self.pm_type = PM_DEAD;
	
	self.svflags |= SVF_DEADMONSTER;
	
	// play death sound
	if( random() > 0.5 )
		playsound(vec3_origin, self, CHAN_BODY, animTree[self.playerAnimTree].snd_death1, 1, ATTN_NORM, 0);
	else
		playsound(vec3_origin, self, CHAN_BODY, animTree[self.playerAnimTree].snd_death2, 1, ATTN_NORM, 0);
	
	Player_SetAnimation(animTree[self.playerAnimTree].death1, AP_DEATH, false);
};


entity(string spawnpointname) SelectSpawnPoint;
void() SpawnPlayer =
{
	entity sp;
	string modelname;
		
	sp = SelectSpawnPoint("info_player_start");
	self.angles = sp.angles;
	setorigin( self, sp.origin );

	self.groundentity_num = -1;
	self.deadflag = DEAD_NO;
	self.takedamage = DAMAGE_AIM;
	self.health = self.max_health = 100;	
	
	self.effects = 0;
	self.skinnum = 0;
	
	self.pain = player_pain;
	self.die = player_die;

	self.movetype = MOVETYPE_WALK;
	self.viewheight = 22;
	self.classname = "player";
	self.mass = 200;
	self.solid = SOLID_BBOX;
	self.clipmask = MASK_PLAYERSOLID;
	self.svflags &= ~SVF_DEADMONSTER;

	self.waterlevel = 0;
	self.watertype = 0;

	setviewoffset(self, '0 0 24');
	setsize(self, '-16 -16 -24', '16 16 32');
	linkentity(self);
	
	self.pm_type = PM_NORMAL;
	self.pm_flags &= ~PMF_NO_PREDICTION;	
//	self.pm_origin[0] = self.origin[0];
//	self.pm_origin[1] = self.origin[1];
//	self.pm_origin[2] = self.origin[2];
	
//	self.angles_x = 0;
//	self.angles_y = sp.angles_y;
//	self.angles_z = 0;
//	self.viewangles = self.angles;
//	self.v_angle = self.angles;

	self.clipammo = 30;
	
	setstat(self, STAT_CURRENT_WEAPON, WEAPON_AR);
	setstat(self, STAT_WEAPON_CLIPAMMO, self.clipammo);
	setstat(self, STAT_WEAPON_RESERVEAMMO, 90);

	
	float human = true;
	if(getentnum(self) == 1 || human)
	{
		modelname = "models/female.md3";
		self.playerAnimTree = ANIMTREE_HUMAN;
	}
	else
	{
		modelname = "models/mutant.md3";
		self.playerAnimTree = ANIMTREE_MUTANT;
	}

#if defined(POLO_FUNC_VEHICLE)	
	modelindex_player = precache_model(modelname);
	modelindex_eyes = modelindex_player;
#endif	
	precache_model(modelname);
	setmodel(self, modelname);
	
	self.animPriority = 0;
	Player_SetAnimation(animTree[self.playerAnimTree].idle, AP_MEDIUM, false);	
	ClientEndServerFrame();	
};





/*
anims:

stand
	- idle
	- attack (one handed)
	- attack (two handed)
	- walk front
	- walk back


*/