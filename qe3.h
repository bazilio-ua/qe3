// qe3.h -- main header

#ifndef __QE3_H__
#define __QE3_H__

// disable data conversion warnings for gl
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
 
#include <windows.h>

#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glaux.h>
#include "glingr.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <io.h>

#include "cmdlib.h"
#include "mathlib.h"
#include "parse.h"
#include "lbmlib.h"

#include <commctrl.h>
#include "afxres.h"
#include "resource.h"

#include "qedefs.h"

typedef struct
{
    vec3_t	normal;
    double	dist;
    int		type;
} plane_t;

#include "qfiles.h"

#include "textures.h"
#include "brush.h"
#include "entity.h"
#include "map.h"
#include "select.h"

#include "camera.h"
#include "xy.h"
#include "z.h"
#include "winding.h"
#include "mru.h"

typedef struct
{
	int		p1, p2;
	face_t	*f1, *f2;
} pedge_t;

typedef struct
{
	int		  iSize;
	int		  iTexMenu;		// nearest, linear, etc
	int		  iVTexMenu;
	float	  fGamma;			// gamma for textures
	char	  szProject[256];	// last project loaded
	vec3_t	  colors[COLOR_LAST];
	qboolean  show_names,
			  show_coordinates,
			  check_sizepaint,
			  view_radiantlights,
			  noclamp,
			  logconsole,
			  hexen2_map;	// enable hexen2 unused 'light/rad' map flag, needed for h2bsp
	int       exclude;
} SavedInfo_t;

//
// system functions
//
void    Sys_UpdateBrushStatusBar (void);
void	Sys_UpdateGridStatusBar (void);
void    Sys_UpdateWindows (int bits);
void    Sys_Beep (void);
void    Sys_ClearPrintf (void);
void    Sys_Printf (char *text, ...);
double	Sys_DoubleTime (void);
void    Sys_GetCursorPos (int *x, int *y);
void    Sys_SetCursorPos (int x, int y);
void    Sys_SetTitle (char *text);
void    Sys_BeginWait (void);
void    Sys_EndWait (void);
void    Sys_Status(const char *psz, int part);
void	Sys_LogFile (void);

/*
** most of the QE globals are stored in this structure
*/
typedef struct
{
	qboolean d_showgrid;
	int      d_gridsize;

	int      d_num_entities;

	entity_t *d_project_entity;

	// defines the boundaries of the current work area
	// is used to guess brushes and drop points third coordinate when creating from 2D view
	vec3_t    d_work_min, d_work_max;
	// not stored in registry, default is off
	qboolean  d_show_work;

//	float     d_new_brush_bottom_z,
//		      d_new_brush_top_z;

	HINSTANCE d_hInstance;
	HWND      d_hInstanceColor; //EER1

	HGLRC     d_hglrcBase;
	HDC       d_hdcBase;

	HWND      d_hwndMain;
	HWND      d_hwndCamera;
	HWND      d_hwndConsole;
	HWND      d_hwndEntity;
	HWND      d_hwndTexture;
	HWND      d_hwndXY;

	HWND      d_hwndXZ; // unused
	HWND      d_hwndYZ; // unused

	HWND      d_hwndZ;
	HWND      d_hwndStatus;

	HWND      d_hwndActiveXY; // unused

	vec3_t    d_points[MAX_POINTS];
	int       d_numpoints;
	pedge_t   d_edges[MAX_EDGES];
	int       d_numedges;

	int       d_num_move_points;
	float    *d_move_points[1024];

	qtexture_t	*d_qtextures;

	texturewin_t d_texturewin;

	int	         d_pointfile_display_list;

	camera_t     d_camera;
	xy_t         d_xyz; // it's now for all view
	z_t          d_z;

	int          d_viewtype; // XY = x0,y1; XZ = x0,y2; YZ = x1,y2.

	LPMRUMENU    d_lpMruMenu;

	SavedInfo_t  d_savedinfo;

	int          d_workcount;

	// connect entities uses the last two brushes selected
	int			 d_select_count;
	brush_t		*d_select_order[2];
	vec3_t       d_select_translate;    // for dragging w/o making new display lists
	select_t     d_select_mode;

	int		     d_font_list;

	int          d_parsed_brushes;

	qboolean     d_show_blocks;

	qboolean     d_textures_lock;

	qboolean     d_clipmode;
	qboolean     d_clipswitch;
	
	brush_t      d_frontsplits;
	brush_t      d_backsplits;
	brush_t     *d_splitlist;

	int          d_inspector_mode;		// W_TEXTURE, W_ENTITY, or W_CONSOLE

	// handle to the console log file
	// we use low level I/O to get rid of buffering and have everything on file if we crash
	int          d_logfile;

} QEGlobals_t;

void *qmalloc (int size);
char *copystring (char *s);
char *ExpandReletivePath (char *p);

void Pointfile_Delete (void);
void Pointfile_Check (void);
void Pointfile_Next (void);
void Pointfile_Prev (void);
void Pointfile_Clear (void);
void Pointfile_Draw( void );
void Pointfile_Load( void );

//
// drag.c
//
void Drag_Begin (int x, int y, int buttons,
		   vec3_t xaxis, vec3_t yaxis,
		   vec3_t origin, vec3_t dir);
void Drag_MouseMoved (int x, int y, int buttons);
void Drag_MouseUp (void);

//
// csg.c
//
void CSG_MakeHollow (void);
void CSG_Subtract (void);
void CSG_Merge (void);

void CSG_SplitBrushByFace (brush_t *in, face_t *f, brush_t **front, brush_t **back);

//
// vertsel.c
//

void SetupVertexSelection (void);
void SelectEdgeByRay (vec3_t org, vec3_t dir);
void SelectVertexByRay (vec3_t org, vec3_t dir);

void ConnectEntities (void);

extern	int	update_bits;

extern	int	screen_width;
extern	int	screen_height;

extern	HANDLE	bsp_process;

char	*TranslateString (char *buf);

void ProjectDialog (void);

void FillTextureMenu (void);
void FillBSPMenu (void);

BOOL CALLBACK Win_Dialog (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
);


//
// win_cam.c
//
void WCam_Create (HINSTANCE hInstance);


//
// win_xy.c
//
void WXY_Create (HINSTANCE hInstance);

//
// win_z.c
//
void WZ_Create (HINSTANCE hInstance);

//
// win_ent.c
//


//
// win_main.c
//
void Main_Create (HINSTANCE hInstance);
extern BOOL SaveWindowState(HWND hWnd, const char *pszName);
extern BOOL LoadWindowState(HWND hWnd, const char *pszName);

extern BOOL SaveRegistryInfo(const char *pszName, void *pvBuf, long lSize);
extern BOOL loadRegistryInfo(const char *pszName, void *pvBuf, long *plSize);

//
// entityw.c
//
void CreateEntityWindow(HINSTANCE hInstance);
void FillClassList (void);
BOOL UpdateEntitySel(eclass_t *pec);	
void SetInspectorMode(int iType);
int DrawTexControls(HWND hWnd);
void SetSpawnFlags(void);
void GetSpawnFlags(void);
void SetKeyValuePairs(void);
extern void BuildGammaTable(float g);


// win_dlg.c

void DoGamma(void);
void DoFindBrush(void);
void DoRotate(void);
void DoSides(void);
void DoAbout(void);
void DoSurface(void);
void DoFindTexture(void);

/*
** QE function declarations
*/
void     QE_CheckAutoSave( void );
void     QE_ConvertDOSToUnixName( char *dst, const char *src );
void     QE_CountBrushesAndUpdateStatusBar( void );
void     QE_CheckOpenGLForErrors(void);
void     QE_ExpandBspString (char *bspaction, char *out, char *mapname);
void     QE_Init (void);
qboolean QE_KeyDown (int key);
qboolean QE_LoadProject (char *projectfile);
qboolean QE_SingleBrush (void);

/*
** QE Win32 function declarations
*/
int  QEW_SetupPixelFormat(HDC hDC, qboolean zbuffer );
void QEW_StopGL( HWND hWnd, HGLRC hGLRC, HDC hDC );

/*
** extern declarations
*/
extern QEGlobals_t   g_qeglobals;

#endif

