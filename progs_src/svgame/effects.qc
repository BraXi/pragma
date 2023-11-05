/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
// effects.qc


float TE_FLASHLIGHT = 36;
void(entity src, vector pos) FX_FlashLight =
{		
	// short short pos pos	
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(TE_FLASHLIGHT);
	MSG_WritePos(pos);
	MSG_WriteShort( getentnum(src) );
	MSG_Multicast(pos, MULTICAST_PVS);
};

void(vector pos) FX_Flame =
{
	// short short pos pos	
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(TE_FLAME);
	MSG_WritePos(pos);
	MSG_Multicast(pos, MULTICAST_PVS);
};

void(vector start, vector end) FX_DebugLine =
{
	// short short pos pos	
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(TE_DEBUGTRAIL);
	MSG_WritePos(start);
	MSG_WritePos(end);
	MSG_Multicast(start, MULTICAST_ALL);
};

void(entity src, entity dst, vector start, vector end) FX_Lighting =
{
	// short short pos pos	
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(TE_LIGHTNING);
	MSG_WriteShort( getentnum(src) );
	MSG_WriteShort( getentnum(dst) );
	MSG_WritePos(start);
	MSG_WritePos(end);
	MSG_Multicast(start, MULTICAST_ALL);
};


void(vector pos, vector start, vector end) FX_RailTrail =
{
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(TE_RAILTRAIL);
	MSG_WritePos(start);
	MSG_WritePos(end);
	MSG_Multicast(pos, MULTICAST_PHS);
};

void(vector pos, string fx) FX_Explosion =
{
	float fxnum;
	if( fx == "small" )
		fxnum = TE_EXPLOSION2;
	else if( fx == "medium" )
		fxnum = TE_EXPLOSION1;
		
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(fxnum);
	MSG_WritePos(pos);
	MSG_Multicast(pos, MULTICAST_PHS /*MULTICAST_PVS*/);
};

/*
==============
FX_BulletImpact
==============
*/
void(vector pos, vector normal, string fx) FX_BulletImpact =
{
	float fxnum;
		
	if( fx == "blood" )
		fxnum = TE_BLOOD;
	else if( fx == "blood_green" )
		fxnum = TE_GREENBLOOD;
	else if( fx == "sparks" )
		fxnum = TE_SPARKS;
	else /*if( fx == "generic" )*/
		fxnum = TE_GUNSHOT;
						
	MSG_WriteByte(SVC_TEMP_ENTITY);
	MSG_WriteByte(fxnum);
	MSG_WritePos(pos);
	MSG_WriteDir(normal);
	MSG_Multicast(pos, MULTICAST_PVS);
};
				

/*
==============
FX_Muzzleflash
Player weapon fire sound and casts a muzzle flash
==============
*/
void(entity ent, float fx) FX_Muzzleflash =
{
	//svc_muzzleflash [entnum] [muzzleflashfx]
		
	MSG_WriteByte(SVC_MUZZLEFLASH);
	MSG_WriteShort( getentnum(ent) );
	MSG_WriteByte(fx);
	MSG_Multicast (ent.origin + '0 0 4', MULTICAST_PVS);	
};