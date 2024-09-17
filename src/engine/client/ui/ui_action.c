/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// ui_action.c -- this is mostly a copy of Cmd_* functions adopted for UI action strings

#pragma once

#include "../client.h"
#include "../../script/scriptvm.h"

#include "ui_local.h"

#define MAX_UI_ACTIONS	32
typedef struct ui_action_s
{
	const char		*name;
	xcommand_t		function; // when command is declared in C
	scr_func_t		progsFunc; // when command is declared in QC
} ui_action_t;

static ui_action_t* ui_actions[MAX_UI_ACTIONS];	// possible commands to execute
static int			ui_actions_count = 0;

static sizebuf_t	uiaction_text;
static byte			uiaction_text_buf[1024];

/*
============
UI_AddAction
GUI progs can declare their own actions too
============
*/
void UI_AddAction(const char* cmd_name, xcommand_t function, scr_func_t progfunc)
{
	int i;
	ui_action_t* cmd;

	// fail if the command already exists
	for (i = 0; i < ui_actions_count; i++)
	{
		cmd = ui_actions[i];
		if (!strcmp(cmd_name, cmd->name))
		{
			Com_Printf("%s: %s already defined\n", __FUNCTION__, cmd_name);
			return;
		}
	}

	if (i == MAX_UI_ACTIONS)
	{
		Com_Error(ERR_DROP, "Increase MAX_UI_ACTIONS (%i)", MAX_UI_ACTIONS);
		return;
	}

	cmd = Z_Malloc(sizeof(ui_action_t));
	cmd->name = cmd_name;

	cmd->function = function;
	cmd->progsFunc = progfunc;


	ui_actions[ui_actions_count] = cmd;
	ui_actions_count++;
}

/*
============
UI_RemoveActions
============
*/
void UI_RemoveActions()
{
	for (int i = 0; i < ui_actions_count; i++)
	{
		Z_Free(ui_actions[i]);
		ui_actions[i] = NULL;
	}
	ui_actions_count = 0;
}

/*
============
UI_ExecuteActionString
Try to execute action string
============
*/
static void UI_ExecuteActionString(char* text)
{
	ui_action_t* cmd;

	Cmd_TokenizeString(text, false); // braxi -- no macros

	// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

	// check functions
	for (int i = 0; i < ui_actions_count; i++)
	{
		cmd = ui_actions[i];
		if (!Q_strcasecmp(Cmd_Argv(0), cmd->name))
		{
			if (cmd->function)
			{
				cmd->function();
			}
			else if (cmd->progsFunc != 0)
			{
				Scr_BindVM(VM_GUI);
				Scr_Execute(VM_GUI, cmd->progsFunc, __FUNCTION__);
			}
			else
			{
				Com_Printf("Unknown UI command: \"%s\"\n", Cmd_Argv(0));
			}

			return;
		}
	}


	print_time = false;
	Com_Printf("Unknown UI command: \"%s\"\n", Cmd_Argv(0));
	print_time = false;
}


/*
============
UIActionBuf_AddText
Adds action command text at the end of the buffer
============
*/
static void UIActionBuf_AddText(const char* text)
{
	int		l;

	l = strlen(text);

	if (uiaction_text.cursize + l >= uiaction_text.maxsize)
	{
		Com_Printf("UIActionBuf_AddText: overflow\n");
		return;
	}
	SZ_Write(&uiaction_text, (char*)text, strlen(text));
}

/*
============
UI_ExecuteAction
============
*/
void UI_ExecuteAction(const char *actionstring)
{
	int		i;
	char	* text;
	char	line[1024];
	int		quotes;

	if(actionstring != NULL)
		UIActionBuf_AddText(actionstring);

	while (uiaction_text.cursize)
	{
		// find a "\n" or ";" line break
		text = (char*)uiaction_text.data;

		quotes = 0;
		for (i = 0; i < uiaction_text.cursize; i++)
		{
			if (text[i] == '"')
				quotes++;
			if (!(quotes & 1) && text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		memcpy(line, text, i);
		line[i] = 0;

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec, alias) can insert data at the
		// beginning of the text buffer -- not really true with GUIs

		if (i == uiaction_text.cursize)
		{
			uiaction_text.cursize = 0;
		}
		else
		{
			i++;
			uiaction_text.cursize -= i;
			memmove(text, text + i, uiaction_text.cursize);
		}

		// execute the command line
		UI_ExecuteActionString(line);

	}
}

/*
============
UI_InitActions
============
*/
void UI_InitActions()
{
	if (ui_actions_count > 0)
	{
		Com_Error(ERR_FATAL, "gui system wasn't properly shutdown\n");
	}

	ui_actions_count = 0;
	SZ_Init(&uiaction_text, uiaction_text_buf, sizeof(uiaction_text_buf));
}

