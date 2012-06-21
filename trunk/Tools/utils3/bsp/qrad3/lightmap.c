#include "qrad.h"

#define	MAX_LSTYLES	256

typedef struct
{
	dface_t		*faces[2];
	qboolean	coplanar;
} edgeshare_t;

edgeshare_t	edgeshare[MAX_MAP_EDGES];

int			facelinks[MAX_MAP_FACES];
int			planelinks[2][MAX_MAP_PLANES];

/*
============
LinkPlaneFaces
============
*/
void LinkPlaneFaces (void)
{
	int		i;
	dface_t	*f;

	f = dfaces;
	for (i=0 ; i<numfaces ; i++, f++)
	{
		facelinks[i] = planelinks[f->side][f->planenum];
		planelinks[f->side][f->planenum] = i;
	}
}

/*
============
PairEdges
============
*/
void PairEdges (void)
{
	int		i, j, k;
	dface_t	*f;
	edgeshare_t	*e;

	f = dfaces;
	for (i=0 ; i<numfaces ; i++, f++)
	{
		for (j=0 ; j<f->numedges ; j++)
		{
			k = dsurfedges[f->firstedge + j];
			if (k < 0)
			{
				e = &edgeshare[-k];
				e->faces[1] = f;
			}
			else
			{
				e = &edgeshare[k];
				e->faces[0] = f;
			}

			if (e->faces[0] && e->faces[1])
			{
				// determine if coplanar
				if (e->faces[0]->planenum == e->faces[1]->planenum)
					e->coplanar = true;
			}
		}
	}
}

/*
=================================================================

  POINT TRIANGULATION

=================================================================
*/

typedef struct triedge_s
{
	int			p0, p1;
	vec3_t		normal;
	vec_t		dist;
	struct triangle_s	*tri;
} triedge_t;

typedef struct triangle_s
{
	triedge_t	*edges[3];
} triangle_t;

#define	MAX_TRI_POINTS		1024
#define	MAX_TRI_EDGES		(MAX_TRI_POINTS*6)
#define	MAX_TRI_TRIS		(MAX_TRI_POINTS*2)

typedef struct
{
	int			numpoints;
	int			numedges;
	int			numtris;
	dplane_t	*plane;
	triedge_t	*edgematrix[MAX_TRI_POINTS][MAX_TRI_POINTS];
	patch_t		*points[MAX_TRI_POINTS];
	triedge_t	edges[MAX_TRI_EDGES];
	triangle_t	tris[MAX_TRI_TRIS];
} triangulation_t;

/*
===============
AllocTriangulation
===============
*/
triangulation_t	*AllocTriangulation (dplane_t *plane)
{
	triangulation_t	*t;

	t = malloc(sizeof(triangulation_t));
	t->numpoints = 0;
	t->numedges = 0;
	t->numtris = 0;

	t->plane = plane;

//	memset (t->edgematrix, 0, sizeof(t->edgematrix));

	return t;
}

/*
===============
FreeTriangulation
===============
*/
void FreeTriangulation (triangulation_t *tr)
{
	free (tr);
}


triedge_t	*FindEdge (triangulation_t *trian, int p0, int p1)
{
	triedge_t	*e, *be;
	vec3_t		v1;
	vec3_t		normal;
	vec_t		dist;

	if (trian->edgematrix[p0][p1])
		return trian->edgematrix[p0][p1];

	if (trian->numedges > MAX_TRI_EDGES-2)
		Error ("trian->numedges > MAX_TRI_EDGES-2");

	VectorSubtract (trian->points[p1]->origin, trian->points[p0]->origin, v1);
	VectorNormalize (v1, v1);
	CrossProduct (v1, trian->plane->normal, normal);
	dist = DotProduct (trian->points[p0]->origin, normal);

	e = &trian->edges[trian->numedges];
	e->p0 = p0;
	e->p1 = p1;
	e->tri = NULL;
	VectorCopy (normal, e->normal);
	e->dist = dist;
	trian->numedges++;
	trian->edgematrix[p0][p1] = e;

	be = &trian->edges[trian->numedges];
	be->p0 = p1;
	be->p1 = p0;
	be->tri = NULL;
	VectorSubtract (vec3_origin, normal, be->normal);
	be->dist = -dist;
	trian->numedges++;
	trian->edgematrix[p1][p0] = be;

	return e;
}

triangle_t	*AllocTriangle (triangulation_t *trian)
{
	triangle_t	*t;

	if (trian->numtris >= MAX_TRI_TRIS)
		Error ("trian->numtris >= MAX_TRI_TRIS");

	t = &trian->tris[trian->numtris];
	trian->numtris++;

	return t;
}

/*
============
TriEdge_r
============
*/
void TriEdge_r (triangulation_t *trian, triedge_t *e)
{
	int		i, bestp = 0;
	vec3_t	v1, v2;
	vec_t	*p0, *p1, *p;
	vec_t	best, ang;
	triangle_t	*nt;

	if (e->tri)
		return;		// allready connected by someone

	// find the point with the best angle
	p0 = trian->points[e->p0]->origin;
	p1 = trian->points[e->p1]->origin;
	best = 1.1;
	for (i=0 ; i< trian->numpoints ; i++)
	{
		p = trian->points[i]->origin;
		// a 0 dist will form a degenerate triangle
		if (DotProduct(p, e->normal) - e->dist < 0)
			continue;	// behind edge
		VectorSubtract (p0, p, v1);
		VectorSubtract (p1, p, v2);
		if (!VectorNormalize (v1,v1))
			continue;
		if (!VectorNormalize (v2,v2))
			continue;
		ang = DotProduct (v1, v2);
		if (ang < best)
		{
			best = ang;
			bestp = i;
		}
	}
	if (best >= 1)
		return;		// edge doesn't match anything

	// make a new triangle
	nt = AllocTriangle (trian);
	nt->edges[0] = e;
	nt->edges[1] = FindEdge (trian, e->p1, bestp);
	nt->edges[2] = FindEdge (trian, bestp, e->p0);
	for (i=0 ; i<3 ; i++)
		nt->edges[i]->tri = nt;
	TriEdge_r (trian, FindEdge (trian, bestp, e->p1));
	TriEdge_r (trian, FindEdge (trian, e->p0, bestp));
}

/*
============
TriangulatePoints
============
*/
void TriangulatePoints (triangulation_t *trian)
{
	vec_t	d, bestd;
	vec3_t	v1;
	int		bp1=0, bp2=0, i, j;
	vec_t	*p1, *p2;
	triedge_t	*e, *e2;

	if (trian->numpoints < 2)
		return;

	// find the two closest points
	bestd = 9999;
	for (i=0 ; i<trian->numpoints ; i++)
	{
		p1 = trian->points[i]->origin;
		for (j=i+1 ; j<trian->numpoints ; j++)
		{
			p2 = trian->points[j]->origin;
			VectorSubtract (p2, p1, v1);
			d = VectorLength (v1);
			if (d < bestd)
			{
				bestd = d;
				bp1 = i;
				bp2 = j;
			}
		}
	}

	e = FindEdge (trian, bp1, bp2);
	e2 = FindEdge (trian, bp2, bp1);
	TriEdge_r (trian, e);
	TriEdge_r (trian, e2);
}

/*
===============
AddPointToTriangulation
===============
*/
void AddPointToTriangulation (patch_t *patch, triangulation_t *trian)
{
	int			pnum;

	pnum = trian->numpoints;
	if (pnum == MAX_TRI_POINTS)
		Error ("trian->numpoints == MAX_TRI_POINTS");
	trian->points[pnum] = patch;
	trian->numpoints++;
}

/*
===============
LerpTriangle
===============
*/
void	LerpTriangle (triangulation_t *trian, triangle_t *t, vec3_t point, vec3_t color)
{
	patch_t		*p1, *p2, *p3;
	vec3_t		base, d1, d2;
	float		x, y, x1, y1;

	p1 = trian->points[t->edges[0]->p0];
	p2 = trian->points[t->edges[1]->p0];
	p3 = trian->points[t->edges[2]->p0];

	VectorCopy (p1->totallight, base);

	x1 = DotProduct (p3->origin, t->edges[0]->normal) - t->edges[0]->dist;
	y1 = DotProduct (p2->origin, t->edges[2]->normal) - t->edges[2]->dist;

	VectorCopy (base, color);

	if (fabs(x1)>=ON_EPSILON)
	{
		VectorSubtract (p3->totallight, base, d2);
		x = DotProduct (point, t->edges[0]->normal) - t->edges[0]->dist;
		x /= x1;
		VectorMA (color, x, d2, color);
	}
	if (fabs(y1)>=ON_EPSILON)
	{
		VectorSubtract (p2->totallight, base, d1);
		y = DotProduct (point, t->edges[2]->normal) - t->edges[2]->dist;
		y /= y1;
		VectorMA (color, y, d1, color);
	}
}

qboolean PointInTriangle (vec3_t point, triangle_t *t)
{
	int		i;
	triedge_t	*e;
	vec_t	d;

	for (i=0 ; i<3 ; i++)
	{
		e = t->edges[i];
		d = DotProduct (e->normal, point) - e->dist;
		if (d < 0)
			return false;	// not inside
	}

	return true;
}

/*
===============
SampleTriangulation
===============
*/
void SampleTriangulation (vec3_t point, triangulation_t *trian, triangle_t **last_valid, vec3_t color)
{
	triangle_t	*t;
	triedge_t	*e;
	vec_t		d, best;
	patch_t		*p0, *p1;
	vec3_t		v1, v2;
	int			i, j;

	if (trian->numpoints == 0)
	{
		VectorClear (color);
		return;
	}

	if (trian->numpoints == 1)
	{
		VectorCopy (trian->points[0]->totallight, color);
		return;
	}

	// try the last one
	if (*last_valid)
	{
		if (PointInTriangle (point, *last_valid))
		{
			LerpTriangle (trian, *last_valid, point, color);
			return;
		}
	}

	// search for triangles
	for (t = trian->tris, j=0 ; j < trian->numtris ; t++, j++)
	{
		if (t == *last_valid)
			continue;

		if (!PointInTriangle (point, t))
			continue;

		*last_valid = t;
		LerpTriangle (trian, t, point, color);
		return;
	}

	// search for exterior edge
	for (e=trian->edges, j=0 ; j< trian->numedges ; e++, j++)
	{
		if (e->tri)
			continue;		// not an exterior edge

		d = DotProduct (point, e->normal) - e->dist;
		if (d < 0)
			continue;	// not in front of edge

		p0 = trian->points[e->p0];
		p1 = trian->points[e->p1];

		VectorSubtract (p1->origin, p0->origin, v1);
		VectorNormalize (v1, v1);
		VectorSubtract (point, p0->origin, v2);
		d = DotProduct (v2, v1);
		if (d < 0)
			continue;
		if (d > 1)
			continue;
		for (i=0 ; i<3 ; i++)
			color[i] = p0->totallight[i] + d * (p1->totallight[i] - p0->totallight[i]);
		return;
	}

	// search for nearest point
	best = 99999;
	p1 = NULL;
	for (j=0 ; j<trian->numpoints ; j++)
	{
		p0 = trian->points[j];
		VectorSubtract (point, p0->origin, v1);
		d = VectorLength (v1);
		if (d < best)
		{
			best = d;
			p1 = p0;
		}
	}

	if (!p1)
		Error ("SampleTriangulation: no points");

	VectorCopy (p1->totallight, color);
}

/*
=================================================================

  LIGHTMAP SAMPLE GENERATION

=================================================================
*/


#define	SINGLEMAP	(64*64*4)

typedef struct
{
	vec_t	facedist;
	vec3_t	facenormal;

	int		numsurfpt;
	vec3_t	surfpt[SINGLEMAP];

	vec3_t	modelorg;		// for origined bmodels

	vec3_t	texorg;
	vec3_t	worldtotex[2];	// s = (world - texorg) . worldtotex[0]
	vec3_t	textoworld[2];	// world = texorg + s * textoworld[0]

	vec_t	exactmins[2], exactmaxs[2];

	int		texmins[2], texsize[2];
	int		surfnum;
	dface_t	*face;
} lightinfo_t;


/*
================
CalcFaceExtents

Fills in s->texmins[] and s->texsize[]
also sets exactmins[] and exactmaxs[]
================
*/
void CalcFaceExtents (lightinfo_t *l)
{
	dface_t *s;
	vec_t	mins[2], maxs[2], val;
	int		i,j, e;
	int 	facenum;
	dvertex_t	*v;
	texinfo_t	*tex;
	vec3_t		vt;

	s = l->face;
	facenum = s-dfaces;

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = &texinfo[s->texinfo];

	for (i=0 ; i<s->numedges ; i++)
	{
		e = dsurfedges[s->firstedge+i];
		if (e >= 0)
			v = dvertexes + dedges[e].v[0];
		else
			v = dvertexes + dedges[-e].v[1];

//		VectorAdd (v->point, l->modelorg, vt);
		VectorCopy (v->point, vt);

		for (j=0 ; j<2 ; j++)
		{
			val = DotProduct (vt, tex->vecs[j]) + tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		l->exactmins[i] = mins[i];
		l->exactmaxs[i] = maxs[i];

		mins[i] = floor((mins[i]*refine_amt)/16);
		maxs[i] = ceil((maxs[i]*refine_amt)/16);

		l->texmins[i] = mins[i];
		l->texsize[i] = maxs[i] - mins[i];
	}
	lfacelookups[facenum].width = l->texsize[0]+1;
	lfacelookups[facenum].height = l->texsize[1]+1;

	if (l->texsize[0] * l->texsize[1] > SINGLEMAP/4)	// div 4 for extrasamples
	{
		char s[3] = {'X', 'Y', 'Z'};

		for (i=0 ; i<2 ; i++)
		{
			printf("Axis: %c\n", s[i]);

			l->exactmins[i] = mins[i];
			l->exactmaxs[i] = maxs[i];

			mins[i] = floor((mins[i]*refine_amt)/16);
			maxs[i] = ceil((maxs[i]*refine_amt)/16);

			l->texmins[i] = mins[i];
			l->texsize[i] = maxs[i] - mins[i];

			printf("  Mins = %10.3f, Maxs = %10.3f,  Size = %10.3f\n", (double)mins[i], (double)maxs[i], (double)(maxs[i] - mins[i]));
		}

		Error ("Surface to large to map");
	}
}

/*
================
CalcFaceVectors

Fills in texorg, worldtotex. and textoworld
================
*/
void CalcFaceVectors (lightinfo_t *l)
{
	texinfo_t	*tex;
	int			i, j;
	vec3_t	texnormal;
	vec_t	distscale;
	vec_t	dist, len;
	int			w, h;

	tex = &texinfo[l->face->texinfo];

// convert from float to double
	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<3 ; j++)
			l->worldtotex[i][j] = tex->vecs[i][j];

// calculate a normal to the texture axis.  points can be moved along this
// without changing their S/T
	texnormal[0] = tex->vecs[1][1]*tex->vecs[0][2]
		- tex->vecs[1][2]*tex->vecs[0][1];
	texnormal[1] = tex->vecs[1][2]*tex->vecs[0][0]
		- tex->vecs[1][0]*tex->vecs[0][2];
	texnormal[2] = tex->vecs[1][0]*tex->vecs[0][1]
		- tex->vecs[1][1]*tex->vecs[0][0];
	VectorNormalize (texnormal, texnormal);

// flip it towards plane normal
	distscale = DotProduct (texnormal, l->facenormal);
	if (!distscale)
	{
		qprintf ("WARNING: Texture axis perpendicular to face\n");
		distscale = 1;
	}
	if (distscale < 0)
	{
		distscale = -distscale;
		VectorSubtract (vec3_origin, texnormal, texnormal);
	}

// distscale is the ratio of the distance along the texture normal to
// the distance along the plane normal
	distscale = 1/distscale;

	for (i=0 ; i<2 ; i++)
	{
		len = VectorLength (l->worldtotex[i]);
		dist = DotProduct (l->worldtotex[i], l->facenormal);
		dist *= distscale;
		VectorMA (l->worldtotex[i], -dist, texnormal, l->textoworld[i]);
		VectorScale (l->textoworld[i], (1/len)*(1/len), l->textoworld[i]);
	}


// calculate texorg on the texture plane
	for (i=0 ; i<3 ; i++)
		l->texorg[i] = -tex->vecs[0][3]* l->textoworld[0][i] - tex->vecs[1][3] * l->textoworld[1][i];

// project back to the face plane
	dist = DotProduct (l->texorg, l->facenormal) - l->facedist - 1;
	dist *= distscale;
	VectorMA (l->texorg, -dist, texnormal, l->texorg);

	// compensate for org'd bmodels
	VectorAdd (l->texorg, l->modelorg, l->texorg);

	// total sample count
	h = l->texsize[1]+1;
	w = l->texsize[0]+1;
	l->numsurfpt = w * h;
}

/*
=================
CalcPoints

For each texture aligned grid point, back project onto the plane
to get the world xyz value of the sample point
=================
*/
void CalcPoints (lightinfo_t *l, float sofs, float tofs)
{
	int		i;
	int		s, t, j;
	int		w, h, step;
	vec_t	starts, startt, us, ut;
	vec_t	*surf;
	vec_t	mids, midt;
	vec3_t	facemid;
	dleaf_t	*leaf;

	surf = l->surfpt[0];
	mids = (l->exactmaxs[0] + l->exactmins[0])/2;
	midt = (l->exactmaxs[1] + l->exactmins[1])/2;

	for (j=0 ; j<3 ; j++)
		facemid[j] = l->texorg[j] + l->textoworld[0][j]*mids + l->textoworld[1][j]*midt;

	h = l->texsize[1]+1;
	w = l->texsize[0]+1;
	l->numsurfpt = w * h;

	step = 16/refine_amt;
	starts = l->texmins[0]*step;
	startt = l->texmins[1]*step;


	for (t=0 ; t<h ; t++)
	{
		for (s=0 ; s<w ; s++, surf+=3)
		{
			us = starts + (s+sofs)*step;
			ut = startt + (t+tofs)*step;


		// if a line can be traced from surf to facemid, the point is good
			for (i=0 ; i<6 ; i++)
			{
			// calculate texture point
				for (j=0 ; j<3 ; j++)
					surf[j] = l->texorg[j] + l->textoworld[0][j]*us
					+ l->textoworld[1][j]*ut;

				leaf = PointInLeaf (surf);
				if (leaf->contents != CONTENTS_SOLID)
				{
					if (!TestLine_r (0, facemid, surf))
						break;	// got it
				}

				// nudge it
				if (i & 1)
				{
					if (us > mids)
					{
						us -= 8;
						if (us < mids)
							us = mids;
					}
					else
					{
						us += 8;
						if (us > mids)
							us = mids;
					}
				}
				else
				{
					if (ut > midt)
					{
						ut -= 8;
						if (ut < midt)
							ut = midt;
					}
					else
					{
						ut += 8;
						if (ut > midt)
							ut = midt;
					}
				}
			}
		}
	}

}


//==============================================================



#define	MAX_STYLES	32
typedef struct
{
	int			numsamples;
	float		*origins;
	int			numstyles;
	int			stylenums[MAX_STYLES];
	float		*samples[MAX_STYLES];
} facelight_t;

directlight_t	*directlights[MAX_MAP_LEAFS];
facelight_t		facelight[MAX_MAP_FACES];
int				numdlights;

/*
==================
FindTargetEntity
==================
*/
entity_t *FindTargetEntity (char *target)
{
	int		i;
	char	*n;

	for (i=0 ; i<num_entities ; i++)
	{
		n = ValueForKey (&entities[i], "targetname");
		if (!strcmp (n, target))
			return &entities[i];
	}

	return NULL;
}

//#define	DIRECT_LIGHT	3000
#define	DIRECT_LIGHT	3

/*
=============
CreateDirectLights
=============
*/
void CreateDirectLights (void)
{
	int		i;
	patch_t	*p;
	directlight_t	*dl;
	dleaf_t	*leaf;
	int		cluster;
	entity_t	*e, *e2;
	char	*name;
	char	*target;
	float	angle;
	vec3_t	dest;
	char	*_color;
	float	intensity;
    char    *sun_target = NULL;
    char    *proc_num;
    qboolean sun_light;

	//
	// entities
	//
	for (i=0 ; i<num_entities ; i++)
	{
		e = &entities[i];
		name = ValueForKey (e, "classname");
		if (strncmp (name, "light", 5))
		{
            if (!strncmp (name, "worldspawn", 10))
			{
                sun_target = ValueForKey(e, "_sun");
                if(strlen(sun_target) > 0)
				{
                    printf("Sun activated.\n");
                    sun = true;
				}

                proc_num = ValueForKey(e, "_sun_ambient");
                if(strlen(proc_num) > 0)
				{
                    sun_ambient = atof(proc_num);
				}

                proc_num = ValueForKey(e, "_sun_light");
                if(strlen(proc_num) > 0)
				{
                    sun_main = atof(proc_num);
				}

                proc_num = ValueForKey(e, "_sun_color");
                if(strlen(proc_num) > 0)
				{
					GetVectorForKey (e, "_sun_color", sun_color);

                    sun_alt_color = true;
					ColorNormalize (sun_color, sun_color);
				}
			}

            continue;
		}

        sun_light = false;

        target = ValueForKey (e, "target");

        if(strlen(target) >= 1 && !strcmp(target, sun_target))
		{
            vec3_t sun_s, sun_t;

            GetVectorForKey(e, "origin", sun_s);

            sun_light = true;

			e2 = FindTargetEntity (target);

			if (!e2)
			{
				printf ("WARNING: sun missing target, 0,0,0 used\n");

                sun_t[0] = 0;
                sun_t[1] = 0;
                sun_t[2] = 0;
			}
			else
			{
				GetVectorForKey (e2, "origin", sun_t);
			}

			VectorSubtract (sun_s, sun_t, sun_pos);
			VectorNormalize (sun_pos, sun_pos);

			continue;
		}

		numdlights++;
		dl = malloc(sizeof(directlight_t));
		memset (dl, 0, sizeof(*dl));

		GetVectorForKey (e, "origin", dl->origin);
		dl->style = FloatForKey (e, "_style");
		if (!dl->style)
			dl->style = FloatForKey (e, "style");
		if (dl->style < 0 || dl->style >= MAX_LSTYLES)
			dl->style = 0;

		leaf = PointInLeaf (dl->origin);
		cluster = leaf->cluster;

		dl->next = directlights[cluster];
		directlights[cluster] = dl;

        proc_num = ValueForKey(e, "_wait");
        if(strlen(proc_num) > 0)
            dl->wait = atof(proc_num);
        else
		{
            proc_num = ValueForKey(e, "wait");

            if(strlen(proc_num) > 0)
                dl->wait = atof(proc_num);
            else
                dl->wait = 1.0f;
		}

		if (dl->wait <= 0.001)
			dl->wait = 1.0f;

		proc_num = ValueForKey(e, "_angwait");
        if(strlen(proc_num) > 0)
            dl->adjangle = atof(proc_num);
        else
            dl->adjangle = 1.0f;

        intensity = FloatForKey (e, "light");
		if (!intensity)
			intensity = FloatForKey (e, "_light");
		if (!intensity)
			intensity = 300;

		_color = ValueForKey (e, "_color");
		if (_color && _color[0])
		{
			sscanf (_color, "%f %f %f", &dl->color[0],&dl->color[1],&dl->color[2]);
			ColorNormalize (dl->color, dl->color);
		}
		else
			dl->color[0] = dl->color[1] = dl->color[2] = 1.0;

		dl->intensity = intensity * entity_scale;
		dl->type = emit_point;

		target = ValueForKey (e, "target");

		if (!strcmp (name, "light_spot") || target[0])
		{
			dl->type = emit_spotlight;
			dl->stopdot = FloatForKey (e, "_cone");
			if (!dl->stopdot)
				dl->stopdot = 10;
			dl->stopdot = cos(dl->stopdot/180*3.14159);
			if (target[0])
			{	// point towards target
				e2 = FindTargetEntity (target);
				if (!e2)
					printf ("WARNING: light at (%i %i %i) has missing target\n",
					(int)dl->origin[0], (int)dl->origin[1], (int)dl->origin[2]);
				else
				{
					GetVectorForKey (e2, "origin", dest);
					VectorSubtract (dest, dl->origin, dl->normal);
					VectorNormalize (dl->normal, dl->normal);
				}
			}
			else
			{	// point down angle
				angle = FloatForKey (e, "angle");
				if (angle == ANGLE_UP)
				{
					dl->normal[0] = dl->normal[1] = 0;
					dl->normal[2] = 1;
				}
				else if (angle == ANGLE_DOWN)
				{
					dl->normal[0] = dl->normal[1] = 0;
					dl->normal[2] = -1;
				}
				else
				{
					dl->normal[2] = 0;
					dl->normal[0] = cos (angle/180*3.14159);
					dl->normal[1] = sin (angle/180*3.14159);
				}
			}
		}
	}


	//
	// surfaces
	//
	for (i=0, p=patches ; i<num_patches ; i++, p++)
	{
		if ((!sun || !p->sky) && p->totallight[0] < DIRECT_LIGHT
			&& p->totallight[1] < DIRECT_LIGHT
			&& p->totallight[2] < DIRECT_LIGHT)
			continue;

		numdlights++;
		dl = malloc(sizeof(directlight_t));
		memset (dl, 0, sizeof(*dl));

		VectorCopy (p->origin, dl->origin);

		leaf = PointInLeaf (dl->origin);
		cluster = leaf->cluster;
		dl->next = directlights[cluster];
		directlights[cluster] = dl;

        VectorCopy (p->plane->normal, dl->normal);

		if(sun && p->sky)
		{
            dl->leaf = leaf;
            dl->plane = p->plane;
            dl->type = emit_sky;
            dl->intensity = 1.0f;
		}
		else
		{
            dl->type = emit_surface;
			dl->intensity = ColorNormalize (p->totallight, dl->color);
			dl->intensity *= p->area * direct_scale;
		}

		VectorClear (p->totallight);	// all sent now
	}


	qprintf ("%i direct lights\n", numdlights);
}

/*
 * Mar. 2009
 * Lighting equations for distance attenuation have been changed to
 * (what appears to be) the form of a standard lighting equation.
 * Coefficients need to be set empirically based on artistic criteria.
 * Helps to plot the curve. Real-world-correct inverse square law is to
 * extreme.
 */

/*
 * PointLight()
 *
 *  note: point lights are affected by -entity (entity_scale)
 */
float PointLight(float dot1, float intensity, float dist)
{
	float scale = 0.0f;

	if( dist < 32.0f ) // gets plenty bright at this distance
		dist = 32.0f;
	scale = 1.0f / ( 0.8f + ( 0.01f * dist ) + ( 0.0001f * dist * dist ) );
	scale *= intensity * dot1;

	return scale;
}

/*
 * SurfaceLight()
 *
 * note: surface lights are affected by -direct (direct_scale)
 *
 */
float SurfaceLight(float dot1, float intensity, float dist, float dot2, float invdist )
{
	float scale = 0.0f;

	if( dot2 > 0.001f )
	{ // point is not behind the surface light
		scale = 0.001f / ( 0.8f + ( 0.01f * dist ) + ( 0.0001f * dist * dist ) );
		scale *= intensity * dot1 * dot2;
	}

	return scale;
}

/*
 * SpotLight()
 *
 * note: spot lights are affected by -entity (entity_scale)
 */
float SpotLight( float dot1, float intensity, float dist, float dot2, float stopdot )
{
	float scale = 0.0f;

	if( dot2 >= stopdot )
	{ // surface point is within the spotlight cone
		// light to surface distance attenuation
		scale = 4.0f / ( 0.8f + ( 0.01f * dist ) + ( 0.0001f * dist * dist ) );
		// surface normal attenuation
		scale *= dot1;
		// spot center to surface point attenuation
		scale *= powf( dot2, 50.0f ); // dot2 range is limited, so exponent is big.
		// this term is not really necessary, could have spots with sharp cutoff
		//   and for fuzzy edges use multiple lights with different cones.

		scale *= intensity;
	}

	return scale;
}

/*
=============
LightContributionToPoint
=============
*/
void LightContributionToPoint	(	directlight_t *l, vec3_t pos, 
									vec3_t normal, vec3_t color, 
									float lightscale2, 
									qboolean *sun_main_once, 
									qboolean *sun_ambient_once
								)
{
	vec3_t			delta, target;
	float			dot, dot2;
	float			dist;
	float			scale = 0.0f;
	float			inv;
    float           main_val;
	qboolean		set_main;

	VectorClear (color);
	VectorSubtract (l->origin, pos, delta);
	dist = DotProduct (delta, delta);

	if (dist == 0)
		return;

	dist = sqrt (dist);
	inv = 1.0f / dist;
	delta[0] *= inv;
	delta[1] *= inv;
	delta[2] *= inv;

	dot = DotProduct (delta, normal);
	if (dot <= 0.001)
		return;		// behind sample surface

	if (!noblock && TestLine_r (0, pos, l->origin))
		return;		// occluded

	if( l->type == emit_sky )
	{  // this might be the sun ambient and it might be directional
		set_main = false;
		if( *sun_main_once ) // don't do -extra multisampling on sun
			return;

		dot2 = -DotProduct (delta, l->normal);
		if( dot2 <= 0.001f )
			return; // behind light surface

		if( !*sun_ambient_once ) // Ambient sky, no -extra multisampling
			scale = sun_ambient;
		else
			scale = 0.0f;

		// Main sky
		dot2 = DotProduct (sun_pos, normal); // sun_pos from target entity
		if( dot2 > 0.001f ) // Main sky
		{
			set_main = true;
			main_val = sun_main * dot2;
			if ( !noblock )
			{
				if( !RayPlaneIntersect(
					l->plane->normal, l->plane->dist, pos, sun_pos, target )
					||
					TestLine_r( 0, pos, target )
				)
				{
					set_main = *sun_main_once;
					main_val = 0.0f;
				}
				else
				{
					scale += main_val;
					main_val = 0.0f; // done with it
				}
			}
		}
		else
		{
			if( *sun_ambient_once )
				return;
			set_main = false;
			main_val = 0.0f;
		}
		if( sun_alt_color ) // set in .map
			VectorScale ( sun_color, scale, color );
		else
			VectorScale ( l->color, scale, color );

		*sun_ambient_once = true;
		*sun_main_once = set_main;
	}
	else
	{
		switch ( l->type )
		{
		case emit_point:
			scale = PointLight( dot, l->intensity, dist );
			break;

		case emit_surface:
			dot2 = -DotProduct (delta, l->normal);
			scale = SurfaceLight( dot, l->intensity, dist, dot2, inv );
			break;

		case emit_spotlight:
			dot2 = -DotProduct(delta, l->normal);
			scale = SpotLight( dot, l->intensity, dist, dot2, l->stopdot );
			break;

		default:
			Error("Invalid light entity type.\n" );
			break;
		} /* switch() */

		if ( scale > 0.0f )
		{
			scale *= lightscale2; // adjust for multisamples, -extra cmd line arg
			VectorScale ( l->color, scale, color );
		}
	}
}

/*
=============
GatherSampleLight

Lightscale2 is the normalizer for multisampling, -extra cmd line arg
=============
*/

void GatherSampleLight (vec3_t pos, vec3_t normal,
			float **styletable, int offset, int mapsize, float lightscale2,
			qboolean *sun_main_once, qboolean *sun_ambient_once)
{
	int				i;
	directlight_t	*l;
	byte			pvs[(MAX_MAP_LEAFS+7)/8];
	float			*dest;
	vec3_t			color;

	// get the PVS for the pos to limit the number of checks
	if (!PvsForOrigin (pos, pvs))
	{
		return;
	}

	for (i = 0 ; i<dvis->numclusters ; i++)
	{
		if ( ! (pvs[ i>>3] & (1<<(i&7))) )
			continue;

		for (l=directlights[i] ; l ; l=l->next)
		{
			LightContributionToPoint ( l, pos, normal, color, lightscale2, sun_main_once, sun_ambient_once);

			// no contribution
			if ( VectorCompare ( color, vec3_origin ) )
				continue;

			// if this style doesn't have a table yet, allocate one
			if (!styletable[l->style])
			{
				styletable[l->style] = malloc (mapsize);
				memset (styletable[l->style], 0, mapsize);
			}

			dest = styletable[l->style] + offset;
			dest[0] += color[0];
			dest[1] += color[1];
			dest[2] += color[2];
		}
	}

}


/*
=============
AddSampleToPatch

Take the sample's collected light and
add it back into the apropriate patch
for the radiosity pass.

The sample is added to all patches that might include
any part of it.  They are counted and averaged, so it
doesn't generate extra light.
=============
*/
void AddSampleToPatch (vec3_t pos, vec3_t color, int facenum)
{
	patch_t	*patch;
	vec3_t	mins, maxs;
	int		i;

	if (numbounce == 0)
		return;
	if (color[0] + color[1] + color[2] < 3)
		return;

	for (patch = face_patches[facenum] ; patch ; patch=patch->next)
	{
		// see if the point is in this patch (roughly)
		WindingBounds (patch->winding, mins, maxs);
		for (i=0 ; i<3 ; i++)
		{
			if (mins[i] > pos[i] + 16/refine_amt)
				goto nextpatch;
			if (maxs[i] < pos[i] - 16/refine_amt)
				goto nextpatch;
		}

		// add the sample to the patch
		patch->samples++;
		VectorAdd (patch->samplelight, color, patch->samplelight);
nextpatch:;
	}

}


/*
=============
BuildFacelights
=============
*/
float	sampleofs[5][2] =
{  {0,0}, {-0.25, -0.25}, {0.25, -0.25}, {0.25, 0.25}, {-0.25, 0.25} };


void BuildFacelights (int facenum)
{
	dface_t	*this_face;
	lightinfo_t	liteinfo[5];
	float		*styletable[MAX_LSTYLES];
	int			i, j;
	float		*spot;
	patch_t		*patch;
	int			numsamples;
	int			tablesize;
	facelight_t		*fl;
	qboolean	sun_main_once, sun_ambient_once;

	this_face = &dfaces[facenum];

	if ( texinfo[this_face->texinfo].flags & (SURF_WARP|SURF_SKY) )
		return;		// non-lit texture

	memset (styletable,0, sizeof(styletable));

	if (extrasamples) // set with -extra option
		numsamples = 5;
	else
		numsamples = 1;
	for (i=0 ; i<numsamples ; i++)
	{
		memset (&liteinfo[i], 0, sizeof(liteinfo[i]));
		liteinfo[i].surfnum = facenum;
		liteinfo[i].face = this_face;
		VectorCopy (dplanes[this_face->planenum].normal, liteinfo[i].facenormal);
		liteinfo[i].facedist = dplanes[this_face->planenum].dist;
		if (this_face->side)
		{
			VectorSubtract (vec3_origin, liteinfo[i].facenormal, liteinfo[i].facenormal);
			liteinfo[i].facedist = -liteinfo[i].facedist;
		}

		// get the origin offset for rotating bmodels
		VectorCopy (face_offset[facenum], liteinfo[i].modelorg);

		CalcFaceVectors (&liteinfo[i]);
		CalcFaceExtents (&liteinfo[i]);
		CalcPoints (&liteinfo[i], sampleofs[i][0], sampleofs[i][1]);
	}

	tablesize = liteinfo[0].numsurfpt * sizeof(vec3_t);
	styletable[0] = malloc(tablesize);
	memset (styletable[0], 0, tablesize);

	fl = &facelight[facenum];
	fl->numsamples = liteinfo[0].numsurfpt;
	fl->origins = malloc (tablesize);
	memcpy (fl->origins, liteinfo[0].surfpt, tablesize);

	for (i=0 ; i<liteinfo[0].numsurfpt ; i++)
	{
        sun_ambient_once = false;
        sun_main_once = false;


		for (j=0 ; j<numsamples ; j++)
		{
			GatherSampleLight (liteinfo[j].surfpt[i], liteinfo[0].facenormal, styletable,
				i*3, tablesize, 1.0/numsamples, &sun_main_once, &sun_ambient_once);
		}

		// contribute the sample to one or more patches
		AddSampleToPatch (liteinfo[0].surfpt[i], styletable[0]+i*3, facenum);
	}

	// average up the direct light on each patch for radiosity
	for (patch = face_patches[facenum] ; patch ; patch=patch->next)
	{
		if (patch->samples)
		{
			VectorScale (patch->samplelight, 1.0/patch->samples, patch->samplelight);
		}
//		else
//		{
//			printf ("patch with no samples\n");
//		}
	}

	for (i=0 ; i<MAX_LSTYLES ; i++)
	{
		if (!styletable[i])
			continue;
		if (fl->numstyles == MAX_STYLES)
			break;
		fl->samples[fl->numstyles] = styletable[i];
		fl->stylenums[fl->numstyles] = i;
		fl->numstyles++;
	}

	// the light from DIRECT_LIGHTS is sent out, but the
	// texture itself should still be full bright
	if (face_patches[facenum]->baselight[0] >= DIRECT_LIGHT ||
		face_patches[facenum]->baselight[1] >= DIRECT_LIGHT ||
		face_patches[facenum]->baselight[2] >= DIRECT_LIGHT
		)
	{
		spot = fl->samples[0];
		for (i=0 ; i<liteinfo[0].numsurfpt ; i++, spot+=3)
		{
			VectorAdd (spot, face_patches[facenum]->baselight, spot);
		}
	}
}


/*
=============
FinalLightFace

Add the indirect lighting on top of the direct
lighting and save into final map format
=============
*/
void FinalLightFace (int facenum)
{
	dface_t		*f;
	int			i, j, /*k,*/ st;
	vec3_t		lb;
	patch_t		*patch;
	triangulation_t	*trian = NULL;
	facelight_t	*fl;
	// float		minlight;
	float		max;
	float newmax;
	byte		*dest;
	triangle_t	*last_valid;
	int			pfacenum;
	vec3_t		facemins, facemaxs;

	f = &dfaces[facenum];
	fl = &facelight[facenum];

	if ( texinfo[f->texinfo].flags & (SURF_WARP|SURF_SKY) )
		return;		// non-lit texture

	ThreadLock ();
	f->lightofs = lightdatasize;
	lightdatasize += fl->numstyles*(fl->numsamples*3);

	if (refine_amt == 0 && lightdatasize > MAX_MAP_LIGHTING)
		Error ("MAX_MAP_LIGHTING");
	if (refine_amt > 1 && lightdatasize > MAX_OVERRIDE_LIGHTING)
		Error ("MAX_OVERRIDE_LIGHTING");
	ThreadUnlock ();

	f->styles[0] = 0;
	f->styles[1] = f->styles[2] = f->styles[3] = 0xff;

	//
	// set up the triangulation
	//
	if (numbounce > 0)
	{
		ClearBounds (facemins, facemaxs);
		for (i=0 ; i<f->numedges ; i++)
		{
			int		ednum;

			ednum = dsurfedges[f->firstedge+i];
			if (ednum >= 0)
				AddPointToBounds (dvertexes[dedges[ednum].v[0]].point,
				facemins, facemaxs);
			else
				AddPointToBounds (dvertexes[dedges[-ednum].v[1]].point,
				facemins, facemaxs);
		}

		trian = AllocTriangulation (&dplanes[f->planenum]);

		// for all faces on the plane, add the nearby patches
		// to the triangulation
		for (pfacenum = planelinks[f->side][f->planenum]
			; pfacenum ; pfacenum = facelinks[pfacenum])
		{
			for (patch = face_patches[pfacenum] ; patch ; patch=patch->next)
			{
				for (i=0 ; i < 3 ; i++)
				{
					if (facemins[i] - patch->origin[i] > subdiv*2)
						break;
					if (patch->origin[i] - facemaxs[i] > subdiv*2)
						break;
				}
				if (i != 3)
					continue;	// not needed for this face
				AddPointToTriangulation (patch, trian);
			}
		}
		for (i=0 ; i<trian->numpoints ; i++)
			memset (trian->edgematrix[i], 0, trian->numpoints*sizeof(trian->edgematrix[0][0]) );
		TriangulatePoints (trian);
	}

	//
	// sample the triangulation
	//


	// _minlight allows models that have faces that would not be
	// illuminated to receive a mottled light pattern instead of
	// black
/*
	// 2010-09 - probably not used, too crude
 	minlight = FloatForKey (face_entity[facenum], "_minlight") * 128;
*/
	dest = &dlightdata[f->lightofs];

	if (fl->numstyles > MAXLIGHTMAPS)
	{
		fl->numstyles = MAXLIGHTMAPS;
		printf ("face with too many lightstyles: (%f %f %f)\n",
			face_patches[facenum]->origin[0],
			face_patches[facenum]->origin[1],
			face_patches[facenum]->origin[2]
			);
	}

//	qprintf("Lightmap output: Styles: %d : Samples/Style: %d\n",
//			fl->numstyles, fl->numsamples );

	for (st=0 ; st<fl->numstyles ; st++)
	{
		last_valid = NULL;
		f->styles[st] = fl->stylenums[st];

		for (j=0 ; j<fl->numsamples ; j++)
		{
			VectorCopy ( (fl->samples[st]+j*3), lb);
			if (numbounce > 0 && st == 0)
			{
				vec3_t	add;

				SampleTriangulation (fl->origins + j*3, trian, &last_valid, add);
				VectorAdd (lb, add, lb);
			}


			/*
			 * to allow experimenting, ambient and lightscale are not limited
			 *  to reasonable ranges.
			 */
			if ( ambient >= -255.0f && ambient <= 255.0f )
			{ // add fixed white ambient.
				lb[0] += ambient;
				lb[1] += ambient;
				lb[2] += ambient;
			}
			if ( lightscale > 0.0f  )
			{ // apply lightscale, scale down or up
				lb[0] *= lightscale;
				lb[1] *= lightscale;
				lb[2] *= lightscale;
			}
			// negative values not allowed
			lb[0] = (lb[0] < 0.0f) ? 0.0f : lb[0];
			lb[1] = (lb[1] < 0.0f) ? 0.0f : lb[1];
			lb[2] = (lb[2] < 0.0f) ? 0.0f : lb[2];


/*			qprintf("{%f %f %f}:",lb[0],lb[1],lb[2]);*/

			// determine max of R,G,B
			max = lb[0] > lb[1] ? lb[0] : lb[1];
			max = max > lb[2] ? max : lb[2];

			if( grayscale > 0.0f && grayscale <= 1.0f  )
            { // reduce color per NTSC model
                max = (0.299f * lb[0]) + (0.587f * lb[1]) * (0.144f * lb[2]);
                lb[0] = (lb[0] * (1.0f - grayscale)) + (max * grayscale);
                lb[1] = (lb[1] * (1.0f - grayscale)) + (max * grayscale);
                lb[2] = (lb[2] * (1.0f - grayscale)) + (max * grayscale);
            }
			if ( max < 1.0f )
				max = 1.0f;

			// note that maxlight based scaling is per-sample based on
			//  highest value of R, G, and B
			// adjust for -maxlight option
			newmax = max;
			if ( max > maxlight ) {
				newmax = maxlight;
				newmax /= max; // scaling factor 0.0..1.0
				// scale into 0.0..maxlight range
				lb[0] *= newmax;
				lb[1] *= newmax;
				lb[2] *= newmax;
			}

/*
			qprintf("{%x %x %x}\n",
					(byte)(lb[0]+0.5), (byte)(lb[1]+0.5), (byte)(lb[2]+0.5) );
*/
			// and output to 8:8:8 RGB
			*dest++ = (byte)(lb[0] + 0.5);
			*dest++ = (byte)(lb[1] + 0.5);
			*dest++ = (byte)(lb[2] + 0.5);
		}
	}

	if (numbounce > 0)
		FreeTriangulation (trian);
}
