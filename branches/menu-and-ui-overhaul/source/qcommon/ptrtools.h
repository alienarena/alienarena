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
 * \brief \link qcommon_ptrtools Pointer tools \endlink declarations
 */

#ifndef __H_QCOMMON_PTRTOOLS
#define __H_QCOMMON_PTRTOOLS

#include "qcommon/qcommon.h"


/** \defgroup qcommon_ptrtools Pointer tools
 * \ingroup qcommon
 *
 * A set of macros which can be used to manipulate pointers; this includes
 * macros to obtain the size or position of a structure's field.
 */
/*@{*/


/** \brief Determine the offset of a field inside a structure
 *
 * Determine the offset of a field inside a structure by using the NULL
 * pointer as a reference point.
 *
 * \param TYPE the composite type in which to look
 * \param FIELD the name of the field
 *
 * \param the offset of the field relative to the start of the structure.
 */
#define PTR_FieldOffset(TYPE,FIELD) \
	( (char *)( &( ( (TYPE *) NULL )->FIELD ) ) - (char *) NULL )


/** \brief Determine the size of a field inside a structure
 *
 * Determine the size of a field inside a structure by measuring the size of
 * the field on the casted NULL pointer.
 *
 * \param TYPE the composite type in which to look
 * \param FIELD the name of the field
 *
 * \param the size of the field
 */
#define PTR_FieldSize(TYPE,FIELD) \
	sizeof( ( (TYPE *) NULL )->FIELD )


/** \brief Compute an address from a base and an offset
 *
 * Compute an address from a base address and an offset by temporarily
 * casting the base address to a byte pointer.
 *
 * \param BASE base address
 * \param OFFSET offset (bytes)
 *
 * \returns the modified address as a void* pointer
 */
#define PTR_OffsetAddr(BASE,OFFSET) \
	( (void *)( ( (char *)( BASE ) ) + (OFFSET) ) )


/*@}*/
#endif // __H_QCOMMON_PTRTOOLS
