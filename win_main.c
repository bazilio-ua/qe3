#include "qe3.h"
#include <process.h>
#include "mru.h"

static HWND      s_hwndToolbar;

BOOL SaveRegistryInfo(const char *pszName, void *pvBuf, long lSize);
BOOL LoadRegistryInfo(const char *pszName, void *pvBuf, long *plSize);

static HWND CreateStatusBar(HINSTANCE hInst);
static HWND CreateToolBar(HINSTANCE hInst);

extern int WXY_Print( void );

/*
==============================================================================

  MENU

==============================================================================
*/

void OpenDialog (void);
void SaveAsDialog (void);
qboolean ConfirmModified (void);
void  Select_Ungroup (void);

void QE_ExpandBspString (char *bspaction, char *out, char *mapname)
{
	char	*in;
	char	src[1024];
	char	rsh[1024];
	char	base[512];

	ExtractFileName (mapname, base);
	sprintf (src, "%s/maps/%s", ValueForKey(g_qeglobals.d_project_entity, "remotebasepath"), base);
	StripExtension (src);
	strcpy (rsh, ValueForKey(g_qeglobals.d_project_entity, "rshcmd"));

	in = ValueForKey( g_qeglobals.d_project_entity, bspaction );
	while (*in)
	{
		if (in[0] == '!')
		{
			strcpy (out, rsh);
			out += strlen(rsh);
			in++;
			continue;
		}
		if (in[0] == '$')
		{
			strcpy (out, src);
			out += strlen(src);
			in++;
			continue;
		}
		if (in[0] == '@')
		{
			*out++ = '"';
			in++;
			continue;
		}
		*out++ = *in++;
	}
	*out = 0;
}



void RunBsp (char *command)
{
	char	sys[1024];
	char	batpath[1024];
	char	outputpath[1024];
	char	temppath[1024];
	char	name[1024];
	char	*basepath;
	FILE	*hFile;
	BOOL	ret;
	PROCESS_INFORMATION ProcessInformation;
	STARTUPINFO	startupinfo;

	SetInspectorMode (W_CONSOLE);

	if (bsp_process)
	{
		Sys_Printf ("BSP is still going...\n");
		return;
	}

	GetTempPath(1024, temppath);
	sprintf (outputpath, "%sjunk.txt", temppath);

	strcpy (name, currentmap);
	if (region_active)
	{
		Map_SaveFile (name, false);
		StripExtension (name);
		strcat (name, ".reg");
	}

	Map_SaveFile (name, region_active);


	QE_ExpandBspString (command, sys, name);

//	Sys_ClearPrintf ();
	Sys_Printf ("======================================\nRunning bsp command...\n");
	Sys_Printf ("\n%s\n", sys);

	//
	// write qe3bsp.bat
	//
	sprintf (batpath, "%sqe3bsp.bat", temppath);
	hFile = fopen(batpath, "w");
	if (!hFile)
		Error ("Can't write to %s", batpath);
	fprintf (hFile, sys);
	fclose (hFile);

	//
	// write qe3bsp2.bat
	//
	sprintf (batpath, "%sqe3bsp2.bat", temppath);
	hFile = fopen(batpath, "w");
	if (!hFile)
		Error ("Can't write to %s", batpath);
	fprintf (hFile, "%sqe3bsp.bat > %s", temppath, outputpath);
	fclose (hFile);

	Pointfile_Delete ();

	basepath = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	if(basepath && !(*basepath))
		basepath = NULL;

	GetStartupInfo (&startupinfo);

	ret = CreateProcess(
    batpath,		// pointer to name of executable module 
    NULL,			// pointer to command line string
    NULL,			// pointer to process security attributes 
    NULL,			// pointer to thread security attributes 
    FALSE,			// handle inheritance flag 
    0 /*DETACHED_PROCESS*/,		// creation flags 
    NULL,			// pointer to new environment block 
//    NULL,			// pointer to current directory name 
	basepath,		// pointer to current directory name 
    &startupinfo,	// pointer to STARTUPINFO 
    &ProcessInformation 	// pointer to PROCESS_INFORMATION  
   );

	if (!ret)
		Error ("CreateProcess failed");

	bsp_process = ProcessInformation.hProcess;

	Sleep (100);	// give the new process a chance to open it's window

	BringWindowToTop( g_qeglobals.d_hwndMain );	// pop us back on top
	SetFocus (g_qeglobals.d_hwndCamera);
}

/*
=============
DoColor

=============
*/
qboolean DoColor(int iIndex)
{
	CHOOSECOLOR	cc;
	static COLORREF	custom[16];

	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = g_qeglobals.d_hwndMain;
//	cc.hInstance = g_qeglobals.d_hInstance;
	cc.hInstance = g_qeglobals.d_hInstanceColor; //EER1
	
	cc.rgbResult = 
		(int)(g_qeglobals.d_savedinfo.colors[iIndex][0]*255) +
		(((int)(g_qeglobals.d_savedinfo.colors[iIndex][1]*255))<<8) +
		(((int)(g_qeglobals.d_savedinfo.colors[iIndex][2]*255))<<16);
    cc.lpCustColors = custom;
    cc.Flags = CC_FULLOPEN|CC_RGBINIT; 
    //cc.lCustData; 
    //cc.lpfnHook; 
    //cc.lpTemplateName

	if (!ChooseColor(&cc))
		return false;

	g_qeglobals.d_savedinfo.colors[iIndex][0] = (cc.rgbResult&255)/255.0;
	g_qeglobals.d_savedinfo.colors[iIndex][1] = ((cc.rgbResult>>8)&255)/255.0;
	g_qeglobals.d_savedinfo.colors[iIndex][2] = ((cc.rgbResult>>16)&255)/255.0;

	/* 
	** scale colors so that at least one component is at 1.0F 
	** if this is meant to select an entity color
	*/
	if ( iIndex == COLOR_ENTITY )
	{
		float largest = 0.0F;

		if ( g_qeglobals.d_savedinfo.colors[iIndex][0] > largest )
			largest = g_qeglobals.d_savedinfo.colors[iIndex][0];
		if ( g_qeglobals.d_savedinfo.colors[iIndex][1] > largest )
			largest = g_qeglobals.d_savedinfo.colors[iIndex][1];
		if ( g_qeglobals.d_savedinfo.colors[iIndex][2] > largest )
			largest = g_qeglobals.d_savedinfo.colors[iIndex][2];

		if ( largest == 0.0F )
		{
			g_qeglobals.d_savedinfo.colors[iIndex][0] = 1.0F;
			g_qeglobals.d_savedinfo.colors[iIndex][1] = 1.0F;
			g_qeglobals.d_savedinfo.colors[iIndex][2] = 1.0F;
		}
		else
		{
			float scaler = 1.0F / largest;

			g_qeglobals.d_savedinfo.colors[iIndex][0] *= scaler;
			g_qeglobals.d_savedinfo.colors[iIndex][1] *= scaler;
			g_qeglobals.d_savedinfo.colors[iIndex][2] *= scaler;
		}
	}

	Sys_UpdateWindows (W_ALL);

	return true;
}


/* Copied from MSDN */

BOOL DoMru(HWND hWnd,WORD wId)
{
	char szFileName[128];
	OFSTRUCT of;
	BOOL fExist;

	GetMenuItem(g_qeglobals.d_lpMruMenu, wId, TRUE, szFileName, sizeof(szFileName));

	// Test if the file exists.

	fExist = OpenFile(szFileName ,&of,OF_EXIST) != HFILE_ERROR;

	if (fExist) {

		// Place the file on the top of MRU.
		AddNewItem(g_qeglobals.d_lpMruMenu,(LPSTR)szFileName);

		// Now perform opening this file !!!
		Map_LoadFile (szFileName);	
	}
	else
		// Remove the file on MRU.
		DelMenuItem(g_qeglobals.d_lpMruMenu,wId,TRUE);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(hWnd),0),
			ID_FILE_EXIT);

	return fExist;
}


/* handle all WM_COMMAND messages here */
LONG WINAPI CommandHandler (
    HWND    hWnd,
    WPARAM  wParam,
    LPARAM  lParam)
{
	HMENU hMenu;

    switch (LOWORD(wParam))
    {
//
// file menu
//
    	case ID_FILE_EXIT:
            /* exit application */
			if (!ConfirmModified())
				return TRUE;

            PostMessage (hWnd, WM_CLOSE, 0, 0L);
			break;

		case ID_FILE_OPEN:
			if (!ConfirmModified())
				return TRUE;
			OpenDialog ();
			break;

		case ID_FILE_NEW:
			if (!ConfirmModified())
				return TRUE;
			Map_New ();
			break;
		case ID_FILE_SAVE:
			if (!strcmp(currentmap, "unnamed.map"))
				SaveAsDialog ();
			else
				Map_SaveFile (currentmap, false);	// ignore region
			break;
		case ID_FILE_SAVEAS:
			SaveAsDialog ();
			break;

		case ID_FILE_LOADPROJECT:
			if (!ConfirmModified())
				return TRUE;
			ProjectDialog ();
			break;

		case ID_FILE_POINTFILE:
			if (g_qeglobals.d_pointfile_display_list)
				Pointfile_Clear ();
			else
				Pointfile_Check ();
			break;

//
// view menu
//
		case ID_VIEW_ENTITY:
			SetInspectorMode(W_ENTITY);
			break;
		case ID_VIEW_CONSOLE:
			SetInspectorMode(W_CONSOLE);
			break;
		case ID_VIEW_TEXTURE:
			SetWindowText(g_qeglobals.d_hwndEntity, "Textures");
			SetInspectorMode(W_TEXTURE);
			break;

		case ID_VIEW_NEXTVIEW:
			if (g_qeglobals.d_viewtype == XY)
			{
				g_qeglobals.d_viewtype = XZ;
				SetWindowText(g_qeglobals.d_hwndXY, "XZ Front");
			}
			else if (g_qeglobals.d_viewtype == XZ)
			{
				g_qeglobals.d_viewtype = YZ;
				SetWindowText(g_qeglobals.d_hwndXY, "YZ Side");
			}
			else
			{
				g_qeglobals.d_viewtype = XY;
				SetWindowText(g_qeglobals.d_hwndXY, "XY Top");
			}
			PositionView ();
			Sys_UpdateWindows (W_XY|W_XY_OVERLAY|W_CAMERA);
			break;

		case ID_VIEW_XY:
			g_qeglobals.d_viewtype = XY;
			SetWindowText(g_qeglobals.d_hwndXY, "XY Top");
			PositionView ();
			Sys_UpdateWindows (W_XY|W_XY_OVERLAY|W_CAMERA);
			break;
		case ID_VIEW_FRONT:
			g_qeglobals.d_viewtype = XZ;
			SetWindowText(g_qeglobals.d_hwndXY, "XZ Front");
			PositionView ();
			Sys_UpdateWindows (W_XY|W_XY_OVERLAY|W_CAMERA);
			break;
		case ID_VIEW_SIDE:
			g_qeglobals.d_viewtype = YZ;
			SetWindowText(g_qeglobals.d_hwndXY, "YZ Side");
			PositionView ();
			Sys_UpdateWindows (W_XY|W_XY_OVERLAY|W_CAMERA);
			break;

		case ID_VIEW_100:
			g_qeglobals.d_xyz.scale = 1;
			Sys_UpdateWindows (W_XY|W_XY_OVERLAY);
			break;
		case ID_VIEW_ZOOMIN:
			g_qeglobals.d_xyz.scale *= (float)5.0/4;
			if (g_qeglobals.d_xyz.scale > (float)16)
				g_qeglobals.d_xyz.scale = (float)16;
			Sys_UpdateWindows (W_XY|W_XY_OVERLAY);
			break;
		case ID_VIEW_ZOOMOUT:
			g_qeglobals.d_xyz.scale *= (float)4.0/5;
			if (g_qeglobals.d_xyz.scale < (float)0.1)
				g_qeglobals.d_xyz.scale = (float)0.1;
			Sys_UpdateWindows (W_XY|W_XY_OVERLAY);
			break;

		case ID_VIEW_Z100:
			g_qeglobals.d_z.scale = 1;
			Sys_UpdateWindows (W_Z|W_Z_OVERLAY);
			break;
		case ID_VIEW_ZZOOMIN:
			g_qeglobals.d_z.scale *= (float)5.0/4;
			if (g_qeglobals.d_z.scale > (float)4)
				g_qeglobals.d_z.scale = (float)4;
			Sys_UpdateWindows (W_Z|W_Z_OVERLAY);
			break;
		case ID_VIEW_ZZOOMOUT:
			g_qeglobals.d_z.scale *= (float)4.0/5;
			if (g_qeglobals.d_z.scale < (float)0.125)
				g_qeglobals.d_z.scale = (float)0.125;
			Sys_UpdateWindows (W_Z|W_Z_OVERLAY);
			break;

		case ID_VIEW_CENTER:
			g_qeglobals.d_camera.angles[ROLL] = g_qeglobals.d_camera.angles[PITCH] = 0;
			g_qeglobals.d_camera.angles[YAW] = (float)22.5 * 
				floor( (g_qeglobals.d_camera.angles[YAW]+11)/(float)22.5 );
			Sys_UpdateWindows (W_CAMERA|W_XY_OVERLAY);
			break;

		case ID_VIEW_UPFLOOR:
			Cam_ChangeFloor (true);
			break;
		case ID_VIEW_DOWNFLOOR:
			Cam_ChangeFloor (false);
			break;

		case IDC_CHECK_SIZEPAINT:
			g_qeglobals.d_savedinfo.check_sizepaint = !g_qeglobals.d_savedinfo.check_sizepaint;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), IDC_CHECK_SIZEPAINT, MF_BYCOMMAND | (g_qeglobals.d_savedinfo.check_sizepaint ? MF_CHECKED : MF_UNCHECKED)  );
			Map_BuildBrushData();
			Sys_UpdateWindows (W_XY);
			break;

		case ID_VIEW_RADIANTLIGHTS:
			g_qeglobals.d_savedinfo.view_radiantlights = !g_qeglobals.d_savedinfo.view_radiantlights;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_RADIANTLIGHTS, MF_BYCOMMAND | (g_qeglobals.d_savedinfo.view_radiantlights ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWNAMES:
			g_qeglobals.d_savedinfo.show_names = !g_qeglobals.d_savedinfo.show_names;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWNAMES, MF_BYCOMMAND | (g_qeglobals.d_savedinfo.show_names ? MF_CHECKED : MF_UNCHECKED)  );
			Map_BuildBrushData();
			Sys_UpdateWindows (W_XY);
			break;

		case ID_VIEW_SHOWCOORDINATES:
			g_qeglobals.d_savedinfo.show_coordinates ^= 1;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWCOORDINATES, MF_BYCOMMAND | (g_qeglobals.d_savedinfo.show_coordinates ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_UpdateWindows (W_XY|W_Z);
			break;

		case ID_VIEW_SHOWBLOCKS:
			g_qeglobals.d_show_blocks ^= 1;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWBLOCKS, MF_BYCOMMAND | (g_qeglobals.d_show_blocks ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_UpdateWindows (W_XY);
			break;

		case ID_VIEW_SHOWWORKZONE:
			g_qeglobals.d_show_work ^= 1;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWWORKZONE, MF_BYCOMMAND | (g_qeglobals.d_show_work ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_UpdateWindows (W_XY);
			break;

		case ID_VIEW_SHOWANGLES:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_ANGLES ) & EXCLUDE_ANGLES )
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWANGLES, MF_BYCOMMAND | MF_UNCHECKED );
			else
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWANGLES, MF_BYCOMMAND | MF_CHECKED );				
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWLIGHTS:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_LIGHTS ) & EXCLUDE_LIGHTS )
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );
			else
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_CHECKED );				
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWPATH:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_PATHS ) & EXCLUDE_PATHS )
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_UNCHECKED );
			else
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_CHECKED );
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWENT:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_ENT ) & EXCLUDE_ENT )
				CheckMenuItem( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWENT, MF_BYCOMMAND | MF_UNCHECKED);
			else
				CheckMenuItem( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWENT, MF_BYCOMMAND | MF_CHECKED);
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWWATER:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_WATER ) & EXCLUDE_WATER )
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_UNCHECKED );
			else
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_CHECKED );
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWCLIP:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_CLIP ) & EXCLUDE_CLIP )
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_UNCHECKED );
			else
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_CHECKED );
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWFUNCWALL:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_FUNC_WALL ) & EXCLUDE_FUNC_WALL )
			{
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWFUNCWALL, MF_BYCOMMAND | MF_UNCHECKED );
			}
			else
			{
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWFUNCWALL, MF_BYCOMMAND | MF_CHECKED );
			}
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWSKY:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_SKY ) & EXCLUDE_SKY )
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWSKY, MF_BYCOMMAND | MF_UNCHECKED );
			else
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWSKY, MF_BYCOMMAND | MF_CHECKED );
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;

		case ID_VIEW_SHOWWORLD:
			if ( ( g_qeglobals.d_savedinfo.exclude ^= EXCLUDE_WORLD ) & EXCLUDE_WORLD )
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_UNCHECKED );
			else
				CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_CHECKED );
			Sys_UpdateWindows (W_XY|W_CAMERA);
			break;


//
// grid menu
//
		case ID_GRID_1:
		case ID_GRID_2:
		case ID_GRID_4:
		case ID_GRID_8:
		case ID_GRID_16:
		case ID_GRID_32:
		case ID_GRID_64:
		case ID_GRID_128:
		case ID_GRID_256:
		{
			hMenu = GetMenu(hWnd);
			
			CheckMenuItem(hMenu, ID_GRID_1, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_2, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_4, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_8, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_16, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_32, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_64, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_128, MF_BYCOMMAND | MF_UNCHECKED);
			CheckMenuItem(hMenu, ID_GRID_256, MF_BYCOMMAND | MF_UNCHECKED);

			switch (LOWORD(wParam))
			{
				case ID_GRID_1: g_qeglobals.d_gridsize = 0; break;
				case ID_GRID_2: g_qeglobals.d_gridsize = 1; break;
				case ID_GRID_4: g_qeglobals.d_gridsize = 2; break;
				case ID_GRID_8: g_qeglobals.d_gridsize = 3; break;
				case ID_GRID_16: g_qeglobals.d_gridsize = 4; break;
				case ID_GRID_32: g_qeglobals.d_gridsize = 5; break;
				case ID_GRID_64: g_qeglobals.d_gridsize = 6; break;
				case ID_GRID_128: g_qeglobals.d_gridsize = 7; break;
				case ID_GRID_256: g_qeglobals.d_gridsize = 8; break;
			}
			g_qeglobals.d_gridsize = 1 << g_qeglobals.d_gridsize;

			CheckMenuItem(hMenu, LOWORD(wParam), MF_BYCOMMAND | MF_CHECKED);
			Sys_UpdateWindows (W_XY|W_Z);
			Sys_UpdateGridStatusBar ();
			break;
		}
		
		case ID_SHOWGRID:
			g_qeglobals.d_showgrid ^= 1;
			CheckMenuItem ( GetMenu(hWnd), ID_SHOWGRID, MF_BYCOMMAND | (g_qeglobals.d_showgrid ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_UpdateWindows (W_XY|W_Z);
			break;
		
		case ID_SNAPTOGRID:
			g_qeglobals.d_savedinfo.noclamp ^= 1;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_SNAPTOGRID, MF_BYCOMMAND | ((!g_qeglobals.d_savedinfo.noclamp) ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_UpdateGridStatusBar ();
			break;
//
// texture menu
//
		case ID_VIEW_TEXTURES:
			Texture_SetMode (g_qeglobals.d_savedinfo.iVTexMenu);
			break;

		case ID_VIEW_NEAREST:
		case ID_VIEW_NEARESTMIPMAP:
		case ID_VIEW_LINEAR:
		case ID_VIEW_BILINEAR:
		case ID_VIEW_BILINEARMIPMAP:
		case ID_VIEW_TRILINEAR:
		case ID_TEXTURES_WIREFRAME:
		case ID_TEXTURES_FLATSHADE:
			Texture_SetMode (LOWORD(wParam));
			break;

		case ID_TEXTURES_LOCK:
			g_qeglobals.d_textures_lock ^= 1;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_TEXTURES_LOCK, MF_BYCOMMAND | (g_qeglobals.d_textures_lock ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_UpdateWindows (W_CAMERA);
			Sys_UpdateGridStatusBar ();
			break;

		case ID_TEXTURES_FLUSH_ALL:
			Sys_BeginWait ();
			Map_New ();
			Texture_Flush ();
			Texture_Init ();
			SetInspectorMode(W_TEXTURE);
			Sys_UpdateWindows(W_TEXTURE);
			SetWindowText(g_qeglobals.d_hwndEntity, "flushed all");
			break;

		case ID_TEXTURES_FLUSH_UNUSED:
			Sys_BeginWait ();
			Texture_ShowInuse ();
			Texture_FlushUnused ();
			SetInspectorMode(W_TEXTURE);
			Sys_UpdateWindows(W_TEXTURE);
			SetWindowText(g_qeglobals.d_hwndEntity, "flushed unused");
			break;

		case ID_TEXTURES_SHOWINUSE:
			Sys_BeginWait ();
			Texture_ShowInuse ();
			SetInspectorMode(W_TEXTURE);
			break;

		case ID_TEXTURES_UPDATEMENU:
			Sys_BeginWait ();
			FillTextureMenu ();
			SetInspectorMode (W_CONSOLE);
			break;

		case ID_TEXTURES_INSPECTOR:
			DoSurface ();
			break;

		case ID_TEXTURE_REPLACEALL:
			DoFindTexture();
			break;

		case CMD_TEXTUREWAD:
		case CMD_TEXTUREWAD+1:
		case CMD_TEXTUREWAD+2:
		case CMD_TEXTUREWAD+3:
		case CMD_TEXTUREWAD+4:
		case CMD_TEXTUREWAD+5:
		case CMD_TEXTUREWAD+6:
		case CMD_TEXTUREWAD+7:
		case CMD_TEXTUREWAD+8:
		case CMD_TEXTUREWAD+9:
		case CMD_TEXTUREWAD+10:
		case CMD_TEXTUREWAD+11:
		case CMD_TEXTUREWAD+12:
		case CMD_TEXTUREWAD+13:
		case CMD_TEXTUREWAD+14:
		case CMD_TEXTUREWAD+15:
		case CMD_TEXTUREWAD+16:
		case CMD_TEXTUREWAD+17:
		case CMD_TEXTUREWAD+18:
		case CMD_TEXTUREWAD+19:
		case CMD_TEXTUREWAD+20:
		case CMD_TEXTUREWAD+21:
		case CMD_TEXTUREWAD+22:
		case CMD_TEXTUREWAD+23:
		case CMD_TEXTUREWAD+24:
		case CMD_TEXTUREWAD+25:
		case CMD_TEXTUREWAD+26:
		case CMD_TEXTUREWAD+27:
		case CMD_TEXTUREWAD+28:
		case CMD_TEXTUREWAD+29:
		case CMD_TEXTUREWAD+30:
		case CMD_TEXTUREWAD+31:
			Sys_BeginWait ();
			Texture_ShowWad (LOWORD(wParam));
			SetInspectorMode(W_TEXTURE);
			break;

//
// bsp menu
//
		case CMD_BSPCOMMAND:
		case CMD_BSPCOMMAND+1:
		case CMD_BSPCOMMAND+2:
		case CMD_BSPCOMMAND+3:
		case CMD_BSPCOMMAND+4:
		case CMD_BSPCOMMAND+5:
		case CMD_BSPCOMMAND+6:
		case CMD_BSPCOMMAND+7:
		case CMD_BSPCOMMAND+8:
		case CMD_BSPCOMMAND+9:
		case CMD_BSPCOMMAND+10:
		case CMD_BSPCOMMAND+11:
		case CMD_BSPCOMMAND+12:
		case CMD_BSPCOMMAND+13:
		case CMD_BSPCOMMAND+14:
		case CMD_BSPCOMMAND+15:
		case CMD_BSPCOMMAND+16:
		case CMD_BSPCOMMAND+17:
		case CMD_BSPCOMMAND+18:
		case CMD_BSPCOMMAND+19:
		case CMD_BSPCOMMAND+20:
		case CMD_BSPCOMMAND+21:
		case CMD_BSPCOMMAND+22:
		case CMD_BSPCOMMAND+23:
		case CMD_BSPCOMMAND+24:
		case CMD_BSPCOMMAND+25:
		case CMD_BSPCOMMAND+26:
		case CMD_BSPCOMMAND+27:
		case CMD_BSPCOMMAND+28:
		case CMD_BSPCOMMAND+29:
		case CMD_BSPCOMMAND+30:
		case CMD_BSPCOMMAND+31:
			{
				extern	char	*bsp_commands[256];

				RunBsp (bsp_commands[LOWORD(wParam-CMD_BSPCOMMAND)]);
			}
			break;

//
// misc menu
//
		case ID_MISC_BENCHMARK:
			SendMessage ( g_qeglobals.d_hwndCamera,
			WM_USER+267,	0, 0);
			SetInspectorMode(W_CONSOLE);
			break;

		case ID_MISC_SELECTENTITYCOLOR:
			{
				
				if ( ( g_qeglobals.d_inspector_mode == W_ENTITY ) && DoColor(COLOR_ENTITY) == true )
				{
					extern void AddProp( void );
					
					char buffer[100];
					
					sprintf( buffer, "%f %f %f", g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][0],
						g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][1],
						g_qeglobals.d_savedinfo.colors[COLOR_ENTITY][2] );
					
					SetWindowText( hwndEnt[EntValueField], buffer );
					SetWindowText( hwndEnt[EntKeyField], "_color" );
					AddProp();
				}
				Sys_UpdateWindows( W_ALL );
			}
			break;

		case ID_MISC_PRINTXY:
			WXY_Print();
			break;

		case ID_CAMERABK:
			DoColor(COLOR_CAMERABACK);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_TEXTUREBK:
			DoColor(COLOR_TEXTUREBACK);
			Sys_UpdateWindows (W_ALL);
			break;
		case ID_TEXTURE_TEXT:
			DoColor(COLOR_TEXTURETEXT);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_COLORS_XYBK:
			DoColor(COLOR_GRIDBACK);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_COLORS_MAJOR:
			DoColor(COLOR_GRIDMAJOR);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_COLORS_MINOR:
			DoColor(COLOR_GRIDMINOR);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_COLORS_GRIDTEXT:
			DoColor(COLOR_GRIDTEXT);
			Sys_UpdateWindows (W_ALL);
			break;
		case ID_COLORS_GRIDBLOCK:
			DoColor(COLOR_GRIDBLOCK);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_COLORS_BRUSH:
			DoColor(COLOR_BRUSHES);
			Sys_UpdateWindows (W_ALL);
			break;
		case ID_COLORS_SELECTEDBRUSH:
			DoColor(COLOR_SELBRUSHES);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_COLORS_CLIPPER:
			DoColor(COLOR_CLIPPER);
			Sys_UpdateWindows (W_ALL);
			break;
		case ID_COLORS_VIEWNAME:
			DoColor(COLOR_VIEWNAME);
			Sys_UpdateWindows (W_ALL);
			break;

		case ID_MISC_GAMMA:
			{
				float g = g_qeglobals.d_savedinfo.fGamma;
				
				DoGamma();
				if (g != g_qeglobals.d_savedinfo.fGamma)
					MessageBox(g_qeglobals.d_hwndMain, "You must restart QE3 for Gamma settings to take place", "Gamma", MB_OK | MB_ICONINFORMATION); 
			}
			break;

		case IDC_CHECK_LOG:
			g_qeglobals.d_savedinfo.logconsole = !g_qeglobals.d_savedinfo.logconsole;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), IDC_CHECK_LOG, MF_BYCOMMAND | (g_qeglobals.d_savedinfo.logconsole ? MF_CHECKED : MF_UNCHECKED)  );
			Sys_LogFile();
			break;

		case ID_MISC_HEXEN2_MAP:
			g_qeglobals.d_savedinfo.hexen2_map = !g_qeglobals.d_savedinfo.hexen2_map;
			CheckMenuItem ( GetMenu(g_qeglobals.d_hwndMain), ID_MISC_HEXEN2_MAP, MF_BYCOMMAND | (g_qeglobals.d_savedinfo.hexen2_map ? MF_CHECKED : MF_UNCHECKED)  );
			break;

		case ID_MISC_FINDBRUSH:
			DoFindBrush();
			break;

		case ID_MISC_NEXTLEAKSPOT:
			Pointfile_Next();
			break;
		case ID_MISC_PREVIOUSLEAKSPOT:
			Pointfile_Prev();
			break;

//
// brush menu
//
		case ID_BRUSH_3SIDED:
			Brush_MakeSided (3);
			break;
		case ID_BRUSH_4SIDED:
			Brush_MakeSided (4);
			break;
		case ID_BRUSH_5SIDED:
			Brush_MakeSided (5);
			break;
		case ID_BRUSH_6SIDED:
			Brush_MakeSided (6);
			break;
		case ID_BRUSH_7SIDED:
			Brush_MakeSided (7);
			break;
		case ID_BRUSH_8SIDED:
			Brush_MakeSided (8);
			break;
		case ID_BRUSH_9SIDED:
			Brush_MakeSided (9);
			break;
		case ID_BRUSH_ARBITRARYSIDED:
			DoSides ();
			break;

//
// select menu
//
		case ID_BRUSH_FLIPX:
			Select_FlipAxis (0);
			break;
		case ID_BRUSH_FLIPY:
			Select_FlipAxis (1);
			break;
		case ID_BRUSH_FLIPZ:
			Select_FlipAxis (2);
			break;
		case ID_BRUSH_ROTATEX:
			Select_RotateAxis (0, 90);
			break;
		case ID_BRUSH_ROTATEY:
			Select_RotateAxis (1, 90);
			break;
		case ID_BRUSH_ROTATEZ:
			Select_RotateAxis (2, 90);
			break;

		case ID_SELECTION_ARBITRARYROTATION:
			DoRotate ();
			break;

		case ID_SELECTION_UNGROUPENTITY:
			Select_Ungroup ();
			break;

		case ID_SELECTION_CONNECT:
			ConnectEntities ();
			break;

		case ID_SELECTION_DRAGVERTECIES:
			if (g_qeglobals.d_select_mode == sel_vertex)
			{
				g_qeglobals.d_select_mode = sel_brush;
				Sys_UpdateWindows (W_ALL);
			}
			else
			{
				SetupVertexSelection ();
				if (g_qeglobals.d_numpoints)
					g_qeglobals.d_select_mode = sel_vertex;
			}
			break;
		case ID_SELECTION_DRAGEDGES:
			if (g_qeglobals.d_select_mode == sel_edge)
			{
				g_qeglobals.d_select_mode = sel_brush;
				Sys_UpdateWindows (W_ALL);
			}
			else
			{
				SetupVertexSelection ();
				if (g_qeglobals.d_numpoints)
					g_qeglobals.d_select_mode = sel_edge;
			}
			break;

		case ID_SELECTION_SELECTPARTIALTALL:
			Select_PartialTall ();
			break;
		case ID_SELECTION_SELECTCOMPLETETALL:
			Select_CompleteTall ();
			break;
		case ID_SELECTION_SELECTTOUCHING:
			Select_Touching ();
			break;
		case ID_SELECTION_SELECTINSIDE:
			Select_Inside ();
			break;

		case ID_SELECTION_MAKEHOLLOW:
			CSG_MakeHollow ();
			break;
		case ID_SELECTION_CSGSUBTRACT:
			CSG_Subtract ();
			break;
		case ID_SELECTION_CSGMERGE:
			CSG_Merge ();
			break;

		case ID_SELECTION_INVERT:
			Select_Invert ();
			break;

		case ID_SELECTION_CLONE:
			Select_Clone ();
			break;
		case ID_SELECTION_DELETE:
			Select_Delete ();
			break;
		case ID_SELECTION_DESELECT:
			if (g_qeglobals.d_clipmode)
				UnsetClipMode ();
			else
				Select_Deselect ();
			break;

		case ID_SELECT_GROUPNEXTBRUSH:
			GroupSelectNextBrush ();
			break;

		case ID_VIEW_CLIPPER:
			SetClipMode ();
			break;
		case ID_CLIP_SELECTED:
			Clip ();
			break;
		case ID_SPLIT_SELECTED:
			SplitClip ();
			break;
		case ID_FLIP_CLIP:
			FlipClip ();
			break;

		case ID_VIEW_HIDESHOW_HIDESELECTED:
			Select_Hide();  
			Select_Deselect();
			break;
		case ID_VIEW_HIDESHOW_SHOWHIDDEN:
			Select_ShowAllHidden();
			break;
//
// region menu
//
		case ID_REGION_OFF:
			Map_RegionOff ();
			break;
		case ID_REGION_SETXY:
			Map_RegionXY ();
			break;
		case ID_REGION_SETTALLBRUSH:
			Map_RegionTallBrush ();
			break;
		case ID_REGION_SETBRUSH:
			Map_RegionBrush ();
			break;
		case ID_REGION_SETSELECTION:
			Map_RegionSelectedBrushes ();
			break;

		case IDMRU+1:
		case IDMRU+2:
		case IDMRU+3:
		case IDMRU+4:
		case IDMRU+5:
		case IDMRU+6:
		case IDMRU+7:
		case IDMRU+8:
		case IDMRU+9:
			DoMru(hWnd,LOWORD(wParam));
			break;

//
// help menu
//

		case ID_HELP_ABOUT:
			DoAbout();
			break;

		default:
            return FALSE;
    }

    return TRUE;
}

/*
============
WMAIN_WndProc
============
*/
LONG WINAPI WMAIN_WndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    LONG    lRet = 1;
    RECT	rect;
	HDC		maindc;

    GetClientRect(hWnd, &rect);

    switch (uMsg)
    {
	case WM_TIMER:
		QE_CountBrushesAndUpdateStatusBar();
		QE_CheckAutoSave();
		return 0;

	case WM_DESTROY:
		SaveMruInReg(g_qeglobals.d_lpMruMenu,"Software\\id\\QuakeEd3\\MRU");
		DeleteMruMenu(g_qeglobals.d_lpMruMenu);
		PostQuitMessage(0);
		KillTimer( hWnd, QE_TIMER0 );
		return 0;

	case WM_CREATE:
        maindc = GetDC(hWnd);
//	    QEW_SetupPixelFormat(maindc, false);
		g_qeglobals.d_lpMruMenu = CreateMruMenuDefault();
		LoadMruInReg(g_qeglobals.d_lpMruMenu,"Software\\id\\QuakeEd3\\MRU");
	
		// Refresh the File menu.
		PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(hWnd),0),
				ID_FILE_EXIT);

		return 0;

	case WM_SIZE:
		// resize the status window
		MoveWindow( g_qeglobals.d_hwndStatus, -100, 100, 10, 10, true);
		return 0;

	case WM_KEYDOWN:
		return QE_KeyDown (wParam);

   	case WM_CLOSE:
        /* call destroy window to cleanup and go away */
		SaveWindowState(g_qeglobals.d_hwndXY, "xywindow");
		SaveWindowState(g_qeglobals.d_hwndCamera, "camerawindow");
		SaveWindowState(g_qeglobals.d_hwndZ, "zwindow");
		SaveWindowState(g_qeglobals.d_hwndEntity, "EntityWindow");
		SaveWindowState(g_qeglobals.d_hwndMain, "mainwindow");

		// FIXME: is this right?
		SaveRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, sizeof(g_qeglobals.d_savedinfo));
        DestroyWindow (hWnd);
		return 0;

    case WM_COMMAND:
		return CommandHandler (hWnd, wParam, lParam);
		return 0;
    }

    return DefWindowProc (hWnd, uMsg, wParam, lParam);
}




/*
==============
Main_Create
==============
*/
void Main_Create (HINSTANCE hInstance)
{
    WNDCLASS   wc;
	int		   i;
	HMENU      hMenu;

    /* Register the main class */
	memset (&wc, 0, sizeof(wc));

    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)WMAIN_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = 0;
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    wc.lpszClassName = MAIN_WINDOW_CLASS;

    if (!RegisterClass (&wc) )
        Error ("Main_Register: failed");

	g_qeglobals.d_hwndMain = CreateWindow (MAIN_WINDOW_CLASS ,
		"QuakeEd 3",
		WS_OVERLAPPEDWINDOW |
		WS_CLIPSIBLINGS |
		WS_CLIPCHILDREN,
		0,0,screen_width,screen_height+GetSystemMetrics(SM_CYSIZE),	// size
		0,
		0,		// no menu
		hInstance,
		NULL);

	if (!g_qeglobals.d_hwndMain)
		Error ("Couldn't create Main Window");

	/* create a timer so that we can count brushes */
	SetTimer( g_qeglobals.d_hwndMain, 
			  QE_TIMER0,
			  1000,
		      NULL );
	
	LoadWindowState(g_qeglobals.d_hwndMain, "mainwindow");

	s_hwndToolbar = CreateToolBar(hInstance);
	
	g_qeglobals.d_hwndStatus = CreateStatusBar(hInstance);

	//
	// load misc info from registry
	//
	i = sizeof(g_qeglobals.d_savedinfo);
	LoadRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, &i);
	
	if (g_qeglobals.d_savedinfo.iSize != sizeof(g_qeglobals.d_savedinfo))
	{
		// fill in new defaults

		g_qeglobals.d_savedinfo.iSize = sizeof(g_qeglobals.d_savedinfo);
		g_qeglobals.d_savedinfo.fGamma = 1.0;
		g_qeglobals.d_savedinfo.iTexMenu = ID_VIEW_LINEAR;

		g_qeglobals.d_savedinfo.exclude = 0;
		g_qeglobals.d_savedinfo.show_coordinates = true;
		g_qeglobals.d_savedinfo.show_names       = true;
		g_qeglobals.d_savedinfo.check_sizepaint  = true;
		g_qeglobals.d_savedinfo.view_radiantlights = true;
		g_qeglobals.d_savedinfo.noclamp          = false;
		g_qeglobals.d_savedinfo.logconsole       = false;
		g_qeglobals.d_savedinfo.hexen2_map       = false;

		for (i=0 ; i<3 ; i++)
		{
			g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][i] = 0.25;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][i] = 1.0;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][i] = 0.75;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][i] = 0.5;
			g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][i] = 0.25;
		}

		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][2] = 1.0;

		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0;
		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][2] = 1.0;

		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.5;
		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.75;

		g_qeglobals.d_savedinfo.colors[COLOR_TEXTURETEXT][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTURETEXT][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTURETEXT][2] = 0.0;

		Sys_UpdateWindows (W_ALL);
	}

	if ( ( hMenu = GetMenu( g_qeglobals.d_hwndMain ) ) != 0 )
	{
		/*
		** by default all of these are checked because that's how they're defined in the menu editor
		*/

		if ( g_qeglobals.d_savedinfo.logconsole )
			CheckMenuItem( hMenu, IDC_CHECK_LOG, MF_BYCOMMAND | MF_CHECKED );

		if ( g_qeglobals.d_savedinfo.hexen2_map )
			CheckMenuItem( hMenu, ID_MISC_HEXEN2_MAP, MF_BYCOMMAND | MF_CHECKED );

		if ( !g_qeglobals.d_savedinfo.check_sizepaint )
			CheckMenuItem( hMenu, IDC_CHECK_SIZEPAINT, MF_BYCOMMAND | MF_UNCHECKED );
		//
		if ( !g_qeglobals.d_savedinfo.view_radiantlights )
			CheckMenuItem( hMenu, ID_VIEW_RADIANTLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );

		if ( g_qeglobals.d_savedinfo.noclamp )
			CheckMenuItem( hMenu, ID_SNAPTOGRID, MF_BYCOMMAND | MF_UNCHECKED );

		if ( !g_qeglobals.d_savedinfo.show_names )
			CheckMenuItem( hMenu, ID_VIEW_SHOWNAMES, MF_BYCOMMAND | MF_UNCHECKED );
		if ( !g_qeglobals.d_savedinfo.show_coordinates )
			CheckMenuItem( hMenu, ID_VIEW_SHOWCOORDINATES, MF_BYCOMMAND | MF_UNCHECKED );

		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_ANGLES )
			CheckMenuItem( hMenu, ID_VIEW_SHOWANGLES, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_LIGHTS )
			CheckMenuItem( hMenu, ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_ENT )
			CheckMenuItem( hMenu, ID_VIEW_ENTITY, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS )
			CheckMenuItem( hMenu, ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WATER )
			CheckMenuItem( hMenu, ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WORLD )
			CheckMenuItem( hMenu, ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_CLIP )
			CheckMenuItem( hMenu, ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_UNCHECKED );
	}

	ShowWindow (g_qeglobals.d_hwndMain, SW_SHOWDEFAULT);
}


/*
=============================================================

REGISTRY INFO

=============================================================
*/

BOOL SaveRegistryInfo(const char *pszName, void *pvBuf, long lSize)
{
	LONG lres;
	DWORD dwDisp;
	HKEY  hKeyId;

	lres = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\id\\QuakeEd3", 0, NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyId, &dwDisp);
    
	if (lres != ERROR_SUCCESS)
		return FALSE;

	lres = RegSetValueEx(hKeyId, pszName, 0, REG_BINARY, pvBuf, lSize);
    
	RegCloseKey(hKeyId);

	if (lres != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOL LoadRegistryInfo(const char *pszName, void *pvBuf, long *plSize)
{
	HKEY  hKey;
	long lres, lType, lSize;

	if (plSize == NULL)
		plSize = &lSize;

	lres = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\id\\QuakeEd3", 0, KEY_READ, &hKey);
    
	if (lres != ERROR_SUCCESS)
		return FALSE;

	lres = RegQueryValueEx(hKey, pszName, NULL, &lType, pvBuf, plSize);

	RegCloseKey(hKey);

	if (lres != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOL SaveWindowState(HWND hWnd, const char *pszName)
{
	RECT rc;
	
	GetWindowRect(hWnd, &rc);
	if (hWnd != g_qeglobals.d_hwndMain)
		MapWindowPoints(NULL, g_qeglobals.d_hwndMain, (POINT *)&rc, 2);
	return SaveRegistryInfo(pszName, &rc, sizeof(rc));
}


BOOL LoadWindowState(HWND hWnd, const char *pszName)
{
	RECT rc;
	LONG lSize = sizeof(rc);

	if (LoadRegistryInfo(pszName, &rc, &lSize))
	{
		if (rc.left < 0)
			rc.left = 0;
		if (rc.top < 0)
			rc.top = 0;
		if (rc.right < rc.left + 16)
			rc.right = rc.left + 16;
		if (rc.bottom < rc.top + 16)
			rc.bottom = rc.top + 16;

		MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, 
				rc.bottom - rc.top, FALSE);
		return TRUE;
	}

	return FALSE;
}

/*
===============================================================

  STATUS WINDOW

===============================================================
*/

void Sys_UpdateBrushStatusBar (void)
{
	extern int   g_numbrushes, g_numentities, g_numtextures;
	char numbrushbuffer[128]="";

	sprintf( numbrushbuffer, "Brushes: %d Entities: %d Textures: %d", g_numbrushes, g_numentities, g_numtextures );
	Sys_Status( numbrushbuffer, 2 );
}

void Sys_UpdateGridStatusBar (void)
{
	char gridstatus[256]="";

	sprintf( gridstatus, "Grid size: %d Snap to grid: %s Clip mode: %s Texture lock: %s", 
			g_qeglobals.d_gridsize, 
			g_qeglobals.d_savedinfo.noclamp?"off":"on", 
			g_qeglobals.d_clipmode?"on":"off", 
			g_qeglobals.d_textures_lock?"on":"off");
	Sys_Status( gridstatus, 1 );
}

void Sys_Status(const char *psz, int part )
{
	SendMessage(g_qeglobals.d_hwndStatus, SB_SETTEXT, part, (LPARAM)psz);
}

static HWND CreateStatusBar(HINSTANCE hInst)
{
	HWND hWnd;
	int partsize[4] = { 300, 640, 900, -1 }; // EER

	hWnd = CreateWindowEx( WS_EX_TOPMOST, // no extended styles
            STATUSCLASSNAME,                 // status bar
            "",                              // no text 
            WS_CHILD | WS_VISIBLE,  // styles
            -100, -100, 10, 10,              // x, y, cx, cy
            g_qeglobals.d_hwndMain,          // parent window
            (HMENU)100,                      // window ID
            hInst,                           // instance
            NULL);							 // window data

	SendMessage( hWnd, SB_SETPARTS, 4, ( long ) partsize ); // EER
    
	return hWnd;
}

//==============================================================

#define NUMBUTTONS 22
static HWND CreateToolBar(HINSTANCE hInst)
{ 
    HWND hwndTB; 
    TBADDBITMAP tbab; 
    TBBUTTON tbb[NUMBUTTONS]; 
    
     // Ensure that the common control DLL is loaded. 
    InitCommonControls(); 
 
    // Fill the TBBUTTON array with button information, and add the 
    // buttons to the toolbar. 
    tbb[0].iBitmap = 0; 
    tbb[0].idCommand = ID_BRUSH_FLIPX; 
    tbb[0].fsState = TBSTATE_ENABLED; 
    tbb[0].fsStyle = TBSTYLE_BUTTON; 
    tbb[0].dwData = 0; 
    tbb[0].iString = 0; 
    
    tbb[1].iBitmap = 2; 
    tbb[1].idCommand = ID_BRUSH_FLIPY; 
    tbb[1].fsState = TBSTATE_ENABLED; 
    tbb[1].fsStyle = TBSTYLE_BUTTON; 
    tbb[1].dwData = 0; 
    tbb[1].iString = 0; 
    
    tbb[2].iBitmap = 4; 
    tbb[2].idCommand = ID_BRUSH_FLIPZ; 
    tbb[2].fsState = TBSTATE_ENABLED;
    tbb[2].fsStyle = TBSTYLE_BUTTON; 
    tbb[2].dwData = 0; 
    tbb[2].iString = 0; 
    
    tbb[3].iBitmap = 1; 
    tbb[3].idCommand = ID_BRUSH_ROTATEX; 
    tbb[3].fsState = TBSTATE_ENABLED; 
    tbb[3].fsStyle = TBSTYLE_BUTTON; 
    tbb[3].dwData = 0; 
    tbb[3].iString = 0; 
    
    tbb[4].iBitmap = 3; 
    tbb[4].idCommand = ID_BRUSH_ROTATEY; 
    tbb[4].fsState = TBSTATE_ENABLED; 
    tbb[4].fsStyle = TBSTYLE_BUTTON; 
    tbb[4].dwData = 0; 
    tbb[4].iString = 0; 
    
    tbb[5].iBitmap = 5; 
    tbb[5].idCommand = ID_BRUSH_ROTATEZ; 
    tbb[5].fsState = TBSTATE_ENABLED; 
    tbb[5].fsStyle = TBSTYLE_BUTTON; 
    tbb[5].dwData = 0; 
    tbb[5].iString = 0; 
    
    tbb[6].iBitmap = 0;
    tbb[6].idCommand = 0;
    tbb[6].fsState = TBSTATE_ENABLED;
    tbb[6].fsStyle = TBSTYLE_SEP;
    tbb[6].dwData = 0;
    tbb[6].iString = 0;
    
    tbb[7].iBitmap = 6; 
    tbb[7].idCommand = ID_SELECTION_SELECTCOMPLETETALL; 
    tbb[7].fsState = TBSTATE_ENABLED; 
    tbb[7].fsStyle = TBSTYLE_BUTTON; 
    tbb[7].dwData = 0; 
    tbb[7].iString = 0; 
    
    tbb[8].iBitmap = 7; 
    tbb[8].idCommand = ID_SELECTION_SELECTTOUCHING; 
    tbb[8].fsState = TBSTATE_ENABLED; 
    tbb[8].fsStyle = TBSTYLE_BUTTON; 
    tbb[8].dwData = 0; 
    tbb[8].iString = 0; 
    
    tbb[9].iBitmap = 8; 
    tbb[9].idCommand = ID_SELECTION_SELECTPARTIALTALL; 
    tbb[9].fsState = TBSTATE_ENABLED; 
    tbb[9].fsStyle = TBSTYLE_BUTTON; 
    tbb[9].dwData = 0; 
    tbb[9].iString = 0; 
    
    tbb[10].iBitmap = 9; 
    tbb[10].idCommand = ID_SELECTION_SELECTINSIDE; 
    tbb[10].fsState = TBSTATE_ENABLED; 
    tbb[10].fsStyle = TBSTYLE_BUTTON; 
    tbb[10].dwData = 0; 
    tbb[10].iString = 0; 
    
    tbb[11].iBitmap = 0;
    tbb[11].idCommand = 0;
    tbb[11].fsState = TBSTATE_ENABLED;
    tbb[11].fsStyle = TBSTYLE_SEP;
    tbb[11].dwData = 0;
    tbb[11].iString = 0;
    
    tbb[12].iBitmap = 10;
    tbb[12].idCommand = ID_SELECTION_MAKEHOLLOW; 
    tbb[12].fsState = TBSTATE_ENABLED; 
    tbb[12].fsStyle = TBSTYLE_BUTTON; 
    tbb[12].dwData = 0; 
    tbb[12].iString = 0; 
    
    tbb[13].iBitmap = 11; 
    tbb[13].idCommand = ID_SELECTION_CSGSUBTRACT; 
    tbb[13].fsState = TBSTATE_ENABLED; 
    tbb[13].fsStyle = TBSTYLE_BUTTON; 
    tbb[13].dwData = 0; 
    tbb[13].iString = 0; 
    
    tbb[14].iBitmap = 12; 
    tbb[14].idCommand = ID_SELECTION_CSGMERGE; 
    tbb[14].fsState = TBSTATE_ENABLED; 
    tbb[14].fsStyle = TBSTYLE_BUTTON; 
    tbb[14].dwData = 0; 
    tbb[14].iString = 0; 
    
    tbb[15].iBitmap = 0;
    tbb[15].idCommand = 0;
    tbb[15].fsState = TBSTATE_ENABLED;
    tbb[15].fsStyle = TBSTYLE_SEP;
    tbb[15].dwData = 0;
    tbb[15].iString = 0;
    
    tbb[16].iBitmap = 13;
    tbb[16].idCommand = ID_TEXTURES_WIREFRAME; 
    tbb[16].fsState = TBSTATE_ENABLED; 
    tbb[16].fsStyle = TBSTYLE_BUTTON; 
    tbb[16].dwData = 0; 
    tbb[16].iString = 0; 
    
    tbb[17].iBitmap = 14;
    tbb[17].idCommand = ID_TEXTURES_FLATSHADE; 
    tbb[17].fsState = TBSTATE_ENABLED; 
    tbb[17].fsStyle = TBSTYLE_BUTTON; 
    tbb[17].dwData = 0; 
    tbb[17].iString = 0; 
    
    tbb[18].iBitmap = 15;
    tbb[18].idCommand = ID_VIEW_TEXTURES; 
    tbb[18].fsState = TBSTATE_ENABLED; 
    tbb[18].fsStyle = TBSTYLE_BUTTON; 
    tbb[18].dwData = 0; 
    tbb[18].iString = 0; 
    
    tbb[19].iBitmap = 16;
    tbb[19].idCommand = ID_TEXTURES_POPUP;
    tbb[19].fsState = TBSTATE_ENABLED;
    tbb[19].fsStyle = TBSTYLE_BUTTON;
    tbb[19].dwData = 0;
    tbb[19].iString = 0;
    
    tbb[20].iBitmap = 0;
    tbb[20].idCommand = 0;
    tbb[20].fsState = TBSTATE_ENABLED;
    tbb[20].fsStyle = TBSTYLE_SEP;
    tbb[20].dwData = 0;
    tbb[20].iString = 0;
    
    tbb[21].iBitmap = 17;
    tbb[21].idCommand = ID_VIEW_NEXTVIEW; 
    tbb[21].fsState = TBSTATE_ENABLED; 
    tbb[21].fsStyle = TBSTYLE_BUTTON; 
    tbb[21].dwData = 0; 
    tbb[21].iString = 0; 

    // Create a toolbar that the user can customize and that has a 
    // tooltip associated with it. 
    hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL, 
        WS_CHILD | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE, 
        0, 0, 0, 0, g_qeglobals.d_hwndMain, (HMENU) IDR_TOOLBAR1, hInst, NULL); 
 
    // Send the TB_BUTTONSTRUCTSIZE message, which is required for 
    // backward compatibility. 
    SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
 
    // Add the bitmap containing button images to the toolbar. 
    tbab.hInst = hInst; 
    tbab.nID   = IDR_TOOLBAR1; 
    SendMessage(hwndTB, TB_ADDBITMAP, (WPARAM)NUMBUTTONS, (WPARAM) &tbab); 

	//
    SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM)NUMBUTTONS,
        (LPARAM) (LPTBBUTTON) &tbb); 
 
    ShowWindow(hwndTB, SW_SHOW); 

    return hwndTB; 
}
