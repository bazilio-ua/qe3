// select.h

typedef enum
{
	sel_brush,
	// sel_sticky_brush,
	// sel_face,
	sel_vertex,
	sel_edge
} select_t;

typedef struct
{
	brush_t		*brush;
	face_t		*face;
	float		dist;
	qboolean	selected;
} trace_t;

#define	SF_SELECTED_ONLY	1
#define	SF_ENTITIES_FIRST	2
#define	SF_SINGLEFACE		4
#define	SF_CYCLE			8

trace_t Test_Ray (vec3_t origin, vec3_t dir, int flags);

void Select_GetBounds (vec3_t mins, vec3_t maxs);
void Select_GetTrueMid (vec3_t mid);
void Select_GetMid (vec3_t mid);
//void Select_Brush (brush_t *b);
void Select_Brush (brush_t *b, qboolean complete);
void Select_Ray (vec3_t origin, vec3_t dir, int flags);
void Select_Delete (void);
void Select_Deselect (void);
void Select_Clone (void);
void Select_Move (vec3_t delta);
void Select_SetTexture (texdef_t *texdef);
void Select_FlipAxis (int axis);
void Select_RotateAxis (int axis, float deg);
void Select_CompleteTall (void);
void Select_PartialTall (void);
void Select_Touching (void);
void Select_Inside (void);

void Select_Invert (void);
void Select_Hide (void);
void Select_ShowAllHidden (void);

void Select_FitTexture(int height, int width);

void Clamp(float *f, int clamp);
void ProjectOnPlane(vec3_t normal,float dist,vec3_t ez, vec3_t p);
void Back(vec3_t dir, vec3_t p);
void ComputeScale(vec3_t rex, vec3_t rey, vec3_t p, face_t *f);
void ComputeAbsolute(face_t *f, vec3_t p1, vec3_t p2, vec3_t p3);
void AbsoluteToLocal(plane_t normal2, face_t *f, vec3_t p1, vec3_t p2, vec3_t p3);
void RotateFaceTexture(face_t* f, int axis, float deg);
void RotateTextures(int axis, float deg, vec3_t origin);

// updating workzone to a given brush (depends on current view)
void UpdateWorkzone_ForBrush( brush_t *b );

void FindReplaceTextures (char *find, char *replace, qboolean selected, qboolean force);
void GroupSelectNextBrush (void);