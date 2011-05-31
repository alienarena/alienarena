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
#include "ref_gl/fnt/renderingstep.h"


/** @defgroup refgl_fnt_renderingoperation Font rendering operation
 * @ingroup refgl_fnt
 *
 * This class contains the results of the execution of the renderer for
 * specific parameters. These results are stored as a set of rendering
 * steps, which can then be executed in bulk without additional computations.
 */
class FNT_RenderingOperation
{
private:
	/** @brief List of free rendering steps
	 *
	 * This list is used to store allocated rendering step structures
	 * which are not being used at the moment. It avoids having to
	 * reallocate rendering step structures all the time.
	 */
	static struct LST_item_s free_steps = { NULL , NULL };

	/** @brief Expand the list of free rendering steps
	 *
	 * This method is called when the list of free steps is found to be
	 * empty; it will allocate new entries and add them to the list.
	 */
	static void ExpandFreeList( );

	/** @brief Rendering steps list head
	 *
	 * The head of the list of rendering steps that constitute a
	 * rendering operation.
	 */
	struct LST_item_s steps;

	/** @brief Base height
	 *
	 * The height of the first line, as that cannot be determined from the
	 * steps.
	 */
	int base_height;

	/** @brief Total vertical translation
	 *
	 * The sum of all vertical translations; combined with the base height
	 * it can be used to determine the text's full height.
	 */
	double total_y_trans;

public:
	/** @brief Rendering operation initialiser
	 *
	 * The initialiser sets up the list head so it indicates an empty list.
	 *
	 * @param object the object being initialised
	 */
	void Initialise( );

	/** @brief Destructor
	 *
	 * The destructor clears the list of steps using the Clear() method.
	 *
	 * @param object the object being destroyed.
	 */
	virtual void Destroy( );

	/** @brief Clear the list of steps
	 *
	 * This method clears the list of rendering steps by re-inserting it
	 * completely into the free list.
	 *
	 * @param object the object
	 */
	void Clear( );

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
	void AddStep( int type , float * color , int gl_list_id , double x , double y );


	/** @brief Update base height
	 *
	 * @param object the object
	 * @param base_height the height of the first line
	 */
	void SetBaseHeight( int base_height );


	/** @brief Obtain the text's heigh
	 *
	 * @param object the object
	 *
	 * @return the total height of the text
	 */
	int GetHeight( );


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
	void Draw( int x , int y , int win_x , int win_y , int win_w ,
		int win_h , float alpha );
};
