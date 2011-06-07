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
 * @brief RGL::Rectangle class declaration
 */
#ifndef __H_CPP_RGL_RECTANGLE
#define __H_CPP_RGL_RECTANGLE


namespace RGL {


/** @brief A rectangle
 *
 * Stores the coordinates of a rectangle, and allows various operations such
 * as intersection computation, clipping, etc...
 */
class Rectangle
{
public:
	/** @brief Invalid rectangle
	 *
	 * An invalid rectangle of size 0 that is returned when an invalid
	 * operation is attempted.
	 */
	const static Rectangle BadRectangle;

private:
	/** @brief Left coordinate of the rectangle */
	int		x;
	/** @brief Top coordinate of the rectangle */
	int		y;
	/** @brief Width of the rectangle */
	int		width;
	/** @brief Height of the rectangle */
	int		height;

public:
	/** @brief Create an invalid rectangle
	 *
	 * Create a rectangle with coordinates and size set to 0.
	 */
	Rectangle( );

	/** @brief Create a rectangle
	 *
	 * @param x left coordinate
	 * @param y top coordinate
	 * @param width width (must be greater than 0)
	 * @param height height (must be greater than 0)
	 */
	Rectangle( int x , int y , int width , int height );

	/** @brief Check if the rectangle is valid
	 *
	 * @return false if either the width or height are negative, true
	 *	otherwise
	 */
	bool IsValid( ) const;

	/** @return the left coordinate of the rectangle */
	int X1( ) const;

	/** @return the top coordinate of the rectangle */
	int Y1( ) const;

	/** @return the right coordinate of the rectangle */
	int X2( ) const;

	/** @return the bottom coordinate of the rectangle */
	int Y2( ) const;

	/** @return the width of the rectangle */
	int Width( ) const;

	/** @return the height of the rectangle */
	int Height( ) const;

	/** @brief Check whether two rectangles are equal
	 *
	 * @param other the other rectangle
	 *
	 * @return true if both rectangles are equal, false otherwise
	 */
	bool operator ==( const Rectangle & other ) const;

	/** @brief Check whether some coordinates are inside the rectangle
	 *
	 * @param x abscissa of the point
	 * @param y ordinate of the point
	 *
	 * @return true if the point is inside the rectangle, false otherwise
	 */
	bool Contains( int x , int y ) const;

	/** @brief Check whether two rectangles intersect
	 *
	 * @param other the other rectangle
	 *
	 * @return true if both rectangles are valid and there is an
	 *	intersection, false otherwise
	 */
	bool Intersects( const Rectangle & other ) const;

	/** @brief Create an intersection rectangle
	 *
	 * @param other the other rectangle
	 *
	 * @return a rectangle that corresponds to the intersection between
	 *	both rectangles, or Rectangle::BadRectangle if there is no
	 *	intersection
	 */
	Rectangle GetIntersection( const Rectangle & other ) const;

	/** @brief Translate the current rectangle
	 *
	 * @param x the horizontal translation
	 * @param y the vertical translation
	 *
	 * @return the current rectangle
	 */
	Rectangle & Translate( int x , int y );

	/** @brief Create a new rectangle from a translation
	 *
	 * @param x the horizontal translation
	 * @param x the vertical translation
	 *
	 * @return the new rectangle
	 */
	Rectangle GetTranslation( int x , int y );

	/** @brief Set GL scissors to the rectangle
	 *
	 * Set GL clipping to the current rectangle by activating
	 * GL_SCISSOR_TEST and setting the scissor box. The current scissor
	 * parameters will be pushed to the state stack, so they can be
	 * reinitialised later using qglPopAttrib().
	 *
	 * @note If the current rectangle is invalid, the GL scissor
	 * parameters will not be changed but the current parameters will
	 * still be pushed to the stack.
	 */
	virtual void SetClipping( ) const;

	/** @brief Fill the rectangle using a color
	 *
	 * Fill the rectangle using a color. Alpha blending will be enabled
	 * and alpha tests will be disabled during the operation.
	 *
	 * @param color the RGBA color of the rectangle
	 */
	void FillColor( float color[4] ) const;
};

inline Rectangle::Rectangle( )
	: x( 0 ) , y( 0 ) , width( 0 ) , height( 0 )
{
	// PURPOSEDLY EMPTY
}

inline Rectangle::Rectangle( int x , int y , int width , int height )
	: x( x ) , y( y ) , width( width ) , height( height )
{
	// PURPOSEDLY EMPTY
}

inline bool Rectangle::IsValid( ) const
{
	return ( this->width > 0 && this->height > 0 );
}

inline int Rectangle::X1( ) const
{
	return this->x;
}

inline int Rectangle::Y1( ) const
{
	return this->y;
}

inline int Rectangle::X2( ) const
{
	return this->x + this->width;
}

inline int Rectangle::Y2( ) const
{
	return this->y + this->height;
}

inline int Rectangle::Width( ) const
{
	return this->width;
}

inline int Rectangle::Height( ) const
{
	return this->height;
}

inline bool Rectangle::operator ==( const Rectangle & other ) const
{
	return this->x == other.x && this->y == other.y
		&& this->width == other.width && this->height == other.height;
}

inline bool Rectangle::Contains( int x , int y ) const
{
	return x >= this->x && x < this->X2( )
			&& y >= this->y && y < this->Y2( );
}

inline bool Rectangle::Intersects( const Rectangle & other ) const
{
	return this->IsValid() && other.IsValid( ) && !(
			this->x >= other.X2( ) || this->y >= other.Y2( )
				|| other.x >= this->X2( )
				|| other.y >= this->Y2( ) );
}

inline Rectangle & Rectangle::Translate( int x , int y )
{
	this->x += x;
	this->y += y;
	return *this;
}

inline Rectangle Rectangle::GetTranslation( int x , int y )
{
	return Rectangle( *this ).Translate( x , y );
}


};

#endif //__H_CPP_RGL_RECTANGLE
