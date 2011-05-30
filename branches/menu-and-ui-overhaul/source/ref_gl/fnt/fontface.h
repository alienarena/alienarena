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
/** \file
 * \brief FNT_FontFace class declarations
 * \note Initially generated from ref_gl/fnt/fontface.cdf
 */
#ifndef __H_REF_GL_FNT_FONTFACE
#define __H_REF_GL_FNT_FONTFACE

#include "qcommon/objects.h"


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
/*@{*/

/** \brief Maximal length of a face's name */
#define FNT_FACE_NAME_LEN 48


/* Forward declaration of instance types */
struct FNT_FontFace_s;
typedef struct FNT_FontFace_s * FNT_FontFace;

/** \brief Class structure for the FNT_FontFace class
 */
struct FNT_FontFace_cs
{
	/** \brief Parent class record */
	struct OOL_Object_cs parent;

	/** \brief Face loader
	 *
	 * This abstract method must be overridden to perform the actual
	 * loading of a font. It is called by the initialisation method.
	 *
	 * \returns true if the font was loaded, false otherwise.
	 */
	qboolean (* Load)( FNT_FontFace object );

	/** \brief Face unloader
	 *
	 * This abstract method must be overridden to unload the font face.
	 * It is called from the destructor.
	 */
	void (* Unload)( FNT_FontFace object );
};

/** \brief Instance structure for the FNT_FontFace class
 */
struct FNT_FontFace_s
{
	/** \brief Parent instance */
	struct OOL_Object_s parent;

	/** \brief Base directory
	 *
	 * This property indicates the font's base directory. If it is left
	 * empty, it will be initialised to "fonts".
	 */
	char base_directory[ MAX_OSPATH + 1 ];

	/** \brief Font face name
	 *
	 * This property contains the name of the face to load.
	 */
	char name[ FNT_FACE_NAME_LEN + 1 ];

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
};


/** \brief Defining function for the FNT_FontFace class
 *
 * Initialise the class' definition if needed.
 *
 * \return the class' definition
 */
OOL_Class FNT_FontFace__Class( );


/** \brief Font loader with path
 *
 * Attempt to load the face from the specified directory. Try a
 * TrueType face first, and a bitmap one if that fails.
 *
 * \param name the name of the font face
 * \param directory the name of the directory to load from
 * \returns the loaded font face or NULL on failure
 */
OOL_Object FNT_FontFace_GetFrom( const char * name , const char * directory );


/** \brief Font destruction callback
 *
 * This method is called by the fonts' destructor to notify that it is being
 * destroyed. It will remove the font from the fonts cache.
 *
 * \param font the font being destroyed
 */
void FNT_FontFace_NotifyFontDestroy( OOL_Object font );


/** \brief Default font loader
 *
 * Attempt to load the face from the default font directory. Try a
 * TrueType face first, and a bitmap one if that fails.
 *
 * \param name the name of the font face
 * \returns the loaded font face or NULL on failure
 */
static inline OOL_Object FNT_FontFace_Get( const char * name )
{
	return FNT_FontFace_GetFrom( name , "" );
}


/** \brief Wrapper for the \link FNT_FontFace_cs::Load Load \endlink method */
static inline qboolean FNT_FontFace_Load( OOL_Object object )
{
	assert( OOL_GetClassAs( object , FNT_FontFace )->Load != NULL );
	return OOL_GetClassAs( object , FNT_FontFace )->Load( OOL_Cast( object , FNT_FontFace ) );
}


/** \brief Wrapper for the \link FNT_FontFace_cs::Unload Unload \endlink method */
static inline void FNT_FontFace_Unload( OOL_Object object )
{
	assert( OOL_GetClassAs( object , FNT_FontFace )->Unload != NULL );
	OOL_GetClassAs( object , FNT_FontFace )->Unload( OOL_Cast( object , FNT_FontFace ) );
}


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
OOL_Object FNT_FontFace_GetFont( FNT_FontFace object , int size , qboolean force_fixed );


/*@}*/


#endif /*__H_REF_GL_FNT_FONTFACE*/
