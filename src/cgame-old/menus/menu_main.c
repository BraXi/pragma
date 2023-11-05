#include "../cg_local.h"

static MenuDef_t* menu = NULL;
static ItemDef* title = NULL;

static void OnOpen(MenuDef_t* self)
{
}

static void OnClose(MenuDef_t* self)
{
}

static void ButtonFocus(ItemDef* self, qboolean state)
{
	if (state == false)
	{
		VectorSet(self->textColor, 0.92, 0.92, 0.92);
		self->textColor[3] = 1.0;
	}
	else
	{
		VectorSet(self->textColor, 0.9, 0.3, 0.1);
		self->textColor[3] = 1.0;
	}
}

extern void CG_UI_CloseMenu(char* name);
static void ButtonPlayAction(ItemDef* self)
{
	CG_UI_CloseMenu("main");
	CG_UI_OpenMenu("play");
}


static void ButtonHighScoresAction(ItemDef* self)
{
}


static void ButtonExitAction(ItemDef* self)
{
	CG_UI_OpenMenu("exit");
}

static void Update(struct MenuDef_t* self)
{
	if (title != NULL)
		title->fontSize = (float)(3.0f + (cos(loc.time / 500) * 0.3f));
}


void menu_main_init()
{
	ItemDef* item;

	menu = &menuList[MENU_MAIN];

	menu->name = "main";
	menu->Update = Update;
	menu->drawCursor = true;

	menu->OnOpen = OnOpen;
	menu->OnClose = OnClose;

#if 0
	item = Menu_AddItem(menu, ITYPE_DEFAULT, ITEMFLAG_IMAGE, "background");
	{
		item->mat = "backtile";
		item->rect[0] = item->rect[1] = 0;
		item->rect[2] = 800;
		item->rect[3] = 600;

		VectorSet(item->color, 1,1,1);
		item->color[3] = 0.7;
		item->visible = true;
	}
#endif
	item = Menu_AddItem(menu, ITYPE_DEFAULT, (ITEMFLAG_TEXT), "title");
	{
		item->fontSize = 3.0;
		item->textOffset[0] = 400;
		item->textOffset[1] = 200;
		item->textAlign = XALIGN_CENTER;
		item->text = "pragma";

		title = item;
	}

	item = Menu_AddItem(menu, ITYPE_DEFAULT, (ITEMFLAG_TEXT), "title2");
	{
		item->fontSize = 1;
		item->textOffset[0] = 450;
		item->textOffset[1] = 240;
		item->textAlign = XALIGN_CENTER;
		item->text = "now in truecolor RGBA!";
		item->textColor[3] = 0.5;
	}



	item = Menu_AddItem(menu, ITYPE_BUTTON, (ITEMFLAG_TEXT | ITEMFLAG_IMAGE | ITEMFLAG_BORDER), "play");
	{
		VectorSet(item->color, 0.5, 0.2, 0.2);
		item->color[3] = 0.4;

		item->rect[0] = 20;
		item->rect[1] = 400;
		item->rect[2] = 100;
		item->rect[3] = 30;

		item->fontSize = 1;

		item->textOffset[0] = 50;
		item->textOffset[1] = 10;

		VectorSet(item->textColor, 0.2, 0.2, 0.2, 1.0);
		item->textColor[3] = 1.0;

		item->textAlign = XALIGN_CENTER;
		item->text = "Continue";

		item->borderSize = 1.5;
		item->borderColor[0] = 1;
		item->borderColor[1] = 0;
		item->borderColor[2] = 0;
		item->borderColor[0] = 0.5;

		item->OnFocus = ButtonFocus;
		item->OnAction = ButtonExitAction;
		item->visible = true;
	}

	item = Menu_AddItem(menu, ITYPE_BUTTON, (ITEMFLAG_TEXT | ITEMFLAG_IMAGE), "options");
	{
		VectorSet(item->color, 0.5, 0.2, 0.2);
		item->color[3] = 0.4;

		item->rect[0] = 20;
		item->rect[1] = 450;
		item->rect[2] = 100;
		item->rect[3] = 30;

		item->fontSize = 1;

		item->textOffset[0] = 50;
		item->textOffset[1] = 10;

		VectorSet(item->textColor, 0.2, 0.2, 0.2, 1.0);
		item->textColor[3] = 1.0;

		item->textAlign = XALIGN_CENTER;
		item->text = "Options";

		item->OnFocus = ButtonFocus;
		item->OnAction = ButtonExitAction;
		item->visible = true;
	}

	item = Menu_AddItem(menu, ITYPE_BUTTON, (ITEMFLAG_TEXT | ITEMFLAG_IMAGE), "exit");
	{
		VectorSet(item->color, 0.5, 0.2, 0.2);
		item->color[3] = 0.4;

		item->rect[0] = 20;
		item->rect[1] = 500;
		item->rect[2] = 100;
		item->rect[3] = 30;

		item->fontSize = 1;

		item->textOffset[0] = 50;
		item->textOffset[1] = 10;

		VectorSet(item->textColor, 0.2, 0.2, 0.2, 1.0);
		item->textColor[3] = 1.0;

		item->textAlign = XALIGN_CENTER;
		item->text = "Disconnect";

		item->OnFocus = ButtonFocus;
		item->OnAction = ButtonExitAction;
		item->visible = true;
	}
}