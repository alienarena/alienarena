/*
Copyright (C) 2011-2014 COR Entertainment

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

static GLuint bsp_vboId = 0;
GLuint minimap_vboId = 0;

GLvoid			(APIENTRY * qglBindBufferARB)(GLenum target, GLuint buffer);
GLvoid			(APIENTRY * qglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
GLvoid			(APIENTRY * qglGenBuffersARB)(GLsizei n, GLuint *buffers);
GLvoid			(APIENTRY * qglBufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
GLvoid			(APIENTRY * qglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
void *			(APIENTRY * qglMapBufferARB)(GLenum target, GLenum access);
GLboolean		(APIENTRY * qglUnmapBufferARB)(GLenum target);

static void VB_VCInit (void);
void R_LoadVBOSubsystem(void)
{
	if (strstr(gl_config.extensions_string, "GL_ARB_vertex_buffer_object"))
	{
		qglBindBufferARB = (void *)qwglGetProcAddress("glBindBufferARB");
		qglDeleteBuffersARB = (void *)qwglGetProcAddress("glDeleteBuffersARB");
		qglGenBuffersARB = (void *)qwglGetProcAddress("glGenBuffersARB");
		qglBufferDataARB = (void *)qwglGetProcAddress("glBufferDataARB");
		qglBufferSubDataARB = (void *)qwglGetProcAddress("glBufferSubDataARB");
		qglMapBufferARB = (void *)qwglGetProcAddress("glMapBufferARB");
		qglUnmapBufferARB = (void *)qwglGetProcAddress("glUnmapBufferARB");

		if (qglGenBuffersARB && qglBindBufferARB && qglBufferDataARB && qglDeleteBuffersARB && qglMapBufferARB && qglUnmapBufferARB)
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

#define MAX_VBO_XYZs		65536
static int VB_AddWorldSurfaceToVBO (msurface_t *surf, int currVertexNum)
{
	glpoly_t *p;
	float	*v;
	int		i, j;
	int		n;
	int		trinum;
	float	map[MAX_VBO_XYZs];
	
	surf->vbo_num_verts = 0;
	n = 0;
	
	// XXX: for future reference, the glDrawRangeElements code was last seen
	// here at revision 3246.
	for (p = surf->polys; p != NULL; p = p->next)
	{
		for (trinum = 1; trinum < p->numverts-1; trinum++)
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
		
		surf->vbo_num_verts += 3*(p->numverts-2);
	}
	
	qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, currVertexNum * 7 * sizeof(float), n * sizeof(float), &map);
	
	surf->vbo_first_vert = currVertexNum;
	return currVertexNum + surf->vbo_num_verts;
}

static void VB_BuildWorldSurfaceVBO (void)
{
	msurface_t *surf, *surfs;
	int i, firstsurf, lastsurf;
	int	totalVBObufferSize = 0;
	int currVertexNum = 0;
	
	// just to keep the lines of code short
	surfs = r_worldmodel->surfaces;
	firstsurf = 0;
	lastsurf = r_worldmodel->numsurfaces;
	
	for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
	{
		glpoly_t *p;
		if (surf->texinfo->flags & (SURF_SKY|SURF_NODRAW))
			continue;
		for (p = surf->polys; p != NULL; p = p->next)
			totalVBObufferSize += 7*3*(p->numverts-2);
	}
	
	qglGenBuffersARB(1, &bsp_vboId);
		
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, bsp_vboId);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	for (i = 0; i < currentmodel->num_unique_texinfos; i++)
	{
		if (currentmodel->unique_texinfo[i]->flags & (SURF_SKY|SURF_NODRAW))
			continue;
		for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
		{
			if (	(currentmodel->unique_texinfo[i] != surf->texinfo->equiv) ||
					(surf->iflags & ISURF_PLANEBACK))
				continue;
			currVertexNum = VB_AddWorldSurfaceToVBO (surf, currVertexNum);
		}
		for	(surf = &surfs[firstsurf]; surf < &surfs[lastsurf]; surf++)
		{
			if (	(currentmodel->unique_texinfo[i] != surf->texinfo->equiv) ||
					!(surf->iflags & ISURF_PLANEBACK))
				continue;
			currVertexNum = VB_AddWorldSurfaceToVBO (surf, currVertexNum);
		}
	}
	
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

static void VB_BuildMinimapVBO (void)
{
	int i, j;
	int totalVBObufferSize = 0;
	int currVertexNum = 0;
	float *vbo_idx = 0;
	
	for (i = 1; i < r_worldmodel->numedges; i++)
	{
		if (r_worldmodel->edges[i].iscorner)
			totalVBObufferSize += (3+2)*2;
	}
	
	qglGenBuffersARB(1, &minimap_vboId);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, minimap_vboId);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, totalVBObufferSize*sizeof(float), 0, GL_STATIC_DRAW_ARB);
	
	for (i = 1; i < r_worldmodel->numedges; i++)
	{
		medge_t *edge = &r_worldmodel->edges[i];
		
		if (!edge->iscorner)
			continue;
		
		edge->vbo_start = currVertexNum;
		currVertexNum += 2;
		
		for (j = 0; j < 2; j++)
		{
			vec_t *v = r_worldmodel->vertexes[edge->v[j]].position;
			
			qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)vbo_idx, 3 * sizeof(float), v);
			vbo_idx += 3;
			qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)(vbo_idx++), sizeof(float), &edge->sColor);
			qglBufferSubDataARB (GL_ARRAY_BUFFER_ARB, (GLintptrARB)(vbo_idx++), sizeof(float), &edge->alpha);
		}
	}
	
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

void VB_BuildWorldVBO (void)
{
	VB_BuildWorldSurfaceVBO ();
	VB_BuildMinimapVBO ();
	fflush (stdout);
}

void GL_SetupWorldVBO (void)
{
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, bsp_vboId);

	R_VertexPointer (3, 7*sizeof(float), (void *)0);
	R_TexCoordPointer (0, 7*sizeof(float), (void *)(3*sizeof(float)));
	R_TexCoordPointer (1, 7*sizeof(float), (void *)(5*sizeof(float)));
	
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}



// "Special" VBOs: snippets of static geometry that stay unchanged between
// maps.


static GLuint skybox_vbo;

static void VB_BuildSkyboxVBO (void)
{
	// s, t, 1 (the 1 is for convenience)
	static float skyquad_texcoords[4][3] = 
	{
		{0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}
	};
	
	// 1 = s, 2 = t, 3 = SKYDIST
	static int	side_to_vec[6][3] =
	{
		{3,-1,-2}, {-3,1,-2},
		{1,3,-2}, {-1,-3,-2},
		{2,-1,3}, {-2,-1,-3}
	};
	
	int i, side, axis, vertnum, coordidx;
	float vertbuf[6 * 4][5], coordcoef;
	
	for (i = 0; i < 4 * 6; i++)
	{
		side = i / 4; 
		vertnum = i % 4;
		for (axis = 0; axis < 3; axis++)
		{
			coordidx = abs (side_to_vec[side][axis]) - 1;
			coordcoef = side_to_vec[side][axis] < 0 ? -SKYDIST : SKYDIST;
			vertbuf[i][axis] = coordcoef * (2.0f * skyquad_texcoords[vertnum][coordidx] - 1.0);
		}
		for (; axis < 5; axis++)
			vertbuf[i][axis] = skyquad_texcoords[vertnum][axis - 3];
	}
	
	qglGenBuffersARB (1, &skybox_vbo);
	GL_BindVBO (skybox_vbo);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, sizeof(vertbuf), vertbuf, GL_STATIC_DRAW_ARB);
	GL_BindVBO (0);
}

void GL_SetupSkyboxVBO (void)
{
	GL_BindVBO (skybox_vbo);
	R_VertexPointer (3, 5*sizeof(float), (void *)0);
	R_TexCoordPointer (0, 5*sizeof(float), (void *)(3*sizeof(float)));
	GL_BindVBO (0);
}


// For fullscreen postprocessing passes, etc. To be rendered in a glOrtho
// coordinate space from 0 to 1 on each axis.
static GLuint wholescreen2D_vbo;

static void VB_BuildWholeScreen2DVBO (void)
{
	static float vertbuf[4][2][2] = 
	{
	//	vertex coords/texcoords		vertically flipped texcoords
		{{0, 0},					{0, 1}},
		{{1, 0},					{1, 1}},
		{{1, 1},					{1, 0}},
		{{0, 1},					{0, 0}}
	};
	
	qglGenBuffersARB (1, &wholescreen2D_vbo);
	GL_BindVBO (wholescreen2D_vbo);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, sizeof(vertbuf), vertbuf, GL_STATIC_DRAW_ARB);
	GL_BindVBO (0);
}

void GL_SetupWholeScreen2DVBO (wholescreen_drawtype_t drawtype)
{
	GL_BindVBO (wholescreen2D_vbo);
	R_VertexPointer (2, 4*sizeof(float), (void *)0);
	switch (drawtype)
	{
	case wholescreen_blank:
		break;
	case wholescreen_textured:
		R_TexCoordPointer (0, 4*sizeof(float), (void *)0);
		break;
	case wholescreen_fliptextured:
		R_TexCoordPointer (0, 4*sizeof(float), (void *)(2*sizeof(float)));
		break;
	}
	GL_BindVBO (0);
}


void VB_WorldVCInit (void)
{
	//clear out previous buffer
	qglDeleteBuffersARB (1, &bsp_vboId);
	qglDeleteBuffersARB (1, &minimap_vboId);
}

static void VB_VCInit (void)
{
	VB_WorldVCInit ();
	
	// "Special" VBOs
	VB_BuildSkyboxVBO ();
	VB_BuildWholeScreen2DVBO ();
}

void R_VCShutdown (void)
{
	// delete buffers
	VB_WorldVCInit ();
	qglDeleteBuffersARB (1, &skybox_vbo);
	qglDeleteBuffersARB (1, &wholescreen2D_vbo);
}
