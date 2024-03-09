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

typedef enum { SH_FRAG, SH_VERT } glShaderType_t;

typedef struct glprogloc_s
{
	int loc;
	char* name;
	fieldtype_t type;
} glprogloc_t;

static glprogloc_t progUniLocs[NUM_LOCS] =
{
	/* global */
	{ LOC_COLORMAP,			"colormap",			F_INT },
	{ LOC_COLOR4,			"color_rgba",		F_FLOAT },
	{ LOC_SCALE,			"scale",			F_VECTOR3 },
	{ LOC_TIME,				"time",				F_FLOAT }, // fixme: unset

	/* md3 models */
	{ LOC_SHADEVECTOR,		"shade_vector",		F_VECTOR3 },
	{ LOC_SHADECOLOR,		"shade_light",		F_VECTOR3 },
	{ LOC_LERPFRAC,			"lerpFrac",			F_FLOAT },

	/* for materials */
	{ LOC_PARM0,			"parm0_f",			F_FLOAT },
	{ LOC_PARM1,			"parm1_f",			F_FLOAT },
	{ LOC_PARM2,			"parm2_f",			F_FLOAT },

	/* post processing */
	{ LOC_SCREENSIZE,		"screensize",		F_VECTOR2 },
	{ LOC_INTENSITY,		"r_intensity",		F_FLOAT },
	{ LOC_GAMMA,			"r_gamma",			F_FLOAT },
	{ LOC_BLUR,				"fx_blur",			F_FLOAT },
	{ LOC_CONTRAST,			"fx_contrast",		F_FLOAT },
	{ LOC_GRAYSCALE,		"fx_grayscale",		F_FLOAT },
	{ LOC_INVERSE,			"fx_inverse",		F_FLOAT },
	{ LOC_NOISE,			"fx_noise",			F_FLOAT },

};

/* vertex attributes */
static glprogloc_t progVertAtrribLocs[NUM_VALOCS] =
{
	{ VALOC_POS,			"inVertPos",		F_VECTOR3 },
	{ VALOC_NORMAL,			"inNormal",			F_VECTOR3 },
	{ VALOC_TEXCOORD,		"inTexCoord",		F_VECTOR3 },
	{ VALOC_COLOR,			"inVertCol",		F_VECTOR3 },

	/*md3 rendering only*/
	{ VALOC_OLD_POS,		"inOldVertPos",		F_VECTOR3 },
	{ VALOC_OLD_NORMAL,		"inOldNormal",		F_VECTOR3 },
};

#define CheckProgUni(uni) \
{ \
	if (pCurrentProgram == NULL || pCurrentProgram->isValid == false || uni == -1) \
	{ \
		return; \
	} \
	if (pCurrentProgram->locs[uni] == -1) \
	{ \
		return; \
	} \
}

/*
=================
R_ProgramIndex
=================
*/
glprog_t *R_ProgramIndex(int progindex)
{
	if (progindex >= MAX_GLPROGS || progindex < 0)
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
R_FindProgramUniform
=================
*/
int R_FindProgramUniform(char *name)
{
	GLint loc;
	if(pCurrentProgram == NULL || pCurrentProgram->isValid == false)
		return -1;

	loc = glGetUniformLocation(pCurrentProgram->programObject, name);
//	if(loc == -1)
//		ri.Printf(PRINT_ALERT, "no uniform %s in program %s\n", name, pCurrentProgram->name);
	return loc;
}

// single
/*
=================
R_ProgUniform1i
=================
*/
void R_ProgUniform1i(int uniform, int val)
{
	CheckProgUni(uniform);
	glUniform1i(pCurrentProgram->locs[uniform], val);
}

/*
=================
R_ProgUniform1f
=================
*/
void R_ProgUniform1f(int uniform, float val)
{
	CheckProgUni(uniform);
	glUniform1f(pCurrentProgram->locs[uniform], val);
}

// two params
/*
=================
R_ProgUniform2i
=================
*/
void R_ProgUniform2i(int uniform, int val, int val2)
{
	CheckProgUni(uniform);
	glUniform2i(pCurrentProgram->locs[uniform], val, val2);
}

/*
=================
R_ProgUniform2f
=================
*/
void R_ProgUniform2f(int uniform, float val, float val2)
{
	CheckProgUni(uniform);	
	glUniform2f(pCurrentProgram->locs[uniform], val, val2);
}

/*
=================
R_ProgUniformVec2
=================
*/
void R_ProgUniformVec2(int uniform, vec2_t v)
{
	R_ProgUniform2f(uniform, v[0], v[1]);
}

// three params
/*
=================
R_ProgUniform3i
=================
*/
void R_ProgUniform3i(int uniform, int val, int val2, int val3)
{
	CheckProgUni(uniform);
	glUniform3i(pCurrentProgram->locs[uniform], val, val2, val3);
}

/*
=================
R_ProgUniform3f
=================
*/
void R_ProgUniform3f(int uniform, float val, float val2, float val3)
{
	CheckProgUni(uniform);
	glUniform3f(pCurrentProgram->locs[uniform], val, val2, val3);
}

/*
=================
R_ProgUniformVec3
=================
*/
void R_ProgUniformVec3(int uniform, vec3_t v)
{
	CheckProgUni(uniform);
	glUniform3f(pCurrentProgram->locs[uniform], v[0], v[1], v[2]);
}


// four params
/*
=================
R_ProgUniform4i
=================
*/
void R_ProgUniform4i(int uniform, int val, int val2, int val3, int val4)
{
	CheckProgUni(uniform);
	glUniform4i(pCurrentProgram->locs[uniform], val, val2, val3, val4);
}

/*
=================
R_ProgUniform3f
=================
*/
void R_ProgUniform4f(int uniform, float val, float val2, float val3, float val4)
{
	CheckProgUni(uniform);
	glUniform4f(pCurrentProgram->locs[uniform], val, val2, val3, val4);
}

/*
=================
R_ProgUniformVec4
=================
*/
void R_ProgUniformVec4(int uniform, vec4_t v)
{
	CheckProgUni(uniform);
	glUniform4f(pCurrentProgram->locs[uniform], v[0], v[1], v[2], v[3]);
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
static qboolean R_CheckShaderObject(glprog_t *prog, unsigned int shaderObject, char *name)
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
		ri.Error(ERR_FATAL, "Failed to compile %s shader for program \"%s\".\n\nError log:\n%s\n", name, prog->name, errorLog);

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
	if (!R_CheckShaderObject(prog, prog->fragmentShader, "fragment"))
	{
		prog->fragmentShader = 0;
		return false;
	}

	if (!R_CheckShaderObject(prog, prog->vertexShader, "vertex"))
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
R_FindUniformLocations
=================
*/
static void R_FindUniformLocations(glprog_t* prog)
{
	R_BindProgram(prog->index);

	for (int i = 0; i < NUM_LOCS; i++)
		prog->locs[i] = -1;

	// find default uniform locations
	for (int i = 0; i < NUM_LOCS; i++)
		prog->locs[progUniLocs[i].loc] = R_FindProgramUniform(progUniLocs[i].name);

	// set default values
	R_ProgUniform1i(LOC_COLORMAP, 0);
	R_UnbindProgram();
}

/*
=================
R_FindVertexAttrivLocations
=================
*/
static void R_FindVertexAttribLocations(glprog_t* prog)
{
	int i;
	R_BindProgram(prog->index);

	for (int i = 0; i < NUM_VALOCS; i++)
		prog->valocs[i] = -1;

	for (i = 0; i < NUM_VALOCS; i++)
	{
		prog->valocs[progVertAtrribLocs[i].loc] = glGetAttribLocation(pCurrentProgram->programObject, progVertAtrribLocs[i].name);
//		if(prog->index == 3 && prog->valocs[progVertAtrribLocs[i].loc] == -1)
//				ri.Printf(PRINT_ALERT, "attrib %s not found in program %s\n", progVertAtrribLocs[i].name, prog->name);
	}

	R_UnbindProgram();
}

/*
=================
R_GetProgAttribLoc
=================
*/
int R_GetProgAttribLoc(glprogLoc_t attrib)
{
	if (pCurrentProgram == NULL)
		return -1;
	return pCurrentProgram->valocs[attrib];
}

/*
=================
R_GetProgAttribName
=================
*/
char *R_GetProgAttribName(glprogLoc_t attrib)
{
	return progVertAtrribLocs[attrib].name;
}
/*
=================
R_UsingProgram

Returns true if any program is in use
=================
*/
qboolean R_UsingProgram()
{
	if (pCurrentProgram == NULL || pCurrentProgram->isValid == false )
	{
		return false;
	}
	return true;
}

/*
=================
R_GetCurrentProgramName

Returns the name of currently used program
=================
*/
char *R_GetCurrentProgramName()
{
	if (!R_UsingProgram())
	{
		return "*none*";
	}
	return pCurrentProgram->name;
}

/*
=================
R_LoadProgram
=================
*/
static int R_LoadProgram(int program, char *name)
{
	glprog_t* prog;
	
	prog = R_ProgramIndex(program);

	strcpy(prog->name, name);

	if (!R_CompileShader(prog, true))
		return -1;
	if (!R_CompileShader(prog, false))
		return -1;
	if (!R_LinkProgram(prog))
		return -1;

	prog->isValid = true;
	prog->index = program;
	numProgs++;

	R_FindUniformLocations(prog);
	R_FindVertexAttribLocations(prog);

	return prog->index;
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

	for (int i = 0; i < MAX_GLPROGS; i++)
	{
		prog = &glprogs[i];
		R_FreeProgram(prog);
	}

	memset(glprogs, 0, sizeof(glprogs));
	pCurrentProgram = NULL;
	numProgs = 0;
}


/*
=================
R_InitPrograms
=================
*/
void R_InitPrograms()
{
	R_FreePrograms();

	R_LoadProgram(GLPROG_WORLD, "world");
	R_LoadProgram(GLPROG_WORLD_LIQUID, "world-liquid");
	R_LoadProgram(GLPROG_SKY, "sky");
	R_LoadProgram(GLPROG_ALIAS, "model_alias");
	R_LoadProgram(GLPROG_SPRITE, "model_sprite");
	R_LoadProgram(GLPROG_PARTICLE, "particle");
	R_LoadProgram(GLPROG_GUI, "gui");
	R_LoadProgram(GLPROG_POSTFX, "postfx");
	R_LoadProgram(GLPROG_DEBUGSTRING, "debug_string");
	R_LoadProgram(GLPROG_DEBUGLINE, "debug_line");
}


void R_ShaderList_f(void)
{
	glprog_t* prog;
	int cnt = 0;

	ri.Printf(PRINT_ALL, "------------------\n");
	for (int i = 0; i < MAX_GLPROGS; i++)
	{
		prog = &glprogs[i];

		if (prog->isValid)
		{
			ri.Printf(PRINT_ALL, "%i '%s'\n", i, prog->name);
			cnt++;
		}
	}
	ri.Printf(PRINT_ALL, "Total %i shader programs.\n", cnt);
}