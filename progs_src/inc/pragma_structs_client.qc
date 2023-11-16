// DO. NOT. EDIT.
// structures shared with engine
// this must be the first file to compile into client progs!
// add anything at the *end* of the file, it will help you maintaining parity with current server version


// ========================================================
// begin of engine defs 
// ========================================================

//
// client game globals
//
float 		frametime;
int 		time; 
float 		realtime; 

int 		vid_width;
int 		vid_height;

float		localplayernum;

vector		v_forward, v_up, v_right;

float		trace_allsolid, trace_startsolid, trace_fraction, trace_plane_dist;
vector		trace_endpos, trace_plane_normal;
float		trace_entnum;
int			trace_contents;
string		trace_surface_name;
float		trace_surface_flags;
float		trace_surface_value;
entity		trace_ent;

float		pm_state_pm_type;		// byte
vector		pm_state_origin;		// floats
vector		pm_state_velocity;		// floats
float		pm_state_gravity;		// short
vector		pm_state_mins;			// char [-127,127 range]
vector		pm_state_maxs;			// char [-127,127 range]
float		pm_state_pm_flags;		// byte [0-255]
float		pm_state_pm_time;		// byte [0-255]
vector		pm_state_delta_angles;	// shorts, use ANGLE2SHORT/SHORT2ANGLE

vector		cam_viewangles;
vector		cam_viewoffset;
	
void()		CG_Main;
void()		CG_Frame;
void()		CG_DrawGUI;
void(vector cmdMove, vector cmdAngles, float cmdMsec)	CG_PlayerMove;
	
void end_sys_globals;

.string	str;
.float	var1;
.entity	ent;
void end_sys_fields;