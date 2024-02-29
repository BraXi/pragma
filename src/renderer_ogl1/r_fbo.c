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
extern cvar_t* r_postfx_grayscale;
extern cvar_t* r_postfx_inverse;
extern cvar_t* r_postfx_noise;

void R_RenderToFBO(qboolean enable)
{
	if (enable)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		//glPushAttrib(GL_VIEWPORT_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, vid.width, vid.height);
		glEnable(GL_DEPTH_TEST);
	}
	else
	{
		//glPopAttrib();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

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

void R_FreeFrameBuffer()
{
	glDeleteTextures(1, &fbo_tex_diffuse);
//	glDeleteTextures(1, &fbo_tex_depth);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &depth_rb);
}


void R_DrawFBO(int x, int y, int w, int h, qboolean diffuse)
{
	rect_t rect;
	rect[0] = x;
	rect[1] = y;
	rect[2] = w;
	rect[3] = h;

	glColor4f(1, 1, 1, 1);

	R_BindProgram(GLPROG_POSTFX);

	R_ProgUniform2f(pCurrentProgram->locs[LOC_SCREENSIZE], (float)vid.width, (float)vid.height);

	R_ProgUniform1f(pCurrentProgram->locs[LOC_INTENSITY], r_intensity->value);
	R_ProgUniform1f(pCurrentProgram->locs[LOC_GAMMA], r_gamma->value);

	R_ProgUniform1f(pCurrentProgram->locs[LOC_BLUR], r_postfx_blur->value);
	R_ProgUniform1f(pCurrentProgram->locs[LOC_GRAYSCALE], r_postfx_grayscale->value);
	R_ProgUniform1f(pCurrentProgram->locs[LOC_INVERSE], r_postfx_inverse->value);
	//R_ProgUniform1f(pCurrentProgram->locs[LOC_NOISE], r_postfx_noise->value);

	if(diffuse)
		glBindTexture(GL_TEXTURE_2D, fbo_tex_diffuse);
//	else
//		glBindTexture(GL_TEXTURE_2D, fbo_tex_depth);

	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0f, 1.0f);	glVertex2f(rect[0], rect[1]);
		glTexCoord2f(1.0f, 1.0f);	glVertex2f(rect[0] + rect[2], rect[1]);
		glTexCoord2f(1.0f, 0.0f);	glVertex2f(rect[0] + rect[2], rect[1] + rect[3]);
		glTexCoord2f(0.0f, 0.0f);	glVertex2f(rect[0], rect[1] + rect[3]);
	}
	glEnd();

	R_UnbindProgram();
}


#if 0
static unsigned int fbo;
unsigned int textureColorbuffer;

void R_InitFBO()
{
#if 1
	glGenFramebuffersEXT(1, &fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);


	// generate texture
	glGenTextures(1, &textureColorbuffer);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	// attach it to currently bound framebuffer object
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif
}


void R_RenderToTexture()
{
	// first pass
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

}

void R_RenderFullScreen()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); // back to default
//	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
//	glClear(GL_COLOR_BUFFER_BIT);

	R_UnbindProgram();
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	glEnable(GL_TEXTURE_2D);
}
#endif