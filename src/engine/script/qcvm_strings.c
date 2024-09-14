/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

//quakespasm inspired

#include "../server/server.h"
#include "qcvm_private.h"


static qcvm_strings_t* pVMStr = NULL;

/*
============
Scr_MapTempStringsToStringTable
============
*/
static void Scr_MapTempStringsToStringTable()
{
	int strindex;

	pVMStr->numTempStrings = 0;
	for (strindex = 0; strindex < PR_TEMP_STRINGS; strindex++)
	{
		pVMStr->stringTable[strindex] = pVMStr->tempStrings[strindex];
		pVMStr->numStringsInTable++;
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
	CheckScriptVM(__FUNCTION__);
	pVMStr = &active_qcvm->strTable;

	pVMStr->stringTableSize += 128;

	// if no string table was initialized, create one
	if (pVMStr->stringTable == NULL)
	{
		pVMStr->numStringsInTable = 0;
		pVMStr->stringTableSize += PR_TEMP_STRINGS; // add some space for tempstrings

		pVMStr->stringTable = (const char**)malloc(pVMStr->stringTableSize * sizeof(char*));
		if (pVMStr->stringTable == NULL)
		{
			Com_Error(ERR_FATAL, "Could not allocate space for string table.\n");
			return;
		}

		Scr_MapTempStringsToStringTable();
		return;
	}

	// if the table exists -- expand it
	void* ptr = (void*)pVMStr->stringTable;
	pVMStr->stringTable = (const char**)realloc(ptr, pVMStr->stringTableSize * sizeof(char*));
	if (pVMStr->stringTable == NULL)
	{
		Com_Error(ERR_FATAL, "Could not expand string table.\n");
	}

	Com_Printf("Scr_AllocStringTable: %d string slots.\n", pVMStr->stringTableSize);
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

	CheckScriptVM(__FUNCTION__);
	pVMStr = &active_qcvm->strTable;

	if (pVMStr->numTempStrings >= PR_TEMP_STRINGS)
		pVMStr->numTempStrings = 0;

	strindex = pVMStr->numTempStrings;
	pVMStr->stringTable[strindex] = str;
	pVMStr->numTempStrings++;

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
	pVMStr = &active_qcvm->strTable;

	if (!str)
		return 0;

	// if the string lies within progs range, map it directly
	// bounds offset -2 to avoid trailing NULL char at the end of strings array
	if ((str >= active_qcvm->pStrings) && str <= (active_qcvm->pStrings + Scr_GetProgStringsSize() - 2)) 
	{	
		return (int)(str - active_qcvm->pStrings);
	}

	// negative str addresses are remapped to string table
	for (strindex = 0; strindex < pVMStr->numStringsInTable; strindex++)
	{
		if (pVMStr->stringTable[strindex] == str)
			return -1 - strindex;
	}

	if (strindex >= pVMStr->stringTableSize)
	{
		// string table is at full capacity, allocate space for more entries
		Scr_CreateStringTable();
	}

	pVMStr->numStringsInTable++;
	pVMStr->stringTable[strindex] = str;

#ifdef _DEBUG
	Com_Printf("New string '%s' (%i strings, strtable for %i)\n", str, pVMStr->numStringsInTable, pVMStr->stringTableSize);
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
	CheckScriptVM(__FUNCTION__);
	pVMStr = &active_qcvm->strTable;

	if (num >= 0 && num < Scr_GetProgStringsSize())
	{
		return active_qcvm->pStrings + num;
	}
	else if (num < 0 && num >= -pVMStr->numStringsInTable)
	{
		if (!pVMStr->stringTable[-1 - num])
		{
			Com_Error(ERR_FATAL, "%s: non-existant string %d in strings table.\n", __FUNCTION__, num);
			return "";
		}
		return pVMStr->stringTable[-1 - num];
	}
	else
	{
		Com_Error(ERR_FATAL, "%s: invalid string offset %d.\n", __FUNCTION__, num);
		return "";
	}
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

	strbuf = pVMStr->tempStrings[current_tempstr];
	memset(strbuf, 0, sizeof(pVMStr->tempStrings[0]));
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

	CheckScriptVM(__FUNCTION__);
	pVMStr = &active_qcvm->strTable;

	str = Scr_GrabTempStringBuffer();

	if (first >= Scr_NumArgs() || first < 0)
		return str;

	for (i = first; i < Scr_NumArgs(); i++)
	{
		//strcat(str[varstring], Scr_GetParmString(i));
		len = Q_strlcat(str, Scr_GetParmString(i), sizeof(pVMStr->tempStrings[0]));
		if (len >= sizeof(pVMStr->tempStrings[0]))
		{
			Com_Printf("%s: string truncated.\n", __FUNCTION__);
			return str;
		}
		
	}
	return str;
}

/*
============
Scr_AllocString
Allocates a new string and puts it in string table.
============
*/
scr_string_t Scr_AllocString(int size, char** ptr)
{
	scr_string_t strindex;

	if (!size)
		return 0;

	CheckScriptVM(__FUNCTION__);
	pVMStr = &active_qcvm->strTable;

	for (strindex = 0; strindex < pVMStr->numStringsInTable; strindex++)
	{
		if (!pVMStr->stringTable[strindex])
			break;
	}

	if (strindex >= pVMStr->stringTableSize)
	{
		Scr_CreateStringTable();
	}

	pVMStr->stringTable[strindex] = (char*)Z_TagMalloc(size, (TAG_QCVM_MEMORY + active_qcvm->progsType));
	pVMStr->numStringsInTable++;

	if (ptr)
	{
		*ptr = (char*)pVMStr->stringTable[strindex];
	}

	return -1 - strindex;
}


/*
============
Scr_NewString
Creates new constant string in active qcvm.
============
*/
scr_string_t Scr_NewString(const char* string)
{
	char* new_p;
	int		i, l;
	scr_string_t num;

	l = (int)strlen(string) + 1;
	num = Scr_AllocString(l, &new_p);

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

	return num;
}