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

#include "ref_gl/fnt/fontface"


/** @defgroup refgl_fnt_bitmapface Bitmap font face
 * @ingroup refgl_fnt
 *
 * The FNT_BitmapFace class implements the loader and unloader for a bitmap
 * font's face. It consists in a texture that is loaded from the face's full
 * path.
 */
class FNT_BitmapFace : FNT_FontFace
{
public:
	/** @brief Face bitmap
	 *
	 * This texture is the font face's bitmap, which will be used by the
	 * fonts.
	 */
	image_t * texture;

protected:
	/** @brief Bitmap face loader
	 *
	 * Attempt to load the bitmap face's texture from the face's full
	 * path.
	 *
	 * @return true if the texture was loaded, false otherwise.
	 */
	virtual qboolean Load( );

	/** @brief Bitmap face unloader
	 *
	 * Free the bitmap face's texture.
	 */
	virtual void Unload( );
};
