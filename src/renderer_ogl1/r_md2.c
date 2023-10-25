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


#include "r_local.h"

void R_SetEntityShadeLight(centity_t* ent);
extern model_t* Mod_ForNum(int index);

extern float	r_avertexnormals[162][3];
extern float	r_avertexnormal_dots[16][256];
extern float	*model_shadedots;
extern vec3_t	model_shadevector;
extern float	model_shadelight[3];

/*
==============================================================================
QUAKE2 MD2 MODELS
==============================================================================
*/
static	vec4_t	s_lerped[MD2_MAX_VERTS];

/*
=================
Mod_LoadMD2

Load .md2 model
=================
*/
void Mod_LoadMD2(model_t* mod, void* buffer)
{
	int					i, j;
	md2Header_t* pinmodel, * pheader;
	md2St_t* pinst, * poutst;
	md2Triangle_t* pintri, * pouttri;
	md2Frame_t* pinframe, * poutframe;
	int* pincmd, * poutcmd;
	int					version;

	pinmodel = (md2Header_t*)buffer;

	version = LittleLong(pinmodel->version);
	if (version != MD2_VERSION)
		ri.Sys_Error(ERR_DROP, "%s has wrong version number (%i should be %i)", mod->name, version, MD2_VERSION);

	pheader = Hunk_Alloc(LittleLong(pinmodel->ofs_end));

	// byte swap the header fields and sanity check
	for (i = 0; i < sizeof(md2Header_t) / 4; i++)
		((int*)pheader)[i] = LittleLong(((int*)buffer)[i]);

	if (pheader->skinheight > 480)
		ri.Sys_Error(ERR_DROP, "model %s has a skin taller than %d", mod->name, 480);

	if (pheader->num_xyz <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no vertices", mod->name);

	if (pheader->num_xyz > MD2_MAX_VERTS)
		ri.Sys_Error(ERR_DROP, "model %s has too many vertices", mod->name);

	if (pheader->num_st <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no st vertices", mod->name);

	if (pheader->num_tris <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no triangles", mod->name);

	if (pheader->num_frames <= 0)
		ri.Sys_Error(ERR_DROP, "model %s has no frames", mod->name);

	//
	// load base s and t vertices (not used in gl version)
	//
	pinst = (md2St_t*)((byte*)pinmodel + pheader->ofs_st);
	poutst = (md2St_t*)((byte*)pheader + pheader->ofs_st);

	for (i = 0; i < pheader->num_st; i++)
	{
		poutst[i].s = LittleShort(pinst[i].s);
		poutst[i].t = LittleShort(pinst[i].t);
	}

	//
	// load triangle lists
	//
	pintri = (md2Triangle_t*)((byte*)pinmodel + pheader->ofs_tris);
	pouttri = (md2Triangle_t*)((byte*)pheader + pheader->ofs_tris);

	for (i = 0; i < pheader->num_tris; i++)
	{
		for (j = 0; j < 3; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort(pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort(pintri[i].index_st[j]);
		}
	}

	//
	// load the frames
	//
	for (i = 0; i < pheader->num_frames; i++)
	{
		pinframe = (md2Frame_t*)((byte*)pinmodel
			+ pheader->ofs_frames + i * pheader->framesize);
		poutframe = (md2Frame_t*)((byte*)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy(poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j = 0; j < 3; j++)
		{
			poutframe->scale[j] = LittleFloat(pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat(pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		memcpy(poutframe->verts, pinframe->verts,
			pheader->num_xyz * sizeof(md2Vertex_t));

	}

	mod->type = MOD_MD2;

	//
	// load the glcmds
	//
	pincmd = (int*)((byte*)pinmodel + pheader->ofs_glcmds);
	poutcmd = (int*)((byte*)pheader + pheader->ofs_glcmds);
	for (i = 0; i < pheader->num_glcmds; i++)
		poutcmd[i] = LittleLong(pincmd[i]);


	// register all skins
	memcpy((char*)pheader + pheader->ofs_skins, (char*)pinmodel + pheader->ofs_skins, pheader->num_skins * MD2_MAX_SKINNAME);
	for (i = 0; i < pheader->num_skins; i++)
	{
		mod->skins[i] = GL_FindImage((char*)pheader + pheader->ofs_skins + i * MD2_MAX_SKINNAME, it_model);
	}

	mod->mins[0] = -32;
	mod->mins[1] = -32;
	mod->mins[2] = -32;
	mod->maxs[0] = 32;
	mod->maxs[1] = 32;
	mod->maxs[2] = 32;
}


/*
=============
R_MD2_LerpVerts
=============
*/
void R_MD2_LerpVerts(int nverts, md2Vertex_t* v, md2Vertex_t* ov, md2Vertex_t* verts, float* lerp, float move[3], float frontv[3], float backv[3])
{
	int i;
	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))
	{
		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			float* normal = r_avertexnormals[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0] + normal[0] * POWERSUIT_SCALE;
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1] + normal[1] * POWERSUIT_SCALE;
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2] + normal[2] * POWERSUIT_SCALE;
		}
	}
	else
	{
		for (i = 0; i < nverts; i++, v++, ov++, lerp += 4)
		{
			lerp[0] = move[0] + ov->v[0] * backv[0] + v->v[0] * frontv[0];
			lerp[1] = move[1] + ov->v[1] * backv[1] + v->v[1] * frontv[1];
			lerp[2] = move[2] + ov->v[2] * backv[2] + v->v[2] * frontv[2];
		}
	}
}

/*
=============
R_DrawMD2FrameLerp

interpolates between two frames and origins
FIXME: batch lerp all vertexes
=============
*/
void R_DrawMD2FrameLerp(md2Header_t* paliashdr, float backlerp)
{
	float 	l;
	md2Frame_t* frame, * oldframe;
	md2Vertex_t* v, * ov, * verts;
	int* order;
	int		count;
	float	frontlerp;
	float	alpha;
	vec3_t	move, delta, vectors[3];
	vec3_t	frontv, backv;
	int		i;
	int		index_xyz;
	float* lerp;

	frame = (md2Frame_t*)((byte*)paliashdr + paliashdr->ofs_frames
		+ currententity->frame * paliashdr->framesize);
	verts = v = frame->verts;

	oldframe = (md2Frame_t*)((byte*)paliashdr + paliashdr->ofs_frames
		+ currententity->oldframe * paliashdr->framesize);
	ov = oldframe->verts;

	order = (int*)((byte*)paliashdr + paliashdr->ofs_glcmds);

	//	glTranslatef (frame->translate[0], frame->translate[1], frame->translate[2]);
	//	glScalef (frame->scale[0], frame->scale[1], frame->scale[2]);

	if (currententity->flags & RF_TRANSLUCENT)
		alpha = currententity->alpha;
	else
		alpha = 1.0;

	// PMM - added double shell
	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))
		qglDisable(GL_TEXTURE_2D);

	frontlerp = 1.0 - backlerp;

	// move should be the delta back to the previous frame * backlerp
	VectorSubtract(currententity->oldorigin, currententity->origin, delta);
	AngleVectors(currententity->angles, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct(delta, vectors[0]);	// forward
	move[1] = -DotProduct(delta, vectors[1]);	// left
	move[2] = DotProduct(delta, vectors[2]);	// up

	VectorAdd(move, oldframe->translate, move);

	for (i = 0; i < 3; i++)
	{
		move[i] = backlerp * move[i] + frontlerp * frame->translate[i];
	}

	for (i = 0; i < 3; i++)
	{
		frontv[i] = frontlerp * frame->scale[i];
		backv[i] = backlerp * oldframe->scale[i];
	}

	lerp = s_lerped[0];

	R_MD2_LerpVerts(paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv);

	if (r_vertex_arrays->value)
	{
		float colorArray[MD2_MAX_VERTS * 4];

		qglEnableClientState(GL_VERTEX_ARRAY);
		qglVertexPointer(3, GL_FLOAT, 16, s_lerped);	// padded for SIMD

//		if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE ) )
		// PMM - added double damage shell
		if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))
		{
			qglColor4f(model_shadelight[0], model_shadelight[1], model_shadelight[2], alpha);
		}
		else
		{
			qglEnableClientState(GL_COLOR_ARRAY);
			qglColorPointer(3, GL_FLOAT, 0, colorArray);

			//
			// pre light everything
			//
			for (i = 0; i < paliashdr->num_xyz; i++)
			{
				float l = model_shadedots[verts[i].lightnormalindex];

				colorArray[i * 3 + 0] = l * model_shadelight[0];
				colorArray[i * 3 + 1] = l * model_shadelight[1];
				colorArray[i * 3 + 2] = l * model_shadelight[2];
			}
		}

		if (qglLockArraysEXT != 0)
			qglLockArraysEXT(0, paliashdr->num_xyz);

		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				qglBegin(GL_TRIANGLE_FAN);
			}
			else
			{
				qglBegin(GL_TRIANGLE_STRIP);
			}

			// PMM - added double damage shell
			if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					qglVertex3fv(s_lerped[index_xyz]);

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					qglTexCoord2f(((float*)order)[0], ((float*)order)[1]);
					index_xyz = order[2];

					order += 3;

					// normals and vertexes come from the frame list
//					l = shadedots[verts[index_xyz].lightnormalindex];

//					qglColor4f (l* model_shadelight[0], l*model_shadelight[1], l*model_shadelight[2], alpha);
					qglArrayElement(index_xyz);

				} while (--count);
			}
			qglEnd();
		}

		if (qglUnlockArraysEXT != 0)
			qglUnlockArraysEXT();
	}
	else
	{
		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				qglBegin(GL_TRIANGLE_FAN);
			}
			else
			{
				qglBegin(GL_TRIANGLE_STRIP);
			}

			if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
			{
				do
				{
					index_xyz = order[2];
					order += 3;

					qglColor4f(model_shadelight[0], model_shadelight[1], model_shadelight[2], alpha);
					qglVertex3fv(s_lerped[index_xyz]);

				} while (--count);
			}
			else
			{
				do
				{
					// texture coordinates come from the draw list
					qglTexCoord2f(((float*)order)[0], ((float*)order)[1]);
					index_xyz = order[2];
					order += 3;

					// normals and vertexes come from the frame list
					l = model_shadedots[verts[index_xyz].lightnormalindex];

					qglColor4f(l * model_shadelight[0], l * model_shadelight[1], l * model_shadelight[2], alpha);
					qglVertex3fv(s_lerped[index_xyz]);
				} while (--count);
			}

			qglEnd();
		}
	}

	//	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE ) )
		// PMM - added double damage shell
	if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM))
		qglEnable(GL_TEXTURE_2D);
}




/*
=============
R_DrawMD2Shadow
=============
*/
extern	vec3_t			lightspot;

void R_MD2_DrawShadow(md2Header_t* paliashdr, int posenum)
{
	md2Vertex_t* verts;
	int* order;
	vec3_t	point;
	float	height, lheight;
	int		count;
	md2Frame_t* frame;

	lheight = currententity->origin[2] - lightspot[2];

	frame = (md2Frame_t*)((byte*)paliashdr + paliashdr->ofs_frames
		+ currententity->frame * paliashdr->framesize);
	verts = frame->verts;

	height = 0;

	order = (int*)((byte*)paliashdr + paliashdr->ofs_glcmds);

	height = -lheight + 1.0;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			qglBegin(GL_TRIANGLE_FAN);
		}
		else
			qglBegin(GL_TRIANGLE_STRIP);

		do
		{
#if 0
			// normals and vertexes come from the frame list
			point[0] = verts[order[2]].v[0] * frame->scale[0] + frame->translate[0];
			point[1] = verts[order[2]].v[1] * frame->scale[1] + frame->translate[1];
			point[2] = verts[order[2]].v[2] * frame->scale[2] + frame->translate[2];
#endif
			memcpy(point, s_lerped[order[2]], sizeof(point));

			point[0] -= model_shadevector[0] * (point[2] + lheight);
			point[1] -= model_shadevector[1] * (point[2] + lheight);
			point[2] = height;
			//			height -= 0.001;
			qglVertex3fv(point);

			order += 3;

			//			verts++;

		} while (--count);

		qglEnd();
	}
}

/*
=============
R_MD2_CullModel
=============
*/
qboolean R_MD2_CullModel(vec3_t bbox[8], centity_t* e)
{
	int i;
	vec3_t		mins, maxs;
	md2Header_t* paliashdr;
	vec3_t		vectors[3];
	vec3_t		thismins, oldmins, thismaxs, oldmaxs;
	md2Frame_t* pframe, * poldframe;
	vec3_t angles;

	paliashdr = (md2Header_t*)currentmodel->extradata;

	if ((e->frame >= paliashdr->num_frames) || (e->frame < 0))
	{
		ri.Con_Printf(PRINT_ALL, "R_CullMD2Model: no such oldframe %d in '%s'\n", e->frame, currentmodel->name);
		e->frame = 0;
	}
	if ((e->oldframe >= paliashdr->num_frames) || (e->oldframe < 0))
	{
		ri.Con_Printf(PRINT_ALL, "R_CullMD2Model: no such oldframe %d in '%s'\n", e->oldframe, currentmodel->name );
		e->oldframe = 0;
	}

	pframe = (md2Frame_t*)((byte*)paliashdr +
		paliashdr->ofs_frames +
		e->frame * paliashdr->framesize);

	poldframe = (md2Frame_t*)((byte*)paliashdr +
		paliashdr->ofs_frames +
		e->oldframe * paliashdr->framesize);

	/*
	** compute axially aligned mins and maxs
	*/
	if (pframe == poldframe)
	{
		for (i = 0; i < 3; i++)
		{
			mins[i] = pframe->translate[i];
			maxs[i] = mins[i] + pframe->scale[i] * 255;
		}
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			thismins[i] = pframe->translate[i];
			thismaxs[i] = thismins[i] + pframe->scale[i] * 255;

			oldmins[i] = poldframe->translate[i];
			oldmaxs[i] = oldmins[i] + poldframe->scale[i] * 255;

			if (thismins[i] < oldmins[i])
				mins[i] = thismins[i];
			else
				mins[i] = oldmins[i];

			if (thismaxs[i] > oldmaxs[i])
				maxs[i] = thismaxs[i];
			else
				maxs[i] = oldmaxs[i];
		}
	}

	/*
	** compute a full bounding box
	*/
	for (i = 0; i < 8; i++)
	{
		vec3_t   tmp;

		if (i & 1)
			tmp[0] = mins[0];
		else
			tmp[0] = maxs[0];

		if (i & 2)
			tmp[1] = mins[1];
		else
			tmp[1] = maxs[1];

		if (i & 4)
			tmp[2] = mins[2];
		else
			tmp[2] = maxs[2];

		VectorCopy(tmp, bbox[i]);
	}

	/*
	** rotate the bounding box
	*/
	VectorCopy(e->angles, angles);
	angles[YAW] = -angles[YAW];
	AngleVectors(angles, vectors[0], vectors[1], vectors[2]);

	for (i = 0; i < 8; i++)
	{
		vec3_t tmp;

		VectorCopy(bbox[i], tmp);

		bbox[i][0] = DotProduct(vectors[0], tmp);
		bbox[i][1] = -DotProduct(vectors[1], tmp);
		bbox[i][2] = DotProduct(vectors[2], tmp);

		VectorAdd(e->origin, bbox[i], bbox[i]);
	}

	{
		int p, f, aggregatemask = ~0;

		for (p = 0; p < 8; p++)
		{
			int mask = 0;

			for (f = 0; f < 4; f++)
			{
				float dp = DotProduct(frustum[f].normal, bbox[p]);

				if ((dp - frustum[f].dist) < 0)
				{
					mask |= (1 << f);
				}
			}

			aggregatemask &= mask;
		}

		if (aggregatemask)
		{
			return true;
		}

		return false;
	}
}


/*
=================
R_DrawMD2Model

=================
*/

void R_DrawMD2Model(centity_t* e)
{
	md2Header_t* paliashdr;
	vec3_t		bbox[8];
	image_t* skin;

	if (!(e->flags & RF_WEAPONMODEL))
	{
		if (R_MD2_CullModel(bbox, e))
			return;
	}

	if (e->flags & RF_WEAPONMODEL)
	{
		if (r_lefthand->value == 2)
			return;
	}

	paliashdr = (md2Header_t*)currentmodel->extradata;

	// 
	// setup lighting

	R_SetEntityShadeLight(e);



	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if (currententity->flags & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
		qglDepthRange(gldepthmin, gldepthmin + 0.3 * (gldepthmax - gldepthmin));

	if ((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F))
	{
		extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();
		qglScalef(-1, 1, 1);
		MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / r_newrefdef.height, 4, 4096);
		qglMatrixMode(GL_MODELVIEW);

		qglCullFace(GL_BACK);
	}

	qglPushMatrix();
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	R_RotateForEntity(e);
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.

	// select skin
	if (currententity->skin)
		skin = currententity->skin;	// custom player skin
	else
	{
		if (currententity->skinnum >= MD2_MAX_SKINS)
			skin = currentmodel->skins[0];
		else
		{
			skin = currentmodel->skins[currententity->skinnum];
			if (!skin)
				skin = currentmodel->skins[0];
		}
	}
	if (!skin)
		skin = r_notexture;	// fallback...
	GL_Bind(skin->texnum);

	// draw it

	qglShadeModel(GL_SMOOTH);

	GL_TexEnv(GL_MODULATE);
	if (currententity->flags & RF_TRANSLUCENT)
	{
		qglEnable(GL_BLEND);
	}


	if ((currententity->frame >= paliashdr->num_frames) || (currententity->frame < 0))
	{
		ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such frame %d\n", currentmodel->name, currententity->frame);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ((currententity->oldframe >= paliashdr->num_frames) || (currententity->oldframe < 0))
	{
		ri.Con_Printf(PRINT_ALL, "R_DrawAliasModel %s: no such oldframe %d\n", currentmodel->name, currententity->oldframe);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if (!r_lerpmodels->value)
		currententity->backlerp = 0;

	R_DrawMD2FrameLerp(paliashdr, currententity->backlerp);

	GL_TexEnv(GL_REPLACE);
	qglShadeModel(GL_FLAT);

	qglPopMatrix();



#if 0
	qglDisable(GL_CULL_FACE);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	qglDisable(GL_TEXTURE_2D);
	qglBegin(GL_TRIANGLE_STRIP);
	for (i = 0; i < 8; i++)
	{
		qglVertex3fv(bbox[i]);
	}
	qglEnd();
	qglEnable(GL_TEXTURE_2D);
	qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	qglEnable(GL_CULL_FACE);
#endif

	if ((currententity->flags & RF_WEAPONMODEL) && (r_lefthand->value == 1.0F))
	{
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		qglCullFace(GL_FRONT);
	}

	if (currententity->flags & RF_TRANSLUCENT)
	{
		qglDisable(GL_BLEND);
	}

	if (currententity->flags & RF_DEPTHHACK)
		qglDepthRange(gldepthmin, gldepthmax);

#if 1
	if (r_shadows->value && !(currententity->flags & (RF_TRANSLUCENT | RF_WEAPONMODEL)))
	{
		qglPushMatrix();
		R_RotateForEntity(e);
		qglDisable(GL_TEXTURE_2D);
		qglEnable(GL_BLEND);
		qglColor4f(0, 0, 0, 0.5);
		R_MD2_DrawShadow(paliashdr, currententity->frame);
		qglEnable(GL_TEXTURE_2D);
		qglDisable(GL_BLEND);
		qglPopMatrix();
	}
#endif
	qglColor4f(1, 1, 1, 1);
}
