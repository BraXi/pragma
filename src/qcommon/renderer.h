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

//
// THIS FILE IS SHARED BETWEEN RENDERER AND CLIENT (KERNEL)
//

#include "../qcommon/qcommon.h"

#define	MAX_DLIGHTS		64		// was 32
#define	MAX_ENTITIES	512		// max visible entities, was 128 [previously MAX_ENTITIES]
#define	MAX_PARTICLES	8192	// was 4096
#define	MAX_LIGHTSTYLES	256
#define MAX_DEBUG_PRIMITIVES	4096


//
// CLIENT ENTITY
//
typedef struct centity_s
{
	struct model_s		*model;			// opaque type outside refresh
	float				angles[3];

	/*
	** most recent data
	*/
	float				origin[3];		// also used as RF_BEAM's "from"
	int					frame;			// also used as RF_BEAM's diameter

	/*
	** previous data for lerping
	*/
	float				oldorigin[3];	// also used as RF_BEAM's "to"
	int					oldframe;

	/*
	** misc
	*/

	float	animbacklerp;			// 0.0 = current, 1.0 = old
	float	backlerp;				// 0.0 = current, 1.0 = old
	int		skinnum;				// braxi -- WAS used as RF_BEAM's palette index
	vec4_t	color;

	int		lightstyle;				// for flashing entities
	float	alpha;					// ignore if RF_TRANSLUCENT isn't set
	float	scale;					// ignore if RF_SCALE isn't set, MD3 only
	vec3_t	renderColor;
	

	struct image_s	*skin;			// NULL for inline skin
	int		renderfx;

} centity_t;


typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
} dlight_t;

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	alpha;
} particle_t;

typedef struct
{
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightstyle_t;


typedef struct
{
	int				x, y, width, height;// in virtual screen coordinates
	float			fov_x, fov_y;

	float			vieworg[3];
	float			viewangles[3];
	float			blend[4];			// rgba 0-1 full screen blend
	float			time;				// time is uesed to auto animate
	int				rdflags;			// RDF_UNDERWATER, etc

	byte			*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]

	int				num_entities;
	centity_t		*entities;

	int				num_dlights;
	dlight_t		*dlights;

	int				num_particles;
	particle_t		*particles;

	int				num_debugprimitives;
	debugprimitive_t		*debugprimitives;
} refdef_t;


#define	API_VERSION		('B'+'X'+'I'+'3')

//
// these are the functions exported by the refresh module
//
typedef struct
{
	// if api_version is different, the dll cannot be used
	int		api_version;

	// called when the library is loaded
	qboolean	(*Init) ( void *hinstance, void *wndproc );

	// called before the library is unloaded
	void	(*Shutdown) (void);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any model_s or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors, and pics have
	// an implicit "pics/" prepended to the name. (a pic name that starts with a
	// slash will not use the "pics/" prefix or the ".pcx" postfix)
	void	(*BeginRegistration) (char *map);
	void	(*EndRegistration) (void);

	struct model_s *(*RegisterModel) (char *name);
	struct image_s *(*RegisterSkin) (char *name);
	struct image_s *(*RegisterPic) (char *name);

	void	(*SetSky) (char *name, float rotate, vec3_t axis, vec3_t color);


	void	(*RenderFrame) (refdef_t *fd);

	void	(*DrawGetPicSize) (int *w, int *h, char *name);	// will return 0 0 if not found
	void	(*DrawPic) (int x, int y, char *name);
	void	(*DrawStretchPic) (int x, int y, int w, int h, char *name);
	void	(*DrawChar) (int x, int y, int c);
	void	(*Draw_Char2)(float x, float y, float w, float h, int num, rgba_t color);
	void	(*DrawTileClear) (int x, int y, int w, int h, char *name);
	void	(*DrawFill) (int x, int y, int w, int h);
	void	(*DrawFadeScreen) (float *rgba);

	// Draw images for cinematic rendering (which can have a different palette)
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, byte *data);

	void	(*DrawString)(char* string, float x, float y, float fontSize, int alignx, rgba_t color);
	void	(*DrawStretchedImage)(rect_t rect, rgba_t color, char* pic);
	void	(*NewDrawFill) (rect_t rect, rgba_t color);

	void	(*SetColor)(float r, float g, float b, float a);

	/*
	** video mode and refresh state management entry points
	*/
	void	(*BeginFrame)( float camera_separation );
	void	(*EndFrame) (void);

	void	(*AppActivate)( qboolean activate );

} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct
{
	void	(*Sys_Error) (int err_level, char *str, ...);

	void	(*Cmd_AddCommand) (char *name, void(*cmd)(void));
	void	(*Cmd_RemoveCommand) (char *name);
	int		(*Cmd_Argc) (void);
	char	*(*Cmd_Argv) (int i);
	void	(*Cmd_ExecuteText) (int exec_when, char *text);

	void	(*Con_Printf) (int print_level, char *str, ...);

	// files will be memory mapped read only
	// the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path
	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int		(*FS_LoadFile) (char *name, void **buf);
	void	(*FS_FreeFile) (void *buf);

	// gamedir will be the current directory that generated
	// files should be stored to, ie: "f:\quake\id1"
	char	*(*FS_Gamedir) (void);

	cvar_t	*(*Cvar_Get) (char *name, char *value, int flags);
	cvar_t	*(*Cvar_Set)( char *name, char *value );
	void	 (*Cvar_SetValue)( char *name, float value );

	qboolean	(*Vid_GetModeInfo)( int *width, int *height, int mode );
	void		(*Vid_MenuInit)( void );
	void		(*Vid_NewWindow)( int width, int height );
} refimport_t;


// this is the only function actually exported at the linker level
typedef	refexport_t	(*GetRefAPI_t) (refimport_t);
