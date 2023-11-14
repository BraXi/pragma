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
	vector s;
	vector e;
	
	if( wpn <= 0 )
		return;
			
	if( g_weapons[wpn].muzzleflash > 0 )
		FX_Muzzleflash(ent, g_weapons[ent.weapon].muzzleflash);
	
	anglevectors(fireAngles);
	
	s = ent.origin;
	s = s + offset; //start_z += ent.viewheight;
	e = s + v_forward * g_weapons[wpn].range;
	
	W_CalcEndPos(e, g_weapons[wpn].spread); // add random bullet spread
	
	if( g_weapons[wpn].dmgmethod == DMG_BULLET )
		FireBullet(ent, s, e, wpn);
};
