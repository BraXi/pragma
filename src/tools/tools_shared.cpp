/*
prtool, part of pragma
Copyright (C) 2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "tools_shared.h"
#include <math.h>

#ifdef _WIN32
static HANDLE hConsole = NULL;
static WORD w_initialTextColor; // this is the console color

void InitConsole() 
{
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD attrib;
	static bool coninit = false;

	if (coninit)
		return;

	coninit = true;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	if (!GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
	{
#if _DEBUG
		Com_Warning("!GetConsoleScreenBufferInfo");
#endif
		return;
	}

	attrib = consoleInfo.wAttributes;
	w_initialTextColor = consoleInfo.wAttributes & 0x0F;
}
#endif

static int numWarnings = 0;

#define MAX_ARGS 80
#define MAX_TOKEN_CHARS 2048
static int com_argc = 0;
static char* com_argv[MAX_ARGS];
static char com_token[MAX_TOKEN_CHARS];
static char* com_args = NULL;

static const byte swaptest[2] = { 1,0 }; // endian test

void* Com_SafeMalloc(const size_t size, const char *sWhere)
{
	void* newmem = malloc(size);
	if (!newmem || newmem == NULL)
	{
		Com_Error("malloc(%i) failed in %s", (int)size, sWhere);
		return NULL;
	}
	memset(newmem, 0, size);
	return newmem;
}

void Com_Exit(const int code)
{
#ifdef _WIN32
	if(hConsole)
		SetConsoleTextAttribute(hConsole, w_initialTextColor);
#endif
	printf(CON_COL_WHITE);
	exit(1);
}

int GetNumWarnings()
{
	return numWarnings;
}

void Com_HappyPrintf(const char* text, ...) // because green means happy mkay? don't ask further
{
	va_list argptr;
	static char msg[MAXPRINTMSG];

	va_start(argptr, text);
	vsnprintf(msg, sizeof(msg), text, argptr);
	va_end(argptr);

#ifdef _WIN32
	InitConsole();
	if (hConsole)
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
#endif
	printf(CON_COL_GREEN"%s", msg);
}

void Com_Printf(const char* text, ...)
{
	va_list argptr;
	static char msg[MAXPRINTMSG];

	va_start(argptr, text);
	vsnprintf(msg, sizeof(msg), text, argptr);
	va_end(argptr);

#ifdef _WIN32
	InitConsole();
	if (hConsole)
		SetConsoleTextAttribute(hConsole, w_initialTextColor);
#endif
	printf(CON_COL_WHITE"%s", msg);
}

void Com_Warning(const char* text, ...)
{
	va_list argptr;
	static char msg[MAXPRINTMSG];

	va_start(argptr, text);
	vsnprintf(msg, sizeof(msg), text, argptr);
	va_end(argptr);

#ifdef _WIN32
	InitConsole();
	if (hConsole)
		SetConsoleTextAttribute(hConsole, (FOREGROUND_RED | FOREGROUND_GREEN));
#endif

	printf(CON_COL_YELLOW"Warning: %s\n", msg);

	numWarnings++;
}

void Com_Error2(const char* text, ...)
{
	va_list argptr;
	static char msg[MAXPRINTMSG];

	va_start(argptr, text);
	vsnprintf(msg, sizeof(msg), text, argptr);
	va_end(argptr);

#ifdef _WIN32
	InitConsole();
	if (hConsole)
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
#endif
	printf(CON_COL_RED"Error: %s\n", msg);

//	Com_Exit(1);
}

void Com_Error(const char* text, ...)
{
	va_list argptr;
	static char msg[MAXPRINTMSG];

	va_start(argptr, text);
	vsnprintf(msg, sizeof(msg), text, argptr);
	va_end(argptr);

#ifdef _WIN32
	InitConsole();
	if (hConsole)
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
#endif
	printf(CON_COL_RED"Fatal Error: %s\n", msg);

	Com_Exit(1);
}

FILE* Com_OpenReadFile(const char* filename, const int crash)
{
	FILE* handle = NULL;
	handle = fopen(filename, "rb");
	if (!handle || handle == NULL)
	{
		if(crash)
			Com_Error("Cannot open %s for reading.", filename);
		//else
		//	Com_Warning("Cannot open %s for reading.", filename);
	}	
	return handle;
}

FILE* Com_OpenWriteFile(const char* filename, const int crash)
{
	FILE* handle = NULL;
	handle = fopen(filename, "wb");
	if (!handle || handle == NULL)
	{
		if (crash)
			Com_Error("Cannot open %s for writing.\n", filename);
		//else
		//	Com_Warning("Cannot open %s for writing.\n", filename);
	}
	return handle;
}

void Com_SafeRead(FILE* f, void* buffer, const unsigned int count)
{
	if (fread(buffer, 1, count, f) != (size_t)count)
	{
		Com_Error("fread() failed.\n");
	}
}

void Com_SafeWrite(FILE* f, void* buffer, const unsigned int count)
{
	if (fwrite(buffer, 1, count, f) != (size_t)count)
	{
		Com_Error("fwrite() failed.\n");
	}
}

long Com_FileLength(FILE* f)
{
	long pos, end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);
	return end;
}

int Com_LoadFile(const char* filename, void** buffer, int crash)
{
	FILE *h;
	byte* buf = NULL;
	long fileLength;

	h = Com_OpenReadFile(filename, false);
	if (!h)
	{
		if (buffer)
			*buffer = NULL;
		if (crash)
			Com_Error("Failed to load file: \"%s\"", filename);
		return -1;
	}

	fileLength = Com_FileLength(h);

	if (!buffer)
	{
		fclose(h);
		return fileLength;
	}

	buf = (byte*)Com_SafeMalloc((size_t)(fileLength + (size_t)1), __FUNCTION__);
	*buffer = buf;

	fread(buf, fileLength, 1, h);

	fclose(h);
	return fileLength;
}

int Com_FileExists(const char* filename, ...)
{
	va_list argptr;
	static char realfilename[MAXPATH];
	FILE* f;

	va_start(argptr, filename);
	vsnprintf(realfilename, sizeof(realfilename), filename, argptr);
	va_end(argptr);

	f = Com_OpenReadFile(realfilename, false);
	if (f)
	{
		fclose(f);
		return 1;
	}
	return 0;
}

int Com_NumArgs()
{
	return com_argc;
}

char* Com_GetArg(int arg)
{
	if (arg >= com_argc)
		return (char*)"";
	return com_argv[arg];
}

char* Com_Args()
{
	if (!com_args)
		return (char*)"";
	return com_args;
}

char* Com_Parse(char* data)
{
	int        c;
	int        len;

	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

	// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;            // end of file;
		data++;
	}

	// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}


	// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	} while (c > 32);

	com_token[len] = 0;
	return data;
}

void Com_Tokenize(char* text)
{
	memset(com_argv, 0, sizeof(char*) * MAX_ARGS);

	com_argc = 0;
	com_args = NULL;

	while (1)
	{
		// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}

		if (*text == '\n')
		{    // a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;

		if (com_argc == 1)
			com_args = text;

		text = Com_Parse(text);

		if (!text)
			return;

		if (com_argc < MAX_ARGS)
		{
			com_argv[com_argc] = new char[strlen(com_token) + 1];
			strcpy(com_argv[com_argc], com_token);
			com_argc++;
		}
	}
}



int Com_EndianLong(int val)
{
	if (*(short*)swaptest == 1)
	{
		// little endian
		return val;
	}
	else
	{
		// big endian
		byte b1, b2, b3, b4;
		b1 = val & 255;
		b2 = (val >> 8) & 255;
		b3 = (val >> 16) & 255;
		b4 = (val >> 24) & 255;
		return ((int)b1 << 24) + ((int)b2 << 16) + ((int)b3 << 8) + b4;
	}
	
}

short Com_EndianShort(short val)
{
	if (*(short*)swaptest == 1)
	{
		// little endian
		return val;
	}
	else
	{
		// big endian
		byte b1, b2;
		b1 = val & 255;
		b2 = (val >> 8) & 255;
		return (b1 << 8) + b2;
	}
}

float Com_EndianFloat(float val)
{
	if (*(short*)swaptest == 1)
	{
		// little endian
		return val;
	}
	else
	{
		// big endian
		union
		{
			float	f;
			byte	b[4];
		} dat1, dat2;


		dat1.f = val;
		dat2.b[0] = dat1.b[3];
		dat2.b[1] = dat1.b[2];
		dat2.b[2] = dat1.b[1];
		dat2.b[3] = dat1.b[0];
		return dat2.f;
	}
}

int Com_stricmp(const char* s1, const char* s2)
{
#if defined(_WIN32)
	return _stricmp(s1, s2);
#else
	return strcasecmp(s1, s2);
#endif
}


vec_t VectorLength(vec3_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i = 0; i < 3; i++)
		length += v[i] * v[i];
	length = (float)sqrt(length);

	return length;
}

/*
===============
ClearBounds
===============
*/
void ClearBounds(vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999.0f;
	maxs[0] = maxs[1] = maxs[2] = -99999.0f;
}

/*
===============
AddPointToBounds
===============
*/
void AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i = 0; i < 3; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i = 0; i < 3; i++)
	{
		corner[i] = (float)(fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]));
	}

	return VectorLength(corner);
}