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

#include "r_local.h"
#include "ode/ode.h"

//This file will handle all of the ragdoll calculations, etc.

//Notes:

//There are alot of tutorials in which to glean this information.  As best I can tell we will have to do the following

//We will need to get a velocity vector.  This could be done by either calculating the distance from entity origin's last frame, or
//by some other means such as a nearby effect like a rocket explosion to get the vector.  We also may need to figure out which part
//of the ragdoll to apply this velocity to - ie, say an explosion hits their feet rather than their torso.  For starting out, let's 
//concentrate on the torso.

//Once the frames are at the death stage, the ragdoll takes over.

//From what I best understand from reading the tuts, we have to build a "world" in which things collide.  These would be done easily
//by the existing bsp drawing routines, and simply putting some calls before the vertex arrays are built.  

//The ragdoll will need to either be hardcoded in, or created by an external program and read in at load time.  To begin, we will
//hardcode one in, using the tuts.  Since our playermodels use roughly the same skeleton, this shouldn't be too bad.

//So how does the ragoll then animate the mesh?  As best I can tell, we would need to get rotation and location vectors from the
//ragdoll, and apply them to the models skeleton.  We would need to use the same tech we use for player pitch spine bending to 
//manipulate the skeleton.

//This tutorial is straightforward and useful, and though it's in python, it's easily translated into C code.

//There are several examples at http://opende.sourceforge.net/wiki/index.php/HOWTO_rag-doll