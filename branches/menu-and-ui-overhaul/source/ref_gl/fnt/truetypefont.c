/*
Copyright (C) 2010 COR Entertainment, LLC.

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
/** \file
 * \brief FNT_TrueTypeFont class implementation
 * \note Initially generated from ref_gl/fnt/truetypefont.cdf
 */

#include "ref_gl/fnt/truetypefont.h"
#include "ref_gl/fnt/truetypeface.h"


#include <ft2build.h>
#include FT_FREETYPE_H

/** \brief TrueType texture width
 *
 * Width of the textures used to store TTF fonts. Should be greater than 64
 * to prevent TTF textures from being uploaded to the scrap buffer.
 */
#define TTF_TEXTURE_WIDTH 512

/** \brief Supported TTF characters
 *
 * Amount of TTF characters to draw, starting from and including space.
 */
#define TTF_CHARACTERS	96

/* Declarations of method implementations */
static qboolean _FNT_TrueTypeFont_LoadFace( FNT_TrueTypeFont object,  FT_Face * face );
static unsigned int _FNT_TrueTypeFont_ComputeSizeInfo( FNT_TrueTypeFont object,  FT_Face face,  unsigned int * glyphs );
static qboolean _FNT_TrueTypeFont_GetKerningInfo( FNT_TrueTypeFont object,  FT_Face face,  unsigned int * glyphs );
static qboolean _FNT_TrueTypeFont_GetRenderData( FNT_TrueTypeFont object,  FT_Face face,  unsigned int * glyphs,  unsigned int n_lines );
static void _FNT_TrueTypeFont_Load( FNT_Font object );
static void _FNT_TrueTypeFont_Unload( FNT_Font object );


/** \brief FNT_TrueTypeFont class definition */
static struct FNT_TrueTypeFont_cs _FNT_TrueTypeFont_class;
/** \brief FNT_TrueTypeFont class definition pointer */
static OOL_Class _FNT_TrueTypeFont_cptr = NULL;


/* Class-defining function; documentation in ref_gl/fnt/truetypefont.h */
OOL_Class FNT_TrueTypeFont__Class( )
{
	if( _FNT_TrueTypeFont_cptr != NULL ) {
		return _FNT_TrueTypeFont_cptr;
	}

	_FNT_TrueTypeFont_cptr = (OOL_Class) & _FNT_TrueTypeFont_class;
	OOL_InitClassBasics( _FNT_TrueTypeFont_class , FNT_TrueTypeFont , FNT_Font );

	// Set virtual method pointers
	OOL_SetMethod( FNT_TrueTypeFont , FNT_Font , Load );
	OOL_SetMethod( FNT_TrueTypeFont , FNT_Font , Unload );

	return _FNT_TrueTypeFont_cptr;
}


/** \brief Load a TrueType face
 *
 * Load the data from the TrueType file's contents and set rendering
 * information.
 *
 * \param face pointer to the FreeType face handle
 *
 * \return true on success, false on error
 */
static qboolean _FNT_TrueTypeFont_LoadFace( FNT_TrueTypeFont object,  FT_Face * face )
{
	FT_Error error;
	FNT_TrueTypeFace tt_face;

	tt_face = OOL_Cast( OOL_Field( object , FNT_Font , face ) , FNT_TrueTypeFace );

	// Load TTF face information
	error = FT_New_Memory_Face( OOL_GetClassAs( tt_face , FNT_TrueTypeFace )->library ,
				tt_face->file_data , tt_face->file_size , 0 , face );
	if ( error != 0 ) {
		Com_Printf( "FNT_TrueTypeFont: failed to load font face '%s' (error code %d)\n" ,
				OOL_Field( tt_face , FNT_FontFace , name ) , error );
		return false;
	}

	// Sets the font's size
	error = FT_Set_Pixel_Sizes( *face , OOL_Field( object , FNT_Font , size ) , 0 );
	if ( error != 0 ) {
		Com_Printf( "FNT_TrueTypeFont: could not set size %lu to font '%s' (error code %d)\n" ,
			OOL_Field( object , FNT_Font , size ) , OOL_Field( tt_face , FNT_FontFace , name ) , error );
		FT_Done_Face( *face );
		return false;
	}

	// Set rendering mode
	object->_render_mode = FT_LOAD_RENDER;
	if ( OOL_GetClassAs( tt_face , FNT_TrueTypeFace )->autohint->integer ) {
		object->_render_mode |=  FT_LOAD_FORCE_AUTOHINT;
	}

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	if ( OOL_GetClassAs( tt_face , FNT_TrueTypeFace )->subpixel->integer ) {
		object->_render_mode |= FT_LOAD_TARGET_LCD;
	} else {
		object->_render_mode |= FT_LOAD_TARGET_NORMAL;
	}
#else //FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	object->_render_mode |= FT_LOAD_TARGET_NORMAL;
#endif //FT_CONFIG_OPTION_SUBPIXEL_RENDERING

	return true;
}


/** \brief Compute the face's size information
 *
 * Compute information such as total height, width or widths, and amount
 * of lines on the texture.
 *
 * \param face the FreeType face handle
 * \param glyphs the uninitialised glyph array
 *
 * \return the amount of lines on the texture or 0 on error.
 */
static unsigned int _FNT_TrueTypeFont_ComputeSizeInfo( FNT_TrueTypeFont object,  FT_Face face,  unsigned int * glyphs )
{
	unsigned int n_lines , max_width , x , i;
	int * w_buffer;
	int max_ascent , max_descent;
	FT_Error error;

	// Allocate array of widths if necessary
	if ( OOL_Field( object , FNT_Font , force_fixed ) ) {
		w_buffer = NULL;
	} else {
		w_buffer = Z_Malloc( TTF_CHARACTERS * sizeof( int ) );
		if ( w_buffer == NULL ) {
			Com_Printf( "FNT_TrueTypeFont: out of memory\n" );
			return false;
		}
	}

	// Get size information
	x = TTF_TEXTURE_WIDTH;
	n_lines = 1;
	max_ascent = max_descent = 0;
	max_width = 0;
	for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
		unsigned int w;
		int temp;

		// Load the character
		glyphs[ i ] = FT_Get_Char_Index( face , i + ' ' );
		error = FT_Load_Glyph( face , glyphs[ i ] , object->_render_mode );
		if ( error != 0 ) {
			Com_Printf( "FNT_TrueTypeFont: could not load character '%c' from font '%s' (error code %d)\n" ,
				i + ' ' , OOL_Field( OOL_Field( object , FNT_Font , face ) , FNT_FontFace , name ) , error );
			if ( w_buffer ) {
				Z_Free( w_buffer );
			}
			return 0;
		}

		// Get horizontal advance
		w = face->glyph->metrics.horiAdvance >> 6;
		if ( ( face->glyph->metrics.horiAdvance & 0x20 ) != 0 ) {
			w ++;
		}
		if ( w_buffer ) {
			w_buffer[ i ] = w;
		} else if ( w > max_width ) {
			max_width = w;
		}

		// Now get the "real" width and check if the line is full
		temp = face->glyph->bitmap.width;
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
		if ( OOL_GetClassAs( OOL_Field( object , FNT_Font , face ) , FNT_TrueTypeFace )->subpixel->integer ) {
			temp /= 3;
		}
#endif //FT_CONFIG_OPTION_SUBPIXEL_RENDERING

		if ( temp > x ) {
			n_lines ++;
			x = TTF_TEXTURE_WIDTH;
		}
		x -= temp;

		// Update max ascent / descent
		if ( face->glyph->bitmap_top > max_ascent ) {
			max_ascent = face->glyph->bitmap_top;
		}
		temp = 1 + face->glyph->bitmap.rows - face->glyph->bitmap_top;
		if ( temp > max_descent ) {
			max_descent = temp;
		}
	}

	OOL_Field( object , FNT_Font , height ) = max_ascent + max_descent;
	OOL_Field( object , FNT_Font , base_line ) = max_ascent;
	OOL_Field( object , FNT_Font , fixed_width ) = max_width;
	OOL_Field( object , FNT_Font , widths ) = w_buffer;

	return n_lines;
}


/** \brief Load kerning information
 *
 * If the font supports kerning and is not being forced to fixed width,
 * load kerning values for all supported glyphs.
 *
 * \param face the FreeType face handle
 * \param glyphs the glyph array
 *
 * \return true on success, false on failure
 */
static qboolean _FNT_TrueTypeFont_GetKerningInfo( FNT_TrueTypeFont object,  FT_Face face,  unsigned int * glyphs )
{
	int * kerning;
	int i , j;

	if ( OOL_Field( object , FNT_Font , force_fixed ) || ! FT_HAS_KERNING( face ) ) {
		return true;
	}

	kerning = Z_Malloc( sizeof( int ) * TTF_CHARACTERS * TTF_CHARACTERS );
	if ( kerning == NULL ) {
		Com_Printf( "FNT_TrueTypeFont: out of memory\n" );
		return false;
	}

	for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
		for ( j = 0 ; j < TTF_CHARACTERS ; j ++ ) {
			FT_Vector kvec;
			FT_Get_Kerning( face , glyphs[ i ] , glyphs[ j ] , FT_KERNING_DEFAULT , &kvec );
			kerning[ i * TTF_CHARACTERS + j ] = kvec.x >> 6;
		}
	}

	OOL_Field( object , FNT_Font , kerning ) = kerning;
	return true;
}


/** \brief Initialise rendering data
 *
 * Initialise the bitmap, texture and GL lists that will be used to render
 * the font.
 *
 * \param face the FreeType face handle
 * \param glyphs the glyph array
 * \param n_lines the amount of lines on the texture, as returned by
 * <b>FNT_TrueTypeFont::ComputeSizeInfo</b>
 *
 * \return true on success, false on failure
 */
static qboolean _FNT_TrueTypeFont_GetRenderData( FNT_TrueTypeFont object,  FT_Face face,  unsigned int * glyphs,  unsigned int n_lines )
{
	unsigned char * bitmap;
	unsigned int texture_height;
	unsigned int list_id , x , y;
	image_t * texture;
	int max_descent , i;

	// Compute texture height
	texture_height = OOL_Field( object , FNT_Font , height ) * n_lines;
	for ( i = 16 ; i <= 4096 ; i <<= 1 ) {
		if ( i > texture_height ) {
			texture_height = i;
			break;
		}
	}
	if ( i > 4096 ) {
		Com_Printf( "FNT_TrueTypeFont: font too big for texture\n" );
		return false;
	}

	// Allocate texture
	bitmap = Z_Malloc( TTF_TEXTURE_WIDTH * texture_height * 4 );
	if ( bitmap == NULL ) {
		Com_Printf( "FNT_TrueTypeFont: out of memory\n" );
		return false;
	}
	memset( bitmap , 0 , TTF_TEXTURE_WIDTH * texture_height * 4 );

	// Initialise GL lists
	//
	// XXX: make sure there's no pending error first; this should not be
	// needed, but since the rest of the GL code doesn't check for errors
	// that often...
	qglGetError( );
	list_id = qglGenLists( TTF_CHARACTERS );
	if ( qglGetError( ) != GL_NO_ERROR ) {
		Com_Printf( "FNT_TrueTypeFont: could not create OpenGL lists\n" );
		Z_Free( bitmap );
		return false;
	}

	// Draw glyphs on texture
	x = 0;
	y = OOL_Field( object , FNT_Font , base_line );
	max_descent = OOL_Field( object , FNT_Font , height ) - OOL_Field( object , FNT_Font , base_line );
	for ( i = 0 ; i < TTF_CHARACTERS ; i ++ ) {
		unsigned int rWidth , tx , ty;
		unsigned char * bptr , * fptr;
		FT_Error error;

		// Render the character
		error = FT_Load_Glyph( face , glyphs[ i ] , object->_render_mode );
		if ( error != 0 ) {
			Com_Printf( "TTF: could not load character '%c' from font '%s' (error code %d)\n" ,
				i + ' ' ,  OOL_Field( OOL_Field( object , FNT_Font , face ) , FNT_FontFace , name ) , error );
			Z_Free( bitmap );
			qglDeleteLists( list_id , TTF_CHARACTERS );
			return false;
		}

		// Compute actual width
		rWidth = face->glyph->bitmap.width;
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
		if ( OOL_GetClassAs( OOL_Field( object , FNT_Font , face ) , FNT_TrueTypeFace )->subpixel->integer ) {
			rWidth /= 3;
		}
#endif //FT_CONFIG_OPTION_SUBPIXEL_RENDERING

		// Wrap current line if needed
		if ( rWidth > TTF_TEXTURE_WIDTH - x ) {
			x = 0;
			y += OOL_Field( object , FNT_Font , height );
		}

		// Create GL display list
		qglNewList( list_id + i , GL_COMPILE );
		qglBegin( GL_QUADS );
		qglTexCoord2i( x , y - OOL_Field( object , FNT_Font , base_line ) );
		qglVertex2i( 0 , 0 );
		qglTexCoord2i( x + rWidth , y - OOL_Field( object , FNT_Font , base_line ) );
		qglVertex2i( rWidth , 0 );
		qglTexCoord2i( x + rWidth , y + max_descent );
		qglVertex2i( rWidth , OOL_Field( object , FNT_Font , height ) );
		qglTexCoord2i( x , y + max_descent );
		qglVertex2i( 0 , OOL_Field( object , FNT_Font , height ) );
		qglEnd( );
		qglEndList( );

		// Copy glyph image to bitmap
		bptr = bitmap + ( x + ( y - face->glyph->bitmap_top ) * TTF_TEXTURE_WIDTH ) * 4;
		fptr = face->glyph->bitmap.buffer;
		for ( ty = 0 ; ty < face->glyph->bitmap.rows ; ty ++ ) {
			unsigned char * rbptr = bptr;

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
			if ( OOL_GetClassAs( OOL_Field( object , FNT_Font , face ) , FNT_TrueTypeFace )->subpixel->integer ) {
				for ( tx = 0 ; tx < face->glyph->bitmap.width / 3 ; tx ++ , rbptr += 4 , fptr += 3 ) {
					rbptr[0] = fptr[0];
					rbptr[1] = fptr[1];
					rbptr[2] = fptr[2];
					rbptr[3] = fptr[0] / 3 + fptr[1] / 3 + fptr[2] / 3;
				}
			} else {
#endif //FT_CONFIG_OPTION_SUBPIXEL_RENDERING
				for ( tx = 0 ; tx < face->glyph->bitmap.width ; tx ++ , rbptr += 4 , fptr ++ ) {
					rbptr[0] = 255;
					rbptr[1] = 255;
					rbptr[2] = 255;
					rbptr[3] = *fptr;
				}
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
			}
#endif //FT_CONFIG_OPTION_SUBPIXEL_RENDERING

			bptr += TTF_TEXTURE_WIDTH * 4;
			fptr += face->glyph->bitmap.pitch - face->glyph->bitmap.width;
		}

		// Update current X
		x += rWidth;
	}

	// Upload the texture
	// FIXME: if GL_LoadPic fails, we just wasted some memory
	texture = GL_LoadPic( OOL_Field( object , FNT_Font , font_key ) ,
				bitmap , TTF_TEXTURE_WIDTH , texture_height ,
				it_pic , 32 );
	if ( ! texture ) {
		Z_Free( bitmap );
		qglDeleteLists( list_id , TTF_CHARACTERS );
		return false;
	}

	object->_bitmap = bitmap;
	OOL_Field( object , FNT_Font , texture ) = texture;
	OOL_Field( object , FNT_Font , gl_lists ) = list_id;

	return true;
}


/** \brief TrueType font loader
 *
 * Attempt to load the TrueType face's information, compute font metrics,
 * including kerning if available, and generate the font's texture and
 * GL lists.
 */
static void _FNT_TrueTypeFont_Load( FNT_Font object )
{
	FNT_TrueTypeFont self = OOL_Cast( object , FNT_TrueTypeFont );
	unsigned int glyphs[ TTF_CHARACTERS ];
	unsigned int n_lines;
	qboolean ok;
	FT_Face ft_face;

	// Load face
	if ( ! _FNT_TrueTypeFont_LoadFace( self , &ft_face ) ) {
		return;
	}

	// Compute size information, get kerning data, generate rendering information
	n_lines = _FNT_TrueTypeFont_ComputeSizeInfo( self , ft_face , glyphs );
	ok = false;
	if ( n_lines != 0 ) {
		if ( _FNT_TrueTypeFont_GetKerningInfo( self , ft_face , glyphs ) ) {
			ok = _FNT_TrueTypeFont_GetRenderData( self , ft_face , glyphs , n_lines );
		}
	}

	if ( !ok ) {
		// Free memory on failure
		if ( object->widths ) {
			Z_Free( object->widths );
		}
		if ( object->kerning ) {
			Z_Free( object->kerning );
		}
	} else {
		object->first_character = ' ';
		object->num_characters = TTF_CHARACTERS;
	}

	// Free FreeType face
	FT_Done_Face( ft_face );
}


/** \brief TrueType font unloader
 *
 * Free width and kerning arrays, font bitmap and texture, and GL lists.
 */
static void _FNT_TrueTypeFont_Unload( FNT_Font object )
{
	if ( object->widths ) {
		Z_Free( object->widths );
	}
	if ( object->kerning ) {
		Z_Free( object->kerning );
	}

	GL_FreeImage( object->texture );
	qglDeleteLists( object->gl_lists , TTF_CHARACTERS );
	Z_Free( OOL_Field( object , FNT_TrueTypeFont , _bitmap ) );
}
