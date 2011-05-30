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


/** @defgroup refgl_fnt_truetypefont TrueType font
 * @ingroup refgl_fnt
 *
 * The FNT_TrueTypeFont class implements the loader and unloader for a TrueType
 * font.
 */
class FNT_TrueTypeFont : FNT_Font
{
private:
	/** @brief Bitmap of the font */
	unsigned char * bitmap;

	/** @brief Rendering mode */
	unsigned int render_mode;


	/** @brief Load a TrueType face
	 *
	 * Load the data from the TrueType file's contents and set rendering
	 * information.
	 *
	 * @param face pointer to the FreeType face handle
	 *
	 * @return true on success, false on error
	 */
	qboolean LoadFace( FT_Face * face );

	/** @brief Compute the face's size information
	 *
	 * Compute information such as total height, width or widths, and amount
	 * of lines on the texture.
	 *
	 * @param face the FreeType face handle
	 * @param glyphs the uninitialised glyph array
	 *
	 * @return the amount of lines on the texture or 0 on error.
	 */
	unsigned int ComputeSizeInfo( FT_Face face , unsigned int * glyphs );

	/** @brief Load kerning information
	 *
	 * If the font supports kerning and is not being forced to fixed width,
	 * load kerning values for all supported glyphs.
	 *
	 * @param face the FreeType face handle
	 * @param glyphs the glyph array
	 *
	 * @return true on success, false on failure
	 */
	qboolean GetKerningInfo( FT_Face face , unsigned int * glyphs );


	/** @brief Initialise rendering data
	 *
	 * Initialise the bitmap, texture and GL lists that will be used to render
	 * the font.
	 *
	 * @param face the FreeType face handle
	 * @param glyphs the glyph array
	 * @param n_lines the amount of lines on the texture, as returned by
	 * <b>FNT_TrueTypeFont::ComputeSizeInfo</b>
	 *
	 * @return true on success, false on failure
	 */
	qboolean GetRenderData( FT_Face face , unsigned int * glyphs , unsigned int n_lines );

public:

	/** @brief TrueType font loader
	 *
	 * Attempt to load the TrueType face's information, compute font metrics,
	 * including kerning if available, and generate the font's texture and
	 * GL lists.
	 */
	virtual void Load( );

	/** @brief TrueType font unloader
	 *
	 * Free width and kerning arrays, font bitmap and texture, and GL lists.
	 */
	virtual void Unload( );
};

