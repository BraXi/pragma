/*
P R A G M A
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

// DO. NOT. EDIT.
// defines in here must match C code


//
// SERVER_FPS timings
//
float TIME_100_MS = 0.1;	// 10 ticks per second, standard Q2
float TIME_50_MS = 0.05;	// 20 ticks per second, pragma default
float TIME_25_MS = 0.025;	// 40 ticks per second, must compile engine with SERVER_FPS set to 40

//
// returned by trace function to mark nothing was hit, used in .groundentity_num
//
#define ENTITYNUM_NONE -1	

//
// angle indexes
//
#define	PITCH		0		// up / down
#define	YAW			1		// left / right
#define	ROLL		2		// fall over


//
// button bits
//
#define	BUTTON_ATTACK	1
#define	BUTTON_USE		2
#define	BUTTON_RELOAD	4
#define	BUTTON_MELEE	8
#define	BUTTON_ANY		128		// any key whatsoever


//
// print levels
//
float PRINT_LOW		= 0;	// pickup messages
float PRINT_MEDIUM	= 1;	// death messages
float PRINT_HIGH	= 2;	// critical messages
float PRINT_CHAT	= 3;	// chat message


//
// contents
//
int CONTENTS_SOLID			= 1i;		// an eye is never valid in a solid
int CONTENTS_WINDOW			= 2i;		// translucent, but not watery
int CONTENTS_AUX			= 4i;
int CONTENTS_LAVA			= 8i;
int CONTENTS_SLIME			= 16i;
int CONTENTS_WATER			= 32i;
int CONTENTS_MIST			= 64i;
int LAST_VISIBLE_CONTENTS	= CONTENTS_MIST;

// remaining contents are non-visible, and don't eat brushes
int CONTENTS_AREAPORTAL		= 0x8000i;
int CONTENTS_PLAYERCLIP		= 0x10000i;
int CONTENTS_MONSTERCLIP	= 0x20000i;

// currents can be added to any other contents, and may be mixed
int CONTENTS_CURRENT_0		= 0x40000i;
int CONTENTS_CURRENT_90		= 0x80000i;
int CONTENTS_CURRENT_180	= 0x100000i;
int CONTENTS_CURRENT_270	= 0x200000i;
int CONTENTS_CURRENT_UP		= 0x400000i;
int CONTENTS_CURRENT_DOWN	= 0x800000i;
int CONTENTS_ORIGIN			= 0x1000000i;	// removed before bsping an entity
int CONTENTS_MONSTER		= 0x2000000i;	// should never be on a brush, only in game
int CONTENTS_DEADMONSTER	= 0x4000000i;
int CONTENTS_DETAIL			= 0x8000000i;	// brushes to be added after vis leafs
int CONTENTS_TRANSLUCENT	= 0x10000000i;	// auto set if any surface has trans
int CONTENTS_LADDER			= 0x20000000i;


//
// content masks
//
int MASK_ALL			= -1;
int MASK_SOLID			= (CONTENTS_SOLID|CONTENTS_WINDOW);
int MASK_PLAYERSOLID	= (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER);
int MASK_DEADSOLID		= (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW);
int MASK_MONSTERSOLID	= (CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER);
int MASK_WATER			= (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME);
int MASK_OPAQUE			= (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA);
int MASK_SHOT			= (CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER);
int MASK_CURRENT		= (CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN);


//
// surface flags
//
float SURF_LIGHT		= 1;		// value will hold the light strength
float SURF_SLICK		= 2;		// effects game physics (pmove.qc)
float SURF_SKY			= 4;		// don't draw, but add to skybox
float SURF_WARP			= 8;		// turbulent water warp
float SURF_TRANS33		= 16;		// 33% translucent
float SURF_TRANS66		= 32;		// 66% translucent
float SURF_FLOWING		= 64;		// scroll towards angle
float SURF_NODRAW		= 128;		// don't bother referencing the texture


//
// entity server flags (.svflags)
//
#define	SVF_NOCLIENT		1	// don't send entity to clients, even if it has effects
#define	SVF_DEADMONSTER		2	// treat as CONTENTS_DEADMONSTER for collision
#define	SVF_MONSTER			4	// treat as CONTENTS_MONSTER for collision
#define	SVF_NOCULL			8	// entity will be _always_ sent regardless of PVS/PHS
#define	SVF_SINGLECLIENT	16	// send to only one client (.showto must be set to desider player entity number)
#define	SVF_ONLYTEAM		32	// send only to players in matching team team (.showto must match team)


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity. An entity that has 
// effects will be sent to the client even if it has a zero index model.
#define	EF_ROTATEYAW		1		// rotate
#define	EF_GIB				2		// leave a trail
#define	EF_BLASTER			4		// redlight + trail
#define	EF_ROCKET			8		// redlight + trail
#define	EF_GRENADE			16
#define	EF_HYPERBLASTER		32
#define	EF_ANIM01			64		// automatically cycle between frames 0 and 1 at 2 hz
#define	EF_ANIM23			128		// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			256		// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		512		// automatically cycle through all frames at 10hz
#define	EF_FLIES			1024
#define	EF_TELEPORTER		2048	// particle fountain
#define EF_FLAG1			4096
#define EF_FLAG2			8192
#define EF_IONRIPPER		16384
#define EF_GREENGIB			32768
#define	EF_BLUEHYPERBLASTER 65536
#define EF_PLASMA			131072
#define EF_TRAP				262144
#define EF_TRACKER_DLIGHT	524288
#define	EF_DOUBLE			1048576
#define EF_TAGTRAIL			2097152
#define EF_TRACKERTRAIL		4194304


//
// .renderFlags
//
#define	RF_MINLIGHT			1		// allways have some light, never go full dark
#define	RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define	RF_VIEW_MODEL		4		// only draw through eyes
#define	RF_FULLBRIGHT		8		// allways draw full intensity
#define	RF_DEPTHHACK		16		// entity can not be occluded by other geometry
#define	RF_TRANSLUCENT		32		// entity uses .renderAlpha
#define	RF_LIGHT			64		// entity has gamecode controlled dynamic light attached
#define RF_BEAM				128
#define	RF_COLOR			256		// entity uses .renderColor, was RF_CUSTOMSKIN
#define	RF_GLOW				512		// pulse lighting for bonus items
#define RF_SCALE			1024	// entity model is scaled by .renderScale
#define	RF_NOANIMLERP		2048	// animation is not lerped (software q1 style)
#define RF_NEGATIVE_LIGHT	4096	// light that substracts from world!
#define RF_YAWHACK			8192	// entity uses .renderScale for rendering

//
// svc_muzzleflash effects
//
enum
{
	FX_MUZZLEFLASH_PISTOL,
	FX_MUZZLEFLASH_RIFLE,
	FX_MUZZLEFLASH_SHOTGUN,
	FX_MUZZLEFLASH_NEGATIVE_LIGHT,

	// MOVE THESE SOMEWHERE ELSE
	FX_MUZZLEFLASH_LOGIN,
	FX_MUZZLEFLASH_LOGOUT,
	FX_MUZZLEFLASH_RESPAWN,
	FX_MUZZLEFLASH_ITEMRESPAWN,
	FX_MUZZLEFLASH_SILENCED = 128		// bit flag ORed with one of the above numbers
};


//
// TEMPORARY ENTITY EVENTS
// Temp entity events are for things that happen at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed and broadcast.
//
float TE_GUNSHOT				= 0; 	// [in]: vector pos, vector dir
float TE_BLOOD					= 1;	// [in]: vector pos, vector dir
float TE_BLASTER				= 2;
float TE_RAILTRAIL				= 3;
float TE_SHOTGUN				= 4;	// [in]: vector pos, vector dir
float TE_EXPLOSION1				= 5;
float TE_EXPLOSION2				= 6;
float TE_ROCKET_EXPLOSION		= 7;
float TE_GRENADE_EXPLOSION		= 8;
float TE_SPARKS					= 9;	// [in]: vector pos, vector dir
float TE_SPLASH					= 10;	// [in]: vector pos, vector dir, byte color
float TE_BUBBLETRAIL			= 11;
float TE_SCREEN_SPARKS			= 12;	// [in]: vector pos, vector dir
float TE_SHIELD_SPARKS			= 13;	// [in]: vector pos, vector dir
float TE_BULLET_SPARKS			= 14;	// [in]: vector pos, vector dir
float TE_LASER_SPARKS			= 15;
float TE_PARASITE_ATTACK		= 16;
float TE_ROCKET_EXPLOSION_WATER = 17;
float TE_GRENADE_EXPLOSION_WATER = 18;
float TE_MEDIC_CABLE_ATTACK 	= 19;
float TE_BFG_EXPLOSION			= 20;
float TE_BFG_BIGEXPLOSION		= 21;
float TE_BOSSTPORT				= 22; // used as '22' in a map = ; so DON'T RENUMBER!!!
float TE_BFG_LASER				= 23;
float TE_GRAPPLE_CABLE			= 24;
float TE_WELDING_SPARKS			= 25;
float TE_GREENBLOOD				= 26;
float TE_BLUEHYPERBLASTER		= 27;
float TE_PLASMA_EXPLOSION		= 28;
float TE_TUNNEL_SPARKS			= 29;
float TE_BLASTER2				= 30; /* begin ROGUE */
float TE_RAILTRAIL2				= 31;
float TE_FLAME					= 32;
float TE_LIGHTNING				= 33;
float TE_DEBUGTRAIL				= 34;
float TE_PLAIN_EXPLOSION		= 35;
float TE_FLASHLIGH				= 36;
float TE_FORCEWALL				= 37;
float TE_HEATBEAM				= 38;
float TE_MONSTER_HEATBEAM		= 39;
float TE_STEAM					= 40;
float TE_BUBBLETRAIL2			= 41;
float TE_MOREBLOOD				= 42;
float TE_HEATBEAM_SPARKS		= 43;
float TE_HEATBEAM_STEAM			= 44;
float TE_CHAINFIST_SMOKE		= 45;
float TE_ELECTRIC_SPARKS		= 46;
float TE_TRACKER_EXPLOSION		= 47;
float TE_TELEPORT_EFFECT		= 48;
float TE_DBALL_GOAL				= 49;
float TE_WIDOWBEAMOUT			= 50;
float TE_NUKEBLAST				= 51;
float TE_WIDOWSPLASH			= 52;
float TE_EXPLOSION1_BIG			= 53;
float TE_EXPLOSION1_NP			= 54;
float TE_FLECHETTE				= 55; /* end ROGUE */


//
// used for particle colors
//
float SPLASH_UNKNOWN		= 0;
float SPLASH_SPARKS			= 1;
float SPLASH_BLUE_WATER		= 2;
float SPLASH_BROWN_WATER	= 3;
float SPLASH_SLIME			= 4;
float SPLASH_LAVA			= 5;
float SPLASH_BLOOD			= 6;


//
// SOUND CHANNELS
//
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
//
float CHAN_AUTO 			= 0;
float CHAN_WEAPON			= 1;
float CHAN_VOICE 			= 2;
float CHAN_ITEM				= 3;
float CHAN_BODY				= 4;
float CHAN_EXTRA1			= 5;
float CHAN_EXTRA2			= 6;
float CHAN_EXTRA3			= 7;

//
// SOUND CHANNEL MODIFIER FLAGS
//
float CHAN_NO_PHS_ADD		= 8;	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
float CHAN_RELIABLE			= 16;	// send by reliable message, not datagram

//
// SOUND ATTENUATION VALUES
//
float ATTN_NONE				= 0;	// full volume the entire level
float ATTN_NORM				= 1;
float ATTN_IDLE				= 2;
float ATTN_STATIC			= 3;	// diminish very rapidly with distance

//
// ENTITY EVENTS
// are for effects that take place reletive to an existing entities origin. Very network efficient.
//
float EV_NONE				= 0;
float EV_ITEM_RESPAWN		= 1;
float EV_FOOTSTEP			= 2;
float EV_FALLSHORT			= 3;
float EV_FALL				= 4;
float EV_FALLFAR			= 5;
float EV_PLAYER_TELEPORT	= 6;
float EV_OTHER_TELEPORT		= 7;


//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most 64 (MAX_QPATH) characters.
//
#define	CS_NAME				0		// level name is set in worldspawn's 'message' key

#define	CS_SKY				1		// name of sky image without _<size> postfix
#define	CS_SKYAXIS			2		// %f %f %f RGB format
#define	CS_SKYROTATE		3		// boolean. does the sky rotate? 
#define	CS_SKYCOLOR			4		// %f %f %f RGB format

#define	CS_HUD				5		// display program string
#define CS_AIRACCEL			29		// air acceleration control
#define	CS_MAXCLIENTS		30