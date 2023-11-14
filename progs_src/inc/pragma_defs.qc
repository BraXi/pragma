// DO. NOT. EDIT.
// defines in here must match q_shared.h !


// --------------------------------------
// shared by both client and server:
// --------------------------------------

// print levels
float PRINT_LOW		= 0;	// pickup messages
float PRINT_MEDIUM	= 1;	// death messages
float PRINT_HIGH	= 2;	// critical messages
float PRINT_CHAT	= 3;	// chat message

#define	CONTENTS_SOLID			1i		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2i		// translucent, but not watery
#define	CONTENTS_AUX			4i
#define	CONTENTS_LAVA			8i
#define	CONTENTS_SLIME			16i
#define	CONTENTS_WATER			32i
#define	CONTENTS_MIST			64i
#define	LAST_VISIBLE_CONTENTS		64i
// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL		0x8000i
#define	CONTENTS_PLAYERCLIP		0x10000i
#define	CONTENTS_MONSTERCLIP		0x20000i
// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000i
#define	CONTENTS_CURRENT_90		0x80000i
#define	CONTENTS_CURRENT_180		0x100000i
#define	CONTENTS_CURRENT_270		0x200000i
#define	CONTENTS_CURRENT_UP		0x400000i
#define	CONTENTS_CURRENT_DOWN		0x800000i
#define	CONTENTS_ORIGIN			0x1000000i	// removed before bsping an entity
#define	CONTENTS_MONSTER		0x2000000i	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER		0x4000000i
#define	CONTENTS_DETAIL			0x8000000i	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT		0x10000000i	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000i

// content masks
int MASK_ALL			= -1;
int MASK_SOLID			= (CONTENTS_SOLID|CONTENTS_WINDOW);
int MASK_PLAYERSOLID		= (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER);
int MASK_DEADSOLID		= (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW);
int MASK_MONSTERSOLID		= (CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER);
int MASK_WATER			= (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME);
int MASK_OPAQUE			= (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA);
int MASK_SHOT			= (CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER);
int MASK_CURRENT		= (CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN);

//surface flags
float SURF_LIGHT		= 0x1;		// value will hold the light strength
float SURF_SLICK		= 0x2;		// effects game physics (pmove.qc)
float SURF_SKY			= 0x4;		// don't draw, but add to skybox
float SURF_WARP			= 0x8;		// turbulent water warp
float SURF_TRANS33		= 0x10;		// 33% translucent
float SURF_TRANS66		= 0x20;		// 66% translucent
float SURF_FLOWING		= 0x40;		// scroll towards angle
float SURF_NODRAW		= 0x80;		// don't bother referencing the texture

// .svflags
float SVF_NOCLIENT			= 0x00000001;	// don't send entity to clients, even if it has effects
float SVF_DEADMONSTER		= 0x00000002;	// treat as CONTENTS_DEADMONSTER for collision
float SVF_MONSTER			= 0x00000004;	// treat as CONTENTS_MONSTER for collision

// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client even if it has a zero index model.
float EF_ROTATE			= 0x00000001;		// rotate (bonus items)
float EF_GIB			= 0x00000002;		// leave a trail
float EF_BLASTER		= 0x00000008;		// redlight + trail
float EF_ROCKET			= 0x00000010;		// redlight + trail
float EF_GRENADE		= 0x00000020;
float EF_HYPERBLASTER	= 0x00000040;
float EF_BFG			= 0x00000080;
float EF_COLOR_SHELL	= 0x00000100;
float EF_POWERSCREEN	= 0x00000200;
float EF_ANIM01			= 0x00000400;	// automatically cycle between frames 0 and 1 at 2 hz
float EF_ANIM23			= 0x00000800;	// automatically cycle between frames 2 and 3 at 2 hz
float EF_ANIM_ALL		= 0x00001000;	// automatically cycle through all frames at 2hz
float EF_ANIM_ALLFAST	= 0x00002000;	// automatically cycle through all frames at 10hz
float EF_FLIES			= 0x00004000;
float EF_QUAD			= 0x00008000;
float EF_PENT			= 0x00010000;
float EF_TELEPORTER		= 0x00020000;	// particle fountain
float EF_FLAG1			= 0x00040000;
float EF_FLAG2			= 0x00080000;
float EF_IONRIPPER		= 0x00100000; // begin RAFAEL
float EF_GREENGIB		= 0x00200000;
float EF_BLUEHYPERBLASTER = 0x00400000;
float EF_SPINNINGLIGHTS	= 0x00800000;
float EF_PLASMA			= 0x01000000;
float EF_TRAP			= 0x02000000; // end RAFAEL
float EF_TRACKER		= 0x04000000; //begin ROGUE
float EF_DOUBLE			= 0x08000000;
float EF_SPHERETRANS	= 0x10000000;
float EF_TAGTRAIL		= 0x20000000;
float EF_HALF_DAMAGE	= 0x40000000;
float EF_TRACKERTRAIL	= 0x80000000; //end ROGUE
//ROGUE

// entity_state_t->renderfx flags
#define	RF_MINLIGHT			1		// allways have some light, never go full dark
#define	RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define	RF_VIEW_MODEL		4		// only draw through eyes
#define	RF_FULLBRIGHT		8		// allways draw full intensity
#define	RF_DEPTHHACK		16		// for view weapon Z crunching
#define	RF_TRANSLUCENT		32		// entity uses .renderAlpha
#define	RF_FRAMELERP		64
#define RF_BEAM				128
#define	RF_COLOR			256		// entity uses .renderColor, was RF_CUSTOMSKIN
#define	RF_GLOW				512		// pulse lighting for bonus items
#define RF_SHELL_RED		1024
#define	RF_SHELL_GREEN		2048
#define RF_SHELL_BLUE		4096
#define RF_SCALE			8192	// entity uses .renderScale for rendering
#define RF_NOANIMLERP		16384	// animation is not lerped (software q1 style)
#define RF_UNUSED1			32768	


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

// temp entity events
// typedef enum temp_event_t
// Temp entity events are for things that happen at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed and broadcast.
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

float SPLASH_UNKNOWN		= 0;
float SPLASH_SPARKS			= 1;
float SPLASH_BLUE_WATER		= 2;
float SPLASH_BROWN_WATER	= 3;
float SPLASH_SLIME			= 4;
float SPLASH_LAVA			= 5;
float SPLASH_BLOOD			= 6;

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
float CHAN_AUTO 			= 0;
float CHAN_WEAPON			= 1;
float CHAN_VOICE 			= 2;
float CHAN_ITEM				= 3;
float CHAN_BODY				= 4;
float CHAN_EXTRA1			= 5;
float CHAN_EXTRA2			= 6;
float CHAN_EXTRA3			= 7;

// modifier flags
float CHAN_NO_PHS_ADD		= 8;	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
float CHAN_RELIABLE			= 16;	// send by reliable message, not datagram

// sound attenuation values
float ATTN_NONE				= 0;	// full volume the entire level
float ATTN_NORM				= 1;
float ATTN_IDLE				= 2;
float ATTN_STATIC			= 3;	// diminish very rapidly with distance

// player_state->stats[] indexes - 32 max
float STAT_HEALTH_ICON		= 0;
float STAT_HEALTH			= 1;
float STAT_AMMO_ICON		= 2;
float STAT_AMMO				= 3;
float STAT_ARMOR_ICON		= 4;
float STAT_ARMOR			= 5;
float STAT_SELECTED_ICON	= 6;
float STAT_PICKUP_ICON		= 7;
float STAT_PICKUP_STRING	= 8;
float STAT_TIMER_ICON		= 9;
float STAT_TIMER			= 10;
float STAT_HELPICON			= 11;
float STAT_SELECTED_ITEM	= 12;
float STAT_LAYOUTS			= 13;
float STAT_FRAGS			= 14;
float STAT_FLASHES			= 15;	// cleared each frame, 1 = health, 2 = armor
float STAT_CHASE			= 16;
float STAT_SPECTATOR		= 17;

float MAX_STATS				= 32;

// entity_state_t->event values
// ertity events are for effects that take place reletive to an existing entities origin.
// Very network efficient. All muzzle flashes really should be converted to events...
// typedef enum entity_event_t
float EV_NONE				= 0;
float EV_ITEM_RESPAWN		= 1;
float EV_FOOTSTEP			= 2;
float EV_FALLSHORT			= 3;
float EV_FALL				= 4;
float EV_FALLFAR			= 5;
float EV_PLAYER_TELEPORT	= 6;
float EV_OTHER_TELEPORT		= 7;

//
// per-level limits
// q_shared.h
//
float MAX_CLIENTS			= 256;		// absolute limit
float MAX_EDICTS			= 1024;		// must change protocol to increase more
float MAX_LIGHTSTYLES		= 256;
float MAX_MODELS			= 256;		// these are sent over the net as bytes
float MAX_SOUNDS			= 256;		// so they cannot be blindly increased
float MAX_IMAGES			= 256;
float MAX_ITEMS				= 256;
float MAX_GENERAL			= (MAX_CLIENTS*2);	// general config strings

//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
// q_shared.h
float CS_NAME				= 0;	// level name
float CS_CDTRACK			= 1;	// level music
float CS_SKY				= 2;	// skybox
float CS_SKYAXIS			= 3;	// %f %f %f format
float CS_SKYROTATE			= 4;	// dm, coop or singleplayer
float CS_STATUSBAR			= 5;	// display program string
float CS_AIRACCEL			= 29;	// air acceleration control
float CS_MAXCLIENTS			= 30;	// maxclients->value
float CS_MAPCHECKSUM		= 31;	// for catching cheater maps
float CS_MODELS				= 32;
float CS_SOUNDS				= (CS_MODELS+MAX_MODELS);
float CS_IMAGES				= (CS_SOUNDS+MAX_SOUNDS);
float CS_LIGHTS				= (CS_IMAGES+MAX_IMAGES);
float CS_ITEMS				= (CS_LIGHTS+MAX_LIGHTSTYLES);
float CS_PLAYERSKINS		= (CS_ITEMS+MAX_ITEMS);
float CS_GENERAL			= (CS_PLAYERSKINS+MAX_CLIENTS);
float MAX_CONFIGSTRINGS		= (CS_GENERAL+MAX_GENERAL);


// --------------------------------------
// server defines:
// --------------------------------------

// --------------------------------------
// client defines:
// --------------------------------------

// ====================================================
// end q_shared.h !!!
// ====================================================