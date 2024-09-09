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
// sys_win.h

#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <mntent.h>

#include <dlfcn.h>

cvar_t *nostdout;
qboolean stdin_active = true;
uid_t saved_euid;

unsigned	sys_msg_time;
unsigned	sys_frame_time;

#define	MAX_NUM_ARGVS	128
int		argc;
char	*argv[MAX_NUM_ARGVS];


#define exit _exit

/*
===============================================================================

SYSTEM IO

===============================================================================
*/


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	
	// change stdin to non blocking
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);	

	Qcommon_Shutdown ();

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	fprintf(stderr, "Error: %s\n", text);
//	printf("Sys_Error: %s\n", text);

	exit (1);
}

void Sys_Quit (void)
{
	Qcommon_Shutdown ();
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	exit (0);
}


//================================================================

/*
================
Sys_Init
================
*/
void Sys_Init (void)
{

}
/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (!dedicated || !dedicated->value)
		return NULL;

	if (!stdin_active)
		return NULL;

	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
		return NULL;

	len = read (0, text, sizeof(text));
	if (len == 0) { // eof!
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
		return NULL;
	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
	// grab frame time 
	sys_frame_time = Sys_Milliseconds();	
}



/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	return data;
}


/*
==================
main

entry point for dedicated server
==================
*/
int main(int inargc, char** inargv)
{
	int		time, oldtime, newtime;


	// go back to real user for config loads
	saved_euid = geteuid();
	seteuid(getuid());
	
	Qcommon_Init(inargc, inargv);
	
	
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) 
	{
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
	}
	
	oldtime = Sys_Milliseconds();

	/* main essage loop */
	while (1)
	{
		sleep(0.1);
		
		newtime = Sys_Milliseconds();
		time = newtime - oldtime;
		if(time < 1)
			continue;				
		/*do
		{
			newtime = Sys_Milliseconds();
			time = newtime - oldtime;
		} while (time < 1);*/


		Qcommon_Frame(time);

		oldtime = newtime;
	}

	// never gets here
	return true;
}
