/*
Copyright (C) 2004-2009 Berserker.

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
// r_vbo.c: vertex buffer object managment

#include "r_local.h"


GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);

void GL_BindVBO(vertCache_t *cache)
{
	if (cache)
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, cache->id);
	else
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

vertCache_t *R_VCFindCache(vertStoreMode_t store, entity_t *ent)
{
	model_t		*mod;
	int			frame;
	float		backlerp;
	vec3_t		angles, orgs;
	vertCache_t	*cache, *next;

	if (store == VBO_STORE_XYZ)
	{
		mod = ent->model;
		frame = ent->frame;
		if ( r_lerpmodels->value )
			backlerp = ent->backlerp;
		else
			backlerp = 0;
		angles[0] = ent->angles[0];
		angles[1] = ent->angles[1];
		angles[2] = ent->angles[2];
		orgs[0] = ent->origin[0];
		orgs[1] = ent->origin[1];
		orgs[2] = ent->origin[2];
		for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
		{
			next = cache->next;
			if (backlerp)
			{	//  oldorigin è oldframe.
				if (cache->store == store && cache->mod == mod && cache->frame == frame && cache->backlerp == backlerp && cache->angles[0] == angles[0] && cache->angles[1] == angles[1] && cache->angles[2] == angles[2] && cache->origin[0] == orgs[0] && cache->origin[1] == orgs[1] && cache->origin[2] == orgs[2])
				{	// already cached!
					GL_BindVBO(cache);
					return cache;
				}
			}
			else
			{	
				if (cache->store == store && cache->mod == mod && cache->frame == frame && cache->angles[0] == angles[0] && cache->angles[1] == angles[1] && cache->angles[2] == angles[2])
				{	// already cached!
					GL_BindVBO(cache);
					return cache;
				}
			}
		}
	}
	else if (store == VBO_STORE_NORMAL || store == VBO_STORE_BINORMAL || store == VBO_STORE_TANGENT)
	{
		mod = ent->model;
		frame = ent->frame;
		if ( r_lerpmodels->value )
			backlerp = ent->backlerp;
		else
			backlerp = 0;
		for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
		{
			next = cache->next;
			if (cache->store == store && cache->mod == mod && cache->frame == frame && cache->backlerp == backlerp)
			{	// already cached!
				GL_BindVBO(cache);
				return cache;
			}
		}
	}

	return NULL;
}


vertCache_t *R_VCLoadData(vertCacheMode_t mode, int size, void *buffer, vertStoreMode_t store, entity_t *ent)
{
	vertCache_t *cache;

	if (!vcm.freeVertCache)
		Com_Error(ERR_FATAL, "VBO cache overflow\n");

	cache = vcm.freeVertCache;
	cache->mode = mode;
	cache->size = size;
	cache->pointer = buffer;
	cache->store = store;
	if (store != VBO_STORE_ANY)
	{
		cache->mod = ent->model;
		cache->frame = ent->frame;
		cache->backlerp = ent->backlerp;
		cache->angles[0] = ent->angles[0];
		cache->angles[1] = ent->angles[1];
		cache->angles[2] = ent->angles[2];
		cache->origin[0] = ent->origin[0];
		cache->origin[1] = ent->origin[1];
		cache->origin[2] = ent->origin[2];
	}

	// link
	vcm.freeVertCache = vcm.freeVertCache->next;

	cache->next = vcm.activeVertCache.next;
	cache->prev = &vcm.activeVertCache;

	vcm.activeVertCache.next->prev = cache;
	vcm.activeVertCache.next = cache;

	GL_BindVBO(cache);

	switch (cache->mode)
	{
		case VBO_STATIC:
			qglBufferDataARB(GL_ARRAY_BUFFER_ARB, cache->size, cache->pointer, GL_STATIC_DRAW_ARB);
		case VBO_DYNAMIC:
			qglBufferDataARB(GL_ARRAY_BUFFER_ARB, cache->size, cache->pointer, GL_DYNAMIC_DRAW_ARB);
	}

	return cache;
}


vertCache_t *R_VCCreate(vertCacheMode_t mode, int size, void *buffer, vertStoreMode_t store, entity_t *ent, int mesh)
{
	vertCache_t	*cache;

	cache = R_VCFindCache(store, ent);
	if (cache)
		return cache;

	return R_VCLoadData(mode, size, buffer, store, ent);
}


void R_VCFree(vertCache_t *cache)
{
	if (!cache)
		return;

	// unlink
	cache->prev->next = cache->next;
	cache->next->prev = cache->prev;

	cache->next = vcm.freeVertCache;
	vcm.freeVertCache = cache;
}

void R_VCInit()
{
	int	i;

	if (!gl_state.vbo)
		return;

	Com_Printf(S_COLOR_GREEN"...Initializing VBO cache\n");

	memset(&vcm, 0, sizeof(vcm));

	// setup the linked lists
	vcm.activeVertCache.next = &vcm.activeVertCache;
	vcm.activeVertCache.prev = &vcm.activeVertCache;

	vcm.freeVertCache = vcm.vertCacheList;

	for (i=0; i<MAX_VERTEX_CACHES-1; i++)
		vcm.vertCacheList[i].next = &vcm.vertCacheList[i+1];

	for (i=0; i<MAX_VERTEX_CACHES; i++)
		qglGenBuffersARB(1, &vcm.vertCacheList[i].id);
}

/*
===============
R_VCFreeFrame

Deletes all non-STATIC buffers from the previous frame.
PS: VBO_STATIC using for ST texture/skin coordinates and static shadow volumes
===============
*/
void R_VCFreeFrame()
{
	vertCache_t	*cache, *next;

	if (!gl_state.vbo)
		return;

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;
		if (cache->mode != VBO_STATIC)
			R_VCFree(cache);
	}
}

void R_VCShutdown()
{
	int			i;
	vertCache_t	*cache, *next;

	if (!gl_state.vbo)
		return;

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;
		R_VCFree(cache);
	}

	for (i=0; i<MAX_VERTEX_CACHES; i++)
		qglDeleteBuffersARB(1, &vcm.vertCacheList[i].id);
}
