// texture.h

typedef struct
{
	char	name[32];
	float	shift[2];
	float	rotate;
	float	scale[2];
} texdef_t;

typedef struct
{
	int			width, height;
	int			originy;
	texdef_t	texdef;
} texturewin_t;

typedef struct qtexture_s
{
	struct	qtexture_s *next;
	char	name[64];		// includes partial directory and extension
    int		width,  height;
	int		texture_number;	// gl bind number
	vec3_t	color;			// for flat shade mode
	qboolean	inuse;		// true = is present on the level
} qtexture_t;


// a texturename of the form (0 0 0) will
// create a solid color texture

extern	char		currentwad[1024];
extern	char		wadstring[1024];

void	Texture_Init (void);
void	Texture_Flush (void);
void	Texture_FlushUnused (void);
void	Texture_MakeNotexture (void);
void	Texture_ClearInuse (void);
void	Texture_ShowInuse (void);
void	Texture_ShowWad (int menunum);
void	Texture_InitFromWad (char *file);
void	Texture_SetTexture (texdef_t *texdef, qboolean set_selection);	// sikk - Multiple Face Selection: added set_selection
void	Texture_SetMode(int iMenu);	// GL_NEAREST, etc..
qtexture_t *Texture_ForName (char *name);

