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

#include "qcommon/objects"

/** \defgroup Engine-aware objects
 * \ingroup refgl
 *
 * This abstract class is used to create objects which are kept informed of
 * the rendering engine's initialisation and shutdown. A list of all
 * such objects is kept and their OnVidStart()/OnVidStop() methods are called
 * as necessary.
 */
class R_EngineAware
{
	/** \brief List of all engine-aware objects
	 *
	 * List head for the list of all engine-aware objects; it is updated
	 * as objects are created and destroyed, and used by SetStatus() to
	 * call the appropriate handlers.
	 */
	static struct LST_item_s ea_objects = { NULL , NULL };

	/** \brief Current engine status */
	static qboolean renderer_status = false;

	/** \brief List entry
	 *
	 * This field is used to store an engine-aware object's list data.
	 */
	struct LST_item_s eal_item;

public:
	/** \brief Add the instance to the list of engine-aware objects
	 *
	 * The newly created instance is added to the list of engine-aware
	 * objects so that calls to \link R_EngineAware_SetStatus
	 * R_EngineAware::SetStatus \endlink will affect it.
	 *
	 * \param object the newly created instance
	 */
	virtual void PrepareInstance( );

	/** \brief Call the renderer start handler if necessary
	 *
	 * When the object is being initialised, the renderer start handler
	 * will be called automatically if the renderer is already started.
	 *
	 * \param object the object being initialised
	 */
	virtual void Initialise( );

	/** \brief Remove the object from the list and call stop handler
	 *
	 * If the renderer is currently active, the destructor will call the
	 * renderer stop handler. It will then remove the object from the list
	 * of engine-aware objects.
	 *
	 * \param object the object being destroyed.
	 */
	virtual void Destroy( );

	/** \brief Rendering engine start handler */
	virtual void OnRendererStart( ) = 0;

	/** \brief Rendering engine stop handler */
	virtual void OnRendererStop( ) = 0;

	/** \brief Set the status of the rendering engine
	 *
	 * This method sets the status of the rendering engine and calls the
	 * appropriate handler for all known engine-aware objects.
	 *
	 * \param status true if the engine is starting, false if it is being
	 * shut down.
	 */
	static void SetStatus( qboolean status );
	
};
