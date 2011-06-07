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
 * @brief RGL::EngineAware class implementation
 */

#include "rgl/engineaware.h"


using namespace RGL;

LST_item_s EngineAware::ea_objects = {
	&EngineAware::ea_objects ,
	&EngineAware::ea_objects
};
bool EngineAware::renderer_status = false;


EngineAware::EngineAware( )
	: eal_item( this )
{
	SLST_Append( EngineAware::ea_objects , this->eal_item.head );
}


EngineAware::~EngineAware( )
{
	SLST_Remove( this->eal_item.head );
}


void EngineAware::SetStatus( bool status )
{
	if ( status == EngineAware::renderer_status ) {
		return;
	}

	struct LST_item_s * iterator;
	LST_Foreach( EngineAware::ea_objects , iterator ) {
		EngineAware * obj;
		obj = ( ( COM::List<EngineAware> *) iterator )->item;
		if ( status ) {
			obj->OnRendererStart( );
		} else {
			obj->OnRendererStop( );
		}
	}
	EngineAware::renderer_status = status;
}
