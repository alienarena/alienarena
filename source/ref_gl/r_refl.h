// gl_refl.h
// by Matt Ownby

// max # of reflections we will draw
// (this can be arbitrarily large, but of course performace will suffer)
void R_init_refl	( int maxNoReflections  ); 
void R_setupArrays	( int maxNoReflections	);
void R_clear_refl	( void					);
void R_add_refl		( float Z, float distance);

static int txm_genTexObject(unsigned char *texData, int w, int h,
								int format, qboolean repeat, qboolean mipmap);
void R_RecursiveFindRefl (mnode_t *node); //may try this again
void R_DrawDebugReflTexture();
void R_UpdateReflTex(refdef_t *fd);
void R_DoReflTransform();
void R_LoadReflMatrix();
void R_ClearReflMatrix();

void mesa_frustum(GLdouble left, GLdouble right,
        GLdouble bottom, GLdouble top, 
        GLdouble nearval, GLdouble farval);

//////////////////////////////////

// vars other files need access to
extern qboolean g_drawing_refl;
extern qboolean g_refl_enabled;
extern unsigned int g_reflTexW, g_reflTexH;
extern float g_refl_aspect;
extern float *g_refl_Z;
extern int *g_tex_num;
extern int g_active_refl;
extern int g_num_refl;
