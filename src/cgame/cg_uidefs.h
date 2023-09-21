/*
pragma engine
Copyright (C) 2023 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.
*/
//cg_uidefs.h - all definitions for UI and menu elements
#pragma once


#define ITEMFLAG_NONE		0
#define ITEMFLAG_IMAGE		2
#define ITEMFLAG_BORDER		4
#define ITEMFLAG_TEXT		8


typedef enum UI_ITEM_TYPE_s
{
	ITYPE_DEFAULT,
	ITYPE_BUTTON,
	ITYPE_DUMMY
} UI_ITEM_TYPE;

//rgba_t		rgba_def = { 1,1,1,1 };

// ========================================================================
// ItemDef
//
// Stores all variables used by UI elements
// ========================================================================
typedef struct ItemDef_s
{
	qboolean			used;
	UI_ITEM_TYPE		type;			// enum UI_ITEM_TYPE
	int					flags;			// ITEMFLAG_*

	qboolean			visible;		// don't draw & update if false
	qboolean			focused;		// is mouse over/item selected?

	qboolean			positiveDim;
	rect_t				rect;			// rectangle - XY position, width & height
	rect_t				collRect;

	float				borderSize;
	rgba_t				borderColor;	// rgba

	int			textAlign;		// enum UI_ITEM_ALIGN_HORIZONTAL
	rgba_t				textColor;		// rgba
	vec2_t				textOffset;

	float				fontSize;
	int					font;			// enum UI_FONT_TYPE

	char*				mat;			// background image
	rgba_t				color;			// rgba

	char *				name;
	char *				text;

	void				(*OnFocus)(struct ItemDef* self, qboolean state);	// called when mouse is over
	void				(*OnAction)(struct ItemDef* self);					// called on action
	void				(*OnSecondaryAction)(struct ItemDef* self);		// called on secondary right mouse button

} ItemDef;


// ========================================================================
// MenuDef
//
// 
// ========================================================================
typedef struct MenuDef_s
{
	char				*name;		// name of this menu
	qboolean			active;
	qboolean			opened;
	qboolean			drawCursor;

	ItemDef				itemList[128];
	unsigned int		numItemDefs;

	void				(*Init)(struct MenuDef_t* self);		// In here we add items
	void				(*Update)(struct MenuDef_t* self);		// In here we update special itemdefs
	void				(*OnOpen)(struct MenuDef_t* self);		// When menu opens
	void				(*OnClose)(struct MenuDef_t* self);	// ... and when it closes
} MenuDef_t;

extern cvar_t* cg_debugmenus;


extern void UIItem_SetDefaults(ItemDef* self);
extern void UIItem_Draw(ItemDef* self);
extern qboolean UIItem_IsMouseOver(int x, int y, int w, int h);

extern MenuDef_t* Menu_Create(const char *name);
extern void Menu_Free(MenuDef_t* self);
extern const char* Menu_GetName(MenuDef_t* menu);
extern qboolean Menu_IsOpen(MenuDef_t* menu);
extern qboolean Menu_IsActive(MenuDef_t* menu);
extern ItemDef* Menu_FindItem(MenuDef_t* menu, char* name);
extern ItemDef* Menu_AddItem(MenuDef_t* menu, int type, int flags, char* name);


extern void CG_UI_OpenMenu(const char* name);

#define MAX_MENUS	32
#define MENU_MAIN	0
extern MenuDef_t	menuList[MAX_MENUS];