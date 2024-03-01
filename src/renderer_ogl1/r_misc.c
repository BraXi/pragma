/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_misc.c

#include "r_local.h"

/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	r_particletexture = R_LoadTexture("*particle", (byte *)data, 8, 8, it_sprite, 32);

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 0;// dottexture[x & 3][y & 3] * 255;
			data[y][x][1] = dottexture[x&3][y&3]*255;
			data[y][x][2] = 0;// dottexture[x & 3][y & 3] * 255;
			data[y][x][3] = 255;
		}
	}
	r_notexture = R_LoadTexture ("*default_texture", (byte *)data, 8, 8, it_texture, 32);
}


/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/* 
================== 
GL_ScreenShot_f
================== 
*/  
void GL_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;

	buffer = malloc(vid.width * vid.height * 3 + 18);
	if (!buffer)
	{
		ri.Printf(PRINT_ALL, "GL_ScreenShot_f: failed to allocate pixels\n");
		return;
	}

	// create the screenshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/screenshots", ri.GetGameDir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"shot000.tga");

	for (i=0 ; i<=999; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, sizeof(checkname), "%s/screenshots/%s", ri.GetGameDir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==1000) 
	{
		ri.Printf (PRINT_ALL, "GL_ScreenShot_f: Couldn't create a file\n"); 
		free(buffer);
		return;
 	}


	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	glPixelStorei(GL_PACK_ALIGNMENT, 1); //fix from yquake2
	glReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 ); 

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);


	free (buffer);
	ri.Printf (PRINT_ALL, "Wrote %s\n", picname);
} 

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	ri.Printf (PRINT_ALL, "GL_VENDOR: %s\n", gl_config.vendor_string );
	ri.Printf (PRINT_ALL, "GL_RENDERER: %s\n", gl_config.renderer_string );
	ri.Printf (PRINT_ALL, "GL_VERSION: %s\n", gl_config.version_string );
//	ri.Printf (PRINT_ALL, "GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	R_UnbindProgram();

	glClearColor (1,0, 0.5 , 0.5);
	R_SetCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	R_AlphaTest(true);
	glAlphaFunc(GL_GREATER, 0.666);

	R_DepthTest(false);
	R_CullFace(false);
	R_Blend(false);

	glColor4f (1,1,1,1);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_FLAT);

	GL_TextureMode( r_texturemode->string );
	GL_TextureAlphaMode( r_texturealphamode->string );
	GL_TextureSolidMode( r_texturesolidmode->string );

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	R_BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

	if ( glPointParameterf )
	{
		float attenuations[3];

		attenuations[0] = r_particle_att_a->value;
		attenuations[1] = r_particle_att_b->value;
		attenuations[2] = r_particle_att_c->value;

		glEnable( GL_POINT_SMOOTH );
		glPointParameterf( GL_POINT_SIZE_MIN, r_particle_min_size->value );
		glPointParameterf( GL_POINT_SIZE_MAX, r_particle_max_size->value );
		glPointParameterfv( GL_POINT_DISTANCE_ATTENUATION, attenuations );
	}
	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( r_swapinterval->modified )
	{
		r_swapinterval->modified = false;

		if ( !gl_state.stereo_enabled ) 
		{
#ifdef _WIN32
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( r_swapinterval->value );
#endif
		}
	}
}