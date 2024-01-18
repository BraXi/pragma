/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <ctype.h>
#include "client.h"


void UI_DrawString(int x, int y, UI_AlignX alignx, char* string)
{
	int ofs_x = 0;

	// align text
	int strX = (strlen(string) * CHAR_SIZEX);
	if (alignx == XALIGN_CENTER)
	{
		ofs_x -= (strX / 2);
	}
	else if (alignx == XALIGN_RIGHT)
	{
		ofs_x -= ((strlen(string) * CHAR_SIZEX));
	}

	// draw string
	while (*string)
	{
		re.DrawChar(ofs_x + x, y, *string);
		x += CHAR_SIZEX;
		string++;
	}
}

void M_ForceMenuOff (void)
{
	cls.key_dest = key_game;
}


/*
=================
M_Draw
=================
*/
void M_Draw (void)
{
	static float fadeColor[4] = {0,0,0,0.4};
	if (cls.key_dest != key_menu)
		return;

	// repaint everything next frame
	SCR_DirtyScreen ();

	// dim everything behind it down
	if (cl.cinematictime > 0)
		re.DrawFill (0,0,viddef.width, viddef.height);
	else
	{
		if (Com_ServerState() == 0)
			fadeColor[3] = 1;
		else
			fadeColor[3] = 0.3;

		re.DrawFadeScreen(fadeColor);
	}
		
}


/*
=================
M_Keydown
=================
*/
void M_Keydown (int key)
{
}


