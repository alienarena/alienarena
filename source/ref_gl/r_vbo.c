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
			map3[n++] = v[0];
			map3[n++] = v[1];
			map3[n++] = v[2];

			// world texture coords
			map[l++] = v[3];
			map[l++] = v[4];

			// lightmap texture coords
			map2[m++] = v[5];
			map2[m++] = v[6];
		}

		surf->vbo_pos = vboPosition;

		surf->xyz_size = n*sizeof(float);
		surf->st_size = l*sizeof(float);
		surf->lm_size = m*sizeof(float);

		surf->has_vbo = true;
		
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vboPosition, surf->xyz_size, &vbo_shadow3);                             
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vboPosition + surf->xyz_size, surf->st_size, &vbo_shadow);                
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vboPosition + surf->xyz_size + surf->st_size, surf->lm_size, &vbo_shadow2);  

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

	if(gl_state.vbo)
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
void VB_VCInit()
{
	if (!gl_state.vbo)
		return;

	//clear out previous buffer
	qglDeleteBuffersARB(1, &vboId);

	vboPosition = 0;
	totalVBObufferSize = 0;	
}


void R_VCShutdown()
{

	if (!gl_state.vbo)
		return;

	//delete buffers
	qglDeleteBuffersARB(1, &vboId);
}
