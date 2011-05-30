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
/** @file
 * @brief FNT_TrueTypeFont class declarations
 * @note Initially generated from ref_gl/fnt/truetypefont.cdf
 */
#ifndef __H_REF_GL_FNT_TRUETYPEFONT
#define __H_REF_GL_FNT_TRUETYPEFONT

#include "ref_gl/fnt/font.h"


/** @defgroup refgl_fnt_truetypefont TrueType font
 * @ingroup refgl_fnt
 *
 * The FNT_TrueTypeFont class implements the loader and unloader for a TrueType
 * font.
 */
/*@{*/


/* Forward declaration of instance types */
struct FNT_TrueTypeFont_s;
typedef struct FNT_TrueTypeFont_s * FNT_TrueTypeFont;

/** @brief Class structure for the FNT_TrueTypeFont class
 */
struct FNT_TrueTypeFont_cs
{
	/** @brief Parent class record */
	struct FNT_Font_cs parent;
};

/** @brief Instance structure for the FNT_TrueTypeFont class
 */
struct FNT_TrueTypeFont_s
{
	/** @brief Parent instance */
	struct FNT_Font_s parent;

	/** @brief Bitmap of the font */
	unsigned char * _bitmap;

	/** @brief Rendering mode */
	unsigned int _render_mode;
};


/** @brief Defining function for the FNT_TrueTypeFont class
 *
 * Initialise the class' definition if needed.
 *
 * @return the class' definition
 */
OOL_Class FNT_TrueTypeFont__Class( );


/*@}*/


#endif /*__H_REF_GL_FNT_TRUETYPEFONT*/
