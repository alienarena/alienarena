/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "client.h"
#include "qmenu.h"

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *vid_contrast;

extern cvar_t *r_bloom_intensity;
extern cvar_t *gl_normalmaps;
extern cvar_t *gl_shadowmaps;
extern cvar_t *gl_parallaxmaps;
extern cvar_t *gl_glsl_postprocess;
extern cvar_t *gl_glsl_shaders;
extern cvar_t *gl_modulate;
extern cvar_t *r_legacy;

extern cvar_t *vid_width;
extern cvar_t *vid_height;

static cvar_t *gl_mode;
static cvar_t *gl_picmip;
static cvar_t *gl_finish;
static cvar_t *gl_swapinterval;
static cvar_t *r_bloom;
static cvar_t *r_overbrightbits;
static cvar_t *gl_ext_mtexcombine;

static cvar_t *_windowed_mouse;

extern float banneralpha;

extern void M_ForceMenuOff( void );

#ifdef __unix__
extern qboolean vid_restart;
#endif

/*
====================================================================

MENU INTERACTION

====================================================================
*/

static menuframework_s	s_opengl_menu;
static menuframework_s *s_current_menu;
static int				s_current_menu_index;

static menulist_s		s_mode_list;
static menuslider_s		s_tq_slider;
static menuslider_s		s_brightness_slider;
static menuslider_s		s_contrast_slider;
static menulist_s  		s_fs_box;
static menulist_s  		s_finish_box;
static menulist_s		s_vsync_box;
static menulist_s  		s_windowed_mouse;
static menuaction_s		s_apply_action;
static menuaction_s		s_lowest_action;
static menuaction_s		s_low_action;
static menuaction_s		s_medium_action;
static menuaction_s		s_high_action;
static menuaction_s		s_highest_action;
static menulist_s		s_bloom_box;
static menuslider_s		s_bloom_slider;
static menulist_s		s_texcombine_box;
static menuslider_s		s_overbright_slider;
static menuslider_s		s_modulate_slider;
static menufield_s		s_height_field;
static menufield_s		s_width_field;
static menulist_s		s_normalmaps_box;
static menulist_s		s_shadowmaps_box;
static menulist_s		s_parallaxmaps_box;
static menulist_s		s_postprocess_box;
static menulist_s		s_glsl_box;

static void DriverCallback( void *unused )
{
	s_current_menu = &s_opengl_menu;
}

static void BrightnessCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;
	float gamma = slider->curvalue/10.0;
	Cvar_SetValue( "vid_gamma", gamma );
}

static void ContrastCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;
	float contrast = slider->curvalue/10.0;
	Cvar_SetValue( "vid_contrast", contrast );
}

static void BloomCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "r_bloom_intensity", slider->curvalue/10);
}

static void BloomSetCallback( void *s)
{
	Cvar_SetValue("r_bloom", s_bloom_box.curvalue);
}

static void MtexCallback( void *s)
{
	Cvar_SetValue("gl_ext_mtexcombine", s_texcombine_box.curvalue);
}

static void ModulateCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "gl_modulate", slider->curvalue);
}

static void NormalMapsCallback( void *s )
{
	Cvar_SetValue("gl_normalmaps", s_normalmaps_box.curvalue);
	if(s_normalmaps_box.curvalue) { //must turn this on for normal 
		Cvar_SetValue("gl_glsl_shaders", s_normalmaps_box.curvalue);
		s_glsl_box.curvalue = s_normalmaps_box.curvalue;
	}
}

static void ShadowMapsCallback( void *s )
{
	Cvar_SetValue("gl_shadowmaps", s_shadowmaps_box.curvalue);
	if(s_shadowmaps_box.curvalue) { //must turn this on for shadowmaps 
		Cvar_SetValue("gl_glsl_shaders", s_shadowmaps_box.curvalue);
		Cvar_SetValue("gl_shadows", 0);
		s_glsl_box.curvalue = s_shadowmaps_box.curvalue;
	}
}

static void ParallaxCallback( void *s )
{
	Cvar_SetValue("gl_parallaxmaps", s_parallaxmaps_box.curvalue);
	if(s_parallaxmaps_box.curvalue) { //must turn this on for parallax 
		Cvar_SetValue("gl_glsl_shaders", s_parallaxmaps_box.curvalue);
		s_glsl_box.curvalue = s_parallaxmaps_box.curvalue;
	}
}
static void PostProcessCallback( void *s )
{
	Cvar_SetValue("gl_parallaxmaps", s_postprocess_box.curvalue);
	if(s_postprocess_box.curvalue) { //must turn this on for parallax 
		Cvar_SetValue("gl_glsl_postprocess", s_postprocess_box.curvalue);
		s_glsl_box.curvalue = s_postprocess_box.curvalue;
	}
}
static void GlslCallback( void *s )
{
	Cvar_SetValue("gl_glsl_shaders", s_glsl_box.curvalue);
	if(!s_glsl_box.curvalue) { 
		Cvar_SetValue("gl_parallaxmaps", s_glsl_box.curvalue);
		s_parallaxmaps_box.curvalue = s_glsl_box.curvalue;
		Cvar_SetValue("gl_normalmaps", s_glsl_box.curvalue);
		s_normalmaps_box.curvalue = s_glsl_box.curvalue;
		Cvar_SetValue("gl_shadowmaps", s_glsl_box.curvalue);
		s_shadowmaps_box.curvalue = s_glsl_box.curvalue;
		Cvar_SetValue("gl_glsl_postprocess", s_glsl_box.curvalue);
		s_postprocess_box.curvalue = s_glsl_box.curvalue;
	}
}
static void SetLowest( void *unused )
{
	Cvar_SetValue("r_bloom", 0);
	Cvar_SetValue("r_bloom_intensity", 0.5);
	Cvar_SetValue("gl_ext_mtexcombine", 1);
	Cvar_SetValue("r_overbrightbits", 2);
	Cvar_SetValue("gl_modulate", 2);
	Cvar_SetValue("gl_picmip", 0);
	Cvar_SetValue("vid_gamma", 1);
	Cvar_SetValue("vid_contrast", 1);
	Cvar_SetValue("gl_normalmaps", 0);
	Cvar_SetValue("gl_shadowmaps", 0);
	Cvar_SetValue("gl_parallaxmaps", 0);
	Cvar_SetValue("gl_glsl_postprocess", 0);
	Cvar_SetValue("gl_glsl_shaders", 0);

	//do other things that aren't in the vid menu per se, but are related "high end" effects
	Cvar_SetValue("r_shaders", 0);
	Cvar_SetValue("gl_shadows", 0);
	Cvar_SetValue("gl_dynamic", 0);
	Cvar_SetValue("gl_mirror", 0);
	Cvar_SetValue("r_legacy", 1);

	VID_MenuInit();
}
static void SetLow( void *unused )
{
	Cvar_SetValue("r_bloom", 0);
	Cvar_SetValue("r_bloom_intensity", 0.5);
	Cvar_SetValue("gl_ext_mtexcombine", 1);
	Cvar_SetValue("r_overbrightbits", 2);
	Cvar_SetValue("gl_modulate", 2);
	Cvar_SetValue("gl_picmip", 0);
	Cvar_SetValue("vid_gamma", 1);
	Cvar_SetValue("vid_contrast", 1);
	Cvar_SetValue("gl_normalmaps", 0);
	Cvar_SetValue("gl_shadowmaps", 0);
	Cvar_SetValue("gl_parallaxmaps", 0);
	Cvar_SetValue("gl_glsl_postprocess", 0);
	Cvar_SetValue("gl_glsl_shaders", 0);

	//do other things that aren't in the vid menu per se, but are related "high end" effects
	Cvar_SetValue("r_shaders", 1);
	Cvar_SetValue("gl_shadows", 2);
	Cvar_SetValue("gl_dynamic", 0);
	Cvar_SetValue("gl_mirror", 1);
	Cvar_SetValue("r_legacy", 0);

	VID_MenuInit();
}

static void SetMedium( void *unused )
{
	Cvar_SetValue("r_bloom", 0);
	Cvar_SetValue("r_bloom_intensity", 0.5);
	Cvar_SetValue("gl_ext_mtexcombine", 1);
	Cvar_SetValue("r_overbrightbits", 2);
	Cvar_SetValue("gl_modulate", 2);
	Cvar_SetValue("gl_picmip", 0);
	Cvar_SetValue("vid_gamma", 1);
	Cvar_SetValue("vid_contrast", 1);
	Cvar_SetValue("gl_normalmaps", 0);
	Cvar_SetValue("gl_shadowmaps", 0);
	Cvar_SetValue("gl_parallaxmaps", 0);
	Cvar_SetValue("gl_glsl_postprocess", 1);
	Cvar_SetValue("gl_glsl_shaders", 1);

	//do other things that aren't in the vid menu per se, but are related "high end" effects
	Cvar_SetValue("r_shaders", 1);
	Cvar_SetValue("gl_shadows", 2);
	Cvar_SetValue("gl_dynamic", 1);
	Cvar_SetValue("gl_mirror", 1);
	Cvar_SetValue("r_legacy", 0);

	VID_MenuInit();
}

static void SetHigh( void *unused )
{
	Cvar_SetValue("r_bloom", 1);
	Cvar_SetValue("r_bloom_intensity", 0.5);
	Cvar_SetValue("gl_ext_mtexcombine", 1);
	Cvar_SetValue("r_overbrightbits", 2);
	Cvar_SetValue("gl_modulate", 2);
	Cvar_SetValue("gl_picmip", 0);
	Cvar_SetValue("vid_gamma", 1);
	Cvar_SetValue("vid_contrast", 1);
	Cvar_SetValue("gl_normalmaps", 1);
	Cvar_SetValue("gl_shadowmaps", 0);
	Cvar_SetValue("gl_parallaxmaps", 1);
	Cvar_SetValue("gl_glsl_postprocess", 1);
	Cvar_SetValue("gl_glsl_shaders", 1);

	//do other things that aren't in the vid menu per se, but are related "high end" effects
	Cvar_SetValue("r_shaders", 1);
	Cvar_SetValue("gl_shadows", 2);
	Cvar_SetValue("gl_dynamic", 1);
	Cvar_SetValue("gl_mirror", 1);
	Cvar_SetValue("r_legacy", 0);

	VID_MenuInit();
}

static void SetHighest( void *unused )
{
	Cvar_SetValue("r_bloom", 1);
	Cvar_SetValue("r_bloom_intensity", 0.5);
	Cvar_SetValue("gl_ext_mtexcombine", 1);
	Cvar_SetValue("r_overbrightbits", 2);
	Cvar_SetValue("gl_modulate", 2);
	Cvar_SetValue("gl_picmip", 0);
	Cvar_SetValue("vid_gamma", 1);
	Cvar_SetValue("vid_contrast", 1);
	Cvar_SetValue("gl_normalmaps", 1);
	Cvar_SetValue("gl_shadowmaps", 1);
	Cvar_SetValue("gl_parallaxmaps", 1);
	Cvar_SetValue("gl_glsl_postprocess", 1);
	Cvar_SetValue("gl_glsl_shaders", 1);

	//do other things that aren't in the vid menu per se, but are related "high end" effects
	Cvar_SetValue("r_shaders", 1);
	Cvar_SetValue("gl_shadows", 0);
	Cvar_SetValue("gl_dynamic", 1);
	Cvar_SetValue("gl_mirror", 1);
	Cvar_SetValue("r_legacy", 0);

	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
	float gamma;
	float contrast;
	int w, h;

	gamma = s_brightness_slider.curvalue/10.0;
	contrast = s_contrast_slider.curvalue/10.0;

	Cvar_SetValue( "vid_gamma", gamma );
	Cvar_SetValue( "vid_contrast", contrast );
	Cvar_SetValue( "gl_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValue( "vid_fullscreen", s_fs_box.curvalue );
	Cvar_SetValue( "gl_finish", s_finish_box.curvalue );
	Cvar_SetValue( "gl_swapinterval", s_vsync_box.curvalue );

	if (s_mode_list.curvalue == 10) {
		Cvar_SetValue( "gl_mode", -1);

		//set custom width and height only in this case
		//check for sane values
		w = atoi(s_width_field.buffer);
		if ( w > 2048 )
	       	{
			strcpy(s_width_field.buffer, "2048");
			w = 2048;
		}
		else if ( w < 640 )
		{
			strcpy(s_width_field.buffer, "640");
			w = 640;
		}

		h = atoi(s_height_field.buffer);
		if (h > 1536)
		{
			strcpy(s_height_field.buffer, "1536");
			h = 1536;
		}
	       	else if (h < 480)
		{
			strcpy(s_height_field.buffer, "480");
			h = 480;
		}

		Cvar_SetValue("vid_width", w);
		Cvar_SetValue("vid_height", h);
	}
	else
		Cvar_SetValue( "gl_mode", s_mode_list.curvalue ); 

	Cvar_SetValue( "r_bloom", s_bloom_box.curvalue);
	Cvar_SetValue( "r_bloom_intensity", s_bloom_slider.curvalue/10);
	Cvar_SetValue( "gl_ext_mtexcombine", s_texcombine_box.curvalue);
	Cvar_SetValue( "r_overbrightbits",
			(s_overbright_slider.curvalue == 3.0f ? 4.0f : s_overbright_slider.curvalue) );
	Cvar_SetValue( "_windowed_mouse", s_windowed_mouse.curvalue);
	Cvar_SetValue( "gl_modulate", s_modulate_slider.curvalue);
	Cvar_SetValue( "gl_normalmaps", s_normalmaps_box.curvalue);
	Cvar_SetValue( "gl_shadowmaps", s_shadowmaps_box.curvalue);
	Cvar_SetValue( "gl_parallaxmaps", s_parallaxmaps_box.curvalue);
	Cvar_SetValue( "gl_glsl_postprocess", s_postprocess_box.curvalue);
	Cvar_SetValue( "gl_glsl_shaders", s_glsl_box.curvalue);
	if(s_normalmaps_box.curvalue)
		Cvar_SetValue("r_shaders", 1); //shaders must be enabled for this to work
									  
	vid_ref->modified = true;
#ifdef __unix__
	vid_restart = true;
#endif

	M_ForceMenuOff();
}

static void CancelChanges( void *unused )
{
	extern void M_PopMenu( void );

	M_PopMenu();
}

/*
** VID_MenuInit
*/
void VID_MenuInit( void )
{
	static const char *resolutions[] =
	{
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 960 ]",
		"[1360 768 ]",
		"[1600 1200]",
		"[1680 1050]",
		"[2048 1536]",
		"[custom   ]",
		0
	};

	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};
	static const char *onoff_names[]  =
	{
		"off",
		"on",
		0
	};
	float scale;

	scale = viddef.height/600;
	if(scale < 1)
		scale = 1;

	banneralpha = 0.1;

	if ( !gl_picmip )
		gl_picmip = Cvar_Get( "gl_picmip", "0", CVAR_ARCHIVE );
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "3", CVAR_ARCHIVE );
	if ( !gl_finish )
		gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );
	if ( !gl_swapinterval )
		gl_swapinterval = Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );
	if ( !r_bloom )
		r_bloom = Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	if ( !r_bloom_intensity )
		r_bloom_intensity = Cvar_Get( "r_bloom_intensity", "0.5", CVAR_ARCHIVE);
	if ( !gl_ext_mtexcombine )
		gl_ext_mtexcombine = Cvar_Get( "gl_ext_mtexcombine", "1", CVAR_ARCHIVE);
	if ( !r_overbrightbits )
		r_overbrightbits = Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE);
	if ( !gl_modulate )
		gl_modulate = Cvar_Get( "gl_modulate", "2", CVAR_ARCHIVE);
	if ( !vid_width )
		vid_width = Cvar_Get( "vid_width", "640", CVAR_ARCHIVE);
	if ( !vid_height )
		vid_height = Cvar_Get( "vid_height", "400", CVAR_ARCHIVE);
	if (!gl_normalmaps)
		gl_normalmaps = Cvar_Get( "gl_normalmaps", "0", CVAR_ARCHIVE);
	if (!gl_shadowmaps)
		gl_shadowmaps = Cvar_Get( "gl_shadowmaps", "0", CVAR_ARCHIVE);
	if (!gl_parallaxmaps)
		gl_parallaxmaps = Cvar_Get( "gl_parallaxmaps", "0", CVAR_ARCHIVE);
	if (!gl_glsl_postprocess)
		gl_glsl_postprocess = Cvar_Get( "gl_glsl_postprocess", "1", CVAR_ARCHIVE);
	if (!gl_glsl_shaders)
		gl_glsl_shaders = Cvar_Get( "gl_glsl_shaders", "1", CVAR_ARCHIVE);

	if ( !_windowed_mouse)
		_windowed_mouse = Cvar_Get( "_windowed_mouse", "1", CVAR_ARCHIVE );

	if(gl_mode->value == -1)
		s_mode_list.curvalue = 10;
	else
		s_mode_list.curvalue = gl_mode->value; 

	if ( s_mode_list.curvalue < 0 )
		s_mode_list.curvalue = 0;

	s_bloom_slider.curvalue = r_bloom_intensity->value*10;

	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;

	s_mode_list.generic.type = MTYPE_SPINCONTROL;
	s_mode_list.generic.name = "video mode";
	s_mode_list.generic.x = 24;
	s_mode_list.generic.y = 10*scale;
	s_mode_list.itemnames = resolutions;

	s_width_field.generic.type = MTYPE_FIELD;
	s_width_field.generic.name = "custom width";
	s_width_field.generic.flags = QMF_NUMBERSONLY;
	s_width_field.generic.x	= 32;
	s_width_field.generic.y	= 30*scale;
	s_width_field.generic.statusbar = "set custom width";
	s_width_field.length = 4*scale;
	s_width_field.visible_length = 4*scale;
	strcpy(s_width_field.buffer, Cvar_VariableString("vid_width"));

	s_height_field.generic.type = MTYPE_FIELD;
	s_height_field.generic.name = "custom height";
	s_height_field.generic.flags = QMF_NUMBERSONLY;
	s_height_field.generic.x	= 32;
	s_height_field.generic.y	= 50*scale;
	s_height_field.generic.statusbar = "set custom height";
	s_height_field.length = 4*scale;
	s_height_field.visible_length = 4*scale;
	strcpy(s_height_field.buffer, Cvar_VariableString("vid_height"));

	s_brightness_slider.generic.type	= MTYPE_SLIDER;
	s_brightness_slider.generic.x	= 24;
	s_brightness_slider.generic.y	= 70*scale;
	s_brightness_slider.generic.name	= "texture brightness";
	s_brightness_slider.generic.callback = BrightnessCallback;
	s_brightness_slider.minvalue = 1;
	s_brightness_slider.maxvalue = 20;
	s_brightness_slider.curvalue = vid_gamma->value * 10;

	s_contrast_slider.generic.type	= MTYPE_SLIDER;
	s_contrast_slider.generic.x	= 24;
	s_contrast_slider.generic.y	= 80*scale;
	s_contrast_slider.generic.name	= "texture contrast";
	s_contrast_slider.generic.callback = ContrastCallback;
	s_contrast_slider.minvalue = 1;
	s_contrast_slider.maxvalue = 20;
	s_contrast_slider.curvalue = vid_contrast->value * 10;

	s_modulate_slider.generic.type = MTYPE_SLIDER;
	s_modulate_slider.generic.x	= 24;
	s_modulate_slider.generic.y	= 90*scale;
	s_modulate_slider.generic.name = "lightmap brightness";
	s_modulate_slider.minvalue = 1;
	s_modulate_slider.maxvalue = 5;
	s_modulate_slider.generic.callback = ModulateCallback;
	s_modulate_slider.curvalue = gl_modulate->value;

	s_fs_box.generic.type = MTYPE_SPINCONTROL;
	s_fs_box.generic.x	= 24;
	s_fs_box.generic.y	= 100*scale;
	s_fs_box.generic.name	= "fullscreen";
	s_fs_box.itemnames = yesno_names;
	s_fs_box.curvalue = vid_fullscreen->value;

	s_bloom_box.generic.type = MTYPE_SPINCONTROL;
	s_bloom_box.generic.x	= 24;
	s_bloom_box.generic.y	= 120*scale;
	s_bloom_box.generic.name	= "light bloom";
	s_bloom_box.itemnames = onoff_names;
	s_bloom_box.generic.callback = BloomSetCallback;
	s_bloom_box.curvalue = r_bloom->value;

	s_bloom_slider.generic.type	= MTYPE_SLIDER;
	s_bloom_slider.generic.x		= 24;
	s_bloom_slider.generic.y		= 130*scale;
	s_bloom_slider.generic.name	= "bloom intensity";
	s_bloom_slider.minvalue = 0;
	s_bloom_slider.maxvalue = 20;
	s_bloom_slider.generic.callback = BloomCallback;

	s_texcombine_box.generic.type = MTYPE_SPINCONTROL;
	s_texcombine_box.generic.x	= 24;
	s_texcombine_box.generic.y	= 150*scale;
	s_texcombine_box.generic.name	= "multitexture combine";
	s_texcombine_box.itemnames = onoff_names;
	s_texcombine_box.generic.callback = MtexCallback;
	s_texcombine_box.curvalue = gl_ext_mtexcombine->value;

	s_overbright_slider.generic.type	= MTYPE_SLIDER;
	s_overbright_slider.generic.x		= 24;
	s_overbright_slider.generic.y		= 160*scale;
	s_overbright_slider.generic.name	= "overbright bits";
	s_overbright_slider.minvalue = 1;
	s_overbright_slider.maxvalue = 3;
	s_overbright_slider.curvalue = 
		(r_overbrightbits->value == 4.0f) ? 3.0f : r_overbrightbits->value;

	s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.x		= 24;
	s_tq_slider.generic.y		= 170*scale;
	s_tq_slider.generic.name	= "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3-gl_picmip->value;

	s_normalmaps_box.generic.type	= MTYPE_SPINCONTROL;
	s_normalmaps_box.generic.x		= 24;
	s_normalmaps_box.generic.y		= 190*scale;
	s_normalmaps_box.generic.name	= "normal maps";
	s_normalmaps_box.curvalue = gl_normalmaps->value;
	s_normalmaps_box.itemnames = yesno_names;
	s_normalmaps_box.generic.callback = NormalMapsCallback;

	s_shadowmaps_box.generic.type	= MTYPE_SPINCONTROL;
	s_shadowmaps_box.generic.x		= 24;
	s_shadowmaps_box.generic.y		= 200*scale;
	s_shadowmaps_box.generic.name	= "shadow maps";
	s_shadowmaps_box.curvalue = gl_shadowmaps->value;
	s_shadowmaps_box.itemnames = yesno_names;
	s_shadowmaps_box.generic.callback = ShadowMapsCallback;
	s_shadowmaps_box.generic.statusbar = "for testing only, incomplete code";

	s_parallaxmaps_box.generic.type	= MTYPE_SPINCONTROL;
	s_parallaxmaps_box.generic.x		= 24;
	s_parallaxmaps_box.generic.y		= 210*scale;
	s_parallaxmaps_box.generic.name	= "parallax maps";
	s_parallaxmaps_box.curvalue = gl_parallaxmaps->value;
	s_parallaxmaps_box.itemnames = yesno_names;
	s_parallaxmaps_box.generic.callback = ParallaxCallback;

	s_postprocess_box.generic.type	= MTYPE_SPINCONTROL;
	s_postprocess_box.generic.x		= 24;
	s_postprocess_box.generic.y		= 220*scale;
	s_postprocess_box.generic.name	= "post process effects";
	s_postprocess_box.curvalue = gl_glsl_postprocess->value;
	s_postprocess_box.itemnames = yesno_names;
	s_postprocess_box.generic.callback = PostProcessCallback;

	s_glsl_box.generic.type	= MTYPE_SPINCONTROL;
	s_glsl_box.generic.x		= 24;
	s_glsl_box.generic.y		= 230*scale;
	s_glsl_box.generic.name	= "use GLSL shaders";
	s_glsl_box.curvalue = gl_glsl_shaders->value;
	s_glsl_box.itemnames = yesno_names;
	s_glsl_box.generic.callback = GlslCallback;

	s_finish_box.generic.type = MTYPE_SPINCONTROL;
	s_finish_box.generic.x	= 24;
	s_finish_box.generic.y	= 250*scale;
	s_finish_box.generic.name	= "draw frame completely";
	s_finish_box.curvalue = gl_finish->value;
	s_finish_box.itemnames = yesno_names;

	s_vsync_box.generic.type = MTYPE_SPINCONTROL;
    s_vsync_box.generic.x  = 24;
    s_vsync_box.generic.y  = 260*scale;
    s_vsync_box.generic.name       = "vertical sync";
    s_vsync_box.curvalue = gl_swapinterval->value;
    s_vsync_box.itemnames = onoff_names;

	s_windowed_mouse.generic.type = MTYPE_SPINCONTROL;
	s_windowed_mouse.generic.x  = 24;
	s_windowed_mouse.generic.y  = 270*scale;
	s_windowed_mouse.generic.name   = "windowed mouse";
	s_windowed_mouse.curvalue = _windowed_mouse->value;
	s_windowed_mouse.itemnames = yesno_names;

	s_lowest_action.generic.type = MTYPE_ACTION;
	s_lowest_action.generic.name = "lowest settings";
	s_lowest_action.generic.x    = 24;
	s_lowest_action.generic.y    = 290*scale;
	s_lowest_action.generic.callback = SetLowest;
	s_lowest_action.generic.statusbar = "no shaders, no dynamic lighting";

	s_low_action.generic.type = MTYPE_ACTION;
	s_low_action.generic.name = "low settings";
	s_low_action.generic.x    = 24;
	s_low_action.generic.y    = 300*scale;
	s_low_action.generic.callback = SetLow;
	s_low_action.generic.statusbar = "shaders, but no dynamic lighting";

	s_medium_action.generic.type = MTYPE_ACTION;
	s_medium_action.generic.name = "medium settings";
	s_medium_action.generic.x    = 24;
	s_medium_action.generic.y    = 310*scale;
	s_medium_action.generic.callback = SetMedium;
	s_medium_action.generic.statusbar = "GLSL per-pixel dynamic lighting and postprocess";

	s_high_action.generic.type = MTYPE_ACTION;
	s_high_action.generic.name = "high settings";
	s_high_action.generic.x    = 24;
	s_high_action.generic.y    = 320*scale;
	s_high_action.generic.callback = SetHigh;
	s_high_action.generic.statusbar = "GLSL per-pixel effects on most surfaces";

	s_highest_action.generic.type = MTYPE_ACTION;
	s_highest_action.generic.name = "highest settings";
	s_highest_action.generic.x    = 24;
	s_highest_action.generic.y    = 330*scale;
	s_highest_action.generic.callback = SetHighest;
	s_highest_action.generic.statusbar = "GLSL per-pixel effects on all surfaces";

	s_apply_action.generic.type = MTYPE_ACTION;
	s_apply_action.generic.name = "apply changes";
	s_apply_action.generic.x    = 24;
	s_apply_action.generic.y    = 345*scale;
	s_apply_action.generic.callback = ApplyChanges;

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_width_field);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_height_field);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_contrast_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_modulate_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fs_box);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_bloom_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_bloom_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_texcombine_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_overbright_slider );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_tq_slider );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_normalmaps_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_shadowmaps_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_parallaxmaps_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_postprocess_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_glsl_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_finish_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_vsync_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_windowed_mouse );

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_lowest_action);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_low_action);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_medium_action);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_high_action);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_highest_action);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_apply_action);

	Menu_Center( &s_opengl_menu );
	s_opengl_menu.x -= 8;
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	int w, h;
	float scale;

	scale = viddef.height/600;
	if(scale < 1)
		scale = 1;

	banneralpha += 0.01;
	if (banneralpha > 1)
		banneralpha = 1;

	s_current_menu = &s_opengl_menu;
	/*
	** draw the banner
	*/
	Draw_StretchPic(0, 0, viddef.width, viddef.height, "menu_back");
	Draw_GetPicSize( &w, &h, "m_video" );
	w*=scale;
	h*=scale;
	Draw_AlphaStretchPic( viddef.width / 2 - w / 2, viddef.height /2 - 275*scale, w, h, "m_video", banneralpha );

	/*
	** move cursor to a reasonable starting position
	*/
	Menu_AdjustCursor( s_current_menu, 1 );

	/*
	** draw the menu
	*/
	Menu_Draw( s_current_menu );
}

/*
================
VID_MenuKey
================
*/
const char *VID_MenuKey( int key )
{
	extern void M_PopMenu( void );

	menucommon_s *item;
	menuframework_s *m = s_current_menu;
	static const char *sound = "misc/menu1.wav";

	if ( m )
	{
		if ( ( item = Menu_ItemAtCursor( m ) ) != 0 )
		{
			if ( item->type == MTYPE_FIELD )
			{
				if ( Field_Key( ( menufield_s * ) item, key ) )
					return NULL;
			}
		}
	}

	switch ( key )
	{
	case K_ESCAPE:
		CancelChanges( 0 );
		return NULL;
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor( m, -1 );
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem( m, -1 );
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem( m, 1 );
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if ( !Menu_SelectItem( m ) )
			ApplyChanges( NULL );
		break;
	}

	return sound;
}
