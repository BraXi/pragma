/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// vertex buffer objects


#include "r_local.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

static void SetVBOClientState(vbo_t* vbo, qboolean enable)
{
	if (enable)
	{
		glEnableClientState(GL_VERTEX_ARRAY);

		if (vbo->flags & V_NORMAL)
			glEnableClientState(GL_NORMAL_ARRAY);

		if (vbo->flags & V_UV)
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		if (vbo->flags & V_COLOR)
			glEnableClientState(GL_COLOR_ARRAY);
	}
	else
	{
		glDisableClientState(GL_VERTEX_ARRAY);

		if (vbo->flags & V_NORMAL)
			glDisableClientState(GL_NORMAL_ARRAY);

		if (vbo->flags & V_UV)
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		if (vbo->flags & V_COLOR)
			glDisableClientState(GL_COLOR_ARRAY);
	}

}

/*
===============
R_StuffVBO
===============
*/
void R_StuffVBO(vbo_t* vbo, glvert_t* verts, unsigned int numVerts, vboFlags_t flags)
{
	if (vbo->vboBuf == 0)
		glGenBuffers(1, &vbo->vboBuf);

	vbo->indexBuf = 0;
//	vbo->verts = verts;
	vbo->numVerts = numVerts;
	vbo->flags = flags;

	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);
	glBufferData(GL_ARRAY_BUFFER, (numVerts * sizeof(glvert_t)), &verts->xyz[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
===============
R_RenderVBO
===============
*/
void R_RenderVBO(vbo_t* vbo, unsigned int startVert, unsigned int numVerts)
{
	SetVBOClientState(vbo, true);
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);

	glVertexPointer(3, GL_FLOAT, sizeof(glvert_t), BUFFER_OFFSET(0));
	if (vbo->flags & V_NORMAL)
		glNormalPointer(GL_FLOAT, sizeof(glvert_t), BUFFER_OFFSET(12));	
	if (vbo->flags & V_UV)
		glTexCoordPointer(2, GL_FLOAT, sizeof(glvert_t), BUFFER_OFFSET(24));
	if (vbo->flags & V_COLOR)
		glColorPointer(3, GL_FLOAT, sizeof(glvert_t), BUFFER_OFFSET(32));

	// case one: we have index buffer
	if ((vbo->flags & V_INDICES) && vbo->indexBuf)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->indexBuf);
		
		//glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
		if(numVerts)
			glDrawRangeElements(GL_TRIANGLES, startVert, startVert + numVerts, numVerts, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
		else
			glDrawRangeElements(GL_TRIANGLES, 0, vbo->numVerts, vbo->numVerts, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else // case two: we don't have index buffer and for simple primitives like gui we usualy don't
	{
		if (!numVerts)
			glDrawArrays(GL_TRIANGLES, 0, vbo->numVerts);
		else
			glDrawArrays(GL_TRIANGLES, startVert, numVerts);
	}

	SetVBOClientState(vbo, false);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


/*
===============
R_FreeVBO
===============
*/
void R_FreeVBO(vbo_t* vbo)
{
	if(vbo->vboBuf)
		glDeleteBuffers(1, &vbo->vboBuf);

	if (vbo->indexBuf)
		glDeleteBuffers(1, &vbo->indexBuf);

	memset(vbo, 0, sizeof(*vbo));
}