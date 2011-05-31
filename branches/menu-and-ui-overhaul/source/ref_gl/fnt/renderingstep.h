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
 * @brief Text rendering step definition
 */
#ifndef __H_REF_GL_FNT_RENDERINGSTEP
#define __H_REF_GL_FNT_RENDERINGSTEP

#include "qcommon/lists.h"

/** @defgroup refgl_fnt_renderingstep Rendering step structure and constants
 * @ingroup refgl_fnt
 *
 * When text is being rendered through a FNT_Renderer, a set of predefined
 * operations is generated.
 */
/*@{*/

/** @brief Rendering step structure
 *
 * This structure describe a step of font rendering. Each step may be composed
 * of a color change, a display operation, and a translation.
 */ 
struct FNT_rendering_step_s
{
	/** @brief List entry
	 *
	 * Used to create lists of rendering steps.
	 */
	struct LST_item_s	list;

	/** @brief Operation types
	 *
	 * A bit mask that indicates which operations should be applied
	 * at this step.
	 */
	int			op_type;

	/** @brief Color
	 *
	 * If the current color must be changed at this step, this field
	 * contains the R, G, B and A components of the color to switch
	 * to.
	 */
	float			color[4];

	/** @brief GL list identifier
	 *
	 * If there is something to draw at this step, this field contains
	 * the identifier of the GL list that must be executed.
	 */
	int			gl_list_exec;

	/** @brief Horizontal movement
	 *
	 * The horizontal movement to apply after any drawing is done.
	 */
	double			x_move;

	/** @brief Vertical movement
	 *
	 * The vertical movement to apply after any drawing is done.
	 */
	double			y_move;
};


/** @brief Color change operation */
#define FNT_RNSTEP_SETCOLOR	1

/** @brief Drawing operation */
#define FNT_RNSTEP_DRAW		2

/** @brief Translation */
#define FNT_RNSTEP_MOVE		4


/*@}*/


#endif /*__H_REF_GL_FNT_RENDERINGSTEP*/
