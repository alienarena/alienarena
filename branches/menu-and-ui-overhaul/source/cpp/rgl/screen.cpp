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
 * @brief RGL::Screen class implementation
 */
#include "rgl/screen.h"
#include "ref_gl/r_local.h"


namespace RGL {
	/** @brief Screen rectangle class
	 *
	 * A special case of Rectangle, since the size and coordinates of the
	 * rectangle are known at construction time. Setting clipping to the
	 * screen rectangle disables the scissor test.
	 */
	class _ScreenRectangle : public Rectangle
	{
	public:
		/** @brief Initialise the screen rectangle
		 *
		 * The coordinates are set to (0,0) and the size is read from
		 * the engine's viddef structure
		 */
		_ScreenRectangle( );

		/** @brief Disable clipping
		 *
		 * The scissor test is disabled. Attributes are pushed to the
		 * stack before the operation, for compatibility with
		 * Rectangle::SetClipping()
		 */
		virtual void SetClipping( ) const;
	};
};


using namespace RGL;


// _ScreenRectangle class

inline _ScreenRectangle::_ScreenRectangle( )
	: Rectangle( 0 , 0 , viddef.width , viddef.height )
{
	// PURPOSEDLY EMPTY
}

void _ScreenRectangle::SetClipping( ) const
{
	qglPushAttrib( GL_SCISSOR_BIT );
	qglDisable( GL_SCISSOR_TEST );
}


// Screen class

Screen * Screen::singleton;

Screen::Screen( ) : EngineAware( )
{
	if ( IsRendererActive( ) ) {
		this->rectangle = new _ScreenRectangle( );
	} else {
		this->rectangle = NULL;
	}
}

Screen::~Screen( )
{
	this->DestroyScreenRectangle( );
}


void Screen::DestroyScreenRectangle( )
{
	if ( this->rectangle ) {
		delete this->rectangle;
		this->rectangle = NULL;
	}
}


void Screen::OnRendererStart( )
{
	if ( ! this->rectangle ) {
		this->rectangle = new _ScreenRectangle( );
	}
}


void Screen::OnRendererStop( )
{
	this->DestroyScreenRectangle( );
}


const Rectangle & Screen::GetRectangle( )
{
	if ( ! Screen::singleton ) {
		Screen::singleton = new Screen( );
	}
	if ( Screen::singleton->rectangle ) {
		return *( Screen::singleton->rectangle );
	}
	return Rectangle::BadRectangle;
}
