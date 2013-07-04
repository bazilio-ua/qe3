#ifndef __QEDEFS_H__
#define __QEDEFS_H__

#define QE_VERSION  0x0302

#define QE3_STYLE (WS_OVERLAPPED| WS_CAPTION | WS_THICKFRAME | \
		/* WS_MINIMIZEBOX | */ WS_MAXIMIZEBOX  | WS_CLIPSIBLINGS | \
		WS_CLIPCHILDREN | WS_CHILD)

#define QE_AUTOSAVE_INTERVAL  5       // number of minutes between autosaves

#define	MAIN_WINDOW_CLASS	"QMAIN"
#define	CAMERA_WINDOW_CLASS	"QCAM"
#define	XY_WINDOW_CLASS	    "QXY"

#define	XZ_WINDOW_CLASS	    "QXZ" // unused
#define	YZ_WINDOW_CLASS	    "QYZ" // unused

#define	Z_WINDOW_CLASS   	"QZ"
#define	ENT_WINDOW_CLASS	"QENT"
#define	TEXTURE_WINDOW_CLASS	"QTEX"

#define	ZWIN_WIDTH	40
#define CWIN_SIZE	(0.4)

#define	MAX_EDGES	256
#define	MAX_POINTS	512

#define	CMD_TEXTUREWAD	60000
#define	CMD_BSPCOMMAND	61000

#define	PITCH	0	// up / down
#define	YAW		1	// left / right
#define	ROLL	2	// fall over

#define QE_TIMER0   1

#define	PLANE_X		0
#define	PLANE_Y		1
#define	PLANE_Z		2
#define	PLANE_ANYX	3
#define	PLANE_ANYY	4
#define	PLANE_ANYZ	5

#define	ON_EPSILON	0.01

#define	KEY_FORWARD		1
#define	KEY_BACK		2
#define	KEY_TURNLEFT	4
#define	KEY_TURNRIGHT	8
#define	KEY_LEFT		16
#define	KEY_RIGHT		32
#define	KEY_LOOKUP		64
#define	KEY_LOOKDOWN	128
#define	KEY_UP			256
#define	KEY_DOWN		512

// xy.c
#define EXCLUDE_LIGHTS		1
#define EXCLUDE_ENT			2
#define EXCLUDE_PATHS		4
#define EXCLUDE_WATER		8
#define EXCLUDE_WORLD		16
#define EXCLUDE_CLIP		32
#define	EXCLUDE_FUNC_WALL	64
#define	EXCLUDE_SKY			128
#define	EXCLUDE_ANGLES		256


#define     ECLASS_LIGHT      0x00000001
#define     ECLASS_ANGLE      0x00000002
#define     ECLASS_PATH       0x00000004

//
// menu indexes for modifying menus
//
#define	MENU_VIEW		2
#define	MENU_BSP		4
#define	MENU_TEXTURE	6


// odd things not in windows header...
#define	VK_COMMA		188
#define	VK_PERIOD		190

/*
** window bits
*/
#define	W_CAMERA		0x0001
#define	W_XY			0x0002
#define	W_XY_OVERLAY	0x0004
#define	W_Z				0x0008
#define	W_TEXTURE		0x0010
#define	W_Z_OVERLAY		0x0020
#define W_CONSOLE		0x0040
#define W_ENTITY		0x0080

#define W_XZ			0x0100 // unused
#define W_YZ			0x0200 // unused

#define	W_ALL			0xFFFFFFFF


#define	COLOR_TEXTUREBACK	0
#define	COLOR_GRIDBACK		1
#define	COLOR_GRIDMINOR		2
#define	COLOR_GRIDMAJOR		3
#define	COLOR_CAMERABACK	4
#define COLOR_ENTITY      5
#define COLOR_GRIDBLOCK   6
#define COLOR_GRIDTEXT    7
#define COLOR_BRUSHES     8
#define COLOR_SELBRUSHES  9
#define COLOR_CLIPPER     10
#define COLOR_VIEWNAME    11
#define COLOR_TEXTURETEXT    12
#define COLOR_LAST        13


// used in some Drawing routines
enum VIEWTYPE {YZ, XZ, XY};
// XY = x0,y1; XZ = x0,y2; YZ = x1,y2.

#endif
