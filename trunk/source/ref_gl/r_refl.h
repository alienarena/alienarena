// gl_refl.h
// by Matt Ownby

// max # of reflections we will draw
// (this can be arbitrarily large, but of course performace will suffer)
void R_init_refl (int maxNoReflections); 
void R_setupArrays (int maxNoReflections);
void R_clear_refl (void);
void R_add_refl (float x, float y, float z);

static int txm_genTexObject(unsigned char *texData, int w, int h,
								int format, qboolean repeat, qboolean mipmap);
void R_RecursiveFindRefl (mnode_t *node); //may try this again
void R_DrawDebugReflTexture (void);
void R_UpdateReflTex (refdef_t *fd);
void R_DoReflTransform (void);
void R_LoadReflMatrix (void);
void R_ClearReflMatrix (void);

void mesa_frustum (GLdouble left, GLdouble right,
        GLdouble bottom, GLdouble top, 
        GLdouble nearval, GLdouble farval);

//////////////////////////////////

// vars other files need access to
extern qboolean g_drawing_refl;
extern qboolean g_refl_enabled;
extern unsigned int g_reflTexW, g_reflTexH;
extern float g_refl_aspect;
extern float *g_refl_X;
extern float *g_refl_Y;
extern float *g_refl_Z;
extern int *g_tex_num;
extern int g_active_refl;
extern int g_num_refl;
extern unsigned int g_water_program_id; // jitwater
