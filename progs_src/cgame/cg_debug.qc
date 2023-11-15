/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

float debuggui = 1;

void() debuggui_pmove;

void() CG_DebugGUI =
{
	if(!debuggui)
		return;
	
	if(cvarstring("pmdebug_0") != "none")
	{
		debuggui_pmove();
	}
};


void() debuggui_pmove =
{
	float fsize;
	vector xy_align;
	string cvarname;
	
	xy_align = '10 400 0';
	drawfill(xy_align_x-5, xy_align_y-3, 350, 100, '0 0 0', 0.5);	
	
	fsize = 1.2;
	drawstring(xy_align, fsize, '0.5 1 0.5', 1, "pmove debug");
	
	xy_align_y += 24;
	
	fsize = 0.9;
	
	cvarname = "pmdebug_0";
	if(cvarstring(cvarname) != "none")
	{
		drawstring(xy_align, fsize, '1 1 1', 1, cvarstring(cvarname));
		xy_align_y += 12;
	}

	cvarname = "pmdebug_1";
	if(cvarstring(cvarname) != "none")
	{
		drawstring(xy_align, fsize, '1 1 1', 1, cvarstring(cvarname));
		xy_align_y += 12;
	}
	
	cvarname = "pmdebug_2";
	if(cvarstring(cvarname) != "none")
	{
		drawstring(xy_align, fsize, '1 1 1', 1, cvarstring(cvarname));
		xy_align_y += 12;
	}
	
	cvarname = "pmdebug_3";
	if(cvarstring(cvarname) != "none")
	{
		drawstring(xy_align, fsize, '1 1 1', 1, cvarstring(cvarname));
		xy_align_y += 12;
	}
	
	cvarname = "pmdebug_4";
	if(cvarstring(cvarname) != "none")
	{
		drawstring(xy_align, fsize, '1 1 1', 1, cvarstring(cvarname));
		xy_align_y += 12;
	}
	
	cvarname = "pmdebug_5";
	if(cvarstring(cvarname) != "none")
	{
		drawstring(xy_align, fsize, '1 1 1', 1, cvarstring(cvarname));
		xy_align_y += 12;
	}	
};