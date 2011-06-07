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
 * @brief RGL::Rectangle class definition
 */

#include "rgl/rectangle.h"
#include "ref_gl/r_local.h"


using namespace RGL;


const Rectangle Rectangle::BadRectangle;


Rectangle Rectangle::GetIntersection( const Rectangle & other ) const
{
	if ( ! this->Intersects( other ) ) {
		return Rectangle::BadRectangle;
	}

	int nx1 = other.x > this->x ? other.x : this->x;
	int ny1 = other.y > this->y ? other.y : this->y;
	int nx2 = this->X2( ) < other.X2( ) ? this->X2( ) : other.X2( );
	int ny2 = this->Y2( ) < other.Y2( ) ? this->Y2( ) : other.Y2( );

	return Rectangle( nx1 , ny1 , nx2 - nx1 , ny2 - ny1 );
}


void Rectangle::SetClipping( ) const
{
	qglPushAttrib( GL_SCISSOR_BIT );
	if ( ! this->IsValid( ) ) {
		return;
	}
	qglScissor( this->x , viddef.height - ( this->Y2( ) ) ,
			this->width , this->height );
	qglEnable( GL_SCISSOR_TEST );
}


void Rectangle::FillColor( float color[ 4 ] ) const
{
	if ( ! this->IsValid( ) ) {
		return;
	}

	qglPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
	qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_ALPHA_TEST );
	qglEnable( GL_BLEND );
	qglBlendFunc ( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
	qglColor4fv( (GLfloat *) color );

	qglBegin( GL_QUADS );
	qglVertex2f( this->x , this->y );
	qglVertex2f( this->X2( ) , this->y );
	qglVertex2f( this->X2( ) , this->Y2( ) );
	qglVertex2f( this->x , this->Y2( ) );
	qglEnd( );

	qglPopAttrib( );
}
