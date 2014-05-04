// z.c

#include "qe3.h"

//#define	PAGEFLIPS	2

/*
============
Z_Init
============
*/
void Z_Init (void)
{
	g_qeglobals.d_z.origin[0] = 0;
	g_qeglobals.d_z.origin[1] = 20;
	g_qeglobals.d_z.origin[2] = 46;

	g_qeglobals.d_z.scale = 1;
}



/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

static	int	cursorx, cursory;

/*
==============
Z_MouseDown
==============
*/
void Z_MouseDown (int x, int y, int buttons)
{
	vec3_t	org, dir, vup, vright;
	brush_t	*b;

	Sys_GetCursorPos (&cursorx, &cursory);

	vup[0] = 0; vup[1] = 0; vup[2] = 1/g_qeglobals.d_z.scale;

	VectorCopy (g_qeglobals.d_z.origin, org);
	org[2] += (y - (g_qeglobals.d_z.height/2))/g_qeglobals.d_z.scale;
	org[1] = -8192;

	b = selected_brushes.next;
	if (b != &selected_brushes)
	{
		org[0] = (b->mins[0] + b->maxs[0])/2;
	}

	dir[0] = 0; dir[1] = 1; dir[2] = 0;

	vright[0] = 0; vright[1] = 0; vright[2] = 0;

	// LBUTTON = manipulate selection
	// shift-LBUTTON = select
	// middle button = grab texture
	// ctrl-middle button = set entire brush to texture
	// ctrl-shift-middle button = set single face to texture
	if ( (buttons == MK_LBUTTON)
		|| (buttons == (MK_LBUTTON | MK_SHIFT))
		|| (buttons == MK_MBUTTON)
//		|| (buttons == (MK_MBUTTON|MK_CONTROL))
		|| (buttons == (MK_MBUTTON|MK_SHIFT|MK_CONTROL)) )
	{
		Drag_Begin (x, y, buttons,
			vright, vup,
			org, dir);
		return;
	}

	// control mbutton = move camera
	if ((buttons == (MK_CONTROL|MK_MBUTTON) ) || (buttons == (MK_CONTROL|MK_LBUTTON)))
	{	
		g_qeglobals.d_camera.origin[2] = org[2] ;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z);
	}


}

/*
==============
Z_MouseUp
==============
*/
void Z_MouseUp (int x, int y, int buttons)
{
	Drag_MouseUp ();
}

/*
==============
Z_MouseMoved
==============
*/
void Z_MouseMoved (int x, int y, int buttons)
{
	char	zstring[256];
	float	fz;

	if (!buttons)
	{
		fz = g_qeglobals.d_z.origin[2] + (y - (g_qeglobals.d_z.height/2)) / g_qeglobals.d_z.scale;
		fz = floor(fz / g_qeglobals.d_gridsize + 0.5) * g_qeglobals.d_gridsize;

		sprintf (zstring, "z Coordinate: (%i)", (int)fz);
		Sys_Status (zstring, 0);

		return;
	}

	if (buttons == MK_LBUTTON)
	{
		Drag_MouseMoved (x, y, buttons);
		Sys_UpdateWindows (W_Z|W_CAMERA);
		return;
	}
	// rbutton = drag z origin
	if (buttons == MK_RBUTTON)
	{
		SetCursor(NULL);
		Sys_GetCursorPos (&x, &y);
		if ( y != cursory)
		{
			g_qeglobals.d_z.origin[2] += y-cursory;

			Sys_SetCursorPos (cursorx, cursory);
			Sys_UpdateWindows (W_Z);

			sprintf (zstring, "z Origin: (%i)", (int)g_qeglobals.d_z.origin[2]);
			Sys_Status (zstring, 0);
		}
		return;
	}
		// control mbutton = move camera
	if ((buttons == (MK_CONTROL|MK_MBUTTON) ) || (buttons == (MK_CONTROL|MK_LBUTTON)))
	{	
		g_qeglobals.d_camera.origin[2] = (y - (g_qeglobals.d_z.height/2))/g_qeglobals.d_z.scale;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z);
	}

}


/*
============================================================================

DRAWING

============================================================================
*/


/*
==============
Z_DrawGrid
==============
*/
void Z_DrawGrid (void)
{
	float	zz, zb, ze;
	int		w, h;
	char	text[32];
	int		size;

	if (g_qeglobals.d_gridsize == 128)
		size = 128;
	else if (g_qeglobals.d_gridsize == 256)
		size = 256;
	else
		size = 64;

	w = g_qeglobals.d_z.width/2 / g_qeglobals.d_z.scale;
	h = g_qeglobals.d_z.height/2 / g_qeglobals.d_z.scale;

	zb = g_qeglobals.d_z.origin[2] - h;
	if (zb < region_mins[2])
		zb = region_mins[2];
	zb = size * floor (zb/size);

	ze = g_qeglobals.d_z.origin[2] + h;
	if (ze > region_maxs[2])
		ze = region_maxs[2];
	ze = size * ceil (ze/size);
	
	// draw major blocks
	if ( g_qeglobals.d_showgrid )
	{
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR]);
		
		glBegin (GL_LINES);
		glVertex2f (0, zb);
		glVertex2f (0, ze);
		for (zz=zb ; zz<ze ; zz+=size)
		{
			glVertex2f (-w, zz);
			glVertex2f (w, zz);
		}
		glEnd ();
	}
	
	// draw minor blocks
	if ( g_qeglobals.d_showgrid && g_qeglobals.d_gridsize*g_qeglobals.d_z.scale >= 4)
	{
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR]);

		glBegin (GL_LINES);
		for (zz=zb ; zz<ze ; zz+=g_qeglobals.d_gridsize)
		{
			if ( ! ((int)zz & 63) )
				continue;
			glVertex2f (-w, zz);
			glVertex2f (w, zz);
		}
		glEnd ();
	}
	
	// draw coordinate text if needed
	if ( g_qeglobals.d_savedinfo.show_coordinates)
	{
//		glColor4f(0, 0, 0, 0);
		glColor3fv(g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT]);
		
		for (zz=zb ; zz<ze ; zz+=size)
		{
			glRasterPos2f (-w+1, zz);
			sprintf (text, "%i",(int)zz);
			glCallLists (strlen(text), GL_UNSIGNED_BYTE, text);
		}
	}
}

#define CAM_HEIGHT		48 // height of main part
#define CAM_GIZMO		8	// height of the gizmo

void ZDrawCameraIcon (void)
{
	float	x, y;
	int	xCam = g_qeglobals.d_z.width/4;

	x = 0;
	y = g_qeglobals.d_camera.origin[2];

	glColor3f (0.0, 0.0, 1.0);
	glBegin(GL_LINE_STRIP);
	glVertex3f (x-xCam,y,0);
	glVertex3f (x,y+CAM_GIZMO,0);
	glVertex3f (x+xCam,y,0);
	glVertex3f (x,y-CAM_GIZMO,0);
	glVertex3f (x-xCam,y,0);
	glVertex3f (x+xCam,y,0);
	glVertex3f (x+xCam,y-CAM_HEIGHT,0);
	glVertex3f (x-xCam,y-CAM_HEIGHT,0);
	glVertex3f (x-xCam,y,0);
	glEnd ();

}

GLbitfield glbitClear = GL_COLOR_BUFFER_BIT; //HACK

/*
==============
Z_Draw
==============
*/
void Z_Draw (void)
{
    brush_t	*brush;
	float	w, h;
	double	start, end;
	qtexture_t	*q;
	float	top, bottom;
	vec3_t	org_top, org_bottom, dir_up, dir_down;
	int xCam = g_qeglobals.d_z.width/3;

	if (!active_brushes.next)
		return;	// not valid yet

	if (g_qeglobals.d_z.timing)
		start = Sys_DoubleTime ();

	//
	// clear
	//
	glViewport(0, 0, g_qeglobals.d_z.width, g_qeglobals.d_z.height);

	glClearColor (
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][0],
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][1],
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][2],
		0);

    /* GL Bug */ 
	/* When not using hw acceleration, gl will fault if we clear the depth 
	buffer bit on the first pass. The hack fix is to set the GL_DEPTH_BUFFER_BIT
	only after Z_Draw() has been called once. Yeah, right. */
	glClear(glbitClear); 
	glbitClear |= GL_DEPTH_BUFFER_BIT;

	glMatrixMode(GL_PROJECTION);

    glLoadIdentity ();
	w = g_qeglobals.d_z.width/2 / g_qeglobals.d_z.scale;
	h = g_qeglobals.d_z.height/2 / g_qeglobals.d_z.scale;
	glOrtho (-w, w, g_qeglobals.d_z.origin[2]-h, g_qeglobals.d_z.origin[2]+h, -8, 8);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);


	//
	// now draw the grid
	//
	Z_DrawGrid ();

	//
	// draw stuff
	//

	glDisable(GL_CULL_FACE);

	glShadeModel (GL_FLAT);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);


	// draw filled interiors and edges
	dir_up[0] = 0 ; dir_up[1] = 0; dir_up[2] = 1;
	dir_down[0] = 0 ; dir_down[1] = 0; dir_down[2] = -1;
	VectorCopy (g_qeglobals.d_z.origin, org_top);
	org_top[2] = 4096;
	VectorCopy (g_qeglobals.d_z.origin, org_bottom);
	org_bottom[2] = -4096;

	for (brush = active_brushes.next ; brush != &active_brushes ; brush=brush->next)
	{
		if (brush->mins[0] >= g_qeglobals.d_z.origin[0]
			|| brush->maxs[0] <= g_qeglobals.d_z.origin[0]
			|| brush->mins[1] >= g_qeglobals.d_z.origin[1]
			|| brush->maxs[1] <= g_qeglobals.d_z.origin[1])
			continue;

		if (!Brush_Ray (org_top, dir_down, brush, &top))
			continue;
		top = org_top[2] - top;
		if (!Brush_Ray (org_bottom, dir_up, brush, &bottom))
			continue;
		bottom = org_bottom[2] + bottom;

		q = Texture_ForName (brush->brush_faces->texdef.name);
		glColor3f (q->color[0], q->color[1], q->color[2]);
		glBegin (GL_QUADS);
		glVertex2f (-xCam, bottom);
		glVertex2f (xCam, bottom);
		glVertex2f (xCam, top);
		glVertex2f (-xCam, top);
		glEnd ();

		glColor3f (1,1,1);
		glBegin (GL_LINE_LOOP);
		glVertex2f (-xCam, bottom);
		glVertex2f (xCam, bottom);
		glVertex2f (xCam, top);
		glVertex2f (-xCam, top);
		glEnd ();
	}

	//
	// now draw selected brushes
	//
	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
	{
		if ( !(brush->mins[0] >= g_qeglobals.d_z.origin[0]
			|| brush->maxs[0] <= g_qeglobals.d_z.origin[0]
			|| brush->mins[1] >= g_qeglobals.d_z.origin[1]
			|| brush->maxs[1] <= g_qeglobals.d_z.origin[1]) )
		{
			if (Brush_Ray (org_top, dir_down, brush, &top))
			{
				top = org_top[2] - top;
				if (Brush_Ray (org_bottom, dir_up, brush, &bottom))
				{
					bottom = org_bottom[2] + bottom;

					q = Texture_ForName (brush->brush_faces->texdef.name);
					glColor3f (q->color[0], q->color[1], q->color[2]);
					glBegin (GL_QUADS);
					glVertex2f (-xCam, bottom);
					glVertex2f (xCam, bottom);
					glVertex2f (xCam, top);
					glVertex2f (-xCam, top);
					glEnd ();
				}
			}
		}

		glColor3f (1,0,0);
		glBegin (GL_LINE_LOOP);
		glVertex2f (-xCam, brush->mins[2]);
		glVertex2f (xCam, brush->mins[2]);
		glVertex2f (xCam, brush->maxs[2]);
		glVertex2f (-xCam, brush->maxs[2]);
		glEnd ();
	}


	ZDrawCameraIcon ();

    glFinish();
	QE_CheckOpenGLForErrors();

	if (g_qeglobals.d_z.timing)
	{
		end = Sys_DoubleTime ();
		Sys_Printf ("z: %i ms\n", (int)(1000*(end-start)));
	}
}

