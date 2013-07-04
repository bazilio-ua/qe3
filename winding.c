

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
