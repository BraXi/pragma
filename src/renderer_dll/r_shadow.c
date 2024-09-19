/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

vec3_t shadow_origin, shadow_angles;

void R_SetupProjection(unsigned int width, unsigned int height, float fov_y, float znear, float zfar, vec3_t origin, vec3_t angles);

static unsigned int shadowMapSize[2] = { 0,0 };

static GLuint r_shadow_fbo = 0;
GLuint r_texture_shadow_id = 0;

void R_DestroyShadowMapFBO()
{
    if (r_shadow_fbo != 0) 
    {
        glDeleteFramebuffers(1, &r_shadow_fbo);
        r_shadow_fbo = 0;
    }

    if (r_texture_shadow_id != 0) 
    {
        glDeleteTextures(1, &r_texture_shadow_id);
        r_texture_shadow_id = 0;
    }
}

qboolean R_InitShadowMap(unsigned int width, unsigned int height)
{
    if (shadowMapSize[0] != width || shadowMapSize[1] != height)
    {
        // force a rebuild when screen dimensions change
        R_DestroyShadowMapFBO();
    }
    else if (r_shadow_fbo)
    {
        return true;
    }

    shadowMapSize[0] = width;
    shadowMapSize[1] = height;

    glGenFramebuffers(1, &r_shadow_fbo);

    glGenTextures(1, &r_texture_shadow_id);
    glBindTexture(GL_TEXTURE_2D, r_texture_shadow_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize[0], shadowMapSize[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_shadow_fbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_texture_shadow_id, 0);

    glDrawBuffer(GL_NONE);

    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Status != GL_FRAMEBUFFER_COMPLETE) 
    {
        ri.Error(ERR_FATAL, "Shadow FBO error (0x%x)\n", Status);
        return false;
    }

#ifdef _DEBUG
    ri.Printf(PRINT_LOW, "r_texture_shadow_id = %i\n", r_texture_shadow_id);
#endif

    return true;
}

void R_BeginShadowMapPass()
{
    R_SetupProjection(shadowMapSize[0], shadowMapSize[1], 30.0f, 1.0f, 2048.0f, r_newrefdef.view.origin, r_newrefdef.view.flashlight_angles);
    R_UpdateCommonProgUniforms(false);

    R_MultiTextureBind(TMU_DIFFUSE, r_texture_white->texnum);
    R_MultiTextureBind(TMU_LIGHTMAP, r_texture_white->texnum);
    //R_MultiTextureBind(TMU_SHADOWMAP, r_texture_shadow_id);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_shadow_fbo);
    glViewport(0, 0, shadowMapSize[0], shadowMapSize[1]); 
    glClear(GL_DEPTH_BUFFER_BIT);

    gl_state.bShadowMapPass = true; // this also locks texture changes
}

void R_EndShadowMapPass()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    gl_state.bShadowMapPass = false;
}


void BindForReading()
{
    R_MultiTextureBind(TMU_SHADOWMAP, r_texture_shadow_id);
}