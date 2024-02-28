/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"



typedef struct glprog_s
{
	char name[MAX_QPATH];

	/*GLuint*/ unsigned int		programObject;
	/*GLuint*/ unsigned int		vertexShader, fragmentShader;

	qboolean isValid;
} glprog_t;


glprog_t *pCurrentProgram = NULL;


/*
=================
R_BindProgram
=================
*/
void R_BindProgram(glprog_t* prog)
{
	if (prog != NULL && pCurrentProgram == prog)
		return;

	if (prog->isValid == false)
		return;

	glUseProgram(prog->programObject);
	pCurrentProgram = prog;
}


/*
=================
R_UnbindProgram
=================
*/
void R_UnbindProgram()
{
	if (pCurrentProgram == NULL)
		return;

	glUseProgram(0);
}

void R_FreeProgram(glprog_t* prog)
{
	if (pCurrentProgram == prog)
		R_UnbindProgram();

	if (prog->vertexShader)
			glDeleteShader(prog->vertexShader);
	if (prog->fragmentShader)
		glDeleteShader(prog->fragmentShader);
	if (prog->programObject)
		glDeleteProgram(prog->programObject);

//	memset(prog, 0, sizeof(glprog_t));
	free(prog);
	prog = NULL;
}

/*
=================
R_CheckShaderObject
=================
*/
static qboolean R_CheckShaderObject(glprog_t *shader, unsigned int shaderObject)
{
	GLint compiled = 0;
	GLint blen = 0;
	GLsizei slen = 0;

#if 0
	glGetObjectParameteriv((GLuint)shaderObject, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		glGetShaderiv((GLuint)shaderObject, GL_INFO_LOG_LENGTH, &blen);
		if (blen > 1)
		{
			GLchar* compiler_log = (GLchar*)malloc(blen);

			glGetInfoLogARB((GLuint)shaderObject, blen, &slen, compiler_log);
			ri.Error(ERR_FATAL, "Failed to compile shader %s\n log: %s\n", shader->name, compiler_log);
			free(compiler_log);
		}
		return false;
	}
#endif
	return true;
}

/*
=================
R_CompileShaderProgram
=================
*/
static qboolean R_CompileShaderProgram(glprog_t* shader, qboolean isfrag)
{
	char	fileName[MAX_OSPATH];
	char	*data = NULL;
	int		len;
	unsigned int prog;

	Com_sprintf(fileName, sizeof(fileName), "shaders/%s.%s", shader->name);
	len = FS_LoadTextFile(fileName, (void**)&data);
	if (!len || len == -1 || data == NULL)
	{
		return false;
	}

	prog = glCreateShader( isfrag == true ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
	glShaderSource(prog, 1, (const GLcharARB**)&data, (const GLint*)&len);
	glCompileShader(prog);

	if(R_CheckShaderObject)
	if (isfrag == true)
		shader->fragmentShader = prog;
	else
		shader->vertexShader = prog;

	return true;
}



static qboolean R_LinkProgram(glprog_t* shader)
{
	GLint linked;

	// create program, and link vp and fp to it
	shader->programObject = glCreateProgram();
	glAttachShader(shader->programObject, shader->vertexShader);
	glAttachShader(shader->programObject, shader->fragmentShader);
	glLinkProgram(shader->programObject);

	glGetProgramiv(shader->programObject, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		ri.Printf(PRINT_LOW, "Failed to load shader program: %s\n", shader->name);
		return false;
	}

	shader->isValid = true;
	ri.Printf(PRINT_LOW, "Loaded shader program: %s\n", shader->name);
	return true;
}

