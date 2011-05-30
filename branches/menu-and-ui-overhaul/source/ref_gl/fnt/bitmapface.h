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
 * \brief FNT_BitmapFace class declarations
 * \note Initially generated from ref_gl/fnt/bitmapface.cdf
 */
#ifndef __H_REF_GL_FNT_BITMAPFACE
#define __H_REF_GL_FNT_BITMAPFACE

#include "ref_gl/fnt/fontface.h"
#include "ref_gl/r_local.h"


/** \defgroup refgl_fnt_bitmapface Bitmap font face
 * \ingroup refgl_fnt
 *
 * The FNT_BitmapFace class implements the loader and unloader for a bitmap
 * font's face. It consists in a texture that is loaded from the face's full
 * path.
 */
/*@{*/


/* Forward declaration of instance types */
struct FNT_BitmapFace_s;
typedef struct FNT_BitmapFace_s * FNT_BitmapFace;

/** \brief Class structure for the FNT_BitmapFace class
 */
struct FNT_BitmapFace_cs
{
	/** \brief Parent class record */
	struct FNT_FontFace_cs parent;
};

/** \brief Instance structure for the FNT_BitmapFace class
 */
struct FNT_BitmapFace_s
{
	/** \brief Parent instance */
	struct FNT_FontFace_s parent;

	/** \brief Face bitmap
	 *
	 * This texture is the font face's bitmap, which will be used by the
	 * fonts.
	 */
	image_t * texture;
};


/** \brief Defining function for the FNT_BitmapFace class
 *
 * Initialise the class' definition if needed.
 *
 * \return the class' definition
 */
OOL_Class FNT_BitmapFace__Class( );


/*@}*/


#endif /*__H_REF_GL_FNT_BITMAPFACE*/
