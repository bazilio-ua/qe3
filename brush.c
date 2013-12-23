#include <assert.h>
#include "qe3.h"
#include "winding.h"

#define MAX_POINTS_ON_WINDING	64

face_t *Face_Alloc( void );
void    Face_Free( face_t *f );

winding_t	*NewWinding (int points);
void		FreeWinding (winding_t *w);
winding_t	*Winding_Clone( winding_t *w );
winding_t	*ClipWinding (winding_t *in, plane_t *split, qboolean keepon);

brush_t *Brush_Alloc()
{
  brush_t *b = (brush_t*)qmalloc(sizeof(brush_t));
  return b;
}

void PrintWinding (winding_t *w)
{
	int		i;
	
	printf ("-------------\n");
	for (i=0 ; i<w->numpoints ; i++)
		printf ("(%5.2f, %5.2f, %5.2f)\n", w->points[i][0]
		, w->points[i][1], w->points[i][2]);
}

void PrintPlane (plane_t *p)
{
    printf ("(%5.2f, %5.2f, %5.2f) : %5.2f\n",  p->normal[0],  p->normal[1], 
    p->normal[2],  p->dist);
}

void PrintVector (vec3_t v)
{
     printf ("(%5.2f, %5.2f, %5.2f)\n",  v[0],  v[1], v[2]);
}

//============================================================================

#define	BOGUS_RANGE	18000


/*
==================
NewWinding
==================
*/
winding_t *NewWinding (int points)
{
	winding_t	*w;
	int			size;
	
	if (points > MAX_POINTS_ON_WINDING)
		Error ("NewWinding: %i points", points);
	
	size = (int)((winding_t *)0)->points[points];
	w = malloc (size);
	memset (w, 0, size);
	w->maxpoints = points;
	
	return w;
}


void FreeWinding (winding_t *w)
{
	free (w);
}


/*
==================
Winding_Clone
==================
*/
winding_t *Winding_Clone(winding_t *w)
{
	int			size;
	winding_t	*c;
	
	size = (int)((winding_t *)0)->points[w->numpoints];
	c = qmalloc (size);
	memcpy (c, w, size);
	return c;
}


/*
==================
ClipWinding

Clips the winding to the plane, returning the new winding on the positive side
Frees the input winding.
If keepon is true, an exactly on-plane winding will be saved, otherwise
it will be clipped away.
==================
*/
winding_t *ClipWinding (winding_t *in, plane_t *split, qboolean keepon)
{
	vec_t	dists[MAX_POINTS_ON_WINDING];
	int		sides[MAX_POINTS_ON_WINDING];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;
	winding_t	*neww;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (keepon && !counts[0] && !counts[1])
		return in;
		
	if (!counts[0])
	{
		FreeWinding (in);
		return NULL;
	}
	if (!counts[1])
		return in;
	
	maxpts = in->numpoints+4;	// can't use counts[0]+2 because
								// of fp grouping errors
	neww = NewWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		p2 = in->points[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}
	
	if (neww->numpoints > maxpts)
		Error ("ClipWinding: points exceeded estimate");
		
// free the original winding
	FreeWinding (in);
	
	return neww;
}


/*
=============================================================================

			TEXTURE COORDINATES

=============================================================================
*/


/*
==================
textureAxisFromPlane
==================
*/
vec3_t	baseaxis[18] =
{
{0,0,1}, {1,0,0}, {0,-1,0},			// floor
{0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
{1,0,0}, {0,1,0}, {0,0,-1},			// west wall
{-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
{0,1,0}, {1,0,0}, {0,0,-1},			// south wall
{0,-1,0}, {1,0,0}, {0,0,-1}			// north wall
};

void TextureAxisFromPlane(plane_t *pln, vec3_t xv, vec3_t yv)
{
	int		bestaxis;
	float	dot,best;
	int		i;
	
	best = 0;
	bestaxis = 0;
	
	for (i=0 ; i<6 ; i++)
	{
		dot = DotProduct (pln->normal, baseaxis[i*3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
	VectorCopy (baseaxis[bestaxis*3+1], xv);
	VectorCopy (baseaxis[bestaxis*3+2], yv);
}


//float	lightaxis[3] = {0.6, 0.8, 1.0};
vec_t	lightaxis[3] = {(vec_t)0.6, (vec_t)0.8, (vec_t)1.0};
/*
================
SetShadeForPlane

Light different planes differently to
improve recognition
================
*/
float SetShadeForPlane (plane_t *p)
{
	int		i;
	float	f;

	// axial plane
	for (i=0 ; i<3 ; i++)
		if (fabs(p->normal[i]) > 0.9)
		{
			f = lightaxis[i];
			return f;
		}

	// between two axial planes
	for (i=0 ; i<3 ; i++)
		if (fabs(p->normal[i]) < 0.1)
		{
			f = (lightaxis[(i+1)%3] + lightaxis[(i+2)%3])/2;
			return f;
		}

	// other
	f= (lightaxis[0] + lightaxis[1] + lightaxis[2]) / 3;
	return f;
}

vec3_t  vecs[2];
float	shift[2];

/*
================
BeginTexturingFace
================
*/
void BeginTexturingFace (brush_t *b, face_t *f, qtexture_t *q)
{
	vec3_t	pvecs[2];
	int		sv, tv;
	float	ang, sinv, cosv;
	float	ns, nt;
	int		i,j;
	float	shade;

	// get natural texture axis
	TextureAxisFromPlane(&f->plane, pvecs[0], pvecs[1]);

	// set shading for face
	shade = SetShadeForPlane (&f->plane);
	if (g_qeglobals.d_camera.draw_mode == cd_texture && !b->owner->eclass->fixedsize)
	{
		f->d_color[0] = 
		f->d_color[1] = 
		f->d_color[2] = shade;
	}
	else
	{
		f->d_color[0] = shade*q->color[0];
		f->d_color[1] = shade*q->color[1];
		f->d_color[2] = shade*q->color[2];
	}

	if (g_qeglobals.d_camera.draw_mode != cd_texture)
		return;

	if (!f->texdef.scale[0])
		f->texdef.scale[0] = 1;
	if (!f->texdef.scale[1])
		f->texdef.scale[1] = 1;


// rotate axis
	if (f->texdef.rotate == 0)
		{ sinv = 0 ; cosv = 1; }
	else if (f->texdef.rotate == 90)
		{ sinv = 1 ; cosv = 0; }
	else if (f->texdef.rotate == 180)
		{ sinv = 0 ; cosv = -1; }
	else if (f->texdef.rotate == 270)
		{ sinv = -1 ; cosv = 0; }
	else
	{	
		ang = f->texdef.rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if (pvecs[0][0])
		sv = 0;
	else if (pvecs[0][1])
		sv = 1;
	else
		sv = 2;
				
	if (pvecs[1][0])
		tv = 0;
	else if (pvecs[1][1])
		tv = 1;
	else
		tv = 2;
					
	for (i=0 ; i<2 ; i++)
	{
		ns = cosv * pvecs[i][sv] - sinv * pvecs[i][tv];
		nt = sinv * pvecs[i][sv] +  cosv * pvecs[i][tv];
		vecs[i][sv] = ns;
		vecs[i][tv] = nt;
	}

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<3 ; j++)
			vecs[i][j] = vecs[i][j] / f->texdef.scale[i];
}


void _EmitTextureCoordinates (vec3_t v, qtexture_t *q)
{
	float	s, t;

	s = DotProduct (v, vecs[0]);
	t = DotProduct (v, vecs[1]);
	
	s += shift[0];
	t += shift[1];
	
	s /= q->width;
	t /= q->height;

	glTexCoord2f (s, t);
}

void EmitTextureCoordinates ( float *xyzst, qtexture_t *q, face_t *f)
{
	float	s, t, ns, nt;
	float	ang, sinv, cosv;
	vec3_t	vecs[2];
	texdef_t	*td;

	// get natural texture axis
	TextureAxisFromPlane(&f->plane, vecs[0], vecs[1]);

	td = &f->texdef;

	ang = td->rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);
	
	if (!td->scale[0])
		td->scale[0] = 1;
	if (!td->scale[1])
		td->scale[1] = 1;

	s = DotProduct(xyzst, vecs[0]);
	t = DotProduct(xyzst, vecs[1]);

	ns = cosv * s - sinv * t;
	nt = sinv * s +  cosv * t;

	s = ns/td->scale[0] + td->shift[0];
	t = nt/td->scale[1] + td->shift[1];

	// gl scales everything from 0 to 1
	s /= q->width;
	t /= q->height;

	xyzst[3] = s;
	xyzst[4] = t;
}


//==========================================================================


/*
================
Face_Clone
================
*/
face_t	*Face_Clone (face_t *f)
{
	face_t	*n;

	n = Face_Alloc();
	n->texdef = f->texdef;
	memcpy (n->planepts, f->planepts, sizeof(n->planepts));

	// all other fields are derived, and will be set by Brush_Build
	return n;
}


/*
================
Face_MoveTexture
================
*/
void Face_MoveTexture(face_t *f, vec3_t move)
{
	vec3_t		pvecs[2];
	vec_t		s, t, ns, nt;
	vec_t		ang, sinv, cosv;
	texdef_t	*td;

	TextureAxisFromPlane(&f->plane, pvecs[0], pvecs[1]);
	td = &f->texdef;
	ang = td->rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);

	if (!td->scale[0])
		td->scale[0] = 1;
	if (!td->scale[1])
		td->scale[1] = 1;

	s = DotProduct(move, pvecs[0]);
	t = DotProduct(move, pvecs[1]);
	ns = cosv * s - sinv * t;
	nt = sinv * s +  cosv * t;
	s = ns/td->scale[0];
	t = nt/td->scale[1];

	f->texdef.shift[0] -= s;
	f->texdef.shift[1] -= t;

	while(f->texdef.shift[0] > f->d_texture->width)
		f->texdef.shift[0] -= f->d_texture->width;
	while(f->texdef.shift[1] > f->d_texture->height)
		f->texdef.shift[1] -= f->d_texture->height;
	while(f->texdef.shift[0] < 0)
		f->texdef.shift[0] += f->d_texture->width;
	while(f->texdef.shift[1] < 0)
		f->texdef.shift[1] += f->d_texture->height;

}


/*
=================
BasePolyForPlane
=================
*/
winding_t *BasePolyForPlane (plane_t *p)
{
	int		i, x;
	vec_t	max, v;
	vec3_t	org, vright, vup;
	winding_t	*w;
	
// find the major axis

	max = -BOGUS_RANGE;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabs(p->normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
		Error ("BasePolyForPlane: no axis found");
		
	VectorCopy (vec3_origin, vup);	
	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;		
	case 2:
		vup[0] = 1;
		break;		
	}


	v = DotProduct (vup, p->normal);
	VectorMA (vup, -v, p->normal, vup);
	VectorNormalize (vup);
		
	VectorScale (p->normal, p->dist, org);
	
	CrossProduct (vup, p->normal, vright);
	
	VectorScale (vup, 8192, vup);
	VectorScale (vright, 8192, vright);

// project a really big	axis aligned box onto the plane
	w = NewWinding (4);
	
	VectorSubtract (org, vright, w->points[0]);
	VectorAdd (w->points[0], vup, w->points[0]);
	
	VectorAdd (org, vright, w->points[1]);
	VectorAdd (w->points[1], vup, w->points[1]);
	
	VectorAdd (org, vright, w->points[2]);
	VectorSubtract (w->points[2], vup, w->points[2]);
	
	VectorSubtract (org, vright, w->points[3]);
	VectorSubtract (w->points[3], vup, w->points[3]);
	
	w->numpoints = 4;
	
	return w;	
}

void Brush_MakeFacePlanes (brush_t *b)
{
	face_t	*f;
	int		j;
	vec3_t	t1, t2, t3;

	for (f=b->brush_faces ; f ; f=f->next)
	{
	// convert to a vector / dist plane
		for (j=0 ; j<3 ; j++)
		{
			t1[j] = f->planepts[0][j] - f->planepts[1][j];
			t2[j] = f->planepts[2][j] - f->planepts[1][j];
			t3[j] = f->planepts[1][j];
		}
		
		CrossProduct(t1,t2, f->plane.normal);
		if (VectorCompare (f->plane.normal, vec3_origin))
			printf ("WARNING: brush plane with no normal\n");
		VectorNormalize (f->plane.normal);
		f->plane.dist = DotProduct (t3, f->plane.normal);
	}
}

void DrawBrushEntityName (brush_t *b)
{
	char	*name;
	float	a, s, c;
	vec3_t	mid;
	int		i;

	if (!b->owner)
		return;		// during contruction

	if (b->owner == world_entity)
		return;

	if (b != b->owner->brushes.onext)
		return;	// not key brush

	// draw the angle pointer
	a = FloatForKey (b->owner, "angle");
	if (a)
	{
		s = sin (a/180*Q_PI);
		c = cos (a/180*Q_PI);
		for (i=0 ; i<3 ; i++)
			mid[i] = (b->mins[i] + b->maxs[i])*0.5; 

		glBegin (GL_LINE_STRIP);
		glVertex3fv (mid);
		mid[0] += c*8;
		mid[1] += s*8;
		glVertex3fv (mid);
		mid[0] -= c*4;
		mid[1] -= s*4;
		mid[0] -= s*4;
		mid[1] += c*4;
		glVertex3fv (mid);
		mid[0] += c*4;
		mid[1] += s*4;
		mid[0] += s*4;
		mid[1] -= c*4;
		glVertex3fv (mid);
		mid[0] -= c*4;
		mid[1] -= s*4;
		mid[0] += s*4;
		mid[1] -= c*4;
		glVertex3fv (mid);
		glEnd ();
	}
/*
	if (!g_qeglobals.d_savedinfo.show_names)
		return;

	name = ValueForKey (b->owner, "classname");
	glRasterPos2f (b->mins[0]+4, b->mins[1]+4);
	glCallLists (strlen(name), GL_UNSIGNED_BYTE, name);
*/

	if (g_qeglobals.d_savedinfo.show_names)
	{
		name = ValueForKey (b->owner, "classname");
		glRasterPos3f (b->mins[0]+4, b->mins[1]+4, b->mins[2]+4);
		glCallLists (strlen(name), GL_UNSIGNED_BYTE, name);
	}
}

/*
=================
MakeFaceWinding

returns the visible polygon on a face
=================
*/
winding_t	*MakeFaceWinding (brush_t *b, face_t *face)
{
	winding_t	*w;
	face_t		*clip;
	plane_t			plane;
	qboolean		past;

	// get a poly that covers an effectively infinite area
	w = BasePolyForPlane (&face->plane);

	// chop the poly by all of the other faces
	past = false;
	for (clip = b->brush_faces ; clip && w ; clip=clip->next)
	{
		if (clip == face)
		{
			past = true;
			continue;
		}
		if (DotProduct (face->plane.normal, clip->plane.normal) > 0.999
			&& fabs(face->plane.dist - clip->plane.dist) < 0.01 )
		{	// identical plane, use the later one
			if (past)
			{
				free (w);
				return NULL;
			}
			continue;
		}

		// flip the plane, because we want to keep the back side
		VectorSubtract (vec3_origin,clip->plane.normal, plane.normal);
		plane.dist = -clip->plane.dist;
		
		w = ClipWinding (w, &plane, false);
		if (!w)
			return w;
	}
	
	if (w->numpoints < 3)
	{
		free(w);
		w = NULL;
	}

	if (!w)
		printf ("unused plane\n");

	return w;
}


void Brush_SnapPlanepts (brush_t *b)
{
	int		i, j;
	face_t	*f;

	if (g_qeglobals.d_savedinfo.noclamp)
		return;

	for (f=b->brush_faces ; f; f=f->next)
		for (i=0 ; i<3 ; i++)
			for (j=0 ; j<3 ; j++)
				f->planepts[i][j] = floor (f->planepts[i][j] + 0.5);
}
	
/*
** Brush_Build
**
** Builds a brush rendering data and also sets the min/max bounds
*/
#define	ZERO_EPSILON	0.001
void Brush_Build( brush_t *b )
{
//	int				order;
//	face_t			*face;
//	winding_t		*w;
	char			title[1024];

	if (modified != 1)
	{
		modified = true;	// mark the map as changed
		sprintf (title, "%s *", currentmap);

		QE_ConvertDOSToUnixName( title, title );
		Sys_SetTitle (title);
	}

	/*
	** build the windings and generate the bounding box
	*/
	Brush_BuildWindings( b );

	/*
	** move the points and edges if in select mode
	*/
	if (g_qeglobals.d_select_mode == sel_vertex || g_qeglobals.d_select_mode == sel_edge)
		SetupVertexSelection ();
}

/*
=================
Brush_Parse

The brush is NOT linked to any list
=================
*/
brush_t *Brush_Parse (void)
{
	brush_t		*b;
	face_t		*f;
	int			i,j;

	g_qeglobals.d_parsed_brushes++;
	b = qmalloc(sizeof(brush_t));
		
	do
	{
		if (!GetToken (true))
			break;
		if (!strcmp (token, "}") )
			break;
		
		f = Face_Alloc();

		// add the brush to the end of the chain, so
		// loading and saving a map doesn't reverse the order

		f->next = NULL;
		if (!b->brush_faces)
		{
			b->brush_faces = f;
		}
		else
		{
			face_t *scan;

			for (scan=b->brush_faces ; scan->next ; scan=scan->next)
				;
			scan->next = f;
		}

		// read the three point plane definition
		for (i=0 ; i<3 ; i++)
		{
			if (i != 0)
				GetToken (true);
			if (strcmp (token, "(") )
				Error ("parsing brush");
			
			for (j=0 ; j<3 ; j++)
			{
				GetToken (false);
				f->planepts[i][j] = atoi(token);
			}
			
			GetToken (false);
			if (strcmp (token, ")") )
				Error ("parsing brush");
				
		}

	// read the texturedef
		GetToken (false);
		strcpy(f->texdef.name, token);
		GetToken (false);
		f->texdef.shift[0] = atoi(token);
		GetToken (false);
		f->texdef.shift[1] = atoi(token);
		GetToken (false);
		f->texdef.rotate = atoi(token);	
		GetToken (false);
		f->texdef.scale[0] = atof(token);
		GetToken (false);
		f->texdef.scale[1] = atof(token);

		if ( g_qeglobals.d_savedinfo.hexen2_map )
		{
			int	l;
			GetToken (false);
			l = atoi(token); // just read hexen2 unused 'light/rad' map flag, needed for h2bsp
		}

		StringTolower (f->texdef.name);

		// the flags and value field aren't necessarily present
		f->d_texture = Texture_ForName( f->texdef.name );
//		f->texdef.flags = f->d_texture->flags;
//		f->texdef.value = f->d_texture->value;
//		f->texdef.contents = f->d_texture->contents;
/*
		if (TokenAvailable ())
		{
			GetToken (false);
			f->texdef.contents = atoi(token);
			GetToken (false);
			f->texdef.flags = atoi(token);
			GetToken (false);
			f->texdef.value = atoi(token);
		}
*/
	} while (1);

	return b;
}

/*
=================
Brush_Write
=================
*/
void Brush_Write (brush_t *b, FILE *f)
{
	face_t	*fa;
	char *pname;
	int		i;

	fprintf (f, "{\n");
	for (fa=b->brush_faces ; fa ; fa=fa->next)
	{
		for (i=0 ; i<3 ; i++)
			fprintf (f, "( %i %i %i ) ", (int)fa->planepts[i][0]
			, (int)fa->planepts[i][1], (int)fa->planepts[i][2]);

		pname = fa->texdef.name;
		if (pname[0] == 0)
			pname = "unnamed";

		fprintf (f, "%s %i %i %i ", pname,
			(int)fa->texdef.shift[0], (int)fa->texdef.shift[1],
			(int)fa->texdef.rotate);

		if (fa->texdef.scale[0] == (int)fa->texdef.scale[0])
			fprintf (f, "%i ", (int)fa->texdef.scale[0]);
		else
			fprintf (f, "%f ", (float)fa->texdef.scale[0]);
		if (fa->texdef.scale[1] == (int)fa->texdef.scale[1])
			fprintf (f, "%i", (int)fa->texdef.scale[1]);
		else
			fprintf (f, "%f", (float)fa->texdef.scale[1]);

		if ( g_qeglobals.d_savedinfo.hexen2_map )
		{
			int	l = 0;
			fprintf (f, " %i", (int)l); // just write hexen2 unused 'light/rad' map flag, needed for h2bsp
		}

		// only output flags and value if not default
/*
		if (fa->texdef.value != fa->d_texture->value
			|| fa->texdef.flags != fa->d_texture->flags
			|| fa->texdef.contents != fa->d_texture->contents)
		{
			fprintf (f, " %i %i %i", fa->texdef.contents, fa->texdef.flags, fa->texdef.value);
		}
*/
		fprintf (f, "\n");
	}
	fprintf (f, "}\n");
}


/*
=============
Brush_Create

Create non-textured blocks for entities
The brush is NOT linked to any list
=============
*/
brush_t	*Brush_Create (vec3_t mins, vec3_t maxs, texdef_t *texdef)
{
	int		i, j;
	vec3_t	pts[4][2];
	face_t	*f;
	brush_t	*b;

	for (i=0 ; i<3 ; i++)
		if (maxs[i] < mins[i])
			Error ("Brush_InitSolid: backwards");

	b = qmalloc (sizeof(brush_t));
	
	pts[0][0][0] = mins[0];
	pts[0][0][1] = mins[1];
	
	pts[1][0][0] = mins[0];
	pts[1][0][1] = maxs[1];
	
	pts[2][0][0] = maxs[0];
	pts[2][0][1] = maxs[1];
	
	pts[3][0][0] = maxs[0];
	pts[3][0][1] = mins[1];
	
	for (i=0 ; i<4 ; i++)
	{
		pts[i][0][2] = mins[2];
		pts[i][1][0] = pts[i][0][0];
		pts[i][1][1] = pts[i][0][1];
		pts[i][1][2] = maxs[2];
	}
	
	for (i=0 ; i<4 ; i++)
	{
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;
		j = (i+1)%4;

		VectorCopy (pts[j][1], f->planepts[0]);
		VectorCopy (pts[i][1], f->planepts[1]);
		VectorCopy (pts[i][0], f->planepts[2]);
	}
	
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy (pts[0][1], f->planepts[0]);
	VectorCopy (pts[1][1], f->planepts[1]);
	VectorCopy (pts[2][1], f->planepts[2]);

	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy (pts[2][0], f->planepts[0]);
	VectorCopy (pts[1][0], f->planepts[1]);
	VectorCopy (pts[0][0], f->planepts[2]);

	return b;
}


/*
=============
Brush_MakeSided

Makes the current brushhave the given number of 2d sides
=============
*/
void Brush_MakeSided (int sides)
{
	int		i, axis;
	vec3_t	mins, maxs;
	brush_t	*b;
	texdef_t	*texdef;
	face_t	*f;
	vec3_t	mid;
	float	width;
	float	sv, cv;

	if (sides < 3)
	{
		Sys_Printf ("Bad sides number\n");
		return;
	}

	if (sides >= MAX_POINTS_ON_WINDING-4)
	{
		Sys_Printf ("Too many sides\n");
		return;
	}

	if (!QE_SingleBrush ())
	{
		Sys_Printf ("Must have a single brush selected\n");
		return;
	}

	b = selected_brushes.next;
	VectorCopy (b->mins, mins);
	VectorCopy (b->maxs, maxs);
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free (b);

	switch(g_qeglobals.d_viewtype)
	{
		case XY: axis = 2; break;
		case XZ: axis = 1; break;
		case YZ: axis = 0; break;
	}

	// find center of brush
	width = 8;
	for (i = 0 ; i < 3 ; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (i == axis) continue;

//		if (maxs[i] - mins[i] > width)
//			width = maxs[i] - mins[i];

		if ((maxs[i] - mins[i]) * 0.5 > width)
			width = (maxs[i] - mins[i]) * 0.5;

	}

//	width /= 2;

	b = qmalloc (sizeof(brush_t));
		
	// create top face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[2][(axis+1)%3] = mins[(axis+1)%3]; f->planepts[2][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[2][axis] = maxs[axis];
	f->planepts[1][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[1][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[1][axis] = maxs[axis];
	f->planepts[0][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[0][(axis+2)%3] = maxs[(axis+2)%3]; f->planepts[0][axis] = maxs[axis];
/*
	f->planepts[2][0] = mins[0];f->planepts[2][1] = mins[1];f->planepts[2][2] = maxs[2];
	f->planepts[1][0] = maxs[0];f->planepts[1][1] = mins[1];f->planepts[1][2] = maxs[2];
	f->planepts[0][0] = maxs[0];f->planepts[0][1] = maxs[1];f->planepts[0][2] = maxs[2];
*/
	// create bottom face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[0][(axis+1)%3] = mins[(axis+1)%3]; f->planepts[0][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[0][axis] = mins[axis];
	f->planepts[1][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[1][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[1][axis] = mins[axis];
	f->planepts[2][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[2][(axis+2)%3] = maxs[(axis+2)%3]; f->planepts[2][axis] = mins[axis];
/*
	f->planepts[0][0] = mins[0];f->planepts[0][1] = mins[1];f->planepts[0][2] = mins[2];
	f->planepts[1][0] = maxs[0];f->planepts[1][1] = mins[1];f->planepts[1][2] = mins[2];
	f->planepts[2][0] = maxs[0];f->planepts[2][1] = maxs[1];f->planepts[2][2] = mins[2];
*/
	for (i=0 ; i<sides ; i++)
	{
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		sv = sin (i*Q_PI*2/sides);
		cv = cos (i*Q_PI*2/sides);

		f->planepts[0][(axis+1)%3] = floor(mid[(axis+1)%3]+width*cv+0.5);
		f->planepts[0][(axis+2)%3] = floor(mid[(axis+2)%3]+width*sv+0.5);
		f->planepts[0][axis] = mins[axis];

		f->planepts[1][(axis+1)%3] = f->planepts[0][(axis+1)%3];
		f->planepts[1][(axis+2)%3] = f->planepts[0][(axis+2)%3];
		f->planepts[1][axis] = maxs[axis];

		f->planepts[2][(axis+1)%3] = floor(f->planepts[0][(axis+1)%3] - width*sv + 0.5);
		f->planepts[2][(axis+2)%3] = floor(f->planepts[0][(axis+2)%3] + width*cv + 0.5);
		f->planepts[2][axis] = maxs[axis];
/*
		f->planepts[0][0] = floor(mid[0]+width*cv+0.5);
		f->planepts[0][1] = floor(mid[1]+width*sv+0.5);
		f->planepts[0][2] = mins[2];

		f->planepts[1][0] = f->planepts[0][0];
		f->planepts[1][1] = f->planepts[0][1];
		f->planepts[1][2] = maxs[2];

		f->planepts[2][0] = floor(f->planepts[0][0] - width*sv + 0.5);
		f->planepts[2][1] = floor(f->planepts[0][1] + width*cv + 0.5);
		f->planepts[2][2] = maxs[2];
*/
	}

	Brush_AddToList (b, &selected_brushes);

	Entity_LinkBrush (world_entity, b);

	Brush_Build( b );

	Sys_UpdateWindows (W_ALL);
}


/*
=============
Brush_Free

Frees the brush with all of its faces and display list.
Unlinks the brush from whichever chain it is in.
Decrements the owner entity's brushcount.
Removes owner entity if this was the last brush
unless owner is the world.
=============
*/
void Brush_Free (brush_t *b)
{
	face_t	*f, *next;

	// free faces
	for (f=b->brush_faces ; f ; f=next)
	{
		next = f->next;
		Face_Free( f );
	}

	/*
	for ( i = 0; i < b->d_numwindings; i++ )
	{
		if ( b->d_windings[i] )
		{
			FreeWinding( b->d_windings[i] );
			b->d_windings[i] = 0;
		}
	}
	*/

	// unlink from active/selected list
	if (b->next)
		Brush_RemoveFromList (b);

	// unlink from entity list
	if (b->onext)
		Entity_UnlinkBrush (b);

	free (b);
}

/*
============
Brush_Move
============
*/
void Brush_Move (brush_t *b, vec3_t move)
{
	int		i;
	face_t	*f;
/*
	vec3_t		pvecs[2];
	vec_t		s, t, ns, nt;
	vec_t		ang, sinv, cosv;
	texdef_t	*td;
*/
	for (f=b->brush_faces ; f ; f=f->next)
	{
		if(g_qeglobals.d_textures_lock)
		{
			Face_MoveTexture(f, move);
/*

			TextureAxisFromPlane(&f->plane, pvecs[0], pvecs[1]);
			td = &f->texdef;
			ang = td->rotate / 180 * Q_PI;
			sinv = sin(ang);
			cosv = cos(ang);

			if (!td->scale[0])
				td->scale[0] = 1;
			if (!td->scale[1])
				td->scale[1] = 1;

			s = DotProduct(move, pvecs[0]);
			t = DotProduct(move, pvecs[1]);
			ns = cosv * s - sinv * t;
			nt = sinv * s +  cosv * t;
			s = ns/td->scale[0];
			t = nt/td->scale[1];

			f->texdef.shift[0] -= s;
			f->texdef.shift[1] -= t;

			while(f->texdef.shift[0] > f->d_texture->width)
				f->texdef.shift[0] -= f->d_texture->width;
			while(f->texdef.shift[1] > f->d_texture->height)
				f->texdef.shift[1] -= f->d_texture->height;
			while(f->texdef.shift[0] < 0)
				f->texdef.shift[0] += f->d_texture->width;
			while(f->texdef.shift[1] < 0)
				f->texdef.shift[1] += f->d_texture->height;
*/

		}
		for (i=0 ; i<3 ; i++)
			VectorAdd (f->planepts[i], move, f->planepts[i]);
	}
	Brush_Build( b );

	// PGM - keep the origin vector up to date on fixed size entities.
	if(b->owner->eclass->fixedsize)
	{
		VectorAdd(b->owner->origin, move, b->owner->origin);
	}
}

/*
============
Brush_Clone

Does NOT add the new brush to any lists
============
*/
brush_t *Brush_Clone (brush_t *b)
{
	brush_t	*n;
	face_t	*f, *nf;

	n = qmalloc(sizeof(brush_t));
	n->owner = b->owner;
	for (f=b->brush_faces ; f ; f=f->next)
	{
		nf = Face_Clone( f );
		nf->next = n->brush_faces;
		n->brush_faces = nf;
	}
	return n;
}

/*
==============
Brush_Ray

Itersects a ray with a brush
Returns the face hit and the distance along the ray the intersection occured at
Returns NULL and 0 if not hit at all
==============
*/
face_t *Brush_Ray (vec3_t origin, vec3_t dir, brush_t *b, float *dist)
{
	face_t	*f, *firstface;
	vec3_t	p1, p2;
	float	frac, d1, d2;
	int		i;

	VectorCopy (origin, p1);
	for (i=0 ; i<3 ; i++)
		p2[i] = p1[i] + dir[i]*16384;

	for (f=b->brush_faces ; f ; f=f->next)
	{
		d1 = DotProduct (p1, f->plane.normal) - f->plane.dist;
		d2 = DotProduct (p2, f->plane.normal) - f->plane.dist;
		if (d1 >= 0 && d2 >= 0)
		{
			*dist = 0;
			return NULL;	// ray is on front side of face
		}
		if (d1 <=0 && d2 <= 0)
			continue;
	// clip the ray to the plane
		frac = d1 / (d1 - d2);
		if (d1 > 0)
		{
			firstface = f;
			for (i=0 ; i<3 ; i++)
				p1[i] = p1[i] + frac *(p2[i] - p1[i]);
		}
		else
		{
			for (i=0 ; i<3 ; i++)
				p2[i] = p1[i] + frac *(p2[i] - p1[i]);
		}
	}

	// find distance p1 is along dir
	VectorSubtract (p1, origin, p1);
	d1 = DotProduct (p1, dir);

	*dist = d1;

	return firstface;
}

void	Brush_AddToList (brush_t *b, brush_t *list)
{
	if (b->next || b->prev)
		Error ("Brush_RemoveFromList: already linked");
	b->next = list->next;
	list->next->prev = b;
	list->next = b;
	b->prev = list;
}

void	Brush_RemoveFromList (brush_t *b)
{
	if (!b->next || !b->prev)
		Error ("Brush_RemoveFromList: not linked");
	b->next->prev = b->prev;
	b->prev->next = b->next;
	b->next = b->prev = NULL;
}

void	Brush_SetTexture (brush_t *b, texdef_t *texdef)
{
	face_t	*f;

	for (f=b->brush_faces ; f ; f=f->next)
		f->texdef = *texdef;
	Brush_Build( b );
}


qboolean ClipLineToFace (vec3_t p1, vec3_t p2, face_t *f)
{
	float	d1, d2, fr;
	int		i;
	float	*v;

	d1 = DotProduct (p1, f->plane.normal) - f->plane.dist;
	d2 = DotProduct (p2, f->plane.normal) - f->plane.dist;

	if (d1 >= 0 && d2 >= 0)
		return false;		// totally outside
	if (d1 <= 0 && d2 <= 0)
		return true;		// totally inside

	fr = d1 / (d1 - d2);

	if (d1 > 0)
		v = p1;
	else
		v = p2;

	for (i=0 ; i<3 ; i++)
		v[i] = p1[i] + fr*(p2[i] - p1[i]);

	return true;
}


int AddPlanept (float *f)
{
	int		i;

	for (i=0 ; i<g_qeglobals.d_num_move_points ; i++)
		if (g_qeglobals.d_move_points[i] == f)
			return 0;
	g_qeglobals.d_move_points[g_qeglobals.d_num_move_points++] = f;
	return 1;
}

/*
==============
Brush_SelectFaceForDragging

Adds the faces planepts to move_points, and
rotates and adds the planepts of adjacent face if shear is set
==============
*/
void Brush_SelectFaceForDragging (brush_t *b, face_t *f, qboolean shear)
{
	int		i;
	face_t	*f2;
	winding_t	*w;
	float	d;
	brush_t	*b2;
	int		c;

	if (b->owner->eclass->fixedsize)
		return;

	c = 0;
	for (i=0 ; i<3 ; i++)
		c += AddPlanept (f->planepts[i]);
	if (c == 0)
		return;		// already completely added

	// select all points on this plane in all brushes the selection
	for (b2=selected_brushes.next ; b2 != &selected_brushes ; b2 = b2->next)
	{
		if (b2 == b)
			continue;
		for (f2=b2->brush_faces ; f2 ; f2=f2->next)
		{
			for (i=0 ; i<3 ; i++)
				if (fabs(DotProduct(f2->planepts[i], f->plane.normal)
				-f->plane.dist) > ON_EPSILON)
					break;
			if (i==3)
			{	// move this face as well
				Brush_SelectFaceForDragging (b2, f2, shear);
				break;
			}
		}
	}


	// if shearing, take all the planes adjacent to 
	// selected faces and rotate their points so the
	// edge clipped by a selcted face has two of the points
	if (!shear)
		return;

	for (f2=b->brush_faces ; f2 ; f2=f2->next)
	{
		if (f2 == f)
			continue;
		w = MakeFaceWinding (b, f2);
		if (!w)
			continue;

		// any points on f will become new control points
		for (i=0 ; i<w->numpoints ; i++)
		{
			d = DotProduct (w->points[i], f->plane.normal) 
				- f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				break;
		}

		//
		// if none of the points were on the plane,
		// leave it alone
		//
		if (i != w->numpoints)
		{
			if (i == 0)
			{	// see if the first clockwise point was the
				// last point on the winding
				d = DotProduct (w->points[w->numpoints-1]
					, f->plane.normal) - f->plane.dist;
				if (d > -ON_EPSILON && d < ON_EPSILON)
					i = w->numpoints - 1;
			}

			AddPlanept (f2->planepts[0]);

			VectorCopy (w->points[i], f2->planepts[0]);
			if (++i == w->numpoints)
				i = 0;
			
			// see if the next point is also on the plane
			d = DotProduct (w->points[i]
				, f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				AddPlanept (f2->planepts[1]);

			VectorCopy (w->points[i], f2->planepts[1]);
			if (++i == w->numpoints)
				i = 0;

			// the third point is never on the plane

			VectorCopy (w->points[i], f2->planepts[2]);
		}

		free(w);
	}
}

/*
==============
Brush_SideSelect

The mouse click did not hit the brush, so grab one or more side
planes for dragging
==============
*/
void Brush_SideSelect (brush_t *b, vec3_t origin, vec3_t dir
					   , qboolean shear)
{
	face_t	*f, *f2;
	vec3_t	p1, p2;

	for (f=b->brush_faces ; f ; f=f->next)
	{
		VectorCopy (origin, p1);
		VectorMA (origin, 16384, dir, p2);

		for (f2=b->brush_faces ; f2 ; f2=f2->next)
		{
			if (f2 == f)
				continue;
			ClipLineToFace (p1, p2, f2);
		}

		if (f2)
			continue;

		if (VectorCompare (p1, origin))
			continue;
		if (ClipLineToFace (p1, p2, f))
			continue;

		Brush_SelectFaceForDragging (b, f, shear);
	}

	
}

void Brush_BuildWindings( brush_t *b )
{
	winding_t *w;
	face_t    *face;
	vec_t      v;

	Brush_SnapPlanepts( b );

	// clear the mins/maxs bounds
	b->mins[0] = b->mins[1] = b->mins[2] = 99999;
	b->maxs[0] = b->maxs[1] = b->maxs[2] = -99999;

	Brush_MakeFacePlanes (b);

	face = b->brush_faces;

	for ( ; face ; face=face->next)
	{
		int i, j;

		w = face->face_winding = MakeFaceWinding (b, face);
		face->d_texture = Texture_ForName( face->texdef.name );

		if (!w)
		{
			continue;
		}
	
	    for (i=0 ; i<w->numpoints ; i++)
	    {
			// add to bounding box
			for (j=0 ; j<3 ; j++)
			{
				v = w->points[i][j];
				if (v > b->maxs[j])
					b->maxs[j] = v;
				if (v < b->mins[j])
					b->mins[j] = v;
			}
	    }
		// setup s and t vectors, and set color
		BeginTexturingFace( b, face, face->d_texture);


	    for (i=0 ; i<w->numpoints ; i++)
	    {
			EmitTextureCoordinates( w->points[i], face->d_texture, face);
	    }
	}
}

/*
==================
Brush_RemoveEmptyFaces

Frees any overconstraining faces
==================
*/
void Brush_RemoveEmptyFaces ( brush_t *b )
{
	face_t	*f, *next;

	f = b->brush_faces;
	b->brush_faces = NULL;

	for ( ; f ; f=next)
	{
		next = f->next;
		if (!f->face_winding)
			Face_Free (f);
		else
		{
			f->next = b->brush_faces;
			b->brush_faces = f;
		}

	}
}


/*
==================
DrawLight
==================
*/
void DrawLight(brush_t *b)
{
	vec3_t color;
	vec3_t corners[4];
	vec3_t top, bottom;
	vec3_t save;
	qboolean paint;
	char *strcolor;
	float cr, cg, cb;
	float mid;
	int n, i;

  paint = false;

  color[0] = color[1] = color[2] = 1.0;

  paint = true;

  strcolor = ValueForKey(b->owner, "_color");

  if (strcolor)
  {
	n = sscanf(strcolor,"%f %f %f", &cr, &cg, &cb);
    if (n == 3)
    {
      color[0] = cr;
      color[1] = cg;
      color[2] = cb;
    }
  }
  glColor3f(color[0], color[1], color[2]);
  
  mid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

  corners[0][0] = b->mins[0];
  corners[0][1] = b->mins[1];
  corners[0][2] = mid;

  corners[1][0] = b->mins[0];
  corners[1][1] = b->maxs[1];
  corners[1][2] = mid;

  corners[2][0] = b->maxs[0];
  corners[2][1] = b->maxs[1];
  corners[2][2] = mid;

  corners[3][0] = b->maxs[0];
  corners[3][1] = b->mins[1];
  corners[3][2] = mid;

  top[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
  top[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
  top[2] = b->maxs[2];

  VectorCopy(top, bottom);
  bottom[2] = b->mins[2];

  VectorCopy(color, save);

  glBegin(GL_TRIANGLE_FAN);
  glVertex3fv(top);
  for (i = 0; i <= 3; i++)
  {
    color[0] *= (float)0.95;
    color[1] *= (float)0.95;
    color[2] *= (float)0.95;
    glColor3f(color[0], color[1], color[2]);
    glVertex3fv(corners[i]);
  }
  glVertex3fv(corners[0]);
  glEnd();
  
  VectorCopy(save, color);
  color[0] *= (float)0.95;
  color[1] *= (float)0.95;
  color[2] *= (float)0.95;

  glBegin(GL_TRIANGLE_FAN);
  glVertex3fv(bottom);
  glVertex3fv(corners[0]);
  for (i = 3; i >= 0; i--)
  {
    color[0] *= (float)0.95;
    color[1] *= (float)0.95;
    color[2] *= (float)0.95;
    glColor3f(color[0], color[1], color[2]);
    glVertex3fv(corners[i]);
  }
  glEnd();
}


/*
==================
FacingVectors
==================
*/
void FacingVectors (entity_t *e, vec3_t forward, vec3_t right, vec3_t up)
{
	int			angle;
	vec3_t		angles;

	angle = IntForKey(e, "angle");
	if (angle == -1)				// up
	{
		VectorSet(angles, 270, 0, 0);
	}
	else if(angle == -2)		// down
	{
		VectorSet(angles, 90, 0, 0);
	}
	else
	{
		VectorSet(angles, 0, angle, 0);
	}

	AngleVectors(angles, forward, right, up);
}


/*
==================
Brush_DrawFacingAngle
==================
*/
void Brush_DrawFacingAngle (brush_t *b, entity_t *e)
{
	vec3_t	forward, right, up;
	vec3_t	endpoint, tip1, tip2;
	vec3_t	start;
	float	dist;

	VectorAdd(e->brushes.onext->mins, e->brushes.onext->maxs, start);
	VectorScale(start, 0.5, start);
	dist = (b->maxs[0] - start[0]) * 2.5;

	FacingVectors (e, forward, right, up);
	VectorMA (start, dist, forward, endpoint);

	dist = (b->maxs[0] - start[0]) * 0.5;
	VectorMA (endpoint, -dist, forward, tip1);
	VectorMA (tip1, -dist, up, tip1);
	VectorMA (tip1, 2*dist, up, tip2);

	glColor4f (1, 1, 1, 1);
	glLineWidth (4);
	glBegin (GL_LINES);
	glVertex3fv (start);
	glVertex3fv (endpoint);
	glVertex3fv (endpoint);
	glVertex3fv (tip1);
	glVertex3fv (endpoint);
	glVertex3fv (tip2);
	glEnd ();
	glLineWidth (1);
}


/*
==================
Brush_Draw
==================
*/
void Brush_Draw( brush_t *b )
{
	face_t			*face;
	int				i, order;
    qtexture_t		*prev = 0;
	winding_t *w;

	if (b->hiddenbrush)
		return;

//	if (b->owner->eclass->fixedsize && g_qeglobals.d_camera.draw_mode == cd_texture)
//		glDisable (GL_TEXTURE_2D);

	if (b->owner->eclass->fixedsize)
	{

		if (!(g_qeglobals.d_savedinfo.exclude & EXCLUDE_ANGLES) && (b->owner->eclass->showflags & ECLASS_ANGLE))
		{
			Brush_DrawFacingAngle(b, b->owner);
		}

		if (g_qeglobals.d_savedinfo.view_radiantlights && (b->owner->eclass->showflags & ECLASS_LIGHT))
		{
			DrawLight(b);
			return;
		}
/*
		if (g_qeglobals.d_savedinfo.view_radiantlights && (!strncmp(b->owner->eclass->name, "light", 5)))
		{
			DrawLight(b);
			return;
		}
*/
		if (g_qeglobals.d_camera.draw_mode == cd_texture)
			glDisable (GL_TEXTURE_2D);

	}

	// guarantee the texture will be set first
	prev = NULL;
	for (face = b->brush_faces,order = 0 ; face ; face=face->next, order++)
	{
		w = face->face_winding;
		if (!w)
			continue;		// freed face

		if ( face->d_texture != prev && g_qeglobals.d_camera.draw_mode == cd_texture)
		{
			// set the texture for this face
			prev = face->d_texture;
			glBindTexture( GL_TEXTURE_2D, face->d_texture->texture_number );
		}

		glColor3fv( face->d_color );

		// draw the polygon
		glBegin(GL_POLYGON);
	    for (i=0 ; i<w->numpoints ; i++)
		{
			if (g_qeglobals.d_camera.draw_mode == cd_texture)
				glTexCoord2fv( &w->points[i][3] );
			glVertex3fv(w->points[i]);
		}
		glEnd();
	}

	if (b->owner->eclass->fixedsize && g_qeglobals.d_camera.draw_mode == cd_texture)
		glEnable (GL_TEXTURE_2D);

	glBindTexture( GL_TEXTURE_2D, 0 );
}

void Face_Draw( face_t *f )
{
	int i;

	if ( f->face_winding == 0 )
		return;
	glBegin( GL_POLYGON );
	for ( i = 0 ; i < f->face_winding->numpoints; i++)
		glVertex3fv( f->face_winding->points[i] );
	glEnd();
}

void Brush_DrawXY(brush_t *b, int viewtype)
{
	face_t *face;
	int     order;
	winding_t *w;
	int        i;

	vec3_t corners[4];
	vec3_t top, bottom;
	float mid;

	if (b->hiddenbrush)
		return;


	if (b->owner->eclass->fixedsize)
	{
//		if (g_qeglobals.d_savedinfo.view_radiantlights && (!strncmp(b->owner->eclass->name, "light", 5)))
		if (g_qeglobals.d_savedinfo.view_radiantlights && (b->owner->eclass->showflags & ECLASS_LIGHT))
		{
    
			mid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

			corners[0][0] = b->mins[0];
			corners[0][1] = b->mins[1];
			corners[0][2] = mid;

			corners[1][0] = b->mins[0];
			corners[1][1] = b->maxs[1];
			corners[1][2] = mid;

			corners[2][0] = b->maxs[0];
			corners[2][1] = b->maxs[1];
			corners[2][2] = mid;

			corners[3][0] = b->maxs[0];
			corners[3][1] = b->mins[1];
			corners[3][2] = mid;

			top[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
			top[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
			top[2] = b->maxs[2];

			VectorCopy(top, bottom);
			bottom[2] = b->mins[2];
	    
			glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
			glBegin(GL_TRIANGLE_FAN);
			glVertex3fv(top);
			glVertex3fv(corners[0]);
			glVertex3fv(corners[1]);
			glVertex3fv(corners[2]);
			glVertex3fv(corners[3]);
			glVertex3fv(corners[0]);
			glEnd();
			glBegin(GL_TRIANGLE_FAN);
			glVertex3fv(bottom);
			glVertex3fv(corners[0]);
			glVertex3fv(corners[3]);
			glVertex3fv(corners[2]);
			glVertex3fv(corners[1]);
			glVertex3fv(corners[0]);
			glEnd();
			DrawBrushEntityName (b);
			return;
		}
	}

	for (face = b->brush_faces,order = 0 ; face ; face=face->next, order++)
	{
		// only draw up facing polygons //
//		if (face->plane.normal[2] <= 0)
//			continue;

		// only draw polygons facing in a direction we care about
		if (viewtype == XY)
		{
			if (face->plane.normal[2] <= 0)
				continue;
		}
		else
		{
			if (viewtype == XZ)
			{
				if (face->plane.normal[1] <= 0)
					continue;
			}
			else 
			{
				if (face->plane.normal[0] <= 0)
					continue;
			}
		}

		w = face->face_winding;
		if (!w)
			continue;

		// draw the polygon
		glBegin(GL_LINE_LOOP);
	    for (i=0 ; i<w->numpoints ; i++)
			glVertex3fv(w->points[i]);
		glEnd();
	}

	// optionally add a text label
	if ( g_qeglobals.d_savedinfo.show_names )
		DrawBrushEntityName (b);
}

face_t *Face_Alloc( void )
{
	face_t *f = qmalloc( sizeof( *f ) );

	return f;
}

void Face_Free( face_t *f )
{
	assert( f != 0 );

	if ( f->face_winding )
		free( f->face_winding ), f->face_winding = 0;
	free( f );
}


void Face_FitTexture( face_t *face, int height, int width )
{
	winding_t *w;
	vec3_t   mins, maxs;
	int i;
	vec_t temp;
	vec_t rot_width, rot_height;
	vec_t cosv,sinv,ang;
	vec_t min_t, min_s, max_t, max_s;
	vec_t s,t;
	vec3_t	vecs[2];
	vec3_t   coords[4];
	texdef_t	*td;
	
	if (height < 1)
		height = 1;
	if (width < 1)
		width = 1;
	
	ClearBounds (mins, maxs);
	
	w = face->face_winding;
	if (!w)
	{
		return;
	}
	for (i=0 ; i<w->numpoints ; i++)
	{
		AddPointToBounds( w->points[i], mins, maxs );
	}
	
	td = &face->texdef;
	// 
	// get the current angle
	//
	ang = td->rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);
	
	// get natural texture axis
	TextureAxisFromPlane(&face->plane, vecs[0], vecs[1]);
	
	min_s = DotProduct( mins, vecs[0] );
	min_t = DotProduct( mins, vecs[1] );
	max_s = DotProduct( maxs, vecs[0] );
	max_t = DotProduct( maxs, vecs[1] );
	coords[0][0] = min_s;
	coords[0][1] = min_t;
	coords[1][0] = max_s;
	coords[1][1] = min_t;
	coords[2][0] = min_s;
	coords[2][1] = max_t;
	coords[3][0] = max_s;
	coords[3][1] = max_t;
	min_s = min_t = 99999;
	max_s = max_t = -99999;
	for (i=0; i<4; i++)
	{
		s = cosv * coords[i][0] - sinv * coords[i][1];
		t = sinv * coords[i][0] + cosv * coords[i][1];
		if (i&1)
		{
			if (s > max_s) 
			{
				max_s = s;
			}
		}
		else
		{
			if (s < min_s) 
			{
				min_s = s;
			}
			if (i<2)
			{
				if (t < min_t) 
				{
					min_t = t;
				}
			}
			else
			{
				if (t > max_t) 
				{
					max_t = t;
				}
			}
		}
	}
	rot_width =  (max_s - min_s);
	rot_height = (max_t - min_t);
	td->scale[0] = -(rot_width/((float)(face->d_texture->width*width)));
	td->scale[1] = -(rot_height/((float)(face->d_texture->height*height)));
	td->shift[0] = min_s/td->scale[0];
	temp = (int)(td->shift[0] / (face->d_texture->width*width));
	temp = (temp+1)*face->d_texture->width*width;
	td->shift[0] = (int)(temp - td->shift[0])%(face->d_texture->width*width);
	
	td->shift[1] = min_t/td->scale[1];
	temp = (int)(td->shift[1] / (face->d_texture->height*height));
	temp = (temp+1)*(face->d_texture->height*height);
	td->shift[1] = (int)(temp - td->shift[1])%(face->d_texture->height*height);
}


void Brush_FitTexture( brush_t *b, int height, int width )
{
	face_t *face;
	
	for (face = b->brush_faces ; face ; face=face->next)
	{
		Face_FitTexture( face, height, width );
	}
}
