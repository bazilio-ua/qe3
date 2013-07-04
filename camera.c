
#include "qe3.h"

#define	PAGEFLIPS	2

/*
============
Cam_Init
============
*/
void Cam_Init (void)
{
//	g_qeglobals.d_camera.draw_mode = cd_texture;
//	g_qeglobals.d_camera.draw_mode = cd_solid;
//	g_qeglobals.d_camera.draw_mode = cd_wire;

	g_qeglobals.d_camera.timing = false;

	g_qeglobals.d_camera.origin[0] = 0;
	g_qeglobals.d_camera.origin[1] = 20;
	g_qeglobals.d_camera.origin[2] = 46;

	g_qeglobals.d_camera.color[0] = (float)0.3;
	g_qeglobals.d_camera.color[1] = (float)0.3;
	g_qeglobals.d_camera.color[2] = (float)0.3;

//	g_qeglobals.d_camera.color[0] = 0.3;
//	g_qeglobals.d_camera.color[1] = 0.3;
//	g_qeglobals.d_camera.color[2] = 0.3;
}


//============================================================================

void Cam_BuildMatrix (void)
{
	float	xa, ya;
	float	matrix[4][4];
	int		i;

	xa = g_qeglobals.d_camera.angles[0]/180*Q_PI;
	ya = g_qeglobals.d_camera.angles[1]/180*Q_PI;

	// the movement matrix is kept 2d

    g_qeglobals.d_camera.forward[0] = cos(ya);
    g_qeglobals.d_camera.forward[1] = sin(ya);
    g_qeglobals.d_camera.right[0] = g_qeglobals.d_camera.forward[1];
    g_qeglobals.d_camera.right[1] = -g_qeglobals.d_camera.forward[0];

	glGetFloatv (GL_PROJECTION_MATRIX, &matrix[0][0]);

	for (i=0 ; i<3 ; i++)
	{
		g_qeglobals.d_camera.vright[i] = matrix[i][0];
		g_qeglobals.d_camera.vup[i] = matrix[i][1];
		g_qeglobals.d_camera.vpn[i] = matrix[i][2];
	}

	VectorNormalize (g_qeglobals.d_camera.vright);
	VectorNormalize (g_qeglobals.d_camera.vup);
	VectorNormalize (g_qeglobals.d_camera.vpn);
}

//===============================================

/*
===============
Cam_ChangeFloor
===============
*/
void Cam_ChangeFloor (qboolean up)
{
	brush_t	*b;
	float	d, bestd, current;
	vec3_t	start, dir;

	start[0] = g_qeglobals.d_camera.origin[0];
	start[1] = g_qeglobals.d_camera.origin[1];
	start[2] = 8192;
	dir[0] = dir[1] = 0;
	dir[2] = -1;

	current = 8192 - (g_qeglobals.d_camera.origin[2] - 48);
	if (up)
		bestd = 0;
	else
		bestd = 16384;

	for (b=active_brushes.next ; b != &active_brushes ; b=b->next)
	{
		if (!Brush_Ray (start, dir, b, &d))
			continue;
		if (up && d < current && d > bestd)
			bestd = d;
		if (!up && d > current && d < bestd)
			bestd = d;
	}

	if (bestd == 0 || bestd == 16384)
		return;

	g_qeglobals.d_camera.origin[2] += current - bestd;
	Sys_UpdateWindows (W_CAMERA|W_Z_OVERLAY);
}


//===============================================

int	cambuttonstate;
static	int	buttonx, buttony;
static	int	cursorx, cursory;

face_t	*side_select;

#define	ANGLE_SPEED	300
#define	MOVE_SPEED	400

/*
================
Cam_PositionDrag
================
*/
void Cam_PositionDrag (void)
{
	int		x, y;

	SetCursor(NULL);
	Sys_GetCursorPos (&x, &y);
	if (x != cursorx || y != cursory)
	{
		x -= cursorx;
		VectorMA (g_qeglobals.d_camera.origin, x, g_qeglobals.d_camera.vright, g_qeglobals.d_camera.origin);
		y -= cursory;
		g_qeglobals.d_camera.origin[2] -= y;

		Sys_SetCursorPos (cursorx, cursory);
		Sys_UpdateWindows (W_CAMERA | W_XY_OVERLAY);
	}
}


/*
================
Cam_PositionRotate
================
*/
void PositionRotate (int x, int y, vec3_t origin)
{
	vec3_t		work, forward, dir, vecdist;
	vec_t		distance;
	int			i;


	for(i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((g_qeglobals.d_camera.origin[i] - origin[i]));
		vecdist[i] *= vecdist[i];
	}

	g_qeglobals.d_camera.viewdistance = distance = sqrt(vecdist[0]+vecdist[1]+vecdist[2]);
	VectorSubtract (g_qeglobals.d_camera.origin, origin, work);
	VectorToAngles(work, g_qeglobals.d_camera.angles);

	if(g_qeglobals.d_camera.angles[PITCH] > 100)
		g_qeglobals.d_camera.angles[PITCH] -= 360;

	g_qeglobals.d_camera.angles[PITCH] -= y;
	
	if(g_qeglobals.d_camera.angles[PITCH] > 85)
		g_qeglobals.d_camera.angles[PITCH] = 85;

	if(g_qeglobals.d_camera.angles[PITCH] < -85)
		g_qeglobals.d_camera.angles[PITCH] = -85;

	g_qeglobals.d_camera.angles[YAW] -= x;

	AngleVectors(g_qeglobals.d_camera.angles, forward, NULL, NULL);
	forward[2] = -forward[2];
	VectorMA (origin, distance, forward, g_qeglobals.d_camera.origin);

	VectorSubtract (origin, g_qeglobals.d_camera.origin, dir);
	VectorNormalize (dir);
	g_qeglobals.d_camera.angles[1] = atan2(dir[1], dir[0])*180/Q_PI;
	g_qeglobals.d_camera.angles[0] = asin(dir[2])*180/Q_PI;

	Cam_BuildMatrix();
	Sys_SetCursorPos (cursorx, cursory);

	Sys_UpdateWindows (W_XY|W_CAMERA|W_Z);
}

void Cam_PositionRotate (void)
{
	int			x, y, i, j;
	vec3_t		mins, maxs, forward, vecdist;
	vec3_t		origin;
	brush_t		*b;
	face_t		*f;

	SetCursor(NULL);
	Sys_GetCursorPos (&x, &y);

	if (x == cursorx && y == cursory)
		return;
	
	x -= cursorx;
	y -= cursory;

	if(selected_brushes.next != &selected_brushes)
	{
		mins[0] = mins[1] = mins[2] = 99999;
		maxs[0] = maxs[1] = maxs[2] = -99999;
		for(b = selected_brushes.next; b != &selected_brushes; b = b->next)
		{
			for(i = 0; i < 3; i++)
			{
				if(b->maxs[i] > maxs[i])
					maxs[i] = b->maxs[i];
				if(b->mins[i] < mins[i])
					mins[i] = b->mins[i];
			}
		}
		origin[0] = (mins[0] + maxs[0])/2;
		origin[1] = (mins[1] + maxs[1])/2;
		origin[2] = (mins[2] + maxs[2])/2;
	}
	else if(selected_face)
	{
		mins[0] = mins[1] = mins[2] = 99999;
		maxs[0] = maxs[1] = maxs[2] = -99999;

		f = selected_face;
		for ( j = 0 ; j < f->face_winding->numpoints; j++)
		{
			for(i = 0; i < 3; i++)
			{
				if(f->face_winding->points[j][i] > maxs[i])
					maxs[i] = f->face_winding->points[j][i];
				if(f->face_winding->points[j][i] < mins[i])
					mins[i] = f->face_winding->points[j][i];
			}
		}

		origin[0] = (mins[0] + maxs[0])/2;
		origin[1] = (mins[1] + maxs[1])/2;
		origin[2] = (mins[2] + maxs[2])/2;
	}
	else
	{
		AngleVectors(g_qeglobals.d_camera.angles, forward, NULL, NULL);
		forward[2] = -forward[2];
		VectorMA (g_qeglobals.d_camera.origin, g_qeglobals.d_camera.viewdistance, forward, origin);
	}

	for(i = 0; i < 3; i++)
	{
		vecdist[i] = fabs((g_qeglobals.d_camera.origin[i] - origin[i]));
		vecdist[i] *= vecdist[i];
	}

	PositionRotate(x, y, origin);
	return;
}

/*
================
Cam_FreeLook
================
*/
void Cam_FreeLook (void)
{
	int			x, y;

	SetCursor(NULL);
	Sys_GetCursorPos (&x, &y);

	if (x == cursorx && y == cursory)
		return;
	
	x -= cursorx;
	y -= cursory;

	if(g_qeglobals.d_camera.angles[PITCH] > 100)
		g_qeglobals.d_camera.angles[PITCH] -= 360;

	g_qeglobals.d_camera.angles[PITCH] -= y;
	
	if(g_qeglobals.d_camera.angles[PITCH] > 85)
		g_qeglobals.d_camera.angles[PITCH] = 85;

	if(g_qeglobals.d_camera.angles[PITCH] < -85)
		g_qeglobals.d_camera.angles[PITCH] = -85;

	g_qeglobals.d_camera.angles[YAW] -= x;

	Sys_SetCursorPos (cursorx, cursory);
	Sys_UpdateWindows (W_XY|W_CAMERA|W_Z);
}

/*
===============
Cam_MouseControl
===============
*/
void Cam_MouseControl (float dtime)
{
	int		xl, xh;
	int		yl, yh;
	float	xf, yf;

	if (cambuttonstate != MK_RBUTTON)
		return;

	xf = (float)(buttonx - g_qeglobals.d_camera.width/2) / (g_qeglobals.d_camera.width/2);
	yf = (float)(buttony - g_qeglobals.d_camera.height/2) / (g_qeglobals.d_camera.height/2);

	xl = g_qeglobals.d_camera.width/3;
	xh = xl*2;
	yl = g_qeglobals.d_camera.height/3;
	yh = yl*2;

#if 0
	// strafe
	if (buttony < yl && (buttonx < xl || buttonx > xh))
		VectorMA (g_qeglobals.d_camera.origin, xf*dtime*MOVE_SPEED, g_qeglobals.d_camera.right, g_qeglobals.d_camera.origin);
	else
#endif
	{
		xf *= 1.0 - fabs(yf);
		if (xf < 0)
		{
			xf += (float)0.1;
			if (xf > 0)
				xf = 0;
		}
		else
		{
			xf -= (float)0.1;
			if (xf < 0)
				xf = 0;
		}
		
		VectorMA (g_qeglobals.d_camera.origin, yf*dtime*MOVE_SPEED, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);
		g_qeglobals.d_camera.angles[YAW] += xf*-dtime*ANGLE_SPEED;
	}
	Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
}




/*
==============
Cam_MouseDown
==============
*/
void Cam_MouseDown (int x, int y, int buttons)
{
	vec3_t		dir;
	float		f, r, u;
	int			i;

	//
	// calc ray direction
	//
	u = (float)(y - g_qeglobals.d_camera.height/2) / (g_qeglobals.d_camera.width/2);
	r = (float)(x - g_qeglobals.d_camera.width/2) / (g_qeglobals.d_camera.width/2);
	f = 1;

	for (i=0 ; i<3 ; i++)
		dir[i] = g_qeglobals.d_camera.vpn[i] * f + g_qeglobals.d_camera.vright[i] * r + g_qeglobals.d_camera.vup[i] * u;
	VectorNormalize (dir);

	Sys_GetCursorPos (&cursorx, &cursory);

	cambuttonstate = buttons;
	buttonx = x;
	buttony = y;

	// LBUTTON = manipulate selection
	// shift-LBUTTON = select
	// middle button = grab texture
	// ctrl-middle button = set entire brush to texture
	// ctrl-shift-middle button = set single face to texture
	if ( (buttons == MK_LBUTTON)
		|| (buttons == (MK_LBUTTON | MK_SHIFT))
		|| (buttons == (MK_LBUTTON | MK_CONTROL))
		|| (buttons == (MK_LBUTTON | MK_CONTROL | MK_SHIFT))
		|| (buttons == MK_MBUTTON)
		|| (buttons == (MK_MBUTTON|MK_CONTROL))
		|| (buttons == (MK_MBUTTON|MK_SHIFT|MK_CONTROL)) )
	{
		Drag_Begin (x, y, buttons, 
			g_qeglobals.d_camera.vright, g_qeglobals.d_camera.vup,
			g_qeglobals.d_camera.origin, dir);
		return;
	}

	if (buttons == MK_RBUTTON)
	{
		SetCursor(NULL);
		Cam_MouseControl ((float)0.1);
		return;
	}
}

/*
==============
Cam_MouseUp
==============
*/
void Cam_MouseUp (int x, int y, int buttons)
{
	cambuttonstate = 0;
	Drag_MouseUp ();
}


/*
==============
Cam_MouseMoved
==============
*/
void Cam_MouseMoved (int x, int y, int buttons)
{
	trace_t		t;
	vec3_t		dir;
	float		f, r, u;
	int			i;
	char		camstring[256];

	cambuttonstate = buttons;
	if (!buttons)
	{
		//
		// calc ray direction
		//
		u = (float)(y - g_qeglobals.d_camera.height/2) / (g_qeglobals.d_camera.width/2);
		r = (float)(x - g_qeglobals.d_camera.width/2) / (g_qeglobals.d_camera.width/2);
		f = 1;

		for (i=0 ; i<3 ; i++)
			dir[i] = g_qeglobals.d_camera.vpn[i] * f + g_qeglobals.d_camera.vright[i] * r + g_qeglobals.d_camera.vup[i] * u;
		VectorNormalize (dir);
		t = Test_Ray (g_qeglobals.d_camera.origin, dir, false);

		if (t.brush)
		{
			brush_t	*b;
			int		i;
			vec3_t mins, maxs, size;

			for (i=0 ; i<3 ; i++)
			{
				mins[i] = 99999;
				maxs[i] = -99999;
			}

			b = t.brush;
			for (i=0 ; i<3 ; i++)
			{
				if (b->mins[i] < mins[i])
					mins[i] = b->mins[i];
				if (b->maxs[i] > maxs[i])
					maxs[i] = b->maxs[i];
			}

			VectorSubtract(maxs, mins, size);
			if (t.brush->owner->eclass->fixedsize)
				sprintf (camstring, "%s (%i %i %i)", t.brush->owner->eclass->name, (int)size[0], (int)size[1], (int)size[2]);
			else
				sprintf (camstring, "%s (%i %i %i) %s", t.brush->owner->eclass->name, (int)size[0], (int)size[1], (int)size[2], t.face->texdef.name);
		}
		else
			sprintf (camstring, "");

		Sys_Status (camstring, 0);

		return;
	}

	buttonx = x;
	buttony = y;

	if (buttons == (MK_RBUTTON|MK_CONTROL) )
	{
		Cam_PositionDrag ();
		Sys_UpdateWindows (W_XY|W_CAMERA|W_Z);
		return;
	}

	if (buttons == (MK_RBUTTON|MK_SHIFT))
	{
		Cam_PositionRotate();
		Sys_UpdateWindows (W_XY|W_CAMERA|W_Z);
		return;
	}

	if (buttons == (MK_MBUTTON|MK_SHIFT))
	{
		Cam_FreeLook();
		Sys_UpdateWindows (W_XY|W_CAMERA|W_Z);
		return;
	}

	Sys_GetCursorPos (&cursorx, &cursory);

	if (buttons & (MK_LBUTTON | MK_MBUTTON) )
	{
		Drag_MouseMoved (x, y, buttons);
		Sys_UpdateWindows (W_XY|W_CAMERA|W_Z);
	}
}


vec3_t	cull1, cull2;
int		cullv1[3], cullv2[3];

void InitCull (void)
{
	int		i;

	VectorSubtract (g_qeglobals.d_camera.vpn, g_qeglobals.d_camera.vright, cull1);
	VectorAdd (g_qeglobals.d_camera.vpn, g_qeglobals.d_camera.vright, cull2);

	for (i=0 ; i<3 ; i++)
	{
		if (cull1[i] > 0)
			cullv1[i] = 3+i;
		else
			cullv1[i] = i;
		if (cull2[i] > 0)
			cullv2[i] = 3+i;
		else
			cullv2[i] = i;
	}
}

qboolean CullBrush (brush_t *b)
{
	int		i;
	vec3_t	point;
	float	d;

	for (i=0 ; i<3 ; i++)
		point[i] = b->mins[cullv1[i]] - g_qeglobals.d_camera.origin[i];

	d = DotProduct (point, cull1);
	if (d < -1)
		return true;

	for (i=0 ; i<3 ; i++)
		point[i] = b->mins[cullv2[i]] - g_qeglobals.d_camera.origin[i];

	d = DotProduct (point, cull2);
	if (d < -1)
		return true;

	return false;
}


void DrawClipSplits (void)
{
	g_qeglobals.pSplitList = NULL;
	if (g_qeglobals.clipmode)
	{
		if (Clip1.bSet && Clip2.bSet)
		{
			g_qeglobals.pSplitList = ( (g_qeglobals.ViewType == XZ) ? !g_qeglobals.clipbSwitch : g_qeglobals.clipbSwitch) ? &g_qeglobals.brFrontSplits : &g_qeglobals.brBackSplits;
		}
	}
}


/*
==============
Cam_Draw
==============
*/
void Cam_Draw (void)
{
    brush_t	*brush;
	face_t	*face;
	float	screenaspect;
	float	yfov;
	double	start, end;
	int		i;
	brush_t *pList;

	if (!active_brushes.next)
		return;	// not valid yet

	if (g_qeglobals.d_camera.timing)
		start = Sys_DoubleTime ();

	//
	// clear
	//
	QE_CheckOpenGLForErrors();

	glViewport(0, 0, g_qeglobals.d_camera.width, g_qeglobals.d_camera.height);
	glScissor(0, 0, g_qeglobals.d_camera.width, g_qeglobals.d_camera.height);
	glClearColor (
		g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][0],
		g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][1],
		g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][2],
		0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//
	// set up viewpoint
	//
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();

    screenaspect = (float)g_qeglobals.d_camera.width/g_qeglobals.d_camera.height;
	yfov = 2*atan((float)g_qeglobals.d_camera.height/g_qeglobals.d_camera.width)*180/Q_PI;
    gluPerspective (yfov,  screenaspect,  2,  8192);

    glRotatef (-90,  1, 0, 0);	    // put Z going up
    glRotatef (90,  0, 0, 1);	    // put Z going up
    glRotatef (g_qeglobals.d_camera.angles[0],  0, 1, 0);
    glRotatef (-g_qeglobals.d_camera.angles[1],  0, 0, 1);
    glTranslatef (-g_qeglobals.d_camera.origin[0],  -g_qeglobals.d_camera.origin[1],  -g_qeglobals.d_camera.origin[2]);

	Cam_BuildMatrix ();

	InitCull ();

	//
	// draw stuff
	//

	switch (g_qeglobals.d_camera.draw_mode)
	{
	case cd_wire:
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	    glDisable(GL_TEXTURE_2D);
	    glDisable(GL_TEXTURE_1D);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
	    glColor3f(1.0, 1.0, 1.0);
//		glEnable (GL_LINE_SMOOTH);
		break;

	case cd_solid:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		glShadeModel (GL_FLAT);

		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_TEXTURE_2D);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc (GL_LEQUAL);
		break;

	case cd_texture:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);

		glShadeModel (GL_FLAT);

		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc (GL_LEQUAL);

#if 0

		{
	   GLfloat fogColor[4] = {0.0, 1.0, 0.0, 0.25};

		glFogi (GL_FOG_MODE, GL_LINEAR);
		glHint (GL_FOG_HINT, GL_NICEST);  /*  per pixel   */
		glFogf (GL_FOG_START, -8192);
		glFogf (GL_FOG_END, 65536);
	   glFogfv (GL_FOG_COLOR, fogColor);
 
		}

#endif
		break;

	case cd_blend:
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);

		glShadeModel (GL_FLAT);

		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glEnable(GL_TEXTURE_2D);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glDisable(GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	}

	glMatrixMode(GL_TEXTURE);
	for (brush = active_brushes.next ; brush != &active_brushes ; brush=brush->next)
	{
		if (CullBrush (brush))
			continue;
		if (FilterBrush (brush))
			continue;

		Brush_Draw( brush );
	}
	glMatrixMode(GL_PROJECTION);

	//
	// now draw selected brushes
	//

	glTranslatef (g_qeglobals.d_select_translate[0], g_qeglobals.d_select_translate[1], g_qeglobals.d_select_translate[2]);
	glMatrixMode(GL_TEXTURE);

	DrawClipSplits ();

	pList = (g_qeglobals.clipmode && g_qeglobals.pSplitList) ? g_qeglobals.pSplitList : &selected_brushes;

/*
	// draw normally
	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
	{
		Brush_Draw( brush );
	}
*/
	// draw normally
	for (brush = pList->next ; brush != pList ; brush=brush->next)
	{
		Brush_Draw(brush);
	}

	// blend on top
	glMatrixMode(GL_PROJECTION);

	glColor4f((float)1.0, (float)0.0, (float)0.0, (float)0.3);
	glEnable (GL_BLEND);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable (GL_TEXTURE_2D);

	for (brush = pList->next ; brush != pList ; brush=brush->next)
	{
		for (face=brush->brush_faces ; face ; face=face->next)
			Face_Draw( face );
	}

/*
	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
		for (face=brush->brush_faces ; face ; face=face->next)
			Face_Draw( face );
*/

	if (selected_face)
		Face_Draw(selected_face);

	// non-zbuffered outline

	glDisable (GL_BLEND);
	glDisable (GL_DEPTH_TEST);
	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glColor3f (1, 1, 1);

	for (brush = pList->next ; brush != pList ; brush=brush->next)
	{
		for (face=brush->brush_faces ; face ; face=face->next)
			Face_Draw( face );
	}

/*
	for (brush = selected_brushes.next ; brush != &selected_brushes ; brush=brush->next)
		for (face=brush->brush_faces ; face ; face=face->next)
			Face_Draw( face );
*/

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

	//
	// draw pointfile
	//
	glEnable(GL_DEPTH_TEST);

	DrawPathLines ();

	if (g_qeglobals.d_pointfile_display_list)
	{
		Pointfile_Draw();
//		glCallList (g_qeglobals.d_pointfile_display_list);
	}

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture( GL_TEXTURE_2D, 0 );

    glFinish();
	QE_CheckOpenGLForErrors();
//	Sys_EndWait();
	if (g_qeglobals.d_camera.timing)
	{
		end = Sys_DoubleTime ();
		Sys_Printf ("Camera: %i ms\n", (int)(1000*(end-start)));
	}
}

