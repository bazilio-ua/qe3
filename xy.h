
// window system independent camera view code

typedef struct
{
	int		width, height;

	qboolean	timing;

	vec3_t	origin;			// at center of window
	float	scale;

	float	topclip, bottomclip;

	qboolean d_dirty;
} xy_t;

typedef struct
{
	qboolean	bSet;
	vec3_t		ptClip;      // the 3d point
} clipPoint_t;

extern clipPoint_t	Clip1;
extern clipPoint_t	Clip2;
extern clipPoint_t	Clip3;
extern clipPoint_t	*pMovingClip;

BOOL XYExcludeBrush(brush_t	*pb);

void XY_Init (void);
void XY_MouseDown (int x, int y, int buttons);
void XY_MouseUp (int x, int y, int buttons);
void XY_MouseMoved (int x, int y, int buttons);
void XY_Draw (void);
void XY_Overlay (void);

void PaintSizeInfo (int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds);

BOOL FilterBrush (brush_t *pb);

void VectorCopyXY(vec3_t in, vec3_t out);
void PositionView (void);

void XY_ToPoint (int x, int y, vec3_t point);
void XY_ToGridPoint (int x, int y, vec3_t point);
void SnapToPoint (int x, int y, vec3_t point);

void DrawPathLines (void);
void DrawCameraIcon (void);

float Betwixt (float f1, float f2);
float fDiff (float f1, float f2);

void SetClipMode (void);
void UnsetClipMode (void);
void ResetClipMode (void);
void Clip (void);
void SplitClip (void);
void FlipClip (void);

void CleanList (brush_t *pList);
void ProduceSplitLists (void);
void Brush_CopyList (brush_t *pFrom, brush_t *pTo);

void DropClipPoint (int x, int y);
void MoveClipPoint (int x, int y);
void DrawClipPoint (void);
void EndClipPoint (void);

