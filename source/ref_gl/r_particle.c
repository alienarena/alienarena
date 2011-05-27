/*
Copyright (C) 2010 COR Entertainment, LLC

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
// r_particle.c - particle subsystem, renders particles, lightvolumes, lensflares, vegetation, etc

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

particle_t gparticles[MAX_PARTICLES];
int			num_gparticles;

/*
===============
PART_DrawParticles
===============
*/
static float yawOrRoll;
void PART_DrawParticles( int num_particles, particle_t **particles, const unsigned colortable[768])
{
	particle_t **p1;
	particle_t *p;
	int				i, k;
	vec3_t			corner[4], up, right, pup, pright, dir;
	float			scale, oldscale=0.0f;
	byte			color[4], oldcolor[4]= {0,0,0,0};
	int			    oldtype;
	int				texnum=0, blenddst, blendsrc;
	float			*corner0 = corner[0];
	vec3_t move, delta, v;

	if ( !num_particles )
		return;

	qglDepthMask( GL_FALSE );	 	// no z buffering
	qglEnable( GL_BLEND);
	GL_TexEnv( GL_MODULATE );

	R_InitVArrays (VERT_SINGLE_TEXTURED);

	for ( p1 = particles, i=0, oldtype=-1 ; i < num_particles ; i++,p1++)
	{
	    p = *p1;

		if( R_CullSphere( p->current_origin, 64, 15 ) )
			continue;

		if(p->type == PARTICLE_NONE) {

			blendsrc = GL_SRC_ALPHA;
			blenddst = GL_ONE;

			*(int *)color = colortable[p->current_color];
			scale = 1;
		}
		else {
			texnum = p->texnum;
			blendsrc = p->blendsrc;
			blenddst = p->blenddst;
			scale = p->current_scale;
			*(int *)color = colortable[p->current_color];
		}

		color[3] = p->current_alpha*255;

		if (
			p->type != oldtype
			|| color[0] != oldcolor[0] || color[1] != oldcolor[1]
			|| color[2] != oldcolor[2] || color[3] != oldcolor[3] || scale != oldscale)
		{
			if ( scale != 1 )
			{
				VectorScale (vup, scale, up);
				VectorScale (vright, scale, right);
			}
			else
			{
				VectorCopy (vup, up);
				VectorCopy (vright, right);
			}

			oldtype = p->type;
			oldscale = scale;
			oldcolor[3] = color[3];
			VectorCopy ( color, oldcolor );

			GL_Bind ( texnum );
			qglBlendFunc ( blendsrc, blenddst );
			if(p->type == PARTICLE_RAISEDDECAL) {
				qglColor4f(1,1,1,1);
			}
			else
				qglColor4ubv( color );
		}

		if(p->type == PARTICLE_BEAM) {

			VectorSubtract(p->current_origin, p->angle, move);
			VectorNormalize(move);

			VectorCopy(move, pup);
			VectorSubtract(r_newrefdef.vieworg, p->angle, delta);
			CrossProduct(pup, delta, pright);
			VectorNormalize(pright);

			VectorScale(pright, 5*scale, pright);
			VectorScale(pup, 5*scale, pup);
		}
		else if(p->type == PARTICLE_DECAL || p->type == PARTICLE_RAISEDDECAL) {
			VectorCopy(p->angle, dir);
			AngleVectors(dir, NULL, right, up);
			VectorScale ( right, p->dist, pright );
			VectorScale ( up, p->dist, pup );
			VectorScale(pright, 5*scale, pright);
			VectorScale(pup, 5*scale, pup);

		}
		else if(p->type == PARTICLE_FLAT) {

			VectorCopy(r_newrefdef.viewangles, dir);

			dir[0] = -90;  // and splash particles horizontal by setting it
			AngleVectors(dir, NULL, right, up);

			if(p->current_origin[2] > r_newrefdef.vieworg[2]){  // it's above us
				VectorScale(right, 5*scale, pright);
				VectorScale(up, 5*scale, pup);
			}
			else {  // it's below us
				VectorScale(right, 5*scale, pright);
				VectorScale(up, -5*scale, pup);
			}
		}
		else if(p->type == PARTICLE_WEATHER || p->type == PARTICLE_VERT){  // keep it vertical
			VectorCopy(r_newrefdef.viewangles, v);
			v[0] = 0;  // keep weather particles vertical by removing pitch
			AngleVectors(v, NULL, right, up);
			VectorScale(right, 3*scale, pright);
			VectorScale(up, 3*scale, pup);
		}
		else if(p->type == PARTICLE_ROTATINGYAW || p->type == PARTICLE_ROTATINGROLL || p->type == PARTICLE_ROTATINGYAWMINUS){  // keep it vertical, and rotate on axis
			VectorCopy(r_newrefdef.viewangles, v);
			v[0] = 0;  // keep weather particles vertical by removing pitch
			yawOrRoll += r_frametime*50;
			if (yawOrRoll > 360)
				yawOrRoll = 0;
			if(p->type == PARTICLE_ROTATINGYAW)
				v[1] = yawOrRoll;
			else if(p->type == PARTICLE_ROTATINGROLL)
				v[2] = yawOrRoll;
			else
				v[1] = yawOrRoll+180;
			AngleVectors(v, NULL, right, up);
			VectorScale(right, 3*scale, pright);
			VectorScale(up, 3*scale, pup);
		}
		else {
			VectorScale ( right, p->dist, pright );
			VectorScale ( up, p->dist, pup );
		}

		VectorSet (corner[0],
			p->current_origin[0] + (pup[0] + pright[0])*(-0.5),
			p->current_origin[1] + (pup[1] + pright[1])*(-0.5),
			p->current_origin[2] + (pup[2] + pright[2])*(-0.5));

		VectorSet ( corner[1],
			corner0[0] + pup[0], corner0[1] + pup[1], corner0[2] + pup[2]);
		VectorSet ( corner[2], corner0[0] + (pup[0]+pright[0]),
			corner0[1] + (pup[1]+pright[1]), corner0[2] + (pup[2]+pright[2]));
		VectorSet ( corner[3],
			corner0[0] + pright[0], corner0[1] + pright[1], corner0[2] + pright[2]);

		VArray = &VArrayVerts[0];

		for(k = 0; k < 4; k++) {

			 VArray[0] = corner[k][0];
             VArray[1] = corner[k][1];
             VArray[2] = corner[k][2];

			 switch(k) {
				case 0:
					VArray[3] = 1;
					VArray[4] = 1;
					break;
				case 1:
					VArray[3] = 0;
					VArray[4] = 1;
					break;
				case 2:
					VArray[3] = 0;
					VArray[4] = 0;
					break;
				case 3:
					VArray[3] = 1;
					VArray[4] = 0;
					break;
			 }

			 VArray += VertexSizes[VERT_SINGLE_TEXTURED];
		}

		if(qglLockArraysEXT)
			qglLockArraysEXT(0, 4);

		qglDrawArrays(GL_QUADS,0,4);

		if(qglUnlockArraysEXT)
			qglUnlockArraysEXT();

	}

	R_KillVArrays ();
	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	qglColor4f( 1,1,1,1 );
	qglDisable(GL_BLEND);
	qglDepthMask( GL_TRUE );	// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
    if ( !r_newrefdef.num_particles )
        return;

    if(map_fog)
        qglDisable(GL_FOG);

    PART_DrawParticles( r_newrefdef.num_particles, r_newrefdef.particles, d_8to24table);

    if(map_fog)
        qglEnable(GL_FOG);
}

//lens flares

void Mod_AddFlareSurface (msurface_t *surf, int type )
{
     int i, width, height, intens;
     glpoly_t *poly;
     flare_t  *light;
     byte     *buffer;
	 byte     *p;
     float    *v, surf_bound;
	 vec3_t origin = {0,0,0}, color = {1,1,1}, tmp, rgbSum;
     vec3_t poly_center, mins, maxs, tmp1;

     if (surf->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING|SURF_DRAWTURB|SURF_WARP))
          return;

     if (!(surf->texinfo->flags & (SURF_LIGHT)))
          return;

	 if (r_numflares >= MAX_FLARES)
			return;

    light = &r_flares[r_numflares++];
    intens = surf->texinfo->value;


     //if (intens < 100) //if not a lighted surf, don't do a flare, right?
	//	 return;
    /*
	===================
	find poligon centre
	===================
	*/
	VectorSet(mins, 999999, 999999, 999999);
	VectorSet(maxs, -999999, -999999, -999999);

    for ( poly = surf->polys; poly; poly = poly->chain ) {
         for (i=0, v=poly->verts[0] ; i< poly->numverts; i++, v+= VERTEXSIZE) {

               if(v[0] > maxs[0])   maxs[0] = v[0];
               if(v[1] > maxs[1])   maxs[1] = v[1];
               if(v[2] > maxs[2])   maxs[2] = v[2];

               if(v[0] < mins[0])   mins[0] = v[0];
               if(v[1] < mins[1])   mins[1] = v[1];
               if(v[2] < mins[2])   mins[2] = v[2];
            }
      }

     poly_center[0] = (mins[0] + maxs[0]) /2;
     poly_center[1] = (mins[1] + maxs[1]) /2;
     poly_center[2] = (mins[2] + maxs[2]) /2;
	 VectorCopy(poly_center, origin);

     /*
	=====================================
	calc light surf bounds and flare size
	=====================================
	*/


	 VectorSubtract(maxs, mins, tmp1);
     surf_bound = VectorLength(tmp1);
	 if (surf_bound <=25) light->size = 10;
	 else if (surf_bound <=50)  light->size = 15;
     else if (surf_bound <=100) light->size = 20;
	 else if (surf_bound <=150) light->size = 25;
	 else if (surf_bound <=200) light->size = 30;
	 else if (surf_bound <=250) light->size = 35;


	/*
	===================
	calc texture color
	===================
	*/

     GL_Bind( surf->texinfo->image->texnum );
     width = surf->texinfo->image->upload_width;
	 height = surf->texinfo->image->upload_height;

     buffer = malloc(width * height * 3);
     qglGetTexImage (GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer);
     VectorClear(rgbSum);

     for (i=0, p=buffer; i<width*height; i++, p += 3)
      {
        rgbSum[0] += (float)p[0]  * (1.0 / 255);
        rgbSum[1] += (float)p[1]  * (1.0 / 255);
        rgbSum[2] += (float)p[2]  * (1.0 / 255);
      }

      VectorScale(rgbSum, r_lensflare_intens->value / (width *height), color);

      for (i=0; i<3; i++) {
          if (color[i] < 0.5)
               color[i] = color[i] * 0.5;
          else
               color[i] = color[i] * 0.5 + 0.5;
          }
      VectorCopy(color, light->color);

	/*
	==================================
	move flare origin in to map bounds
	==================================
	*/

     if (surf->flags & SURF_PLANEBACK)
		VectorNegate(surf->plane->normal, tmp);
     else
		VectorCopy(surf->plane->normal, tmp);

     VectorMA(origin, 2, tmp, origin);
     VectorCopy(origin, light->origin);

     light->style = type;

     free (buffer);
}

void PART_RenderFlare (flare_t *light)
{
	vec3_t	v, tmp;
	int j;
	float	dist;
	unsigned	flaretex;

	if(light->style == 0)
		flaretex =  r_flare->texnum;
	else
		flaretex = r_flare1->texnum;

	VectorSubtract (light->origin, r_origin, v);
	dist = VectorLength(v) * (light->size*0.01);

	//limit their size to reasonable.
	if(dist > 10*light->size)
		dist = 10*light->size;

	R_InitQuadVarrays();

	qglDisable(GL_DEPTH_TEST);
	qglEnable (GL_TEXTURE_2D);
	GL_Bind(flaretex);
	qglEnableClientState( GL_COLOR_ARRAY );
	GL_TexEnv( GL_MODULATE );

	VectorScale(light->color, light->alpha, tmp );
	for (j=0; j<4; j++)
	   VA_SetElem4(col_array[j], tmp[0],tmp[1],tmp[2], 1);


	VectorMA (light->origin, -1-dist, vup, vert_array[0]);
	VectorMA (vert_array[0], 1+dist, vright, vert_array[0]);
    VA_SetElem2(tex_array[0], 0, 1);

	VectorMA (light->origin, -1-dist, vup, vert_array[1]);
	VectorMA (vert_array[1], -1-dist, vright, vert_array[1]);
    VA_SetElem2(tex_array[1], 0, 0);

    VectorMA (light->origin, 1+dist, vup, vert_array[2]);
	VectorMA (vert_array[2], -1-dist, vright, vert_array[2]);
    VA_SetElem2(tex_array[2], 1, 0);

	VectorMA (light->origin, 1+dist, vup, vert_array[3]);
	VectorMA (vert_array[3], 1+dist, vright, vert_array[3]);
    VA_SetElem2(tex_array[3], 1, 1);

	qglDrawArrays(GL_QUADS, 0 , 4);

	GL_TexEnv( GL_REPLACE );
	qglEnable(GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);
	qglDisableClientState(GL_COLOR_ARRAY);

	R_KillVArrays();

}

void R_RenderFlares (void)
{
	flare_t	*l;
    int i;
	qboolean visible;
	vec3_t mins, maxs;
	trace_t r_trace;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	if(!r_lensflare->value)
		return;
	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )return;
	qglDepthMask (0);
	qglDisable (GL_TEXTURE_2D);
	qglShadeModel (GL_SMOOTH);
	qglEnable (GL_BLEND);
	qglBlendFunc   (GL_SRC_ALPHA, GL_ONE);

    l = r_flares;
    for (i=0; i<r_numflares ; i++, l++)
	{

		// periodically test visibility to ramp alpha
		if(rs_realtime - l->time > 0.02){

			r_trace = CM_BoxTrace(r_origin, l->origin, mins, maxs, r_worldmodel->firstnode, MASK_VISIBILILITY);
			visible = r_trace.fraction == 1.0;

			l->alpha += (visible ? 0.03 : -0.15);  // ramp

			if(l->alpha > 0.5)  // clamp
				l->alpha = 0.5;
			else if(l->alpha < 0)
				l->alpha = 0.0;

			l->time = rs_realtime;
		}

		if (l->alpha > 0)
		{
			PART_RenderFlare (l);
				c_flares++;
		}
	}
	qglColor3f (1,1,1);
	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
    qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (1);
}

void R_ClearFlares(void)
{
	memset(r_flares, 0, sizeof(r_flares));
	r_numflares = 0;
}

/*
================
Sun Object
================
*/

void transform_point(float out[4], const float m[16], const float in[4])
{
#define M(row,col)  m[col*4+row]
	out[0] =
		M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0,
																3) * in[3];
	out[1] =
		M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1,
																3) * in[3];
	out[2] =
		M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2,
																3) * in[3];
	out[3] =
		M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3,
																3) * in[3];
#undef M
}

qboolean gluProject2(float objx, float objy, float objz, const float model[16], const float proj[16], const int viewport[4], float *winx, float *winy)
{
	/* transformation matrix */
	float in[4], out[4], temp;

	/* initialise the matrix and the vector to transform */
	in[0] = objx;
	in[1] = objy;
	in[2] = objz;
	in[3] = 1.0;
	transform_point(out, model, in);
	transform_point(in, proj, out);

	/* normalise result to [-1;1] */
	if (in[3] == 0.0)
		return false;

	temp = 1.0 / in[3];
	in[0] *= temp;
	in[1] *= temp;
	in[2] *= temp;

	/* display coordinates */
	*winx = viewport[0] + (1 + in[0]) * (float) viewport[2] * 0.5;
	*winy = viewport[1] + (1 + in[1]) * (float) viewport[3] * 0.5;
	return true;
}


float sun_time = 0;
float sun_alpha = 0;
void R_InitSun()
{
    draw_sun = false;

    if (!r_drawsun->value)
        return;

    if (spacebox)
        sun_size = 0.1f;
    else
        sun_size = 0.2f;

    if (R_CullOrigin(sun_origin))
        return;

    draw_sun = true;

    gluProject2(sun_origin[0], sun_origin[1], sun_origin[2], r_world_matrix, r_project_matrix, (int *) r_viewport, &sun_x, &sun_y); // /,
                                                                                                                                    // &sun_z);
    sun_y = r_newrefdef.height - sun_y;
}


void PART_RenderSunFlare(image_t * tex, float offset, float size, float r,
                      float g, float b, float alpha)
{
    float minx, miny, maxx, maxy;
    float new_x, new_y, corr;

    qglColor4f(r, g, b, alpha);
    GL_Bind(tex->texnum);

    if (offset) {
        new_x = offset * (r_newrefdef.width / 2 - sun_x) + sun_x;
        new_y = offset * (r_newrefdef.height / 2 - sun_y) + sun_y;
    } else {
        new_x = sun_x;
        new_y = sun_y;
    }

    corr = 1;

    minx = new_x - size * corr;
    miny = new_y - size;
    maxx = new_x + size * corr;
    maxy = new_y + size;

    qglBegin(GL_QUADS);
    qglTexCoord2f(0, 0);
    qglVertex2f(minx, miny);
    qglTexCoord2f(1, 0);
    qglVertex2f(maxx, miny);
    qglTexCoord2f(1, 1);
    qglVertex2f(maxx, maxy);
    qglTexCoord2f(0, 1);
    qglVertex2f(minx, maxy);
    qglEnd();
}

void R_RenderSun()
{
    float l, hx, hy;
    float vec[2];
    float size;

    if (!draw_sun)
        return;

    if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
        return;

    qglReadPixels(sun_x, r_newrefdef.height - sun_y, 1, 1,
                  GL_DEPTH_COMPONENT, GL_FLOAT, &l);

    // periodically test visibility to ramp alpha
    if(rs_realtime - sun_time > 0.02) {

        sun_alpha += (l == 1.0 ? 0.15 : -0.15);  // ramp

        if(sun_alpha > 1.0)  // clamp
            sun_alpha = 1.0;
        else if(sun_alpha < 0)
            sun_alpha = 0.0;

        sun_time = rs_realtime;
    }

    if (sun_alpha > 0)
    {

        hx = r_newrefdef.width / 2;
        hy = r_newrefdef.height / 2;
        vec[0] = 1 - fabs(sun_x - hx) / hx;
        vec[1] = 1 - fabs(sun_y - hy) / hy;
        l = 3 * vec[0] * vec[1] + 0.25;

        // set 2d
        qglMatrixMode(GL_PROJECTION);
        qglPushMatrix();
        qglLoadIdentity();
        qglOrtho(0, r_newrefdef.width, r_newrefdef.height, 0, -99999,
                 99999);
        qglMatrixMode(GL_MODELVIEW);
        qglPushMatrix();
        qglLoadIdentity();
        qglEnable(GL_BLEND);
        qglBlendFunc(GL_SRC_ALPHA, GL_ONE);
        GL_TexEnv(GL_MODULATE);
        qglDepthRange(0, 0.3);

        size = r_newrefdef.width * sun_size;
        PART_RenderSunFlare(sun_object, 0, size, .75, .75, .75, sun_alpha);
        if (r_drawsun->value == 2) {

            PART_RenderSunFlare(sun2_object, -0.9, size * 0.07, 0.1, 0.1, 0, sun_alpha);
            PART_RenderSunFlare(sun2_object, -0.7, size * 0.15, 0, 0, 0.1, sun_alpha);
            PART_RenderSunFlare(sun2_object, -0.5, size * 0.085, 0.1, 0, 0, sun_alpha);
            PART_RenderSunFlare(sun1_object, 0.3, size * 0.25, 0.1, 0.1, 0.1, sun_alpha);
            PART_RenderSunFlare(sun2_object, 0.5, size * 0.05, 0.1, 0, 0, sun_alpha);
            PART_RenderSunFlare(sun2_object, 0.64, size * 0.05, 0, 0.1, 0, sun_alpha);
            PART_RenderSunFlare(sun2_object, 0.7, size * 0.25, 0.1, 0.1, 0, sun_alpha);
            PART_RenderSunFlare(sun1_object, 0.85, size * 0.5, 0.1, 0.1, 0.1, sun_alpha);
            PART_RenderSunFlare(sun2_object, 1.1, size * 0.125, 0.1, 0, 0, sun_alpha);
            PART_RenderSunFlare(sun2_object, 1.25, size * 0.08, 0.1, 0.1, 0, sun_alpha);
        }

        qglDepthRange(0, 1);
        qglColor4f(1, 1, 1, 1);
        qglDisable(GL_BLEND);
        qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // set 3d
        qglPopMatrix();
        qglMatrixMode(GL_PROJECTION);
        qglPopMatrix();
        qglMatrixMode(GL_MODELVIEW);
    }
}

//Vegetation

void Mod_AddVegetationSurface (msurface_t *surf, int texnum, vec3_t color, float size, char name[MAX_QPATH], int type)
{
    glpoly_t *poly;
    grass_t  *grass;
	image_t *gl;
	vec3_t origin = {0,0,0}, binormal, tangent, tmp;

	if (r_numgrasses >= MAX_GRASSES)
			return;

	if(size == 0.0)
		size = 1.0f;

	poly = surf->polys;

	VectorCopy(poly->verts[0], origin);

	AngleVectors(surf->plane->normal, NULL, tangent, binormal);
	VectorNormalize(tangent);
	VectorNormalize(binormal);

	VectorMA(origin, -32*frand(), tangent, origin);

	if (surf->flags & SURF_PLANEBACK)
		VectorNegate(surf->plane->normal, tmp);
	else
		VectorCopy(surf->plane->normal, tmp);

	VectorMA(origin, 2, tmp, origin);

	grass = &r_grasses[r_numgrasses++];
	VectorCopy(origin, grass->origin);

	gl = R_RegisterGfxPic (name);
	if (gl)
		grass->texsize = gl->height;
	else
		grass->texsize = 64; //sane default

	grass->texnum = texnum;
	VectorCopy(color, grass->color);
	grass->size = size;
	strcpy(grass->name, name);
	grass->type = type;

	if(grass->type == 1)
		r_hasleaves = true;
}

//rendering

void R_DrawVegetationSurface ( void )
{
    int		i, k;
	grass_t *grass;
    float   scale;
	vec3_t	origin, mins, maxs, angle, right, up, corner[4];
	float	*corner0 = corner[0];
	qboolean visible;
	float	lightLevel[3];
	trace_t r_trace;
	float	sway;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	grass = r_grasses;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs,	0, 0, 0);

	R_InitVArrays (VERT_SINGLE_TEXTURED);

    for (i=0; i<r_numgrasses; i++, grass++) {

		scale = 10.0*grass->size;

		VectorCopy(r_newrefdef.viewangles, angle);

		if(!grass->type)
			angle[0] = 0;  // keep vertical by removing pitch(grass and plants grow upwards)

		AngleVectors(angle, NULL, right, up);
		VectorScale(right, scale, right);
		VectorScale(up, scale, up);
		VectorCopy(grass->origin, origin);

		// adjust vertical position, scaled
		origin[2] += (grass->texsize/32) * grass->size;

		if(!grass->type) {
			r_trace = CM_BoxTrace(r_origin, origin, maxs, mins, r_worldmodel->firstnode, MASK_VISIBILILITY);
			visible = r_trace.fraction == 1.0;
		}
		else
			visible = true; //leaves tend to use much larger images, culling results in undesired effects

		if(visible) {

			//render grass polygon
			qglDepthMask( GL_FALSE );
			qglEnable( GL_BLEND);
			qglBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			GL_Bind(grass->texnum);

			if(gl_dynamic->value)
				R_LightPoint (origin, lightLevel, true);
			else
				R_LightPoint (origin, lightLevel, false);
			VectorScale(lightLevel, 2.0, lightLevel);
			qglColor4f( grass->color[0]*(lightLevel[0]+0.1),grass->color[1]*(lightLevel[1]+0.1),grass->color[2]*(lightLevel[2]+0.1), 1 );
			GL_TexEnv( GL_MODULATE );

			VectorSet (corner[0],
				origin[0] + (up[0] + right[0])*(-0.5),
				origin[1] + (up[1] + right[1])*(-0.5),
				origin[2] + (up[2] + right[2])*(-0.5));

			//the next two statements create a slight swaying in the wind
			//perhaps we should add a parameter to control ammount in shader?

			if(grass->type) {
				sway = 3;
			}
			else
				sway = 2;

			VectorSet ( corner[1],
				corner0[0] + up[0] + sway*sin (rs_realtime*sway),
				corner0[1] + up[1] + sway*sin (rs_realtime*sway),
				corner0[2] + up[2]);

			VectorSet ( corner[2],
				corner0[0] + (up[0]+right[0] + sway*sin (rs_realtime*sway)),
				corner0[1] + (up[1]+right[1] + sway*sin (rs_realtime*sway)),
				corner0[2] + (up[2]+right[2]));

			VectorSet ( corner[3],
				corner0[0] + right[0],
				corner0[1] + right[1],
				corner0[2] + right[2]);

			VArray = &VArrayVerts[0];

			for(k = 0; k < 4; k++) {

				VArray[0] = corner[k][0];
				VArray[1] = corner[k][1];
				VArray[2] = corner[k][2];

				switch(k) {
					case 0:
						VArray[3] = 1;
						VArray[4] = 1;
						break;
					case 1:
						VArray[3] = 0;
						VArray[4] = 1;
						break;
					case 2:
						VArray[3] = 0;
						VArray[4] = 0;
						break;
					case 3:
						VArray[3] = 1;
						VArray[4] = 0;
						break;
				}

				VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			}

			if(qglLockArraysEXT)
				qglLockArraysEXT(0, 4);

			qglDrawArrays(GL_QUADS,0,4);

			if(qglUnlockArraysEXT)
				qglUnlockArraysEXT();

			c_grasses++;
		}
	}

	R_KillVArrays ();

	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	qglColor4f( 1,1,1,1 );
	qglDisable(GL_BLEND);
	qglDepthMask( GL_TRUE );
	GL_TexEnv( GL_REPLACE );
}

void R_ClearGrasses(void)
{
	memset(r_grasses, 0, sizeof(r_grasses));
	r_numgrasses = 0;
	r_hasleaves = 0;
}

//Light beams/volumes

void Mod_AddBeamSurface (msurface_t *surf, int texnum, vec3_t color, float size, char name[MAX_QPATH], int type, float xang, float yang,
	qboolean rotating)
{
    glpoly_t *poly;
    beam_t  *beam;
	image_t *gl;
	vec3_t poly_center, mins, maxs;
	float *v;
	int i;
	vec3_t origin = {0,0,0}, binormal, tangent, tmp;

	if (r_numbeams >= MAX_BEAMS)
			return;

	if(size == 0.0)
		size = 1.0f;

	poly = surf->polys;

	 /*
	===================
	find poligon centre
	===================
	*/
	VectorSet(mins, 999999, 999999, 999999);
	VectorSet(maxs, -999999, -999999, -999999);

    for ( poly = surf->polys; poly; poly = poly->chain ) {
         for (i=0, v=poly->verts[0] ; i< poly->numverts; i++, v+= VERTEXSIZE) {

               if(v[0] > maxs[0])   maxs[0] = v[0];
               if(v[1] > maxs[1])   maxs[1] = v[1];
               if(v[2] > maxs[2])   maxs[2] = v[2];

               if(v[0] < mins[0])   mins[0] = v[0];
               if(v[1] < mins[1])   mins[1] = v[1];
               if(v[2] < mins[2])   mins[2] = v[2];
            }
      }

     poly_center[0] = (mins[0] + maxs[0]) /2;
     poly_center[1] = (mins[1] + maxs[1]) /2;
     poly_center[2] = (mins[2] + maxs[2]) /2;
	 VectorCopy(poly_center, origin);

	AngleVectors(surf->plane->normal, NULL, tangent, binormal);
	VectorNormalize(tangent);
	VectorNormalize(binormal);

	if (surf->flags & SURF_PLANEBACK)
		VectorNegate(surf->plane->normal, tmp);
	else
		VectorCopy(surf->plane->normal, tmp);

	VectorMA(origin, 2, tmp, origin);

	beam = &r_beams[r_numbeams++];
	VectorCopy(origin, beam->origin);

	gl = R_RegisterGfxPic (name);
	if (gl)
		beam->texsize = gl->height;
	else
		beam->texsize = 64; //sane default

	beam->texnum = texnum;
	VectorCopy(color, beam->color);
	beam->size = size;
	strcpy(beam->name, name);
	beam->type = type;
	beam->xang = xang;
	beam->yang = yang;
	beam->rotating = rotating;
}

//Rendering

void R_DrawBeamSurface ( void )
{
    int		i, k;
	beam_t *beam;
    double   scale, maxang;
	vec3_t	start, end, mins, maxs, angle, right, up, move, delta, vec, corner[4];
	float	*corner0 = corner[0];
	qboolean visible;
	trace_t r_trace;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	beam = r_beams;

	VectorSet(mins, 32, 32, 64);
	VectorSet(maxs, -32, -32, -64);

	R_InitVArrays (VERT_SINGLE_TEXTURED);

    for (i=0; i<r_numbeams; i++, beam++) {
		float movdir;

		scale = 10.0*beam->size;

		if(fabs(beam->xang) > fabs(beam->yang))
			maxang = beam->xang;
		else
			maxang = beam->yang;

		if(maxang >= 0.0)
			movdir = 1.0;
		else
			movdir = 0;
		
		//to do - this is rather hacky, and really only works for 0 degrees and 45 degree angles(0.25 rads).  
		//will revisit this as needed, for now it works for what I need it for.
		VectorCopy(beam->origin, start);
		if(!beam->type)
			start[2] -= (2.5 - (10.0*maxang))*beam->size*movdir;
		else
			start[2] += (2.5 - (10.0*maxang))*beam->size*movdir;
		
		VectorCopy(start, end);
		if(!beam->type)
			end[2] -= (2.5 + pow(fabs(maxang),2)*10)*beam->size;
		else
			end[2] += (2.5 + pow(fabs(maxang),2)*10)*beam->size;

		if(beam->rotating)
		{
			end[0] += sin(rs_realtime)*(pow(fabs(maxang*10), 3)*beam->xang)*beam->size; //angle in rads
			end[1] += cos(rs_realtime)*(pow(fabs(maxang*10), 3)*beam->yang)*beam->size;
		}
		else
		{
			end[0] += (pow(fabs(maxang*10), 3)*beam->xang)*beam->size; //angle in rads
			end[1] += (pow(fabs(maxang*10), 3)*beam->yang)*beam->size;
		}

		VectorSubtract(end, start, vec);
		if(!beam->type)
			VectorScale(vec, beam->size, vec);
		else
			VectorScale(vec, -beam->size, vec);

		VectorAdd(start, vec, angle);

		VectorSubtract(start, angle, move);
		VectorNormalize(move);

		VectorCopy(move, up);
		VectorSubtract(r_newrefdef.vieworg, angle, delta);
		CrossProduct(up, delta, right);
		VectorNormalize(right);

		VectorScale(right, scale, right);
		VectorScale(up, scale, up);
		
		r_trace = CM_BoxTrace(r_origin, beam->origin, mins, maxs, r_worldmodel->firstnode, MASK_VISIBILILITY);
		visible = r_trace.fraction == 1.0;

		if(visible) {

			//render polygon
			qglDepthMask( GL_FALSE );
			qglEnable( GL_BLEND);
			qglBlendFunc   (GL_SRC_ALPHA, GL_ONE);

			qglColor4f( beam->color[0],beam->color[1],beam->color[2], 1 );
			GL_TexEnv( GL_MODULATE );

			GL_Bind(beam->texnum);

			VectorSet (corner[0],
				end[0] + (up[0] + right[0])*(-0.5),
				end[1] + (up[1] + right[1])*(-0.5),
				end[2] + (up[2] + right[2])*(-0.5));

			VectorSet ( corner[1],
				corner0[0] + up[0],
				corner0[1] + up[1],
				corner0[2] + up[2]);

			VectorSet ( corner[2],
				corner0[0] + (up[0]+right[0]),
				corner0[1] + (up[1]+right[1]),
				corner0[2] + (up[2]+right[2]));

			VectorSet ( corner[3],
				corner0[0] + right[0],
				corner0[1] + right[1],
				corner0[2] + right[2]);

			VArray = &VArrayVerts[0];

			for(k = 0; k < 4; k++) {

				VArray[0] = corner[k][0];
				VArray[1] = corner[k][1];
				VArray[2] = corner[k][2];

				switch(k) {
					case 0:
						VArray[3] = 1;
						VArray[4] = 1;
						break;
					case 1:
						VArray[3] = 0;
						VArray[4] = 1;
						break;
					case 2:
						VArray[3] = 0;
						VArray[4] = 0;
						break;
					case 3:
						VArray[3] = 1;
						VArray[4] = 0;
						break;
				}

				VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			}

			if(qglLockArraysEXT)
				qglLockArraysEXT(0, 4);

			qglDrawArrays(GL_QUADS,0,4);

			if(qglUnlockArraysEXT)
				qglUnlockArraysEXT();

			c_beams++;
		}
	}

	R_KillVArrays ();

	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	qglColor4f( 1,1,1,1 );
	qglDisable(GL_BLEND);
	qglDepthMask( GL_TRUE );
	GL_TexEnv( GL_REPLACE );
}

void R_ClearBeams(void)
{
	memset(r_beams, 0, sizeof(r_beams));
	r_numbeams = 0;
}