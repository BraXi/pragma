/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// cl_view.c -- player rendering positioning

#include "client.h"

//=============
//
// development tools for weapons
//
int			gun_frame;
struct model_s	*gun_model;

//=============

cvar_t		*cl_testparticles;
cvar_t		*testmodel;
cvar_t		*cl_testlights;
cvar_t		*cl_testblend;

cvar_t		*cl_stats;


int			r_numdlights;
dlight_t	r_dlights[MAX_DLIGHTS];

int			r_numentities;
rentity_t	r_entities[MAX_VISIBLE_ENTITIES];

int			r_numparticles;
particle_t	r_particles[MAX_PARTICLES];

int			r_numdebugprimitives;
debugprimitive_t	r_debugprimitives[MAX_DEBUG_PRIMITIVES];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

/*
====================
V_ClearScene

Specifies the model that will be used as the world
====================
*/
void V_ClearScene (void)
{
	r_numdlights = 0;
	r_numentities = 0;
	r_numparticles = 0;
	r_numdebugprimitives = 0;
}


/*
=====================
V_AddEntity
=====================
*/
void V_AddEntity(rentity_t* ent)
{
	if (r_numentities >= MAX_VISIBLE_ENTITIES)
	{
		Com_DPrintf(DP_REND, "V_AddEntity: r_numentities >= MAX_VISIBLE_ENTITIES\n");
		return;
	}
	ent->index = r_numentities;
	r_entities[r_numentities++] = *ent;
}


/*
=====================
V_AddDebugPrimitive
=====================
*/
void V_AddDebugPrimitive(debugprimitive_t *obj)
{
	if (r_numdebugprimitives >= MAX_DEBUG_PRIMITIVES)
	{
		Com_DPrintf(DP_REND, "V_AddDebugPrimitive: r_numdebugprimitives >= MAX_DEBUG_PRIMITIVES\n");
		return;
	}
	r_debugprimitives[r_numdebugprimitives++] = *obj;
}

/*
=====================
V_AddParticle
=====================
*/
void V_AddParticle (vec3_t org, vec3_t color, float alpha, vec2_t size)
{
	particle_t	*p;

	if (r_numparticles >= MAX_PARTICLES)
	{
		Com_DPrintf(DP_REND, "V_AddParticle: r_numparticles >= MAX_PARTICLES\n");
		return;
	}

	p = &r_particles[r_numparticles++];
	VectorCopy (org, p->origin);
	VectorCopy(color, p->color);
	VectorCopy(size, p->size);
	p->alpha = alpha;
}

/*
=====================
V_AddPointLight
=====================
*/
void V_AddPointLight(vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
	{
		Com_DPrintf(DP_REND, "V_AddPointLight: r_numdlights >= MAX_DLIGHTS\n");
		return;
	}

	dl = &r_dlights[r_numdlights++];
	dl->type = DL_POINTLIGHT;

	VectorCopy (org, dl->origin);
	dl->intensity = intensity;
	VectorSet(dl->color, r, g, b);
}

/*
=====================
V_AddSpotLight
=====================
*/
void V_AddSpotLight(vec3_t org, vec3_t dir, float intensity, float cutoff, float r, float g, float b)
{
	dlight_t* dl;

	if (r_numdlights >= MAX_DLIGHTS)
	{
		Com_DPrintf(DP_REND, "V_AddSpotLight: r_numdlights >= MAX_DLIGHTS\n");
		return;
	}

	dl = &r_dlights[r_numdlights++];
	dl->type = DL_SPOTLIGHT;

	VectorCopy(org, dl->origin);
	VectorCopy(dir, dl->dir);
	dl->intensity = intensity;
	dl->cutoff = cutoff;
	VectorSet(dl->color, r, g, b);
}


/*
=====================
V_AddLightStyle
=====================
*/
void V_AddLightStyle (int style, float r, float g, float b)
{
	lightstyle_t	*ls;

	if (style < 0 || style > MAX_LIGHTSTYLES)
	{
		Com_Error(ERR_DROP, "V_AddLightStyle: Bad light style %i", style);
		return;
	}
	
	ls = &r_lightstyles[style];

	ls->white = r+g+b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

/*
================
V_TestParticles

If cl_testparticles is set, create MAX_PARTICLES particles in the view
================
*/
void V_TestParticles (void)
{
	particle_t	*p;
	int			i, j;
	float		d, r, u;

	r_numparticles = MAX_PARTICLES;
	for (i = 0; i < r_numparticles; i++)
	{
		d = i*0.25;
		r = 4*((i&7)-3.5);
		u = 4*(((i>>3)&7)-3.5);
		p = &r_particles[i];

		for (j = 0; j < 3; j++)
			p->origin[j] = cl.refdef.view.origin[j] + cl.v_forward[j]*d + cl.v_right[j]*r + cl.v_up[j]*u;

		VectorSet(p->color, 0.382353, 0.882353, 0.482353);
		p->alpha = cl_testparticles->value;
	}
}

/*
================
V_TestModel
================
*/

static rentity_t tm;
void V_TestModel(void)
{
	char modname[MAX_QPATH];

	if (testmodel->modified)
	{
		testmodel->modified = false;
		memset(&tm, 0, sizeof(tm));

		//for (int i = 0; i < 3; i++)
		//	tm.origin[i] = cl.refdef.view.origin[i] + cl.v_forward[i] * 96 - cl.v_up[i] * 10;

		for (int i = 0; i < 2; i++)
			tm.origin[i] = cl.refdef.view.origin[i] + cl.v_forward[i] * 96;

		tm.origin[2] = cl.refdef.view.origin[2] - 60.0f; // HARDCODED view height

		tm.angles[1] = cl.refdef.view.angles[1];

		Com_sprintf(modname, sizeof(modname), "models/%s", testmodel->string);
		tm.model = re.RegisterModel(modname);
	}

	if(tm.model)
		V_AddEntity(&tm);
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
static void V_TestLights()
{
	int			i, j;
	float		f, r;
	dlight_t	*dl;

	if (!cl_testlights->value)
		return;

	if (cl_testlights->value > 1.0f)
	{
		//r_numdlights = 1;

		dl = &r_dlights[r_numdlights++];
		dl->type = DL_SPOTLIGHT;

		for (j = 0; j < 3; j++)
			dl->origin[j] = cl.refdef.view.origin[j] + (cl.v_forward[j] * 10);

		VectorSet(dl->color, 1.0f, 1.0f, 1.0f);
		dl->intensity = 800;
		dl->cutoff = -0.96f;
		VectorCopy(cl.v_forward, dl->dir);
		return;
	}

	memset(r_dlights, 0, sizeof(r_dlights));
	r_numdlights = MAX_DLIGHTS;

	for (i = 0; i < r_numdlights; i++)
	{
		dl = &r_dlights[i];
		dl->type = DL_POINTLIGHT;

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j = 0; j < 3; j++)
			dl->origin[j] = cl.refdef.view.origin[j] + (cl.v_forward[j] * f) + (cl.v_right[j] * r);
		dl->color[0] = ((i%6)+1) & 1;
		dl->color[1] = (((i%6)+1) & 2)>>1;
		dl->color[2] = (((i%6)+1) & 4)>>2;
		dl->intensity = 200;
	}
}

//===================================================================

/*
=================
CL_SetSkyFromConfigstring
=================
*/
void CL_SetSkyFromConfigstring(void)
{
	float	skyrotate;
	vec3_t	skyaxis, skycolor;
	int		val; // shut up compiler warning

	skyrotate = atof(cl.configstrings[CS_SKYROTATE]);

	val = sscanf(cl.configstrings[CS_SKYAXIS], "%f %f %f", &skyaxis[0], &skyaxis[1], &skyaxis[2]);
	val = sscanf(cl.configstrings[CS_SKYCOLOR], "%f %f %f", &skycolor[0], &skycolor[1], &skycolor[2]);

	re.SetSky(cl.configstrings[CS_SKY], skyrotate, skyaxis, skycolor);
}



/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepRefresh (void)
{
	char		mapname[32]; // FIXME stack corruption with long map names "sewerjam1_doomshakalaka_full"
	int			i;
	char		name[MAX_QPATH];


	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded

	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width-1, viddef.height-1);

	// let the render dll load the map
	strcpy (mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[strlen(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf ("Map: %s\r", mapname); 
	SCR_UpdateScreen ();
	re.BeginRegistration (mapname);
	Com_Printf ("                                     \r");

	// precache status bar pics
	Com_Printf ("pics\r"); 
	SCR_UpdateScreen ();
	Com_Printf ("                                     \r");

	CG_RegisterMedia ();


	for (i=1 ; i < MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
	{
		strcpy (name, cl.configstrings[CS_MODELS+i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			Com_Printf ("%s\r", name); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop

		char* n = cl.configstrings[CS_MODELS + i];
		cl.model_draw[i] = re.RegisterModel (n);
		if (name[0] == '*')
			cl.model_clip[i] = CM_InlineModel (n);
		else
			cl.model_clip[i] = NULL;

		if (name[0] != '*')
			Com_Printf ("                                     \r");
	}

	Com_Printf ("images\r", i); 
	SCR_UpdateScreen ();
	for (i=1 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
	{
		cl.image_precache[i] = re.RegisterPic (cl.configstrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}
	
	Com_Printf ("                                     \r");


	// set sky textures and speed
	Com_Printf ("sky\r", i); 
	SCR_UpdateScreen ();

	CL_SetSkyFromConfigstring();

	Com_Printf ("                                     \r");

	// the renderer can now free unneeded stuff
	re.EndRegistration ();

	// clear any lines of console text
	Con_ClearNotify ();

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================


// gun frame debugging functions
void V_Gun_Next_f (void)
{
	if (!CL_CheatsAllowed())
	{
		Com_Printf("'%s' - cheats not allowed\n", Cmd_Argv(0));
		return;
	}

	gun_frame++;
	Com_Printf ("gun_frame %i\n", gun_frame);
}

void V_Gun_Prev_f (void)
{
	if (!CL_CheatsAllowed())
	{
		Com_Printf("'%s' - cheats not allowed\n", Cmd_Argv(0));
		return;
	}
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf ("gun_frame %i\n", gun_frame);
}

void V_Gun_Model_f (void)
{
	char	name[MAX_QPATH];

	if (!CL_CheatsAllowed())
	{
		Com_Printf("'%s' - cheats not allowed\n", Cmd_Argv(0));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		gun_model = NULL;
		return;
	}

	Com_sprintf (name, sizeof(name), "models/%s", Cmd_Argv(1));
	gun_model = re.RegisterModel (name);

	if(gun_model)
		Com_Printf("gun_model set to: %s\n", name);
}
//============================================================================


/*
==================
V_RenderView
==================
*/
extern void SV_AddDebugPrimitives();
void V_RenderView(float stereo_separation)
{
	extern int entitycmpfnc(const rentity_t*, const rentity_t*);

	if (!cl.refresh_prepped) // still loading
	{
		re.RenderFrame(&cl.refdef, true);
		return;		
	}

	if (cls.state != CS_ACTIVE) // not fully on server
	{
		re.RenderFrame(&cl.refdef, true);
		return;
	}

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
	{
		cl.force_refdef = false;

		V_ClearScene ();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities();

		SV_AddDebugPrimitives();


		if (cl_testparticles->value)
			V_TestParticles ();

		if (testmodel->string[0])
			V_TestModel ();
		
		V_TestLights ();

		if (cl_testblend->value)
		{
			cl.refdef.view.fx.blend[0] = 1;
			cl.refdef.view.fx.blend[1] = 0.5;
			cl.refdef.view.fx.blend[2] = 0.25;
			cl.refdef.view.fx.blend[3] = 0.5;
		}


		// offset vieworg appropriately if we're doing stereo separation
		if ( stereo_separation != 0 )
		{
			vec3_t tmp;

			VectorScale( cl.v_right, stereo_separation, tmp );
			VectorAdd( cl.refdef.view.origin, tmp, cl.refdef.view.origin );
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.view.origin[0] += 1.0/16;
		cl.refdef.view.origin[1] += 1.0/16;
		cl.refdef.view.origin[2] += 1.0/16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		cl.refdef.view.fov_y = CalcFov (cl.refdef.view.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time*0.001;

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;

		if (!cl_add_particles->value)
			r_numparticles = 0;

		if (!cl_add_lights->value)
			r_numdlights = 0;

		if (!cl_add_blend->value)
			VectorClear (cl.refdef.view.fx.blend);


		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;

		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;

		cl.refdef.num_debugprimitives = r_numdebugprimitives;
		cl.refdef.debugprimitives = r_debugprimitives;

		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;

		cl.refdef.lightstyles = r_lightstyles;

		cl.refdef.view.flags = cl.frame.playerstate.rdflags;

		// sort entities for better cache locality
        qsort( cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), (int (*)(const void *, const void *))entitycmpfnc );
	}

	re.RenderFrame (&cl.refdef, false);

	if (cl_stats->value)
		Com_Printf ("ents:%i dlights:%i particles:%i dbg:%i\n", r_numentities, r_numdlights, r_numparticles, r_numdebugprimitives);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,%i,",r_numentities, r_numdlights, r_numparticles, r_numdebugprimitives);

	SCR_AddDirtyPoint (scr_vrect.x, scr_vrect.y);
	SCR_AddDirtyPoint (scr_vrect.x+scr_vrect.width-1, scr_vrect.y+scr_vrect.height-1);
}


/*
=============
V_Viewpos_f
=============
*/
void V_Viewpos_f (void)
{
	Com_Printf ("(%i %i %i) : yaw %i\n", (int)cl.refdef.view.origin[0],
		(int)cl.refdef.view.origin[1], (int)cl.refdef.view.origin[2], 
		(int)cl.refdef.view.angles[YAW]);
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	// these 3 are cheats
	Cmd_AddCommand ("gun_next", V_Gun_Next_f); 
	Cmd_AddCommand ("gun_prev", V_Gun_Prev_f);
	Cmd_AddCommand ("gun_model", V_Gun_Model_f);

	Cmd_AddCommand ("viewpos", V_Viewpos_f);

	cl_testblend = Cvar_Get ("cl_testblend", "0", CVAR_CHEAT, NULL);
	cl_testparticles = Cvar_Get ("cl_testparticles", "0", CVAR_CHEAT, NULL);
	testmodel = Cvar_Get ("testmodel", "", CVAR_CHEAT, NULL);
	cl_testlights = Cvar_Get ("cl_testlights", "0", CVAR_CHEAT, NULL);

	cl_stats = Cvar_Get ("cl_stats", "0", 0, NULL);
}
