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
 * @brief R_EngineAware class implementation
 * @note Initially generated from ref_gl/r_engineaware.cdf
 */

#include "ref_gl/r_engineaware.h"


/* Declarations of method implementations */
static void _R_EngineAware_PrepareInstance( OOL_Object object );
static void _R_EngineAware_Initialise( OOL_Object object );
static void _R_EngineAware_Destroy( OOL_Object object );

/** @brief List of all engine-aware objects
 *
 * List head for the list of all engine-aware objects; it is updated
 * as objects are created and destroyed, and used by SetStatus() to
 * call the appropriate handlers.
 */
static struct LST_item_s _R_EngineAware_ea_objects;

/** @brief Current engine status */
static qboolean _R_EngineAware_renderer_status;


/** @brief R_EngineAware class definition */
static struct R_EngineAware_cs _R_EngineAware_class;
/** @brief R_EngineAware class definition pointer */
static OOL_Class _R_EngineAware_cptr = NULL;


/* Class-defining function; documentation in ref_gl/r_engineaware.h */
OOL_Class R_EngineAware__Class( )
{
	if( _R_EngineAware_cptr != NULL ) {
		return _R_EngineAware_cptr;
	}

	_R_EngineAware_cptr = (OOL_Class) & _R_EngineAware_class;
	OOL_InitClassBasics( _R_EngineAware_class , R_EngineAware , OOL_Object );

	// Set virtual method pointers
	OOL_SetMethod( R_EngineAware , OOL_Object , PrepareInstance );
	OOL_SetMethod( R_EngineAware , OOL_Object , Initialise );
	OOL_SetMethod( R_EngineAware , OOL_Object , Destroy );
	_R_EngineAware_class.OnRendererStart = NULL;
	_R_EngineAware_class.OnRendererStop = NULL;

	// Initialise static fields
	LST_Init( &_R_EngineAware_ea_objects );
	_R_EngineAware_renderer_status = false;

	return _R_EngineAware_cptr;
}


/** @brief Add the instance to the list of engine-aware objects
 *
 * The newly created instance is added to the list of engine-aware
 * objects so that calls to @link R_EngineAware_SetStatus
 * R_EngineAware::SetStatus @endlink will affect it.
 *
 * @param object the newly created instance
 */
static void _R_EngineAware_PrepareInstance( OOL_Object object )
{
	OOL_ClassCast( OOL_Object__Class( ) , OOL_Object )->PrepareInstance( object );
	LST_Append( &_R_EngineAware_ea_objects , &OOL_Field( object , R_EngineAware , _eal_item ) );
}


/** @brief Call the renderer start handler if necessary
 *
 * When the object is being initialised, the renderer start handler
 * will be called automatically if the renderer is already started.
 *
 * @param object the object being initialised
 */
static void _R_EngineAware_Initialise( OOL_Object object )
{
	OOL_ClassCast( OOL_Object__Class( ) , OOL_Object )->Initialise( object );
	if ( _R_EngineAware_renderer_status ) {
		R_EngineAware_OnRendererStart( object );
	}
}


/** @brief Remove the object from the list and call stop handler
 *
 * If the renderer is currently active, the destructor will call the
 * renderer stop handler. It will then remove the object from the list
 * of engine-aware objects.
 *
 * @param object the object being destroyed.
 */
static void _R_EngineAware_Destroy( OOL_Object object )
{
	if ( _R_EngineAware_renderer_status ) {
		R_EngineAware_OnRendererStop( object );
	}
	LST_Remove( &OOL_Field( object , R_EngineAware , _eal_item ) );
	OOL_ClassCast( OOL_Object__Class( ) , OOL_Object )->Destroy( object );
}


/* Documentation in ref_gl/r_engineaware.h */
void R_EngineAware_SetStatus( qboolean status )
{
	if ( status != _R_EngineAware_renderer_status ) {
		struct LST_item_s * iterator;

		LST_Foreach( _R_EngineAware_ea_objects , iterator ) {
			OOL_Object obj = PTR_OffsetAddr( iterator , - PTR_FieldOffset( struct R_EngineAware_s , _eal_item ) );
			if ( status ) {
				R_EngineAware_OnRendererStart( obj );
			} else {
				R_EngineAware_OnRendererStop( obj );
			}
		}

		_R_EngineAware_renderer_status = status;
	}
}
