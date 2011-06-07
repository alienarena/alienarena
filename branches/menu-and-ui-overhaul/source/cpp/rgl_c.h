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
 * @brief C interfaces for RGL C++ code
 */
#ifndef __H_CPP_RGLC
#define __H_CPP_RGLC

#include "qcommon/qcommon.h"

#ifdef	__cplusplus
extern "C" {
#endif //__cplusplus


/** @brief Set the status of engine-aware objects
 *
 * @param status whether the renderer is starting (true) or being shut down
 *	(false)
 */
extern void RGL_EngineAware_SetStatus( qboolean status );


#ifdef	__cplusplus
}
#endif //__cplusplus

#endif //__H_CPP_RGLC
