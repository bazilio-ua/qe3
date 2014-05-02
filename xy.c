// xy.c

#include "qe3.h"


#define	PAGEFLIPS	2

const char *dimstrings[] = {"x:%.f", "y:%.f", "z:%.f"};
const char *orgstrings[] = {"(x:%.f  y:%.f)", "(x:%.f  z:%.f)", "(y:%.f  z:%.f)"};

clippoint_t	clip1;
clippoint_t	clip2;
clippoint_t	clip3;
clippoint_t	*movingclip;

/*
============
XY_Init
============
*/
void XY_Init (void)
{
	g_qeglobals.d_xyz.origin[0] = 0;
	g_qeglobals.d_xyz.origin[1] = 20;
	g_qeglobals.d_xyz.origin[2] = 46;

	g_qeglobals.d_xyz.scale = 1;
}

void PositionView (void)
{
	int			dim1, dim2;
	brush_t		*b;

	dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
	dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;

	b = selected_brushes.next;
	if (b && b->next != b)
	{
	  g_qeglobals.d_xyz.origin[dim1] = b->mins[dim1];
	  g_qeglobals.d_xyz.origin[dim2] = b->mins[dim2];
	}
	else
	{
	  g_qeglobals.d_xyz.origin[dim1] = g_qeglobals.d_camera.origin[dim1];
	  g_qeglobals.d_xyz.origin[dim2] = g_qeglobals.d_camera.origin[dim2];
	}
}

float Betwixt (float f1, float f2)
{
  if (f1 > f2)
    return f2 + ((f1 - f2) / 2);
  else
    return f1 + ((f2 - f1) / 2);
}

float fDiff (float f1, float f2)
{
  if (f1 > f2)
    return f1 - f2;
  else
    return f2 - f1;
}

void VectorCopyXY (vec3_t in, vec3_t out)
{
  if (g_qeglobals.d_viewtype == XY)
  {
    out[0] = in[0];
    out[1] = in[1];
  }
  else
  if (g_qeglobals.d_viewtype == XZ)
  {
    out[0] = in[0];
    out[2] = in[2];
  }
  else
  {
    out[1] = in[1];
    out[2] = in[2];
  }
}


/*
============================================================================
  CLIPPOINT
============================================================================
*/
void SetClipMode (void)
{
	g_qeglobals.d_clipmode ^= 1;
	CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_CLIPPER, MF_BYCOMMAND | (g_qeglobals.d_clipmode ? MF_CHECKED : MF_UNCHECKED)  );

	ResetClipMode ();
	Sys_UpdateGridStatusBar ();
}

void UnsetClipMode (void)
{
	g_qeglobals.d_clipmode = false;
	CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_CLIPPER, MF_BYCOMMAND | (g_qeglobals.d_clipmode ? MF_CHECKED : MF_UNCHECKED)  );

	ResetClipMode ();
	Sys_UpdateGridStatusBar ();
}

void ResetClipMode (void)
{
  if (g_qeglobals.d_clipmode)
  {
	clip1.ptclip[0] = clip1.ptclip[1] = clip1.ptclip[2] = 0.0;
	clip1.set = false;
	clip2.ptclip[0] = clip2.ptclip[1] = clip2.ptclip[2] = 0.0;
	clip2.set = false;
	clip3.ptclip[0] = clip3.ptclip[1] = clip3.ptclip[2] = 0.0;
	clip3.set = false;

    CleanList(&g_qeglobals.d_frontsplits);
    CleanList(&g_qeglobals.d_backsplits);

    g_qeglobals.d_frontsplits.next = &g_qeglobals.d_frontsplits;
    g_qeglobals.d_backsplits.next = &g_qeglobals.d_backsplits;
  }
  else
  {
    if (movingclip)
    {
      ReleaseCapture();
      movingclip = NULL;
    }
    CleanList(&g_qeglobals.d_frontsplits);
    CleanList(&g_qeglobals.d_backsplits);

    g_qeglobals.d_frontsplits.next = &g_qeglobals.d_frontsplits;
    g_qeglobals.d_backsplits.next = &g_qeglobals.d_backsplits;

	Sys_UpdateWindows (W_ALL);
  }
}


void CleanList (brush_t *list)
{
	brush_t		*brush;
	brush_t		*next;

	brush = list->next; 
	while (brush != NULL && brush != list)
	{
		next = brush->next;
		Brush_Free(brush);
		brush = next;
	}
}

void ProduceSplitLists (void)
{
	brush_t		*brush;
	brush_t		*front;
	brush_t		*back;
	face_t		face;

  CleanList (&g_qeglobals.d_frontsplits);
  CleanList (&g_qeglobals.d_backsplits);

  g_qeglobals.d_frontsplits.next = &g_qeglobals.d_frontsplits;
  g_qeglobals.d_backsplits.next = &g_qeglobals.d_backsplits;

  for (brush = selected_brushes.next ; brush != NULL && brush != &selected_brushes ; brush=brush->next)
  {
    front = NULL;
    back = NULL;
    
      if (clip1.set && clip2.set)
      {
        
        VectorCopy (clip1.ptclip, face.planepts[0]);
        VectorCopy (clip2.ptclip, face.planepts[1]);
        VectorCopy (clip3.ptclip, face.planepts[2]);
        if (clip3.set == false)
        {
          if (g_qeglobals.d_viewtype == XY)
          {
            face.planepts[0][2] = brush->mins[2];
            face.planepts[1][2] = brush->mins[2];
            face.planepts[2][0] = Betwixt (clip1.ptclip[0], clip2.ptclip[0]);
            face.planepts[2][1] = Betwixt (clip1.ptclip[1], clip2.ptclip[1]);
            face.planepts[2][2] = brush->maxs[2];
          }
          else if (g_qeglobals.d_viewtype == YZ)
          {
            face.planepts[0][0] = brush->mins[0];
            face.planepts[1][0] = brush->mins[0];
            face.planepts[2][1] = Betwixt (clip1.ptclip[1], clip2.ptclip[1]);
            face.planepts[2][2] = Betwixt (clip1.ptclip[2], clip2.ptclip[2]);
            face.planepts[2][0] = brush->maxs[0];
          }
          else
          {
            face.planepts[0][1] = brush->mins[1];
            face.planepts[1][1] = brush->mins[1];
            face.planepts[2][0] = Betwixt (clip1.ptclip[0], clip2.ptclip[0]);
            face.planepts[2][2] = Betwixt (clip1.ptclip[2], clip2.ptclip[2]);
            face.planepts[2][1] = brush->maxs[1];
          }
        }
        CSG_SplitBrushByFace (brush, &face, &front, &back);
        if (back)
          Brush_AddToList(back, &g_qeglobals.d_backsplits);
        if (front)
          Brush_AddToList(front, &g_qeglobals.d_frontsplits);
      }

  }
}

void Brush_CopyList (brush_t *from, brush_t *to)
{
	brush_t *brush;
	brush_t *next;

	brush = from->next;

	while (brush != NULL && brush != from)
	{
		next = brush->next;
		Brush_RemoveFromList(brush);
		Brush_AddToList(brush, to);
		brush = next;
	}
}


brush_t hold_brushes;
void Clip (void)
{
	brush_t		*list;

  if (g_qeglobals.d_clipmode)
  {
    hold_brushes.next = &hold_brushes;
    ProduceSplitLists();

	list = ( (g_qeglobals.d_viewtype == XZ) ? !g_qeglobals.d_clipswitch : g_qeglobals.d_clipswitch) ? &g_qeglobals.d_frontsplits : &g_qeglobals.d_backsplits;
    
	if (list->next != list)
    {
      Brush_CopyList(list, &hold_brushes);
      CleanList(&g_qeglobals.d_frontsplits);
      CleanList(&g_qeglobals.d_backsplits);
      Select_Delete();
      Brush_CopyList(&hold_brushes, &selected_brushes);
	}
	ResetClipMode ();
	Sys_UpdateWindows(W_ALL);
  }
}


void SplitClip (void)
{
  if (g_qeglobals.d_clipmode)
  {

	ProduceSplitLists();
	if ((g_qeglobals.d_frontsplits.next != &g_qeglobals.d_frontsplits) &&
		(g_qeglobals.d_backsplits.next != &g_qeglobals.d_backsplits))
	{
		Select_Delete();
		Brush_CopyList(&g_qeglobals.d_frontsplits, &selected_brushes);
		Brush_CopyList(&g_qeglobals.d_backsplits, &selected_brushes);
		CleanList(&g_qeglobals.d_frontsplits);
		CleanList(&g_qeglobals.d_backsplits);
	}
	ResetClipMode ();
	Sys_UpdateWindows (W_ALL);
  }
}

void FlipClip (void)
{
  if (g_qeglobals.d_clipmode)
  {
	g_qeglobals.d_clipswitch = !g_qeglobals.d_clipswitch;
	Sys_UpdateWindows (W_ALL);
  }
}


/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

static	int	cursorx, cursory;
static	int	buttonstate;
static	int	pressx, pressy;
static	vec3_t	pressdelta;
static	qboolean	press_selection;

void XY_ToPoint (int x, int y, vec3_t point)
{
	if (g_qeglobals.d_viewtype == XY)
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[1] = g_qeglobals.d_xyz.origin[1] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
		point[2] = 0;
	}
	else if (g_qeglobals.d_viewtype == YZ)
	{
		point[0] = 0;
		point[1] = g_qeglobals.d_xyz.origin[1] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
	}
	else
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[1] = 0;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
	}
}

void XY_ToGridPoint (int x, int y, vec3_t point)
{
	if (g_qeglobals.d_viewtype == XY)
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[1] = g_qeglobals.d_xyz.origin[1] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
		point[2] = 0;

		point[0] = floor(point[0]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
		point[1] = floor(point[1]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
	}
	else if (g_qeglobals.d_viewtype == YZ)
	{
		point[0] = 0;
		point[1] = g_qeglobals.d_xyz.origin[1] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
		
		point[1] = floor(point[1]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
		point[2] = floor(point[2]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;

	}
	else
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[1] = 0;
		point[2] = g_qeglobals.d_xyz.origin[2] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
		
		point[0] = floor(point[0]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
		point[2] = floor(point[2]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
	}
}

void SnapToPoint (int x, int y, vec3_t point)
{
  if (g_qeglobals.d_savedinfo.noclamp)
  {
    XY_ToPoint (x, y, point);
  }
  else
  {
    XY_ToGridPoint (x, y, point);
  }
}


/*
==============
XY_MouseDown
==============
*/
void XY_MouseDown (int x, int y, int buttons)
{
	vec3_t	point;
	vec3_t	origin, dir, right, up;

	int n1, n2;
	int angle;

	buttonstate = buttons;

	// clipper
	if ((buttonstate & MK_LBUTTON) && g_qeglobals.d_clipmode)
	{
		DropClipPoint (x, y);
		Sys_UpdateWindows (W_ALL);
	}
	else
	{
		pressx = x;
		pressy = y;
	
		VectorCopy (vec3_origin, pressdelta);
	
		XY_ToPoint (x, y, point);
	
		VectorCopy (point, origin);
	
		dir[0] = 0; dir[1] = 0; dir[2] = 0;
		if (g_qeglobals.d_viewtype == XY)
		{
			origin[2] = 8192;
			dir[2] = -1;
	
			right[0] = 1/g_qeglobals.d_xyz.scale;
			right[1] = 0;
			right[2] = 0;
	
			up[0] = 0;
			up[1] = 1/g_qeglobals.d_xyz.scale;
			up[2] = 0;
		}
		else if (g_qeglobals.d_viewtype == YZ)
		{
			origin[0] = 8192;
			dir[0] = -1;
	
			right[0] = 0;
			right[1] = 1/g_qeglobals.d_xyz.scale;
			right[2] = 0; 
			
			up[0] = 0; 
			up[1] = 0;
			up[2] = 1/g_qeglobals.d_xyz.scale;
		}
		else
		{
			origin[1] = 8192;
			dir[1] = -1;
	
			right[0] = 1/g_qeglobals.d_xyz.scale;
			right[1] = 0;
			right[2] = 0; 
	
			up[0] = 0; 
			up[1] = 0;
			up[2] = 1/g_qeglobals.d_xyz.scale;
		}
	
		press_selection = (selected_brushes.next != &selected_brushes);
	
		Sys_GetCursorPos (&cursorx, &cursory);
	
		// lbutton = manipulate selection
		// shift-LBUTTON = select
		if ( (buttonstate == MK_LBUTTON)
			|| (buttonstate == (MK_LBUTTON | MK_SHIFT))
			|| (buttonstate == (MK_LBUTTON | MK_CONTROL))
			|| (buttonstate == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) )
//		if (buttonstate & MK_LBUTTON)
		{
			Drag_Begin (x, y, buttons, 
				right, up,
				origin, dir);
			return;
		}
	
		// control mbutton = move camera
		if (buttonstate == (MK_CONTROL|MK_MBUTTON) )
		{	
	//		g_qeglobals.d_camera.origin[0] = point[0];
	//		g_qeglobals.d_camera.origin[1] = point[1];
			VectorCopyXY(point, g_qeglobals.d_camera.origin);
	
			Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z);
		}
	
		// mbutton = angle camera
		if (buttonstate == MK_MBUTTON)
		{	
			VectorSubtract (point, g_qeglobals.d_camera.origin, point);
	
			n1 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;
			n2 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
			angle = (g_qeglobals.d_viewtype == XY) ? YAW : PITCH;
	
			if (point[n1] || point[n2])
			{
				g_qeglobals.d_camera.angles[angle] = 180/Q_PI*atan2 (point[n1], point[n2]);
				Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z);
			}
		}
	
		// shift mbutton = move z checker
		if (buttonstate == (MK_SHIFT|MK_MBUTTON) )
		{
	//		XY_ToPoint (x, y, point);
			SnapToPoint (x, y, point);
	
			if (g_qeglobals.d_viewtype == XY)
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[1];
			}
			else if (g_qeglobals.d_viewtype == YZ)
			{
				g_qeglobals.d_z.origin[0] = point[1];
				g_qeglobals.d_z.origin[1] = point[2];
			}
			else
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[2];
			}
			Sys_UpdateWindows (W_XY_OVERLAY|W_Z);
			return;
		}
	}
}


/*
==============
XY_MouseUp
==============
*/
void XY_MouseUp (int x, int y, int buttons)
{
	// clipper
	if (g_qeglobals.d_clipmode)
		EndClipPoint ();
	else
	{
		Drag_MouseUp ();
		
		if (!press_selection)
			Sys_UpdateWindows (W_ALL);
	}

	buttonstate = 0;
}

//DragDelta
qboolean DragDelta (int x, int y, vec3_t move)
{
	vec3_t	xvec, yvec, delta;
	int		i;

	xvec[0] = 1/g_qeglobals.d_xyz.scale;
	xvec[1] = xvec[2] = 0;
	yvec[1] = 1/g_qeglobals.d_xyz.scale;
	yvec[0] = yvec[2] = 0;

	for (i=0 ; i<3 ; i++)
	{
		delta[i] = xvec[i]*(x - pressx) + yvec[i]*(y - pressy);

		if (!g_qeglobals.d_savedinfo.noclamp)
		{
			delta[i] = floor(delta[i]/g_qeglobals.d_gridsize+ 0.5) * g_qeglobals.d_gridsize;		
		}
	}
	VectorSubtract (delta, pressdelta, move);
	VectorCopy (delta, pressdelta);

	if (move[0] || move[1] || move[2])
		return true;
	return false;
}

/*
==============
NewBrushDrag
==============
*/
void NewBrushDrag (int x, int y)
{
	vec3_t	mins, maxs, junk;
	int		i;
	float	temp;
	brush_t	*n;
	int dim;

	if (!DragDelta (x,y, junk))
		return;

	// delete the current selection
	if (selected_brushes.next != &selected_brushes)
		Brush_Free (selected_brushes.next);

//	XY_ToGridPoint (pressx, pressy, mins);
	SnapToPoint (pressx, pressy, mins);

	dim = (g_qeglobals.d_viewtype == XY) ? 2 : (g_qeglobals.d_viewtype == YZ) ? 0 : 1;

//	mins[2] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_bottom_z/g_qeglobals.d_gridsize));
	mins[dim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_work_min[dim]/g_qeglobals.d_gridsize));

//	XY_ToGridPoint (x, y, maxs);
	SnapToPoint (x, y, maxs);

//	maxs[2] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_top_z/g_qeglobals.d_gridsize));
	maxs[dim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_work_max[dim]/g_qeglobals.d_gridsize));

//	if (maxs[2] <= mins[2])
//		maxs[2] = mins[2] + g_qeglobals.d_gridsize;

	if (maxs[dim] <= mins[dim])
		maxs[dim] = mins[dim] + g_qeglobals.d_gridsize;

	for (i=0 ; i<3 ; i++)
	{
		if (mins[i] == maxs[i])
			return;	// don't create a degenerate brush
		if (mins[i] > maxs[i])
		{
			temp = mins[i];
			mins[i] = maxs[i];
			maxs[i] = temp;
		}
	}

	n = Brush_Create (mins, maxs, &g_qeglobals.d_texturewin.texdef);
	if (!n)
		return;

	Brush_AddToList (n, &selected_brushes);

	Entity_LinkBrush (world_entity, n);

	Brush_Build( n );

//	Sys_UpdateWindows (W_ALL);
	Sys_UpdateWindows (W_XY| W_CAMERA);
}


/*
==============
XY_MouseMoved
==============
*/
void XY_MouseMoved (int x, int y, int buttons)
{
	vec3_t	point;

    int n1, n2;
    int angle;
	int dim1, dim2;

	vec3_t	tdp;
	char	xystring[256];

	if (!buttonstate || buttonstate ^ MK_RBUTTON)
	{
		SnapToPoint (x, y, tdp);
		sprintf (xystring, "xyz Coordinates: (%i %i %i)", (int)tdp[0], (int)tdp[1], (int)tdp[2]);
		Sys_Status (xystring, 0);
	}

	// clipper
	if ((!buttonstate || buttonstate & MK_LBUTTON) && g_qeglobals.d_clipmode)
	{
		MoveClipPoint (x, y);
		Sys_UpdateWindows (W_ALL);
	}
	else
	{
		if (!buttonstate)
			return;
	
		// lbutton without selection = drag new brush
		if (buttonstate == MK_LBUTTON && !press_selection)
		{
			NewBrushDrag (x, y);
			return;
		}
	
		// lbutton (possibly with control and or shift)
		// with selection = drag selection
		if (buttonstate & MK_LBUTTON)
		{
			Drag_MouseMoved (x, y, buttons);
			Sys_UpdateWindows (W_XY_OVERLAY | W_CAMERA | W_Z);
			return;
		}
	
		// control mbutton = move camera
		if (buttonstate == (MK_CONTROL|MK_MBUTTON) )
		{
	//		XY_ToPoint (x, y, point);
	//		g_qeglobals.d_camera.origin[0] = point[0];
	//		g_qeglobals.d_camera.origin[1] = point[1];
	
			SnapToPoint (x, y, point);
			VectorCopyXY(point, g_qeglobals.d_camera.origin);
	
			Sys_UpdateWindows (W_XY_OVERLAY|W_CAMERA|W_Z);
			return;
		}
	
		// shift mbutton = move z checker
		if (buttonstate == (MK_SHIFT|MK_MBUTTON) )
		{
	
	//		XY_ToPoint (x, y, point);
			SnapToPoint (x, y, point);
	
			if (g_qeglobals.d_viewtype == XY)
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[1];
			}
			else if (g_qeglobals.d_viewtype == YZ)
			{
				g_qeglobals.d_z.origin[0] = point[1];
				g_qeglobals.d_z.origin[1] = point[2];
			}
			else
			{
				g_qeglobals.d_z.origin[0] = point[0];
				g_qeglobals.d_z.origin[1] = point[2];
			}
				Sys_UpdateWindows (W_XY_OVERLAY|W_Z);
				return;
		}
		// mbutton = angle camera
		if (buttonstate == MK_MBUTTON )
		{	
	//		XY_ToPoint (x, y, point);
			SnapToPoint (x, y, point);
			VectorSubtract (point, g_qeglobals.d_camera.origin, point);
	
			n1 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;
			n2 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
			angle = (g_qeglobals.d_viewtype == XY) ? YAW : PITCH;
	
			if (point[n1] || point[n2])
			{
				g_qeglobals.d_camera.angles[angle] = 180/Q_PI*atan2 (point[n1], point[n2]);
				Sys_UpdateWindows (W_XY_OVERLAY | W_CAMERA);
			}
			return;
		}
	
		// rbutton = drag xy origin
		if (buttonstate == MK_RBUTTON)
		{
			SetCursor(NULL);
			Sys_GetCursorPos (&x, &y);
			if (x != cursorx || y != cursory)
			{
				dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
				dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;
	
				g_qeglobals.d_xyz.origin[dim1] -= (x-cursorx)/g_qeglobals.d_xyz.scale;
				g_qeglobals.d_xyz.origin[dim2] += (y-cursory)/g_qeglobals.d_xyz.scale;
	
				Sys_SetCursorPos (cursorx, cursory);
				Sys_UpdateWindows (W_XY | W_XY_OVERLAY| W_Z);
	
				sprintf (xystring, "xyz Origin: (%i %i %i)", (int)g_qeglobals.d_xyz.origin[0], (int)g_qeglobals.d_xyz.origin[1], (int)g_qeglobals.d_xyz.origin[2]);
				Sys_Status (xystring, 0);
			}
			return;
		}
	}
}


/*
============================================================================

CLIPPOINT

============================================================================
*/


/*
==============
DropClipPoint
==============
*/
void DropClipPoint (int x, int y)
{
	clippoint_t *pt;
	int viewtype;
	int dim;

	if (movingclip)
	{
		SnapToPoint (x, y, movingclip->ptclip);
	}
	else
	{
		pt = NULL;
		if (clip1.set == false)
		{
			pt = &clip1;
			clip1.set = true;
		}
		else 
		if (clip2.set == false)
		{
			pt = &clip2;
			clip2.set = true;
		}
		else 
		if (clip3.set == false)
		{
			pt = &clip3;
			clip3.set = true;
		}
		else 
		{
			ResetClipMode ();
			pt = &clip1;
			clip1.set = true;
		}
		SnapToPoint (x, y, pt->ptclip);
		// third coordinates for clip point: use d_work_max
		// cf VIEWTYPE defintion: enum VIEWTYPE {YZ, XZ, XY};
		viewtype = g_qeglobals.d_viewtype;
		dim = (viewtype == YZ ) ? 0 : ( (viewtype == XZ) ? 1 : 2 );
		(pt->ptclip)[dim] = g_qeglobals.d_work_max[dim];
	}
	Sys_UpdateWindows (W_XY | W_XY_OVERLAY| W_Z);
}


/*
==============
MoveClipPoint
==============
*/
vec3_t tdp;
void MoveClipPoint (int x, int y)
{
	int dim1, dim2;

	tdp[0] = tdp[1] = tdp[2] = 0.0;

	SnapToPoint (x, y, tdp);

	if (movingclip && GetCapture() == g_qeglobals.d_hwndXY)
	{
		SnapToPoint (x, y, movingclip->ptclip);
	}
	else
	{
		movingclip = NULL;
		dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
		dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;
		if (clip1.set)
		{
			if ( fDiff(clip1.ptclip[dim1], tdp[dim1]) < 1 &&
				 fDiff(clip1.ptclip[dim2], tdp[dim2]) < 1 )
			{
				movingclip = &clip1;
			}
		}
		if (clip2.set)
		{
			if ( fDiff(clip2.ptclip[dim1], tdp[dim1]) < 1 &&
				 fDiff(clip2.ptclip[dim2], tdp[dim2]) < 1 )
			{
				movingclip = &clip2;
			}
		}
		if (clip3.set)
		{
			if ( fDiff(clip3.ptclip[dim1], tdp[dim1]) < 1 &&
				 fDiff(clip3.ptclip[dim2], tdp[dim2]) < 1 )
			{
				movingclip = &clip3;
			}
		}
	}
}


/*
==============
DrawClipPoint
==============
*/
void DrawClipPoint (void)
{
	char strmsg[128];
	brush_t *brush;
	brush_t *list;
	face_t *face;
	winding_t *w;
	int order;
	int i;
	
	if (g_qeglobals.d_viewtype != XY)
	{
		glPushMatrix();
		if (g_qeglobals.d_viewtype == YZ)
			glRotatef (-90,  0, 1, 0);	    // put Z going up
		glRotatef (-90,  1, 0, 0);	    // put Z going up
	}
 
	glPointSize (4);
	glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER]);
	glBegin (GL_POINTS);

	if (clip1.set)
		glVertex3fv (clip1.ptclip);
	if (clip2.set)
		glVertex3fv (clip2.ptclip);
	if (clip3.set)
		glVertex3fv (clip3.ptclip);

	glEnd ();
	glPointSize (1);
     
	if (clip1.set)
	{
		glRasterPos3f (clip1.ptclip[0]+2, clip1.ptclip[1]+2, clip1.ptclip[2]+2);
		strcpy (strmsg, "1");
		glCallLists (strlen(strmsg), GL_UNSIGNED_BYTE, strmsg);
	}
	if (clip2.set)
	{
		glRasterPos3f (clip2.ptclip[0]+2, clip2.ptclip[1]+2, clip2.ptclip[2]+2);
		strcpy (strmsg, "2");
		glCallLists (strlen(strmsg), GL_UNSIGNED_BYTE, strmsg);
	}
	if (clip3.set)
	{
		glRasterPos3f (clip3.ptclip[0]+2, clip3.ptclip[1]+2, clip3.ptclip[2]+2);
		strcpy (strmsg, "3");
		glCallLists (strlen(strmsg), GL_UNSIGNED_BYTE, strmsg);
	}
	if (clip1.set && clip2.set)
	{
		ProduceSplitLists();
		list = ( (g_qeglobals.d_viewtype == XZ) ? !g_qeglobals.d_clipswitch : g_qeglobals.d_clipswitch) ? &g_qeglobals.d_frontsplits : &g_qeglobals.d_backsplits;

		for (brush = list->next ; brush != NULL && brush != list ; brush=brush->next)
		{
			glColor3f (1,1,0);
			for (face = brush->brush_faces,order = 0 ; face ; face=face->next, order++)
			{
				w = face->face_winding;
				if (!w)
					continue;
				// draw the polygon
				glBegin(GL_LINE_LOOP);
				for (i=0 ; i<w->numpoints ; i++)
					glVertex3fv(w->points[i]);
				glEnd();
			}
		}
	}
	if (g_qeglobals.d_viewtype != XY)
		glPopMatrix();
}


/*
==============
EndClipPoint
==============
*/
void EndClipPoint (void)
{
	if (movingclip)
	{
		movingclip = NULL;
		ReleaseCapture ();
	}
}


/*
============================================================================

DRAWING

============================================================================
*/


/*
==============
XY_DrawGrid
==============
*/
void XY_DrawGrid (void)
{
	float	x, y, xb, xe, yb, ye;
	int		w, h;
	char	text[32];
	int		dim1, dim2;
	char	view[20];
	int		size;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (g_qeglobals.d_gridsize == 128)
		size = 128;
	else if (g_qeglobals.d_gridsize == 256)
		size = 256;
	else
		size = 64;

	w = g_qeglobals.d_xyz.width/2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height/2 / g_qeglobals.d_xyz.scale;

	dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
	dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;

	xb = g_qeglobals.d_xyz.origin[dim1] - w;
	if (xb < region_mins[dim1])
		xb = region_mins[dim1];
	xb = size * floor (xb/size);

	xe = g_qeglobals.d_xyz.origin[dim1] + w;
	if (xe > region_maxs[dim1])
		xe = region_maxs[dim1];
	xe = size * ceil (xe/size);

	yb = g_qeglobals.d_xyz.origin[dim2] - h;
	if (yb < region_mins[dim2])
		yb = region_mins[dim2];
	yb = size * floor (yb/size);

	ye = g_qeglobals.d_xyz.origin[dim2] + h;
	if (ye > region_maxs[dim2])
		ye = region_maxs[dim2];
	ye = size * ceil (ye/size);

	// draw major blocks
	if ( g_qeglobals.d_showgrid )
	{
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR]);
		
		glBegin (GL_LINES);
		for (x=xb ; x<=xe ; x+=size)
		{
			glVertex2f (x, yb);
			glVertex2f (x, ye);
		}
		for (y=yb ; y<=ye ; y+=size)
		{
			glVertex2f (xb, y);
			glVertex2f (xe, y);
		}
		glEnd ();
	}

	// draw minor blocks
	if ( g_qeglobals.d_showgrid && g_qeglobals.d_gridsize*g_qeglobals.d_xyz.scale >= 4)
	{
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR]);

		glBegin (GL_LINES);
		for (x=xb ; x<xe ; x += g_qeglobals.d_gridsize)
		{
			if ( ! ((int)x & 63) )
				continue;
			glVertex2f (x, yb);
			glVertex2f (x, ye);
		}
		for (y=yb ; y<ye ; y+=g_qeglobals.d_gridsize)
		{
			if ( ! ((int)y & 63) )
				continue;
			glVertex2f (xb, y);
			glVertex2f (xe, y);
		}
		glEnd ();
	}

	// draw coordinate text if needed
	if ( g_qeglobals.d_savedinfo.show_coordinates)
	{
		//glColor4f(0, 0, 0, 0);
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT]);

		for (x=xb ; x<xe ; x+=size)
		{
			glRasterPos2f (x, g_qeglobals.d_xyz.origin[dim2] + h - 6/g_qeglobals.d_xyz.scale);
			sprintf (text, "%i",(int)x);
			glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
		}
		for (y=yb ; y<ye ; y+=size)
		{
			glRasterPos2f (g_qeglobals.d_xyz.origin[dim1] - w + 1, y);
			sprintf (text, "%i",(int)y);
			glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
		}
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME]);

		glRasterPos2f ( g_qeglobals.d_xyz.origin[dim1] - w + 35 / g_qeglobals.d_xyz.scale, g_qeglobals.d_xyz.origin[dim2] + h - 20 / g_qeglobals.d_xyz.scale );
   
		if (g_qeglobals.d_viewtype == XY)
			strcpy(view, "XY Top");
		else if (g_qeglobals.d_viewtype == XZ)
			strcpy(view, "XZ Front");
		else
			strcpy(view, "YZ Side");

		glCallLists (strlen(view), GL_UNSIGNED_BYTE, view);
	}

	// show current work zone?
	// the work zone is used to place dropped points and brushes
	if (g_qeglobals.d_show_work)
	{
		glColor3f( 1.0f, 0.0f, 0.0f );
		glBegin( GL_LINES );
		glVertex2f( xb, g_qeglobals.d_work_min[dim2] );
		glVertex2f( xe, g_qeglobals.d_work_min[dim2] );
		glVertex2f( xb, g_qeglobals.d_work_max[dim2] );
		glVertex2f( xe, g_qeglobals.d_work_max[dim2] );
		glVertex2f( g_qeglobals.d_work_min[dim1], yb );
		glVertex2f( g_qeglobals.d_work_min[dim1], ye );
		glVertex2f( g_qeglobals.d_work_max[dim1], yb );
		glVertex2f( g_qeglobals.d_work_max[dim1], ye );
		glEnd();
	}
}

/*
==============
XY_DrawBlockGrid
==============
*/
void XY_DrawBlockGrid (void)
{
	float	x, y, xb, xe, yb, ye;
	int		w, h;
	char	text[32];
	int dim1, dim2;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	w = g_qeglobals.d_xyz.width/2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height/2 / g_qeglobals.d_xyz.scale;

	dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
	dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;

	xb = g_qeglobals.d_xyz.origin[dim1] - w;
	if (xb < region_mins[dim1])
		xb = region_mins[dim1];
	xb = 1024 * floor (xb/1024);

	xe = g_qeglobals.d_xyz.origin[dim1] + w;
	if (xe > region_maxs[dim1])
		xe = region_maxs[dim1];
	xe = 1024 * ceil (xe/1024);

	yb = g_qeglobals.d_xyz.origin[dim2] - h;
	if (yb < region_mins[dim2])
		yb = region_mins[dim2];
	yb = 1024 * floor (yb/1024);

	ye = g_qeglobals.d_xyz.origin[dim2] + h;
	if (ye > region_maxs[dim2])
		ye = region_maxs[dim2];
	ye = 1024 * ceil (ye/1024);

	// draw major blocks
	glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK]);
	glLineWidth (2);

	glBegin (GL_LINES);
	
	for (x=xb ; x<=xe ; x+=1024)
	{
		glVertex2f (x, yb);
		glVertex2f (x, ye);
	}
	for (y=yb ; y<=ye ; y+=1024)
	{
		glVertex2f (xb, y);
		glVertex2f (xe, y);
	}
	
	glEnd ();
	glLineWidth (1);

	// draw coordinate text if needed
	for (x=xb ; x<xe ; x+=1024)
		for (y=yb ; y<ye ; y+=1024)
		{
			glRasterPos2f (x+512, y+512);
			sprintf (text, "%i,%i",(int)floor(x/1024), (int)floor(y/1024) );
			glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
		}

	glColor4f(0, 0, 0, 0);
}


void DrawCameraIcon (void)
{
	float	x, y, a;
//	char	text[128];

	if (g_qeglobals.d_viewtype == XY)
	{
		x = g_qeglobals.d_camera.origin[0];
		y = g_qeglobals.d_camera.origin[1];
		a = g_qeglobals.d_camera.angles[YAW]/180*Q_PI;
	}
	else if (g_qeglobals.d_viewtype == YZ)
	{
		x = g_qeglobals.d_camera.origin[1];
		y = g_qeglobals.d_camera.origin[2];
		a = g_qeglobals.d_camera.angles[PITCH]/180*Q_PI;
	}
	else
	{
		x = g_qeglobals.d_camera.origin[0];
		y = g_qeglobals.d_camera.origin[2];
		a = g_qeglobals.d_camera.angles[PITCH]/180*Q_PI;
	}

	glColor3f (0.0, 0.0, 1.0);
	glBegin(GL_LINE_STRIP);
	glVertex3f (x-16,y,0);
	glVertex3f (x,y+8,0);
	glVertex3f (x+16,y,0);
	glVertex3f (x,y-8,0);
	glVertex3f (x-16,y,0);
	glVertex3f (x+16,y,0);
	glEnd ();
	
	glBegin(GL_LINE_STRIP);
	glVertex3f (x+48*cos(a+Q_PI/4), y+48*sin(a+Q_PI/4), 0);
	glVertex3f (x, y, 0);
	glVertex3f (x+48*cos(a-Q_PI/4), y+48*sin(a-Q_PI/4), 0);
	glEnd ();

//	glRasterPos2f (x+64, y+64);
//	sprintf (text, "angle: %f", g_qeglobals.d_camera.angles[YAW]);
//	glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
}

void DrawZIcon (void)
{
	float	x, y;

	if (g_qeglobals.d_viewtype == XY)
	{
		x = g_qeglobals.d_z.origin[0];
		y = g_qeglobals.d_z.origin[1];

		glEnable (GL_BLEND);
		glDisable (GL_TEXTURE_2D);
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glDisable (GL_CULL_FACE);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f (0.0, 0.0, 1.0, 0.25);
		glBegin(GL_QUADS);
		glVertex3f (x-8,y-8,0);
		glVertex3f (x+8,y-8,0);
		glVertex3f (x+8,y+8,0);
		glVertex3f (x-8,y+8,0);
		glEnd ();
		glDisable (GL_BLEND);

		glColor4f (0.0, 0.0, 1.0, 1);

		glBegin(GL_LINE_LOOP);
		glVertex3f (x-8,y-8,0);
		glVertex3f (x+8,y-8,0);
		glVertex3f (x+8,y+8,0);
		glVertex3f (x-8,y+8,0);
		glEnd ();

		glBegin(GL_LINE_STRIP);
		glVertex3f (x-4,y+4,0);
		glVertex3f (x+4,y+4,0);
		glVertex3f (x-4,y-4,0);
		glVertex3f (x+4,y-4,0);
		glEnd ();
	}
}


/*
==================
FilterBrush
==================
*/
BOOL FilterBrush(brush_t *pb)
{
	if (!pb->owner)
		return FALSE;		// during construction

	if (pb->hiddenbrush)
		return TRUE;

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_CLIP)
	{
		if (!strncmp(pb->brush_faces->texdef.name, "clip", 4))
			return TRUE;
	}

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_WATER)
	{
		if (pb->brush_faces->texdef.name[0] == '*')
			return TRUE;
	}

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_SKY)
	{
		if (!strncmp(pb->brush_faces->texdef.name, "sky", 3))
			return TRUE;
	}

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_FUNC_WALL)
	{
		if (!strncmp(pb->owner->eclass->name, "func_wall", 9))
			return TRUE;
	}

	if (pb->owner == world_entity)
	{
		if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_WORLD)
			return TRUE;
		return FALSE;
	}
	else if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_ENT)
		return TRUE;
/*	
	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_LIGHTS)
	{
		if (!strncmp(pb->owner->eclass->name, "light", 5))
			return TRUE;
	}

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS)
	{
		if (!strncmp(pb->owner->eclass->name, "path", 4))
			return TRUE;
	}
*/
	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_LIGHTS)
	{
		return (pb->owner->eclass->showflags & ECLASS_LIGHT);
		//if (!strncmp(pb->owner->eclass->name, "light", 5))
		//	return TRUE;
	}

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS)
	{
		return (pb->owner->eclass->showflags & ECLASS_PATH);
		//if (!strncmp(pb->owner->eclass->name, "path", 4))
		//	return TRUE;
	}

	return FALSE;
}

/*
=============================================================

  PATH LINES

=============================================================
*/

/*
==================
DrawPathLines

Draws connections between entities.
Needs to consider all entities, not just ones on screen,
because the lines can be visible when neither end is.
Called for both camera view and xy view.
==================
*/
void DrawPathLines (void)
{
	int		i, j, k;
	vec3_t	mid, mid1;
	entity_t *se, *te;
	brush_t	*sb, *tb;
	char	*psz;
	vec3_t	dir, s1, s2;
	vec_t	len, f;
	int		arrows;
	int			num_entities;
	char		*ent_target[MAX_MAP_ENTITIES];
	entity_t	*ent_entity[MAX_MAP_ENTITIES];

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS)
		return;

	num_entities = 0;
	for (te = entities.next ; te != &entities && num_entities != MAX_MAP_ENTITIES ; te = te->next)
	{
		ent_target[num_entities] = ValueForKey (te, "target");
		if (ent_target[num_entities][0])
		{
			ent_entity[num_entities] = te;
			num_entities++;
		}
	}

	for (se = entities.next ; se != &entities ; se = se->next)
	{
		psz = ValueForKey(se, "targetname");
	
		if (psz == NULL || psz[0] == '\0')
			continue;
		
		sb = se->brushes.onext;
		if (sb == &se->brushes)
			continue;

		for (k=0 ; k<num_entities ; k++)
		{
			if (strcmp (ent_target[k], psz))
				continue;

			te = ent_entity[k];
			tb = te->brushes.onext;
			if (tb == &te->brushes)
				continue;

			for (i=0 ; i<3 ; i++)
				mid[i] = (sb->mins[i] + sb->maxs[i])*0.5; 

			for (i=0 ; i<3 ; i++)
				mid1[i] = (tb->mins[i] + tb->maxs[i])*0.5; 

			VectorSubtract (mid1, mid, dir);
			len = VectorNormalize (dir);
			s1[0] = -dir[1]*8 + dir[0]*8;
			s2[0] = dir[1]*8 + dir[0]*8;
			s1[1] = dir[0]*8 + dir[1]*8;
			s2[1] = -dir[0]*8 + dir[1]*8;

			glColor3f (se->eclass->color[0], se->eclass->color[1], se->eclass->color[2]);

			glBegin(GL_LINES);
			glVertex3fv(mid);
			glVertex3fv(mid1);

			arrows = (int)(len / 256) + 1;

			for (i=0 ; i<arrows ; i++)
			{
				f = len * (i + 0.5) / arrows;

				for (j=0 ; j<3 ; j++)
					mid1[j] = mid[j] + f*dir[j];
				glVertex3fv (mid1);
				glVertex3f (mid1[0] + s1[0], mid1[1] + s1[1], mid1[2]);
				glVertex3fv (mid1);
				glVertex3f (mid1[0] + s2[0], mid1[1] + s2[1], mid1[2]);
			}

			glEnd();
		}
	}

	return;
}

//=============================================================


// can be greatly simplified but per usual i am in a hurry 
// which is not an excuse, just a fact
void PaintSizeInfo (int dim1, int dim2, vec3_t minbounds, vec3_t maxbounds)
{
  vec3_t size;
  char	dimstr[128];

  VectorSubtract(maxbounds, minbounds, size);

  glColor3f(g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] * .65, 
            g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] * .65,
            g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] * .65);

  if (g_qeglobals.d_viewtype == XY)
  {
    glBegin (GL_LINES);

    glVertex3f(minbounds[dim1], minbounds[dim2] - 6.0f  / g_qeglobals.d_xyz.scale, 0.0f);
    glVertex3f(minbounds[dim1], minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);

    glVertex3f(minbounds[dim1], minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);
    glVertex3f(maxbounds[dim1], minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);

    glVertex3f(maxbounds[dim1], minbounds[dim2] - 6.0f  / g_qeglobals.d_xyz.scale, 0.0f);
    glVertex3f(maxbounds[dim1], minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);
  

    glVertex3f(maxbounds[dim1] + 6.0f  / g_qeglobals.d_xyz.scale, minbounds[dim2], 0.0f);
    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, minbounds[dim2], 0.0f);

    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, minbounds[dim2], 0.0f);
    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, maxbounds[dim2], 0.0f);
  
    glVertex3f(maxbounds[dim1] + 6.0f  / g_qeglobals.d_xyz.scale, maxbounds[dim2], 0.0f);
    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, maxbounds[dim2], 0.0f);

    glEnd();

    glRasterPos3f (Betwixt(minbounds[dim1], maxbounds[dim1]),  minbounds[dim2] - 20.0  / g_qeglobals.d_xyz.scale, 0.0f);

    sprintf(dimstr, dimstrings[dim1], size[dim1]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
    glRasterPos3f (maxbounds[dim1] + 16.0  / g_qeglobals.d_xyz.scale, Betwixt(minbounds[dim2], maxbounds[dim2]), 0.0f);

    sprintf(dimstr, dimstrings[dim2], size[dim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

    glRasterPos3f (minbounds[dim1] + 4, maxbounds[dim2] + 8 / g_qeglobals.d_xyz.scale, 0.0f);

    sprintf(dimstr, orgstrings[0], minbounds[dim1], maxbounds[dim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	
  }
  else if (g_qeglobals.d_viewtype == XZ)
  {
    glBegin (GL_LINES);

    glVertex3f(minbounds[dim1], 0.0f, minbounds[dim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(minbounds[dim1], 0.0f, minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale);

    glVertex3f(minbounds[dim1], 0.0f, minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale);
    glVertex3f(maxbounds[dim1], 0.0f, minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale);

    glVertex3f(maxbounds[dim1], 0.0f, minbounds[dim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(maxbounds[dim1], 0.0f, minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale);
  

    glVertex3f(maxbounds[dim1] + 6.0f  / g_qeglobals.d_xyz.scale, 0.0f, minbounds[dim2]);
    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, minbounds[dim2]);

    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, minbounds[dim2]);
    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, maxbounds[dim2]);
  
    glVertex3f(maxbounds[dim1] + 6.0f  / g_qeglobals.d_xyz.scale, 0.0f, maxbounds[dim2]);
    glVertex3f(maxbounds[dim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, maxbounds[dim2]);

    glEnd();

    glRasterPos3f (Betwixt(minbounds[dim1], maxbounds[dim1]), 0.0f, minbounds[dim2] - 20.0  / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, dimstrings[dim1], size[dim1]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
    glRasterPos3f (maxbounds[dim1] + 16.0  / g_qeglobals.d_xyz.scale, 0.0f, Betwixt(minbounds[dim2], maxbounds[dim2]));
    sprintf(dimstr, dimstrings[dim2], size[dim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

    glRasterPos3f (minbounds[dim1] + 4, 0.0f, maxbounds[dim2] + 8 / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, orgstrings[1], minbounds[dim1], maxbounds[dim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
	
  }
  else
  {
    glBegin (GL_LINES);

    glVertex3f(0.0f, minbounds[dim1], minbounds[dim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(0.0f, minbounds[dim1], minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale);

    glVertex3f(0.0f, minbounds[dim1], minbounds[dim2] - 10.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(0.0f, maxbounds[dim1], minbounds[dim2] - 10.0f  / g_qeglobals.d_xyz.scale);

    glVertex3f(0.0f, maxbounds[dim1], minbounds[dim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(0.0f, maxbounds[dim1], minbounds[dim2] - 10.0f / g_qeglobals.d_xyz.scale);
  

    glVertex3f(0.0f, maxbounds[dim1] + 6.0f  / g_qeglobals.d_xyz.scale, minbounds[dim2]);
    glVertex3f(0.0f, maxbounds[dim1] + 10.0f  / g_qeglobals.d_xyz.scale, minbounds[dim2]);

    glVertex3f(0.0f, maxbounds[dim1] + 10.0f  / g_qeglobals.d_xyz.scale, minbounds[dim2]);
    glVertex3f(0.0f, maxbounds[dim1] + 10.0f  / g_qeglobals.d_xyz.scale, maxbounds[dim2]);
  
    glVertex3f(0.0f, maxbounds[dim1] + 6.0f  / g_qeglobals.d_xyz.scale, maxbounds[dim2]);
    glVertex3f(0.0f, maxbounds[dim1] + 10.0f  / g_qeglobals.d_xyz.scale, maxbounds[dim2]);

    glEnd();

    glRasterPos3f (0.0f, Betwixt(minbounds[dim1], maxbounds[dim1]),  minbounds[dim2] - 20.0  / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, dimstrings[dim1], size[dim1]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
    glRasterPos3f (0.0f, maxbounds[dim1] + 16.0  / g_qeglobals.d_xyz.scale, Betwixt(minbounds[dim2], maxbounds[dim2]));
    sprintf(dimstr, dimstrings[dim2], size[dim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

    glRasterPos3f (0.0f, minbounds[dim1] + 4.0, maxbounds[dim2] + 8 / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, orgstrings[2], minbounds[dim1], maxbounds[dim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

  }
}


/*
==============
XY_Draw
==============
*/
void XY_Draw (void)
{
    brush_t	*brush;
	float	w, h;
	entity_t	*e;
	double	start, end;
	vec3_t	mins, maxs;
	int		drawn, culled;
	int		i;
	int dim1, dim2;
	int savedrawn;
	qboolean fixedsize;
	vec3_t minbounds, maxbounds;


	if (!active_brushes.next)
		return;	// not valid yet

	if (g_qeglobals.d_xyz.timing)
		start = Sys_DoubleTime ();

	//
	// clear
	//
	g_qeglobals.d_xyz.d_dirty = false;

	glViewport(0, 0, g_qeglobals.d_xyz.width, g_qeglobals.d_xyz.height);
	glClearColor (
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0],
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1],
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2],
		0);

    glClear(GL_COLOR_BUFFER_BIT);

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();

	w = g_qeglobals.d_xyz.width/2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height/2 / g_qeglobals.d_xyz.scale;

	dim1 = (g_qeglobals.d_viewtype == YZ) ? 1 : 0;
	dim2 = (g_qeglobals.d_viewtype == XY) ? 1 : 2;

	mins[0] = g_qeglobals.d_xyz.origin[dim1] - w;
	maxs[0] = g_qeglobals.d_xyz.origin[dim1] + w;
	mins[1] = g_qeglobals.d_xyz.origin[dim2] - h;
	maxs[1] = g_qeglobals.d_xyz.origin[dim2] + h;

	glOrtho (mins[0], maxs[0], mins[1], maxs[1], -8192, 8192);

	//
	// now draw the grid
	//
	XY_DrawGrid ();

	//
	// draw stuff
	//
    glShadeModel (GL_FLAT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColor3f(0, 0, 0);
//		glEnable (GL_LINE_SMOOTH);

	drawn = culled = 0;

	if (g_qeglobals.d_viewtype != XY)
	{
		glPushMatrix();
		if (g_qeglobals.d_viewtype == YZ)
			glRotatef (-90,  0, 1, 0);	    // put Z going up
		//else
			glRotatef (-90,  1, 0, 0);	    // put Z going up
	}

//	e = NULL;
	e = world_entity;
	for (brush = active_brushes.next ; brush != &active_brushes ; brush=brush->next)
	{
		if (brush->mins[dim1] > maxs[0]
			|| brush->mins[dim2] > maxs[1]
			|| brush->maxs[dim1] < mins[0]
			|| brush->maxs[dim2] < mins[1]	)
		{
			culled++;
			continue;		// off screen
		}

		if (FilterBrush (brush))
			continue;

		drawn++;

		if (brush->owner != e && brush->owner)
		{
//			e = brush->owner;
//			glColor3fv(e->eclass->color);
			glColor3fv(brush->owner->eclass->color);
		}
		else
		{
			glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES]);
		}
		Brush_DrawXY( brush, g_qeglobals.d_viewtype);
	}

	DrawPathLines ();

	//
	// draw pointfile
	//
	if ( g_qeglobals.d_pointfile_display_list)
		glCallList (g_qeglobals.d_pointfile_display_list);

	if (!(g_qeglobals.d_viewtype == XY))
		glPopMatrix();

	//
	// draw block grid
	//
	if ( g_qeglobals.d_show_blocks)
		XY_DrawBlockGrid ();

	//
	// now draw selected brushes
	//

	if (g_qeglobals.d_viewtype != XY)
	{
		glPushMatrix();
		if (g_qeglobals.d_viewtype == YZ)
			glRotatef (-90,  0, 1, 0);	    // put Z going up
		//else
			glRotatef (-90,  1, 0, 0);	    // put Z going up
	}

	glTranslatef( g_qeglobals.d_select_translate[0], g_qeglobals.d_select_translate[1], g_qeglobals.d_select_translate[2]);

//	glColor3f(1.0, 0.0, 0.0);
	glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES]);

	glEnable (GL_LINE_STIPPLE);
	glLineStipple (3, 0xaaaa);

	glLineWidth (2);

// paint size
	minbounds[0] = minbounds[1] = minbounds[2] = 8192.0f;
	maxbounds[0] = maxbounds[1] = maxbounds[2] = -8192.0f;

	savedrawn = drawn;
	fixedsize = false;
//

	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
	{
		drawn++;

		Brush_DrawXY( brush, g_qeglobals.d_viewtype );

// paint size
	    if (!fixedsize)
	    {
			if (brush->owner->eclass->fixedsize)
				fixedsize = true;
			if (g_qeglobals.d_savedinfo.check_sizepaint)
			{
				for (i = 0; i < 3; i++)
				{
					if (brush->mins[i] < minbounds[i])
						minbounds[i] = brush->mins[i];
					if (brush->maxs[i] > maxbounds[i])
						maxbounds[i] = brush->maxs[i];
				}
			}
		}
//
	}

	glDisable (GL_LINE_STIPPLE);
	glLineWidth (1);

// paint size
	if (!fixedsize && drawn - savedrawn > 0 && g_qeglobals.d_savedinfo.check_sizepaint)
		PaintSizeInfo(dim1, dim2, minbounds, maxbounds);
//

	// edge / vertex flags
	if (g_qeglobals.d_select_mode == sel_vertex)
	{
		glPointSize (4);
		glColor3f (0,1,0);
		glBegin (GL_POINTS);
		for (i=0 ; i<g_qeglobals.d_numpoints ; i++)
			glVertex3fv (g_qeglobals.d_points[i]);
		glEnd ();
		glPointSize (1);
	}
	else if (g_qeglobals.d_select_mode == sel_edge)
	{
		float	*v1, *v2;

		glPointSize (4);
		glColor3f (0,0,1);
		glBegin (GL_POINTS);
		for (i=0 ; i<g_qeglobals.d_numedges ; i++)
		{
			v1 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p1];
			v2 = g_qeglobals.d_points[g_qeglobals.d_edges[i].p2];
			glVertex3f ( (v1[0]+v2[0])*0.5,(v1[1]+v2[1])*0.5,(v1[2]+v2[2])*0.5);
		}
		glEnd ();
		glPointSize (1);
	}

	glTranslatef (-g_qeglobals.d_select_translate[0], -g_qeglobals.d_select_translate[1], -g_qeglobals.d_select_translate[2]);

	if (!(g_qeglobals.d_viewtype == XY))
		glPopMatrix();

	// clipper
	if (g_qeglobals.d_clipmode)
		DrawClipPoint ();

	//
	// now draw camera point
	//
	DrawCameraIcon ();
	DrawZIcon ();

    glFinish();
	QE_CheckOpenGLForErrors();

	if (g_qeglobals.d_xyz.timing)
	{
		end = Sys_DoubleTime ();
		Sys_Printf ("xyz: %i ms\n", (int)(1000*(end-start)));
	}
}

/*
==============
XY_Overlay
==============
*/
void XY_Overlay (void)
{
	int	w, h;
	int	r[4];
	static	vec3_t	lastz;
	static	vec3_t	lastcamera;

	glViewport(0, 0, g_qeglobals.d_xyz.width, g_qeglobals.d_xyz.height);

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();

	w = g_qeglobals.d_xyz.width/2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height/2 / g_qeglobals.d_xyz.scale;

	glOrtho (g_qeglobals.d_xyz.origin[0] - w, g_qeglobals.d_xyz.origin[0] + w, g_qeglobals.d_xyz.origin[1] - h, g_qeglobals.d_xyz.origin[1] + h, -8192, 8192);
	
	//
	// erase the old camera and z checker positions
	// if the entire xy hasn't been redrawn
	//
	if (g_qeglobals.d_xyz.d_dirty)
	{
		glReadBuffer (GL_BACK);
		glDrawBuffer (GL_FRONT);

		glRasterPos2f (lastz[0]-9, lastz[1]-9);
		glGetIntegerv (GL_CURRENT_RASTER_POSITION,r);
		glCopyPixels(r[0], r[1], 18,18, GL_COLOR);

		glRasterPos2f (lastcamera[0]-50, lastcamera[1]-50);
		glGetIntegerv (GL_CURRENT_RASTER_POSITION,r);
		glCopyPixels(r[0], r[1], 100,100, GL_COLOR);
	}
	g_qeglobals.d_xyz.d_dirty = true;

	//
	// save off underneath where we are about to draw
	//
	VectorCopy (g_qeglobals.d_z.origin, lastz);
	VectorCopy (g_qeglobals.d_camera.origin, lastcamera);

	glReadBuffer (GL_FRONT);
	glDrawBuffer (GL_BACK);

	glRasterPos2f (lastz[0]-9, lastz[1]-9);
	glGetIntegerv (GL_CURRENT_RASTER_POSITION,r);
	glCopyPixels(r[0], r[1], 18,18, GL_COLOR);

	glRasterPos2f (lastcamera[0]-50, lastcamera[1]-50);
	glGetIntegerv (GL_CURRENT_RASTER_POSITION,r);
	glCopyPixels(r[0], r[1], 100,100, GL_COLOR);

	//
	// draw the new icons
	//
	glDrawBuffer (GL_FRONT);

    glShadeModel (GL_FLAT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glColor3f(0, 0, 0);

	DrawCameraIcon ();
	DrawZIcon ();

	glDrawBuffer (GL_BACK);
    glFinish();
}

