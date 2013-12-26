
#include "qe3.h"

BOOL CALLBACK GammaDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char sz[256];

	switch (uMsg)
    {
	case WM_INITDIALOG:
		sprintf(sz, "%1.1f", g_qeglobals.d_savedinfo.fGamma);		
		SetWindowText(GetDlgItem(hwndDlg, IDC_G_EDIT), sz);
		return TRUE;
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		
		case IDOK:
			GetWindowText(GetDlgItem(hwndDlg, IDC_G_EDIT), sz, 255);
			g_qeglobals.d_savedinfo.fGamma = atof(sz);
			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}



void DoGamma(void)
{
	if ( DialogBox(g_qeglobals.d_hInstance, (char *)IDD_GAMMA, g_qeglobals.d_hwndMain, GammaDlgProc))
	{
	}
}		

//================================================


void SelectBrush (int entitynum, int brushnum)
{
	entity_t	*e;
	brush_t		*b;
	int			i;

	if (entitynum == 0)
		e = world_entity;
	else
	{
		e = entities.next;
		while (--entitynum)
		{
			e=e->next;
			if (e == &entities)
			{
				Sys_Printf ("No such entity.\n");
				return;
			}
		}
	}

	b = e->brushes.onext;
	if (b == &e->brushes)
	{
		Sys_Printf ("No such brush.\n");
		return;
	}
	while (brushnum--)
	{
		b=b->onext;
		if (b == &e->brushes)
		{
			Sys_Printf ("No such brush.\n");
			return;
		}
	}

	Brush_RemoveFromList (b);
	Brush_AddToList (b, &selected_brushes);


	Sys_UpdateWindows (W_ALL);
	for (i=0 ; i<3 ; i++)
		g_qeglobals.d_xyz.origin[i] = (b->mins[i] + b->maxs[i])/2;

	Sys_Printf ("Selected.\n");
}

/*
=================
GetSelectionIndex
=================
*/
void GetSelectionIndex (int *ent, int *brush)
{
	brush_t		*b, *b2;
	entity_t	*entity;

	*ent = *brush = 0;

	b = selected_brushes.next;
	if (b == &selected_brushes)
		return;

	// find entity
	if (b->owner != world_entity)
	{
		(*ent)++;
		for (entity = entities.next ; entity != &entities 
			; entity=entity->next, (*ent)++)
		;
	}

	// find brush
	for (b2=b->owner->brushes.onext 
		; b2 != b && b2 != &b->owner->brushes
		; b2=b2->onext, (*brush)++)
	;
}

BOOL CALLBACK FindBrushDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char entstr[256];
	char brushstr[256];
	HWND	h;
	int		ent, brush;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		// set entity and brush number
		GetSelectionIndex (&ent, &brush);
		sprintf (entstr, "%i", ent);
		sprintf (brushstr, "%i", brush);
		SetWindowText(GetDlgItem(hwndDlg, IDC_FIND_ENTITY), entstr);
		SetWindowText(GetDlgItem(hwndDlg, IDC_FIND_BRUSH), brushstr);

		h = GetDlgItem(hwndDlg, IDC_FIND_ENTITY);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
			case IDOK:
				GetWindowText(GetDlgItem(hwndDlg, IDC_FIND_ENTITY), entstr, 255);
				GetWindowText(GetDlgItem(hwndDlg, IDC_FIND_BRUSH), brushstr, 255);
				SelectBrush (atoi(entstr), atoi(brushstr));
				EndDialog(hwndDlg, 1);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}	
	}
	return FALSE;
}



void DoFindBrush(void)
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_FINDBRUSH, g_qeglobals.d_hwndMain, FindBrushDlgProc);
}	
	
/*
===================================================

  ARBITRARY ROTATE

===================================================
*/


BOOL CALLBACK RotateDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char	str[256];
	HWND	h;
	float	v;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_ROTX);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		
		case IDOK:
			GetWindowText(GetDlgItem(hwndDlg, IDC_ROTX), str, 255);
			v = atof(str);
			if (v)
				Select_RotateAxis (0, v);

			GetWindowText(GetDlgItem(hwndDlg, IDC_ROTY), str, 255);
			v = atof(str);
			if (v)
				Select_RotateAxis (1, v);

			GetWindowText(GetDlgItem(hwndDlg, IDC_ROTZ), str, 255);
			v = atof(str);
			if (v)
				Select_RotateAxis (2, v);

			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}	
	}

	return FALSE;
}



void DoRotate(void)
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_ROTATE, g_qeglobals.d_hwndMain, RotateDlgProc);
}
		
/*
===================================================

  ARBITRARY SIDES

===================================================
*/


BOOL CALLBACK SidesDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char str[256];
	HWND	h;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_SIDES);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) { 
		
		case IDOK:
			GetWindowText(GetDlgItem(hwndDlg, IDC_SIDES), str, 255);
			Brush_MakeSided (atoi(str));

			EndDialog(hwndDlg, 1);
		break;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
		break;
	}	
	default:
		return FALSE;
	}
}



void DoSides(void)
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_SIDES, g_qeglobals.d_hwndMain, SidesDlgProc);
}		

//======================================================================

/*
===================
DoAbout
===================
*/
BOOL CALLBACK AboutDlgProc( HWND hwndDlg,
						    UINT uMsg,
						    WPARAM wParam,
						    LPARAM lParam )
{
	switch (uMsg)
    {
	case WM_INITDIALOG:
		{
			char renderer[1024];
			char version[1024];
			char vendor[1024];
			char extensions[4096];
			char extensionscopy[4096];
			char *s;

			sprintf( renderer, "Renderer:\t%s", glGetString( GL_RENDERER ) );
			sprintf( version, "Version:\t\t%s", glGetString( GL_VERSION ) );
			sprintf( vendor, "Vendor:\t\t%s", glGetString( GL_VENDOR ) );
			strcpy ( extensions, glGetString( GL_EXTENSIONS ));
			extensionscopy[0] = 0;

			for(s = strtok(extensions, " \n\t"); s; s = strtok(NULL, " \n"))
			{
				strcat(extensionscopy, s);
				strcat(extensionscopy, "\r\n");
			}

			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLRENDERER ),   renderer );
			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLVERSION ),    version );
			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLVENDOR ),     vendor );
			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLEXTENSIONS ), extensionscopy );
		}
		return TRUE;

	case WM_CLOSE:
		EndDialog( hwndDlg, 1 );
		return TRUE;

	case WM_COMMAND:
		if ( LOWORD( wParam ) == IDOK )
			EndDialog(hwndDlg, 1);
		return TRUE;
	}
	return FALSE;
}

void DoAbout(void)
{
	DialogBox( g_qeglobals.d_hInstance, ( char * ) IDD_ABOUT, g_qeglobals.d_hwndMain, AboutDlgProc );
}

// FIT:
int		g_fitwidth = 1, g_fitheight = 1;

/*
===================================================

  SURFACE INSPECTOR

===================================================
*/

texdef_t	g_old_texdef;
HWND		g_surfwin;
qboolean	g_changed_surface;

int	g_checkboxes[64] = { 
	IDC_CHECK1, IDC_CHECK2, IDC_CHECK3, IDC_CHECK4, 
	IDC_CHECK5, IDC_CHECK6, IDC_CHECK7, IDC_CHECK8, 
	IDC_CHECK9, IDC_CHECK10, IDC_CHECK11, IDC_CHECK12, 
	IDC_CHECK13, IDC_CHECK14, IDC_CHECK15, IDC_CHECK16,
	IDC_CHECK17, IDC_CHECK18, IDC_CHECK19, IDC_CHECK20,
	IDC_CHECK21, IDC_CHECK22, IDC_CHECK23, IDC_CHECK24,
	IDC_CHECK25, IDC_CHECK26, IDC_CHECK27, IDC_CHECK28,
	IDC_CHECK29, IDC_CHECK30, IDC_CHECK31, IDC_CHECK32,

	IDC_CHECK33, IDC_CHECK34, IDC_CHECK35, IDC_CHECK36,
	IDC_CHECK37, IDC_CHECK38, IDC_CHECK39, IDC_CHECK40,
	IDC_CHECK41, IDC_CHECK42, IDC_CHECK43, IDC_CHECK44,
	IDC_CHECK45, IDC_CHECK46, IDC_CHECK47, IDC_CHECK48,
	IDC_CHECK49, IDC_CHECK50, IDC_CHECK51, IDC_CHECK52,
	IDC_CHECK53, IDC_CHECK54, IDC_CHECK55, IDC_CHECK56,
	IDC_CHECK57, IDC_CHECK58, IDC_CHECK59, IDC_CHECK60,
	IDC_CHECK61, IDC_CHECK62, IDC_CHECK63, IDC_CHECK64
 };

/*
==============
SetTexMods

Set the fields to the current texdef
===============
*/
void SetTexMods(void)
{
	char	sz[128];
	texdef_t *pt;
//	int		i;

	pt = &g_qeglobals.d_texturewin.texdef;

	SendMessage (g_surfwin, WM_SETREDRAW, 0, 0);

	SetWindowText(GetDlgItem(g_surfwin, IDC_TEXTURE), pt->name);

	sprintf(sz, "%d", (int)pt->shift[0]);
	SetWindowText(GetDlgItem(g_surfwin, IDC_HSHIFT), sz);

	sprintf(sz, "%d", (int)pt->shift[1]);
	SetWindowText(GetDlgItem(g_surfwin, IDC_VSHIFT), sz);

	sprintf(sz, "%4.2f", pt->scale[0]);
	SetWindowText(GetDlgItem(g_surfwin, IDC_HSCALE), sz);

	sprintf(sz, "%4.2f", pt->scale[1]);
	SetWindowText(GetDlgItem(g_surfwin, IDC_VSCALE), sz);

	sprintf(sz, "%d", (int)pt->rotate);
	SetWindowText(GetDlgItem(g_surfwin, IDC_ROTATE), sz);
// FIT:
	sprintf(sz, "%d", (int)g_fitheight);
	SetWindowText(GetDlgItem(g_surfwin, IDC_HFIT), sz);

	sprintf(sz, "%d", (int)g_fitwidth);
	SetWindowText(GetDlgItem(g_surfwin, IDC_WFIT), sz);
//
//	sprintf(sz, "%d", (int)pt->value);
//	SetWindowText(GetDlgItem(g_surfwin, IDC_VALUE), sz);
/*
	for (i=0 ; i<32 ; i++)
		SendMessage(GetDlgItem(g_surfwin, g_checkboxes[i]), BM_SETCHECK, !!(pt->flags&(1<<i)), 0 );
	for (i=0 ; i<32 ; i++)
		SendMessage(GetDlgItem(g_surfwin, g_checkboxes[32+i]), BM_SETCHECK, !!(pt->contents&(1<<i)), 0 );
*/
	SendMessage (g_surfwin, WM_SETREDRAW, 1, 0);
	InvalidateRect (g_surfwin, NULL, true);
}


/*
==============
GetTexMods

Reads the fields to get the current texdef
===============
*/
void GetTexMods(void)
{
	char	sz[128];
	texdef_t *pt;
//	int		b;
//	int		i;

	pt = &g_qeglobals.d_texturewin.texdef;

	GetWindowText (GetDlgItem(g_surfwin, IDC_TEXTURE), sz, 127);
	strncpy (pt->name, sz, sizeof(pt->name)-1);
	if (pt->name[0] <= ' ')
	{
		strcpy (pt->name, "none");
		SetWindowText(GetDlgItem(g_surfwin, IDC_TEXTURE), pt->name);
	}

	GetWindowText (GetDlgItem(g_surfwin, IDC_HSHIFT), sz, 127);
	pt->shift[0] = atof(sz);

	GetWindowText (GetDlgItem(g_surfwin, IDC_VSHIFT), sz, 127);
	pt->shift[1] = atof(sz);

	GetWindowText(GetDlgItem(g_surfwin, IDC_HSCALE), sz, 127);
	pt->scale[0] = atof(sz);

	GetWindowText(GetDlgItem(g_surfwin, IDC_VSCALE), sz, 127);
	pt->scale[1] = atof(sz);

	GetWindowText(GetDlgItem(g_surfwin, IDC_ROTATE), sz, 127);
	pt->rotate = atof(sz);

// FIT:
	GetWindowText(GetDlgItem(g_surfwin, IDC_HFIT), sz, 127);
	g_fitheight = atof(sz);

	GetWindowText(GetDlgItem(g_surfwin, IDC_WFIT), sz, 127);
	g_fitwidth = atof(sz);
//

//	GetWindowText(GetDlgItem(g_surfwin, IDC_VALUE), sz, 127);
//	pt->value = atof(sz);
/*
	pt->flags = 0;
	for (i=0 ; i<32 ; i++)
	{
		b = SendMessage(GetDlgItem(g_surfwin, g_checkboxes[i]), BM_GETCHECK, 0, 0);
		if (b != 1 && b != 0)
			continue;
		pt->flags |= b<<i;
	}

	pt->contents = 0;
	for (i=0 ; i<32 ; i++)
	{
		b = SendMessage(GetDlgItem(g_surfwin, g_checkboxes[32+i]), BM_GETCHECK, 0, 0);
		if (b != 1 && b != 0)
			continue;
		pt->contents |= b<<i;
	}
*/
	g_changed_surface = true;
	Select_SetTexture(pt);
}

/*
=================
UpdateSpinners
=================
*/
void UpdateSpinners(unsigned uMsg, WPARAM wParam, LPARAM lParam)
{
	int nScrollCode;

	HWND hwnd;
	texdef_t *pt;

	pt = &g_qeglobals.d_texturewin.texdef;

	nScrollCode = (int) LOWORD(wParam);  // scroll bar value 
	hwnd = (HWND) lParam;       // handle of scroll bar 

	if ((nScrollCode != SB_LINEUP) && (nScrollCode != SB_LINEDOWN))
		return;
	
	if (hwnd == GetDlgItem(g_surfwin, IDC_ROTATEA))
	{
		if (nScrollCode == SB_LINEUP)
			pt->rotate += 45;
		else
			pt->rotate -= 45;

		if (pt->rotate < 0)
			pt->rotate += 360;

		if (pt->rotate >= 360)
			pt->rotate -= 360;
	}

	else if (hwnd == GetDlgItem(g_surfwin, IDC_HSCALEA))
	{
		if (nScrollCode == SB_LINEDOWN)
			pt->scale[0] -= (float)0.1;
		else
			pt->scale[0] += (float)0.1;
	}
	
	else if (hwnd == GetDlgItem(g_surfwin, IDC_VSCALEA))
	{
		if (nScrollCode == SB_LINEUP)
			pt->scale[1] += (float)0.1;
		else
			pt->scale[1] -= (float)0.1;
	} 
	
	else if (hwnd == GetDlgItem(g_surfwin, IDC_HSHIFTA))
	{
		if (nScrollCode == SB_LINEDOWN)
			pt->shift[0] -= 8;
		else
			pt->shift[0] += 8;
	}
	
	else if (hwnd == GetDlgItem(g_surfwin, IDC_VSHIFTA))
	{
		if (nScrollCode == SB_LINEUP)
			pt->shift[1] += 8;
		else
			pt->shift[1] -= 8;
	}
// FIT:
	else if (hwnd == GetDlgItem(g_surfwin, IDC_HFITA))
	{
		if (nScrollCode == SB_LINEDOWN)
			g_fitheight -= 1;
		else
			g_fitheight += 1;
	}

	else if (hwnd == GetDlgItem(g_surfwin, IDC_WFITA))
	{
		if (nScrollCode == SB_LINEUP)
			g_fitwidth += 1;
		else
			g_fitwidth -= 1;
	}
//

	SetTexMods();
	g_changed_surface = true;
	Select_SetTexture(pt);
}



BOOL CALLBACK SurfaceDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	switch (uMsg)
    {
	case WM_INITDIALOG:
		g_surfwin = hwndDlg;
		SetTexMods ();
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) { 
		
		case IDOK:
			GetTexMods ();
			EndDialog(hwndDlg, 1);
		break;

		case IDAPPLY:
			GetTexMods ();
			InvalidateRect(g_qeglobals.d_hwndCamera, NULL, false);
			UpdateWindow (g_qeglobals.d_hwndCamera);
		break;
// FIT:
		case IDC_FIT:
		{
			char	sz[128];

			GetWindowText(GetDlgItem(g_surfwin, IDC_HFIT), sz, 127);
			g_fitheight = atof(sz);
			GetWindowText(GetDlgItem(g_surfwin, IDC_WFIT), sz, 127);
			g_fitwidth = atof(sz);

			Select_FitTexture(g_fitheight, g_fitwidth);

			sprintf(sz, "%d", (int)g_fitheight);
			SetWindowText(GetDlgItem(g_surfwin, IDC_HFIT), sz);
			sprintf(sz, "%d", (int)g_fitwidth);
			SetWindowText(GetDlgItem(g_surfwin, IDC_WFIT), sz);

			InvalidateRect(g_qeglobals.d_hwndCamera, NULL, false);
			UpdateWindow (g_qeglobals.d_hwndCamera);
		}
		break;
//
		case IDCANCEL:
			g_qeglobals.d_texturewin.texdef = g_old_texdef;
			if (g_changed_surface)
				Select_SetTexture(&g_qeglobals.d_texturewin.texdef);
			EndDialog(hwndDlg, 0);
		break;
		}	
		break;

	case WM_HSCROLL:
	case WM_VSCROLL:
		UpdateSpinners(uMsg, wParam, lParam);
		InvalidateRect(g_qeglobals.d_hwndCamera, NULL, false);
		UpdateWindow (g_qeglobals.d_hwndCamera);
		return 0;

	default:
		return FALSE;
	}
	return FALSE; //EER1
}


void DoSurface (void)
{
	// save current state for cancel
	g_old_texdef = g_qeglobals.d_texturewin.texdef;
	g_changed_surface = false;

	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_SURFACE, g_qeglobals.d_hwndMain, SurfaceDlgProc);
}		

HWND		g_ftexwin;

BOOL CALLBACK FindTextureDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char findstr[256];
	char replacestr[256];

	qboolean selected, force;

	texdef_t *pt;
	pt = &g_qeglobals.d_texturewin.texdef;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		// 
		g_ftexwin = hwndDlg;
		SendMessage (g_ftexwin, WM_SETREDRAW, 0, 0);

		strcpy (findstr, pt->name);
		strcpy (replacestr, pt->name);

		SetWindowText(GetDlgItem(g_ftexwin, IDC_EDIT_FIND), findstr);
		SetWindowText(GetDlgItem(g_ftexwin, IDC_EDIT_REPLACE), replacestr);

		selected = SendMessage(GetDlgItem(g_ftexwin, IDC_CHECK_SELECTED), BM_GETCHECK, 0, 0);
		force = SendMessage(GetDlgItem(g_ftexwin, IDC_CHECK_FORCE), BM_GETCHECK, 0, 0);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
			case IDOK:
				GetWindowText (GetDlgItem(g_ftexwin, IDC_EDIT_FIND), findstr, 255);
				strncpy (pt->name, findstr, sizeof(pt->name)-1);
				if (pt->name[0] <= ' ')
				{
					strcpy (pt->name, "none");
				}

				GetWindowText (GetDlgItem(g_ftexwin, IDC_EDIT_REPLACE), replacestr, 255);
				strncpy (pt->name, replacestr, sizeof(pt->name)-1);
				if (pt->name[0] <= ' ')
				{
					strcpy (pt->name, "none");
				}

				selected = SendMessage(GetDlgItem(g_ftexwin, IDC_CHECK_SELECTED), BM_GETCHECK, 0, 0);
				force = SendMessage(GetDlgItem(g_ftexwin, IDC_CHECK_FORCE), BM_GETCHECK, 0, 0);

				FindReplaceTextures (findstr, replacestr, selected, force);

				EndDialog(hwndDlg, 1);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}	
	}
	return FALSE;
}

void DoFindTexture (void)
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_FINDREPLACE, g_qeglobals.d_hwndMain, FindTextureDlgProc);
}		

