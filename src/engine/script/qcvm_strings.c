/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/


#include "../server/server.h"
#include "qcvm_private.h"

size_t Q_strlcat(char* dst, const char* src, size_t dsize);

const char** pr_stringTable = NULL;
static int pr_stringTableSize = 0;
static int pr_numStringsInTable = 0;

#define PR_TEMP_STRINGS 32 // number of temporary string buffers
#define	PR_TEMP_STRING_LEN 512 // length of a temporary string buffer
static char pr_tempStrings[PR_TEMP_STRINGS][PR_TEMP_STRING_LEN];
static int pr_numTempStrings = 0;


/*
============
Scr_MapTempStringsToStringTable
============
*/
static void Scr_MapTempStringsToStringTable()
{
	int strindex;

	pr_numTempStrings = 0;
	for (strindex = 0; strindex < PR_TEMP_STRINGS; strindex++)
	{
		pr_numStringsInTable++;
		pr_stringTable[strindex] = pr_tempStrings[strindex];
	}
}

/*
============
Scr_AllocStringTable
Create or expand strings table for the QCVM to properly address strings on non-32bit archs.
Reserves first few slots for the temporary strings mambo jambo.
============
*/
static void Scr_CreateStringTable()
{
	pr_stringTableSize += 128;

	// if no string table was initialized, create one
	if (pr_stringTable == NULL)
	{
		pr_numStringsInTable = 0;
		pr_stringTableSize += PR_TEMP_STRINGS; // add some space for tempstrings

		pr_stringTable = (const char**)malloc(pr_stringTableSize * sizeof(char*));
		if (pr_stringTable == NULL)
		{
			Com_Error(ERR_FATAL, "Could not allocate space for string table.\n");
			return;
		}

		Scr_MapTempStringsToStringTable();
		return;
	}

	// if the table exists -- expand it
	void* ptr = (void*)pr_stringTable;
	pr_stringTable = (const char**)realloc(ptr, pr_stringTableSize * sizeof(char*));
	if (pr_stringTable == NULL)
	{
		Com_Error(ERR_FATAL, "Could not expand string table.\n");
	}

	Com_Printf("Scr_AllocStringTable: %d string slots.\n", pr_stringTableSize);
}

/*
============
Scr_GetProgStringsSize
Returns the size of strings in a qcvm program file.
============
*/
static int Scr_GetProgStringsSize()
{
	CheckScriptVM(__FUNCTION__);
	return active_qcvm->progs->numstrings;
}


/*
============
Scr_SetTempString
Sets temporary string in script (the string will be overwritten later), 
used for builtins that return strings such as vtos(), ftos(), argv() etc..
============
*/
int Scr_SetTempString(const char* str)
{
	int strindex;

	if (pr_numTempStrings >= PR_TEMP_STRINGS)
		pr_numTempStrings = 0;

	strindex = pr_numTempStrings;
	pr_stringTable[strindex] = str;
	pr_numTempStrings++;

	return -1 - strindex;
}

/*
============
Scr_SetString
Sets persistant string in program
============
*/
int Scr_SetString(const char* str)
{
	int strindex;

	CheckScriptVM(__FUNCTION__);

	if (!str)
		return 0;

	// if the string lies within progs range, map it directly
	// bounds offset -2 to avoid trailing NULL char at the end of strings array
	if ((str >= active_qcvm->pStrings) && str <= (active_qcvm->pStrings + Scr_GetProgStringsSize() - 2)) 
	{	
		return (int)(str - active_qcvm->pStrings);
	}

	// negative str addresses are remapped to string table
	for (strindex = 0; strindex < pr_numStringsInTable; strindex++)
	{
		if (pr_stringTable[strindex] == str)
			return -1 - strindex;
	}

	if (strindex >= pr_stringTableSize)
	{
		// string table is at full capacity, allocate space for more entries
		Scr_CreateStringTable();
	}

	pr_numStringsInTable++;
	pr_stringTable[strindex] = str;

#ifdef _DEBUG
	Com_Printf("New string '%s' (%i strings, strtable for %i)\n", str, pr_numStringsInTable, pr_stringTableSize);
#endif
	return -1 - strindex;
}

/*
============
Scr_GetString
Returns string from active script
============
*/
const char* Scr_GetString(int num)
{
	//CheckScriptVM(__FUNCTION__); // done in Scr_GetProgStringsSize()
	if (num >= 0 && num < Scr_GetProgStringsSize())
	{
		return active_qcvm->pStrings + num;
	}
	else if (num < 0 && num >= -pr_numStringsInTable)
	{
		if (!pr_stringTable[-1 - num])
		{
			Com_Printf("PR_GetString: attempt to get a non-existant string %d\n", num);
			return "";
		}
		return pr_stringTable[-1 - num];
	}
	else
	{
		Com_Printf("PR_GetString: invalid string offset %d\n", num);
		return "";
	}
}

/*
============
Scr_AllocString
Allocates a new string
============
*/
scr_string_t Scr_AllocString(int size, char** ptr)
{
	scr_string_t strindex;

	if (!size)
		return 0;

	for (strindex = 0; strindex < pr_numStringsInTable; strindex++)
	{
		if (!pr_stringTable[strindex])
			break;
	}

	if (strindex >= pr_stringTableSize)
	{
		Scr_CreateStringTable();
	}

	pr_numStringsInTable++;

	pr_stringTable[strindex] = (char*)malloc(size);
	if (pr_stringTable[strindex] == NULL)
	{
		Com_Error(ERR_FATAL, "%s !malloc.\n", __FUNCTION__);
	}

	if (ptr)
	{
		*ptr = (char*)pr_stringTable[strindex];
	}

	return -1 - strindex;
}


/*
============
Scr_GrabTempStringBuffer
Returns an zero filled temporary string buffer.
============
*/
static char* Scr_GrabTempStringBuffer()
{
	char* strbuf = NULL;
	static int current_tempstr = 0;

	if (current_tempstr >= PR_TEMP_STRINGS)
		current_tempstr = 0;

	strbuf = pr_tempStrings[current_tempstr];
	memset(strbuf, 0, sizeof(pr_tempStrings[0]));
	current_tempstr++;

	return strbuf;
}

/*
============
Scr_VarString
Returns a single temporary string which will contain strings from first argument up to numargs.
============
*/
const char* Scr_VarString(int first)
{
	char* str;
	int	i;
	size_t len;

	str = Scr_GrabTempStringBuffer();

	if (first >= Scr_NumArgs() || first < 0)
		return str;

	for (i = first; i < Scr_NumArgs(); i++)
	{
		//strcat(str[varstring], Scr_GetParmString(i));
		len = Q_strlcat(str, Scr_GetParmString(i), sizeof(pr_tempStrings[0]));
		if (len >= sizeof(pr_tempStrings[0]))
		{
			Com_Printf("%s: string truncated.\n", __FUNCTION__);
			return str;
		}
		
	}
	return str;
}

size_t Q_strlcat(char* dst, const char* src, size_t dsize)
{
	// from https://github.com/libressl/openbsd/blob/master/src/lib/libc/string/strlcat.c
	const char* odst = dst;
	const char* osrc = src;
	size_t n = dsize;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end. */
	while (n-- != 0 && *dst != '\0')
		dst++;

	dlen = dst - odst;
	n = dsize - dlen;

	if (n-- == 0)
		return(dlen + strlen(src));

	while (*src != '\0') 
	{
		if (n != 0) 
		{
			*dst++ = *src;
			n--;
		}
		src++;
	}
	*dst = '\0';

	return(dlen + (src - osrc));	/* count does not include NULL */
}
