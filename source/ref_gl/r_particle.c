/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// r_particle.c
#include "r_local.h"

gparticle_t gparticles[MAX_PARTICLES];
int			num_gparticles;

/*
===============
GL_DrawParticles
===============
*/
static float yawOrRoll;
void GL_DrawParticles( int num_particles, gparticle_t particles[], const unsigned colortable[768])
{
	const gparticle_t *p;
	int				i, k;
	vec3_t			corner[4], up, right, pup, pright, dir;
	float			scale, oldscale;
	byte			color[4], oldcolor[4];
	int			    oldtype;
	int				texnum, blenddst, blendsrc;
	float			*corner0 = corner[0];
	vec3_t move, delta, v;

	if ( !num_particles )
		return;

	qglDepthMask( GL_FALSE );	 	// no z buffering
	qglEnable( GL_BLEND);
	GL_TexEnv( GL_MODULATE );

	R_InitVArrays (VERT_SINGLE_TEXTURED);

	for ( p = particles, i=0, oldtype=-1 ; i < num_particles ; i++,p++)
	{

		if(p->type == PARTICLE_NONE || p->type == PARTICLE_WEATHER){
			if(r_weather == 1)
				texnum = r_raintexture->texnum; //rain
			else
				texnum = r_particletexture->texnum; //snow
			scale = 1;
			*(int *)color = colortable[p->color];
			blendsrc = GL_SRC_ALPHA;
			blenddst = GL_ONE;
		}
		else {
			texnum = p->texnum;
			blendsrc = p->blendsrc;
			blenddst = p->blenddst;
			scale = p->scale;
			*(int *)color = colortable[p->color];
		}

		color[3] = p->alpha*255;

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

			VectorSubtract(p->origin, p->angle, move);
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

			if(p->origin[2] > r_newrefdef.vieworg[2]){  // it's above us
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
			p->origin[0] + (pup[0] + pright[0])*(-0.5),
			p->origin[1] + (pup[1] + pright[1])*(-0.5),
			p->origin[2] + (pup[2] + pright[2])*(-0.5));

		VectorSet ( corner[1],
			corner0[0] + pup[0], corner0[1] + pup[1], corner0[2] + pup[2]);
		VectorSet ( corner[2], corner0[0] + (pup[0]+pright[0]),
			corner0[1] + (pup[1]+pright[1]), corner0[2] + (pup[2]+pright[2]));
		VectorSet ( corner[3],
			corner0[0] + pright[0], corner0[1] + pright[1], corner0[2] + pright[2]);
	

		//to do - VBO first attempt here, since this is the simplest example of VA in the game
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
R_SortParticles
===============
*/
#define R_CopyParticle(a,b) (a.type=b.type,a.alpha=b.alpha,a.color=b.color,a.dist=b.dist,a.scale=b.scale, a.texnum=b.texnum, a.blenddst=b.blenddst, a.blendsrc=b.blendsrc, VectorCopy(b.origin,a.origin), VectorCopy(b.angle, a.angle))

static void R_SortParticles (gparticle_t *particles, int Li, int Ri)
{
	int pivot;
	gparticle_t temppart;
	int li, ri;

mark0:
	li = Li;
	ri = Ri;

	pivot = particles[(Li+Ri) >> 1].type;
	while (li < ri)
	{
		while (particles[li].type < pivot) li++;
		while (particles[ri].type > pivot) ri--;

		if (li <= ri)
		{
			R_CopyParticle( temppart, particles[ri] );
			R_CopyParticle( particles[ri], particles[li] );
			R_CopyParticle( particles[li], temppart );

			li++;
			ri--;
		}
	}

	if ( Li < ri ) {
		R_SortParticles( particles, Li, ri );
	}
	if (li < Ri) {
		Li = li;
		goto mark0;
	}
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	int				i;
	float			dist;
	gparticle_t		*gp;
	const particle_t *p;

	if ( !r_newrefdef.num_particles )
		return;

	gp = gparticles;
	num_gparticles = 0;
	for ( p = r_newrefdef.particles, i=0 ; i < r_newrefdef.num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		dist = ( p->origin[0] - r_origin[0] ) * vpn[0] +
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (dist < 0)
			continue;
		else if (dist >= 40)
			dist = 2 + dist * 0.004;
		else
			dist = 2;

		gp->alpha = p->alpha;
		gp->color = p->color;
		gp->type = p->type;
		gp->dist = dist;
		gp->scale = p->scale;
		gp->texnum = p->texnum;
		gp->blenddst = p->blenddst;
		gp->blendsrc = p->blendsrc;
		VectorCopy ( p->origin, gp->origin );
		VectorCopy ( p->angle, gp->angle );

		gp++;
		num_gparticles++;

	}
	if(map_fog)
		qglDisable(GL_FOG);
	R_SortParticles ( gparticles, 0, num_gparticles - 1 );

	// we are always going to used textured particles!
	GL_DrawParticles( num_gparticles, gparticles, d_8to24table);
	if(map_fog)
		qglEnable(GL_FOG);
}