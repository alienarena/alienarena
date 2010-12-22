/*
Copyright (C) 2010 COR Entertainment

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
 * \brief \link qcommon_lists Lists \endlink
 */

#ifndef __H_QCOMMON_LISTS
#define __H_QCOMMON_LISTS

#include "qcommon/ptrtools.h"


/** \defgroup qcommon_ptrtools Doubly-linked, guarded circular lists
 * \ingroup qcommon
 *
 * A set of type definitions, inline functions and macros that implement
 * generic lists.
 */
/*@{*/


/** \brief A list item
 *
 * A list item, allowing a doubly-linked list to be created. This structure
 * must be used as the type for a structure's list field, or on its own as
 * the list's head and sentinel.
 */
struct LST_item_s {
	/** \brief Previous element in the list */
	struct LST_item_s *	previous;

	/** \brief Next element in the list */
	struct LST_item_s *	next;
};


/** \brief Initialise a list item
 *
 * Initialise a list item by setting it to loop upon itself.
 *
 * \param item a pointer to the list's item.
 */
static inline void LST_Init( struct LST_item_s * item )
{
	item->previous = item->next = item;
}


/** \brief Insert an item
 *
 * Add an item to a list after another item. If the base item is the list's
 * head, then the item will be inserted at the beginning of the list.
 *
 * \param base a pointer to the item after which the insertion will take place
 * \param item a pointer to the item to insert
 */
static inline void LST_Insert( struct LST_item_s * base , struct LST_item_s * item )
{
	item->next = base->next;
	item->previous = base;
	item->next->previous = base->next = item;
}


/** \brief Append an item
 *
 * Add an item before another item. If the base item is the list's head, then
 * the item will be inserted at the end of the list.
 *
 * \param base a pointer to the item before which the insertion will take
 * place
 * \param item a pointer to the item to insert
 */
static inline void LST_Append( struct LST_item_s * base , struct LST_item_s * item )
{
	item->previous = base->previous;
	item->next = base;
	base->previous = item->previous->next = item;
}


/** \brief Remove an item
 *
 * Remove a list item from the list it is in.
 *
 * \param item a pointer to the item to remove from the list it's in
 */
static inline void LST_Remove( struct LST_item_s * item )
{
	item->previous->next = item->next;
	item->next->previous = item->previous;
	item->next = item->previous = item;
}


/** \brief Remove the first item from a list and return it
 *
 * The first item from the list will be removed and returned. If the list
 * was empty, then NULL will be returned and no action will be taken.
 *
 * \param list a pointer to the list's sentry
 * \returns a pointer to list entry that was removed, or NULL if the list was
 * empty
 */
static inline struct LST_item_s * LST_TakeFirst( struct LST_item_s * list )
{
	struct LST_item_s * f;
	if ( list->next == list ) {
		return NULL;
	}
	LST_Remove( f = list->next );
	return f;
}


/** \brief Remove the first item from a list
 *
 * If the list is not empty, its first item will be removed.
 *
 * \param list a pointer to the list's head
 * \returns true if an item was removed, false if the list was empty
 */
#define LST_RemoveFirst(list) \
	( LST_TakeFirst( list ) != NULL )



/** \brief Next item in list
 *
 * \param head a pointer to the list's head
 * \param from a pointer to the list's item
 *
 * \returns the list's next item or NULL if the end of the list has been reached.
 */
static inline void * LST_GetNext( struct LST_item_s * head , struct LST_item_s * from )
{
	if ( from->next == head ) {
		return NULL;
	}
	return from->next;
}


/** \brief Previous item in the list
 *
 * \param head a pointer to the list's head
 * \param from a pointer to the list's item
 *
 * \returns the list's previous item or NULL if the start of the list has been reached.
 */
static inline void * LST_GetPrevious( struct LST_item_s * head , struct LST_item_s * from )
{
	if ( from->previous == head ) {
		return NULL;
	}
	return from->previous;
}


/** \brief Check whether a list is empty
 *
 * \param list pointer to the list's head
 *
 * \return true if the list is empty, false otherwise
 */
#define LST_IsEmpty(list) \
	( list->next == list )


/** \brief Iterate over a list's items
 *
 * Iterate over a list's item; once the loop is finished, the iterator variable
 * points to the list head.
 *
 * \param head the list's head
 * \param iterator the name of the variable to use as the iterator
 */
#define LST_Foreach(head, iterator) \
	iterator = &(head); \
	while ( ((iterator) = (iterator)->next) != &(head) )


/*@}*/
#endif /*__H_QCOMMON_LISTS*/
