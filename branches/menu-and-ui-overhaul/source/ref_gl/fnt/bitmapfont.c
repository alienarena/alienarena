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
/** @file
 * @brief FNT_BitmapFont class implementation
 * @note Initially generated from ref_gl/fnt/bitmapfont.cdf
 */

#include "ref_gl/fnt/bitmapfont.h"
#include "ref_gl/fnt/bitmapface.h"


/* Declarations of method implementations */
static void _FNT_BitmapFont_Load( FNT_Font object );
static void _FNT_BitmapFont_Unload( FNT_Font object );


/** @brief FNT_BitmapFont class definition */
static struct FNT_BitmapFont_cs _FNT_BitmapFont_class;
/** @brief FNT_BitmapFont class definition pointer */
static OOL_Class _FNT_BitmapFont_cptr = NULL;


/* Class-defining function; documentation in ref_gl/fnt/bitmapfont.h */
OOL_Class FNT_BitmapFont__Class( )
{
	if( _FNT_BitmapFont_cptr != NULL ) {
		return _FNT_BitmapFont_cptr;
	}

	_FNT_BitmapFont_cptr = (OOL_Class) & _FNT_BitmapFont_class;
	OOL_InitClassBasics( _FNT_BitmapFont_class , FNT_BitmapFont , FNT_Font );

	// Set virtual method pointers
	OOL_SetMethod( FNT_BitmapFont , FNT_Font , Load );
	OOL_SetMethod( FNT_BitmapFont , FNT_Font , Unload );

	return _FNT_BitmapFont_cptr;
}


/** @brief Bitmap font loader
 *
 * The bitmap font loader generates the GL lists and initialises the
 * font's information. It fails if the GL lists cannot be allocated.
 */
static void _FNT_BitmapFont_Load( FNT_Font object )
{
	FNT_BitmapFace face;
	unsigned int list_base;
	int i;

	// Initialise GL lists
	//
	// XXX: make sure there's no pending error first; this should not be
	// needed, but since the rest of the GL code doesn't check for errors
	// that often...
	qglGetError( );
	list_base = object->gl_lists = qglGenLists( 256 );
	if ( qglGetError( ) != GL_NO_ERROR ) {
		Com_Printf( "FNT_BitmapFont: could not create OpenGL lists@n" );
		return;
	}

	// Set up the rest of the font's data
	face = OOL_Cast( object->face , FNT_BitmapFace );
	object->texture = face->texture;
	object->first_character = 0;
	object->num_characters = 256;
	object->fixed_width = object->size;
	object->base_line = object->size;
	object->height = object->size;

	// Set up GL lists from the font
	for ( i = 0 ; i < 256 ; i ++ ) {
		unsigned int	row , col;

		// Grid coordinates
		row = i >> 4;
		col = i & 0xf;

		// Initialise list
		qglNewList( list_base , GL_COMPILE );
		qglBegin( GL_QUADS );
		qglTexCoord2f( (float)( col * object->texture->width / 8 ) , (float)( row * object->texture->height / 8 ) );
		qglVertex2f( 0 , 0 );
		qglTexCoord2f( (float)( ( col + 1 ) * object->texture->width / 8 ) , (float)( row * object->texture->height / 8 ) );
		qglVertex2f( object->size , 0 );
		qglTexCoord2f( (float)( ( col + 1 ) * object->texture->width / 8 ) , (float)( ( row + 1 ) * object->texture->height / 8 ) );
		qglVertex2f( object->size , object->size );
		qglTexCoord2f( (float)( col * object->texture->width / 8 ) , (float)( ( row + 1 ) * object->texture->height / 8 ) );
		qglVertex2f( 0 , object->size );
		qglEnd( );
		qglEndList( );

		list_base ++;
	}
}


/** @brief Bitmap font unloader
 *
 * The unloader for bitmap fonts simply clears the GL lists.
 */
static void _FNT_BitmapFont_Unload( FNT_Font object )
{
	qglDeleteLists( object->gl_lists , 256 );
}
