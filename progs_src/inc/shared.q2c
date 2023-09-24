// globals shared by both client and server
// you can add new globals here

float FRAMETIME = 0.1;
float FALSE = 0;
float TRUE = 1;
float false = FALSE;
float true = TRUE;

float developer, developer_script; // cvars for debugging

string string_null = "";
vector vec_null = '0 0 0';
vector vec3_origin = vec_null; //temporary, ugly

float STEPSIZE	= 18; // its the same in C code for actors movestep and pmove
float MAXTOUCH	= 32; // same in C code

// usercmd_t is sent to the server each client frame
// if this is changed, the engine needs to update too!!!
typedef struct
{
	float	msec;			// byte
	float	buttons;		// byte
	vector	angles;			// short
	float	forwardmove, sidemove, upmove; // shorts
	float	impulse;		// [0-255], TODO reimplement impulses?
	float	lightlevel;		// [0-255], light level the player is standing on
} usercmd_t;

// -------- pmove.qc --------
// pm_flags 
// if this is changed, the engine needs to update too!!!
float PMF_DUCKED			= 1;
float PMF_JUMP_HELD			= 2;
float PMF_ON_GROUND			= 4;
float PMF_TIME_WATERJUMP	= 8;	// pm_time is waterjump
float PMF_TIME_LAND			= 16;	// pm_time is time before rejump
float PMF_TIME_TELEPORT		= 32;	// pm_time is non-moving time
float PMF_NO_PREDICTION		= 64;	// temporarily disables prediction (used for grappling hook)

enum :float // pmtype_t
{
	PM_NORMAL,		// can accelerate and turn
	PM_SPECTATOR,	// can accelerate and turn
	PM_DEAD, 		// no acceleration or turning
	PM_GIB,			// different bounding box
	PM_FREEZE		// no movement at all
};

// pmove_state_t is the information necessary for client side movement prediction
// this structure needs to be communicated bit-accurate from the server to the client to guarantee that
// prediction stays in sync, so no floats are used. if any part of the game code modifies this struct, 
// it will result in a prediction error of some degree.
// if this is changed, the engine needs to update too!!!
typedef struct
{
	float	pm_type;		// pmtype_t
	vector	origin;			// short
	vector	velocity;		// short
	float	pm_flags;		// ducked, jump_held, etc
	float	pm_time;		// each unit = 8 ms
	float	gravity;		// short
	vector	delta_angles;	// short, add to command angles to get view direction. changed by spawns, rotating objects, and teleporters
} pmove_state_t;

// if this is changed, the engine needs to update too!!!
typedef struct
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t		cmd;
	float			snapinitial;	// if s has been changed outside pmove

	// results (out)
	float			numtouch;
	entity			touchents[MAXTOUCH];

	vector			viewangles;			// clamped
	float			viewheight;

	vector			mins, maxs;			// bounding box size

	entity			groundentity;
	float			watertype;
	float			waterlevel;
} pmove_t;
// --------------------------