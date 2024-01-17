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
// r_main.c
#include "r_local.h"

refimport_t	ri;


cvar_t* r_norefresh;
cvar_t* r_drawentities;
cvar_t* r_drawworld;
cvar_t* r_speeds;
cvar_t* r_fullbright;
cvar_t* r_novis;
cvar_t* r_nocull;
cvar_t* r_lerpmodels;
cvar_t* r_lefthand;

cvar_t* r_overbrightbits;

cvar_t* r_particle_min_size;
cvar_t* r_particle_max_size;
cvar_t* r_particle_size;
cvar_t* r_particle_att_a;
cvar_t* r_particle_att_b;
cvar_t* r_particle_att_c;

cvar_t* gl_ext_swapinterval;
cvar_t* gl_ext_pointparameters;

cvar_t* r_log;
cvar_t* r_bitdepth;
cvar_t* r_drawbuffer;
cvar_t* gl_driver;
cvar_t* r_lightmap;
cvar_t* r_mode;
cvar_t* r_dynamic;
cvar_t* r_monolightmap;
cvar_t* r_modulate;
cvar_t* r_nobind;
cvar_t* r_round_down;
cvar_t* r_picmip;
cvar_t* r_skymip;
cvar_t* r_showtris;
cvar_t* r_ztrick;
cvar_t* r_finish;
cvar_t* r_clear;
cvar_t* r_cull;
cvar_t* r_saturatelighting;
cvar_t* r_swapinterval;
cvar_t* r_texturemode;
cvar_t* r_texturealphamode;
cvar_t* r_texturesolidmode;
cvar_t* r_lockpvs;

cvar_t* r_fullscreen;
cvar_t* r_gamma;
cvar_t* r_renderer;

float sinTable[FUNCTABLE_SIZE];

void GL_Strings_f(void);

/*
==================
R_RegisterCvarsAndCommands

register all cvars and commands
==================
*/
void R_RegisterCvarsAndCommands(void)
{
	r_lefthand = ri.Cvar_Get("cl_hand", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);

	r_fullbright = ri.Cvar_Get("r_fullbright", "0", CVAR_CHEAT);

	r_drawentities = ri.Cvar_Get("r_drawentities", "1", CVAR_CHEAT);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", CVAR_CHEAT);

	r_novis = ri.Cvar_Get("r_novis", "0", CVAR_CHEAT);
	r_nocull = ri.Cvar_Get("r_nocull", "0", CVAR_CHEAT);

	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", CVAR_CHEAT);

	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	r_particle_min_size = ri.Cvar_Get("r_particle_min_size", "2", CVAR_ARCHIVE | CVAR_CHEAT);
	r_particle_max_size = ri.Cvar_Get("r_particle_max_size", "40", CVAR_ARCHIVE | CVAR_CHEAT);
	r_particle_size = ri.Cvar_Get("r_particle_size", "40", CVAR_ARCHIVE | CVAR_CHEAT);
	r_particle_att_a = ri.Cvar_Get("r_particle_att_a", "0.01", CVAR_ARCHIVE | CVAR_CHEAT);
	r_particle_att_b = ri.Cvar_Get("r_particle_att_b", "0.0", CVAR_ARCHIVE | CVAR_CHEAT);
	r_particle_att_c = ri.Cvar_Get("r_particle_att_c", "0.01", CVAR_ARCHIVE | CVAR_CHEAT);

	r_modulate = ri.Cvar_Get("r_modulate", "1", CVAR_ARCHIVE | CVAR_CHEAT);
	r_log = ri.Cvar_Get("r_log", "0", 0);
	r_bitdepth = ri.Cvar_Get("r_bitdepth", "0", 0);
	r_mode = ri.Cvar_Get("r_mode", "3", CVAR_ARCHIVE);
	r_lightmap = ri.Cvar_Get("r_lightmap", "0",CVAR_CHEAT);
	r_dynamic = ri.Cvar_Get("r_dynamic", "1", CVAR_CHEAT);
	r_nobind = ri.Cvar_Get("r_nobind", "0", CVAR_CHEAT);
	r_round_down = ri.Cvar_Get("r_round_down", "0", 0);
	r_picmip = ri.Cvar_Get("r_picmip", "0", 0);
	r_skymip = ri.Cvar_Get("r_skymip", "0", 0);
	r_showtris = ri.Cvar_Get("r_showtris", "0", CVAR_CHEAT);
	r_ztrick = ri.Cvar_Get("r_ztrick", "0", 0);
	r_finish = ri.Cvar_Get("r_finish", "0", CVAR_ARCHIVE);
	r_clear = ri.Cvar_Get("r_clear", "0", 0);
	r_cull = ri.Cvar_Get("r_cull", "1", CVAR_CHEAT);
	r_monolightmap = ri.Cvar_Get("r_monolightmap", "0", CVAR_CHEAT);
	gl_driver = ri.Cvar_Get("gl_driver", "opengl32", CVAR_ARCHIVE);
	r_texturemode = ri.Cvar_Get("r_texturemode", "GL_NEAREST_MIPMAP_NEAREST", CVAR_ARCHIVE);
	r_texturealphamode = ri.Cvar_Get("r_texturealphamode", "default", CVAR_ARCHIVE);
	r_texturesolidmode = ri.Cvar_Get("r_texturesolidmode", "default", CVAR_ARCHIVE);
	r_lockpvs = ri.Cvar_Get("r_lockpvs", "0", CVAR_CHEAT);

	gl_ext_swapinterval = ri.Cvar_Get("gl_ext_swapinterval", "1", CVAR_ARCHIVE);
	gl_ext_pointparameters = ri.Cvar_Get("gl_ext_pointparameters", "1", CVAR_ARCHIVE);

	r_drawbuffer = ri.Cvar_Get("r_drawbuffer", "GL_BACK", CVAR_CHEAT);
	r_swapinterval = ri.Cvar_Get("r_swapinterval", "1", CVAR_ARCHIVE);

	// --- begin yquake2 ---
	r_overbrightbits = ri.Cvar_Get("r_overbrightbits", "1", CVAR_ARCHIVE | CVAR_CHEAT);
	// --- end yquake2 ---

	r_saturatelighting = ri.Cvar_Get("r_saturatelighting", "0", CVAR_CHEAT);

	r_fullscreen = ri.Cvar_Get("r_fullscreen", "0", CVAR_ARCHIVE);
	r_gamma = ri.Cvar_Get("r_gamma", "1.0", CVAR_ARCHIVE);

	r_renderer = ri.Cvar_Get("r_renderer", DEFAULT_RENDERER, CVAR_ARCHIVE);

	ri.Cmd_AddCommand("imagelist", GL_ImageList_f);
	ri.Cmd_AddCommand("screenshot", GL_ScreenShot_f);
	ri.Cmd_AddCommand("modellist", Mod_Modellist_f);
	ri.Cmd_AddCommand("gl_strings", GL_Strings_f);
}

/*
==================
R_SetMode

Sets window width & height as well as fullscreen
==================
*/
qboolean R_SetMode(void)
{
	rserr_t err;
	qboolean fullscreen;

// braxi -- maybe reuse later for smth?
//	if (r_fullscreen->modified)
//	{
//		ri.Con_Printf(PRINT_ALL, "R_SetMode() - CDS not allowed with this driver\n");
//		ri.Cvar_SetValue("r_fullscreen", !r_fullscreen->value);
//		r_fullscreen->modified = false;
//	}

	fullscreen = r_fullscreen->value;

	r_fullscreen->modified = false;
	r_mode->modified = false;

	if ((err = GLimp_SetMode(&vid.width, &vid.height, r_mode->value, fullscreen)) == rserr_ok)
	{
		gl_state.prev_mode = r_mode->value;
	}
	else
	{
		if (err == rserr_invalid_fullscreen)
		{
			ri.Cvar_SetValue("r_fullscreen", 0);
			r_fullscreen->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - fullscreen unavailable in this mode\n");
			if ((err = GLimp_SetMode(&vid.width, &vid.height, r_mode->value, false)) == rserr_ok)
				return true;
		}
		else if (err == rserr_invalid_mode)
		{
			ri.Cvar_SetValue("gl_mode", gl_state.prev_mode);
			r_mode->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode(&vid.width, &vid.height, gl_state.prev_mode, false)) != rserr_ok)
		{
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}
	return true;
}


/*
===============
R_Init
===============
*/
int R_Init(void* hinstance, void* hWnd)
{
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int		err;
	int		j;
	extern float r_turbsin[256];

	for (j = 0; j < 256; j++)
	{
		r_turbsin[j] *= 0.5;
	}

	ri.Con_Printf(PRINT_ALL, "OpenGL 1.4 renderer version : "REF_VERSION"\n");

	R_RegisterCvarsAndCommands();

	// initialize our QGL dynamic bindings
	if (!QGL_Init(gl_driver->string))
	{
		QGL_Shutdown();
		ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - could not load \"%s\"\n", gl_driver->string);
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if (!GLimp_Init(hinstance, hWnd))
	{
		QGL_Shutdown();
		return -1;
	}

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if (!R_SetMode())
	{
		QGL_Shutdown();
		ri.Con_Printf(PRINT_ALL, "ref_gl::R_Init() - could not R_SetMode()\n");
		return -1;
	}

	ri.Vid_MenuInit();

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = qglGetString(GL_VENDOR);
	ri.Con_Printf(PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string);
	gl_config.renderer_string = qglGetString(GL_RENDERER);
	ri.Con_Printf(PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string);
	gl_config.version_string = qglGetString(GL_VERSION);
	ri.Con_Printf(PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string);
	gl_config.extensions_string = qglGetString(GL_EXTENSIONS);
	ri.Con_Printf(PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string);

	strcpy(renderer_buffer, gl_config.renderer_string);
	_strlwr(renderer_buffer);

	strcpy(vendor_buffer, gl_config.vendor_string);
	_strlwr(vendor_buffer);


	if (strstr(renderer_buffer, "nvidia"))
		gl_config.renderer = GL_RENDERER_NVIDIA;
	else if (strstr(renderer_buffer, "amd"))
		gl_config.renderer = GL_RENDERER_AMD;
	else if (strstr(renderer_buffer, "intel"))
		gl_config.renderer = GL_RENDERER_INTEL;
	else
		gl_config.renderer = GL_RENDERER_OTHER;


	if (toupper(r_monolightmap->string[1]) != 'F')
	{
		ri.Cvar_Set("r_monolightmap", "0");

	}

	ri.Cvar_Set("scr_drawall", "1");

	/*
	** grab extensions
	*/
#ifdef WIN32
	if (strstr(gl_config.extensions_string, "GL_EXT_compiled_vertex_array"))
	{
//		ri.Con_Printf(PRINT_ALL, "...enabling GL_EXT_compiled_vertex_array\n");
		qglLockArraysEXT = (void*)qwglGetProcAddress("glLockArraysEXT");
		qglUnlockArraysEXT = (void*)qwglGetProcAddress("glUnlockArraysEXT");
	}
	else
	{
		ri.Con_Printf(PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n");
	}

	if (strstr(gl_config.extensions_string, "WGL_EXT_swap_control"))
	{
		qwglSwapIntervalEXT = (BOOL(WINAPI*)(int)) qwglGetProcAddress("wglSwapIntervalEXT");
		ri.Con_Printf(PRINT_ALL, "...enabling WGL_EXT_swap_control\n");
	}
	else
	{
		ri.Con_Printf(PRINT_ALL, "...WGL_EXT_swap_control not found\n");
	}

	if (strstr(gl_config.extensions_string, "GL_EXT_point_parameters"))
	{
		if (gl_ext_pointparameters->value)
		{
			qglPointParameterfEXT = (void (APIENTRY*)(GLenum, GLfloat)) qwglGetProcAddress("glPointParameterfEXT");
			qglPointParameterfvEXT = (void (APIENTRY*)(GLenum, const GLfloat*)) qwglGetProcAddress("glPointParameterfvEXT");
			ri.Con_Printf(PRINT_ALL, "...using GL_EXT_point_parameters\n");
		}
		else
		{
			ri.Con_Printf(PRINT_ALL, "...ignoring GL_EXT_point_parameters\n");
		}
	}
	else
	{
		ri.Con_Printf(PRINT_ALL, "...GL_EXT_point_parameters not found\n");
	}


	// GL_ARB_multitexture is mandatory
	if (strstr(gl_config.extensions_string, "GL_ARB_multitexture"))
	{
		qglActiveTextureARB = (void*)qwglGetProcAddress("glActiveTextureARB");
		qglMultiTexCoord2fARB = (void*)qwglGetProcAddress("glMultiTexCoord2fARB");
	}
	else
	{
		ri.Sys_Error(ERR_FATAL, "GL_ARB_multitexture not found\n");
	}
#endif
	ri.Con_Printf(PRINT_ALL, "--- GL_ARB_multitexture forced off ---\n");
	qglActiveTextureARB = 0;
	qglMultiTexCoord2fARB = 0;
	GL_SetDefaultState();
	R_InitialOGLState(); //wip

	/*
	** draw our stereo patterns
	*/
#if 0
	GL_DrawStereoPattern();
#endif

	for (int i = 0; i < FUNCTABLE_SIZE; i++)
		sinTable[i] = sin(DEG2RAD(i * 360.0f / ((float)(FUNCTABLE_SIZE - 1))));

	GL_InitImages();
	Mod_Init();
	R_InitParticleTexture();
	Draw_InitLocal();

	err = qglGetError();
	if (err != GL_NO_ERROR)
		ri.Con_Printf(PRINT_ALL, "glGetError() = 0x%x\n", err);
	return 1;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown(void)
{
	ri.Cmd_RemoveCommand("modellist");
	ri.Cmd_RemoveCommand("screenshot");
	ri.Cmd_RemoveCommand("imagelist");
	ri.Cmd_RemoveCommand("gl_strings");

	Mod_FreeAll();

	GL_ShutdownImages();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();

	/*
	** shutdown our QGL subsystem
	*/
	QGL_Shutdown();
}


//===================================================================

void R_BeginRegistration(char* map);
struct model_s* R_RegisterModel(char* name);
struct image_s* R_RegisterSkin(char* name);
void R_SetSky(char* name, float rotate, vec3_t axis, vec3_t color);
void	R_EndRegistration(void);

void	R_RenderFrame(refdef_t* fd);

struct image_s* R_RegisterPic(char* name);

void	Draw_Pic(int x, int y, char* name);
void	Draw_Char(int x, int y, int c);
void	Draw_TileClear(int x, int y, int w, int h, char* name);
void	Draw_Fill(int x, int y, int w, int h);
void	Draw_FadeScreen(float* rgba);

void	R_DrawString(char* string, float x, float y, float fontSize, int alignx, rgba_t color);
void	R_DrawStretchedImage(rect_t rect, rgba_t color, char* pic);
void	R_DrawFill(rect_t rect, rgba_t color);

static void	RR_SetColor(float r, float g, float b, float a)
{
	qglColor4f(r,g,b,a);
}

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI(refimport_t rimp)
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.EndRegistration = R_EndRegistration;

	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = R_RegisterPic;

	re.SetSky = R_SetSky;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen = Draw_FadeScreen;
	re.DrawStretchRaw = Draw_StretchRaw;
	re.SetColor = RR_SetColor;

	// braxi -- newer replacements
	re.DrawString = R_DrawString;
	re.DrawStretchedImage = R_DrawStretchedImage;
	re.NewDrawFill = R_DrawFill;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.BeginFrame = R_BeginFrame;
	re.EndFrame = GLimp_EndFrame;

	re.AppActivate = GLimp_AppActivate;

	Swap_Init();

	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in shared.c and win_shared.c can link
void Sys_Error(char* error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	ri.Sys_Error(ERR_FATAL, "%s", text);
}

void Com_Printf(char* fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start(argptr, fmt);
	vsprintf(text, fmt, argptr);
	va_end(argptr);

	ri.Con_Printf(PRINT_ALL, "%s", text);
}

#endif
