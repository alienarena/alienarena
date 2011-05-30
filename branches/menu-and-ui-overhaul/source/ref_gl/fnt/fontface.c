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
 * \brief FNT_FontFace class implementation
 * \note Initially generated from ref_gl/fnt/fontface.cdf
 */

#include "ref_gl/fnt/fontface.h"
#include "ref_gl/fnt/font.h"
#include "ref_gl/fnt/truetypeface.h"
#include "ref_gl/fnt/bitmapface.h"


/* Declarations of method implementations */
static void _FNT_FontFace_Initialise( OOL_Object object );
static void _FNT_FontFace_Destroy( OOL_Object object );

/** \brief Table of font faces
 *
 * This hash table keeps track of all initialised font faces, using a
 * face's full name as the key.
 */
static hashtable_t _FNT_FontFace_faces;

/** \brief Table of fonts
 *
 * This hash table keeps track of all initialised fonts. It uses a
 * concatenation of the font face's full name, the font's size and a
 * character indicating whether fixed width is being forced or not.
 */
static hashtable_t _FNT_FontFace_fonts;


/** \brief FNT_FontFace class definition */
static struct FNT_FontFace_cs _FNT_FontFace_class;
/** \brief FNT_FontFace class definition pointer */
static OOL_Class _FNT_FontFace_cptr = NULL;


/* Class-defining function; documentation in ref_gl/fnt/fontface.h */
OOL_Class FNT_FontFace__Class( )
{
	if( _FNT_FontFace_cptr != NULL ) {
		return _FNT_FontFace_cptr;
	}

	_FNT_FontFace_cptr = (OOL_Class) & _FNT_FontFace_class;
	OOL_InitClassBasics( _FNT_FontFace_class , FNT_FontFace , OOL_Object );

	// Set virtual method pointers
	OOL_SetMethod( FNT_FontFace , OOL_Object , Initialise );
	OOL_SetMethod( FNT_FontFace , OOL_Object , Destroy );
	_FNT_FontFace_class.Load = NULL;
	_FNT_FontFace_class.Unload = NULL;

	// Register properties
	OOL_Class_AddProperty( _FNT_FontFace_cptr , struct FNT_FontFace_s , base_directory , OOL_PTYPE_DIRECT_STRING );
	OOL_Class_AddProperty( _FNT_FontFace_cptr , struct FNT_FontFace_s , name , OOL_PTYPE_DIRECT_STRING );

	// Initialise static fields
	_FNT_FontFace_faces = HT_Create( 100 , 0 , sizeof( struct FNT_FontFace_s ) ,
					PTR_FieldOffset( struct FNT_FontFace_s , full_name ) ,
					PTR_FieldSize( struct FNT_FontFace_s , full_name ) );
	_FNT_FontFace_fonts = HT_Create( 200 , 0 , sizeof( struct FNT_Font_s ) ,
					PTR_FieldOffset( struct FNT_Font_s , font_key ) ,
					PTR_FieldSize( struct FNT_Font_s , font_key ) );

	return _FNT_FontFace_cptr;
}


/** \brief Initialisation
 *
 * Initialise the font's base directory if it has not been set, then
 * compute the face's full name. Once this is done, attempt to load
 * the face and, on success, add it to the table.
 */
static void _FNT_FontFace_Initialise( OOL_Object object )
{
	FNT_FontFace self;
	OOL_ClassCast( OOL_Object__Class( ) , OOL_Object )->Initialise( object );
	self = OOL_Cast( object , FNT_FontFace );

	// Check name and base directory
	assert( self->name[ 0 ] != 0 );
	if ( self->base_directory[ 0 ] == 0 ) {
		strcpy( self->base_directory , "fonts" );
	}

	// Compute full name
	Com_sprintf( self->full_name , sizeof( self->full_name ) , "%s/%s" ,
		self->base_directory , self->name );

	// Try loading the font
	self->loaded = FNT_FontFace_Load( object );

	// Add to table on success
	if ( self->loaded ) {
		HT_PutItem( _FNT_FontFace_faces , self , false );
	}
}


/** \brief Destructor
 *
 * If the font face had been loaded successfully, unload it and remove
 * it from the table of faces.
 */
static void _FNT_FontFace_Destroy( OOL_Object object )
{
	if ( OOL_Field( object , FNT_FontFace , loaded ) ) {
		FNT_FontFace_Unload( object );
	}

	OOL_ClassCast( OOL_Object__Class( ) , OOL_Object )->Destroy( object );
}


/** \brief <b>FNT_FontFace::GetFont</b> method implementation
 * \note public method
 *
 * Compute the font's key and try to find it in the table of fonts. If it
 * doesn't already exist, create it and check whether it was loaded. On
 * success, add it to the table; otherwise destroy it.
 */
OOL_Object FNT_FontFace_GetFont( FNT_FontFace object,  int size,  qboolean force_fixed )
{
	char font_key[ MAX_OSPATH + FNT_FACE_NAME_LEN + 14 + 1 ];
	OOL_Object font;

	// Compute font key
	Com_sprintf( font_key , sizeof( font_key ) , "%s|%d|%c" ,
		object->full_name , size , force_fixed ? 'Y' : 'N' );

	// Try finding the font
	font = HT_GetItem( _FNT_FontFace_fonts , font_key , false );
	if ( font ) {
		return font;
	}

	// Try creating the font
	font = OOL_Object_CreateEx( object->font_class ,
		"font_key" , font_key ,
		"face" , object ,
		"size" , size ,
		"force_fixed" , force_fixed ,
		NULL );
	if ( OOL_Field( font , FNT_Font , texture ) == NULL ) {
		// Could not load - destroy object
		OOL_Object_Destroy( font );
		font = NULL;
	} else {
		// Add font to table
		HT_PutItem( _FNT_FontFace_fonts , font , false );
	}

	return font;
}


/* Documentation in ref_gl/fnt/fontface.h */
OOL_Object FNT_FontFace_GetFrom( const char * name,  const char * directory )
{
	OOL_Class classes[2] = {
		FNT_BitmapFace__Class( ) ,
		FNT_TrueTypeFace__Class( )
	};
	char full_name[ MAX_OSPATH + FNT_FACE_NAME_LEN + 2 ];
	OOL_Object font;
	int i;

	// Try obtaining the face from the table of loaded faces
	Com_sprintf( full_name , sizeof( full_name ) , "%s/%s" , directory , name );
	font = HT_GetItem( _FNT_FontFace_faces , full_name , NULL );
	if ( font ) {
		return font;
	}

	// Try actually loading the font face
	for ( i = 0 ; i < 2 ; i ++ ) {
		font = OOL_Object_CreateEx( classes[ i ] ,
			"name" , name ,
			"base_directory" , directory ,
			NULL );

		if ( OOL_Field( font , FNT_FontFace , loaded ) ) {
			break;
		}

		OOL_Object_Destroy( font );
		font = NULL;
	}

	return font;
}
