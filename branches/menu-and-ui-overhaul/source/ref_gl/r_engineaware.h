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
 * \brief R_EngineAware class declarations
 * \note Initially generated from ref_gl/r_engineaware.cdf
 */
#ifndef __H_REF_GL_R_ENGINEAWARE
#define __H_REF_GL_R_ENGINEAWARE

#include "qcommon/objects.h"
#include "qcommon/lists.h"


/** \defgroup Engine-aware objects
 * \ingroup ref_gl
 *
 * This abstract class is used to create objects which are kept informed of
 * the rendering engine's initialisation and shutdown. A list of all
 * such objects is kept and their OnVidStart()/OnVidStop() methods are called
 * as necessary.
 */
/*@{*/


/* Forward declaration of instance types */
struct R_EngineAware_s;
typedef struct R_EngineAware_s * R_EngineAware;

/** \brief Class structure for the R_EngineAware class
 */
struct R_EngineAware_cs
{
	/** \brief Parent class record */
	struct OOL_Object_cs parent;

	/** \brief Rendering engine start handler */
	void (* OnRendererStart)( R_EngineAware object );

	/** \brief Rendering engine stop handler */
	void (* OnRendererStop)( R_EngineAware object );
};

/** \brief Instance structure for the R_EngineAware class
 */
struct R_EngineAware_s
{
	/** \brief Parent instance */
	struct OOL_Object_s parent;

	/** \brief List entry
	 *
	 * This field is used to store an engine-aware object's list data.
	 */
	struct LST_item_s _eal_item;
};


/** \brief Defining function for the R_EngineAware class
 *
 * Initialise the class' definition if needed.
 *
 * \return the class' definition
 */
OOL_Class R_EngineAware__Class( );


/** \brief Set the status of the rendering engine
 *
 * This method sets the status of the rendering engine and calls the
 * appropriate handler for all known engine-aware objects.
 *
 * \param status true if the engine is starting, false if it is being
 * shut down.
 */
void R_EngineAware_SetStatus( qboolean status );


/** \brief Wrapper for the \link R_EngineAware_cs::OnRendererStart OnRendererStart \endlink method */
static inline void R_EngineAware_OnRendererStart( OOL_Object object )
{
	assert( OOL_GetClassAs( object , R_EngineAware )->OnRendererStart != NULL );
	OOL_GetClassAs( object , R_EngineAware )->OnRendererStart( OOL_Cast( object , R_EngineAware ) );
}


/** \brief Wrapper for the \link R_EngineAware_cs::OnRendererStop OnRendererStop \endlink method */
static inline void R_EngineAware_OnRendererStop( OOL_Object object )
{
	assert( OOL_GetClassAs( object , R_EngineAware )->OnRendererStop != NULL );
	OOL_GetClassAs( object , R_EngineAware )->OnRendererStop( OOL_Cast( object , R_EngineAware ) );
}


/*@}*/


#endif /*__H_REF_GL_R_ENGINEAWARE*/
