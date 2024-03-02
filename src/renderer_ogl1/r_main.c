/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_main.c
#include "r_local.h"

void R_Clear (void);

viddef_t	vid;


model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

glconfig_t gl_config;
glstate_t  gl_state;

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles

rentity_t	*pCurrentRefEnt;
model_t		*pCurrentModel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

rperfcounters_t rperf;

float		v_blend[4];			// final blending color

extern vec3_t	model_shadevector;
extern float	model_shadelight[3];

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;



/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (rentity_t *e)
{
    glTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    glRotatef (e->angles[1],  0, 0, 1);
    glRotatef (-e->angles[0],  0, 1, 0);
    glRotatef (-e->angles[2],  1, 0, 0);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (rentity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	sp2Frame_t	*frame;
	float		*up, *right;
	sp2Header_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (sp2Header_t *)pCurrentModel->extradata;

	if (e->frame < 0 || e->frame >= psprite->numframes)
	{
		ri.Printf (PRINT_ALL, "no such sprite frame %i\n", e->frame);
		e->frame = 0;
	}

	// advance frame
	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

#if 0
	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
	vec3_t		v_forward, v_right, v_up;

	AngleVectors (currententity->angles, v_forward, v_right, v_up);
		up = v_up;
		right = v_right;
	}
	else
#endif
	{	// normal sprite
		up = vup;
		right = vright;
	}

	if ( e->renderfx & RF_TRANSLUCENT )
		alpha = e->alpha;

	R_Blend(alpha != 1.0F);

	glColor4f( 1, 1, 1, alpha );

    GL_Bind(pCurrentModel->skins[e->frame]->texnum);
	GL_TexEnv( GL_MODULATE );

	R_AlphaTest(true); // IS THIS REALY CORRECT?

	glBegin (GL_QUADS);

	glTexCoord2f (0, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (0, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, -frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 0);
	VectorMA (e->origin, frame->height - frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	glVertex3fv (point);

	glTexCoord2f (1, 1);
	VectorMA (e->origin, -frame->origin_y, up, point);
	VectorMA (point, frame->width - frame->origin_x, right, point);
	glVertex3fv (point);
	
	glEnd ();

	R_AlphaTest(false);
	GL_TexEnv( GL_REPLACE );

	R_Blend(false);

	glColor4f( 1, 1, 1, 1 );
}

//==================================================================================

/*
=============
R_DrawNullModel

Draw the default model
=============
*/
static void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( pCurrentRefEnt->renderfx & RF_FULLBRIGHT )
		model_shadelight[0] = model_shadelight[1] = model_shadelight[2] = 1.0F;
	else
		R_LightPoint (pCurrentRefEnt->origin, shadelight);

    glPushMatrix ();
	R_RotateForEntity (pCurrentRefEnt);

	glDisable (GL_TEXTURE_2D);
	glColor3fv (shadelight);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		glVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	glEnd ();

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		glVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	glEnd ();

	glColor3f (1,1,1);
	glPopMatrix ();
	glEnable (GL_TEXTURE_2D);
}


void R_DrawEntityModel(rentity_t* ent);
static inline void R_DrawCurrentEntity()
{
	if (pCurrentRefEnt->renderfx & RF_BEAM)
	{
		R_DrawBeam(pCurrentRefEnt);
	}
	else
	{
		if (!pCurrentModel)
		{
			R_DrawNullModel();
		}
		else
		{
			if (pCurrentModel->type == MOD_BRUSH)
			{
				R_DrawBrushModel(pCurrentRefEnt);
			}
			else
			{
				R_DrawEntityModel(pCurrentRefEnt);
			}
		}
	}
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		pCurrentRefEnt = &r_newrefdef.entities[i];
		pCurrentModel = pCurrentRefEnt->model;
		if ((pCurrentRefEnt->renderfx & RF_TRANSLUCENT))
			continue;	// reject transparent
		R_DrawCurrentEntity();
	}


	// draw transparent entities
	R_WriteToDepthBuffer(GL_FALSE);	// no z writes
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		pCurrentRefEnt = &r_newrefdef.entities[i];
		pCurrentModel = pCurrentRefEnt->model;
		if (!(pCurrentRefEnt->renderfx & RF_TRANSLUCENT))
			continue;	// reject solid
		R_DrawCurrentEntity();
	}
	R_WriteToDepthBuffer(GL_TRUE);	// reenable z writing

	R_UnbindProgram();
}

/*
** GL_DrawParticles
**
*/
void GL_DrawParticles( int num_particles, const particle_t particles[] )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	float			scale;
	float			color[4];

    GL_Bind(r_particletexture->texnum);

	R_WriteToDepthBuffer(GL_FALSE);		// no z buffering
	R_Blend(true);
	GL_TexEnv( GL_MODULATE );

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	glBegin(GL_TRIANGLES);
	for ( p = particles, i=0 ; i < num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		scale = ( p->origin[0] - r_origin[0] ) * vpn[0] + 
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (scale < 20)
			scale = 1;
		else
			scale = 1 + scale * 0.004;

		VectorCopy(p->color, color);
		//VectorScale(vup, p->size[0], up);
		//VectorScale(vright, p->size[1], right);

		color[3] = p->alpha;

		glColor4fv( color );

		glTexCoord2f( 0.0625, 0.0625 );
		glVertex3fv( p->origin );

		glTexCoord2f( 1.0625, 0.0625 );
		glVertex3f( p->origin[0] + up[0]*scale, 
			         p->origin[1] + up[1]*scale, 
					 p->origin[2] + up[2]*scale);

		glTexCoord2f( 0.0625, 1.0625 );
		glVertex3f( p->origin[0] + right[0]*scale, 
			         p->origin[1] + right[1]*scale, 
					 p->origin[2] + right[2]*scale);
	}

	glEnd ();
	R_Blend(false);
	glColor4f( 1,1,1,1 );
	R_WriteToDepthBuffer(GL_TRUE);		// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	GL_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles);
}

/*
============
R_ViewBlendEffect
============
*/
void R_ViewBlendEffect (void)
{
	if (!v_blend[3])
		return; // transparent

	R_AlphaTest(false);
	R_Blend(true);
	R_DepthTest(false);
	glDisable (GL_TEXTURE_2D);

    glLoadIdentity ();

	// FIXME: get rid of these
    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up

	glColor4fv (v_blend);

	glBegin (GL_QUADS);

	glVertex3f (10, 100, 100);
	glVertex3f (10, -100, 100);
	glVertex3f (10, -100, -100);
	glVertex3f (10, 100, -100);
	glEnd ();

	R_AlphaTest(true);
	R_Blend(false);

	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
}

//=======================================================================

static int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.view.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.view.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.view.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.view.fov_y / 2 ) );

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.view.origin, r_origin);

	AngleVectors (r_newrefdef.view.angles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.view.flags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.view.fx.blend[i];

	rperf.brush_polys = 0;
	rperf.alias_tris = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if ( r_newrefdef.view.flags & RDF_NOWORLDMODEL )
	{
		glEnable( GL_SCISSOR_TEST );
		glClearColor( 0.3, 0.3, 0.3, 1 );
		glScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glClearColor( 1, 0, 0.5, 0.5 );
		glDisable( GL_SCISSOR_TEST );
	}
}


void MYgluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
   GLdouble xmin, xmax, ymin, ymax;

   ymax = zNear * tan( fovy * M_PI / 360.0 );
   ymin = -ymax;

   xmin = ymin * aspect;
   xmax = ymax * aspect;

   xmin += -( 2 * gl_state.camera_separation ) / zNear;
   xmax += -( 2 * gl_state.camera_separation ) / zNear;

   glFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
//	float	yfov;
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	glViewport (x, y2, w, h);

	//
	// set up projection matrix
	//
    screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
//	yfov = 2*atan((float)r_newrefdef.height/r_newrefdef.width)*180/M_PI;
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
    MYgluPerspective (r_newrefdef.view.fov_y,  screenaspect,  4, 4096);

	R_SetCullFace(GL_FRONT);

	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up
    glRotatef (-r_newrefdef.view.angles[2],  1, 0, 0);
    glRotatef (-r_newrefdef.view.angles[0],  0, 1, 0);
    glRotatef (-r_newrefdef.view.angles[1],  0, 0, 1);
    glTranslatef (-r_newrefdef.view.origin[0],  -r_newrefdef.view.origin[1],  -r_newrefdef.view.origin[2]);


	glGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	R_CullFace(r_cull->value);

	R_Blend(false);
	R_AlphaTest(false);
	R_DepthTest(true);
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_ztrick->value)
	{
		static int trickframe;

		if (r_clear->value)
			glClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			glDepthFunc (GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			glDepthFunc (GL_GEQUAL);
		}
	}
	else
	{
		if (r_clear->value)
			glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		glDepthFunc (GL_LEQUAL);
	}

	glDepthRange (gldepthmin, gldepthmax);

}


/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/

void R_ClearFBO();
void R_RenderToFBO(qboolean enable);
extern void R_DrawDebugLines(void);
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.view.flags & RDF_NOWORLDMODEL ) )
		ri.Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		rperf.brush_polys = 0;
		rperf.alias_tris = 0;
		rperf.texture_binds = 0;
	}



	R_PushDlights ();

	if (r_finish->value)
		glFinish ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_MarkLeaves ();	// done here so we know if we're in water

// begin rendering to fbo
	R_ClearFBO();
	R_RenderToFBO(true);

	R_DrawWorld();

	R_DrawEntitiesOnList();

	R_DrawDebugLines();

//	R_RenderDlights ();

	R_DrawAlphaSurfaces ();

	R_DrawParticles();

	R_ViewBlendEffect(); // braxi -- i don't like the old way of how blend is rendered, but i'll leave it for now and get back to it later

	R_RenderToFBO(false);
	// end rendering to fbo

	if (r_speeds->value)
	{
		ri.Printf (PRINT_ALL, "%4i bsppolys, %4i mdltris, %i vistex, %i vislmaps, %i texbinds\n",
			rperf.brush_polys,
			rperf.alias_tris,
			rperf.visible_textures, 
			rperf.visible_lightmaps,
			rperf.texture_binds); 
	}
}


void R_SetGL2D(void)
{
	// set 2D virtual screen size
	glViewport (0,0, vid.width, vid.height);
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
	glOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
	R_DepthTest(false);
	R_CullFace(false);
	R_Blend(false);
	R_AlphaTest(true);

	GL_TexEnv(GL_MODULATE);
}

static void GL_DrawColoredStereoLinePair( float r, float g, float b, float y )
{
	glColor3f( r, g, b );
	glVertex2f( 0, y );
	glVertex2f( vid.width, y );
	glColor3f( 0, 0, 0 );
	glVertex2f( 0, y + 1 );
	glVertex2f( vid.width, y + 1 );
}

static void GL_DrawStereoPattern( void )
{
	int i;

	if ( !gl_state.stereo_enabled )
		return;

	R_SetGL2D();

	glDrawBuffer( GL_BACK_LEFT );

	for ( i = 0; i < 20; i++ )
	{
		glBegin( GL_LINES );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 0 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 2 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 4 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 6 );
			GL_DrawColoredStereoLinePair( 0, 1, 0, 8 );
			GL_DrawColoredStereoLinePair( 1, 1, 0, 10);
			GL_DrawColoredStereoLinePair( 1, 1, 0, 12);
			GL_DrawColoredStereoLinePair( 0, 1, 0, 14);
		glEnd();
		
		GLimp_EndFrame();
	}
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	GL_TexEnv(GL_REPLACE);
	R_RenderView( fd );
	R_SetGL2D ();

	R_DrawFBO(0, 0, r_newrefdef.width, r_newrefdef.height, true);
//	R_DrawFBO(r_newrefdef.width/2, 0, r_newrefdef.width/2, r_newrefdef.height/2, false); // depth
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{

	gl_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( r_mode->modified || r_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = ri.Cvar_Get ("r_renderer", DEFAULT_RENDERER, 0);
		ref->modified = true;
	}

	if ( r_log->modified )
	{
		GLimp_EnableLogging( r_log->value );
		r_log->modified = false;
	}

	if ( r_log->value )
	{
		GLimp_LogNewFrame();
	}

	if ( r_gamma->modified )
	{
		r_gamma->modified = false;

		if (r_gamma->value < 0.3)
			ri.Cvar_Set("r_gamma", "0.3");
		else if (r_gamma->value > 2)
			ri.Cvar_Set("r_gamma", "2");
	}

	if (r_intensity->modified)
	{
		r_intensity->modified = false;

		if (r_intensity->value < 1)
			ri.Cvar_Set("r_intensity", "1");
		else if (r_intensity->value > 6)
			ri.Cvar_Set("r_intensity", "6");
	}

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	R_SetGL2D();

	/*
	** draw buffer stuff
	*/
	if ( r_drawbuffer->modified )
	{
		r_drawbuffer->modified = false;

		if ( gl_state.camera_separation == 0 || !gl_state.stereo_enabled )
		{
			if ( Q_stricmp( r_drawbuffer->string, "GL_FRONT" ) == 0 )
				glDrawBuffer( GL_FRONT );
			else
				glDrawBuffer( GL_BACK );
		}
	}

	/*
	** texturemode stuff
	*/
	if ( r_texturemode->modified )
	{
		GL_TextureMode( r_texturemode->string );
		r_texturemode->modified = false;
	}

	if ( r_texturealphamode->modified )
	{
		GL_TextureAlphaMode( r_texturealphamode->string );
		r_texturealphamode->modified = false;
	}

	if ( r_texturesolidmode->modified )
	{
		GL_TextureSolidMode( r_texturesolidmode->string );
		r_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
** R_DrawBeam
*/
void R_DrawBeam( rentity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	glDisable( GL_TEXTURE_2D );
	R_Blend(true);
	R_WriteToDepthBuffer(GL_FALSE);

//	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
//	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
//	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r = g = b = 0; // FIXME!
	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	glColor4f( r, g, b, e->alpha );

	glBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[i] );
		glVertex3fv( end_points[i] );
		glVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		glVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	glEnd();

	glEnable( GL_TEXTURE_2D );
	R_Blend(false);
	R_WriteToDepthBuffer(GL_TRUE);
}

