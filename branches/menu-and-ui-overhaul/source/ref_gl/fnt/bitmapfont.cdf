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

#include "ref_gl/fnt/font"


/** \defgroup refgl_fnt_bitmapfont Bitmap font
 * \ingroup refgl_fnt
 *
 * The FNT_BitmapFont class implements the loader and unloader for a bitmap
 * font.
 */
class FNT_BitmapFont : FNT_Font
{
public:

	/** \brief Bitmap font loader
	 *
	 * The bitmap font loader generates the GL lists and initialises the
	 * font's information. It fails if the GL lists cannot be allocated.
	 */
	virtual void Load( );

	/** \brief Bitmap font unloader
	 *
	 * The unloader for bitmap fonts simply clears the GL lists.
	 */
	virtual void Unload( );
};
