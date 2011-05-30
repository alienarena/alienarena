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
 * \brief FNT_Font class implementation
 * \note Initially generated from ref_gl/fnt/font.cdf
 */

#include "ref_gl/fnt/font.h"


/* Declarations of method implementations */
static void _FNT_Font_Initialise( OOL_Object object );
static void _FNT_Font_Destroy( OOL_Object object );

/* Declarations of property handlers */
static qboolean _PV_FNT_Font_face(const void * value);


/** \brief FNT_Font class definition */
static struct FNT_Font_cs _FNT_Font_class;
/** \brief FNT_Font class definition pointer */
static OOL_Class _FNT_Font_cptr = NULL;


/* Class-defining function; documentation in ref_gl/fnt/font.h */
OOL_Class FNT_Font__Class( )
{
	if( _FNT_Font_cptr != NULL ) {
		return _FNT_Font_cptr;
	}

	_FNT_Font_cptr = (OOL_Class) & _FNT_Font_class;
	OOL_InitClassBasics( _FNT_Font_class , FNT_Font , OOL_Object );

	// Set virtual method pointers
	_FNT_Font_class.Load = NULL;
	_FNT_Font_class.Unload = NULL;
	OOL_SetMethod( FNT_Font , OOL_Object , Initialise );
	OOL_SetMethod( FNT_Font , OOL_Object , Destroy );

	// Register properties
	OOL_Class_AddPropertyEx( _FNT_Font_cptr , "face" ,
		PTR_FieldOffset( struct FNT_Font_s , face ) , OOL_PTYPE_POINTER ,
		PTR_FieldSize( struct FNT_Font_s , face ) ,
		_PV_FNT_Font_face ,
		NULL ,
		NULL ,
		NULL );
	OOL_Class_AddProperty( _FNT_Font_cptr , struct FNT_Font_s , size , OOL_PTYPE_WORD );
	OOL_Class_AddProperty( _FNT_Font_cptr , struct FNT_Font_s , force_fixed , OOL_PTYPE_WORD );
	OOL_Class_AddProperty( _FNT_Font_cptr , struct FNT_Font_s , font_key , OOL_PTYPE_DIRECT_STRING );

	return _FNT_Font_cptr;
}


/** \brief Font initialisation
 *
 * The initialiser sets up the font's key and calls the Load() method.
 * If loading succeeds, it initialises the font's renderer object.
 */
static void _FNT_Font_Initialise( OOL_Object object )
{
	FNT_Font self;
	OOL_ClassCast( OOL_Object__Class( ) , OOL_Object )->Initialise( object );
	self = OOL_Cast( object , FNT_Font );

	// Check face, key and size
	assert( self->face != NULL );
	assert( self->font_key[ 0 ] != 0 );
	if ( self->size < 4 || self->size > 64 ) {
		return;
	}

	// Load font
	FNT_Font_Load( object );
	if ( self->texture == NULL ) {
		return;
	}

	// Increase face's font counter
	OOL_Field( self->face , FNT_FontFace , n_fonts ) ++;

	// XXX: initialise renderer
}


/** \brief Font destructor
 *
 * The destructor calls the font's Unload() method if the font had
 * been loaded successfully. It also decrements the font face's use
 * counter and destroys the face if appropriate.
 *
 * \note The renderer is not affected, as it actually contains the
 * font object and triggers its destruction.
 */
static void _FNT_Font_Destroy( OOL_Object object )
{
	FNT_Font self = OOL_Cast( object , FNT_Font);
	if ( self->texture != NULL ) {
		FNT_Font_Unload( object );

		OOL_Field( self->face , FNT_FontFace , n_fonts ) --;
		if ( ! OOL_Field( self->face , FNT_FontFace , n_fonts ) ) {
			OOL_Object_Destroy( self->face );
		}
	}

	OOL_ClassCast( OOL_Object__Class( ) , OOL_Object )->Destroy( object );
}


/** \brief Validator for property <b>FNT_Font::face</b>
 *
 * This validator ensures that face is not NULL and that it is a
 * FNT_Face instance.
 */
static qboolean _PV_FNT_Font_face( const void * value )
{
	return ( value != NULL && OOL_IsInstance( (OOL_Object) value , FNT_FontFace__Class( ) ) );
}
