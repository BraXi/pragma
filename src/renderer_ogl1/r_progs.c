/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

glprog_t *pCurrentProgram = NULL;
int numProgs;

glprog_t glprogs[MAX_GLPROGS];

/*
=================
R_ProgramIndex
=================
*/
glprog_t *R_ProgramIndex(int progindex)
{
	if (progindex >= numProgs || progindex < 0)
		return NULL;

	return &glprogs[progindex];
}

/*
=================
R_BindProgram
=================
*/
void R_BindProgram(int progindex)
{
	glprog_t* prog = R_ProgramIndex(progindex);

	if (prog == NULL || pCurrentProgram == prog)
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
	pCurrentProgram = NULL;
}

/*
=================
R_FreeProgram
=================
*/
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

	memset(prog, 0, sizeof(glprog_t));
//	free(prog);
//	prog = NULL;
}

/*
=================
R_CheckShaderObject
=================
*/
static qboolean R_CheckShaderObject(glprog_t *prog, unsigned int shaderObject)
{
	GLint	isCompiled = 0;
	char	*errorLog;

	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &maxLength);

		errorLog = (char*)malloc(sizeof(char) * maxLength);

		glGetShaderInfoLog(shaderObject, maxLength, &maxLength, &errorLog[0]);
		ri.Error(ERR_DROP, "Failed to compile shader object for program %s (error log below)\n%s\n", prog->name, errorLog);

		glDeleteShader(shaderObject);
		free(errorLog);
		return false;
	}

	return true;
}

/*
=================
R_CompileShader
=================
*/
static qboolean R_CompileShader(glprog_t* glprog, qboolean isfrag)
{
	char	fileName[MAX_OSPATH];
	char	*data = NULL;
	int		len;
	unsigned int prog;

	Com_sprintf(fileName, sizeof(fileName), "shaders/%s.%s", glprog->name, isfrag == true ? "fp" : "vp");
	len = ri.LoadTextFile(fileName, (void**)&data);
	if (!len || len == -1 || data == NULL)
	{
		ri.Error(ERR_FATAL, "failed to load shader: %s\n", fileName);
		return false;
	}

	prog = glCreateShader( isfrag == true ? GL_FRAGMENT_SHADER : GL_VERTEX_SHADER);
	glShaderSource(prog, 1, (const GLcharARB**)&data, (const GLint*)&len);
	glCompileShader(prog);

	if (isfrag == true)
		glprog->fragmentShader = prog;
	else
		glprog->vertexShader = prog;

	return true;
}


/*
=================
R_LinkProgram
=================
*/
static qboolean R_LinkProgram(glprog_t* prog)
{
	GLint linked;

	//
	// check for errors
	//
	if (!R_CheckShaderObject(prog, prog->fragmentShader))
	{
		prog->fragmentShader = 0;
		return false;
	}

	if (!R_CheckShaderObject(prog, prog->vertexShader))
	{
		prog->vertexShader = 0;
		return false;
	}

	//
	// create program, and link vp and fp to it
	//
	prog->programObject = glCreateProgram();
	glAttachShader(prog->programObject, prog->vertexShader);
	glAttachShader(prog->programObject, prog->fragmentShader);
	glLinkProgram(prog->programObject);

	glGetProgramiv(prog->programObject, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		ri.Printf(PRINT_ALERT, "Failed to load shader program: %s\n", prog->name);
		return false;
	}

	prog->isValid = true;
	ri.Printf(PRINT_LOW, "Loaded shader program: %s\n", prog->name);
	return true;
}

/*
=================
R_LoadProgram
=================
*/
int R_LoadProgram(char *name)
{
	glprog_t* prog;
	
	prog = &glprogs[numProgs];


	strncpy(prog->name, name, sizeof(prog->name));

	R_CompileShader(prog, true);
	R_CompileShader(prog, false);
	R_LinkProgram(prog);

	numProgs++;
	return numProgs - 1;
}


/*
=================
R_FreePrograms
=================
*/
void R_FreePrograms()
{
	glprog_t* prog;

	R_UnbindProgram();

	for (int i = 0; i < numProgs; i++)
	{
		prog = &glprogs[numProgs];
		R_FreeProgram(prog);
	}

	memset(glprogs, 0, sizeof(glprogs));
	pCurrentProgram = NULL;
	numProgs = 0;
}