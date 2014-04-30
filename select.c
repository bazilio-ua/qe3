// select.c

#include "qe3.h"


/*
===========
Test_Ray
===========
*/
trace_t Test_Ray (vec3_t origin, vec3_t dir, int flags)
{
	brush_t	*brush;
	face_t	*face;
	float	dist;
	trace_t	t;

	memset (&t, 0, sizeof(t));
	t.dist = DIST_START;

	// single selection cycle (shift+alt+LBUTTON)
	if (flags & SF_CYCLE)
	{
		int			size, j = 0;
		brush_t	   *select;
		brush_t	   *array[MAX_MAP_BRUSHES];  

		select = (selected_brushes.next != &selected_brushes) ? selected_brushes.next : NULL;
		Select_Deselect();

		// go through active brushes and accumulate all "hit" brushes
		for (brush = active_brushes.next; brush != &active_brushes; brush = brush->next)
		{
			if (FilterBrush (brush))
				continue;

			face = Brush_Ray (origin, dir, brush, &dist);

			if (face)
			{
				array[j] = brush;
				j++;
			}
		}

		size = j;
		if (size)
		{
			brush_t	   *b;
			qboolean	found = false;
			int			i;

			for (i = 0; i < size; i++)
			{
				b = array[i];
				// did we hit the last one selected yet ?
				if (b == select)
				{
					// yes we want to select the next one in the list 
					int n = (i > 0) ? i - 1: size - 1;
					select = array[n];
					found = true;
					break;
				}
			}
			if (!found)
			{
				j = 0;
				select = array[0];
			}
		}
		if (select)
		{
			face = Brush_Ray (origin, dir, select, &dist);
			t.dist = dist;
			t.brush = select;
			t.face = face;
			t.selected = false;
			return t;
		}
	}

	if (! (flags & SF_SELECTED_ONLY) )
	{
		for (brush = active_brushes.next ; brush != &active_brushes ; brush=brush->next)
		{
			if ( (flags & SF_ENTITIES_FIRST) && brush->owner == world_entity)
				continue;
			if (FilterBrush (brush))
				continue;
			face = Brush_Ray (origin, dir, brush, &dist);
			if (dist > 0 && dist < t.dist)
			{
				t.dist = dist;
				t.brush = brush;
				t.face = face;
				t.selected = false;
			}
		}
	}
	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
	{
		if ( (flags & SF_ENTITIES_FIRST) && brush->owner == world_entity)
			continue;
		if (FilterBrush (brush))
			continue;
		face = Brush_Ray (origin, dir, brush, &dist);
		if (dist > 0 && dist < t.dist)
		{
			t.dist = dist;
			t.brush = brush;
			t.face = face;
			t.selected = true;
		}
	}

	// if entites first, but didn't find any, check regular
	if ( (flags & SF_ENTITIES_FIRST) && t.brush == NULL)
		return Test_Ray (origin, dir, flags - SF_ENTITIES_FIRST);

	return t;
}


/*
============
Select_Brush

============
*/
//void Select_Brush (brush_t *brush)
void Select_Brush (brush_t *brush, qboolean complete)
{
	brush_t		*b;
	entity_t	*e;
	char	selectionstring[256];
	char	*name;
	vec3_t mins, maxs, size;

	selected_face = NULL;
	if (g_qeglobals.d_select_count < 2)
		g_qeglobals.d_select_order[g_qeglobals.d_select_count] = brush;
	g_qeglobals.d_select_count++;

	e = brush->owner;
	if (e)
	{
		// select complete entity on first click
		if (e != world_entity && complete == true)
		{
			for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
				if (b->owner == e)
					goto singleselect;
			for (b=e->brushes.onext ; b != &e->brushes ; b=b->onext)
			{
				Brush_RemoveFromList (b);
				Brush_AddToList (b, &selected_brushes);
			}
		}
		else
		{
singleselect:
			Brush_RemoveFromList (brush);
			Brush_AddToList (brush, &selected_brushes);
		}

		if (e->eclass)
		{
			UpdateEntitySel(brush->owner->eclass);
		}
	}
	Select_GetBounds (mins, maxs);
	VectorSubtract(maxs, mins, size);
	name = ValueForKey (brush->owner, "classname");
	sprintf (selectionstring, "Selected object: %s (%i %i %i)", name, (int)size[0], (int)size[1], (int)size[2]);
	Sys_Status (selectionstring, 3);
}


/*
============
Select_Ray

If the origin is inside a brush, that brush will be ignored.
============
*/
void Select_Ray (vec3_t origin, vec3_t dir, int flags)
{
	trace_t	t;

	t = Test_Ray (origin, dir, flags);
	if (!t.brush)
		return;

	if (flags == SF_SINGLEFACE)
	{
		selected_face = t.face;
		selected_face_brush = t.brush;
		Sys_UpdateWindows (W_ALL);
		g_qeglobals.d_select_mode = sel_brush;
		Texture_SetTexture (&t.face->texdef);
		return;
	}

	// move the brush to the other list

	g_qeglobals.d_select_mode = sel_brush;

	if (t.selected)
	{		
		Brush_RemoveFromList (t.brush);
		Brush_AddToList (t.brush, &active_brushes);
	} else
	{
		Select_Brush (t.brush, true);
	}

	Sys_UpdateWindows (W_ALL);
}


void Select_Delete (void)
{
	brush_t	*brush;

	selected_face = NULL;
	g_qeglobals.d_select_mode = sel_brush;

	g_qeglobals.d_select_count = 0;
	g_qeglobals.d_num_move_points = 0;
	while (selected_brushes.next != &selected_brushes)
	{
		brush = selected_brushes.next;
		Brush_Free (brush);
	}

	// FIXME: remove any entities with no brushes

	Sys_UpdateWindows (W_ALL);
}

// update the workzone to a given brush
void UpdateWorkzone_ForBrush( brush_t* b )
{
	int dim1, dim2;

	VectorCopy( b->mins, g_qeglobals.d_work_min );
	VectorCopy( b->maxs, g_qeglobals.d_work_max );

	// will update the workzone to the given brush
	dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
	dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;

	g_qeglobals.d_work_min[dim1] = b->mins[dim1];
	g_qeglobals.d_work_max[dim1] = b->maxs[dim1];
	g_qeglobals.d_work_min[dim2] = b->mins[dim2];
	g_qeglobals.d_work_max[dim2] = b->maxs[dim2];

}

void Select_Deselect (void)
{
	brush_t	*b;

	g_qeglobals.d_workcount++;
	g_qeglobals.d_select_count = 0;
	g_qeglobals.d_num_move_points = 0;
	b = selected_brushes.next;

	if (b == &selected_brushes)
	{
		if (selected_face)
		{
			selected_face = NULL;
			Sys_UpdateWindows (W_ALL);
		}
		return;
	}

	selected_face = NULL;
	g_qeglobals.d_select_mode = sel_brush;

	// grab top / bottom height for new brushes
//	if (b->mins[2] < b->maxs[2])
//	{
//		g_qeglobals.d_new_brush_bottom_z = b->mins[2];
//		g_qeglobals.d_new_brush_top_z = b->maxs[2];
//	}

	UpdateWorkzone_ForBrush(b);

	selected_brushes.next->prev = &active_brushes;
	selected_brushes.prev->next = active_brushes.next;
	active_brushes.next->prev = selected_brushes.prev;
	active_brushes.next = selected_brushes.next;
	selected_brushes.prev = selected_brushes.next = &selected_brushes;	

	Sys_UpdateWindows (W_ALL);
}

/*
============
Select_Move
============
*/
void Select_Move (vec3_t delta)
{
	brush_t	*b;

// actually move the selected brushes
	for (b = selected_brushes.next ; b != &selected_brushes ; b=b->next)
		Brush_Move (b, delta);
//	Sys_UpdateWindows (W_ALL);
}

/*
============
Select_Clone

Creates an exact duplicate of the selection in place, then moves
the selected brushes off of their old positions
============
*/
void Select_Clone (void)
{
	brush_t		*b, *b2, *n, *next, *next2;
	vec3_t		delta;
	entity_t	*e;

	g_qeglobals.d_workcount++;
	g_qeglobals.d_select_mode = sel_brush;

	delta[0] = g_qeglobals.d_gridsize;
	delta[1] = g_qeglobals.d_gridsize;
	delta[2] = 0;

	for (b=selected_brushes.next ; b != &selected_brushes ; b=next)
	{
		next = b->next;
		// if the brush is a world brush, handle simply
		if (b->owner == world_entity)
		{
			n = Brush_Clone (b);
			Brush_AddToList (n, &active_brushes);
			Entity_LinkBrush (world_entity, n);
			Brush_Build( n );
			Brush_Move (b, delta);
			continue;
		}

		e = Entity_Clone (b->owner);
		// clear the target / targetname
		DeleteKey (e, "target");
		DeleteKey (e, "targetname");

		// if the brush is a fixed size entity, create a new entity
		if (b->owner->eclass->fixedsize)
		{
			n = Brush_Clone (b);
			Brush_AddToList (n, &active_brushes);
			Entity_LinkBrush (e, n);
			Brush_Build( n );
			Brush_Move (b, delta);
			continue;
		}
        
		// brush is a complex entity, grab all the other ones now

		next = &selected_brushes;

		for ( b2 = b ; b2 != &selected_brushes ; b2=next2)
		{
			next2 = b2->next;
			if (b2->owner != b->owner)
			{
				if (next == &selected_brushes)
					next = b2;
				continue;
			}

			// move b2 to the start of selected_brushes,
			// so it won't be hit again
			Brush_RemoveFromList (b2);
			Brush_AddToList (b2, &selected_brushes);
			
			n = Brush_Clone (b2);
			Brush_AddToList (n, &active_brushes);
			Entity_LinkBrush (e, n);
			Brush_Build( n );
			Brush_Move (b2, delta);
		}

	}
	Sys_UpdateWindows (W_ALL);
}



/*
============
Select_SetTexture
============
*/
void Select_SetTexture (texdef_t *texdef)
{
	brush_t	*b;

	if (selected_face)
	{
		selected_face->texdef = *texdef;
		Brush_Build(selected_face_brush);
	}
	else
	{
		for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
			if (!b->owner->eclass->fixedsize)
				Brush_SetTexture (b, texdef);
	}
	Sys_UpdateWindows (W_ALL);
}


/*
================================================================

  TRANSFORMATIONS

================================================================
*/

void Select_GetBounds (vec3_t mins, vec3_t maxs)
{
	brush_t	*b;
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = 99999;
		maxs[i] = -99999;
	}

	for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
		for (i=0 ; i<3 ; i++)
		{
			if (b->mins[i] < mins[i])
				mins[i] = b->mins[i];
			if (b->maxs[i] > maxs[i])
				maxs[i] = b->maxs[i];
		}
}

void Select_GetTrueMid (vec3_t mid)
{
	vec3_t	mins, maxs;
	int		i;
	Select_GetBounds (mins, maxs);

  for (i=0 ; i<3 ; i++)
    mid[i] = (mins[i] + ((maxs[i] - mins[i]) / 2));
}

void Select_GetMid (vec3_t mid)
{
	vec3_t	mins, maxs;
	int		i;

	if (g_qeglobals.d_savedinfo.noclamp)
	{
		Select_GetTrueMid(mid);
		return;
	}

	Select_GetBounds (mins, maxs);
	for (i=0 ; i<3 ; i++)
		mid[i] = g_qeglobals.d_gridsize*floor ( ( (mins[i] + maxs[i])*0.5 )/g_qeglobals.d_gridsize );
}

vec3_t	select_origin;
vec3_t	select_matrix[3];
qboolean	select_fliporder;

void Select_AplyMatrix (void)
{
	brush_t	*b;
	face_t	*f;
	int		i, j;
	vec3_t	temp;

	for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
	{
		for (f=b->brush_faces ; f ; f=f->next)
		{
			for (i=0 ; i<3 ; i++)
			{
				VectorSubtract (f->planepts[i], select_origin, temp);
				for (j=0 ; j<3 ; j++)
					f->planepts[i][j] = DotProduct(temp, select_matrix[j])
						+ select_origin[j];
			}
			if (select_fliporder)
			{
				VectorCopy (f->planepts[0], temp);
				VectorCopy (f->planepts[2], f->planepts[0]);
				VectorCopy (temp, f->planepts[2]);
			}
		}
		Brush_Build( b );
	}
	Sys_UpdateWindows (W_ALL);
}


void Select_FlipAxis (int axis)
{
	int		i;

	Select_GetMid (select_origin);
	for (i=0 ; i<3 ; i++)
	{
		VectorCopy (vec3_origin, select_matrix[i]);
		select_matrix[i][i] = 1;
	}
	select_matrix[axis][axis] = -1;

	select_fliporder = true;
	Select_AplyMatrix ();
	Sys_UpdateWindows(W_ALL);
}

/*
================
Clamp
================
*/
void Clamp(float *f, int clamp)
{
  float frac = *f - (int)*f;
  *f = (int)*f % clamp;
  *f += frac;
}


void ProjectOnPlane(vec3_t normal,float dist,vec3_t ez, vec3_t p)
{
	if (fabs(ez[0]) == 1)
		p[0] = (dist - normal[1] * p[1] - normal[2] * p[2]) / normal[0];
	else if (fabs(ez[1]) == 1)
		p[1] = (dist - normal[0] * p[0] - normal[2] * p[2]) / normal[1];
	else
		p[2] = (dist - normal[0] * p[0] - normal[1] * p[1]) / normal[2];
}

void Back(vec3_t dir, vec3_t p)
{
	if (fabs(dir[0]) == 1)
		p[0] = 0;
	else if (fabs(dir[1]) == 1)
		p[1] = 0;
	else p[2] = 0;
}


// using scale[0] and scale[1]
void ComputeScale(vec3_t rex, vec3_t rey, vec3_t p, face_t *f)
{
	float px = DotProduct(rex, p);
	float py = DotProduct(rey, p);
  vec3_t aux;
	px *= f->texdef.scale[0];
	py *= f->texdef.scale[1];
  VectorCopy(rex, aux);
  VectorScale(aux, px, aux);
  VectorCopy(aux, p);
  VectorCopy(rey, aux);
  VectorScale(aux, py, aux);
  VectorAdd(p, aux, p);
}


void ComputeAbsolute(face_t *f, vec3_t p1, vec3_t p2, vec3_t p3)
{
	vec3_t ex,ey,ez;	        // local axis base
	vec3_t aux;
	vec3_t rex,rey;

  // compute first local axis base
  TextureAxisFromPlane(&f->plane, ex, ey);
  CrossProduct(ex, ey, ez);
	    
  VectorCopy(ex, aux);
  VectorScale(aux, -f->texdef.shift[0], aux);
  VectorCopy(aux, p1);
  VectorCopy(ey, aux);
  VectorScale(aux, -f->texdef.shift[1], aux);
  VectorAdd(p1, aux, p1);
  VectorCopy(p1, p2);
  VectorAdd(p2, ex, p2);
  VectorCopy(p1, p3);
  VectorAdd(p3, ey, p3);
  VectorCopy(ez, aux);
  VectorScale(aux, -f->texdef.rotate, aux);
  VectorRotate(p1, aux, p1);
  VectorRotate(p2, aux, p2);
  VectorRotate(p3, aux, p3);
	// computing rotated local axis base
  VectorCopy(ex, rex);
  VectorRotate(rex, aux, rex);
  VectorCopy(ey, rey);
  VectorRotate(rey, aux, rey);

  ComputeScale(rex,rey,p1,f);
	ComputeScale(rex,rey,p2,f);
	ComputeScale(rex,rey,p3,f);

	// project on normal plane
	// along ez 
	// assumes plane normal is normalized
	ProjectOnPlane(f->plane.normal,f->plane.dist,ez,p1);
	ProjectOnPlane(f->plane.normal,f->plane.dist,ez,p2);
	ProjectOnPlane(f->plane.normal,f->plane.dist,ez,p3);
}


void AbsoluteToLocal(plane_t normal2, face_t *f, vec3_t p1, vec3_t p2, vec3_t p3)
{
	vec3_t	ex,ey,ez;
	vec3_t	aux;
	vec3_t	rex,rey;
	vec_t	x;
	vec_t	y;
	
	// computing new local axis base
	TextureAxisFromPlane(&normal2, ex, ey);
	CrossProduct(ex, ey, ez);
	
	// projecting back on (ex,ey)
	Back(ez,p1);
	Back(ez,p2);
	Back(ez,p3);
	
	// rotation
	VectorCopy(p2, aux);
	VectorSubtract(aux, p1,aux);
	
	x = DotProduct(aux,ex);
	y = DotProduct(aux,ey);
	f->texdef.rotate = 180 * atan2(y,x) / Q_PI;
	
	// computing rotated local axis base
	VectorCopy(ez, aux);
	VectorScale(aux, f->texdef.rotate, aux);
	VectorCopy(ex, rex);
	VectorRotate(rex, aux, rex);
	VectorCopy(ey, rey);
	VectorRotate(rey, aux, rey);
	
	// scale
	VectorCopy(p2, aux);
	VectorSubtract(aux, p1, aux);
	f->texdef.scale[0] = DotProduct(aux, rex);
	VectorCopy(p3, aux);
	VectorSubtract(aux, p1, aux);
	f->texdef.scale[1] = DotProduct(aux, rey);
	
	// shift
	// only using p1
	x = DotProduct(rex,p1);
	y = DotProduct(rey,p1);
	x /= f->texdef.scale[0];
	y /= f->texdef.scale[1];
	
	VectorCopy(rex, p1);
	VectorScale(p1, x, p1);
	VectorCopy(rey, aux);
	VectorScale(aux, y, aux);
	VectorAdd(p1, aux, p1);
	VectorCopy(ez, aux);
	VectorScale(aux, -f->texdef.rotate, aux);
	VectorRotate(p1, aux, p1);
	f->texdef.shift[0] = -DotProduct(p1, ex);
	f->texdef.shift[1] = -DotProduct(p1, ey);
	
	// stored rot is good considering local axis base
	// change it if necessary
	f->texdef.rotate = -f->texdef.rotate;
	
	Clamp(&f->texdef.shift[0], f->d_texture->width);
	Clamp(&f->texdef.shift[1], f->d_texture->height);
	Clamp(&f->texdef.rotate, 360);
}


void RotateFaceTexture(face_t* f, int axis, float deg)
{
	vec3_t	p1,p2,p3, rota;   
	plane_t	normal2;
	vec3_t	normal;

	p1[0] = p1[1] = p1[2] = 0;
	VectorCopy(p1, p2);
	VectorCopy(p1, p3);
	VectorCopy(p1, rota);
	ComputeAbsolute(f, p1, p2, p3);
  
	rota[axis] = deg;
	VectorRotate2(p1, rota, select_origin, p1);
	VectorRotate2(p2, rota, select_origin, p2);
	VectorRotate2(p3, rota, select_origin, p3);

	normal[0] = f->plane.normal[0];
	normal[1] = f->plane.normal[1];
	normal[2] = f->plane.normal[2];
	VectorRotate(normal, rota, normal);
	normal2.normal[0] = normal[0];
	normal2.normal[1] = normal[1];
	normal2.normal[2] = normal[2];
	AbsoluteToLocal(normal2, f, p1, p2 ,p3);
}


void RotateTextures(int axis, float deg, vec3_t origin)
{
	brush_t *b;
	face_t *f;

	for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
	{
		for (f=b->brush_faces ; f ; f=f->next)
		{
			RotateFaceTexture(f, axis, deg);
			Brush_Build(b);
		}
		Brush_Build(b);
	}
}


void Select_RotateAxis (int axis, float deg)
{
/*
	vec3_t	temp;
	int		i, j;
	vec_t	c, s;

	if (deg == 0)
		return;

	Select_GetMid (select_origin);
	select_fliporder = false;

	if (deg == 90)
	{
		for (i=0 ; i<3 ; i++)
		{
			VectorCopy (vec3_origin, select_matrix[i]);
			select_matrix[i][i] = 1;
		}
		i = (axis+1)%3;
		j = (axis+2)%3;
		VectorCopy (select_matrix[i], temp);
		VectorCopy (select_matrix[j], select_matrix[i]);
		VectorSubtract (vec3_origin, temp, select_matrix[j]);
	}
	else
	{
		deg = -deg;
		if (deg == -180)
		{
			c = -1;
			s = 0;
		}
		else if (deg == -270)
		{
			c = 0;
			s = -1;
		}
		else
		{
			c = cos(deg * Q_PI / 180.0);
			s = sin(deg * Q_PI / 180.0);
		}

		for (i=0 ; i<3 ; i++)
		{
			VectorCopy (vec3_origin, select_matrix[i]);
			select_matrix[i][i] = 1;
		}

		switch (axis)
		{
		case 0:
			select_matrix[1][1] = c;
			select_matrix[1][2] = -s;
			select_matrix[2][1] = s;
			select_matrix[2][2] = c;
			break;
		case 1:
			select_matrix[0][0] = c;
			select_matrix[0][2] = s;
			select_matrix[2][0] = -s;
			select_matrix[2][2] = c;
			break;
		case 2:
			select_matrix[0][0] = c;
			select_matrix[0][1] = -s;
			select_matrix[1][0] = s;
			select_matrix[1][1] = c;
			break;
		}
	}
*/
	int		i;
	vec_t	c, s;

	while(deg >= 360)
		deg -= 360;
	while(deg < 0)
		deg += 360;

	if (deg == 0)
		return;

	Select_GetMid (select_origin);
	select_fliporder = false;
	
	deg = -deg;
	c = cos(deg * Q_PI / 180.0);
	s = sin(deg * Q_PI / 180.0);

	for (i=0 ; i<3 ; i++)
	{
		VectorCopy (vec3_origin, select_matrix[i]);
		select_matrix[i][i] = 1;
	}
	
	switch (axis)
	{
	case 0:
		select_matrix[1][1] = c;
		select_matrix[1][2] = -s;
		select_matrix[2][1] = s;
		select_matrix[2][2] = c;
			break;
	case 1:
		select_matrix[0][0] = c;
		select_matrix[0][2] = s;
		select_matrix[2][0] = -s;
		select_matrix[2][2] = c;
		break;
	case 2:
		select_matrix[0][0] = c;
		select_matrix[0][1] = -s;
		select_matrix[1][0] = s;
		select_matrix[1][1] = c;
		break;
	}

	if(g_qeglobals.d_textures_lock)
		RotateTextures(axis, deg, select_origin);

	Select_AplyMatrix ();
	Sys_UpdateWindows(W_ALL);
}


void Select_FitTexture(int height, int width)
{
	brush_t		*b;
	
	if(selected_brushes.next == &selected_brushes && !selected_face)
		return;
	
	for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
	{
		Brush_FitTexture(b, height, width);
		Brush_Build(b);
	}

	
	if (selected_face)
	{
		Face_FitTexture(selected_face, height, width);
		Brush_Build(selected_face_brush);
	}
	
	Sys_UpdateWindows (W_CAMERA);
}


/*
================================================================

GROUP SELECTIONS

================================================================
*/

void Select_CompleteTall (void)
{
	brush_t	*b, *next;
//	int		i;
	vec3_t	mins, maxs;
	int dim1, dim2;

	if (!QE_SingleBrush ())
		return;

	g_qeglobals.d_select_mode = sel_brush;

	VectorCopy (selected_brushes.next->mins, mins);
	VectorCopy (selected_brushes.next->maxs, maxs);
	Select_Delete ();

	dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
	dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;

	for (b=active_brushes.next ; b != &active_brushes ; b=next)
	{
		next = b->next;

		if ( (b->maxs[dim1] > maxs[dim1] || b->mins[dim1] < mins[dim1]) 
			|| (b->maxs[dim2] > maxs[dim2] || b->mins[dim2] < mins[dim2]) )
			continue;

	 	if (FilterBrush (b))
	 		continue;

		Brush_RemoveFromList (b);
		Brush_AddToList (b, &selected_brushes);
/*
		// old stuff
		for (i=0 ; i<2 ; i++)
			if (b->maxs[i] > maxs[i] || b->mins[i] < mins[i])
				break;
		if (i == 2)
		{
			Brush_RemoveFromList (b);
			Brush_AddToList (b, &selected_brushes);
		}
*/
	}
	Sys_UpdateWindows (W_ALL);
}

void Select_PartialTall (void)
{
	brush_t	*b, *next;
//	int		i;
	vec3_t	mins, maxs;
	int dim1, dim2;

	if (!QE_SingleBrush ())
		return;

	g_qeglobals.d_select_mode = sel_brush;

	VectorCopy (selected_brushes.next->mins, mins);
	VectorCopy (selected_brushes.next->maxs, maxs);
	Select_Delete ();

	dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
	dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;

	for (b=active_brushes.next ; b != &active_brushes ; b=next)
	{
		next = b->next;

		if ( (b->mins[dim1] > maxs[dim1] || b->maxs[dim1] < mins[dim1]) 
			|| (b->mins[dim2] > maxs[dim2] || b->maxs[dim2] < mins[dim2]) )
			continue;

	 	if (FilterBrush (b))
	 		continue;

		Brush_RemoveFromList (b);
		Brush_AddToList (b, &selected_brushes);
/*
		// old stuff
		for (i=0 ; i<2 ; i++)
			if (b->mins[i] > maxs[i] || b->maxs[i] < mins[i])
				break;
		if (i == 2)
		{
			Brush_RemoveFromList (b);
			Brush_AddToList (b, &selected_brushes);
		}
*/
	}
	Sys_UpdateWindows (W_ALL);
}

void Select_Touching (void)
{
	brush_t	*b, *next;
	int		i;
	vec3_t	mins, maxs;

	if (!QE_SingleBrush ())
		return;

	g_qeglobals.d_select_mode = sel_brush;

	VectorCopy (selected_brushes.next->mins, mins);
	VectorCopy (selected_brushes.next->maxs, maxs);

	for (b=active_brushes.next ; b != &active_brushes ; b=next)
	{
		next = b->next;
		for (i=0 ; i<3 ; i++)
			if (b->mins[i] > maxs[i]+1 || b->maxs[i] < mins[i]-1)
				break;
		if (i == 3)
		{
			Brush_RemoveFromList (b);
			Brush_AddToList (b, &selected_brushes);
		}
	}
	Sys_UpdateWindows (W_ALL);
}

void Select_Inside (void)
{
	brush_t	*b, *next;
	int		i;
	vec3_t	mins, maxs;

	if (!QE_SingleBrush ())
		return;

	g_qeglobals.d_select_mode = sel_brush;

	VectorCopy (selected_brushes.next->mins, mins);
	VectorCopy (selected_brushes.next->maxs, maxs);
	Select_Delete ();

	for (b=active_brushes.next ; b != &active_brushes ; b=next)
	{
		next = b->next;
		for (i=0 ; i<3 ; i++)
			if (b->maxs[i] > maxs[i] || b->mins[i] < mins[i])
				break;
		if (i == 3)
		{
			Brush_RemoveFromList (b);
			Brush_AddToList (b, &selected_brushes);
		}
	}
	Sys_UpdateWindows (W_ALL);
}

/*
=============
Select_Ungroup

Turn the currently selected entity back into normal brushes
=============
*/
void  Select_Ungroup (void)
{
	entity_t	*e;
	brush_t		*b;

	e = selected_brushes.next->owner;

	if (!e || e == world_entity || e->eclass->fixedsize)
	{
		Sys_Printf ("Not a grouped entity.\n");
		return;
	}

	for (b=e->brushes.onext ; b != &e->brushes ; b=e->brushes.onext)
	{
		Brush_RemoveFromList (b);
		Brush_AddToList (b, &active_brushes);
		Entity_UnlinkBrush (b);
		Entity_LinkBrush (world_entity, b);
		Brush_Build( b );
		b->owner = world_entity;
	}

	Entity_Free (e);
	Sys_UpdateWindows (W_ALL);
}


/*
============
Select_Invert
============
*/
void Select_Invert (void)
{
	brush_t *next, *prev;

	Sys_Printf("Inverting selection...\n");

	next = active_brushes.next;
	prev = active_brushes.prev;
	if (selected_brushes.next != &selected_brushes)
	{
		active_brushes.next = selected_brushes.next;
		active_brushes.prev = selected_brushes.prev;
		active_brushes.next->prev = &active_brushes;
		active_brushes.prev->next = &active_brushes;
	}
	else
	{
		active_brushes.next = &active_brushes;
		active_brushes.prev = &active_brushes;
	}
	if (next != &active_brushes)
	{
		selected_brushes.next = next;
		selected_brushes.prev = prev;
		selected_brushes.next->prev = &selected_brushes;
		selected_brushes.prev->next = &selected_brushes;
	}
	else
	{
		selected_brushes.next = &selected_brushes;
		selected_brushes.prev = &selected_brushes;
	}

	Sys_UpdateWindows(W_ALL);

	Sys_Printf("Done.\n");
}

void Select_Hide (void)
{
	brush_t *b;

	for (b=selected_brushes.next ; b && b != &selected_brushes ; b=b->next)
		b->hiddenbrush = true;

	Sys_UpdateWindows (W_ALL);
}

void Select_ShowAllHidden (void)
{
	brush_t *b;

	for (b=selected_brushes.next ; b && b != &selected_brushes ; b=b->next)
		b->hiddenbrush = false;

	for (b=active_brushes.next ; b && b != &active_brushes ; b=b->next)
		b->hiddenbrush = false;

	Sys_UpdateWindows (W_ALL);
}

void FindReplaceTextures (char *find, char *replace, qboolean selected, qboolean force)
{
	brush_t		*list;
	brush_t		*brush;
	face_t		*face;

	list = (selected) ? &selected_brushes : &active_brushes;
	if (!selected)
		Select_Deselect();

	for (brush = list->next ; brush != list; brush = brush->next)
	{
		
		for (face = brush->brush_faces; face; face = face->next)
		{
			if(force || strcmpi(face->texdef.name, find) == 0)
			{
				face->d_texture = Texture_ForName( face->texdef.name );
				strcpy(face->texdef.name, replace);
			}
		}
		Brush_Build(brush);
	}
	Sys_UpdateWindows (W_CAMERA);
}


void GroupSelectNextBrush (void)
{
	brush_t		*b;
	brush_t		*b2;
	entity_t	*e;

	// check to see if the selected brush is part of a func group
	// if it is, deselect everything and reselect the next brush 
	// in the group
	b = selected_brushes.next;
	if (b != &selected_brushes)
	{
		if (strcmpi(b->owner->eclass->name, "worldspawn") != 0)
		{
			e = b->owner;
			Select_Deselect();
			for (b2 = e->brushes.onext ; b2 != &e->brushes ; b2 = b2->onext)
			{
				if (b == b2)
				{
					b2 = b2->onext;
					break;
				}
			}
			if (b2 == &e->brushes)
			b2 = b2->onext;

			Select_Brush(b2, false);
			Sys_UpdateWindows(W_ALL);
		}
	}
}

