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

/** @defgroup refgl_fnt_font Generic font
 * @ingroup refgl_fnt
 *
 * A font is the combination of a face (the way characters are drawn) and a
 * size. The generic font class contains all common information such as
 * supported characters, texture and GL lists, sizes, etc. It also provides
 * abstract methods which are then implemented in child classes depending on
 * the type of face.
 */

class FNT_Font
{
public:
	/** @brief Face of the font
	 *
	 * The FNT_FontFace instance which describes the font's aspect.
	 */
	OOL_Object face |type POINTER validator|;

	/** @brief Font height */
	int size |type WORD|;

	/** @brief Forced fixed width flag
	 *
	 * If this flag is set, the font will be set up as a fixed-width font,
	 * even if it isn't.
	 */
	qboolean force_fixed |type WORD|;

	/** @brief Font key
	 *
	 * A string which represents the font's face, size, and fixed width
	 * flag. It is used to identify fonts in a unique manner.
	 */
	char font_key[ MAX_OSPATH + FNT_FACE_NAME_LEN + 14 + 1 ] |type DIRECT_STRING|;

	/** @brief Font texture
	 *
	 * The generated or loaded texture which stores the font's glyphs.
	 */
	image_t * texture;

	/** @brief GL lists
	 *
	 * The identifier of the first of the GL lists used to render each
	 * character.
	 */
	unsigned int gl_lists;

	/** @brief First supported character
	 *
	 * The index of the first character that this font supports.
	 */
	int first_character;

	/** @brief Supported characters
	 *
	 * The amount of characters that can be displayed, starting from the
	 * first supported character.
	 */
	int num_characters;

	/** @brief Fixed width
	 *
	 * If the font has a fixed width or if fixed width is being enforced,
	 * this field will contain the characters' width. Otherwise it will
	 * be set to 0.
	 */
	unsigned int fixed_width;

	/** @brief Maximal height of characters
	 *
	 * The height of the font's characters in pixels.
	 */
	unsigned int height;

	/** @brief Base line
	 *
	 * The font's base line, relative to the top of the characters.
	 */
	unsigned int base_line;

	/** @brief Character widths
	 *
	 * If characters have variable widths, this field will point to the
	 * array which lists the width of each character. Otherwise it will
	 * be set to NULL.
	 */
	int * widths;

	/** @brief Kerning values
	 *
	 * If the font has variable widths, this field will point to a 2D array
	 * listing the kerning between each combination of supported
	 * characters. Otherwise it will be set to NULL.
	 */
	int * kerning;

	/** @brief Renderer
	 *
	 * The FNT_Renderer instance to use when rendering the font.
	 */
	OOL_Object renderer;

protected:
	/** @brief Font loader
	 *
	 * This method must be overridden to implement the font's loader. It
	 * must set the various values of the font's structure. It is called
	 * by the font's initialiser.
	 */
	virtual void Load( ) = 0;

	/** @brief Font unloader
	 *
	 * This method must be overridden to implement the font's destruction.
	 * It is called from the font's destructor if the font had been
	 * successfully loaded.
	 */
	virtual void Unload( ) = 0;

public:
	/** @brief Font initialisation
	 *
	 * The initialiser calls the Load() method. If loading succeeds, it
	 * initialises the font's renderer object.
	 */
	virtual void Initialise( );

	/** @brief Font destructor
	 *
	 * The destructor calls the font's Unload() method if the font had
	 * been loaded successfully. It also decrements the font face's use
	 * counter and destroys the face if appropriate.
	 *
	 * @note The renderer is not affected, as it actually contains the
	 * font object and triggers its destruction.
	 */
	virtual void Destroy( );
};
