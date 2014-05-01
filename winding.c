

#include "qe3.h"


/*
=============
Plane_Equal
=============
*/
#define	NORMAL_EPSILON	0.0001
#define	DIST_EPSILON	0.02

int Plane_Equal(plane_t *a, plane_t *b, int flip)
{
	vec3_t normal;
	float dist;

	if (flip) {
		normal[0] = - b->normal[0];
		normal[1] = - b->normal[1];
		normal[2] = - b->normal[2];
		dist = - b->dist;
	}
	else {
		normal[0] = b->normal[0];
		normal[1] = b->normal[1];
		normal[2] = b->normal[2];
		dist = b->dist;
	}
	if (
	   fabs(a->normal[0] - normal[0]) < NORMAL_EPSILON
	&& fabs(a->normal[1] - normal[1]) < NORMAL_EPSILON
	&& fabs(a->normal[2] - normal[2]) < NORMAL_EPSILON
	&& fabs(a->dist - dist) < DIST_EPSILON )
		return true;
	return false;
}

/*
============
Plane_FromPoints
============
*/
int Plane_FromPoints(vec3_t p1, vec3_t p2, vec3_t p3, plane_t *plane)
{
	vec3_t v1, v2;

	VectorSubtract(p2, p1, v1);
	VectorSubtract(p3, p1, v2);
	//CrossProduct(v2, v1, plane->normal);
	CrossProduct(v1, v2, plane->normal);
	if (VectorNormalize(plane->normal) < 0.1) return false;
	plane->dist = DotProduct(p1, plane->normal);
	return true;
}

/*
=================
Point_Equal
=================
*/
int Point_Equal(vec3_t p1, vec3_t p2, float epsilon)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		if (fabs(p1[i] - p2[i]) > epsilon) return false;
	}
	return true;
}

/*
==================
Winding_Alloc
==================
*/
#define MAX_POINTS_ON_WINDING 64
winding_t *Winding_Alloc (int points)
{
	winding_t	*w;
	int			size;
	
	if (points > MAX_POINTS_ON_WINDING)
		Error ("Winding_Alloc: %i points", points);
	
	size = (int)((winding_t *)0)->points[points];
	w = (winding_t*) malloc (size);
	memset (w, 0, size);
	w->maxpoints = points;
	
	return w;
}


void Winding_Free (winding_t *w)
{
	free(w);
}

/*
==============
Winding_RemovePoint
==============
*/
void Winding_RemovePoint(winding_t *w, int point)
{
	if (point < 0 || point >= w->numpoints)
		Error("Winding_RemovePoint: point out of range");

	if (point < w->numpoints-1)
	{
		memmove(&w->points[point], &w->points[point+1], (int)((winding_t *)0)->points[w->numpoints - point - 1]);
	}
	w->numpoints--;
}

/*
=============
Winding_PlanesConcave
=============
*/
#define WCONVEX_EPSILON		0.2

int Winding_PlanesConcave(winding_t *w1, winding_t *w2,
							 vec3_t normal1, vec3_t normal2,
							 float dist1, float dist2)
{
	int i;

	if (!w1 || !w2) return false;

	// check if one of the points of winding 1 is at the back of the plane of winding 2
	for (i = 0; i < w1->numpoints; i++)
	{
		if (DotProduct(normal2, w1->points[i]) - dist2 > WCONVEX_EPSILON) return true;
	}
	// check if one of the points of winding 2 is at the back of the plane of winding 1
	for (i = 0; i < w2->numpoints; i++)
	{
		if (DotProduct(normal1, w2->points[i]) - dist1 > WCONVEX_EPSILON) return true;
	}

	return false;
}

/*
=============
Winding_TryMerge

If two windings share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the windings couldn't be merged, or the new winding.
The originals will NOT be freed.

if keep is true no points are ever removed
=============
*/
#define	CONTINUOUS_EPSILON	0.005

winding_t *Winding_TryMerge(winding_t *f1, winding_t *f2, vec3_t planenormal, int keep)
{
	vec_t		*p1, *p2, *p3, *p4, *back;
	winding_t	*newf;
	int			i, j, k, l;
	vec3_t		normal, delta;
	vec_t		dot;
	qboolean	keep1, keep2;
	

	//
	// find a common edge
	//	
	p1 = p2 = NULL;	// stop compiler warning
	j = 0;			// 
	
	for (i = 0; i < f1->numpoints; i++)
	{
		p1 = f1->points[i];
		p2 = f1->points[(i+1) % f1->numpoints];
		for (j = 0; j < f2->numpoints; j++)
		{
			p3 = f2->points[j];
			p4 = f2->points[(j+1) % f2->numpoints];
			for (k = 0; k < 3; k++)
			{
				if (fabs(p1[k] - p4[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
				if (fabs(p2[k] - p3[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
			} //end for
			if (k==3)
				break;
		} //end for
		if (j < f2->numpoints)
			break;
	} //end for
	
	if (i == f1->numpoints)
		return NULL;			// no matching edges

	//
	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	//
	back = f1->points[(i+f1->numpoints-1)%f1->numpoints];
	VectorSubtract (p1, back, delta);
	CrossProduct (planenormal, delta, normal);
	VectorNormalize (normal);
	
	back = f2->points[(j+2)%f2->numpoints];
	VectorSubtract (back, p1, delta);
	dot = DotProduct (delta, normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep1 = (qboolean)(dot < -CONTINUOUS_EPSILON);
	
	back = f1->points[(i+2)%f1->numpoints];
	VectorSubtract (back, p2, delta);
	CrossProduct (planenormal, delta, normal);
	VectorNormalize (normal);

	back = f2->points[(j+f2->numpoints-1)%f2->numpoints];
	VectorSubtract (back, p2, delta);
	dot = DotProduct (delta, normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep2 = (qboolean)(dot < -CONTINUOUS_EPSILON);

	//
	// build the new polygon
	//
	newf = Winding_Alloc (f1->numpoints + f2->numpoints);
	
	// copy first polygon
	for (k=(i+1)%f1->numpoints ; k != i ; k=(k+1)%f1->numpoints)
	{
		if (!keep && k==(i+1)%f1->numpoints && !keep2)
			continue;
		
		VectorCopy (f1->points[k], newf->points[newf->numpoints]);
		newf->numpoints++;
	}
	
	// copy second polygon
	for (l= (j+1)%f2->numpoints ; l != j ; l=(l+1)%f2->numpoints)
	{
		if (!keep && l==(j+1)%f2->numpoints && !keep1)
			continue;
		VectorCopy (f2->points[l], newf->points[newf->numpoints]);
		newf->numpoints++;
	}

	return newf;
}
