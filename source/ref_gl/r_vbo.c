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
int currVertexNum;
size_t	vbo_xyz_base, vbo_xyz_pos, 
		vbo_st_base, vbo_st_pos, 
		vbo_lm_base, vbo_lm_pos;

GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);

void R_LoadVBOSubsystem(void)
{
	gl_state.vbo = false;

	if(!gl_usevbo->integer)
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
	int		l, m, n;
	int		trinum;
	float map[MAX_VBO_XYZs];
	float map2[MAX_VBO_XYZs];
	float map3[MAX_VBO_XYZs];
	int		xyz_size, st_size, lm_size;
	
	if (gl_state.vbo)
	{
		for (trinum = 1, l = 0, m = 0, n = 0; trinum < p->numverts-1; trinum++)
		{
			v = p->verts[0];
			
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
			
			for (v = p->verts[trinum], i = 0; i < 2; i++, v += VERTEXSIZE)
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
		}

		xyz_size = n*sizeof(float);
		st_size = l*sizeof(float);
		lm_size = m*sizeof(float);

		surf->has_vbo = true;
		surf->vbo_first_vert  = currVertexNum;
		surf->vbo_num_verts = 3*(p->numverts-2);
		
		currVertexNum += 3*(p->numverts-2);
		
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vbo_xyz_pos, xyz_size, &map);                             
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vbo_st_pos, st_size, &map2);                
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vbo_lm_pos, lm_size, &map3);  

		vbo_xyz_pos += xyz_size;
		vbo_st_pos += st_size;
		vbo_lm_pos += lm_size;
	}
}

void VB_BuildWorldVBO(void)
{
	msurface_t *surf;
	int num_vertexes = totalVBObufferSize/7;
	currVertexNum = 0;
	vbo_xyz_base = vbo_xyz_pos = 0;
	vbo_st_base = vbo_st_pos = num_vertexes*3*sizeof(float);
	vbo_lm_base = vbo_lm_pos = num_vertexes*5*sizeof(float);
	int i;

	qglGenBuffersARB(1, &vboId);
		
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);

	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
    {
		for (surf = &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface]; surf < &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface + r_worldmodel->nummodelsurfaces] ; surf++)
		{
			if ((currentmodel->unique_texinfo[i] != surf->texinfo->equiv) ||
				(surf->texinfo->flags & SURF_SKY) || 
				(surf->flags & SURF_DRAWTURB))
			{   // no skies here
				continue;
			}
			VB_BuildSurfaceVBO(surf);
		}
	}

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void VB_BuildVBOBufferSize(msurface_t *surf)
{
	glpoly_t *p = surf->polys;

	if (!( surf->flags & SURF_DRAWTURB ) )
	{
		totalVBObufferSize += 7*3*(p->numverts-2);
	}
}

void GL_SetupWorldVBO (void)
{
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);
	qglVertexPointer(3, GL_FLOAT, 0, (void *)vbo_xyz_base);
	
	qglClientActiveTextureARB (GL_TEXTURE0);
	qglTexCoordPointer(2, GL_FLOAT, 0, (void *)vbo_st_base);
	
	qglClientActiveTextureARB (GL_TEXTURE1);
	qglTexCoordPointer(2, GL_FLOAT, 0, (void *)vbo_lm_base);
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
	vertCache_t	*cache, *next;

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;

		if (cache->store == store && !strcmp(cache->mod->name, mod->name))
		{	// already cached!
			return cache;
		}
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

	for (i=0; i<MAX_VERTEX_CACHES; i++)
	{
		if(&vcm.vertCacheList[i])
			qglDeleteBuffersARB(1, &vcm.vertCacheList[i].id);
	}

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
	
	for (i=0; i<MAX_VERTEX_CACHES; i++)
	{
		if(&vcm.vertCacheList[i])
			qglDeleteBuffersARB(1, &vcm.vertCacheList[i].id);
	}

	for (cache = vcm.activeVertCache.next; cache != &vcm.activeVertCache; cache = next)
	{
		next = cache->next;
		R_VCFree(cache);
	}
}
