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
 * \brief FNT_BitmapFace class implementation
 * \note Initially generated from ref_gl/fnt/bitmapface.cdf
 */

#include "ref_gl/fnt/bitmapface.h"
#include "ref_gl/fnt/bitmapfont.h"


/* Declarations of method implementations */
static qboolean _FNT_BitmapFace_Load( FNT_FontFace object );
static void _FNT_BitmapFace_Unload( FNT_FontFace object );


/** \brief FNT_BitmapFace class definition */
static struct FNT_BitmapFace_cs _FNT_BitmapFace_class;
/** \brief FNT_BitmapFace class definition pointer */
static OOL_Class _FNT_BitmapFace_cptr = NULL;


/* Class-defining function; documentation in ref_gl/fnt/bitmapface.h */
OOL_Class FNT_BitmapFace__Class( )
{
	if( _FNT_BitmapFace_cptr != NULL ) {
		return _FNT_BitmapFace_cptr;
	}

	_FNT_BitmapFace_cptr = (OOL_Class) & _FNT_BitmapFace_class;
	OOL_InitClassBasics( _FNT_BitmapFace_class , FNT_BitmapFace , FNT_FontFace );

	// Set virtual method pointers
	OOL_SetMethod( FNT_BitmapFace , FNT_FontFace , Load );
	OOL_SetMethod( FNT_BitmapFace , FNT_FontFace , Unload );

	return _FNT_BitmapFace_cptr;
}


/** \brief Bitmap face loader
 *
 * Attempt to load the bitmap face's texture from the face's full
 * path.
 *
 * \return true if the texture was loaded, false otherwise.
 */
static qboolean _FNT_BitmapFace_Load( FNT_FontFace object )
{
	image_t * texture;
	char file_name[ FNT_FACE_NAME_LEN + MAX_OSPATH + 6 ];

	Com_sprintf( file_name , sizeof( file_name ) , "%s.tga" , object->full_name );
	texture = GL_FindImage( file_name , it_pic );
	if ( texture != NULL ) {
		OOL_Field( object , FNT_BitmapFace , texture ) = texture;
		object->font_class = FNT_BitmapFont__Class( );
	}
	return ( texture != NULL );
}


/** \brief Bitmap face unloader
 *
 * Free the bitmap face's texture.
 */
static void _FNT_BitmapFace_Unload( FNT_FontFace object )
{
	GL_FreeImage( OOL_Field( object , FNT_BitmapFace , texture ) );
}
