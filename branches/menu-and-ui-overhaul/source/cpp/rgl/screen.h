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
 * @brief RGL::Screen class declaration
 */
#ifndef __H_CPP_RGL_SCREEN
#define __H_CPP_RGL_SCREEN

#include "rgl/engineaware.h"
#include "rgl/rectangle.h"

namespace RGL {


/** @brief Screen access
 *
 * Engine-aware access to the screen's coordinates.
 */
class Screen : public EngineAware
{
private:
	/** @brief Screen single instance */
	static Screen * singleton;

	/** @brief Screen rectangle
	 *
	 * If the rendering engine is running, this field will point to the
	 * screen's rectangle.
	 */
	Rectangle * rectangle;

	/** @brief Initialise the screen rectangle as needed
	 *
	 * If the renderer is active when the instance is created, the screen
	 * rectangle will be initialised.
	 */
	Screen( );

	/** @brief Destroy the screen rectangle on deletion */
	virtual ~Screen( );

	/** @brief Disabled, undefined copy constructor */
	Screen( const Screen & other );

	/** @brief Destroy the screen rectangle if it exists */
	void DestroyScreenRectangle( );

public:
	/** @brief Initialise the screen rectangle when the renderer starts */
	virtual void OnRendererStart( );

	/** @brief Destroy the screen rectangle when the renderer is stopped */
	virtual void OnRendererStop( );

	/** @brief Access the screen rectangle
	 *
	 * Return a reference to the screen's rectangle if the rendering
	 * engine is enabled, or Rectangle::BadRectangle if it is not.
	 */
	static const Rectangle & GetRectangle( );
};


};


#endif //__H_CPP_RGL_SCREEN
