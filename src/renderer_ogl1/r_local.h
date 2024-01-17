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
// disable data conversion warnings

#if 0
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
#endif

#ifdef _WIN32
#  include <windows.h>
#endif

#include <stdio.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>

#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif

#include "../qcommon/renderer.h"

#include "win_qgl.h"

#define	REF_VERSION	"0.3"


// -- sin table --
//#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI )
#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )

#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)
// -- sin table --

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


typedef struct
{
	unsigned		width, height;			// coordinates from main game
} viddef_t;

extern	viddef_t	vid;


/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum 
{
	it_model,
	it_sprite,
	it_tga,
	it_texture,
	it_gui,
	it_sky
} imagetype_t;

typedef struct image_s
{
	char	name[MAX_QPATH];			// game path, including extension
	imagetype_t	type;
	int		width, height;				// source image
	int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;	// for sort-by-texture world drawing
	int		texnum;						// gl texture binding
	float	sl, tl, sh, th;				// 0,0 - 1,1
	qboolean	has_alpha;
} image_t;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	1024

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "r_model.h"

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;

typedef struct
{
	int	brush_polys;
	int alias_tris;

	int	texture_binds;

	int	visible_lightmaps;
	int	visible_textures;

} rperfcounters_t;


#define BACKFACE_EPSILON	0.01


//====================================================

extern	image_t		gltextures[MAX_GLTEXTURES];
extern	int			numgltextures;


extern	image_t		*r_notexture;
extern	image_t		*r_particletexture;
extern	centity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];

extern rperfcounters_t rperf;

extern	int			gl_filter_min, gl_filter_max;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern cvar_t	*gl_ext_swapinterval;
extern cvar_t	*gl_ext_pointparameters;

extern cvar_t	*r_overbrightbits;

extern cvar_t	*r_particle_min_size;
extern cvar_t	*r_particle_max_size;
extern cvar_t	*r_particle_size;
extern cvar_t	*r_particle_att_a;
extern cvar_t	*r_particle_att_b;
extern cvar_t	*r_particle_att_c;

extern	cvar_t	*r_bitdepth;
extern	cvar_t	*r_mode;
extern	cvar_t	*r_log;
extern	cvar_t	*r_lightmap;
extern	cvar_t	*r_dynamic;
extern  cvar_t  *r_monolightmap;
extern	cvar_t	*r_nobind;
extern	cvar_t	*r_round_down;
extern	cvar_t	*r_picmip;
extern	cvar_t	*r_skymip;
extern	cvar_t	*r_showtris;
extern	cvar_t	*r_finish;
extern	cvar_t	*r_ztrick;
extern	cvar_t	*r_clear;
extern	cvar_t	*r_cull;
extern	cvar_t	*r_modulate;
extern	cvar_t	*r_drawbuffer;
extern  cvar_t  *gl_driver;
extern	cvar_t	*r_swapinterval;
extern	cvar_t	*r_texturemode;
extern	cvar_t	*r_texturealphamode;
extern	cvar_t	*r_texturesolidmode;
extern  cvar_t  *r_saturatelighting;
extern  cvar_t  *r_lockpvs;

extern	cvar_t	*r_fullscreen;
extern	cvar_t	*r_gamma;

extern	cvar_t		*intensity;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;
extern	int		gl_tex_solid_format;
extern	int		gl_tex_alpha_format;

extern	float	r_world_matrix[16];

void GL_Bind (int texnum);
void GL_MBind( GLenum target, int texnum );
void GL_TexEnv( GLenum value );
void GL_EnableMultitexture( qboolean enable );
void GL_SelectTexture( GLenum );

void R_LightPoint (vec3_t p, vec3_t color);
void R_PushDlights (void);

//====================================================================

extern	model_t	*r_worldmodel;

extern	int		registration_sequence;

// r_init.c
int 	R_Init( void *hinstance, void *hWnd );
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void R_DrawBrushModel (centity_t *e);
void R_DrawSpriteModel (centity_t *e);
void R_DrawBeam( centity_t *e );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void GL_SubdivideSurface (msurface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (centity_t *e);
void R_MarkLeaves (void);

glpoly_t *WaterWarpPolyVerts (glpoly_t *p);
void EmitWaterPolys (msurface_t *fa);
void R_AddSkySurface (msurface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

#if 0
short LittleShort (short l);
short BigShort (short l);
int	LittleLong (int l);
float LittleFloat (float f);

char	*va(char *format, ...);
// does a varargs printf into a temp buffer
#endif

void COM_StripExtension (char *in, char *out);

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h);
void	Draw_FadeScreen(float* rgba);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );

void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);

struct image_s *R_RegisterSkin (char *name);

image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t	*GL_FindImage (char *name, imagetype_t type);
void	GL_TextureMode( char *string );
void	GL_ImageList_f (void);

void	GL_InitImages (void);
void	GL_ShutdownImages (void);

void	GL_FreeUnusedImages (void);

void GL_TextureAlphaMode( char *string );
void GL_TextureSolidMode( char *string );

/*
** GL extension emulation functions
*/
void GL_DrawParticles( int n, const particle_t particles[] );

/*
** GL config stuff
*/
#define GL_RENDERER_OTHER		0
#define GL_RENDERER_NVIDIA		1
#define GL_RENDERER_AMD			2
#define GL_RENDERER_INTEL		3

typedef struct
{
	int         renderer;
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;
} glconfig_t;

typedef struct
{
	float inverse_intensity;
	qboolean fullscreen;

	int     prev_mode;			// previous r_mode->value

	int lightmap_textures;		// TEXNUM_LIGHTMAPS + NUM_LIGHTMAPS

	int	currenttextures[2];		// currently bound textures [TEX0, TEX1]
	int currenttmu;				// GL_TEXTURE0 or GL_TEXTURE1

	float camera_separation;
	qboolean stereo_enabled;


} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

/*
====================================================================
a better gl state tracker
====================================================================
*/

extern inline void R_AlphaTest(qboolean enable);
extern inline void R_Blend(qboolean enable);
extern inline void R_DepthTest(qboolean enable);
extern  void R_Texturing(qboolean enable);
extern inline void R_CullFace(qboolean enable);

extern inline void R_SetCullFace(GLenum newstate);
extern inline void R_WriteToDepthBuffer(GLboolean newstate);
extern inline void R_BlendFunc(GLenum newstateA, GLenum newstateB);

// inline void R_SetColor(float r, float g, float b, float a);
extern inline void R_SetClearColor(float r, float g, float b, float a);

extern void R_InitialOGLState();

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
int 		GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );
int     	GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen );
void		GLimp_AppActivate( qboolean active );
void		GLimp_EnableLogging( qboolean enable );
void		GLimp_LogNewFrame( void );

