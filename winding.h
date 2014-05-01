

//returns true if the planes are equal
int			Plane_Equal(plane_t *a, plane_t *b, int flip);
//returns false if the points are colinear
int			Plane_FromPoints(vec3_t p1, vec3_t p2, vec3_t p3, plane_t *plane);
//returns true if the points are equal
int			Point_Equal(vec3_t p1, vec3_t p2, float epsilon);

//remove a point from the winding
void		Winding_RemovePoint(winding_t *w, int point);
//returns true if the planes are concave
int			Winding_PlanesConcave(winding_t *w1, winding_t *w2,
									 vec3_t normal1, vec3_t normal2,
									 float dist1, float dist2);

//try to merge the windings, returns the new merged winding or NULL
winding_t *Winding_TryMerge(winding_t *f1, winding_t *f2, vec3_t planenormal, int keep);