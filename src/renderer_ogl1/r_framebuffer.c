/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

unsigned int fbo, depth_rb, fbo_tex_diffuse;

extern cvar_t* r_postfx_blur;
extern cvar_t* r_postfx_contrast;
extern cvar_t* r_postfx_grayscale;
extern cvar_t* r_postfx_inverse;
extern cvar_t* r_postfx_noise;

/*
===============
R_ClearFBO

Enabling will clear fbo and start rendering to texture.
===============
*/
void R_ClearFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/*
===============
R_RenderToFBO

Enabling will clear fbo and start rendering to texture.
===============
*/
void R_RenderToFBO(qboolean enable)
{
	if (enable)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		//glPushAttrib(GL_VIEWPORT_BIT);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, vid.width, vid.height);
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		//glPopAttrib();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

/*
===============
InitFrameBufferDepthBuffer

Create depth render target
===============
*/
static void InitFrameBufferDepthBuffer()
{
	glGenRenderbuffers(1, &depth_rb);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, vid.width, vid.height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

//	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, vid.width, vid.height);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

/*
===============
InitFrameBufferDiffuseTexture

Create diffuse texture for rt
===============
*/
static void InitFrameBufferDiffuseTexture()
{
	glGenTextures(1, &fbo_tex_diffuse);
	glBindTexture(GL_TEXTURE_2D, fbo_tex_diffuse);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);
}


/*
===============
R_InitFrameBuffer

Create frame buffer and render target for rendering
===============
*/
qboolean R_InitFrameBuffer()
{
	InitFrameBufferDiffuseTexture();
	InitFrameBufferDepthBuffer();

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex_diffuse, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		ri.Error(ERR_FATAL, "Failed to create frame buffer object\n");
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

/*
===============
R_FreeFrameBuffer
===============
*/
void R_FreeFrameBuffer()
{
	glDeleteTextures(1, &fbo_tex_diffuse);
//	glDeleteTextures(1, &fbo_tex_depth);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &depth_rb);
}

/*
===============
R_ApplyPostEffects

===============
*/
static void R_ApplyPostEffects()
{
	if (r_newrefdef.view.fx.intensity > 10.0f)
		r_newrefdef.view.fx.intensity = 10.0f;

	if (r_newrefdef.view.fx.gamma > 5.0f)
		r_newrefdef.view.fx.gamma = 5.0f;

	if (r_newrefdef.view.fx.blur > 7.0f)
		r_newrefdef.view.fx.blur = 7.0f;

	if (r_newrefdef.view.fx.grayscale > 1.0f)
		r_newrefdef.view.fx.grayscale = 1.0f;

	if (r_newrefdef.view.fx.inverse > 1.0f)
		r_newrefdef.view.fx.inverse = 1.0f;

	if (r_newrefdef.view.fx.noise > 1.0f)
		r_newrefdef.view.fx.noise = 1.0f;

	if (r_newrefdef.view.fx.contrast > 5.0f)
		r_newrefdef.view.fx.contrast = 5.0f;

	// note to self: I should probably send these in vec4 uniforms or even matrix to save on uniforms and calls
	R_ProgUniform1f(LOC_INTENSITY, r_newrefdef.view.fx.intensity > 0.0f ? r_newrefdef.view.fx.intensity : r_intensity->value);
	R_ProgUniform1f(LOC_GAMMA, r_newrefdef.view.fx.gamma > 0.0f ? r_newrefdef.view.fx.gamma : r_gamma->value);
	R_ProgUniform1f(LOC_BLUR, r_newrefdef.view.fx.blur > 0.0f ? r_newrefdef.view.fx.blur : r_postfx_blur->value);
	R_ProgUniform1f(LOC_CONTRAST, r_newrefdef.view.fx.contrast > 0.0f ? r_newrefdef.view.fx.contrast : r_postfx_contrast->value);
	R_ProgUniform1f(LOC_GRAYSCALE, r_newrefdef.view.fx.grayscale > 0.0f ? r_newrefdef.view.fx.grayscale : r_postfx_grayscale->value);
	R_ProgUniform1f(LOC_INVERSE, r_newrefdef.view.fx.inverse == 1.0f ? 1.0f :  r_postfx_inverse->value);
	R_ProgUniform1f(LOC_NOISE, r_newrefdef.view.fx.noise > 0.0f ? r_newrefdef.view.fx.noise :  r_postfx_noise->value);
}

/*
===============
R_DrawFBO

Draw contents of FBO on screen
===============
*/
void R_DrawFBO(int x, int y, int w, int h, qboolean diffuse)
{
	rect_t rect;
	rect[0] = x;
	rect[1] = y;
	rect[2] = w;
	rect[3] = h;

	R_SetColor4(1, 1, 1, 1);

	R_BindProgram(GLPROG_POSTFX);
	R_ProgUniform2f(LOC_SCREENSIZE, (float)vid.width, (float)vid.height);

	R_ApplyPostEffects();

	if(diffuse)
		glBindTexture(GL_TEXTURE_2D, fbo_tex_diffuse);
//	else
//		glBindTexture(GL_TEXTURE_2D, fbo_tex_depth);

	glBegin(GL_TRIANGLES);
	{
		glTexCoord2f(0.0f, 1.0f);	glVertex2f(rect[0], rect[1]);
		glTexCoord2f(1.0f, 1.0f);	glVertex2f(rect[0] + rect[2], rect[1]);
		glTexCoord2f(1.0f, 0.0f);	glVertex2f(rect[0] + rect[2], rect[1] + rect[3]);

		glTexCoord2f(0.0f, 1.0f);	glVertex2f(rect[0], rect[1]);
		glTexCoord2f(1.0f, 0.0f);	glVertex2f(rect[0] + rect[2], rect[1] + rect[3]);
		glTexCoord2f(0.0f, 0.0f);	glVertex2f(rect[0], rect[1] + rect[3]);
	}
	glEnd();

	R_UnbindProgram();
}