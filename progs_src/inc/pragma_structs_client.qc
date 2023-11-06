// DO. NOT. EDIT.
// structures shared with engine
// this must be the first file to compile into client progs!
// add anything at the *end* of the file, it will help you maintaining parity with current server version


// ========================================================
// begin of engine defs 
// ========================================================

//
// clglobalvars_t - server globals
//
	float frametime;
	int time; 
	float realtime; 
	int vid_width;
	int vid_height;

	void() CG_Main;
	void() CG_Frame;	
	void() CG_DrawGUI;
	
void end_sys_globals;

	.string	str;
	.float	var;
	.entity	ent;
void end_sys_fields;