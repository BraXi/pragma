/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

// weaponlogic.qc
/*
=================
W_CalcEndPos

add some spread to endpos
=================
*/
void(__inout vector end, float spread) W_CalcEndPos =
{
	float r, u;			
	r = (2.0 * (random() - 0.5)) * spread;
	u = (2.0 * (random() - 0.5)) * spread;	
	end = end + (r * v_right);
	end = end + (u * v_up);
};
	
/*
=================
FireBullet
=================
*/
float TE_SPLASH = 10;
float TE_BUBBLETRAIL = 11;

#define SPLASH_UNKNOWN		0
#define SPLASH_SPARKS		1
#define SPLASH_BLUE_WATER	2
#define SPLASH_BROWN_WATER	3
#define SPLASH_SLIME		4
#define	SPLASH_LAVA			5
#define SPLASH_BLOOD		6

void(entity who, vector start, vector end, float weaponnum) FireBullet =
{	
	float in_water; // is start pos in water
	
	// first, see if we hit a water and play aproppriate fx
	in_water = (pointcontents(start) & MASK_WATER);
	trace(start, vec3_origin, vec3_origin, end, who, MASK_WATER);	
	
		
		
	if((trace_surface_flags & SURF_WARP))
	{	
//		print("water splash\n");
		float c;
		if (trace_contents & CONTENTS_WATER)
			c = SPLASH_BLUE_WATER;				
		else if (trace_contents & CONTENTS_SLIME)
			c = SPLASH_SLIME;
		else if (trace_contents & CONTENTS_LAVA)
			c = SPLASH_LAVA;
		else
			c = SPLASH_UNKNOWN;		
		
		if (color != SPLASH_UNKNOWN)
		{
			MSG_WriteByte (SVC_TEMP_ENTITY);
			MSG_WriteByte (TE_SPLASH);
			MSG_WriteByte (24);
			MSG_WritePos(trace_endpos);
			MSG_WriteDir (trace_plane_normal);
			MSG_WriteByte (c);
			MSG_Multicast (trace_endpos, MULTICAST_PVS); 
		}
		
		
	}
	

#if 0
				vector new_end;
			vector new_start;
			new_start = trace_endpos;
			
			new_end = new_start + v_forward * 129;
			print("in_water\n");
			MSG_WriteByte (svc_temp_entity);
			MSG_WriteByte(TE_BUBBLETRAIL);
			MSG_WritePos(start);
			MSG_WritePos(trace_endpos);
			MSG_Multicast(start, MULTICAST_PVS);
#endif	
	//FX_DebugLine(start, trace_endpos);
	
	trace(start, vec3_origin, vec3_origin, end, who, MASK_SHOT); 
		
//	FX_DebugLine(start, trace_endpos);		
	if( trace_fraction == 1.0 ) 
		return; // nothing was hit
		
	if((trace_surface_flags & SURF_SKY))
		return; // we've hit sky
	
	if(trace_ent == world || trace_ent.takedamage == DAMAGE_NO)
	{
		// play regular impact FX and return
		FX_BulletImpact(trace_endpos, trace_plane_normal, "generic");
		return;
	}
						
	//DoDamage(entity attacker, entity inflictor, entity targ, float damage, float dmgflags, float method)					
	DoDamage(who, who, trace_ent, g_weapons[weaponnum].damage, DAMAGE_NO_KNOCKBACK, g_weapons[weaponnum].dmgmethod);

	if( trace_ent.classname == "player" )
		FX_BulletImpact(trace_endpos, trace_plane_normal, "blood");
	else
		FX_BulletImpact(trace_endpos, trace_plane_normal, "sparks");			
};

/*
=================
W_Fire

vector ofs;
ofs[2] = self.viewheight;
W_Fire(self, self.v_angle, ofs, self.weapon);
=================
*/
void(entity ent, vector fireAngles, vector offset, float wpn) W_Fire =
{
	vector s, e;
		
	if( g_weapons[wpn].muzzleflash > 0 )
		FX_Muzzleflash(ent, g_weapons[ent.weapon].muzzleflash);
	
	anglevectors(fireAngles);
	
	s = ent.origin;
	s = s + offset; 
	//s_z += ent.viewheight;
	e = s + v_forward * g_weapons[wpn].range;
	
	W_CalcEndPos(e, g_weapons[wpn].spread); // add random bullet spread
	
	if( g_weapons[wpn].dmgmethod == DMG_BULLET )
		FireBullet(ent, s, e, wpn);
};


float WS_READY = 0;
float WS_RELOAD = 1;
float WS_FIRING = 2;

.float weaponframe;
.float weaponframeend;



void( float wpn ) ReloadWeaponAmmo =
{
	float inclip, maxclip, clip_add;
	float ammoweap = self.clipammo + self.reserveammo;

	inclip  = self.clipammo;
	maxclip = g_weapons[wpn].clipAmmo;

	clip_add = maxclip - inclip; 

	if ( clip_add > self.reserveammo ) 
		clip_add = self.reserveammo;

	if ( clip_add ) 
	{
		self.reserveammo -= clip_add;
		self.clipammo += clip_add;
	}
};



/*
=================
SetWeaponAnim
=================
*/
void(entity ent, string animname) SetWeaponAnim =
{
	WeaponDef w;
	vector anim;
	
	w = g_weapons[ent.weapon];
	
	if( animname == "idle")
		anim = w.anim_idle;
	if( animname == "fire")
		anim = w.anim_fire;	
	if( animname == "drop")
		anim = w.anim_drop;
	if( animname == "raise")
		anim = w.anim_raise;	
	if( animname == "reload")
		anim = w.anim_reload;

	ent.weaponframe = anim[0];
	ent.weaponframeend = anim[1];
	ent.weapon_animrate = anim[2];	
};

/*
=================
SetWeapon
=================
*/
void(entity ent, int newweapon) SetWeapon =
{
	if( ent.weapon == newweapon )
		return;
	
	ent.weapon = newweapon;
	ent.weaponState = WS_READY;
	
	ent.clipammo = g_weapons[ent.weapon].clipAmmo;
	ent.reserveammo = g_weapons[ent.weapon].maxAmmo;
	ent.weapon_animrate = 1;	

	SetWeaponAnim(ent, "idle");	
	
	setviewmodel(ent, g_weapons[ent.weapon].viewModel);
	setviewmodelparms(ent, ent.weaponframe, vec3_origin, vec3_origin);	
};


/*
=================
WeaponLogic
=================
*/
void() WeaponLogic =
{
	WeaponDef w;	
	vector ofs, kick_angles;
	
	if( self.weapon >= NUM_WEAPONS || self.weapon < 0 )
		return; // let weapon 0 too
	
	// run only once
	if(self.weaponThinkFrame == g_frameNum)
		return; 
	
	self.weaponThinkFrame = g_frameNum;

	if( !self.weapon || self.health <= 0 )
	{
		setviewmodel(self, "");
		setstat(self, STAT_WEAPON, 0);
		return;
	}
	
	w = g_weapons[self.weapon];
			
	if(self.weaponState == WS_RELOAD)
	{
		if( g_time < self.nextFireTime )
		{
//			return;
		}
		else	
		{	
			ReloadWeaponAmmo(self.weapon);
			self.weaponState = WS_READY;
		}
	}	
	else if(self.clipammo != w.clipAmmo && self.reserveammo > 0)
	{
		/* reloading */
		if(self.buttons & BUTTON_RELOAD)
		{		
			self.weaponState = WS_RELOAD;		
			self.nextFireTime = g_time + w.reloadTime;
			SetWeaponAnim(self, "reload");
			//void(vector v, selfity e, float ch, string snd, float vol, float att, float tofs)
			playsound(vec3_origin, self, CHAN_BODY, w.sfx_reload, 1, ATTN_NORM, 0);
			return;
		}
	}
	if(self.weaponState == WS_READY)
	{
		/* firing */
		if(self.buttons & BUTTON_ATTACK)
		{	
			kickangles(self, vec3_origin);	
			if(self.clipammo > 0)
			{
				if( g_time >= self.nextFireTime )
				{
					self.nextFireTime = g_time + w.fireTime;		
					SetWeaponAnim(self, "fire");
					
					ofs_z = self.viewheight;
					W_Fire(self, self.v_angle, ofs, 1);

					self.clipammo -= 1;

					if(w.vkick != 0.0)
					{
						anglevectors (self.v_angle);	
						kick_angles = v_forward * w.vkick;	
						kick_angles_y = kick_angles_y * 0.5;
						kick_angles_z = 0;

						kickangles(self, kick_angles);
					}
				}	
			}
			else if( g_time >= self.nextFireTime )
			{
				// play clicking sound
				self.nextFireTime = g_time + w.fireTime;		
			}
		}
	}	
	
	// set weapon model and frame
	setviewmodel(self, w.viewModel);
	setviewmodelparms(self, self.weaponframe, vec3_origin, vec3_origin);

	// calculate next weapon frame
	if(self.weaponframe <= self.weaponframeend)
	{
		self.weaponframe = self.weaponframe + self.weapon_animrate;
	}
	
	if(self.weaponframe > self.weaponframeend)
	{
		self.weaponframe = self.weaponframeend;
	}	
};