/*
Copyright (C) 2011 COR Entertainment, LLC.

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
 * @brief C++ wrapper for C lists defined in qcommon/lists.h
 */
#ifndef __H_CPP_COM_LIST
#define __H_CPP_COM_LIST


#include "qcommon/lists.h"

namespace COM {

/** @brief Template-based C++ wrapper for lists
 *
 * This wrapper makes it possible to use LST_* lists in C++ without dirty
 * hacks and hordes of compiler warnings.
 */
template <class T>
struct List
{
	/** @brief List chaining */
	struct LST_item_s	head;

	/** @brief Item this entry refers to */
	T *			item;

	/** @brief Initialise an empty list item */
	List();

	/** @brief Initialise a list item
	 *
	 * @param item the actual object this list item refers to
	 */
	List( T * item );

	/** @brief Remove the item from the list it's attached to */
	~List();
};


template <class T>
inline List<T>::List( )
{
	SLST_Init( this->head );
	this->item = NULL;
}

template <class T>
inline List<T>::List( T * item )
{
	SLST_Init( this->head );
	this->item = item;
}

template <class T>
inline List<T>::~List()
{
	if ( ! SLST_IsEmpty( this->head ) ) {
		SLST_Remove( this->head );
	}
}

};

#endif //__H_CPP_COM_LIST
