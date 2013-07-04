

//returns true if the planes are equal
int			Plane_Equal(plane_t *a, plane_t *b, int flip);

//returns true if the planes are concave
int			Winding_PlanesConcave(winding_t *w1, winding_t *w2,
									 vec3_t normal1, vec3_t normal2,
									 float dist1, float dist2);
