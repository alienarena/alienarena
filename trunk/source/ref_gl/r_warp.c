/*
Copyright (C) 1997-2001 Id Software, Inc.
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
// r_warp.c -- sky and water polygons

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

extern	model_t	*loadmodel;

char	skyname[MAX_QPATH];
float	skyrotate;
vec3_t	skyaxis;
image_t	*sky_images[6];

msurface_t	*warpface;

#define	SUBDIVIDE_SIZE	64

void BoundPoly (int numverts, float *verts, vec3_t mins, vec3_t maxs)
{
	int		i, j;
	float	*v;

	mins[0] = mins[1] = mins[2] = 9999;
	maxs[0] = maxs[1] = maxs[2] = -9999;
	v = verts;
	for (i=0 ; i<numverts ; i++)
		for (j=0 ; j<3 ; j++, v++)
		{
			if (*v < mins[j])
				mins[j] = *v;
			if (*v > maxs[j])
				maxs[j] = *v;
		}
}

void SubdividePolygon (int numverts, float *verts)
{
	int		i, j, k;
	vec3_t	mins, maxs;
	float	m;
	float	*v;
	vec3_t	front[64], back[64];
	int		f, b;
	float	dist[64];
	float	frac;
	glpoly_t	*poly;
	float	s, t;
	vec3_t	total;
	float	total_s, total_t;

	if (numverts > 60)
		Com_Error (ERR_DROP, "numverts = %i", numverts);

	BoundPoly (numverts, verts, mins, maxs);

	for (i=0 ; i<3 ; i++)
	{
		m = (mins[i] + maxs[i]) * 0.5;
		m = SUBDIVIDE_SIZE * floor (m/SUBDIVIDE_SIZE + 0.5);
		if (maxs[i] - m < 8)
			continue;
		if (m - mins[i] < 8)
			continue;

		// cut it
		v = verts + i;
		for (j=0 ; j<numverts ; j++, v+= 3)
			dist[j] = *v - m;

		// wrap cases
		dist[j] = dist[0];
		v-=i;
		VectorCopy (verts, v);

		f = b = 0;
		v = verts;
		for (j=0 ; j<numverts ; j++, v+= 3)
		{
			if (dist[j] >= 0)
			{
				VectorCopy (v, front[f]);
				f++;
			}
			if (dist[j] <= 0)
			{
				VectorCopy (v, back[b]);
				b++;
			}
			if (dist[j] == 0 || dist[j+1] == 0)
				continue;
			if ( (dist[j] > 0) != (dist[j+1] > 0) )
			{
				// clip point
				frac = dist[j] / (dist[j] - dist[j+1]);
				for (k=0 ; k<3 ; k++)
					front[f][k] = back[b][k] = v[k] + frac*(v[3+k] - v[k]);
				f++;
				b++;
			}
		}

		SubdividePolygon (f, front[0]);
		SubdividePolygon (b, back[0]);
		return;
	}

	// add a point in the center to help keep warp valid
	poly = Hunk_Alloc (sizeof(glpoly_t) + ((numverts-4)+2) * VERTEXSIZE*sizeof(float));
	poly->next = warpface->polys;
	warpface->polys = poly;
	poly->numverts = numverts+2;
	VectorClear (total);
	total_s = 0;
	total_t = 0;
	for (i=0 ; i<numverts ; i++, verts+= 3)
	{
		VectorCopy (verts, poly->verts[i+1]);
		s = DotProduct (verts, warpface->texinfo->vecs[0]);
		t = DotProduct (verts, warpface->texinfo->vecs[1]);

		total_s += s;
		total_t += t;
		VectorAdd (total, verts, total);

		poly->verts[i+1][3] = s;
		poly->verts[i+1][4] = t;
	}

	VectorScale (total, (1.0/numverts), poly->verts[0]);
	poly->verts[0][3] = total_s/numverts;
	poly->verts[0][4] = total_t/numverts;

	// copy first vertex to last
	memcpy (poly->verts[i+1], poly->verts[1], sizeof(poly->verts[0]));
}

/*
================
R_SubdivideSurface

Breaks a polygon up along axial 64 unit
boundaries so that turbulent and sky warps
can be done reasonably.
================
*/
void R_SubdivideSurface (msurface_t *fa)
{
	vec3_t		verts[64];
	int			numverts;
	int			i;
	int			lindex;
	float		*vec;

	warpface = fa;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = loadmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = loadmodel->vertexes[loadmodel->edges[lindex].v[0]].position;
		else
			vec = loadmodel->vertexes[loadmodel->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;
	}

	SubdividePolygon (numverts, verts[0]);
}

//=========================================================



// speed up sin calculations - Ed
float	r_turbsin[] =
{
	#include "warpsin.h"
};

#define TURBOSCALE (256.0f / ((float)M_PI / 4.0f))

/*
=============
EmitWaterPolys

Does a water warp on the pre-fragmented glpoly_t chain
=============
*/
void R_RenderWaterPolys (msurface_t *fa, int texnum, float scaleX, float scaleY)
{
	glpoly_t	*p;
	float		*v;
	int			i;
	float		s, t, os, ot;
	float		scroll;
	float		rdt = r_newrefdef.time;
	vec3_t		nv, tangent;

	if (fa->texinfo->flags & SURF_FLOWING)
		scroll = -64.0f * ((r_newrefdef.time * 0.5f) - (int)(r_newrefdef.time * 0.5f));
	else
		scroll = 0.0f;
	  
	if(gl_state.glsl_shaders && gl_glsl_shaders->value
		&& strcmp(fa->texinfo->normalMap->name, fa->texinfo->image->name)) {

		if (SurfaceIsAlphaBlended(fa))
			qglEnable( GL_ALPHA_TEST );

		R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);

		glUseProgramObjectARB( g_waterprogramObj );

		GL_EnableMultitexture( true );

		//qglActiveTextureARB(GL_TEXTURE0);
		GL_MBind(GL_TEXTURE0, fa->texinfo->image->texnum);
		glUniform1iARB( g_location_baseTexture, 0);

		//note - moving this to tmu2 fixed a very odd, obsure bug.  It isn't clear yet why it fixes it, but it does
		GL_MBind(GL_TEXTURE1, fa->texinfo->normalMap->texnum);
		glUniform1iARB( g_location_normTexture, 1);

		if(fa->texinfo->flags &(SURF_TRANS33|SURF_TRANS66))
			glUniform1iARB( g_location_trans, 1);
		else
			glUniform1iARB( g_location_trans, 0);

		if(texnum)
		{
			qglActiveTextureARB(GL_TEXTURE2);
			qglBindTexture(GL_TEXTURE_2D, texnum);
			glUniform1iARB( g_location_refTexture, 2);
			glUniform1iARB( g_location_reflect, 1);
		}
		else
			glUniform1iARB( g_location_reflect, 0);
			
		AngleVectors(fa->plane->normal, NULL, tangent, NULL);

		//send these to the shader program	
		glUniform3fARB( g_location_tangent, tangent[0], tangent[1], tangent[2]);
		glUniform3fARB( g_location_lightPos, r_worldLightVec[0], r_worldLightVec[1], r_worldLightVec[2]);
		glUniform1iARB( g_location_fogamount, map_fog);
		glUniform1fARB( g_location_time, rs_realtime);

		R_AddGLSLShadedWarpSurfToVArray (fa, scroll);

		glUseProgramObjectARB( 0 );

		R_KillVArrays ();

		GL_EnableMultitexture( false );

		if (SurfaceIsAlphaBlended(fa))
			qglDisable( GL_ALPHA_TEST);

		return;
	}
	else {

		if (gl_state.fragment_program && strcmp(fa->texinfo->normalMap->name, fa->texinfo->image->name))
		{
			qglEnable(GL_FRAGMENT_PROGRAM_ARB);
			qglBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, g_water_program_id);
			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0,
				rs_realtime * (0.2f), 1.0f, 1.0f, 1.0f);
			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1,
				rs_realtime * -0.2f, 10.0f, 1.0f, 1.0f);
			qglProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2,
				(fa->polys[0].verts[0][3]-r_newrefdef.vieworg[0]), (fa->polys[0].verts[0][4]-r_newrefdef.vieworg[1]), (fa->polys[0].verts[0][4]-r_newrefdef.vieworg[2]), 1.0f);

			GL_MBind(GL_TEXTURE1, r_distort->texnum);
		}

		GL_MBind(GL_TEXTURE0, fa->texinfo->image->texnum);

		for (p=fa->polys ; p ; p=p->next)
		{
			qglBegin (GL_TRIANGLE_FAN);
			for (i=0,v=p->verts[0] ; i<p->numverts ; i++, v+=VERTEXSIZE)
			{
				os = v[3];
				ot = v[4];

				s = os + r_turbsin[(int)((ot*0.125+r_newrefdef.time) * TURBSCALE) & 255];

				s += scroll;
				s *= (1.0/64);

				t = ot + r_turbsin[(int)((os*0.125+rdt) * TURBSCALE) & 255];

				t *= (1.0/64);

				if (gl_state.fragment_program)
				{
					qglMTexCoord2fARB(GL_TEXTURE0, s, t);
					qglMTexCoord2fARB(GL_TEXTURE1, 20*s, 20*t);
				}
				else
					qglTexCoord2f (s, t);

				if (!(fa->texinfo->flags & SURF_FLOWING))

				{
					nv[0] =v[0];
					nv[1] =v[1];

					nv[2] =v[2] + r_wave->value *sin(v[0]*0.025+r_newrefdef.time)*sin(v[2]*0.05+r_newrefdef.time)

							+ r_wave->value *sin(v[1]*0.025+r_newrefdef.time*2)*sin(v[2]*0.05+r_newrefdef.time);

					qglVertex3fv (nv);
				}
				else
					qglVertex3fv (v);
			}
			qglEnd ();
		}

		if (gl_state.fragment_program)
			qglDisable(GL_FRAGMENT_PROGRAM_ARB);
	}

	//env map if specified by shader
	if(texnum)
		GL_Bind(texnum);
	else
		return;

	for (p=fa->polys ; p ; p=p->next)
	{
		qglBegin (GL_TRIANGLE_FAN);
		for (i=0,v=p->verts[0] ; i<p->numverts ; i++, v+=VERTEXSIZE)
		{
			os = v[3] - r_newrefdef.vieworg[0] + 128*scaleX;
			ot = v[4] + r_newrefdef.vieworg[1] + 128*scaleY;

			if(texnum)
				qglTexCoord2f(1.0/512*scaleX*os, 1.0/512*scaleY*ot);

			if (!(fa->texinfo->flags & SURF_FLOWING))
			{
				nv[0] =v[0];
				nv[1] =v[1];

				nv[2] =v[2] + r_wave->value *sin(v[0]*0.025+r_newrefdef.time)*sin(v[2]*0.05+r_newrefdef.time)
						+ r_wave->value *sin(v[1]*0.025+r_newrefdef.time*2)*sin(v[2]*0.05+r_newrefdef.time);

				qglVertex3fv (nv);
			}
			else
				qglVertex3fv (v);
		}
		qglEnd ();
	}
}

vec3_t	skyclip[6] = {
	{1,1,0},
	{1,-1,0},
	{0,-1,1},
	{0,1,1},
	{1,0,1},
	{-1,0,1}
};

// 1 = s, 2 = t, 3 = 2048
int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},		// 0 degrees yaw, look straight up
	{2,-1,-3}		// look straight down

//	{-1,2,3},
//	{1,2,-3}
};

// s = [0]/[2], t = [1]/[2]
int	vec_to_st[6][3] =
{
	{-2,3,1},
	{2,3,-1},

	{1,3,2},
	{-1,3,-2},

	{-2,-1,3},
	{-2,1,-3}

//	{-1,2,3},
//	{1,2,-3}
};

float	skymins[2][6], skymaxs[2][6];
float	sky_min, sky_max;

void DrawSkyPolygon (int nump, vec3_t vecs)
{
	int		i,j;
	vec3_t	v, av;
	float	s, t, dv;
	int		axis;
	float	*vp;

	// decide which face it maps to
	VectorCopy (vec3_origin, v);
	for (i=0, vp=vecs ; i<nump ; i++, vp+=3)
	{
		VectorAdd (vp, v, v);
	}
	av[0] = fabs(v[0]);
	av[1] = fabs(v[1]);
	av[2] = fabs(v[2]);
	if (av[0] > av[1] && av[0] > av[2])
	{
		if (v[0] < 0)
			axis = 1;
		else
			axis = 0;
	}
	else if (av[1] > av[2] && av[1] > av[0])
	{
		if (v[1] < 0)
			axis = 3;
		else
			axis = 2;
	}
	else
	{
		if (v[2] < 0)
			axis = 5;
		else
			axis = 4;
	}

	// project new texture coords
	for (i=0 ; i<nump ; i++, vecs+=3)
	{
		j = vec_to_st[axis][2];
		if (j > 0)
			dv = vecs[j - 1];
		else
			dv = -vecs[-j - 1];
		if (dv < 0.001)
			continue;	// don't divide by zero
		j = vec_to_st[axis][0];
		if (j < 0)
			s = -vecs[-j -1] / dv;
		else
			s = vecs[j-1] / dv;
		j = vec_to_st[axis][1];
		if (j < 0)
			t = -vecs[-j -1] / dv;
		else
			t = vecs[j-1] / dv;

		if (s < skymins[0][axis])
			skymins[0][axis] = s;
		if (t < skymins[1][axis])
			skymins[1][axis] = t;
		if (s > skymaxs[0][axis])
			skymaxs[0][axis] = s;
		if (t > skymaxs[1][axis])
			skymaxs[1][axis] = t;
	}
}

#define	ON_EPSILON		0.1			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64
void ClipSkyPolygon (int nump, vec3_t vecs, int stage)
{
	float	*norm;
	float	*v;
	qboolean	front, back;
	float	d, e;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];
	int		newc[2];
	int		i, j;

	if (nump > MAX_CLIP_VERTS-2)
		Com_Error (ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS");
	if (stage == 6)
	{	// fully clipped, so draw it
		DrawSkyPolygon (nump, vecs);
		return;
	}

	front = back = false;
	norm = skyclip[stage];
	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		d = DotProduct (v, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (nump, vecs, stage+1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*3)) );
	newc[0] = newc[1] = 0;

	for (i=0, v = vecs ; i<nump ; i++, v+=3)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			break;
		case SIDE_BACK:
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (v, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (v, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{
			e = v[j] + d*(v[j+3] - v[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// continue
	ClipSkyPolygon (newc[0], newv[0][0], stage+1);
	ClipSkyPolygon (newc[1], newv[1][0], stage+1);
}

/*
=================
R_AddSkySurface
=================
*/
void R_AddSkySurface (msurface_t *fa)
{
	int			i;
	vec3_t		verts[MAX_CLIP_VERTS];
	glpoly_t	*p;

	// calculate vertex values for sky box
	for (p=fa->polys ; p ; p=p->next)
	{
		for (i=0 ; i<p->numverts ; i++)
		{
			VectorSubtract (p->verts[i], r_origin, verts[i]);
		}
		ClipSkyPolygon (p->numverts, verts[0], 0);
	}
}


/*
==============
R_ClearSkyBox
==============
*/
void R_ClearSkyBox (void)
{
	int		i;

	for (i=0 ; i<6 ; i++)
	{
		skymins[0][i] = skymins[1][i] = 9999;
		skymaxs[0][i] = skymaxs[1][i] = -9999;
	}
}


void MakeSkyVec (float s, float t, int axis, float *tex_s, float *tex_t, float *vec, float size)
{
	vec3_t		v, b;
	int			j, k;

	b[0] = s * size;
	b[1] = t * size;
	b[2] = size;

	for (j=0 ; j<3 ; j++)
	{
		k = st_to_vec[axis][j];
		if (k < 0)
			v[j] = -b[-k - 1];
		else
			v[j] = b[k - 1];
	}

	// avoid bilerp seam
	s = (s+1)*0.5;
	t = (t+1)*0.5;

	if (s < sky_min)
		s = sky_min;
	else if (s > sky_max)
		s = sky_max;
	if (t < sky_min)
		t = sky_min;
	else if (t > sky_max)
		t = sky_max;

	t = 1.0 - t;

	*tex_s = s;
	*tex_t = t;
	VectorCopy(v, vec);
}

/*
==============
R_DrawSkyBox
==============
*/
int	skytexorder[6] = {0,2,1,3,4,5};
void R_DrawSkyBox (void)
{
	int		i;
	float	s ,t;
	vec3_t	point;
	float	alpha;
	rscript_t *rs = NULL;
	rs_stage_t *stage = NULL;

	if (skyrotate)
	{	// check for no sky at all
		for (i=0 ; i<6 ; i++)
			if (skymins[0][i] < skymaxs[0][i]
			&& skymins[1][i] < skymaxs[1][i])
				break;
		if (i == 6)
			return;		// nothing visible
	}

	qglPushMatrix ();
	qglTranslatef (r_origin[0], r_origin[1], r_origin[2]);
	qglRotatef (r_newrefdef.time * skyrotate, skyaxis[0], skyaxis[1], skyaxis[2]);

	for (i=0 ; i<6 ; i++)
	{
		if (skyrotate)
		{	// hack, forces full sky to draw when rotating
			skymins[0][i] = -1;
			skymins[1][i] = -1;
			skymaxs[0][i] = 1;
			skymaxs[1][i] = 1;
		}

		if (skymins[0][i] >= skymaxs[0][i]
		|| skymins[1][i] >= skymaxs[1][i])
			continue;

		GL_Bind (sky_images[skytexorder[i]]->texnum);

		qglBegin (GL_QUADS);

		MakeSkyVec (skymins[0][i], skymins[1][i], i, &s, &t, point, 3300);
		qglTexCoord2f (s, t);
		qglVertex3fv (point);

		MakeSkyVec (skymins[0][i], skymaxs[1][i], i, &s, &t, point, 3300);
		qglTexCoord2f (s, t);
		qglVertex3fv (point);

		MakeSkyVec (skymaxs[0][i], skymaxs[1][i], i, &s, &t, point, 3300);
		qglTexCoord2f (s, t);
		qglVertex3fv (point);

		MakeSkyVec (skymaxs[0][i], skymins[1][i], i, &s, &t, point, 3300);
		qglTexCoord2f (s, t);
		qglVertex3fv (point);

		qglEnd ();

	}
	qglPopMatrix ();

    if(r_shaders->value) { //just cloud layers for now, we can expand this

		qglPushMatrix (); //rotate the clouds
		qglTranslatef (r_origin[0], r_origin[1], r_origin[2]);
		qglRotatef (rs_realtime * 20, 0, 1, 0);

		for (i=0 ; i<6 ; i++)
		{
			rs=(rscript_t *)sky_images[skytexorder[i]]->script;

			if(rs) {

				stage=rs->stage;
				while (stage)
				{
					qglDepthMask( GL_FALSE );	 	// no z buffering
					qglEnable( GL_BLEND);
					GL_TexEnv( GL_MODULATE );
					GLSTATE_DISABLE_ALPHATEST

					GL_Bind (stage->texture->texnum);

					if (stage->blendfunc.blend)
					{
						GL_BlendFunction(stage->blendfunc.source,stage->blendfunc.dest);
						GLSTATE_ENABLE_BLEND
					}
					else
					{
						GLSTATE_DISABLE_BLEND
					}

					if (stage->alphashift.min || stage->alphashift.speed)
					{
						alpha=0.0f;

						if (!stage->alphashift.speed && stage->alphashift.min > 0)
						{
							alpha=stage->alphashift.min;
						}
						else if (stage->alphashift.speed)
						{
							alpha=sin(rs_realtime * stage->alphashift.speed);
							alpha=(alpha+1)*0.5f;
							if (alpha > stage->alphashift.max) alpha=stage->alphashift.max;
							if (alpha < stage->alphashift.min) alpha=stage->alphashift.min;
						}
					}
					else
						alpha=1.0f;

					qglColor4f(1,1,1,alpha);

					if (stage->alphamask)
					{
						GLSTATE_ENABLE_ALPHATEST
					}
					else
					{
						GLSTATE_DISABLE_ALPHATEST
					}

					skymins[0][i] = -1;
					skymins[1][i] = -1;
					skymaxs[0][i] = 1;
					skymaxs[1][i] = 1;

					qglBegin (GL_QUADS);

					MakeSkyVec (skymins[0][i], skymins[1][i], i, &s, &t, point, 2300);
					RS_SetTexcoords2D (stage, &s, &t);
					qglTexCoord2f (s, t);
					qglVertex3fv (point);

					MakeSkyVec (skymins[0][i], skymaxs[1][i], i, &s, &t, point, 2300);
					RS_SetTexcoords2D (stage, &s, &t);
					qglTexCoord2f (s, t);
					qglVertex3fv (point);

					MakeSkyVec (skymaxs[0][i], skymaxs[1][i], i, &s, &t, point, 2300);
					RS_SetTexcoords2D (stage, &s, &t);
					qglTexCoord2f (s, t);
					qglVertex3fv (point);

					MakeSkyVec (skymaxs[0][i], skymins[1][i], i, &s, &t, point, 2300);
					RS_SetTexcoords2D (stage, &s, &t);
					qglTexCoord2f (s, t);
					qglVertex3fv (point);

					qglEnd();

					stage=stage->next;
				}
			}
		}
		// restore the original blend mode
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_DISABLE_BLEND
		qglBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		qglColor4f( 1,1,1,1 );
		qglDepthMask( GL_TRUE );	// back to normal Z buffering
		GL_TexEnv( GL_REPLACE );
		qglPopMatrix ();
	}
}


/*
============
R_SetSky
============
*/
// 3dstudio environment map names
char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
void R_SetSky (char *name, float rotate, vec3_t axis)
{
	int		i;
	char	pathname[MAX_QPATH];

	strncpy (skyname, name, sizeof(skyname)-1);
	skyrotate = rotate;
	VectorCopy (axis, skyaxis);

	for (i=0 ; i<6 ; i++)
	{
		// chop down rotating skies for less memory
		if (gl_skymip->value || skyrotate)
			gl_picmip->value++;

		Com_sprintf (pathname, sizeof(pathname), "env/%s%s.tga", skyname, suf[i]);

		sky_images[i] = GL_FindImage (pathname, it_sky);
		if (!sky_images[i])
			sky_images[i] = r_notexture;
		else { //valid sky, load shader
			if (r_shaders->value) {
				strcpy(pathname,sky_images[i]->name);
				pathname[strlen(pathname)-4]=0;
				sky_images[i]->script = RS_FindScript(pathname);
				if(sky_images[i]->script)
					RS_ReadyScript(sky_images[i]->script);
			}
		}

		if (strstr(pathname, "space")) {
			VectorSet(sun_origin, -5000, -100000, 115000);
			spacebox = true;
		}
		else if (strstr(pathname, "sea")) {
			VectorSet(sun_origin, 140000, -80000, 125000);
			spacebox = false;
		}
		else if (strstr(pathname, "hell")) {
			VectorSet(sun_origin, 140000, 160000, 85000);
			spacebox = false;
		}
		else {
			VectorSet(sun_origin, 140000, -80000, 45000);
			spacebox = false;
		}

		if (gl_skymip->value || skyrotate)
		{	// take less memory
			gl_picmip->value--;
			sky_min = 1.0/256;
			sky_max = 255.0/256;
		}
		else
		{
			sky_min = 1.0/512;
			sky_max = 511.0/512;
		}
	}
}
