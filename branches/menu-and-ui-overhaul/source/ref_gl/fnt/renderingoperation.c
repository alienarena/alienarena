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
 * @brief FNT_RenderingOperation class implementation
 * @note Initially generated from ref_gl/fnt/renderingoperation.cdf
 */

#include "ref_gl/fnt/renderingoperation.h"


/* Declarations of method implementations */
static void _FNT_RenderingOperation_ExpandFreeList( );
static void _FNT_RenderingOperation_Initialise( OOL_Object object );
static void _FNT_RenderingOperation_Destroy( OOL_Object object );

/** @brief List of free rendering steps
 *
 * This list is used to store allocated rendering step structures
 * which are not being used at the moment. It avoids having to
 * reallocate rendering step structures all the time.
 */
static struct LST_item_s _FNT_RenderingOperation_free_steps;


/** @brief FNT_RenderingOperation class definition */
static struct FNT_RenderingOperation_cs _FNT_RenderingOperation_class;
/** @brief FNT_RenderingOperation class definition pointer */
static OOL_Class _FNT_RenderingOperation_cptr = NULL;


/* Class-defining function; documentation in ref_gl/fnt/renderingoperation.h */
OOL_Class FNT_RenderingOperation__Class( )
{
	if( _FNT_RenderingOperation_cptr != NULL ) {
		return _FNT_RenderingOperation_cptr;
	}

	_FNT_RenderingOperation_cptr = (OOL_Class) & _FNT_RenderingOperation_class;
	OOL_InitClassBasics( _FNT_RenderingOperation_class ,
			FNT_RenderingOperation , OOL_Object );

	// Set virtual method pointers
	OOL_SetMethod( FNT_RenderingOperation , OOL_Object , Initialise );
	OOL_SetMethod( FNT_RenderingOperation , OOL_Object , Destroy );

	// Initialise static fields
	LST_Init( &_FNT_RenderingOperation_free_steps );

	return _FNT_RenderingOperation_cptr;
}


/** @brief Expand the list of free rendering steps
 *
 * This method is called when the list of free steps is found to be
 * empty; it will allocate new entries and add them to the list.
 */
static void _FNT_RenderingOperation_ExpandFreeList( )
{
	// XXX: write method code here
}


/** @brief Rendering operation initialiser
 *
 * The initialiser sets up the list head so it indicates an empty list.
 *
 * @param object the object being initialised
 */
static void _FNT_RenderingOperation_Initialise( OOL_Object object )
{
	FNT_RenderingOperation self;
	OOL_ClassCast( OOL_Object__Class( ) ,
			OOL_Object )->Initialise( object );

	self = OOL_Cast( object , FNT_RenderingOperation );
	self->_steps.previous = self->_steps.next = NULL;
	self->_base_height = 0;
	self->_total_y_trans = 0;
}


/** @brief Destructor
 *
 * The destructor clears the list of steps using the Clear() method.
 *
 * @param object the object being destroyed.
 */
static void _FNT_RenderingOperation_Destroy( OOL_Object object )
{
	FNT_RenderingOperation_Clear( OOL_Cast( object ,
				FNT_RenderingOperation ) );
	OOL_ClassCast( OOL_Object__Class( ) ,
			OOL_Object )->Destroy( object );
}


/* Documentation in ref_gl/fnt/renderingoperation.h */
void FNT_RenderingOperation_Clear( FNT_RenderingOperation object )
{
	// XXX: write method code here
}


/* Documentation in ref_gl/fnt/renderingoperation.h */
void FNT_RenderingOperation_AddStep( FNT_RenderingOperation object ,
		int type , float * color , int gl_list_id , double x ,
		double y )
{
	// XXX: write method code here
}


/* Documentation in ref_gl/fnt/renderingoperation.h */
void FNT_RenderingOperation_SetBaseHeight( FNT_RenderingOperation object ,
		int base_height )
{
	// XXX: write method code here
}


/* Documentation in ref_gl/fnt/renderingoperation.h */
int FNT_RenderingOperation_GetHeight( FNT_RenderingOperation object )
{
	// XXX: write method code here
	return (int)0; // XXX: change this
}


/* Documentation in ref_gl/fnt/renderingoperation.h */
void FNT_RenderingOperation_Draw( FNT_RenderingOperation object , int x ,
		int y , int win_x , int win_y , int win_w , int win_h ,
		float alpha )
{
	// XXX: write method code here
}
