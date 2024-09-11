/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#ifndef _PRAGMA_PAK_H_
#define _PRAGMA_PAK_H_

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')

#define MAX_FILENAME_IN_PACK 56
#define	MAX_FILES_IN_PACK	4096

typedef struct
{
	char	name[MAX_FILENAME_IN_PACK];
	int32_t		filepos, filelen;
} dpackfile_t;

typedef struct
{
	int32_t		ident;		// == IDPAKHEADER
	int32_t		dirofs;
	int32_t		dirlen;
} dpackheader_t;




#endif /*_PRAGMA_PAK_H_*/