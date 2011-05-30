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


/** \defgroup refgl_fnt_truetypeface TrueType font face
 * \ingroup refgl_fnt
 *
 * This class implements support for TrueType faces. It initialises the
 * FreeType library as required, and is capable of verifying that a TrueType
 * face can be used by the engine when loading it.
 */
class FNT_TrueTypeFace : FNT_FontFace
{
private:
	/** \brief TrueType face counter
	 *
	 * This counter stores the amount of loaded TrueType faces. It is used
	 * to initialise and free the library.
	 */
	static unsigned int counter = 0;

public:
	/** \brief FreeType library handle */
	static FT_Library library = 0;

	/** \brief Subpixel rendering control
	 *
	 * This CVar controls sub-pixel rendering. It is ignored if the FreeType
	 * library is not compiled with sub-pixel rendering support.
	 */
	static cvar_t * subpixel = Cvar_Get( "ttf_subpixel" , "0" , CVAR_ARCHIVE );

	/** \brief Auto-hinting control
	 *
	 * This CVar controls whether the FreeType library should force fonts
	 * to use the auto-hinter, even if the face includes hints and hinting is
	 * supported by the library.
	 */
	static cvar_t * autohint = Cvar_Get( "ttf_autohint" , "0" , CVAR_ARCHIVE );

private:
	/** \brief FreeType library initialisation
	 *
	 * Initialise the FreeType library, setting sub-pixel rendering mode if
	 * required.
	 *
	 * \return true on success, false on failure.
	 */
	static qboolean InitialiseLibrary( );

	/** \brief FreeType library destruction
	 *
	 * Free resources used by the FreeType library.
	 */
	static void DestroyLibrary( );

public:
	/** \brief TrueType file size */
	unsigned int file_size;

	/** \brief TrueType file contents */
	void * file_data;

protected:

	/** \brief TrueType face loader
	 *
	 * Check if the library needs to be initialised, and do so if necessary.
	 * Then attempt to load the specified font; on success, make sure it can
	 * be scaled. Finally, increase the counter.
	 *
	 * \return true on success, or false if any of the steps fails.
	 */
	virtual qboolean Load( );

	/** \brief TrueType face unloader
	 *
	 * Free the memory used for the file's contents, then decrease the counter.
	 * If the counter reached 0, destroy the FreeType library's instance.
	 */
	virtual void Unload( );
};
