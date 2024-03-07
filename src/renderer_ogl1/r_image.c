/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

#include "r_local.h"

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;

//qboolean R_UploadTexture32 (unsigned *data, int width, int height,  qboolean mipmap);

int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

void R_EnableMultitexture( qboolean enable )
{
	if (!glActiveTexture)
		return;

	if ( enable )
	{
		R_SelectTextureUnit( GL_TEXTURE1 );
		glEnable( GL_TEXTURE_2D );
		//R_SetTexEnv( GL_REPLACE );
	}
	else
	{
		R_SelectTextureUnit( GL_TEXTURE1 );
		glDisable( GL_TEXTURE_2D );
		//R_SetTexEnv( GL_REPLACE );
	}
	R_SelectTextureUnit( GL_TEXTURE0 );
	//R_SetTexEnv( GL_REPLACE );
}

void R_SelectTextureUnit( GLenum texture )
{
	int tmu;

	if (!glActiveTexture)
		return;

	if ( texture == GL_TEXTURE0 )
		tmu = 0;
	else
		tmu = 1;

	if ( tmu == gl_state.currenttmu )
		return;

	gl_state.currenttmu = tmu;

	if ( tmu == 0 )
		glActiveTexture( GL_TEXTURE0 );
	else
		glActiveTexture( GL_TEXTURE1 );
}

void R_SetTexEnv(GLenum mode)
{
	static int lastmodes[2] = { -1, -1 };

	if ( mode != lastmodes[gl_state.currenttmu] )
	{
		//glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[gl_state.currenttmu] = mode;
	}
}

void R_BindTexture(int texnum)
{
	extern	image_t	*font_current;

	if (r_nobind->value && font_current && texnum == font_current->texnum) // performance evaluation option
		texnum = font_current->texnum;
	if ( gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;

	rperf.texture_binds++;

	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	glBindTexture(GL_TEXTURE_2D, texnum);
}

void R_MultiTextureBind(GLenum target, int texnum)
{
	if (!glActiveTexture)
		return;

	R_SelectTextureUnit( target );
	if ( target == GL_TEXTURE0 )
	{
		if ( gl_state.currenttextures[0] == texnum )
			return;
	}
	else
	{
		if ( gl_state.currenttextures[1] == texnum )
			return;
	}
	R_BindTexture( texnum );
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct
{
	char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
===============
R_SetTextureMode
===============
*/
void R_SetTextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !Q_stricmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_gui && glt->type != it_sky )
		{
			R_BindTexture (glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
R_SetTextureAlphaMode
===============
*/
void R_SetTextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !Q_stricmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		ri.Printf (PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
R_TextureSolidMode
===============
*/
void R_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !Q_stricmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		ri.Printf (PRINT_ALL, "bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
R_TextureList_f
===============
*/
void R_TextureList_f(void)
{
	int		i;
	image_t	*image;
	int		texels;

	ri.Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_model:
			ri.Printf (PRINT_ALL, "MDL ");
			break;
		case it_sprite:
			ri.Printf (PRINT_ALL, "SPR ");
			break;
		case it_texture:
			ri.Printf (PRINT_ALL, "TEX ");
			break;
		case it_gui:
			ri.Printf (PRINT_ALL, "GUI ");
			break;
		case it_tga:
			ri.Printf(PRINT_ALL, "TGA ");
			break;
		case it_sky:
			ri.Printf(PRINT_ALL, "SKY ");
			break;
		default:
			ri.Printf (PRINT_ALL, "?   ");
			break;
		}

		ri.Printf (PRINT_ALL, "%i: [%ix%i %s]: %s\n",
			i, image->upload_width, image->upload_height, (image->has_alpha ? "RGBA" : "RGB"), image->name);
	}
	ri.Printf (PRINT_ALL, "\nTotal texel count (not counting mipmaps): %i\n", texels);
	ri.Printf(PRINT_ALL, "Total %i out of %i textures in use\n\n", numgltextures, MAX_GLTEXTURES);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

#define TGA_TYPE_RAW_RGB 2 // Uncompressed, RGB images
#define TGA_TYPE_RUNLENGHT_RGB 10 // Runlength encoded RGB images


typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
R_LoadTGA
=============
*/
void R_LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = ri.LoadFile (name, (void **)&buffer);
	if (!buffer)
	{
//		ri.Printf (PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type != TGA_TYPE_RAW_RGB && targa_header.image_type != TGA_TYPE_RUNLENGHT_RGB)
	{
		ri.Error(ERR_DROP, "R_LoadTGA: '%s' Only type 2 and 10 targa RGB images supported\n", name);
	}

	if (targa_header.colormap_type !=0 || (targa_header.pixel_size != 32 && targa_header.pixel_size != 24))
		ri.Error (ERR_DROP, "R_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment
	
	if (targa_header.image_type == TGA_TYPE_RAW_RGB)
	{  
		for(row=rows-1; row>=0; row--) 
		{
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) 
			{
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) 
				{
					case 24:
							
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type == TGA_TYPE_RUNLENGHT_RGB) 
	{   
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) 
		{
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) 
			{
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) // run-length packet
				{
					switch (targa_header.pixel_size) 
					{
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
					}
	
					for(j=0;j<packetSize;j++) 
					{
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) // run spans across rows
						{ 
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else // non run-length packet
				{
					for(j=0;j<packetSize;j++) 
					{
						switch (targa_header.pixel_size) 
						{
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column == columns)  // pixel packet run spans across rows
						{ 
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}
	ri.FreeFile (buffer);
}


/*
================
R_ResampleTexture
================
*/
void R_ResampleTexture(unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for (i=0 ; i<outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
R_MipMapTexture

Operates in place, quartering the size of the texture
================
*/
void R_MipMapTexture (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

/*
===============
R_UploadTexture32

Returns has_alpha
===============
*/

static int		upload_width, upload_height;
static char		*upload_name;

static int IsPowerOfTwo(int x)
{
	if (x == 0)
		return 0;
	return (x & (x - 1)) == 0;
}

qboolean R_UploadTexture32(unsigned* data, int width, int height, qboolean mipmap)
{
	int			samples;
	int			i, c;
	byte		*scan;
	int			comp;
	int			scaled_width, scaled_height;

	if (!IsPowerOfTwo(width) || !IsPowerOfTwo(height))
	{
		ri.Error(ERR_FATAL, "Texture \"%s\" [%i x %i] dimensions are not power of two.\n", upload_name, width, height);
	}

	// let people sample down the world textures for speed
	// but don't allow for too blurry textures, refuse to downscale 256^2px textures
	if (mipmap && width >= 256 && height >= 256)
	{
		scaled_width = width;
		scaled_height = height;

		scaled_width >>= (int)r_picmip->value;
		scaled_height >>= (int)r_picmip->value;
		
		if (scaled_width < 256)
			scaled_width = 256;
		if (scaled_height < 256)
			scaled_height = 256;

		upload_width = scaled_width;
		upload_height = scaled_height;
	}
	else
	{
		upload_width = width;
		upload_height = height;
	}

	// scan the texture for any non-255 alpha
	c = width * height;
	scan = ((byte*)data) + 3;
	samples = gl_solid_format;
	for (i = 0; i < c; i++, scan += 4)
	{
		if (*scan != 255)
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if (samples == gl_solid_format) 
	{
		comp = gl_tex_solid_format; // rgb
	}
	else if (samples == gl_alpha_format)
	{
		comp = gl_tex_alpha_format; // rgba
	}
	else // weirdo
	{
		ri.Error(ERR_FATAL, "Texture \"%s\" is not RGB or RGBA (unknown number of texture components %i)\n", upload_name, samples);
		comp = samples;
	}

	if (upload_width != width || upload_height != height)
	{
		upload_width = width;
		upload_height = height;
		glTexImage2D(GL_TEXTURE_2D, 0, comp, upload_width, upload_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
#if 0
		byte* scaled = malloc(sizeof(byte) * upload_width * upload_height * comp);
		if (!scaled)
			ri.Error(ERR_FATAL, "malloc failed\n");

		R_ResampleTexture(data, width, height, scaled, upload_width, upload_height); // this wil crash with too large textures
		glTexImage2D(GL_TEXTURE_2D, 0, comp, upload_width, upload_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
		free(scaled);
#endif
	}
	else
		glTexImage2D(GL_TEXTURE_2D, 0, comp, upload_width, upload_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	if (mipmap)
	{
		//glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST); // FIXME: this should be done once in r_init
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	return (samples == gl_alpha_format);
}

/*
================
R_LoadTexture

This is also used as an entry point for the generated "$particle" & "$default_texture"
================
*/
image_t *R_LoadTexture(char *name, byte *pixels, int width, int height, texType_t type, int bits)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->texnum)
			break;
	}

	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			ri.Error (ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		ri.Error (ERR_DROP, "R_LoadTexture: \"%s\" is too long", name);
	strcpy (image->name, name);

	image->registration_sequence = registration_sequence;

	upload_name = image->name;

	image->width = width;
	image->height = height;
	image->type = type;

	image->texnum = TEXNUM_IMAGES + (image - gltextures);

	R_BindTexture(image->texnum);
	image->has_alpha = R_UploadTexture32 ((unsigned *)pixels, width, height, (image->type != it_gui && image->type != it_sky) );
	image->upload_width = upload_width;		// after power of 2 and scales
	image->upload_height = upload_height;
	
	image->sl = 0;
	image->sh = 1;
	image->tl = 0;
	image->th = 1;

	return image;
}

/*
===============
R_FindTexture

Finds or loads the given image
===============
*/
image_t	*R_FindTexture(char *name, texType_t type)
{
	image_t	*image;
	int		i, len;
	byte	*pic;
	int		width, height;

	if (!name)
	{
		//	ri.Error (ERR_DROP, "R_FindTexture: NULL name");
		return NULL;
	}
	len = strlen(name);
	if (len<5)
		return NULL;	//	ri.Error (ERR_DROP, "R_FindTexture: bad name: %s", name);

	// look for it
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	//
	// load the pic from disk
	//
	pic = NULL;
	if (!strcmp(name+len-4, ".tga"))
	{
		R_LoadTGA (name, &pic, &width, &height);
		if (!pic)
		{
//			ri.Printf(PRINT_LOW, "R_FindTexture: couldn't load %s\n", name);
			return r_notexture;
		}
		image = R_LoadTexture (name, pic, width, height, type, 32);
	}
	else
	{
		ri.Printf(PRINT_LOW, "R_FindTexture: weird file %s\n", name);
		return r_notexture;
	}

	if (pic)
		free(pic);

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return R_FindTexture (name, it_model);
}


/*
================
R_FreeUnusedTextures

Any image that was not touched on this registration sequence will be freed.
================
*/
void R_FreeUnusedTextures()
{
	int		i;
	image_t	*image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_gui)
			continue;		// don't free pics
		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

/*
===============
R_InitTextures
===============
*/
void R_InitTextures()
{
	registration_sequence = 1;
}

/*
===============
R_FreeTextures

Frees all textures, usualy on shutdown
===============
*/
void R_FreeTextures()
{
	int		i;
	image_t	*image;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot

		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

