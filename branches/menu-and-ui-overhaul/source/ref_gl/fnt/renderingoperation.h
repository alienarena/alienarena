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
 * @brief FNT_RenderingOperation class declarations
 * @note Initially generated from ref_gl/fnt/renderingoperation.cdf
 */
#ifndef __H_REF_GL_FNT_RENDERINGOPERATION
#define __H_REF_GL_FNT_RENDERINGOPERATION

#include "qcommon/objects.h"
#include "qcommon/lists.h"


/** @defgroup refgl_fnt_renderingoperation Font rendering operation
 * @ingroup refgl_fnt
 *
 * This class contains the results of the execution of the renderer for
 * specific parameters. These results are stored as a set of rendering
 * steps, which can then be executed in bulk without additional computations.
 */
/*@{*/


/* Forward declaration of instance types */
struct FNT_RenderingOperation_s;
typedef struct FNT_RenderingOperation_s * FNT_RenderingOperation;

/** @brief Class structure for the FNT_RenderingOperation class
 */
struct FNT_RenderingOperation_cs
{
	/** @brief Parent class record */
	struct OOL_Object_cs parent;
};

/** @brief Instance structure for the FNT_RenderingOperation class
 */
struct FNT_RenderingOperation_s
{
	/** @brief Parent instance */
	struct OOL_Object_s parent;

	/** @brief Rendering steps list head
	 *
	 * The head of the list of rendering steps that constitute a
	 * rendering operation.
	 */
	struct LST_item_s _steps;

	/** @brief Base height
	 *
	 * The height of the first line, as that cannot be determined from the
	 * steps.
	 */
	int _base_height;

	/** @brief Total vertical translation
	 *
	 * The sum of all vertical translations; combined with the base height
	 * it can be used to determine the text's full height.
	 */
	double _total_y_trans;
};


/** @brief Defining function for the FNT_RenderingOperation class
 *
 * Initialise the class' definition if needed.
 *
 * @return the class' definition
 */
OOL_Class FNT_RenderingOperation__Class( );


/** @brief Clear the list of steps
 *
 * This method clears the list of rendering steps by re-inserting it
 * completely into the free list.
 *
 * @param object the object
 */
void FNT_RenderingOperation_Clear( FNT_RenderingOperation object );


/** @brief Append a new rendering step
 *
 * This method appends a new rendering step to the list, using the
 * parameters to initialise the step's structure. The step's structure
 * is taken from the free list; if the free list is empty, it is
 * expanded.
 *
 * @param object the object
 * @param type the bitmask for the type of operation
 * @param color the color to apply
 * @param gl_list_id the identifier of the GL list to execute
 * @param x the horizontal translation
 * @param y the vertical translation
 */
void FNT_RenderingOperation_AddStep( FNT_RenderingOperation object ,
		int type , float * color , int gl_list_id , double x ,
		double y );


/** @brief Update base height
 *
 * @param object the object
 * @param base_height the height of the first line
 */
void FNT_RenderingOperation_SetBaseHeight( FNT_RenderingOperation object ,
		int base_height );


/** @brief Obtain the text's heigh
 *
 * @param object the object
 *
 * @return the total height of the text
 */
int FNT_RenderingOperation_GetHeight( FNT_RenderingOperation object );


/** @brief Draw the text
 *
 * This method draws the text at a specified location, using a window
 * which clips the text as necessary. Rendering steps which are
 * totally outside the window will be skipped; GL clipping will be
 * used to draw the rest.
 *
 * @param object the object
 * @param x the horizontal coordinate on the screen at which drawing
 *	must start
 * @param y the vertical coordinate on the screen at which drawing
 *	must start
 * @param win_x the horizontal coordinate of the window's top/left
 *	corner relative to the text's full area
 * @param win_y the vertical coordinate of the window's top/left
 *	corner relative to the text's full area
 * @param win_w the window's width
 * @param win_h the window's height
 * @param alpha the alpha value to use when drawing the text
 */
void FNT_RenderingOperation_Draw( FNT_RenderingOperation object , int x ,
		int y , int win_x , int win_y , int win_w , int win_h ,
		float alpha );


/*@}*/


#endif /*__H_REF_GL_FNT_RENDERINGOPERATION*/
