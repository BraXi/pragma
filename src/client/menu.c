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


