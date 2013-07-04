
#include "qe3.h"

#define	PAGEFLIPS	2

const char *DimStrings[] = {"x:%.f", "y:%.f", "z:%.f"};
const char *OrgStrings[] = {"(x:%.f  y:%.f)", "(x:%.f  z:%.f)", "(y:%.f  z:%.f)"};

clipPoint_t	Clip1;
clipPoint_t	Clip2;
clipPoint_t	Clip3;
clipPoint_t	*pMovingClip;

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
	int			nDim1, nDim2;
	brush_t		*b;

	nDim1 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.ViewType == XY) ? 1 : 2;

	b = selected_brushes.next;
	if (b && b->next != b)
	{
	  g_qeglobals.d_xyz.origin[nDim1] = b->mins[nDim1];
	  g_qeglobals.d_xyz.origin[nDim2] = b->mins[nDim2];
	}
	else
	{
	  g_qeglobals.d_xyz.origin[nDim1] = g_qeglobals.d_camera.origin[nDim1];
	  g_qeglobals.d_xyz.origin[nDim2] = g_qeglobals.d_camera.origin[nDim2];
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
  if (g_qeglobals.ViewType == XY)
  {
	  out[0] = in[0];
	  out[1] = in[1];
  }
  else
  if (g_qeglobals.ViewType == XZ)
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
	g_qeglobals.clipmode ^= 1;
	CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_CLIPPER, MF_BYCOMMAND | (g_qeglobals.clipmode ? MF_CHECKED : MF_UNCHECKED)  );

	ResetClipMode ();
	Sys_UpdateGridStatusBar ();
}

void UnsetClipMode (void)
{
	g_qeglobals.clipmode = false;
	CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_CLIPPER, MF_BYCOMMAND | (g_qeglobals.clipmode ? MF_CHECKED : MF_UNCHECKED)  );

	ResetClipMode ();
	Sys_UpdateGridStatusBar ();
}

void ResetClipMode (void)
{
  if (g_qeglobals.clipmode)
  {
	Clip1.ptClip[0] = Clip1.ptClip[1] = Clip1.ptClip[2] = 0.0;
	Clip1.bSet = false;
	Clip2.ptClip[0] = Clip2.ptClip[1] = Clip2.ptClip[2] = 0.0;
	Clip2.bSet = false;
	Clip3.ptClip[0] = Clip3.ptClip[1] = Clip3.ptClip[2] = 0.0;
	Clip3.bSet = false;

    CleanList(&g_qeglobals.brFrontSplits);
    CleanList(&g_qeglobals.brBackSplits);

    g_qeglobals.brFrontSplits.next = &g_qeglobals.brFrontSplits;
    g_qeglobals.brBackSplits.next = &g_qeglobals.brBackSplits;
  }
  else
  {
    if (pMovingClip)
    {
      ReleaseCapture();
      pMovingClip = NULL;
    }
    CleanList(&g_qeglobals.brFrontSplits);
    CleanList(&g_qeglobals.brBackSplits);

    g_qeglobals.brFrontSplits.next = &g_qeglobals.brFrontSplits;
    g_qeglobals.brBackSplits.next = &g_qeglobals.brBackSplits;

	Sys_UpdateWindows (W_ALL);
  }
}


void CleanList (brush_t *pList)
{
	brush_t		*pBrush;
	brush_t		*pNext;

	pBrush = pList->next; 
	while (pBrush != NULL && pBrush != pList)
	{
		pNext = pBrush->next;
		Brush_Free(pBrush);
		pBrush = pNext;
	}
}

void ProduceSplitLists (void)
{
	brush_t		*pBrush;
	brush_t		*pFront;
	brush_t		*pBack;
	face_t		face;

  CleanList (&g_qeglobals.brFrontSplits);
  CleanList (&g_qeglobals.brBackSplits);

  g_qeglobals.brFrontSplits.next = &g_qeglobals.brFrontSplits;
  g_qeglobals.brBackSplits.next = &g_qeglobals.brBackSplits;

  for (pBrush = selected_brushes.next ; pBrush != NULL && pBrush != &selected_brushes ; pBrush=pBrush->next)
  {
    pFront = NULL;
    pBack = NULL;
    
      if (Clip1.bSet && Clip2.bSet)
      {
        
        VectorCopy (Clip1.ptClip, face.planepts[0]);
        VectorCopy (Clip2.ptClip, face.planepts[1]);
        VectorCopy (Clip3.ptClip, face.planepts[2]);
        if (Clip3.bSet == false)
        {
          if (g_qeglobals.ViewType == XY)
          {
            face.planepts[0][2] = pBrush->mins[2];
            face.planepts[1][2] = pBrush->mins[2];
            face.planepts[2][0] = Betwixt (Clip1.ptClip[0], Clip2.ptClip[0]);
            face.planepts[2][1] = Betwixt (Clip1.ptClip[1], Clip2.ptClip[1]);
            face.planepts[2][2] = pBrush->maxs[2];
          }
          else if (g_qeglobals.ViewType == YZ)
          {
            face.planepts[0][0] = pBrush->mins[0];
            face.planepts[1][0] = pBrush->mins[0];
            face.planepts[2][1] = Betwixt (Clip1.ptClip[1], Clip2.ptClip[1]);
            face.planepts[2][2] = Betwixt (Clip1.ptClip[2], Clip2.ptClip[2]);
            face.planepts[2][0] = pBrush->maxs[0];
          }
          else
          {
            face.planepts[0][1] = pBrush->mins[1];
            face.planepts[1][1] = pBrush->mins[1];
            face.planepts[2][0] = Betwixt (Clip1.ptClip[0], Clip2.ptClip[0]);
            face.planepts[2][2] = Betwixt (Clip1.ptClip[2], Clip2.ptClip[2]);
            face.planepts[2][1] = pBrush->maxs[1];
          }
        }
        CSG_SplitBrushByFace (pBrush, &face, &pFront, &pBack);
        if (pBack)
          Brush_AddToList(pBack, &g_qeglobals.brBackSplits);
        if (pFront)
          Brush_AddToList(pFront, &g_qeglobals.brFrontSplits);
      }

  }
}

void Brush_CopyList (brush_t *pFrom, brush_t *pTo)
{
	brush_t *pBrush;
	brush_t *pNext;

	pBrush = pFrom->next;

	while (pBrush != NULL && pBrush != pFrom)
	{
		pNext = pBrush->next;
		Brush_RemoveFromList(pBrush);
		Brush_AddToList(pBrush, pTo);
		pBrush = pNext;
	}
}


brush_t hold_brushes;
void Clip (void)
{
	brush_t		*pList;

  if (g_qeglobals.clipmode)
  {
    hold_brushes.next = &hold_brushes;
    ProduceSplitLists();

	pList = ( (g_qeglobals.ViewType == XZ) ? !g_qeglobals.clipbSwitch : g_qeglobals.clipbSwitch) ? &g_qeglobals.brFrontSplits : &g_qeglobals.brBackSplits;
    
	if (pList->next != pList)
    {
      Brush_CopyList(pList, &hold_brushes);
      CleanList(&g_qeglobals.brFrontSplits);
      CleanList(&g_qeglobals.brBackSplits);
      Select_Delete();
      Brush_CopyList(&hold_brushes, &selected_brushes);
	}
	ResetClipMode ();
	Sys_UpdateWindows(W_ALL);
  }
}


void SplitClip (void)
{
  if (g_qeglobals.clipmode)
  {

	ProduceSplitLists();
	if ((g_qeglobals.brFrontSplits.next != &g_qeglobals.brFrontSplits) &&
		(g_qeglobals.brBackSplits.next != &g_qeglobals.brBackSplits))
	{
		Select_Delete();
		Brush_CopyList(&g_qeglobals.brFrontSplits, &selected_brushes);
		Brush_CopyList(&g_qeglobals.brBackSplits, &selected_brushes);
		CleanList(&g_qeglobals.brFrontSplits);
		CleanList(&g_qeglobals.brBackSplits);
	}
	ResetClipMode ();
	Sys_UpdateWindows (W_ALL);
  }
}

void FlipClip (void)
{
  if (g_qeglobals.clipmode)
  {
	g_qeglobals.clipbSwitch = !g_qeglobals.clipbSwitch;
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
	if (g_qeglobals.ViewType == XY)
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[1] = g_qeglobals.d_xyz.origin[1] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
		point[2] = 0;
	}
	else if (g_qeglobals.ViewType == YZ)
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
	if (g_qeglobals.ViewType == XY)
	{
		point[0] = g_qeglobals.d_xyz.origin[0] + (x - g_qeglobals.d_xyz.width/2)/g_qeglobals.d_xyz.scale;
		point[1] = g_qeglobals.d_xyz.origin[1] + (y - g_qeglobals.d_xyz.height/2)/g_qeglobals.d_xyz.scale;
		point[2] = 0;

		point[0] = floor(point[0]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
		point[1] = floor(point[1]/g_qeglobals.d_gridsize+0.5)*g_qeglobals.d_gridsize;
	}
	else if (g_qeglobals.ViewType == YZ)
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
	int nAngle;

	buttonstate = buttons;

	// clipper
	if ((buttonstate & MK_LBUTTON) && g_qeglobals.clipmode)
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
	if (g_qeglobals.ViewType == XY)
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
	else if (g_qeglobals.ViewType == YZ)
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
//	if ( (buttonstate == MK_LBUTTON)
//		|| (buttonstate == (MK_LBUTTON | MK_SHIFT))
//		|| (buttonstate == (MK_LBUTTON | MK_CONTROL))
//		|| (buttonstate == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) )
	if (buttonstate & MK_LBUTTON)
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

		n1 = (g_qeglobals.ViewType == XY) ? 1 : 2;
		n2 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
		nAngle = (g_qeglobals.ViewType == XY) ? YAW : PITCH;

		if (point[n1] || point[n2])
		{
			g_qeglobals.d_camera.angles[nAngle] = 180/Q_PI*atan2 (point[n1], point[n2]);
			Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z);
		}
	}

	// shift mbutton = move z checker
	if (buttonstate == (MK_SHIFT|MK_MBUTTON) )
	{
//		XY_ToPoint (x, y, point);
		SnapToPoint (x, y, point);

		if (g_qeglobals.ViewType == XY)
		{
			g_qeglobals.d_z.origin[0] = point[0];
			g_qeglobals.d_z.origin[1] = point[1];
		}
		else if (g_qeglobals.ViewType == YZ)
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
	if (g_qeglobals.clipmode)
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
	int nDim;

	if (!DragDelta (x,y, junk))
		return;

	// delete the current selection
	if (selected_brushes.next != &selected_brushes)
		Brush_Free (selected_brushes.next);

//	XY_ToGridPoint (pressx, pressy, mins);
	SnapToPoint (pressx, pressy, mins);

	nDim = (g_qeglobals.ViewType == XY) ? 2 : (g_qeglobals.ViewType == YZ) ? 0 : 1;

//	mins[2] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_bottom_z/g_qeglobals.d_gridsize));
	mins[nDim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_work_min[nDim]/g_qeglobals.d_gridsize));

//	XY_ToGridPoint (x, y, maxs);
	SnapToPoint (x, y, maxs);

//	maxs[2] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_new_brush_top_z/g_qeglobals.d_gridsize));
	maxs[nDim] = g_qeglobals.d_gridsize * ((int)(g_qeglobals.d_work_max[nDim]/g_qeglobals.d_gridsize));

//	if (maxs[2] <= mins[2])
//		maxs[2] = mins[2] + g_qeglobals.d_gridsize;

	if (maxs[nDim] <= mins[nDim])
		maxs[nDim] = mins[nDim] + g_qeglobals.d_gridsize;

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
    int nAngle;
	int nDim1, nDim2;

	vec3_t	tdp;
	char	xystring[256];

	if (!buttonstate || buttonstate ^ MK_RBUTTON)
	{
		SnapToPoint (x, y, tdp);
		sprintf (xystring, "xyz Coordinates: (%i %i %i)", (int)tdp[0], (int)tdp[1], (int)tdp[2]);
		Sys_Status (xystring, 0);
	}

	// clipper
	if ((!buttonstate || buttonstate & MK_LBUTTON) && g_qeglobals.clipmode)
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

		if (g_qeglobals.ViewType == XY)
		{
			g_qeglobals.d_z.origin[0] = point[0];
			g_qeglobals.d_z.origin[1] = point[1];
		}
		else if (g_qeglobals.ViewType == YZ)
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

		n1 = (g_qeglobals.ViewType == XY) ? 1 : 2;
		n2 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
		nAngle = (g_qeglobals.ViewType == XY) ? YAW : PITCH;

		if (point[n1] || point[n2])
		{
			g_qeglobals.d_camera.angles[nAngle] = 180/Q_PI*atan2 (point[n1], point[n2]);
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
			nDim1 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
			nDim2 = (g_qeglobals.ViewType == XY) ? 1 : 2;

			g_qeglobals.d_xyz.origin[nDim1] -= (x-cursorx)/g_qeglobals.d_xyz.scale;
			g_qeglobals.d_xyz.origin[nDim2] += (y-cursory)/g_qeglobals.d_xyz.scale;

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
	clipPoint_t *pPt;
	int nViewType;
	int nDim;

	if (pMovingClip)
	{
		SnapToPoint (x, y, pMovingClip->ptClip);
	}
	else
	{
		pPt = NULL;
		if (Clip1.bSet == false)
		{
			pPt = &Clip1;
			Clip1.bSet = true;
		}
		else 
		if (Clip2.bSet == false)
		{
			pPt = &Clip2;
			Clip2.bSet = true;
		}
		else 
		if (Clip3.bSet == false)
		{
			pPt = &Clip3;
			Clip3.bSet = true;
		}
		else 
		{
			ResetClipMode ();
			pPt = &Clip1;
			Clip1.bSet = true;
		}
		SnapToPoint (x, y, pPt->ptClip);
		// third coordinates for clip point: use d_work_max
		// cf VIEWTYPE defintion: enum VIEWTYPE {YZ, XZ, XY};
		nViewType = g_qeglobals.ViewType;
		nDim = (nViewType == YZ ) ? 0 : ( (nViewType == XZ) ? 1 : 2 );
		(pPt->ptClip)[nDim] = g_qeglobals.d_work_max[nDim];
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
	int nDim1, nDim2;

	tdp[0] = tdp[1] = tdp[2] = 0.0;

	SnapToPoint (x, y, tdp);

	if (pMovingClip && GetCapture() == g_qeglobals.d_hwndXY)
	{
		SnapToPoint (x, y, pMovingClip->ptClip);
	}
	else
	{
		pMovingClip = NULL;
		nDim1 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
		nDim2 = (g_qeglobals.ViewType == XY) ? 1 : 2;
		if (Clip1.bSet)
		{
			if ( fDiff(Clip1.ptClip[nDim1], tdp[nDim1]) < 1 &&
				 fDiff(Clip1.ptClip[nDim2], tdp[nDim2]) < 1 )
			{
				pMovingClip = &Clip1;
			}
		}
		if (Clip2.bSet)
		{
			if ( fDiff(Clip2.ptClip[nDim1], tdp[nDim1]) < 1 &&
				 fDiff(Clip2.ptClip[nDim2], tdp[nDim2]) < 1 )
			{
				pMovingClip = &Clip2;
			}
		}
		if (Clip3.bSet)
		{
			if ( fDiff(Clip3.ptClip[nDim1], tdp[nDim1]) < 1 &&
				 fDiff(Clip3.ptClip[nDim2], tdp[nDim2]) < 1 )
			{
				pMovingClip = &Clip3;
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
	char strMsg[128];
	brush_t *pBrush;
	brush_t *pList;
	face_t *face;
	winding_t *w;
	int order;
	int i;
	
	if (g_qeglobals.ViewType != XY)
	{
		glPushMatrix();
		if (g_qeglobals.ViewType == YZ)
			glRotatef (-90,  0, 1, 0);	    // put Z going up
		glRotatef (-90,  1, 0, 0);	    // put Z going up
	}
 
	glPointSize (4);
	glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER]);
	glBegin (GL_POINTS);

	if (Clip1.bSet)
		glVertex3fv (Clip1.ptClip);
	if (Clip2.bSet)
		glVertex3fv (Clip2.ptClip);
	if (Clip3.bSet)
		glVertex3fv (Clip3.ptClip);

	glEnd ();
	glPointSize (1);
     
	if (Clip1.bSet)
	{
		glRasterPos3f (Clip1.ptClip[0]+2, Clip1.ptClip[1]+2, Clip1.ptClip[2]+2);
		strcpy (strMsg, "1");
		glCallLists (strlen(strMsg), GL_UNSIGNED_BYTE, strMsg);
	}
	if (Clip2.bSet)
	{
		glRasterPos3f (Clip2.ptClip[0]+2, Clip2.ptClip[1]+2, Clip2.ptClip[2]+2);
		strcpy (strMsg, "2");
		glCallLists (strlen(strMsg), GL_UNSIGNED_BYTE, strMsg);
	}
	if (Clip3.bSet)
	{
		glRasterPos3f (Clip3.ptClip[0]+2, Clip3.ptClip[1]+2, Clip3.ptClip[2]+2);
		strcpy (strMsg, "3");
		glCallLists (strlen(strMsg), GL_UNSIGNED_BYTE, strMsg);
	}
	if (Clip1.bSet && Clip2.bSet)
	{
		ProduceSplitLists();
		pList = ( (g_qeglobals.ViewType == XZ) ? !g_qeglobals.clipbSwitch : g_qeglobals.clipbSwitch) ? &g_qeglobals.brFrontSplits : &g_qeglobals.brBackSplits;

		for (pBrush = pList->next ; pBrush != NULL && pBrush != pList ; pBrush=pBrush->next)
		{
			glColor3f (1,1,0);
			for (face = pBrush->brush_faces,order = 0 ; face ; face=face->next, order++)
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
	if (g_qeglobals.ViewType != XY)
		glPopMatrix();
}


/*
==============
EndClipPoint
==============
*/
void EndClipPoint (void)
{
	if (pMovingClip)
	{
		pMovingClip = NULL;
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
	int		nDim1, nDim2;
	char	cView[20];

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	w = g_qeglobals.d_xyz.width/2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height/2 / g_qeglobals.d_xyz.scale;

	nDim1 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.ViewType == XY) ? 1 : 2;

	xb = g_qeglobals.d_xyz.origin[nDim1] - w;
	if (xb < region_mins[nDim1])
		xb = region_mins[nDim1];
	xb = 64 * floor (xb/64);

	xe = g_qeglobals.d_xyz.origin[nDim1] + w;
	if (xe > region_maxs[nDim1])
		xe = region_maxs[nDim1];
	xe = 64 * ceil (xe/64);

	yb = g_qeglobals.d_xyz.origin[nDim2] - h;
	if (yb < region_mins[nDim2])
		yb = region_mins[nDim2];
	yb = 64 * floor (yb/64);

	ye = g_qeglobals.d_xyz.origin[nDim2] + h;
	if (ye > region_maxs[nDim2])
		ye = region_maxs[nDim2];
	ye = 64 * ceil (ye/64);

	// draw major blocks

	glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR]);

	if ( g_qeglobals.d_showgrid )
	{
		
		glBegin (GL_LINES);
		
		for (x=xb ; x<=xe ; x+=64)
		{
			glVertex2f (x, yb);
			glVertex2f (x, ye);
		}
		for (y=yb ; y<=ye ; y+=64)
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

		for (x=xb ; x<xe ; x+=64)
		{
			glRasterPos2f (x, g_qeglobals.d_xyz.origin[nDim2] + h - 6/g_qeglobals.d_xyz.scale);
			sprintf (text, "%i",(int)x);
			glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
		}
		for (y=yb ; y<ye ; y+=64)
		{
			glRasterPos2f (g_qeglobals.d_xyz.origin[nDim1] - w + 1, y);
			sprintf (text, "%i",(int)y);
			glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
		}
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME]);

		glRasterPos2f ( g_qeglobals.d_xyz.origin[nDim1] - w + 35 / g_qeglobals.d_xyz.scale, g_qeglobals.d_xyz.origin[nDim2] + h - 20 / g_qeglobals.d_xyz.scale );
   
		if (g_qeglobals.ViewType == XY)
			strcpy(cView, "XY Top");
		else if (g_qeglobals.ViewType == XZ)
			strcpy(cView, "XZ Front");
		else
			strcpy(cView, "YZ Side");

		glCallLists (strlen(cView), GL_UNSIGNED_BYTE, cView);
	}

	// show current work zone?
	// the work zone is used to place dropped points and brushes
	if (g_qeglobals.d_show_work)
	{
		glColor3f( 1.0f, 0.0f, 0.0f );
		glBegin( GL_LINES );
		glVertex2f( xb, g_qeglobals.d_work_min[nDim2] );
		glVertex2f( xe, g_qeglobals.d_work_min[nDim2] );
		glVertex2f( xb, g_qeglobals.d_work_max[nDim2] );
		glVertex2f( xe, g_qeglobals.d_work_max[nDim2] );
		glVertex2f( g_qeglobals.d_work_min[nDim1], yb );
		glVertex2f( g_qeglobals.d_work_min[nDim1], ye );
		glVertex2f( g_qeglobals.d_work_max[nDim1], yb );
		glVertex2f( g_qeglobals.d_work_max[nDim1], ye );
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
	int nDim1, nDim2;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	w = g_qeglobals.d_xyz.width/2 / g_qeglobals.d_xyz.scale;
	h = g_qeglobals.d_xyz.height/2 / g_qeglobals.d_xyz.scale;

	nDim1 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.ViewType == XY) ? 1 : 2;

	xb = g_qeglobals.d_xyz.origin[nDim1] - w;
	if (xb < region_mins[nDim1])
		xb = region_mins[nDim1];
	xb = 1024 * floor (xb/1024);

	xe = g_qeglobals.d_xyz.origin[nDim1] + w;
	if (xe > region_maxs[nDim1])
		xe = region_maxs[nDim1];
	xe = 1024 * ceil (xe/1024);

	yb = g_qeglobals.d_xyz.origin[nDim2] - h;
	if (yb < region_mins[nDim2])
		yb = region_mins[nDim2];
	yb = 1024 * floor (yb/1024);

	ye = g_qeglobals.d_xyz.origin[nDim2] + h;
	if (ye > region_maxs[nDim2])
		ye = region_maxs[nDim2];
	ye = 1024 * ceil (ye/1024);

	// draw major blocks
	glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK]);
//	glColor3f(0,0,1);
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

	if (g_qeglobals.ViewType == XY)
	{
		x = g_qeglobals.d_camera.origin[0];
		y = g_qeglobals.d_camera.origin[1];
		a = g_qeglobals.d_camera.angles[YAW]/180*Q_PI;
	}
	else if (g_qeglobals.ViewType == YZ)
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

	if (g_qeglobals.ViewType == XY)
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

	if (pb->hiddenBrush)
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
		return (pb->owner->eclass->nShowFlags & ECLASS_LIGHT);
		//if (!strncmp(pb->owner->eclass->name, "light", 5))
		//	return TRUE;
	}

	if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS)
	{
		return (pb->owner->eclass->nShowFlags & ECLASS_PATH);
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
void PaintSizeInfo (int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds)
{

  vec3_t vSize;
  char	dimstr[128];

  VectorSubtract(vMaxBounds, vMinBounds, vSize);

  glColor3f(g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] * .65, 
            g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] * .65,
            g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] * .65);

  if (g_qeglobals.ViewType == XY)
  {
		glBegin (GL_LINES);

    glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale, 0.0f);
    glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);

    glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);
    glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);

    glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale, 0.0f);
    glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale, 0.0f);
  

    glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMinBounds[nDim2], 0.0f);
    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMinBounds[nDim2], 0.0f);

    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMinBounds[nDim2], 0.0f);
    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2], 0.0f);
  
    glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2], 0.0f);
    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2], 0.0f);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0  / g_qeglobals.d_xyz.scale, 0.0f);

    sprintf(dimstr, DimStrings[nDim1], vSize[nDim1]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
    glRasterPos3f (vMaxBounds[nDim1] + 16.0  / g_qeglobals.d_xyz.scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]), 0.0f);

    sprintf(dimstr, DimStrings[nDim2], vSize[nDim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

    glRasterPos3f (vMinBounds[nDim1] + 4, vMaxBounds[nDim2] + 8 / g_qeglobals.d_xyz.scale, 0.0f);

    sprintf(dimstr, OrgStrings[0], vMinBounds[nDim1], vMaxBounds[nDim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);


	}
	else if (g_qeglobals.ViewType == XZ)
	{
		glBegin (GL_LINES);

    glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);

    glVertex3f(vMinBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);
    glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);

    glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(vMaxBounds[nDim1], 0.0f, vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);
  

    glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, 0.0f, vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMinBounds[nDim2]);

    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMaxBounds[nDim2]);
  
    glVertex3f(vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, 0.0f, vMaxBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f / g_qeglobals.d_xyz.scale, 0.0f, vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), 0.0f, vMinBounds[nDim2] - 20.0  / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, DimStrings[nDim1], vSize[nDim1]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
    glRasterPos3f (vMaxBounds[nDim1] + 16.0  / g_qeglobals.d_xyz.scale, 0.0f, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    sprintf(dimstr, DimStrings[nDim2], vSize[nDim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

    glRasterPos3f (vMinBounds[nDim1] + 4, 0.0f, vMaxBounds[nDim2] + 8 / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, OrgStrings[1], vMinBounds[nDim1], vMaxBounds[nDim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

  }
  else
  {
		glBegin (GL_LINES);

    glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);

    glVertex3f(0.0f, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f  / g_qeglobals.d_xyz.scale);

    glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / g_qeglobals.d_xyz.scale);
    glVertex3f(0.0f, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / g_qeglobals.d_xyz.scale);
  

    glVertex3f(0.0f, vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMinBounds[nDim2]);
    glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f  / g_qeglobals.d_xyz.scale, vMinBounds[nDim2]);

    glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f  / g_qeglobals.d_xyz.scale, vMinBounds[nDim2]);
    glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f  / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2]);
  
    glVertex3f(0.0f, vMaxBounds[nDim1] + 6.0f  / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2]);
    glVertex3f(0.0f, vMaxBounds[nDim1] + 10.0f  / g_qeglobals.d_xyz.scale, vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (0.0f, Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0  / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, DimStrings[nDim1], vSize[nDim1]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);
    
    glRasterPos3f (0.0f, vMaxBounds[nDim1] + 16.0  / g_qeglobals.d_xyz.scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    sprintf(dimstr, DimStrings[nDim2], vSize[nDim2]);
	  glCallLists (strlen(dimstr), GL_UNSIGNED_BYTE, dimstr);

    glRasterPos3f (0.0f, vMinBounds[nDim1] + 4.0, vMaxBounds[nDim2] + 8 / g_qeglobals.d_xyz.scale);
    sprintf(dimstr, OrgStrings[2], vMinBounds[nDim1], vMaxBounds[nDim2]);
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
	int nDim1, nDim2;
	int nSaveDrawn;
	qboolean bFixedSize;
	vec3_t vMinBounds, vMaxBounds;


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

	nDim1 = (g_qeglobals.ViewType == YZ) ? 1 : 0;
	nDim2 = (g_qeglobals.ViewType == XY) ? 1 : 2;

	mins[0] = g_qeglobals.d_xyz.origin[nDim1] - w;
	maxs[0] = g_qeglobals.d_xyz.origin[nDim1] + w;
	mins[1] = g_qeglobals.d_xyz.origin[nDim2] - h;
	maxs[1] = g_qeglobals.d_xyz.origin[nDim2] + h;

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

	if (g_qeglobals.ViewType != XY)
	{
		glPushMatrix();
		if (g_qeglobals.ViewType == YZ)
			glRotatef (-90,  0, 1, 0);	    // put Z going up
		//else
			glRotatef (-90,  1, 0, 0);	    // put Z going up
	}

//	e = NULL;
	e = world_entity;
	for (brush = active_brushes.next ; brush != &active_brushes ; brush=brush->next)
	{
		if (brush->mins[nDim1] > maxs[0]
			|| brush->mins[nDim2] > maxs[1]
			|| brush->maxs[nDim1] < mins[0]
			|| brush->maxs[nDim2] < mins[1]	)
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
		Brush_DrawXY( brush, g_qeglobals.ViewType);
	}

	DrawPathLines ();

	//
	// draw pointfile
	//
	if ( g_qeglobals.d_pointfile_display_list)
		glCallList (g_qeglobals.d_pointfile_display_list);

	if (!(g_qeglobals.ViewType == XY))
		glPopMatrix();

	//
	// draw block grid
	//
	if ( g_qeglobals.d_show_blocks)
		XY_DrawBlockGrid ();

	//
	// now draw selected brushes
	//

	if (g_qeglobals.ViewType != XY)
	{
		glPushMatrix();
		if (g_qeglobals.ViewType == YZ)
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
	vMinBounds[0] = vMinBounds[1] = vMinBounds[2] = 8192.0f;
	vMaxBounds[0] = vMaxBounds[1] = vMaxBounds[2] = -8192.0f;

	nSaveDrawn = drawn;
	bFixedSize = false;
//

	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
	{
		drawn++;

		Brush_DrawXY( brush, g_qeglobals.ViewType );

// paint size

	    if (!bFixedSize)
	    {
			if (brush->owner->eclass->fixedsize)
				bFixedSize = true;
			if (g_qeglobals.d_savedinfo.check_sizepaint)
			{
				for (i = 0; i < 3; i++)
				{
					if (brush->mins[i] < vMinBounds[i])
						vMinBounds[i] = brush->mins[i];
					if (brush->maxs[i] > vMaxBounds[i])
						vMaxBounds[i] = brush->maxs[i];
				}
			}
		}
//
	}

	glDisable (GL_LINE_STIPPLE);
	glLineWidth (1);

// paint size

	if (!bFixedSize && drawn - nSaveDrawn > 0 && g_qeglobals.d_savedinfo.check_sizepaint)
		PaintSizeInfo(nDim1, nDim2, vMinBounds, vMaxBounds);
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

	if (!(g_qeglobals.ViewType == XY))
		glPopMatrix();

	// clipper
	if (g_qeglobals.clipmode)
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

