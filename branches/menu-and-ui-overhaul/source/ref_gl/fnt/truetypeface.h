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
 * \brief FNT_TrueTypeFace class declarations
 * \note Initially generated from ref_gl/fnt/truetypeface.cdf
 */
#ifndef __H_REF_GL_FNT_TRUETYPEFACE
#define __H_REF_GL_FNT_TRUETYPEFACE

#include "ref_gl/fnt/fontface.h"

#include <ft2build.h>
#include FT_FREETYPE_H


/** \defgroup refgl_fnt_truetypeface TrueType font face
 * \ingroup refgl_fnt
 *
 * This class implements support for TrueType faces. It initialises the
 * FreeType library as required, and is capable of verifying that a TrueType
 * face can be used by the engine when loading it.
 */
/*@{*/


/* Forward declaration of instance types */
struct FNT_TrueTypeFace_s;
typedef struct FNT_TrueTypeFace_s * FNT_TrueTypeFace;

/** \brief Class structure for the FNT_TrueTypeFace class
 */
struct FNT_TrueTypeFace_cs
{
	/** \brief Parent class record */
	struct FNT_FontFace_cs parent;

	/** \brief FreeType library handle */
	FT_Library library;

	/** \brief Subpixel rendering control
	 *
	 * This CVar controls sub-pixel rendering. It is ignored if the FreeType
	 * library is not compiled with sub-pixel rendering support.
	 */
	cvar_t * subpixel;

	/** \brief Auto-hinting control
	 *
	 * This CVar controls whether the FreeType library should force fonts
	 * to use the auto-hinter, even if the face includes hints and hinting is
	 * supported by the library.
	 */
	cvar_t * autohint;
};

/** \brief Instance structure for the FNT_TrueTypeFace class
 */
struct FNT_TrueTypeFace_s
{
	/** \brief Parent instance */
	struct FNT_FontFace_s parent;

	/** \brief TrueType file size */
	unsigned int file_size;

	/** \brief TrueType file contents */
	void * file_data;
};


/** \brief Defining function for the FNT_TrueTypeFace class
 *
 * Initialise the class' definition if needed.
 *
 * \return the class' definition
 */
OOL_Class FNT_TrueTypeFace__Class( );


/*@}*/


#endif /*__H_REF_GL_FNT_TRUETYPEFACE*/
