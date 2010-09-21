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

/* r_ttf.c
 *
 * Displays text using TrueType fonts, loaded through the FreeType
 * library.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "r_local.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef FT_LCD_FILTER_H
#include FT_LCD_FILTER_H
#endif



/* Amount of TTF characters to draw, starting from and including space. */
#define TTF_CHARACTERS	96

/*
 * Width of the textures used to store TTF fonts. Should be greater than 64
 * to prevent TTF textures from being uploaded to the scrap buffer.
 */
#define TTF_TEXTURE_WIDTH (1 << 9)

/*
 * Font list
 */
struct _list_head_t
{
	struct _list_head_t *	previous;
	struct _list_head_t *	next;
};


/*
 * Structure representing a TrueType font's rendering information and data.
 */
struct ttf_font_s
{
	/* List entry */
	struct _list_head_t	ttf_list;

	/* Image used as a texture */
	image_t *		texture;

	/* Bitmap containing the font's characters */
	unsigned char *		bitmap;

	/* Width of each character */
	int			widths[ TTF_CHARACTERS ];

	/* "Kerning" - specific offset to use for sequences of characters */
	int			kerning[ TTF_CHARACTERS ][ TTF_CHARACTERS ];

	/* Height of the bitmap */
	int			height;

	/* GL list identifier */
	unsigned int		list_id;
};


/*
 * The TTF library instance.
 */
static FT_Library _ttf_library;


/*
 * The list of all allocated fonts.
 */
static struct _list_head_t _ttf_fonts;


/*
 * When debugging is not disabled, make sure the library is in the
 * right state before initialising or shutting down.
 */
#if !defined NDEBUG
static qboolean _ttf_initialised = false;
#endif // !defined NDEBUG


/* CVar to enable/disable sub-pixel rendering. */
static cvar_t * _ttf_subpixel;

/* CVar that controls autohinting. */
static cvar_t * _ttf_autohint;


/* Blending: additional colour extension */
static PFNGLBLENDCOLOREXTPROC _glBlendColorEXT;




/*
 * Initialise the FreeType library
 */
qboolean TTF_Init( )
{
	FT_Error error;

#if !defined NDEBUG
	if ( _ttf_initialised ) {
		Com_DPrintf( "TTF: library already initialised\n" );
		return false;
	}
#endif // !defined NDEBUG

	// Initialise FreeType
	error = FT_Init_FreeType( &_ttf_library );
	if ( error != 0 ) {
		Com_Printf( "TTF: unable to initialise library (error code %d)\n" , error );
		return false;
	}

	// Initialise list of fonts
	_ttf_fonts.previous = _ttf_fonts.next = &_ttf_fonts;

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	// Access CVar for sub-pixel rendering
	_ttf_subpixel = Cvar_Get( "ttf_subpixel" , "0" , CVAR_ARCHIVE );
	_ttf_subpixel->modified = false;

	// Attempt to initialise sub-pixel rendering
	switch ( _ttf_subpixel->integer ) {
		case 0:
			break;
		case 1:
			FT_Library_SetLcdFilter( _ttf_library , FT_LCD_FILTER_NONE );
			break;
		case 2:
			FT_Library_SetLcdFilter( _ttf_library , FT_LCD_FILTER_DEFAULT );
			break;
		case 3:
			FT_Library_SetLcdFilter( _ttf_library , FT_LCD_FILTER_LIGHT );
			break;
	}
#else // FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	_ttf_subpixel = NULL;
#endif // FT_CONFIG_OPTION_SUBPIXEL_RENDERING

	// Access CVar for autohinting
	_ttf_autohint = Cvar_Get( "ttf_autohint" , "1" , CVAR_ARCHIVE );
	_ttf_autohint->modified = false;

	// Access blending function, if available
	_glBlendColorEXT = (PFNGLBLENDCOLOREXTPROC) qwglGetProcAddress( "glBlendColorEXT" );

#if !defined NDEBUG
	Com_Printf( "...initialised TTF engine\n" );
	_ttf_initialised = true;
#endif // !defined NDEBUG
	return true;
}


/*
 * Free all TTF fonts.
 */
void _TTF_Clear( )
{
	struct _list_head_t *	entry;

	entry = _ttf_fonts.next;
	while ( entry != &_ttf_fonts ) {
		struct ttf_font_s * font = (struct ttf_font_s *)entry;
		entry = entry->next;

		GL_FreeImage( font->texture );
		qglDeleteLists( font->list_id , TTF_CHARACTERS );
		Z_Free( font->bitmap );
		Z_Free( font );
	}

	_ttf_fonts.previous = _ttf_fonts.next = &_ttf_fonts;
}


/*
 * Shutdown the FreeType library
 */
void TTF_Shutdown( )
{
#if !defined NDEBUG
	if ( ! _ttf_initialised ) {
		Com_DPrintf( "TTF: library is not initialised\n" );
		return;
	}
	Com_Printf( "...shutting down TTF engine\n" );
	_ttf_initialised = false;
#endif // !defined NDEBUG
	
	_TTF_Clear( );
	FT_Done_FreeType( _ttf_library );
}



/*
 * Initialise a TrueType font from a file, creating the texture and GL list.
 */
static struct ttf_font_s * _TTF_Load( char * file , const char * font_name , unsigned int font_size )
{
	struct ttf_font_s *	font;
	FT_Byte *		data;
	unsigned int		data_size;
	FT_Face			face;
	FT_Error		error;
	unsigned int		glyphs[ TTF_CHARACTERS ];
	int			max_ascent , max_descent;
	unsigned int		x , y;
	unsigned int		n_lines;
	unsigned int		i;
	unsigned int		texture_height;
	unsigned int		render_mode;

#if !defined NDEBUG
	assert( _ttf_initialised );
	assert( font_size > 0 );
#endif // !defined NDEBUG

	// Attempt to load the font's file
	data_size = FS_LoadFile( file , (void **)&data );
	if ( !data_size ) {
		return NULL;
	}

	// Allocate memory for the font itself
	font = (struct ttf_font_s *) Z_Malloc( sizeof( struct ttf_font_s ) );
	assert( font != NULL );

	// Attempt to load the font and make sure it can be scaled
	error = FT_New_Memory_Face( _ttf_library , data , data_size , 0 , &face );
	if ( error != 0 ) {
		Com_Printf( "TTF: failed to load font from '%s' (error code %d)\n" , file , error );
		goto _ttf_load_err_0;
	} else if ( ! ( (face->face_flags & FT_FACE_FLAG_SCALABLE)
			&& (face->face_flags & FT_FACE_FLAG_HORIZONTAL) ) ) {
		Com_Printf( "TTF: font loaded from '%s' is not scalable\n" , file );
		goto _ttf_load_err_1;
	}

	// Sets the font's size
	error = FT_Set_Pixel_Sizes( face , font_size , 0 );
	if ( error != 0 ) {
		Com_Printf( "TTF: could not set size %d to font loaded from '%s' (error code %d)\n" , font_size , file , error );
		goto _ttf_load_err_1;
	}

	// Set rendering mode
	render_mode = FT_LOAD_RENDER;
	if ( _ttf_autohint->integer )
		render_mode |=  FT_LOAD_FORCE_AUTOHINT;
	if ( _ttf_subpixel && _ttf_subpixel->integer ) {
		render_mode |= FT_LOAD_TARGET_LCD;
	} else {
		render_mode |= FT_LOAD_TARGET_NORMAL;
	}

	// Get size information
	x = TTF_TEXTURE_WIDTH;
	n_lines = 1;
	max_ascent = max_descent = 0;
	for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
		int temp;

		// Load the character
		glyphs[ i ] = FT_Get_Char_Index( face , i + ' ' );
		error = FT_Load_Glyph( face , glyphs[ i ] , render_mode );
		if ( error != 0 ) {
			Com_Printf( "TTF: could not load character '%c' from font '%s' (error code %d)\n" , i + ' ' , font_name , error );
			goto _ttf_load_err_1;
		}

		// Get width, check if the line is full
		temp = face->glyph->metrics.horiAdvance >> 6;
		if ( ( temp & 0x3f ) != 0 )
			temp ++;
		if ( temp > x ) {
			n_lines ++;
			x = TTF_TEXTURE_WIDTH;
		}
		x -= temp;
		font->widths[ i ] = temp;

		// Update max ascent / descent
		if ( face->glyph->bitmap_top > max_ascent )
			max_ascent = face->glyph->bitmap_top;
		temp = 1 + face->glyph->bitmap.rows - face->glyph->bitmap_top;
		if ( temp > max_descent )
			max_descent = temp;
	}
	font->height = max_ascent + max_descent;

	// Get kerning information
	if ( FT_HAS_KERNING( face ) ) {
		for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
			int j;
			for ( j = 0 ; j < TTF_CHARACTERS ; j ++ ) {
				FT_Vector kvec;
				FT_Get_Kerning( face , glyphs[ i ] , glyphs[ j ] , FT_KERNING_DEFAULT , &kvec );
				font->kerning[ i ][ j ] = kvec.x >> 6;
			}
		}
	} else {
		memset( font->kerning , 0 , sizeof( font->kerning ) );
	}

	// Compute texture height
	texture_height = ( max_ascent + max_descent) * n_lines;
	for ( i = 16 ; i <= 4096 ; i <<= 1 ) {
		if ( i > texture_height ) {
			texture_height = i;
			break;
		}
	}

	// Allocate texture and initialise GL list
	font->bitmap = Z_Malloc( TTF_TEXTURE_WIDTH * texture_height * 4 );
	assert( font->bitmap != NULL );
	memset( font->bitmap , 0 , TTF_TEXTURE_WIDTH * texture_height * 4 );
	font->list_id = qglGenLists( TTF_CHARACTERS );

	// Draw glyphs on texture
	x = 0;
	y = max_ascent;
	for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
		unsigned int tx , ty;
		unsigned char * bptr , * fptr;
		float x1 , x2 , y1 , y2;

		// Render the character
		error = FT_Load_Glyph( face , glyphs[ i ] , render_mode );
		if ( error != 0 ) {
			Com_Printf( "TTF: could not load character '%c' from font '%s' (error code %d)\n" , i + ' ' , font_name , error );
			goto _ttf_load_err_2;
		}

		// Wrap current line if needed
		if ( font->widths[ i ] > TTF_TEXTURE_WIDTH - x ) {
			x = 0;
			y += font->height;
		}

		// Compute texture coordinates
		x1 = (float)x / (float)TTF_TEXTURE_WIDTH;
		x2 = (float)( x + font->widths[ i ] ) / (float)TTF_TEXTURE_WIDTH;
		y1 = (float)( y - max_ascent ) / (float)texture_height;
		y2 = y1 + (float)( font->height ) / (float)texture_height;

		// Create GL display list
		qglNewList( font->list_id + i , GL_COMPILE );
		qglBegin( GL_QUADS );
		qglTexCoord2i( x , y - max_ascent );
		qglVertex2i( 0 , 0 );
		qglTexCoord2i( x + font->widths[ i ] , y - max_ascent );
		qglVertex2i( font->widths[ i ] , 0 );
		qglTexCoord2i( x + font->widths[ i ] , y + max_descent );
		qglVertex2i( font->widths[ i ] , font->height );
		qglTexCoord2i( x , y + max_descent );
		qglVertex2i( 0 , font->height );
		qglEnd( );
		qglEndList( );

		// Copy glyph image to bitmap
		bptr = font->bitmap + ( x + ( y - face->glyph->bitmap_top ) * TTF_TEXTURE_WIDTH ) * 4;
		fptr = face->glyph->bitmap.buffer;
		for ( ty = 0 ; ty < face->glyph->bitmap.rows ; ty ++ ) {
			unsigned char * rbptr = bptr;

			if ( _ttf_subpixel && _ttf_subpixel->integer ) {
				for ( tx = 0 ; tx < face->glyph->bitmap.width / 3 ; tx ++ , rbptr += 4 , fptr += 3 ) {
					rbptr[0] = fptr[0];
					rbptr[1] = fptr[1];
					rbptr[2] = fptr[2];
					rbptr[3] = fptr[0] / 3 + fptr[1] / 3 + fptr[2] / 3;
				}
			} else {
				for ( tx = 0 ; tx < face->glyph->bitmap.width ; tx ++ , rbptr += 4 , fptr ++ ) {
					unsigned char c = *fptr;
					rbptr[0] = c;
					rbptr[1] = c;
					rbptr[2] = c;
					rbptr[3] = c;
				}
			}

			bptr += TTF_TEXTURE_WIDTH * 4;
			fptr += face->glyph->bitmap.pitch - face->glyph->bitmap.width;
		}

		// Update current X
		x += font->widths[ i ];
	}

	// Upload the texture
	// FIXME: if GL_LoadPic fails, we just wasted some memory
	font->texture = GL_LoadPic( va( "***ttf*%s*%d***" , font_name , font_size ) ,
				font->bitmap , TTF_TEXTURE_WIDTH , texture_height ,
				it_pic , 32 );

	// Add the font to the list
	font->ttf_list.next = &_ttf_fonts;
	font->ttf_list.previous = _ttf_fonts.previous;
	_ttf_fonts.previous = font->ttf_list.previous->next = (struct _list_head_t *) font;

	FT_Done_Face( face );
	FS_FreeFile( data );
	return font;

_ttf_load_err_2:
	free( font->bitmap );
	qglDeleteLists( font->list_id , TTF_CHARACTERS );
_ttf_load_err_1:
	FT_Done_Face( face );
_ttf_load_err_0:
	FS_FreeFile( data );
	free( font );
	return NULL;
}


/*
 * Look up a TrueType font in the list
 */
static struct ttf_font_s * _TTF_Find( const char * font_name , unsigned int size )
{
	image_t *		image;
	struct _list_head_t *	entry;

	image = GL_GetImage( va( "***ttf*%s*%d***" , font_name , size ) );
	if ( !image )
		return NULL;

	entry = _ttf_fonts.next;
	while ( entry != &_ttf_fonts ) {
		if ( ( (struct ttf_font_s *) entry )->texture == image ) {
			return (struct ttf_font_s *) entry;
		}
		entry = entry->next;
	}

	return NULL;
}



/*
 * Load a TrueType font from a font name and a size.
 */
ttf_font_t TTF_GetFont( const char * font_name , unsigned int size )
{
	struct ttf_font_s *	font;
	char			fn_buffer[ MAX_QPATH ];

	// Check for TrueType configuration changes
	if ( _ttf_autohint->modified ) {
		_TTF_Clear( );
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	}
	if ( _ttf_subpixel->modified ) {
		_TTF_Clear( );
		switch ( _ttf_subpixel->integer ) {
			case 0:
				break;
			case 1:
				FT_Library_SetLcdFilter( _ttf_library , FT_LCD_FILTER_NONE );
				break;
			case 2:
				FT_Library_SetLcdFilter( _ttf_library , FT_LCD_FILTER_DEFAULT );
				break;
			case 3:
				FT_Library_SetLcdFilter( _ttf_library , FT_LCD_FILTER_LIGHT );
				break;
		}
#endif // FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	} else {
		// No change, try finding an existing font
		font = _TTF_Find( font_name , size );
		if ( font != NULL )
			return font;
	}

	// Reset Cvar modification flags
	_ttf_autohint->modified = false;
	if ( _ttf_subpixel )
		_ttf_subpixel->modified = false;

	// Load font
	Com_sprintf( fn_buffer , sizeof( fn_buffer ) , "fonts/%s.ttf" , font_name );
	return _TTF_Load( fn_buffer , font_name , size );
}


/*
 * Prepares the environment before drawing a string.
 */
static void _TTF_PrepareToDraw( image_t * texture )
{
	// Save current context
	qglPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT );
	qglMatrixMode( GL_MODELVIEW );
	qglPushMatrix( );
	qglMatrixMode( GL_TEXTURE );
	qglPushMatrix( );

	// Prepare texture
	GL_Bind( texture->texnum );
	GL_TexEnv( GL_MODULATE );
	qglLoadIdentity();
	qglScalef( 1.0 / texture->width , 1.0 / texture->height , 1 );

	// Set blending function
	if ( _glBlendColorEXT )
		qglBlendFunc( GL_CONSTANT_COLOR_EXT , GL_ONE_MINUS_SRC_COLOR );

	qglMatrixMode( GL_MODELVIEW );
}


/*
 * Restores the environment as if was before _TTF_PrepareToDraw was called
 */
static void _TTF_RestoreEnvironment( )
{
	qglMatrixMode( GL_TEXTURE );
	qglPopMatrix( );
	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix( );
	qglPopAttrib( );
}


/*
 * Sets the current drawing color.
 */
static void _TTF_SetColor( const float color[ 4 ] )
{
	if ( _glBlendColorEXT ) {
		_glBlendColorEXT( color[ 0 ] * color[ 3 ] , color[ 1 ] * color[ 3 ] , color[ 2 ] * color[ 3 ] , 1 ); 
		qglColor3f( color[ 0 ] , color[ 1 ] , color[ 2 ] );
	} else {
		qglColor4fv( color );
	}
}


/*
 * Draws a "raw" string using a TTF font.
 *
 * All control characters will be ignored.
 */
void TTF_RawPrint( ttf_font_t font , const char * text , float x , float y , const float color[4] )
{
	const unsigned char * ptr = ( const unsigned char *) text;
	int previous = -1;

	// Prepare to draw from the texture
	_TTF_PrepareToDraw( font->texture );

	// Enable blending
	_TTF_SetColor( color );

	// Draw text
	while ( *ptr ) {
		unsigned int i = *ptr - ' ';

		if ( i < TTF_CHARACTERS ) {
			if ( previous == -1 ) {
				qglTranslatef( x , y , 0 );
			} else {
				qglTranslatef( font->widths[ previous ] + font->kerning[ previous ][ i ] , 0 , 0 );
			}
			qglCallList( font->list_id + i );
			previous = i;
		}
		ptr ++;
	}

	// Restore environemnt
	_TTF_RestoreEnvironment( );
}



/*
 * Compute the size of a section of text.
 *
 * The text may contain \n's, and it uses \001 followed by three bytes to
 * indicate colour changes.
 */
void TTF_GetSize( ttf_font_t font , const char * text , unsigned int * width , unsigned int * height )
{
	const unsigned char *	ptr = (const unsigned char *) text;
	unsigned int		got_color = 0;
	int			previous = -1;

	*width = 0;
	*height = font->height;
	while ( *ptr ) {
		int ch = *ptr;

		if ( got_color ) {
			got_color = ( got_color + 1 ) % 4;
		} if ( ch == '\n' ) {
			*width = 0;
			*height += font->height;
		} else if ( ch == 1 ) {
			got_color = 1;
		} else if ( ch >= ' ' && ch < ' ' + TTF_CHARACTERS ) {
			ch -= ' ';
			*width += font->widths[ ch ];
			if ( previous != -1 ) {
				*width += font->kerning[ previous ][ ch ];
			}
			previous = ch;
		}

		ptr ++;
	}
}
