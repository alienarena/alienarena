/*
Copyright (C) 2011 COR Entertainment

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

GLuint vboId = 0;
int	vboPosition;
int	totalVBObufferSize;

GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);

void R_LoadVBOSubsystem(void)
{
	gl_state.vbo = false;

	if(!gl_usevbo->value)
		return;

	if (strstr(gl_config.extensions_string, "GL_ARB_vertex_buffer_object"))
	{
		qglBindBufferARB = (void *)qwglGetProcAddress("glBindBufferARB");
		qglDeleteBuffersARB = (void *)qwglGetProcAddress("glDeleteBuffersARB");
		qglGenBuffersARB = (void *)qwglGetProcAddress("glGenBuffersARB");
		qglBufferDataARB = (void *)qwglGetProcAddress("glBufferDataARB");
		qglBufferSubDataARB = (void *)qwglGetProcAddress("glBufferSubDataARB");

		if (qglGenBuffersARB && qglBindBufferARB && qglBufferDataARB && qglDeleteBuffersARB)
		{
			Com_Printf("...using GL_ARB_vertex_buffer_object\n");
			gl_state.vbo = true;
			VB_VCInit();
		}
	} else
	{
		Com_Printf(S_COLOR_RED "...GL_ARB_vertex_buffer_object not found\n");
		gl_state.vbo = false;
	}
}

void VB_BuildSurfaceVBO(msurface_t *surf)
{
	glpoly_t *p = surf->polys;
	float	*v;
	int		i;
	float   *map;
	float	*map2;
	float	*map3;
	int		l, m, n;

	if (gl_state.vbo)
	{
		map = (float*) vbo_shadow;
		map2 = (float*) vbo_shadow2;
		map3 = (float*) vbo_shadow3;
				
		for (v = p->verts[0], i = 0, l = 0, m = 0, n = 0; i < p->numverts; i++, v += VERTEXSIZE)
		{
			// copy in vertex data
			map[n++] = v[0];
			map[n++] = v[1];
			map[n++] = v[2];

			// world texture coords
			map2[l++] = v[3];
			map2[l++] = v[4];

			// lightmap texture coords
			map3[m++] = v[5];
			map3[m++] = v[6];
		}

		surf->vbo_pos = vboPosition;

		surf->xyz_size = n*sizeof(float);
		surf->st_size = l*sizeof(float);
		surf->lm_size = m*sizeof(float);

		surf->has_vbo = true;
		
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vboPosition, surf->xyz_size, &vbo_shadow);                             
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vboPosition + surf->xyz_size, surf->st_size, &vbo_shadow2);                
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vboPosition + surf->xyz_size + surf->st_size, surf->lm_size, &vbo_shadow3);  

		vboPosition += (surf->xyz_size + surf->st_size + surf->lm_size);
	}
}

void VB_BuildWorldVBO(void)
{
	msurface_t *surf;

	qglGenBuffersARB(1, &vboId);
		
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);

	for (surf = &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface]; surf < &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface + r_worldmodel->nummodelsurfaces] ; surf++)
	{
		if (surf->texinfo->flags & SURF_SKY)
		{   // no skies here
			continue;
		}
		else
		{
			if (!( surf->flags & SURF_DRAWTURB ) )
			{
				VB_BuildSurfaceVBO(surf);
			}
		}
	}

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void VB_BuildVBOBufferSize(msurface_t *surf)
{
	glpoly_t *p = surf->polys;

	if (!( surf->flags & SURF_DRAWTURB ) )
	{
		totalVBObufferSize += 7*p->numverts;
	}
}

void GL_BindVBO(vertCache_t *cache)
{
	if (cache) 
	{
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, cache->id);
	}
	else
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void GL_BindIBO(vertCache_t *cache)
{
	if (cache) 
	{
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, cache->id);
	}
	else
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
}

vertCache_t *R_VCFindCache(vertStoreMode_t store, entity_t *ent)
{
	model_t		*mod;
	vertCache_t	*cache, *next;

	mod = ent->model;

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;
#if 0
		if(store == VBO_STORE_SHADOWINDICES || store == VBO_STORE_SHADOWXYZ)
		{
			if (cache->store == store && cache->mod == mod && cache->position[0] == ent->origin[0] && cache->position[1] == ent->origin[1]
				&& cache->position[2] == ent->origin[2])
			{	// already cached!
				if(store == VBO_STORE_SHADOWINDICES)
					GL_BindIBO(cache);
				else
					GL_BindVBO(cache);
				return cache;
			}
		}
		else
#endif
		{
			if (cache->store == store && cache->mod == mod)
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
	cache->mod = ent->model;
	VectorCopy(ent->origin, cache->position);

	// link
	vcm.freeVertCache = vcm.freeVertCache->next;

	cache->next = vcm.activeVertCache.next;
	cache->prev = &vcm.activeVertCache;

	vcm.activeVertCache.next->prev = cache;
	vcm.activeVertCache.next = cache;
#if 0
	if(store == VBO_STORE_SHADOWINDICES)
		GL_BindIBO(cache);
	else
#endif
		GL_BindVBO(cache);

	switch (cache->mode)
	{
		case VBO_STATIC:
#if 0
			if(store == VBO_STORE_SHADOWINDICES)
				qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, cache->size, cache->pointer, GL_STATIC_DRAW_ARB);
			else
#endif
				qglBufferDataARB(GL_ARRAY_BUFFER_ARB, cache->size, cache->pointer, GL_STATIC_DRAW_ARB);
			break;
		case VBO_DYNAMIC:
#if 0
			if(store == VBO_STORE_SHADOWINDICES)
				qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, cache->size, cache->pointer, GL_DYNAMIC_DRAW_ARB);
			else
#endif
				qglBufferDataARB(GL_ARRAY_BUFFER_ARB, cache->size, cache->pointer, GL_DYNAMIC_DRAW_ARB);
			break;
	}

	return cache;
}


vertCache_t *R_VCCreate(vertCacheMode_t mode, int size, void *buffer, vertStoreMode_t store, entity_t *ent)
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

/*
===============
R_VCFreeFrame
Deletes all non-STATIC buffers from the previous frame.
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


void VB_VCInit()
{
	int	i;

	if (!gl_state.vbo)
		return;

	//clear out previous buffer
	qglDeleteBuffersARB(1, &vboId);

	vboPosition = 0;
	totalVBObufferSize = 0;	

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


void R_VCShutdown()
{
	int			i;
	vertCache_t	*cache, *next;

	if (!gl_state.vbo)
		return;

	//delete buffers
	qglDeleteBuffersARB(1, &vboId);

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;
		R_VCFree(cache);
	}

	for (i=0; i<MAX_VERTEX_CACHES; i++)
		qglDeleteBuffersARB(1, &vcm.vertCacheList[i].id);
}
