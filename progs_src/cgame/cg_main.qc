/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/


void() main =
{
	print("HELLO WORLD FROM QCVM ;)\n");

};


void() Draw2D =
{
	//drawfill( float x, float y, float w, float h, vector color, float alpha );
	drawfill( 200, 100, 60, 60, '1 0 0', 0.5 );
	drawfill( 200, 170, 60, 60, '0 1 0', 0.5 );
	drawfill( 200, 240, 60, 60, '0 0 1', 0.5 );	
	drawfill( 200, 310, 60, 60, '1 1 0', 0.5 );	
	
	//drawstring( vector xy_align, float fontSize, vector color, float alpha, string text, ... );
	
	vector xy_align = '280 120 0';
	
	drawstring( xy_align, 2.0, '1 1 1', 1, "hello ", "world ", "!" );
	
	xy_align_y += 65;
	drawstring( xy_align, 3.0, '1 1 1', 1, "str1 ", "str2 ", "str3" );

	xy_align = '400 400 2';
	drawstring( xy_align, 3.0, '1 1 1', 1, "client QC is up and running :3" );	
};