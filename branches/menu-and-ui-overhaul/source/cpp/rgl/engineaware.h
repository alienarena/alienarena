/*
Copyright (C) 2011 COR Entertainment, LLC.

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
 * @brief RGL::EngineAware class declarations
 */
#ifndef __H_CPP_RGL_ENGINEAWARE
#define __H_CPP_RGL_ENGINEAWARE


#include "com/list.h"

namespace RGL {

/** @brief Objects aware of the rendering engine's state
 *
 * Objects that inherit this abstract class will be kept informed of the state
 * of the game's rendering engine.
 * The class provides two abstract method, which will be called automatically
 * for all known instances, when the engine is initialised or shut down.
 */
class EngineAware
{
private:
	/** @brief List of all engine-aware objects
	 *
	 * List head for the list of all engine-aware objects; it is updated
	 * as objects are created and destroyed, and used by SetStatus() to
	 * call the appropriate handlers.
	 */
	static LST_item_s ea_objects;

	/** @brief Current engine status */
	static bool renderer_status;

	/** @brief List entry
	 *
	 * This field is used to store an engine-aware object's list data.
	 */
	struct COM::List<EngineAware> eal_item;

public:
	/** @brief Add the instance to the list of engine-aware objects
	 *
	 * The newly created instance is added to the list of engine-aware
	 * objects so that calls to EngineAware::SetStatus() will affect it.
	 */
	EngineAware( );

	/** @brief Remove the object from the list
	 *
	 * Remove the object from the list of engine-aware objects.
	 */
	virtual ~EngineAware( );

protected:
	/** @brief Check if the renderer is active
	 *
	 * @return true if the renderer is active, false if it isn't.
	 */
	static bool IsRendererActive( );

public:
	/** @brief Rendering engine start handler */
	virtual void OnRendererStart( ) = 0;

	/** @brief Rendering engine stop handler */
	virtual void OnRendererStop( ) = 0;

	/** @brief Set the status of the rendering engine
	 *
	 * This method sets the status of the rendering engine and calls the
	 * appropriate handler for all known engine-aware objects.
	 *
	 * @param status true if the engine is starting, false if it is being
	 * shut down.
	 */
	static void SetStatus( bool status );
};


inline bool EngineAware::IsRendererActive( )
{
	return EngineAware::renderer_status;
}


};

#endif //__H_CPP_RGL_ENGINEAWARE
