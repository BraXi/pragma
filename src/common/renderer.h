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
// ~~ ANY CHANGE HERE REQUIRES RECOMPILING KERNEL AND RENDERER ~~
//

#pragma once
#ifndef _PRAGMA_RENDERER_H_
#define _PRAGMA_RENDERER_H_

#include "../engine/pragma.h"

#define	MAX_DLIGHTS				32		// was 32
#define	MAX_VISIBLE_ENTITIES	1024	// max entities a renderer can process, was 128 [previously MAX_ENTITIES]
#define	MAX_PARTICLES			4096	// can be much higher with the new renderer
#define	MAX_LIGHTSTYLES			256
#define MAX_DEBUG_PRIMITIVES	4096


//
// RENDERER ENTITY
//
typedef struct rentity_s
{
	// model opaque type outside refresh
	struct model_s		*model;			

	//
	// most recent data
	//
	vec3_t				angles;
	vec3_t				origin;		// also used as RF_BEAM's "from"
	int					frame;			// also used as RF_BEAM's diameter

	//
	// previous data for lerping
	//
	vec3_t				oldorigin;	// also used as RF_BEAM's "to"
	int					oldframe;


	//
	// misc
	// 
	int		index;
	float	shadelightpoint[3]; // value from R_LightPoint without any alterations (no applied efx, etc)

	// used to lerp animation frames in new anim system
	// 0.0 = current, 1.0 = old
	float	animbacklerp;			

	// used to lerp origin and between old & current origin
	// 0.0 = current, 1.0 = old
	float	backlerp;				

	// hidden = (hiddenPartsBits & (1 << modelSurfaceNum)
	byte	hiddenPartsBits;

	// index to skin entry
	int		skinnum;				
	
	// index to lightstyles for flashing entities
	int		lightstyle;	

	// transparency, ignored when renderfx RF_TRANSLUCENT isn't set
	float	alpha;

	// scale, ignored when renderfx RF_SCALE isn't set, MD3 only
	float	scale;	

	// base texture color is multiplied by this, ignored when renderfx RF_COLOR isn't set, MD3 only
	vec3_t	renderColor;
	

	struct image_s	*skin;			// NULL for inline skin

	// RF flags
	int		renderfx;


	vec3_t		axis[3];
} rentity_t;

typedef enum
{
	DL_POINTLIGHT,
	DL_SPOTLIGHT,
	DL_VIEW_FLASHLIGHT
} dLightType_t;

typedef struct
{
	dLightType_t	type;

	vec3_t			origin;
	vec3_t			color;
	float			intensity;

	// spot lights
	vec3_t			dir;
	float			cutoff;
} dlight_t;

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	alpha;
	struct image_s* material;
	vec2_t size;
} particle_t;

typedef struct
{
	vec3_t		rgb;			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightstyle_t;


typedef struct
{
	vec3_t origin, angles;
	float alpha;
	struct model_s* model;
} decal_t;

typedef struct rdCamParams_s
{
	float	origin[3];
	float	angles[3];
	float	fov_x, fov_y;
	int		flags;			// RDF_UNDERWATER, etc

	float	flashlight_angles[3];
	rdViewFX_t fx;
}rdCamParams_t;

typedef struct
{
	int				x, y, width, height;// in virtual screen coordinates

	float			time;				// time is used to auto animate
	
	rdCamParams_t	view;

	byte			*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;		// [MAX_LIGHTSTYLES]

	int				num_entities;
	rentity_t		*entities;			// [MAX_VISIBLE_ENTITIES]

	int				num_dlights;
	dlight_t		*dlights;			// [MAX_DLIGHTS]

	int				num_particles;
	particle_t		*particles;			// [MAX_PARTICLES]

	int				num_debugprimitives;
	debugprimitive_t	*debugprimitives; // [MAX_DEBUG_PRIMITIVES]
} refdef_t;


#define	API_VERSION		8

//
// these are the functions exported by the refresh module
//
typedef struct
{
	// if api_version is different, the dll cannot be used
	int		api_version;

	int		rentity_size;
	int		dlight_size;
	int		particle_size;
	int		lightstyle_size;
	int		decal_size;
	int		refdef_size;

	// called when the library is loaded
	qboolean	(*Init) ( void *hinstance, void *wndproc );

	// called before the library is unloaded
	void	(*Shutdown) (void);

	//
	// video mode and refresh state management entry points
	//

	void	(*BeginFrame)(float camera_separation);

	void	(*EndFrame) (void);

	void	(*RenderFrame) (refdef_t* fd, qboolean onlyortho);

	// when window focus changes
	void	(*AppActivate)(qboolean activate);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any model_s or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors, and pics have
	// an implicit "pics/" prepended to the name. (a pic name that starts with a
	// slash will not use the "pics/" prefix or the ".tga" postfix)
	void	(*BeginRegistration)(const char *map);

	void	(*EndRegistration)();

	struct model_s *(*RegisterModel) (const char *name);

	struct image_s *(*RegisterSkin) (const char *name);

	struct image_s *(*RegisterPic) (const char *name);

	void	(*SetSky) (const char *name, float rotate, vec3_t axis, vec3_t color);

	// will return 0 0 if not found
	void	(*GetImageSize) (int *w, int *h, const char *name);	

	void	(*DrawImage) (int x, int y, const char *name);

	void	(*DrawStretchImage) (int x, int y, int w, int h, const char *name);

	void	(*DrawTileClear) (int x, int y, int w, int h, const char *name);

	void	(*DrawFill) (int x, int y, int w, int h);

	int		(*FindFont)(const char* name);
	int		(*GetFontHeight)(int fontId);
	int		(*GetTextWidth)(int fontId, const char* text);
	void	(*NewDrawString)(int x, int y, int alignX, int fontId, float scale, vec4_t color, const char* text);

	void	(*DrawString)(float x, float y, int alignx, int charSize, int fontId, vec4_t color, const char* str);

	void	(*_DrawString)(const char* string, float x, float y, float fontSize, int alignx, rgba_t color); //deprecated
	void	(*DrawStretchedImage)(rect_t rect, rgba_t color, const char* pic);
	void	(*NewDrawFill) (rect_t rect, rgba_t color);

	void	(*SetColor)(float r, float g, float b, float a);

	int		(*TagIndexForName)(struct model_s* model, const char* tagName);
	qboolean (*LerpTag)(orientation_t* tag, struct model_s* model, int startFrame, int endFrame, float frac, int tagIndex);
} refexport_t;

//
// these are the functions imported by the rendering module
//
typedef struct
{
	void	(*Printf) (int print_level, char* str, ...);
	void	(*Error) (int err_level, char *str, ...);
	
	void	*(*MemAlloc)(int size);
	void	(*MemFree)(void *ptr);

	void	(*AddCommand) (const char *name, void(*cmd)(void));
	void	(*RemoveCommand) (const char *name);
	int		(*Cmd_Argc) (void);
	char	*(*Cmd_Argv) (int i);
	void	(*Cmd_ExecuteText) (int exec_when, char *text);

	// files will be memory mapped read only the returned buffer may be part of a larger pak file,
	// or a discrete file from anywhere in the quake search path  a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int		(*LoadFile) (const char *name, void **buf);
	int		(*LoadTextFile)(const char* filename, char** buffer);
	void	(*FreeFile) (void *buf);
	char	*(*GetGameDir) (void);

	cvar_t	*(*Cvar_Get) (const char *name, const char *value, int flags, char *desc);
	cvar_t	*(*Cvar_Set)(const char *name, const char *value );
	void	 (*Cvar_SetValue)(const char *name, float value );

	qboolean	(*Vid_GetModeInfo)( int *width, int *height, int mode );
	void		(*Vid_NewWindow)( int width, int height );

	void	*(*Glob_HunkBegin)(const int maxsize, const char* name);
	void	*(*Glob_HunkAlloc)(int size);
	int		(*Glob_HunkEnd)(void);
	void	(*Glob_HunkFree)(void* base);

	unsigned int (*GetBSPLimit)(bspDataType type, qboolean extendedbsp);
	unsigned int (*GetBSPElementSize)(bspDataType type, qboolean extendedbsp);
} refimport_t;


// this is the only function actually exported at the linker level
typedef	refexport_t	(*GetRefAPI_t) (refimport_t);

#endif /*_PRAGMA_RENDERER_H_*/