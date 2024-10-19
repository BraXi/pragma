/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/
// r_sky.c -- sky rendering

#include "r_local.h"

char	skyname[MAX_QPATH];
float	skyrotate;
vec3_t	skyaxis;
vec3_t	skycolor;
image_t	*sky_images[6];

vertexbuffer_t vb_sky;
static glvert_t skyverts[6];
static int numSkyVerts;
static char* sky_tex_prefix[6] = { "rt", "bk", "lf", "ft", "up", "dn" }; // environment map names

static float	skymins[2][6], skymaxs[2][6];
static float	sky_min, sky_max;

static const vec3_t skyclip[6] = 
{
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1} 
};

static int c_sky;

// 1 = s, 2 = t, 3 = 2048
static const int st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down

//	{-1,2,3},
//	{1,2,-3}
};

// s = [0]/[2], t = [1]/[2]
static const int vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};



/*
=============
R_DrawSkyPolygon
=============
*/
static void R_DrawSkyPolygon (int nump, vec3_t vecs)
{
	int		i,j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float	*vp;

	c_sky++;

#if 0
	glBegin (GL_TRIANGLE_FAN);
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		VectorAdd(vecs, r_origin, v);
		glVertex3fv (v);
	}
	glEnd();
	return;
#endif

	// decide which face it maps to
	VectorCopy (vec3_origin, v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
	{
		VectorAdd (vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];
		if (dv < 0.001)
			continue;	// don't divide by zero
		j = vec_to_st[axis][0];
		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j-1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j-1] / dv;

		if (s < skymins[0][axis])
			skymins[0][axis] = s;
		if (t < skymins[1][axis])
			skymins[1][axis] = t;
		if (s > skymaxs[0][axis])
			skymaxs[0][axis] = s;
		if (t > skymaxs[1][axis])
			skymaxs[1][axis] = t;
	}
}

#define	ON_EPSILON		0.1			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64

/*
=============
R_ClipSkyPolygon
=============
*/
static void R_ClipSkyPolygon(int nump, vec3_t vecs, int stage)
{
	const float *norm;
	float	*v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		ri.Error (ERR_DROP, "R_ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6)
	{	// fully clipped, so draw it
		R_DrawSkyPolygon (nump, vecs);
		return;
	}

	front = back = false;
	norm = skyclip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		R_ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v[j] + d*(v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	R_ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	R_ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}

/*
=================
R_AddSkySurface
=================
*/
void R_AddSkySurface(worldSurface_t *fa)
{
#if 0
	int			i;
	vec3_t		verts[MAX_CLIP_VERTS];
	poly_t	*p;

	// calculate vertex values for sky box
	for (p = fa->polys; p; p = p->next)
	{
		for (i = 0; i<p->numverts ; i++)
		{
			VectorSubtract (p->verts[i].pos, r_origin, verts[i]);
		}
		R_ClipSkyPolygon (p->numverts, verts[0], 0);
	}
#endif
}


/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox()
{
	int		i;

	for (i = 0; i < 6; i++)
	{
		skymins[0][i] = skymins[1][i] = 99999;
		skymaxs[0][i] = skymaxs[1][i] = -99999;
	}
}


/*
=============
R_MakeSkyVec
=============
*/
static void R_MakeSkyVec(float s, float t, int axis)
{
	vec3_t		v, b;
	int			j, k;

	b[0] = s*4048;
	b[1] = t*4048;
	b[2] = 4048;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}

	// avoid bilerp seam
	s = (s+1)*0.5;
	t = (t+1)*0.5;

	if (s < sky_min)
		s = sky_min;
	else if (s > sky_max)
		s = sky_max;
	if (t < sky_min)
		t = sky_min;
	else if (t > sky_max)
		t = sky_max;

//	t = 1.0 - t;	// braxi -- commented out, TGAs were upside down
	VectorCopy(v, skyverts[numSkyVerts].xyz);
	skyverts[numSkyVerts].st[0] = s;
	skyverts[numSkyVerts].st[1] = t;
	numSkyVerts++;
}


/*
==============
R_DrawSkyBox
==============
*/
void R_DrawSkyBox ()
{
	int		i;
	mat4_t mat;

	static const int skytexorder[6] = { 0,2,1,3,4,5 };

	if (skyrotate != 0.0f)
	{	// check for no sky at all
		for (i=0 ; i<6 ; i++)
			if (skymins[0][i] < skymaxs[0][i]
			&& skymins[1][i] < skymaxs[1][i])
				break;
		if (i == 6)
			return;		// nothing visible
	}


	memcpy(mat, r_world_matrix, sizeof(mat));
	Mat4Translate(mat, r_origin[0], r_origin[1], r_origin[2]);
	Mat4Rotate(mat, r_newrefdef.time * skyrotate, skyaxis[0], skyaxis[1], skyaxis[2]);

	for (i = 0; i < 6; i++)
	{
		if (skyrotate != 0.0f)
		{
			// Hack to force full sky to draw when rotating
			skymins[0][i] = -1;
			skymins[1][i] = -1;
			skymaxs[0][i] = 1;
			skymaxs[1][i] = 1;
		}

		if (skymins[0][i] >= skymaxs[0][i] || skymins[1][i] >= skymaxs[1][i])
			continue;
	
		numSkyVerts = 0;
		R_MakeSkyVec(skymins[0][i], skymins[1][i], i);
		R_MakeSkyVec(skymins[0][i], skymaxs[1][i], i);
		R_MakeSkyVec(skymaxs[0][i], skymaxs[1][i], i);
		R_MakeSkyVec(skymins[0][i], skymins[1][i], i);
		R_MakeSkyVec(skymaxs[0][i], skymaxs[1][i], i);
		R_MakeSkyVec(skymaxs[0][i], skymins[1][i], i);
		R_UpdateVertexBuffer(&vb_sky, skyverts, numSkyVerts, V_UV);

		R_BindProgram(GLPROG_SKY);
		R_ProgUniform4f(LOC_COLOR4, skycolor[0], skycolor[1], skycolor[2], 1.0f);
		R_ProgUniformMatrix4fv(LOC_MODELVIEW, 1, mat);
		R_MultiTextureBind(TMU_DIFFUSE, sky_images[skytexorder[i]]->texnum);
		R_DrawVertexBuffer(&vb_sky, 0, 0);
		R_UnbindProgram();
	}
}


/*
============
R_SetSky
============
*/
void R_SetSky(char *name, float rotate, vec3_t axis, vec3_t color)
{
	int		i;
	char	pathname[MAX_QPATH];

	strncpy (skyname, name, sizeof(skyname)-1);
	skyrotate = rotate;

	VectorCopy(axis, skyaxis);
	VectorCopy(color, skycolor);

	for (i = 0; i < 6; i++)
	{
		Com_sprintf (pathname, sizeof(pathname), "textures/sky/%s_%s.tga", skyname, sky_tex_prefix[i]);

		sky_images[i] = R_FindTexture (pathname, it_sky, true);
		if (!sky_images[i])
			sky_images[i] = r_texture_missing;

		sky_min = 1.0/512;
		sky_max = 511.0/512;
	}
}
