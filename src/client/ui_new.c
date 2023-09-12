#if 0

#define CHAR_SIZEX 8

typedef enum
{
	XALIGN_NONE = 0,
	XALIGN_LEFT = 0,
	XALIGN_RIGHT = 1,
	XALIGN_CENTER = 2
} UI_AlignX;

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

#endif