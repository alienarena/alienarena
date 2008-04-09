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
// r_light.c

#include "r_local.h"

int	r_dlightframecount;

// #define	DLIGHT_CUTOFF	64
#define DLIGHT_CUTOFF	0

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	cplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->intensity-DLIGHT_CUTOFF)
	{
		R_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->intensity+DLIGHT_CUTOFF)
	{
		R_MarkLights (light, bit, node->children[1]);
		return;
	}

// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLights (light, bit, node->children[0]);
	R_MarkLights (light, bit, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int		i;
	dlight_t	*l;

	if (gl_flashblend->value)
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	l = r_newrefdef.dlights;
	for (i=0 ; i<r_newrefdef.num_dlights ; i++, l++)
		R_MarkLights ( l, 1<<i, r_worldmodel->nodes );
}

/*
=============
R_PushDlightsForBModel
=============
*/
void R_PushDlightsForBModel (entity_t *e)
{
	int			k;
	dlight_t	*lt;

	lt = r_newrefdef.dlights;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		AngleVectors (e->angles, forward, right, up);

		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
		{
			VectorSubtract (lt->origin, e->origin, temp);
			lt->origin[0] = DotProduct (temp, forward);
			lt->origin[1] = -DotProduct (temp, right);
			lt->origin[2] = DotProduct (temp, up);
			R_MarkLights (lt, 1<<k, e->model->nodes + e->model->firstnode);
			VectorAdd (temp, e->origin, lt->origin);
		}
	}
	else
	{
		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
		{
			VectorSubtract (lt->origin, e->origin, lt->origin);
			R_MarkLights (lt, 1<<k, e->model->nodes + e->model->firstnode);
			VectorAdd (lt->origin, e->origin, lt->origin);
		}
	}
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t			pointcolor;
cplane_t		*lightplane;		// used as shadow plane
vec3_t			lightspot;

int RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	int			maps;
	int			r;
	vec3_t		scale;

	if (node->contents != -1)
		return -1;		// didn't hit anything

// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;

	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end);

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

// go down front side
	r = RecursiveLightPoint (node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something

	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing

// check for impact on this node
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		tex = surf->texinfo;

		if (tex->flags & (SURF_WARP|SURF_SKY))
			continue;	// no lightmaps

		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		if (s < surf->texturemins[0])
			continue;

		ds = s - surf->texturemins[0];
		if (ds > surf->extents[0])
			continue;

		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];
		if (t < surf->texturemins[1])
			continue;

		dt = t - surf->texturemins[1];
		if (dt > surf->extents[1])
			continue;

		lightmap = surf->samples;
		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		VectorClear (pointcolor);

		lightmap += 3*(dt * ((surf->extents[0]>>4)+1) + ds);

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;	maps++)
		{
			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			pointcolor[0] += lightmap[0] * scale[0] * (1.0/255);
			pointcolor[1] += lightmap[1] * scale[1] * (1.0/255);
			pointcolor[2] += lightmap[2] * scale[2] * (1.0/255);
			lightmap += 3*((surf->extents[0]>>4)+1) * ((surf->extents[1]>>4)+1);
		}

		return 1;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end);
}

/*
===============
R_LightPoint
===============
*/
void R_LightPoint (vec3_t p, vec3_t color, qboolean addDynamic)
{
	vec3_t		end;
	float		r;
	int			lnum;
	dlight_t	*dl;
	float		light;
	vec3_t		dist, dlightcolor;
	float		add;

	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	r = RecursiveLightPoint (r_worldmodel->nodes, p, end);

	if (r == -1)
	{
		VectorCopy (vec3_origin, color);
	}
	else
	{
		VectorCopy (pointcolor, color);
	}

	if (!addDynamic)
		return;

	//
	// add dynamic lights
	//
	light = 0;
	dl = r_newrefdef.dlights;
	VectorClear ( dlightcolor );
	for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++, dl++)
	{
		VectorSubtract (currententity->origin,
						dl->origin,
						dist);
		add = dl->intensity - VectorLength(dist);
		if (add > 0)
		{
			add *= (1.0/256);
			VectorMA (dlightcolor, add, dl->color, dlightcolor);
		}
	}

	VectorMA (color, gl_modulate->value, dlightcolor, color);
}

void R_LightPointDynamics (vec3_t p, vec3_t color, m_dlight_t *list, int *amount, int max)
{
	vec3_t		end;
	float		r;
	int			lnum, i, m_dl;
	dlight_t	*dl;
	vec3_t		dist, dlColor;
	float		add;
	worldLight_t *wl;

	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	r = RecursiveLightPoint (r_worldmodel->nodes, p, end);

	if (r == -1)
	{
		VectorCopy (vec3_origin, color);
	}
	else
	{
		VectorCopy (pointcolor, color);
	}

	//this catches too bright modulated color
	for (i=0;i<3;i++)
		if (color[i]>1) color[i] = 1;

	m_dl = 0;

	//add world lights
	for (lnum=0; lnum<r_numWorldLights; lnum++) {

		wl = &r_worldLights[lnum];

		VectorSubtract (wl->origin, p, dist);

		add = wl->intensity - VectorNormalize(dist)/3;
		add *= (DIV256);
		if (add > 0)
		{
			float highest = -1;


			for (i=0;i<3;i++)
				dlColor[i] = 1;

			if (m_dl<max)
			{
				list[m_dl].strength = add;
				VectorCopy(dist, list[m_dl].direction);
				VectorScale(dlColor, add, dlColor);
				VectorCopy(dlColor, list[m_dl].color);
				m_dl++;
			}
		}
	}

	//
	// add dynamic lights
	//
	dl = r_newrefdef.dlights;
	for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++, dl++)
	{

		VectorSubtract (dl->origin, p, dist);

		add = dl->intensity - VectorNormalize(dist);
		add *= (DIV256);
		if (add > 0)
		{
			float highest = -1;

			VectorScale(dl->color, add, dlColor);
			for (i=0;i<3;i++)
				if (highest<dlColor[i]) highest = dlColor[i];

			if (m_dl<max)
			{
				list[m_dl].strength = highest;
				VectorCopy(dist, list[m_dl].direction);
				VectorCopy(dlColor, list[m_dl].color);
				m_dl++;
			}
			else
			{
				float	least_val = 10;
				int		least_index = 0;

				for (i=0;i<m_dl;i++)
					if (list[i].strength<least_val)
					{

						least_val = list[i].strength;
						least_index = i;
					}

				VectorAdd (color, list[least_index].color, color);
				list[least_index].strength = highest;
				VectorCopy(dist, list[least_index].direction);
				VectorCopy(dlColor, list[least_index].color);
			}
		}
	}

	*amount = m_dl;


}

//===================================================================

/*
=============
R_AddStainMap
=============
*/
void R_AddStainMap (stain_t *st, msurface_t *surf)
{
	int			i;
	mtexinfo_t	*tex;
	int			sd, td;
	float		fsacc, ftacc;
	float		dist, fdist, frad, fminlight;
	vec3_t		impact, local;
	int			s, t, smax, tmax;
	byte		*pfBL;

	if (!surf->stainsamples)
		return;
	tex = surf->texinfo;
	if ( (tex->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) ) )
		return;

	if ( surf->plane->type < 3 ) {
		dist = st->origin[surf->plane->type] - surf->plane->dist;
	} else {
		dist = DotProduct (st->origin, surf->plane->normal) - surf->plane->dist;
	}

	if (surf->flags & SURF_PLANEBACK)
		fdist = -dist;
	else
		fdist = dist;

	frad = st->intensity - fabs (fdist);
	if (frad < 0)
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	fminlight = frad;

	for (i = 0 ; i<3 ; i++)
		impact[i] = st->origin[i] - surf->plane->normal[i] * fdist;

	local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
	local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

	pfBL = surf->stainsamples;
	surf->cached_light[0] = 0;

	for (t = 0, ftacc = 0 ; t<tmax ; t++, ftacc += 16)
	{
		td = local[1] - ftacc;
		if ( td < 0 )
			td = -td;
		for ( s=0, fsacc = 0 ; s<smax ; s++, fsacc += 16, pfBL += 3)
		{
			sd = Q_ftol( local[0] - fsacc );

			if ( sd < 0 )
				sd = -sd;

			if (sd > td)
				fdist = sd + (td>>1);
			else
				fdist = td + (sd>>1);

			if ( fdist < fminlight )
			{
				float alpha, mult;
				int test;

				mult = frad / fdist;
				if (mult > 5.0)
					mult = 5.0;

				alpha = st->alpha * mult;
				if (alpha > 255)
					alpha = 255;
				if (alpha > st->alpha)
					alpha = st->alpha;
				if (alpha < 0)
					alpha = 0;
				alpha *= (1.0 / 255.0);

				for (i=0;i<3;i++)
				{
					test = Q_ftol( (1-alpha) * (float)pfBL[i] + alpha * st->color[i] );

					if (test > 255)
						pfBL[i] = 255;
					else if (test < 0)
						pfBL[i] = 0;
					else
						pfBL[i] = (byte)(test);
				}
			}
		}
	}
}

/*
=============
R_StainNode
=============
*/
void R_StainNode (stain_t *st, mnode_t *node)
{
	int		c;
	float	dist;
	msurface_t *surf;

	if (node->contents != -1)
		return;

	if ( node->plane->type < 3 ) {
		dist = st->origin[node->plane->type] - node->plane->dist;
	} else {
		dist = DotProduct (st->origin, node->plane->normal) - node->plane->dist;
	}

	if (dist > st->intensity)
	{
		R_StainNode (st, node->children[0]);
		return;
	}

	if (dist < -st->intensity)
	{
		R_StainNode (st, node->children[1]);
		return;
	}

	for (c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		R_AddStainMap ( st, surf );
	}

	if (node->contents == -1)
		R_StainNode (st, node->children[0]);
	if (node->contents == -1)
		R_StainNode (st, node->children[1]);
}

/*
=============
R_PushStains
=============
*/
void R_PushStains (void)
{
	int		i;
	stain_t	*st;

	st = r_newrefdef.stains;
	for (i=0 ; i<r_newrefdef.num_stains ; i++, st++) {
		R_StainNode ( st, r_worldmodel->nodes );
	}
}

/*
=============
R_PushStainsForBModel
=============
*/
void R_PushStainsForBModel (entity_t *e)
{
	int i, c;
	stain_t	*st;
	model_t *model;
	msurface_t *surf;

	st = r_newrefdef.stains;
	model = e->model;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		vec3_t temp;
		vec3_t forward, right, up;

		AngleVectors ( e->angles, forward, right, up );

		for (i=0 ; i<r_newrefdef.num_stains ; i++, st++) {
			VectorSubtract (st->origin, e->origin, temp);
			st->origin[0] = DotProduct (temp, forward);
			st->origin[1] = -DotProduct (temp, right);
			st->origin[2] = DotProduct (temp, up);

			surf = &model->surfaces[model->firstmodelsurface];
			for (c=0 ; c<model->nummodelsurfaces ; c++, surf++)
			{
				R_AddStainMap ( st, surf );
			}

			VectorAdd ( temp, e->origin, st->origin );
		}
	}
	else
	{
		for (i=0 ; i<r_newrefdef.num_stains ; i++, st++) {
			VectorSubtract (st->origin, e->origin, st->origin);

			surf = &model->surfaces[model->firstmodelsurface];
			for (c=0 ; c<model->nummodelsurfaces ; c++, surf++)
			{
				R_AddStainMap ( st, surf );
			}

			VectorAdd ( st->origin, e->origin, st->origin );
		}
	}
}

//===================================================================

static float s_blocklights[34*34*3];

/*
===============
R_AddStainMaps
===============
*/
void R_AddStainMaps (msurface_t *surf)
{
	int			s, t, i;
	int			smax, tmax;
	float		*pfBL;
	byte		*pfBS;
	float		scale;

	scale = gl_modulate->value;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	pfBL = s_blocklights;
	pfBS = surf->stainsamples;

	for (t = 0; t<tmax; t++)
		for ( s=0; s<smax; s++, pfBL += 3, pfBS +=3)
			for (i=0;i<3;i++)
			{
				if (pfBL[i] > pfBS[i]*scale)
					pfBL[i] = pfBS[i]*scale;
			}
}

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights (msurface_t *surf)
{
	int			lnum;
	int			sd, td;
	float		fdist, frad, fminlight;
	vec3_t		impact, local, dlorigin;
	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;
	dlight_t	*dl;
	float		*pfBL;
	float		fsacc, ftacc;
	qboolean	rotated = false;
	vec3_t		temp;
	vec3_t		forward, right, up;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	tex = surf->texinfo;

	if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
	{
		rotated = true;
		AngleVectors (currententity->angles, forward, right, up);
	}

	dl = r_newrefdef.dlights;
	for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++, dl++)
	{
		if ( !(surf->dlightbits & (1<<lnum) ) )
			continue;		// not lit by this light

		frad = dl->intensity;
		VectorCopy ( dl->origin, dlorigin );

		VectorSubtract (dlorigin, currententity->origin, dlorigin);
		if (rotated)
		{
			VectorCopy (dlorigin, temp);
			dlorigin[0] = DotProduct (temp, forward);
			dlorigin[1] = -DotProduct (temp, right);
			dlorigin[2] = DotProduct (temp, up);
		}

		fdist = DotProduct (dlorigin, surf->plane->normal) - surf->plane->dist;
		frad -= fabs(fdist);
		// rad is now the highest intensity on the plane

		fminlight = DLIGHT_CUTOFF;	// FIXME: make configurable?
		if (frad < fminlight)
			continue;
		fminlight = frad - fminlight;

		for (i=0 ; i<3 ; i++)
		{
			impact[i] = dlorigin[i] -
					surf->plane->normal[i]*fdist;
		}

		local[0] = DotProduct (impact, tex->vecs[0]) + tex->vecs[0][3] - surf->texturemins[0];
		local[1] = DotProduct (impact, tex->vecs[1]) + tex->vecs[1][3] - surf->texturemins[1];

		pfBL = s_blocklights;
		for (t = 0, ftacc = 0 ; t<tmax ; t++, ftacc += 16)
		{
			td = local[1] - ftacc;
			if ( td < 0 )
				td = -td;

			for ( s=0, fsacc = 0 ; s<smax ; s++, fsacc += 16, pfBL += 3)
			{
				sd = Q_ftol( local[0] - fsacc );

				if ( sd < 0 )
					sd = -sd;

				if (sd > td)
					fdist = sd + (td>>1);
				else
					fdist = td + (sd>>1);

				if ( fdist < fminlight )
				{
					pfBL[0] += ( fminlight - fdist ) * dl->color[0];
					pfBL[1] += ( fminlight - fdist ) * dl->color[1];
					pfBL[2] += ( fminlight - fdist ) * dl->color[2];
				}
			}
		}
	}
}


/*
** R_SetCacheState
*/
void R_SetCacheState( msurface_t *surf )
{
	int maps;

	for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
		 maps++)
	{
		surf->cached_light[maps] = r_newrefdef.lightstyles[surf->styles[maps]].white;
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte *dest, int stride)
{
	int			smax, tmax;
	int			r, g, b, a, max;
	int			i, j, size;
	byte		*lightmap;
	float		scale[4];
	int			nummaps;
	float		*bl;
	lightstyle_t	*style;

	if ( SurfaceHasNoLightmap( surf ) )
		Com_Error (ERR_DROP, "R_BuildLightMap called for non-lit surface");

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;
	size = smax*tmax;
	if (size > (sizeof(s_blocklights)>>4) )
		Com_Error (ERR_DROP, "Bad s_blocklights size");

// set to full bright if no light data
	if (!surf->samples)
	{
		int maps;

		for (i=0 ; i<size*3 ; i++)
			s_blocklights[i] = 255;
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			style = &r_newrefdef.lightstyles[surf->styles[maps]];
		}
		goto store;
	}

	// count the # of maps
	for ( nummaps = 0 ; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255 ;
		 nummaps++)
		;

	lightmap = surf->samples;

	// add all the lightmaps
	if ( nummaps == 1 )
	{
		int maps;

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}
	else
	{
		int maps;

		memset( s_blocklights, 0, sizeof( s_blocklights[0] ) * size * 3 );

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3 )
				{
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}

// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights (surf);

	if (surf->stainsamples)
		R_AddStainMaps (surf);

// put into texture format
store:
	stride -= (smax<<2);
	bl = s_blocklights;

	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			r = Q_ftol( bl[0] );
			g = Q_ftol( bl[1] );
			b = Q_ftol( bl[2] );

			// catch negative lights
			if (r < 0)
				r = 0;
			if (g < 0)
				g = 0;
			if (b < 0)
				b = 0;
			/*
			** determine the brightest of the three color components
			*/
			if (r > g)
				max = r;
			else
				max = g;
			if (b > max)
				max = b;

			//255 is best for alpha testing, so textures don't "disapear" in the dark
			a = 255;

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			*/
			if (max > 255)
			{
				float t = 255.0F / max;
				r = r*t;
				g = g*t;
				b = b*t;
				a = a*t;
			}

			dest[0] = r;
			dest[1] = g;
			dest[2] = b;
			dest[3] = a;

			bl += 3;
			dest += 4;
		}
	}
}

//lens flares
extern int c_flares;
void R_RenderFlare (flare_t *light)
{
	vec3_t	v, tmp;
	int j;
	float	dist;
	unsigned	flaretex;

    flaretex =  r_flare->texnum;

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

int r_numflares;
flare_t r_flares[MAX_FLARES];
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
			
			r_trace = CM_BoxTrace(r_origin, l->origin, mins, maxs, r_worldmodel->firstnode, MASK_SHOT);
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
			R_RenderFlare (l);
				c_flares++;
		}
	}
	qglColor3f (1,1,1);
	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
    qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (1);
}

/*
===============
R_ShadowLight
===============
*/
void vectoangles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
	// PMM - fixed to correct for pitch of 0
		if (value1[0])
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

float R_ShadowLight (vec3_t pos, vec3_t lightAdd, int type)
{
	int			lnum, i;
	dlight_t	*dl;
	worldLight_t *wl;
	vec3_t		dist, angle;
	float		add, /* len,*/ shadowdist;
	float   	lintens, intens = 0;

	if (!r_worldmodel)
		return 0;
	if (!r_worldmodel->lightdata) //keep old lame shadow
		return 0;

	VectorClear(lightAdd);
	//
	// add dynamic light shadow angles
	//
	if(!type) {
		dl = r_newrefdef.dlights;
		for (lnum=0; lnum<r_newrefdef.num_dlights; lnum++, dl++)
		{

			VectorSubtract (dl->origin, pos, dist);
			add = sqrt(dl->intensity - VectorLength(dist));
			VectorNormalize(dist);
			if (add > 0)
			{
				VectorScale(dist, add, dist);
				VectorAdd (lightAdd, dist, lightAdd);
				intens = 0.3;
			}
		}
	}
	//
	// add world light shadow angles
	//
	if(gl_shadows->integer == 2 && type) {

		for (i=0; i<r_numWorldLights; i++) {

			wl = &r_worldLights[i];
			VectorSubtract (wl->origin, pos, dist);
			add = sqrt(wl->intensity - VectorLength(dist)/3);
			VectorNormalize(dist);
			if (add > 0)
			{
				VectorScale(dist, add, dist);
				VectorAdd (lightAdd, dist, lightAdd);
				lintens = wl->intensity;
				if(lintens < 300)
					lintens = 300;
				intens+=(lintens/1000 - VectorLength(dist)/30); //darken shadows where light is stronger
			}
		}
		//cap some limits of lightness, darkness, subtley.
		if (intens < 0.3)
			intens = 0.3;
		if (intens > 0.5)
			intens = 0.5;
	}

	// Barnes improved code
	shadowdist = VectorNormalize(lightAdd);
	if (shadowdist > 4) shadowdist = 4;
	if (shadowdist <= 0) // old style static shadow
	{
		angle[PITCH] = currententity->angles[PITCH];
		angle[YAW] =   -currententity->angles[YAW];
		angle[ROLL] =   currententity->angles[ROLL];
		shadowdist = 1;
	}
	else // shadow from dynamic lights
	{
		vectoangles (lightAdd, angle);
		angle[YAW] -= currententity->angles[YAW];
	}
	AngleVectors (angle, dist, NULL, NULL);
	VectorScale (dist, shadowdist, lightAdd);
	// end Barnes improved code

	return intens;
}
