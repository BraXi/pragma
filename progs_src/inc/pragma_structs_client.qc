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

void()		CG_Main;
void()		CG_Frame;	
void()		CG_DrawGUI;
	
void end_sys_globals;

.string	str;
.float	var1;
.entity	ent;
void end_sys_fields;