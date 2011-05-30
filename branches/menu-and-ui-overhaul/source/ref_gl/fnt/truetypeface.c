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
 * @brief FNT_TrueTypeFace class implementation
 * @note Initially generated from ref_gl/fnt/truetypeface.cdf
 */

#include "ref_gl/fnt/truetypeface.h"
#include "ref_gl/fnt/truetypefont.h"

#ifdef FT_LCD_FILTER_H
#include FT_LCD_FILTER_H
#endif

/* Declarations of method implementations */
static qboolean _FNT_TrueTypeFace_InitialiseLibrary( );
static void _FNT_TrueTypeFace_DestroyLibrary( );
static qboolean _FNT_TrueTypeFace_Load( FNT_FontFace object );
static void _FNT_TrueTypeFace_Unload( FNT_FontFace object );

/** @brief TrueType face counter
 *
 * This counter stores the amount of loaded TrueType faces. It is used
 * to initialise and free the library.
 */
static unsigned int _FNT_TrueTypeFace_counter;


/** @brief FNT_TrueTypeFace class definition */
static struct FNT_TrueTypeFace_cs _FNT_TrueTypeFace_class;
/** @brief FNT_TrueTypeFace class definition pointer */
static OOL_Class _FNT_TrueTypeFace_cptr = NULL;


/* Class-defining function; documentation in ref_gl/fnt/truetypeface.h */
OOL_Class FNT_TrueTypeFace__Class( )
{
	if( _FNT_TrueTypeFace_cptr != NULL ) {
		return _FNT_TrueTypeFace_cptr;
	}

	_FNT_TrueTypeFace_cptr = (OOL_Class) & _FNT_TrueTypeFace_class;
	OOL_InitClassBasics( _FNT_TrueTypeFace_class , FNT_TrueTypeFace , FNT_FontFace );

	// Set virtual method pointers
	OOL_SetMethod( FNT_TrueTypeFace , FNT_FontFace , Load );
	OOL_SetMethod( FNT_TrueTypeFace , FNT_FontFace , Unload );

	// Initialise static fields
	_FNT_TrueTypeFace_counter = 0;
	_FNT_TrueTypeFace_class.library = 0;
	_FNT_TrueTypeFace_class.subpixel = Cvar_Get( "ttf_subpixel" , "0" , CVAR_ARCHIVE );
	_FNT_TrueTypeFace_class.autohint = Cvar_Get( "ttf_autohint" , "0" , CVAR_ARCHIVE );

	return _FNT_TrueTypeFace_cptr;
}


/** @brief FreeType library initialisation
 *
 * Initialise the FreeType library, setting sub-pixel rendering mode if
 * required.
 *
 * @return true on success, false on failure.
 */
static qboolean _FNT_TrueTypeFace_InitialiseLibrary( )
{
	FT_Error error;

	// Initialise FreeType
	error = FT_Init_FreeType( &_FNT_TrueTypeFace_class.library );
	if ( error != 0 ) {
		Com_Printf( "FNT_TrueTypeFace: unable to initialise library (error code %d)@n" , error );
		return false;
	}

#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING
	// Attempt to initialise sub-pixel rendering
	switch ( _FNT_TrueTypeFace_class.subpixel->integer ) {
		case 1:
			FT_Library_SetLcdFilter( _FNT_TrueTypeFace_class.library , FT_LCD_FILTER_NONE );
			break;
		case 2:
			FT_Library_SetLcdFilter( _FNT_TrueTypeFace_class.library , FT_LCD_FILTER_DEFAULT );
			break;
		case 3:
			FT_Library_SetLcdFilter( _FNT_TrueTypeFace_class.library , FT_LCD_FILTER_LIGHT );
			break;
	}
#endif //FT_CONFIG_OPTION_SUBPIXEL_RENDERING

	return true;
}


/** @brief FreeType library destruction
 *
 * Free resources used by the FreeType library.
 */
static void _FNT_TrueTypeFace_DestroyLibrary( )
{
	FT_Done_FreeType( _FNT_TrueTypeFace_class.library );
}


/** @brief TrueType face loader
 *
 * Check if the library needs to be initialised, and do so if necessary.
 * Then attempt to load the specified font; on success, make sure it can
 * be scaled. Finally, increase the counter.
 *
 * @return true on success, or false if any of the steps fails.
 */
static qboolean _FNT_TrueTypeFace_Load( FNT_FontFace object )
{
	char file_name[ FNT_FACE_NAME_LEN + MAX_OSPATH + 6 ];
	FT_Byte * file_data;
	unsigned int data_size;
	FT_Face ft_face;
	FT_Error error;
	qboolean ok;

	// Attempt to load the font's file
	Com_sprintf( file_name , sizeof( file_name ) , "%s.ttf" , object->full_name );
	data_size = FS_LoadFile( file_name , (void **)&file_data );
	if ( !file_data ) {
		return false;
	}

	// Initialise library
	if ( _FNT_TrueTypeFace_counter == 0 ) {
		if ( ! _FNT_TrueTypeFace_InitialiseLibrary( ) ) {
			FS_FreeFile( file_data );
			return false;
		}
	}

	// Load TTF face information
	error = FT_New_Memory_Face( _FNT_TrueTypeFace_class.library , file_data , data_size , 0 , &ft_face );
	if ( error != 0 ) {
		Com_Printf( "FNT_TrueTypeFace: failed to load font from '%s' (error code %d)@n" , file_name , error );
		FS_FreeFile( file_data );
		return false;
	}

	// Check font information
	if ( (ft_face->face_flags & FT_FACE_FLAG_SCALABLE) && (ft_face->face_flags & FT_FACE_FLAG_HORIZONTAL) ) {
		ok = true;
	} else {
		Com_Printf( "FNT_TrueTypeFace: font loaded from '%s' is not scalable@n" , file_name );
		ok = false;
	}
	FT_Done_Face( ft_face );

	if ( ! ok ) {
		if ( _FNT_TrueTypeFace_counter == 0 ) {
			_FNT_TrueTypeFace_DestroyLibrary( );
		}
		FS_FreeFile( file_data );
	} else {
		_FNT_TrueTypeFace_counter ++;
		OOL_Field( object , FNT_TrueTypeFace , file_size ) = data_size;
		OOL_Field( object , FNT_TrueTypeFace , file_data ) = file_data;
		object->font_class = FNT_TrueTypeFont__Class( );
	}

	return ok;
}


/** @brief TrueType face unloader
 *
 * Free the memory used for the file's contents, then decrease the counter.
 * If the counter reached 0, destroy the FreeType library's instance.
 */
static void _FNT_TrueTypeFace_Unload( FNT_FontFace object )
{
	assert( _FNT_TrueTypeFace_counter != 0 );
	FS_FreeFile( OOL_Field( object , FNT_TrueTypeFace , file_data ) );
	-- _FNT_TrueTypeFace_counter;
	if ( _FNT_TrueTypeFace_counter == 0 ) {
		_FNT_TrueTypeFace_DestroyLibrary( );
	}
}
