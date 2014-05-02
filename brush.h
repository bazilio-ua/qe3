// brush.h

typedef struct
{
	int		numpoints;
	int		maxpoints;
	float 	points[8][5];			// variable sized
} winding_t;

// the normals on planes point OUT of the brush
#define	MAXPOINTS	16
typedef struct face_s
{
	struct face_s	*next;
	struct face_s	*original;		//used for vertex movement
	struct brush_s	*owner;		// sikk - brush of selected face
	vec3_t		planepts[3];
    texdef_t	texdef;

    plane_t		plane;

	winding_t  *face_winding;

	vec3_t		d_color;
	qtexture_t *d_texture;

//	int         d_numpoints;
//	vec3_t     *d_points;
} face_t;

#define	MAX_FACES	16
typedef struct brush_s
{
	struct brush_s	*prev, *next;	// links in active/selected
	struct brush_s	*oprev, *onext;	// links in entity
	struct entity_s	*owner;
	vec3_t	mins, maxs;
	qboolean	hiddenbrush;

	face_t     *brush_faces;
} brush_t;


void     Brush_AddToList (brush_t *b, brush_t *list);
void     Brush_Build(brush_t *b);
void     Brush_BuildWindings( brush_t *b );
brush_t *Brush_Clone (brush_t *b);
brush_t	*Brush_Create (vec3_t mins, vec3_t maxs, texdef_t *texdef);
void     Brush_Draw( brush_t *b );

void     Brush_DrawXY(brush_t *b, int viewtype);

brush_t	*Brush_Alloc ();
void     Brush_Free (brush_t *b);
void     Brush_MakeSided (int sides);
void     Brush_Move (brush_t *b, vec3_t move);
qboolean Brush_MoveVertex (brush_t *b, vec3_t vertex, vec3_t delta, vec3_t end);
void     Brush_ResetFaceOriginals (brush_t *b);
brush_t *Brush_Parse (void);
face_t  *Brush_Ray (vec3_t origin, vec3_t dir, brush_t *b, float *dist);
void     Brush_RemoveFromList (brush_t *b);
void     Brush_SelectFaceForDragging (brush_t *b, face_t *f, qboolean shear);
void     Brush_SetTexture (brush_t *b, texdef_t *texdef);
void     Brush_SideSelect (brush_t *b, vec3_t origin, vec3_t dir, qboolean shear);
void     Brush_Write (brush_t *b, FILE *f);
void	Brush_RemoveEmptyFaces ( brush_t *b );
winding_t *Brush_MakeFaceWinding (brush_t *b, face_t *face);

int        AddPlanept (float *f);

face_t    *Face_Alloc ( void );
void       Face_Free ( face_t *f );
face_t	  *Face_Clone (face_t *f);
void       Face_MakePlane (face_t *f);
void       Face_Draw( face_t *face );

void	Face_MoveTexture(face_t *f, vec3_t delta);
void	Face_SetTexture (face_t *f, texdef_t *texdef);

void	TextureAxisFromPlane(plane_t *pln, vec3_t xv, vec3_t yv);
void	Brush_FitTexture			(brush_t *b, int height, int width);
void	Face_FitTexture				(face_t *face, int height, int width);

void	DrawLight(brush_t *b);
