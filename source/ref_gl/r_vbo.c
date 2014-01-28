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
GLuint eboId = 0;
int	totalVBObufferSize;
int totalEBObufferSize;
int currVertexNum, currElemNum;
size_t vbo_xyz_pos;

GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);

void VB_VCInit();
void R_LoadVBOSubsystem(void)
{
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
			VB_VCInit();
		}
	}
	else
	{
		Com_Error (ERR_FATAL, "...GL_ARB_vertex_buffer_object not found\n");
	}
}

void VB_BuildSurfaceVBO(msurface_t *surf)
{
	glpoly_t *p = surf->polys;
	float	*v;
	int		i, j;
	int		n;
	int		trinum;
	float map[MAX_VBO_XYZs];
	int		xyz_size;
	
	// XXX: for future reference, the glDrawRangeElements code was last seen
	// here at revision 3246.
	for (trinum = 1, n = 0; trinum < p->numverts-1; trinum++)
	{
		v = p->verts[0];
		
		// copy in vertex data
		for (j = 0; j < 7; j++)
			map[n++] = v[j];
		
		for (i = trinum; i < trinum+2; i++)
		{
			v = p->verts[i];
		
			// copy in vertex data
			for (j = 0; j < 7; j++)
				map[n++] = v[j];
		}
	}
	
	xyz_size = n*sizeof(float);

	surf->has_vbo = true;
	surf->vbo_first_vert = currVertexNum;
	surf->vbo_num_verts = 3*(p->numverts-2);
	currVertexNum += surf->vbo_num_verts;
	
	qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vbo_xyz_pos, xyz_size, &map);

	vbo_xyz_pos += xyz_size;
}

void VB_BuildWorldVBO(void)
{
	msurface_t *surf, *surfs;
	int i, firstsurf, lastsurf;
	
	currVertexNum = currElemNum = 0;
	vbo_xyz_pos = 0;

	qglGenBuffersARB(1, &vboId);
		
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	// just to keep the lines of code short
	surfs = r_worldmodel->surfaces;
	firstsurf = 0;
	lastsurf = r_worldmodel->numsurfaces;
	
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		if (currentmodel->unique_texinfo[i]->flags & (SURF_SKY|SURF_NODRAW))
			continue;
		for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
		{
			if (	(currentmodel->unique_texinfo[i] != surf->texinfo->equiv) ||
					(surf->iflags & ISURF_PLANEBACK))
				continue;
			VB_BuildSurfaceVBO(surf);
		}
		for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
		{
			if (	(currentmodel->unique_texinfo[i] != surf->texinfo->equiv) ||
					!(surf->iflags & ISURF_PLANEBACK))
				continue;
			VB_BuildSurfaceVBO(surf);
		}
	}

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void VB_BuildVBOBufferSize(msurface_t *surf)
{
	glpoly_t *p = surf->polys;
	
	totalVBObufferSize += 7*3*(p->numverts-2);
}

void GL_SetupWorldVBO (void)
{
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);

	R_VertexPointer (3, 7*sizeof(float), (void *)0);
	R_TexCoordPointer (0, 7*sizeof(float), (void *)(3*sizeof(float)));
	R_TexCoordPointer (1, 7*sizeof(float), (void *)(5*sizeof(float)));
	
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void GL_BindVBO(vertCache_t *cache)
{
	if (cache) 
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, cache->id);
	else
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void GL_BindIBO(vertCache_t *cache)
{
	if (cache) 
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, cache->id);
	else
		qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
}

vertCache_t *R_VCFindCache(vertStoreMode_t store, model_t *mod)
{
	vertCache_t	*cache;

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = cache->next)
	{
		if (cache->mod == mod && cache->store == store)
			return cache;
	}

	return NULL;
}

vertCache_t *R_VCLoadData(vertCacheMode_t mode, int size, void *buffer, vertStoreMode_t store, model_t *mod)
{
	vertCache_t *cache;

	if (!vcm.freeVertCache)
		Com_Error(ERR_FATAL, "VBO cache overflow\n");
	
	cache = vcm.freeVertCache;
	cache->mode = mode;
	cache->size = size;
	cache->pointer = buffer;
	cache->store = store;
	cache->mod = mod;
	
	// link
	vcm.freeVertCache = vcm.freeVertCache->next;

	cache->next = vcm.activeVertCache.next;
	cache->prev = &vcm.activeVertCache;

	vcm.activeVertCache.next->prev = cache;
	vcm.activeVertCache.next = cache;

	if(store == VBO_STORE_INDICES)
		GL_BindIBO(cache);
	else
		GL_BindVBO(cache);

	switch (cache->mode)
	{
		case VBO_STATIC:
			if(store == VBO_STORE_INDICES)
				qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, cache->size, cache->pointer, GL_STATIC_DRAW_ARB);
			else
				qglBufferDataARB(GL_ARRAY_BUFFER_ARB, cache->size, cache->pointer, GL_STATIC_DRAW_ARB);
			break;
		case VBO_DYNAMIC:
			if(store == VBO_STORE_INDICES)
				qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, cache->size, cache->pointer, GL_DYNAMIC_DRAW_ARB);
			else
				qglBufferDataARB(GL_ARRAY_BUFFER_ARB, cache->size, cache->pointer, GL_DYNAMIC_DRAW_ARB);
			break;
	}

	if(store == VBO_STORE_INDICES)
		GL_BindIBO(NULL);
	else
		GL_BindVBO(NULL);	

	return cache;
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

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;
		if (cache->mode != VBO_STATIC)
			R_VCFree(cache);
	}
}


void VB_WorldVCInit()
{
	//clear out previous buffer
	qglDeleteBuffersARB(1, &vboId);
	totalVBObufferSize = 0;	
}

void VB_VCInit()
{
	int	i;
	
	VB_WorldVCInit ();

	for (i=0; i<MAX_VERTEX_CACHES; i++)
	{
		if(vcm.vertCacheList[i].id)
			qglDeleteBuffersARB(1, &vcm.vertCacheList[i].id);
	}

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

	//delete buffers
	qglDeleteBuffersARB(1, &vboId);
	
	for (i=0; i<MAX_VERTEX_CACHES; i++)
	{
		if(vcm.vertCacheList[i].id)
			qglDeleteBuffersARB(1, &vcm.vertCacheList[i].id);
	}

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;
		R_VCFree(cache);
	}
}
