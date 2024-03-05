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

/*
===============
SetVBOClientState

Enable or disable client states for vertex buffer rendering
===============
*/
static void SetVBOClientState(vertexbuffer_t* vbo, qboolean enable)
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
R_AllocVertexBuffer

Create vertex buffer object and allocate space for verticles
===============
*/
vertexbuffer_t* R_AllocVertexBuffer(vboFlags_t flags, unsigned int numVerts, unsigned int numIndices)
{
	vertexbuffer_t* vbo = NULL;

	vbo = ri.MemAlloc(sizeof(vertexbuffer_t));
	if (!vbo)
	{
		ri.Error(ERR_FATAL, "R_AllocVertexBuffer failed\n");
		return NULL;
	}

	vbo->flags = flags;

	glGenBuffers(1, &vbo->vboBuf);

//	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);
//	glBufferData(GL_ARRAY_BUFFER, (numVerts * sizeof(glvert_t)), &vbo->verts->xyz[0], GL_STATIC_DRAW);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (numVerts)
	{
		vbo->verts = ri.MemAlloc(sizeof(glvert_t) * numVerts);
		if (!vbo->verts)
		{
			ri.Error(ERR_FATAL, "R_AllocVertexBuffer failed to allocate %i vertices\n", numVerts);
			return NULL;
		}
		vbo->numVerts = numVerts;
	}

	if ((flags & V_INDICES) && numIndices > 0)
	{
		glGenBuffers(1, &vbo->indexBuf);
		vbo->indices = ri.MemAlloc(sizeof(unsigned short) * numIndices);
		{
			ri.Error(ERR_FATAL, "R_AllocVertexBuffer failed to allocate %i indices\n", numIndices);
			return NULL;
		}

		vbo->numIndices = numIndices;
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->indexBuf);
//		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * 3, vbo->indices, GL_STATIC_DRAW);
	}

	return vbo;
}

/*
===============
R_UpdateVertexBuffer
===============
*/
void R_UpdateVertexBuffer(vertexbuffer_t* vbo, glvert_t* verts, unsigned int numVerts, vboFlags_t flags)
{
	if (vbo->vboBuf == 0)
		glGenBuffers(1, &vbo->vboBuf);

	if (vbo->verts != NULL && vbo->numVerts != numVerts)
	{
		ri.Error(ERR_FATAL, "R_UpdateVertexBuffer probably scrapping allocated verts\n", numVerts);
	}

	vbo->numVerts = numVerts;
	vbo->flags = flags;

	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);
	glBufferData(GL_ARRAY_BUFFER, (numVerts * sizeof(glvert_t)), &verts->xyz[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
===============
R_UpdateVertexBufferIndices
===============
*/
void R_UpdateVertexBufferIndices(vertexbuffer_t* vbo, unsigned short* indices, unsigned int numIndices)
{
	if (vbo->indexBuf == 0)
		glGenBuffers(1, &vbo->indexBuf);

	if (vbo->indices != NULL && vbo->numIndices != numIndices)
	{
		ri.Error(ERR_FATAL, "R_UpdateVertexBufferIndices probably scrapping allocated indices\n", numIndices);
	}

	vbo->numIndices = numIndices;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->indexBuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (numIndices * sizeof(unsigned short)), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


/*
===============
R_DrawVertexBuffer
===============
*/
void R_DrawVertexBuffer(vertexbuffer_t* vbo, unsigned int startVert, unsigned int numVerts)
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
	if (vbo->numIndices && vbo->indices != NULL && vbo->indexBuf) //if ((vbo->flags & V_INDICES) && vbo->indexBuf)
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
R_DeleteVertexBuffers
===============
*/
void R_DeleteVertexBuffers(vertexbuffer_t* vbo)
{
	if (vbo->vboBuf)
		glDeleteBuffers(1, &vbo->vboBuf);

	if (vbo->indexBuf)
		glDeleteBuffers(1, &vbo->indexBuf);
}

/*
===============
R_FreeVertexBuffer

this does FREE vertexbuffer
===============
*/
void R_FreeVertexBuffer(vertexbuffer_t* vbo)
{
	if (!vbo || vbo == NULL)
		return;

	R_DeleteVertexBuffers(vbo);

	if (vbo->indices)
	{
		ri.MemFree(vbo->indices);
		vbo->indices = NULL;
	}

	if (vbo->verts)
	{
		ri.MemFree(vbo->verts);
		vbo->verts = NULL;
	}

	ri.MemFree(vbo);
	vbo = NULL;
//	memset(vbo, 0, sizeof(*vbo));
}