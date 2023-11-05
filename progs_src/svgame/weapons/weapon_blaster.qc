/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// weapon_blaster.qc

/*
=================
blaster_effect
=================
*/
void(vec3 norm) blaster_effect =
{
		MSG_WriteByte(svc_temp_entity);
		MSG_WriteByte(TE_BLASTER);
		MSG_WritePos(self.origin);
		MSG_WriteDir(norm);
		MSG_Multicast(self.origin, MULTICAST_PVS);	
};

/*
=================
fire_blaster

Fires a single blaster bolt. Used by the blaster and hyper blaster.
=================
*/
void blaster_touch (cplane_t *plane, csurface_t *surf)
{
	if(other == self.owner)
		return;

	if(surf_flags & SURF_SKY))
	{
		remove(self);
		return;
	}

	if(other.takedamage)
	{
		DoDamage(other, self, self.owner, self.velocity, self.origin, plane_normal, self.dmg, 1, DMG_BULLET);
	}
	else
	{
		blaster_effect(plane_normal);
	}

	remove(self);
}


void Blaster_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 4;
	AngleVectors (ent.client.v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent.viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent.client, ent.s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent.client.kick_origin);
	ent.client.kick_angles[0] = -1;

	fire_blaster (ent, start, forward, damage, 1000, effect, hyper);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER);
	else
		gi.WriteByte (MZ_BLASTER);
	gi.multicast (ent.s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, qboolean hyper)
{
	edict_t	*bolt;
	trace_t	tr;

	VectorNormalize (dir);

	bolt = spawn();
	bolt.svflags = SVF_DEADMONSTER;
	VectorCopy (start, bolt.s.origin);
	VectorCopy (start, bolt.s.old_origin);
	vectoangles (dir, bolt.s.angles);
	VectorScale (dir, speed, bolt.velocity);
	bolt.movetype = MOVETYPE_FLYMISSILE;
	bolt.clipmask = MASK_SHOT;
	bolt.solid = SOLID_BBOX;
	
	bolt.effects |= effect;
	VectorClear (bolt.mins);
	VectorClear (bolt.maxs);
	
	bolt.modelindex = precache_model("models/objects/laser/tris.md2");
	bolt.sound = precache_sound("misc/lasfly.wav");
	bolt.owner = self;
	bolt.touch = blaster_touch;
	bolt.nextthink = g_time + 2;
	bolt.think = SUB_RemoveMe;
	bolt.dmg = 10;
	bolt.classname = "bolt";
	
	gi.linkentity (bolt);



	tr = traceline(self.origin, NULL, NULL, bolt.s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt.s.origin, -10, dir, bolt.s.origin);
		bolt.touch (bolt, tr.ent, NULL, NULL);
	}
}	