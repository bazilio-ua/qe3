
#define MAX_POINTS_ON_WINDING 64

//returns true if the planes are equal
int			Plane_Equal(plane_t *a, plane_t *b, int flip);
//returns false if the points are colinear
int			Plane_FromPoints(vec3_t p1, vec3_t p2, vec3_t p3, plane_t *plane);
//returns true if the points are equal
int			Point_Equal(vec3_t p1, vec3_t p2, float epsilon);

//allocate a winding
winding_t*	Winding_Alloc(int points);
//free the winding
void		Winding_Free(winding_t *w);
//create a base winding for the plane
winding_t*	Winding_BaseForPlane (plane_t *p);
//make a winding clone
winding_t*	Winding_Clone(winding_t *w );
//remove a point from the winding
void		Winding_RemovePoint(winding_t *w, int point);
//returns true if the planes are concave
int			Winding_PlanesConcave(winding_t *w1, winding_t *w2,
									 vec3_t normal1, vec3_t normal2,
									 float dist1, float dist2);
//clip the winding with the plane
winding_t*	Winding_Clip(winding_t *in, plane_t *split, qboolean keepon);
//try to merge the windings, returns the new merged winding or NULL
winding_t *Winding_TryMerge(winding_t *f1, winding_t *f2, vec3_t planenormal, int keep);
