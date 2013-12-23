
#include "qe3.h"

QEGlobals_t  g_qeglobals;

void QE_CheckOpenGLForErrors(void)
{
    int	    i;
    
    while ( ( i = glGetError() ) != GL_NO_ERROR )
    {
		char buffer[100];

		sprintf( buffer, "OpenGL Error: %s", gluErrorString( i ) );

		MessageBox( g_qeglobals.d_hwndMain, buffer , "QuakeEd Error", MB_OK | MB_ICONEXCLAMATION );
		exit( 1 );
    }
}


char *ExpandReletivePath (char *p)
{
	static char	temp[1024];
	char	*base;

	if (!p || !p[0])
		return NULL;
	if (p[0] == '/' || p[0] == '\\')
		return p;

	base = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	sprintf (temp, "%s/%s", base, p);
	return temp;
}



void *qmalloc (int size)
{
	void *b;
	b = malloc(size);
	memset (b, 0, size);
	return b;
}

char *copystring (char *s)
{
	char	*b;
	b = malloc(strlen(s)+1);
	strcpy (b,s);
	return b;
}

/*
===============
QE_CheckAutoSave

If five minutes have passed since making a change
and the map hasn't been saved, save it out.
===============
*/
void QE_CheckAutoSave( void )
{
	static clock_t s_start;
	clock_t        now;

	now = clock();

	if ( modified != 1 || !s_start)
	{
		s_start = now;
		return;
	}

	if ( now - s_start > ( CLOCKS_PER_SEC * 60 * QE_AUTOSAVE_INTERVAL ) )
	{
		Sys_Printf ("Autosaving...\n");
		Sys_Status ("Autosaving...", 0);

		Map_SaveFile (ValueForKey(g_qeglobals.d_project_entity, "autosave"), false);

		Sys_Printf ("Autosaving successfull.\n");
		Sys_Status ("Autosaving successfull.", 0);

		modified = 2;
		s_start = now;
	}
}



/*
===========
QE_LoadProject
===========
*/
qboolean QE_LoadProject (char *projectfile)
{
	char	*data;

	Sys_Printf ("QE_LoadProject (%s)\n", projectfile);

	if ( LoadFileNoCrash (projectfile, (void *)&data) == -1)
		return false;
	StartTokenParsing (data);
	g_qeglobals.d_project_entity = Entity_Parse (true);
	if (!g_qeglobals.d_project_entity)
		Error ("Couldn't parse %s", projectfile);
	free (data);

	// usefull for the log file and debuggin fucked up configurations from users:
	// output the basic information of the .qe3 project file
	Sys_Printf("basepath: %s\n", ValueForKey( g_qeglobals.d_project_entity, "basepath") );
	Sys_Printf("mapspath: %s\n", ValueForKey( g_qeglobals.d_project_entity, "mapspath") );
	Sys_Printf("rshcmd: %s\n", ValueForKey( g_qeglobals.d_project_entity, "rshcmd" ) );
	Sys_Printf("remotebasepath: %s\n", ValueForKey( g_qeglobals.d_project_entity, "remotebasepath" ) );
	Sys_Printf("entitypath: %s\n", ValueForKey( g_qeglobals.d_project_entity, "entitypath" ) );
	Sys_Printf("texturepath: %s\n", ValueForKey( g_qeglobals.d_project_entity, "texturepath" ) );

	Eclass_InitForSourceDirectory (ValueForKey (g_qeglobals.d_project_entity, "entitypath"));

	FillClassList ();		// list in entity window
	FillTextureMenu ();
	FillBSPMenu ();

	Map_New ();

	return true;
}

/*
===========
QE_KeyDown
===========
*/
#define	SPEED_MOVE	32
#define	SPEED_TURN	22.5

qboolean QE_KeyDown (int key)
{
	switch (key)
	{
	case 'K':
		PostMessage( g_qeglobals.d_hwndMain, WM_COMMAND, ID_MISC_SELECTENTITYCOLOR, 0 );
		break;

	case VK_TAB:
		PostMessage( g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_NEXTVIEW, 0 );
		break;

	case VK_UP:
		VectorMA (g_qeglobals.d_camera.origin, SPEED_MOVE, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
	case VK_DOWN:
		VectorMA (g_qeglobals.d_camera.origin, -SPEED_MOVE, g_qeglobals.d_camera.forward, g_qeglobals.d_camera.origin);
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
	case VK_LEFT:
		g_qeglobals.d_camera.angles[1] += SPEED_TURN;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
	case VK_RIGHT:
		g_qeglobals.d_camera.angles[1] -= SPEED_TURN;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
	case 'D':
		g_qeglobals.d_camera.origin[2] += SPEED_MOVE;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z_OVERLAY);
		break;
	case 'C':
		g_qeglobals.d_camera.origin[2] -= SPEED_MOVE;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY|W_Z_OVERLAY);
		break;
	case 'A':
		g_qeglobals.d_camera.angles[0] += SPEED_TURN;
		if (g_qeglobals.d_camera.angles[0] > 85)
			g_qeglobals.d_camera.angles[0] = 85;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
	case 'Z':
		g_qeglobals.d_camera.angles[0] -= SPEED_TURN;
		if (g_qeglobals.d_camera.angles[0] < -85)
			g_qeglobals.d_camera.angles[0] = -85;
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
	case VK_COMMA:
		VectorMA (g_qeglobals.d_camera.origin, -SPEED_MOVE, g_qeglobals.d_camera.right, g_qeglobals.d_camera.origin);
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
	case VK_PERIOD:
		VectorMA (g_qeglobals.d_camera.origin, SPEED_MOVE, g_qeglobals.d_camera.right, g_qeglobals.d_camera.origin);
		Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
		break;
		
	case '0':
		g_qeglobals.d_showgrid = !g_qeglobals.d_showgrid;
		PostMessage( g_qeglobals.d_hwndXY, WM_PAINT, 0, 0 );
		break;
	case '1':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_1, 0);
		break;
	case '2':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_2, 0);
		break;
	case '3':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_4, 0);
		break;
	case '4':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_8, 0);
		break;
	case '5':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_16, 0);
		break;
	case '6':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_32, 0);
		break;
	case '7':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_64, 0);
		break;
	case '8':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_GRID_128, 0);
		break;
		
	case 'E':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DRAGEDGES, 0);
		break;
	case 'V':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DRAGVERTECIES, 0);
		break;

	case 'X':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CLIPPER, 0);
		break;
	case 'Q':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECT_GROUPNEXTBRUSH, 0);
		break;

	case 'N':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_ENTITY, 0);
		break;
	case 'O':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CONSOLE, 0);
		break;
	case 'T':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_TEXTURE, 0);
		break;
	case 'S':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_TEXTURES_INSPECTOR, 0);
		break;
		
	case ' ':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_CLONE, 0);
		break;

	case 'I':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_INVERT, 0);
		break;
		
	case VK_BACK:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DELETE, 0);
		break;
	case VK_ESCAPE:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_SELECTION_DESELECT, 0);
		break;

	case VK_RETURN:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_CLIP_SELECTED, 0);
		break;

	case 'H':
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_HIDESHOW_HIDESELECTED, 0);
		break;

	case VK_END:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_CENTER, 0);
		break;
		
	case VK_DELETE:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_ZOOMIN, 0);
		break;
	case VK_INSERT:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_ZOOMOUT, 0);
		break;
		
	case VK_NEXT:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_DOWNFLOOR, 0);
		break;
	case VK_PRIOR:
		PostMessage (g_qeglobals.d_hwndMain, WM_COMMAND, ID_VIEW_UPFLOOR, 0);
		break;

	default:
		return false;
		
	}

	return true;
}

/*
===============
ConnectEntities

Sets target / targetname on the two entities selected
from the first selected to the secon
===============
*/
void ConnectEntities (void)
{
	entity_t	*e1, *e2, *e;
	char		*target, *tn;
	int			maxtarg, targetnum;
	char		newtarg[32];

	if (g_qeglobals.d_select_count != 2)
	{
		Sys_Printf ("Must have two brushes selected.\n");
		Sys_Beep ();
		return;
	}

	e1 = g_qeglobals.d_select_order[0]->owner;
	e2 = g_qeglobals.d_select_order[1]->owner;

	if (e1 == world_entity || e2 == world_entity)
	{
		Sys_Printf ("Can't connect to the world.\n");
		Sys_Beep ();
		return;
	}

	if (e1 == e2)
	{
		Sys_Printf ("Brushes are from same entity.\n");
		Sys_Beep ();
		return;
	}

	target = ValueForKey (e1, "target");
	if (target && target[0])
		strcpy (newtarg, target);
	else
	{
		target = ValueForKey (e2, "targetname");
		if (target && target[0])
			strcpy (newtarg, target);
		else
		{
			// make a unique target value
			maxtarg = 0;
			for (e=entities.next ; e != &entities ; e=e->next)
			{
				tn = ValueForKey (e, "targetname");
				if (tn && tn[0])
				{
					targetnum = atoi(tn+1);
					if (targetnum > maxtarg)
						maxtarg = targetnum;
				}
			}
			sprintf (newtarg, "t%i", maxtarg+1);
		}
	}

	SetKeyValue (e1, "target", newtarg);
	SetKeyValue (e2, "targetname", newtarg);
	Sys_UpdateWindows (W_XY | W_CAMERA);

	Select_Deselect();
	Select_Brush (g_qeglobals.d_select_order[1], true);
}

qboolean QE_SingleBrush (void)
{
	if ( (selected_brushes.next == &selected_brushes)
		|| (selected_brushes.next->next != &selected_brushes) )
	{
		Sys_Printf ("Must have a single brush selected\n");
		return false;
	}
	if (selected_brushes.next->owner->eclass->fixedsize)
	{
		Sys_Printf ("Cannot manipulate fixed size entities\n");
		return false;
	}

	return true;
}

void QE_Init (void)
{
	/*
	** initialize variables
	*/
	g_qeglobals.d_gridsize = 8;
	g_qeglobals.d_showgrid = true;
	g_qeglobals.d_viewtype = XY;
	g_qeglobals.d_camera.viewdistance = 256;

	Sys_UpdateGridStatusBar ();

	/*
	** other stuff
	*/
	Texture_Init ();
	Cam_Init ();
	XY_Init ();
	Z_Init ();
}

void QE_ConvertDOSToUnixName( char *dst, const char *src )
{
	while ( *src )
	{
		if ( *src == '\\' )
			*dst = '/';
		else
			*dst = *src;
		dst++; src++;
	}
	*dst = 0;
}

int g_numbrushes, g_numentities, g_numtextures;

void QE_CountBrushesAndUpdateStatusBar( void )
{
	static int      s_lastbrushcount, s_lastentitycount, s_lasttexturecount;
	static qboolean s_didonce;
	
//	entity_t   *e;
	brush_t	   *b, *next;
	qtexture_t	*q;

	g_numbrushes = 0;
	g_numentities = 0;
	g_numtextures = 0;
	
	if ( active_brushes.next != NULL )
	{
		for ( b = active_brushes.next ; b != NULL && b != &active_brushes ; b=next)
		{
			next = b->next;
			if (b->brush_faces )
			{
				if ( !b->owner->eclass->fixedsize)
					g_numbrushes++;
				else
					g_numentities++;
			}
		}
	}
/*
	if ( entities.next != NULL )
	{
		for ( e = entities.next ; e != &entities && g_numentities != MAX_MAP_ENTITIES ; e = e->next)
		{
			g_numentities++;
		}
	}
*/
	for (q=g_qeglobals.d_qtextures ; q ; q=q->next)
    {
		if (q->name[0] == '(')
			continue; // don't count entity textures

		if (q->inuse)
			g_numtextures++;
	}

	if ( ( ( g_numbrushes != s_lastbrushcount ) || ( g_numentities != s_lastentitycount  ) || ( g_numtextures != s_lasttexturecount ) ) || ( !s_didonce ) )
	{
		Sys_UpdateBrushStatusBar();

		s_lastbrushcount = g_numbrushes;
		s_lastentitycount = g_numentities;
		s_lasttexturecount = g_numtextures;
		s_didonce = true;
	}
}

