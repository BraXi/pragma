/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/

#include "cg_local.h"



static unsigned int		numOpenedMenus;
MenuDef_t		menuList[MAX_MENUS];

qboolean				ui_drawCursor;
qboolean				ui_updateMousePos;

cvar_t* cg_debugmenus;


static void UI_AdjustToVirtualScreenSize(float* x, float* y)
{
	float    xscale;
	float    yscale;

	xscale = loc.width / 800.0;
	yscale = loc.height / 600.0;

	if (x) *x *= xscale;
	if (y) *y *= yscale;
}


void RgbaSet(rgba_t c, float r, float g, float b, float a)
{
	c[0] = r;
	c[1] = g;
	c[2] = b;
	c[3] = a;
}

static void UI_DrawColoredQuad(rect_t rect, rgba_t color)
{
	gi.DrawFill(rect, color);
}

static void UI_DrawTexturedQuad(rect_t rect, rgba_t color, char *name)
{
	if (!name)
		return;

	gi.DrawStretchedImage(rect, color, name);
}

static void UI_SetDrawColor(rgba_t color)
{

}

static void UI_DrawString(char* string, float x, float y, float fontSize, int alignx, rgba_t color)
{
	gi.DrawString(string, x, y, fontSize, alignx, color);
}


// ---------------------------------------------------------------------------
// MENU ITEMS
// ---------------------------------------------------------------------------

void UIItem_SetDefaults(ItemDef *self)
{
	memset(self, 0, sizeof(ItemDef));

	self->visible = true;

	self->borderSize = 1.0f;
	RgbaSet(&self->borderColor, 1, 1, 1, 1);

	RgbaSet(&self->textColor, 1, 1, 1, 1);
	self->fontSize = 1;

	RgbaSet(&self->color, 1, 1, 1, 1);
}


/*
=================
Update
=================
*/
void UIItem_Update(ItemDef *self)
{
	if (!self->visible)
		return;

	if (self->rect[2] > 0 && self->rect[3] > 0)
		self->positiveDim = true;
	else
		self->positiveDim = false;

	self->collRect[0] = self->rect[0];
	self->collRect[1] = self->rect[1];
	self->collRect[2] = self->rect[2];
	self->collRect[3] = self->rect[3];

	UI_AdjustToVirtualScreenSize(&self->collRect[0], &self->collRect[1]);
	UI_AdjustToVirtualScreenSize(&self->collRect[2], &self->collRect[3]);


	if(self->positiveDim && UIItem_IsMouseOver(self->collRect[0], self->collRect[1], self->collRect[2], self->collRect[3]))
		self->focused = true;
	else
		self->focused = false;

	if (self->OnFocus != NULL)
	{
		self->OnFocus(self, self->focused);
	}

	switch (self->type)
	{
	case ITYPE_DEFAULT:
		break;

	case ITYPE_BUTTON:
		if (self->focused)
		{
#if 0
			if (gi.MouseButtonPressed(MB_LEFT) && self->OnAction != NULL)
			{
				gi.PlayLocalSound("ui/click1.wav");
				self->OnAction(self);
				Engine->p_Input->m_MouseButtons[MB_LEFT] = false;
				self->focused = false;
			}
			else if (gi.MouseButtonPressed(MB_RIGHT) && self->OnSecondaryAction != NULL)
			{
				gi.PlayLocalSound("ui/click2.wav");
				self->OnSecondaryAction(self);
				Engine->p_Input->m_MouseButtons[MB_RIGHT] = false;
				self->focused = false;
			}
#endif
		}
		break;
	}
}

void UIItem_Draw(ItemDef *self)
{
	rect_t line_rect;

	if (!self->visible)
		return;

	if (self->positiveDim)
	{
		// image
		if ((self->flags & ITEMFLAG_IMAGE))
		{
			//UI_SetDrawColor( self->color );
			if (self->mat != NULL)
			{
				UI_DrawTexturedQuad(self->rect, self->color, self->mat);
			}
			else
			{
				//gi.BindImage(0);
				UI_DrawColoredQuad(self->rect, self->color);
			}
		}

		// border
		if ((self->flags & ITEMFLAG_BORDER))
		{
			//ui_SetDrawColor( borderColor );

			// left line			
			line_rect[0] = self->rect[0] - self->borderSize;
			line_rect[1] = self->rect[1] - self->borderSize;
			line_rect[2] = self->borderSize;
			line_rect[3] = self->rect[3] + (self->borderSize * 2);
			UI_DrawColoredQuad(line_rect, self->borderColor);

			// right line
			line_rect[0] = self->rect[0] + self->rect[2];
			line_rect[1] = self->rect[1] - self->borderSize;
			line_rect[2] = self->borderSize;
			line_rect[3] = self->rect[3] + (self->borderSize * 2);
			UI_DrawColoredQuad(line_rect, self->borderColor);

			// top line
			line_rect[0] = self->rect[0];
			line_rect[1] = self->rect[1] - self->borderSize;
			line_rect[2] = self->rect[2];
			line_rect[3] = self->borderSize;
			UI_DrawColoredQuad(line_rect, self->borderColor);

			// bottom line
			line_rect[0] = self->rect[0];
			line_rect[1] = self->rect[1] + self->rect[3];
			line_rect[2] = self->rect[2];
			line_rect[3] = self->borderSize;
			UI_DrawColoredQuad(line_rect, self->borderColor);
		}
	}


	// string
	if (self->text && strlen(self->text) && (self->flags & ITEMFLAG_TEXT))
	{
		UI_SetDrawColor(self->textColor);
		UI_DrawString(self->text, self->rect[0] + self->textOffset[0], self->rect[1] + self->textOffset[1], self->fontSize, self->textAlign, self->textColor);
	}
}

qboolean UIItem_IsMouseOver(int x, int y, int w, int h)
{
	vec2_t mpos;
	static vec2_t points[2];
	points[0][0] = x;
	points[0][1] = y;

	points[1][0] = x + w;
	points[1][1] = y + h;

	gi.GetCursorPos(&mpos);

	if (mpos[0] >= points[0][0] && mpos[1] >= points[0][1] && mpos[0] <= points[1][0] && mpos[1] <= points[1][1])
		return true;

	return false;
}

// ---------------------------------------------------------------------------
// MENUS
// ---------------------------------------------------------------------------


void Menu_Free(MenuDef_t *self)
{
	gi.TagFree(self->itemList);
	memset(self, 0, sizeof(MenuDef_t));
}


const char *Menu_GetName(MenuDef_t *menu)
{
	return menu->name;
}


qboolean Menu_IsOpen(MenuDef_t* menu)
{
	return menu->opened;
}


qboolean Menu_IsActive(MenuDef_t* menu)
{
	return menu->active;
}


/*
=================
Menu_FindItem

Search for ItemDef by name and return pointer to it, NULL if not found
=================
*/
ItemDef* Menu_FindItem(MenuDef_t* menu, char *name)
{
	unsigned int	i;

	if (!name)
		return NULL;

	for (i = 0; i < menu->numItemDefs; i++)
	{
		if (!strcmp(name, menu->itemList[i].name))
		{
			return &menu->itemList[i];
		}		
	}
	return NULL;
}


/*
=================
Menu_AddItem

Add new item to menu
=================
*/
ItemDef* Menu_AddItem(MenuDef_t* menu, int type, int flags, char *name)
{
	if (!menu)
		return NULL;


	ItemDef* item = &menu->itemList[menu->numItemDefs];

//	memset(item, 0, sizeof(ItemDef));
	UIItem_SetDefaults(item);

	item->type = type;
	item->flags = flags;
	item->name = name;

	item->fontSize = 1;
	VectorSet(item->textColor, 1, 1, 1);
	item->textColor[3] = 1;

	VectorSet(item->color, 0,0,0);
	item->color[3] = 1;

	item->visible = true;

	item->used = true;

	menu->numItemDefs++;
	return item;
}


// ---------------------------------------------------------------------------
// USER INTERFACE
// ---------------------------------------------------------------------------

unsigned int UI_GetNumMenus()
{
	return MAX_MENUS;
}

/*
=================
CG_UI_IsMenuOpen

Returns true if menu is open
=================
*/
qboolean CG_UI_IsMenuOpen(const char *name)
{
	unsigned int	i;
	MenuDef_t *menu;

	for (i = 0; i < MAX_MENUS; i++)
	{
		menu = &menuList[i];
		if (!menu->name)
			continue;
			
		if (!strcmp(name, menu->name) && Menu_IsOpen(menu))
			return true;
	}
	return false;
}


/*
=================
CG_UI_OpenMenu
=================
*/
void CG_UI_OpenMenu(const char* name)
{
	unsigned int	i, j;
	MenuDef_t* menu;


	if (!UI_GetNumMenus())
	{
		gi.Error("%s: No MenuDefs\n", __FUNCTION__);
		return;
	}

	for (i = 0; i < UI_GetNumMenus(); i++)
	{
		menu = &menuList[i];

		if (!strcmp(name, menu->name) && !Menu_IsOpen(menu))
		{
			numOpenedMenus++;
			menu->opened = true;
			menu->active = true;

			if(cg_debugmenus->value)
				gi.Printf( "%s: opened %s\n", __FUNCTION__, menu->name );

			if (menu->OnOpen != NULL)
				menu->OnOpen(menu);

			// deactivate other menus
			for (j = 0; j < UI_GetNumMenus(); j++)
			{
				menu = &menuList[j];
				if (Menu_IsActive(menu) && j != i)
					menu->active = false;
			}
			return;
		}
	}

	if (cg_debugmenus->value)
		gi.Printf("%s: \"%s\" does not exist\n", __FUNCTION__, name);
}

/*
=================
CloseMenu
=================
*/
void CG_UI_CloseMenu(const char * name)
{
	unsigned int	i, j;
	MenuDef_t* menu;

	for (i = 0; i < UI_GetNumMenus(); i++)
	{
		menu = &menuList[i];

		if (!strcmp(name, menu->name) && !Menu_IsOpen(menu))
		{
			numOpenedMenus--;
			menu->opened = false;
			menu->active = false;

			if (menu->OnClose != 0x00000000)
				menu->OnClose(menu);

			if (cg_debugmenus->value)
				gi.Printf("%s: closed %s\n", __FUNCTION__, name);

			// activate previous menu
			for (j = UI_GetNumMenus() - 1; j != -1; j--)
			{
				menu = &menuList[j];
				if (Menu_IsOpen(menu) && !Menu_IsActive(menu))
				{
					menu->active = true;
					if (cg_debugmenus->value)
						gi.Printf("%s: %s is the new active menu\n", __FUNCTION__, menu->name);
				}
			}
			return;
		}
	}
}

void CG_UI_Frame(float time)
{
	unsigned int	i, j;
	MenuDef_t		*menu;
	ItemDef			*item;

	if (numOpenedMenus)
	{
		ui_updateMousePos = true;
		ui_drawCursor = true;
#if 0
		// keep mouse inside window
		vec2_t mpos;
		gi.GetCursorPos(&mpos);	
		if(mpos[0] < 0 )
			 gi.SetCursorPos(0, mpos[1]);
		else if( mpos[0] > 639 )
			gi.SetCursorPos( 639, mpos[1]);

		if(mpos[1] < 0 )
			 gi.SetCursorPos( mpos[0], 0);
		else if( mpos[1] > 479)
			gi.SetCursorPos(mpos[0], 479 );
#endif
	}
	else
	{
		ui_drawCursor = false;
	}

	for (i = 0; i < UI_GetNumMenus(); i++)
	{
		menu = &menuList[i];

		if (!Menu_IsOpen(menu) || !Menu_IsActive(menu))
			continue;

		if (menu->Update != NULL)
			menu->Update(menu);

		for (j = 0; j < menu->numItemDefs; j++)
		{
			item = &menu->itemList[j];
			if (item->used)
			{
				UIItem_Update(item);
			}
		}
	}
}



void CG_UI_DrawMenus()
{
	static unsigned int	i, j;
	MenuDef_t* menu;
	for (i = 0; i < UI_GetNumMenus(); i++)
	{
		menu = &menuList[i];

		if (!Menu_IsOpen(menu) /*|| !Menu_IsActive(menu)*/)
			continue;

		for (j = 0; j < menu->numItemDefs; j++)
		{
			if (menu->itemList[j].used)
			{
				UIItem_Draw(&menu->itemList[j]);
			}
		}
	}
}



float rx, ry;

rgba_t c_white = { 1,1,1,1 };
rgba_t c_red = { 1,0,0,1 };
rgba_t c_green = { 0,1,0,1 };
rgba_t c_blue = { 0,0,1,1 };
rgba_t c_yellowa = { 1,1,1,0.5 };
rect_t img = { 100,100,200,200 };

void CG_DrawUI(void)
{
#if 0
	rx = 400 - (cos(loc.time / 800) * 40);
	ry = 420;

	UI_DrawString("QC is lyfe\n", 400, 250, 1, XALIGN_RIGHT, c_white);

	UI_DrawString("CGAME\n", rx, 300, 3, XALIGN_CENTER, c_red);

	UI_DrawString("DLL is meh\n", 400, 350, 1, XALIGN_LEFT, c_green);

	rx = 400 + (cos(loc.time / 800) * 40);
	UI_DrawString("SCALED TEXT\n", rx, ry, 6, XALIGN_CENTER, c_blue);

	img[0] = img[1] = 300;
	img[2] = img[3] = 300;

	gi.DrawStretchedImage(img, c_yellowa, "backtile");

	img[0] = img[1] = 100;
	img[2] = img[3] = 200;
	gi.DrawFill(img, c_yellowa);
#endif
	//CG_UI_DrawMenus();
}

void menu_main_init();

void CG_InitMenus(void)
{
	memset(menuList, 0, sizeof(menuList));

	menu_main_init();
	CG_UI_OpenMenu("main");
}