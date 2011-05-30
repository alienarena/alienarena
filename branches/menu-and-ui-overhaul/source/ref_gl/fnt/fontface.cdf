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

#include "qcommon/objects"

/** \defgroup refgl_fnt_fontface Generic font face
 * \ingroup refgl_fnt
 *
 * A font face corresponds to the shape of a font's characters. It is loaded
 * from a file (either a TrueType font file or a simple texture).
 *
 * The generic font face class implements the generic loading system as well
 * as elements common to all face loaders. It is an abstract class - child
 * classes must implement the Load() and Unload() methods.
 */

class FNT_FontFace
{
private:
	/** \brief Table of font faces
	 *
	 * This hash table keeps track of all initialised font faces, using a
	 * face's full name as the key.
	 */
	static hashtable_t faces = HT_Create( 100 , 0 , sizeof( struct FNT_FontFace_s ) ,
					PTR_FieldOffset( struct FNT_FontFace_s , full_name ) ,
					PTR_FieldSize( struct FNT_FontFace_s , full_name ) );

	/** \brief Table of fonts
	 *
	 * This hash table keeps track of all initialised fonts. It uses a
	 * concatenation of the font face's full name, the font's size and a
	 * character indicating whether fixed width is being forced or not.
	 */
	static hashtable_t fonts = HT_Create( 200 , 0 , sizeof( struct FNT_Font_s ) ,
					PTR_FieldOffset( struct FNT_Font_s , font_key ) ,
					PTR_FieldSize( struct FNT_Font_s , font_key ) );

protected:
	/** \brief Font class
	 *
	 * The class to use when creating a font using this face.
	 */
	OOL_Class font_class;

	/** \brief Full name of the font face
	 *
	 * The full name (including base directory) of a font face.
	 */
	char full_name[ MAX_OSPATH + FNT_FACE_NAME_LEN + 2 ];

public:
	/** \brief Face loader status
	 *
	 * This field will be set to true after the font face has been loaded.
	 */
	qboolean loaded;

	/** \brief Fonts
	 *
	 * The amount of fonts that use this face.
	 */
	unsigned int n_fonts;

	/** \brief Base directory
	 *
	 * This property indicates the font's base directory. If it is left
	 * empty, it will be initialised to "fonts".
	 */
	char base_directory[ MAX_OSPATH + 1 ] |type DIRECT_STRING|;

	/** \brief Font face name
	 *
	 * This property contains the name of the face to load.
	 */
	char name[ FNT_FACE_NAME_LEN + 1 ] |type DIRECT_STRING|;

	/** \brief Initialisation
	 *
	 * Initialise the font's base directory if it has not been set, then
	 * compute the face's full name. Once this is done, attempt to load
	 * the face and, on success, add it to the table.
	 */
	void Initialise( );

	/** \brief Destructor
	 *
	 * If the font face had been loaded successfully, unload it and remove
	 * it from the table of faces.
	 */
	void Destroy( );


	/** \brief Face loader
	 *
	 * This abstract method must be overridden to perform the actual
	 * loading of a font. It is called by the initialisation method.
	 *
	 * \returns true if the font was loaded, false otherwise.
	 */
	virtual qboolean Load( ) = 0;


	/** \brief Face unloader
	 *
	 * This abstract method must be overridden to unload the font face.
	 * It is called from the destructor.
	 */
	virtual void Unload( ) = 0;


	/** \brief Font access
	 *
	 * This method initialises a font using the current face.
	 *
	 * \param size the size of the font to access
	 * \param force_fixed whether fixed width should be enforced on the
	 * font.
	 *
	 * \returns the new font, or NULL if an error occured.
	 */
	OOL_Object GetFont( int size , qboolean force_fixed );


	/** \brief Default font loader
	 *
	 * Attempt to load the face from the default font directory. Try a
	 * TrueType face first, and a bitmap one if that fails.
	 *
	 * \param name the name of the font face
	 * \returns the loaded font face or NULL on failure
	 */
	inline static OOL_Object Get( const char * name );

	/** \brief Font loader with path
	 *
	 * Attempt to load the face from the specified directory. Try a
	 * TrueType face first, and a bitmap one if that fails.
	 *
	 * \param name the name of the font face
	 * \param directory the name of the directory to load from
	 * \returns the loaded font face or NULL on failure
	 */
	static OOL_Object GetFrom( const char * name , const char * directory );


	/** \brief Font destruction callback
	 *
	 * This method is called by the fonts' destructor to notify that it is being
	 * destroyed. It will remove the font from the fonts cache.
	 *
	 * \param font the font being destroyed
	 */
	static void NotifyFontDestroy( OOL_Object font );
};


constants {
	/** \brief Maximal length of a face's name */
	FNT_FACE_NAME_LEN = 48;
};
