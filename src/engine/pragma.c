/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// common.c -- misc functions used in client and server
#include "pragma.h"
#include <setjmp.h>

#define	MAXPRINTMSG	4096

#define MAX_NUM_ARGVS	50

qboolean print_time; // so the dedicated server can print time

int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];

int		realtime;

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame

#ifndef DEDICATED_ONLY
	FILE	*log_stats_file;
	cvar_t	*host_speeds;
	cvar_t	*log_stats;
#endif

cvar_t	*developer;
cvar_t	*timescale;
cvar_t	*fixedtime;
cvar_t	*logfile_active;	// 1 = buffer log, 2 = flush after each print
cvar_t	*showtrace;
cvar_t	*dedicated;

FILE	*logfile;

int			server_state;

// host_speeds times
int		time_before_game;
int		time_after_game;
int		time_before_ref;
int		time_after_ref;

/*
============================================================================

CLIENT / SERVER interactions

============================================================================
*/

static int	rd_target;
static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)(int target, char *buffer);

void Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush))
{
	if (!target || !buffer || !buffersize || !flush)
		return;
	rd_target = target;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	rd_flush(rd_target, rd_buffer);

	rd_target = 0;
	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.
=============
*/


void Com_Printf(char* fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	printf(msg);
#if 0
	if (dedicated != NULL && dedicated->value > 0 && print_time == true)
	{
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		printf("[%02d:%02d:%02d]: ", tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
#endif

	if (rd_target)
	{
		if (((int)strlen (msg) + (int)strlen(rd_buffer)) > (rd_buffersize - 1))
		{
			rd_flush(rd_target, rd_buffer);
			*rd_buffer = 0;
		}
		strcat (rd_buffer, msg);
		return;
	}

#ifndef DEDICATED_ONLY
	Con_Print (msg);
#endif

	// also echo to debugging console
	Sys_ConsoleOutput (msg);

	// logfile
	if (logfile_active && logfile_active->value)
	{
		char	name[MAX_QPATH];
		
		if (!logfile)
		{
			Com_sprintf (name, sizeof(name), "%s/console.log", FS_Gamedir ());
			logfile = fopen (name, "w");
		}
		if (logfile)
			fprintf (logfile, "%s", msg);
		if (logfile_active->value > 1)
			fflush (logfile);		// force it to save every time
	}
}


/*
================
Com_DPrintf

A Com_Printf that only shows up if the "developer" cvar is set
================
*/

void Com_DPrintf(dprintLevel_t chan, char *fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
		
	if (!developer || !developer->value)
		return;			// don't confuse non-developers with techie stuff...

	if (developer->value == DP_ALL || developer->value == chan || developer->value == 1337 && (chan == DP_NET || chan == DP_GAME || chan == DP_SV))
	{
		va_start(argptr, fmt);
		vsprintf(msg, fmt, argptr);
		va_end(argptr);

		Com_Printf("%s", msg);
	}
}


/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error (int code, char *fmt, ...)
{
	va_list		argptr;
	static char		msg[MAXPRINTMSG];
	static	qboolean	recursive;

	if (recursive)
		Sys_Error ("recursive error after: %s", msg);
	recursive = true;

	va_start (argptr,fmt);
	vsprintf (msg,fmt,argptr);
	va_end (argptr);
	

	//printf("Com_Error: %s\n", msg);
	if (code == ERR_DISCONNECT)
	{
		Com_Printf("%s\n", msg);

		if(Com_ServerState())
			SV_Shutdown("Server killed.", false);

#ifndef DEDICATED_ONLY
		CL_Drop ();
#endif
		recursive = false;
		longjmp (abortframe, -1);
	}
	else if (code == ERR_DROP)
	{
		Com_Printf ("********************\nERROR: %s\n********************\n", msg);
		SV_Shutdown (va("Server crashed: %s", msg), false);

#ifndef DEDICATED_ONLY
		CL_Drop ();
#endif
		recursive = false;
		longjmp (abortframe, -1);
	}
	else
	{
		SV_Shutdown (va("Server fatal crashed: %s", msg), false);

#ifndef DEDICATED_ONLY
		CL_Shutdown ();
#endif
	}

	if (logfile)
	{
		fclose (logfile);
		logfile = NULL;
	}

	Sys_Error ("%s", msg);
}


/*
=============
Com_Quit

Both client and server can use this, and it will do the apropriate things.
=============
*/
void Com_Quit (void)
{
	SV_Shutdown ("Server quit.", false);

#ifndef DEDICATED_ONLY
	CL_Shutdown ();
#endif
	if (logfile)
	{
		fclose (logfile);
		logfile = NULL;
	}

	Sys_Quit ();
}


/*
==================
Com_ServerState
==================
*/
int Com_ServerState (void)
{
	return server_state;
}

/*
==================
Com_SetServerState
==================
*/
void Com_SetServerState (int state)
{
	server_state = state;
}









/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void MSG_WriteDeltaEntity(entity_state_t* from, entity_state_t* to, sizebuf_t* msg, qboolean force, qboolean newentity)
{
	int		bits, i;

	if (!to->number)
		Com_Error(ERR_FATAL, "MSG_WriteDeltaEntity: Unset entity number");
	if (to->number >= MAX_GENTITIES)
		Com_Error(ERR_FATAL, "MSG_WriteDeltaEntity: Entity number >= MAX_GENTITIES");

	// send an update
	bits = 0;

	if (to->number >= 256)
		bits |= U_NUMBER_16;		// number8 is implicit otherwise
	
	if (to->eType != from->eType)
		bits |= U_ETYPE;

	// origin
	if (to->origin[0] != from->origin[0] || to->origin[1] != from->origin[1])
		bits |= U_ORIGIN_XY;
	if (to->origin[2] != from->origin[2])
		bits |= U_ORIGIN_Z;

	// angles
	if (to->angles[0] != from->angles[0])
		bits |= U_ANGLE_X;
	if (to->angles[1] != from->angles[1])
		bits |= U_ANGLE_Y;
	if (to->angles[2] != from->angles[2])
		bits |= U_ANGLE_Z;

	if (to->skinnum != from->skinnum)
		bits |= U_SKIN_8;

	if (to->frame != from->frame)
	{
		if (to->frame < 256)
			bits |= U_ANIMFRAME_8;
		else
			bits |= U_ANIMFRAME_16;
	}

	if (to->animationIdx != from->animationIdx || to->animStartTime != from->animStartTime)
	{
		bits |= U_ANIMATION;
	}

	if (to->effects != from->effects)
	{
		if (to->effects < 256)
			bits |= U_EFFECTS_8;
		else if (to->effects < 32768)
			bits |= U_EFFECTS_16;
		else
			bits |= U_EFFECTS_8 | U_EFFECTS_16;
	}

	if (to->renderFlags != from->renderFlags)
	{
		if (to->renderFlags < 256)
			bits |= U_RENDERFLAGS_8;
		else if (to->renderFlags < 32768)
			bits |= U_RENDERFLAGS_16;
		else
			bits |= U_RENDERFLAGS_8 | U_RENDERFLAGS_16;
	}

	if (to->packedSolid != from->packedSolid)
		bits |= U_PACKEDSOLID;

	// event is not delta compressed, just 0 compressed
	if (to->event)
		bits |= U_EVENT_8;

	// main model
	if (to->modelindex != from->modelindex || to->hidePartBits != from->hidePartBits)
	{
		bits |= U_MODELINDEX_16;
	}

	// attached models
	if (to->attachments[0].modelindex != from->attachments[0].modelindex || to->attachments[0].parentTag != from->attachments[0].parentTag)
		bits |= U_ATTACHMENT_1;
	if (to->attachments[1].modelindex != from->attachments[1].modelindex || to->attachments[1].parentTag != from->attachments[1].parentTag)
		bits |= U_ATTACHMENT_2;
	if (to->attachments[2].modelindex != from->attachments[2].modelindex || to->attachments[2].parentTag != from->attachments[2].parentTag)
		bits |= U_ATTACHMENT_3;

	// looping sound
	if (to->loopingSound != from->loopingSound)
		bits |= U_LOOPSOUND;

	// render scale
	if (to->renderScale != from->renderScale)
		bits |= U_RENDERSCALE;

	// render alpha
	if (to->renderAlpha != from->renderAlpha)
		bits |= U_RENDERALPHA;

	// render color
	if (to->renderColor[0] != from->renderColor[0] ||
		to->renderColor[1] != from->renderColor[1] ||
		to->renderColor[2] != from->renderColor[2])
		bits |= U_RENDERCOLOR;

	if (newentity || (to->renderFlags & RF_BEAM))
	{
		if (to->old_origin[0] != from->old_origin[0] || to->old_origin[1] != from->old_origin[1] || to->old_origin[2] != from->old_origin[2])
			bits |= U_OLDORIGIN;
	}

	//
	// write the message
	//
	if (!bits && !force)
		return;		// nothing to send!

	//----------

	if (bits & 0xff000000)
		bits |= U_MOREBITS_3 | U_MOREBITS_2 | U_MOREBITS_1;
	else if (bits & 0x00ff0000)
		bits |= U_MOREBITS_2 | U_MOREBITS_1;
	else if (bits & 0x0000ff00)
		bits |= U_MOREBITS_1;

	MSG_WriteByte(msg, bits & 255);

	if (bits & 0xff000000)
	{
		MSG_WriteByte(msg, (bits >> 8) & 255);
		MSG_WriteByte(msg, (bits >> 16) & 255);
		MSG_WriteByte(msg, (bits >> 24) & 255);
	}
	else if (bits & 0x00ff0000)
	{
		MSG_WriteByte(msg, (bits >> 8) & 255);
		MSG_WriteByte(msg, (bits >> 16) & 255);
	}
	else if (bits & 0x0000ff00)
	{
		MSG_WriteByte(msg, (bits >> 8) & 255);
	}

	//----------

	// ENT NUMBER
	if (bits & U_NUMBER_16)
		MSG_WriteShort(msg, to->number);
	else
		MSG_WriteByte(msg, to->number);

	// entity type
	if (bits & U_ETYPE)
		MSG_WriteByte(msg, to->eType);

	// model and part bits
	if (bits & U_MODELINDEX_16)
	{
		MSG_WriteShort(msg, to->modelindex);
		MSG_WriteByte(msg, to->hidePartBits);
	}

	// attached models
	if (bits & U_ATTACHMENT_1)
	{
		MSG_WriteShort(msg, to->attachments[0].modelindex);
		MSG_WriteByte(msg, to->attachments[0].parentTag);
	}
	if (bits & U_ATTACHMENT_2)
	{
		MSG_WriteShort(msg, to->attachments[1].modelindex);
		MSG_WriteByte(msg, to->attachments[1].parentTag);
	}
	if (bits & U_ATTACHMENT_3)
	{
		MSG_WriteShort(msg, to->attachments[2].modelindex);
		MSG_WriteByte(msg, to->attachments[2].parentTag);
	}

	// animation frame
	if (bits & U_ANIMFRAME_8)
		MSG_WriteByte(msg, to->frame);
	if (bits & U_ANIMFRAME_16)
		MSG_WriteShort(msg, to->frame);

	// animation sequence
	if (bits & U_ANIMATION)
	{
		MSG_WriteByte(msg, to->animationIdx);
		MSG_WriteLong(msg, to->animStartTime);
	}

	// index to model skin
	if (bits & U_SKIN_8)
		MSG_WriteByte(msg, to->skinnum);

	// effects
	if ((bits & (U_EFFECTS_8 | U_EFFECTS_16)) == (U_EFFECTS_8 | U_EFFECTS_16))
		MSG_WriteLong(msg, to->effects);
	else if (bits & U_EFFECTS_8)
		MSG_WriteByte(msg, to->effects);
	else if (bits & U_EFFECTS_16)
		MSG_WriteShort(msg, to->effects);

	// render flags
	if ((bits & (U_RENDERFLAGS_8 | U_RENDERFLAGS_16)) == (U_RENDERFLAGS_8 | U_RENDERFLAGS_16))
		MSG_WriteLong(msg, to->renderFlags);
	else if (bits & U_RENDERFLAGS_8)
		MSG_WriteByte(msg, to->renderFlags);
	else if (bits & U_RENDERFLAGS_16)
		MSG_WriteShort(msg, to->renderFlags);

	// render scale
	if (bits & U_RENDERSCALE)
	{
		if (to->renderScale > 15 || to->renderScale <= 0)
		{
			Com_Printf("%s: to->renderScale %f out of scale [0.0-15.0]\n", __FUNCTION__, to->renderScale);
			to->renderScale = 1.0f;
		}

		MSG_WriteByte(msg, (int)(to->renderScale * 16));
	}

	// render color
	if (bits & U_RENDERCOLOR)
	{
		for (i = 0; i < 3; i++)
		{
			if (to->renderColor[i] > 1.0f || to->renderColor[1] < 0.0f)
			{
				Com_Printf("%s: to->renderColor[%i] %f out of scale [0.0-1.0]\n", __FUNCTION__, i, to->renderColor[i]);
				to->renderScale = 1.0f;
			}
		}
		MSG_WriteByte(msg, (to->renderColor[0] * 255));
		MSG_WriteByte(msg, (to->renderColor[1] * 255));
		MSG_WriteByte(msg, (to->renderColor[2] * 255));
	}

	// render alpha
	if (bits & U_RENDERALPHA)
		MSG_WriteByte(msg, (to->renderAlpha * 255));

	// current origin
	if (bits & U_ORIGIN_XY)
	{
		MSG_WriteCoord(msg, to->origin[0]);
		MSG_WriteCoord(msg, to->origin[1]);
	}
	if (bits & U_ORIGIN_Z)
		MSG_WriteCoord (msg, to->origin[2]);

	// current angles
	if (bits & U_ANGLE_X)
		MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ANGLE_Y)
		MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ANGLE_Z)
		MSG_WriteAngle(msg, to->angles[2]);

	// old origin (used for smoothing move)
	if (bits & U_OLDORIGIN)
	{
		MSG_WriteCoord (msg, to->old_origin[0]);
		MSG_WriteCoord (msg, to->old_origin[1]);
		MSG_WriteCoord (msg, to->old_origin[2]);
	}

	// looping sound
	if (bits & U_LOOPSOUND)
	{
#if PROTO_SHORT_INDEXES
		MSG_WriteShort(msg, to->loopingSound);
#else
		MSG_WriteByte(msg, to->loopingSound);
#endif
	}

	// event
	if (bits & U_EVENT_8)
		MSG_WriteByte (msg, to->event);

	// solid
	if (bits & U_PACKEDSOLID)
	{
		MSG_WriteLong(msg, to->packedSolid);
	}
}


//===========================================================================


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm (char *parm)
{
	int		i;
	
	for (i=1 ; i<com_argc ; i++)
	{
		if (!strcmp (parm,com_argv[i]))
			return i;
	}
		
	return 0;
}

int COM_Argc (void)
{
	return com_argc;
}

char *COM_Argv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return "";
	return com_argv[arg];
}

void COM_ClearArgv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return;
	com_argv[arg] = "";
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, char **argv)
{
	int		i;

	if (argc > MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "argc > MAX_NUM_ARGVS");
	com_argc = argc;
	for (i=0 ; i<argc ; i++)
	{
		if (!argv[i] || strlen(argv[i]) >= MAX_TOKEN_CHARS )
			com_argv[i] = "";
		else
			com_argv[i] = argv[i];
	}
}

/*
================
COM_AddParm

Adds the given string at the end of the current argument list
================
*/
void COM_AddParm (char *parm)
{
	if (com_argc == MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "COM_AddParm: MAX_NUM_ARGS");
	com_argv[com_argc++] = parm;
}




/// just for debugging
int	memsearch (byte *start, int count, int search)
{
	int		i;
	
	for (i=0 ; i<count ; i++)
		if (start[i] == search)
			return i;
	return -1;
}


char *CopyString (const char *in)
{
	char	*out;
	
	out = Z_Malloc ((int)strlen(in)+1);
	strcpy (out, in);
	return out;
}



void Info_Print (char *s)
{
	char	key[512];
	char	value[512];
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}


/*
==============================================================================

						ZONE MEMORY ALLOCATION

just cleared malloc with counters now...

==============================================================================
*/

#define	Z_MAGIC		0x1d1d


typedef struct zhead_s
{
	struct zhead_s	*prev, *next;
	short		magic;
	memtag_t	tag;			// for group free
	int			size;
} zhead_t;

zhead_t		z_chain;
int		z_count, z_bytes;

/*
========================
Z_Free
========================
*/
void Z_Free (void *ptr)
{
	zhead_t	*z;

	z = ((zhead_t *)ptr) - 1;

	if (z->magic != Z_MAGIC)
		Com_Error (ERR_FATAL, "Z_Free: bad magic");

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	free (z);
}


/*
========================
Z_Stats_f
========================
*/
void Z_Stats_f (void)
{
	Com_Printf ("%i bytes in %i blocks\n", z_bytes, z_count);
}

/*
========================
Z_FreeTags
========================
*/
void Z_FreeTags (memtag_t tag)
{
	zhead_t	*z, *next;

	for (z=z_chain.next ; z != &z_chain ; z=next)
	{
		next = z->next;
		if (z->tag == tag)
			Z_Free ((void *)(z+1));
	}
}

/*
========================
Z_TagMalloc
========================
*/
void *Z_TagMalloc (int size, memtag_t tag)
{
	zhead_t	*z;
	
	size = size + sizeof(zhead_t);
	z = malloc(size);

	if (!z)
	{
		Com_Error(ERR_FATAL, "Z_TagMalloc: failed on allocation of %i bytes", size);
		return NULL; //msvc..
	}

	memset (z, 0, size);

	z_count++;
	z_bytes += size;

	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return (void *)(z+1);
}

/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (int size)
{
	return Z_TagMalloc (size, 0);
}

//====================================================================================

/*
=============
COM_NewString
=============
*/
char* COM_NewString(char* string, memtag_t memtag)
{
	char* newb, * new_p;
	int		i, l;

	l = (int)strlen(string) + 1;

	if (memtag > 0)
		newb = Z_TagMalloc(l, memtag);
	else
		newb = Z_Malloc(l);

	new_p = newb;

	for (i = 0; i < l; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}

qboolean COM_ParseField(char* key, char* value, byte* basePtr, parsefield_t* f)
{
	float	vec[4];

	for (; f->name; f++)
	{
		if (!strcmp(f->name, key))
		{
			// found it
			if (f->type == F_HACK && f->function != NULL)
			{
				f->function(value, basePtr);
				return true;
			}

			if (f->ofs == -1)
			{
				return true;
			}

			switch (f->type)
			{
			case F_INT:
				*(int*)(basePtr + f->ofs) = atoi(value);
				break;

			case F_FLOAT:
				*(float*)(basePtr + f->ofs) = atof(value);
				break;

			case F_BOOLEAN:
				if (!Q_stricmp(value, "true"))
					*(qboolean*)(basePtr + f->ofs) = true;
				else if (!Q_stricmp(value, "false"))
					*(qboolean*)(basePtr + f->ofs) = false;
				else
					*(qboolean*)(basePtr + f->ofs) = (atoi(value) > 0 ? true : false);
				break;

			case F_STRING:
				*(char**)(basePtr + f->ofs) = COM_NewString(value, 0); // FIXME memtag
				break;

			case F_VECTOR2:
				sscanf(value, "%f %f", &vec[0], &vec[1]);
				((float*)(basePtr + f->ofs))[0] = vec[0];
				((float*)(basePtr + f->ofs))[1] = vec[1];
				break;

			case F_VECTOR3:
				sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float*)(basePtr + f->ofs))[0] = vec[0];
				((float*)(basePtr + f->ofs))[1] = vec[1];
				((float*)(basePtr + f->ofs))[2] = vec[2];
				break;

			case F_VECTOR4:
				sscanf(value, "%f %f %f %f", &vec[0], &vec[1], &vec[2], &vec[3]);
				((float*)(basePtr + f->ofs))[0] = vec[0];
				((float*)(basePtr + f->ofs))[1] = vec[1];
				((float*)(basePtr + f->ofs))[2] = vec[2];
				((float*)(basePtr + f->ofs))[3] = vec[3];
				break;

			case F_IGNORE:
				break;
			}
			return true;
		}
	}
	return false;
}

//============================================================================

static byte chktbl[1024] = {
0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
byte	COM_BlockSequenceCRCByte (byte *base, int length, int sequence)
{
	int		n;
	byte	*p;
	int		x;
	byte chkb[60 + 4];
	unsigned short crc;


	if (sequence < 0)
		Sys_Error("sequence < 0, this shouldn't happen\n");

	p = chktbl + (sequence % (sizeof(chktbl) - 4));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CRC_Block(chkb, length);

	for (x=0, n=0; n<length; n++)
		x += chkb[n];

	crc = (crc ^ x) & 0xff;

	return crc;
}


/*
================
CRC_ChecksumFile
================
*/
unsigned short CRC_ChecksumFile(const char* name, qboolean fatal)
{
	int length;
	byte* buf;
	unsigned short checksum;
//	unsigned checksum;

	length = FS_LoadFile(name, (void**)&buf);
	if (!buf)
	{
		if(fatal)
			Com_Error(ERR_DROP, "File '%s' is missing\n", name);
		else
			Com_Printf("File '%s' is missing, no CRC check performed\n", name);

		return 0;
	}

	checksum = CRC_Block(buf, length); // CRC
//	checksum = LittleLong(Com_BlockChecksum(buf, length)); // MD4

	Z_Free(buf);
	return checksum;
}


//========================================================

float	frand(void)
{
	return (rand()&32767)* (1.0/32767);
}

float	crand(void)
{
	return (rand()&32767)* (2.0/32767) - 1;
}

void Key_Init (void);
void SCR_EndLoadingPlaque (void);

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
void Com_Error_f (void)
{
	Com_Error (ERR_FATAL, "%s", Cmd_Argv(1));
}


/*
=================
Qcommon_Init
=================
*/
void Qcommon_Init (int argc, char **argv)
{
	char	*s;

	print_time = false;
#ifdef DEDICATED_ONLY
	s = va("pragma dedicated server %s (%s %s %s)", PRAGMA_VERSION, CPUSTRING, __DATE__, BUILDSTRING);
	printf("%s\n", s);
	for( int i = 0; i < strlen(s); i++)
		printf("=");

	printf("\n\n");

	printf("Server runs game at %i ticks per second.\n", SERVER_FPS);
	printf("Protocol version is: %i.\n\n", PROTOCOL_VERSION);
#endif

	if (setjmp (abortframe) )
		Sys_Error ("Error during initialization");

	z_chain.next = z_chain.prev = &z_chain;

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	COM_InitArgv (argc, argv);

	Swap_Init ();
	Cbuf_Init ();

	Cmd_Init ();
	Cvar_Init ();

#ifndef DEDICATED_ONLY
	Key_Init ();
#endif

	// we need to add the early commands twice, because
	// a basedir or cddir needs to be set before execing
	// config files, but we want other parms to override
	// the settings of the config files
	Cbuf_AddEarlyCommands (false);
	Cbuf_Execute ();

	FS_InitFilesystem ();

#ifndef DEDICATED_ONLY
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_AddText ("exec config.cfg\n");
#endif

	Cbuf_AddEarlyCommands (true);
	Cbuf_Execute ();

	//
	// init commands and vars
	//
    Cmd_AddCommand ("z_stats", Z_Stats_f);
    Cmd_AddCommand ("error", Com_Error_f);

#ifndef DEDICATED_ONLY
	host_speeds = Cvar_Get ("host_speeds", "0", 0, NULL);
	log_stats = Cvar_Get ("log_stats", "0", 0, NULL);
#endif

	developer = Cvar_Get ("developer", "1337", 0, NULL);
	timescale = Cvar_Get ("timescale", "1", CVAR_CHEAT, NULL);
	fixedtime = Cvar_Get ("fixedtime", "0", CVAR_CHEAT, NULL);
	logfile_active = Cvar_Get ("logfile", "0", 0, NULL);
	showtrace = Cvar_Get ("showtrace", "0", 0, NULL);
#ifdef DEDICATED_ONLY
	dedicated = Cvar_Get ("dedicated", "1", CVAR_NOSET, NULL);
#else
	dedicated = Cvar_Get ("dedicated", "0", CVAR_NOSET, NULL);
#endif

	s = va("%s %s %s %s", PRAGMA_VERSION, CPUSTRING, __DATE__, BUILDSTRING);
	Cvar_Get ("version", s, CVAR_SERVERINFO|CVAR_NOSET, NULL);


	if (dedicated->value)
	{
		Cmd_AddCommand("quit", Com_Quit);

#ifndef DEDICATED_ONLY
		Com_Printf("pragma %s dedicated server\n", PRAGMA_VERSION);
		Com_Printf("build: %s\n", PRAGMA_TIMESTAMP);
		Com_Printf("------------------------------\n");
#endif
	}


	Sys_Init ();
	NET_Init ();
	Netchan_Init ();

	Scr_PreInitVMs();

	SV_Init ();

#ifndef DEDICATED_ONLY
	CL_Init ();
#endif

	// add + commands from command line
	if (!Cbuf_AddLateCommands ())
	{	
		// if the user didn't give any commands, run default action
		if (!dedicated->value)
			Cbuf_AddText ("ui_open main\n");
		else
			Cbuf_AddText ("dedicated_start\n");
		Cbuf_Execute ();
	}
	else
	{	
#ifndef DEDICATED_ONLY
		// the user asked for something explicit
		// so drop the loading plaque
		SCR_EndLoadingPlaque ();
#endif
	}


#ifndef DEDICATED_ONLY
	Com_Printf("====== pragma initialized ======\n\n");
#else
	printf("====== Server Initialized ======\n\n");
	print_time = true;

#if 0
	if (!logfile_active->value)
		printf("No active logging.\n");

	extern cvar_t* rcon_password;
	if (!rcon_password || strlen(rcon_password->string) == 0)
		printf("No rcon_password set.\n");

	if (Com_ServerState() == 0 /*ss_dead*/)
		printf("No map loaded, use `map` command to load map.\n");
#endif

#endif
}

/*
=================
Qcommon_Frame
=================
*/
#ifndef DEDICATED_ONLY
int		time_before, time_between, time_after, frame_time;
#endif

void Qcommon_Frame (int msec)
{
	char	*s;
	if (setjmp (abortframe) )
		return;			// an ERR_DROP was thrown

#ifndef DEDICATED_ONLY
	if ( log_stats->modified )
	{
		log_stats->modified = false;
		if ( log_stats->value )
		{
			if ( log_stats_file )
			{
				fclose( log_stats_file );
				log_stats_file = 0;
			}
			log_stats_file = fopen( "stats.log", "w" );
			if ( log_stats_file )
				fprintf( log_stats_file, "entities,dlights,parts,frame time\n" );
		}
		else
		{
			if ( log_stats_file )
			{
				fclose( log_stats_file );
				log_stats_file = 0;
			}
		}
	}
#endif /*DEDICATED_ONLY*/

	if (fixedtime->value)
	{
		msec = fixedtime->value;
	}
	else if (timescale->value)
	{
		msec *= timescale->value;
		if (msec < 1)
			msec = 1;
	}

	if (showtrace->value)
	{
		extern	int c_traces, c_brush_traces;
		extern	int	c_pointcontents;

		Com_Printf ("%4i traces  %4i points\n", c_traces, c_pointcontents);
		c_traces = 0;
		c_brush_traces = 0;
		c_pointcontents = 0;
	}

	do
	{
		s = Sys_ConsoleInput ();
		if (s)
			Cbuf_AddText (va("%s\n",s));
	} while (s);
	Cbuf_Execute ();

#ifndef DEDICATED_ONLY
	time_before = Sys_Milliseconds ();
#endif /*DEDICATED_ONLY*/

	SV_Frame (msec);

#ifndef DEDICATED_ONLY
	time_between = Sys_Milliseconds ();		
	CL_Frame (msec);

	time_after = Sys_Milliseconds ();		


	if (host_speeds->value)
	{
		int			all, sv, gm, cl, rf;

		all = time_after - time_before;
		sv = time_between - time_before;
		cl = time_after - time_between;
		gm = time_after_game - time_before_game;
		rf = time_after_ref - time_before_ref;
		sv -= gm;
		cl -= rf;
		Com_Printf ("all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
			all, sv, gm, cl, rf);
	}	
	frame_time = time_after - time_before;
#endif /*DEDICATED_ONLY*/
}

/*
=================
Qcommon_Shutdown
=================
*/
void Qcommon_Shutdown (void)
{
}
