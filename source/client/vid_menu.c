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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#include "qmenu.h"

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *vid_contrast;

extern cvar_t *r_bloom_intensity;
extern cvar_t *gl_normalmaps;
extern cvar_t *gl_shadowmaps;
extern cvar_t *gl_glsl_postprocess;
extern cvar_t *gl_glsl_shaders;
extern cvar_t *gl_modulate;
extern cvar_t *gl_vlights;
extern cvar_t *gl_usevbo;

extern cvar_t *vid_width;
extern cvar_t *vid_height;

static cvar_t *gl_mode;
static cvar_t *gl_picmip;
static cvar_t *gl_finish;
static cvar_t *gl_swapinterval;
static cvar_t *r_bloom;
static cvar_t *r_overbrightbits;

static cvar_t *_windowed_mouse;

extern void M_ForceMenuOff( void );

extern void RS_FreeUnmarked( void );

#if defined UNIX_VARIANT
extern qboolean vid_restart;
#endif

/*
====================================================================

MENU INTERACTION

====================================================================
*/

menuframework_s			s_opengl_screen;

static menuframework_s	s_opengl_menu;
static menuframework_s *s_current_menu;

static menulist_s		s_mode_list;
static menuslider_s		s_tq_slider;
static menuslider_s		s_brightness_slider;
static menuslider_s		s_contrast_slider;
static menulist_s  		s_fs_box;
static menulist_s  		s_finish_box;
static menulist_s		s_vsync_box;
static menulist_s  		s_windowed_mouse;
static menulist_s		s_bloom_box;
static menuslider_s		s_bloom_slider;
static menuslider_s		s_overbright_slider;
static menuslider_s		s_modulate_slider;
static menufield_s		s_height_field;
static menufield_s		s_width_field;
static menulist_s		s_postprocess_box;
static menulist_s		s_glsl_box;
static menulist_s		s_vbo_box;

static menutxt_s		s_separator_presets;

static menuaction_s		s_lowest_action;
static menuaction_s		s_low_action;
static menuaction_s		s_medium_action;
static menuaction_s		s_high_action;
static menuaction_s		s_highest_action;

static menutxt_s		s_separator_apply;

static menuaction_s		s_apply_action;

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

static void ModulateCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "gl_modulate", slider->curvalue);
}

static void PostProcessCallback( void *s )
{
	if(s_postprocess_box.curvalue) 
	{ 
		Cvar_SetValue("gl_glsl_postprocess", s_postprocess_box.curvalue);
		s_glsl_box.curvalue = s_postprocess_box.curvalue;
	}
}

static void GlslCallback( void *s )
{
	Cvar_SetValue("gl_glsl_shaders", s_glsl_box.curvalue);
	if(!s_glsl_box.curvalue) 
	{
		Cvar_SetValue("gl_glsl_postprocess", s_glsl_box.curvalue);
		s_postprocess_box.curvalue = s_glsl_box.curvalue;
	}
}

static void VboCallback( void *s )
{
	Cvar_SetValue("gl_usevbo", s_vbo_box.curvalue);
}

static void SetCompatibility( void *unused )
{
	Cmd_ExecuteString ("exec graphical_presets/compatibility.cfg");
	Cbuf_Execute ();
	VID_MenuInit();
}

static void SetMaxPerformance( void *unused )
{
	Cmd_ExecuteString ("exec graphical_presets/maxperformance.cfg");
	Cbuf_Execute ();
	VID_MenuInit();
}

static void SetPerformance( void *unused )
{
	Cmd_ExecuteString ("exec graphical_presets/performance.cfg");
	Cbuf_Execute ();
	VID_MenuInit();
}

static void SetQuality( void *unused )
{
	Cmd_ExecuteString ("exec graphical_presets/quality.cfg");
	VID_MenuInit();
}

static void SetMaxQuality( void *unused )
{
	Cmd_ExecuteString ("exec graphical_presets/maxquality.cfg");
	Cbuf_Execute ();
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

	if (s_mode_list.curvalue == 13) {
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
	Cvar_SetValue( "r_overbrightbits",
			(s_overbright_slider.curvalue == 3.0f ? 4.0f : s_overbright_slider.curvalue) );
	Cvar_SetValue( "_windowed_mouse", s_windowed_mouse.curvalue);
	Cvar_SetValue( "gl_modulate", s_modulate_slider.curvalue);
	Cvar_SetValue( "gl_glsl_postprocess", s_postprocess_box.curvalue);
	Cvar_SetValue( "gl_glsl_shaders", s_glsl_box.curvalue);
	Cvar_SetValue( "gl_usevbo", s_vbo_box.curvalue);

	RS_FreeUnmarked();
	Cvar_SetValue("scriptsloaded", 0); //scripts get flushed

	vid_ref->modified = true;
#if defined UNIX_VARIANT
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
		"[1280 1024]",
		"[1360 768 ]",
		"[1366 768 ]",
		"[1600 1200]",
		"[1680 1050]",
		"[1920 1080]",
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
	if ( !r_overbrightbits )
		r_overbrightbits = Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE);
	if ( !gl_modulate )
		gl_modulate = Cvar_Get( "gl_modulate", "2", CVAR_ARCHIVE);
	if ( !vid_width )
		vid_width = Cvar_Get( "vid_width", "640", CVAR_ARCHIVE);
	if ( !vid_height )
		vid_height = Cvar_Get( "vid_height", "400", CVAR_ARCHIVE);
	if (!gl_glsl_postprocess)
		gl_glsl_postprocess = Cvar_Get( "gl_glsl_postprocess", "1", CVAR_ARCHIVE);
	if (!gl_glsl_shaders)
		gl_glsl_shaders = Cvar_Get( "gl_glsl_shaders", "1", CVAR_ARCHIVE);
	if (!gl_usevbo)
		gl_usevbo = Cvar_Get( "gl_usevbo", "1", CVAR_ARCHIVE);

	if ( !_windowed_mouse)
	{
		_windowed_mouse = Cvar_Get( "_windowed_mouse", "1", CVAR_ARCHIVE );
	
		// FIXME: MASSIVE HACK to get around the fact that VID_MenuInit is called
		// really freaking early in the game's initialization for some reason--
		// before the OpenGL functions even get loaded! The only reason it's
		// called so early is to initialize all those cvars, which is how
		// we detect when not to proceed.
		return;
	}

	if(gl_mode->value == -1)
		s_mode_list.curvalue = 13;
	else
		s_mode_list.curvalue = gl_mode->value;

	if ( s_mode_list.curvalue < 0 )
		s_mode_list.curvalue = 0;

	s_bloom_slider.curvalue = r_bloom_intensity->value*10;

	s_opengl_screen.nitems = 0;
	
	s_opengl_menu.generic.type = MTYPE_SUBMENU;
	s_opengl_menu.navagable = true;
	s_opengl_menu.nitems = 0;
	s_opengl_menu.bordertitle = "VIDEO SETTINGS";
	s_opengl_menu.bordertexture = "menu/m_";

	s_mode_list.generic.type = MTYPE_SPINCONTROL;
	s_mode_list.generic.name = "video mode";
	s_mode_list.itemnames = resolutions;

	s_width_field.generic.type = MTYPE_FIELD;
	s_width_field.generic.name = "custom width";
	s_width_field.generic.flags = QMF_NUMBERSONLY;
	s_width_field.generic.statusbar = "set custom width";
	s_width_field.generic.visible_length = 4;
	strcpy(s_width_field.buffer, Cvar_VariableString("vid_width"));

	s_height_field.generic.type = MTYPE_FIELD;
	s_height_field.generic.name = "custom height";
	s_height_field.generic.flags = QMF_NUMBERSONLY;
	s_height_field.generic.statusbar = "set custom height";
	s_height_field.generic.visible_length = 4;
	strcpy(s_height_field.buffer, Cvar_VariableString("vid_height"));

	s_brightness_slider.generic.type	= MTYPE_SLIDER;
	s_brightness_slider.generic.name	= "texture brightness";
	s_brightness_slider.generic.callback = BrightnessCallback;
	s_brightness_slider.minvalue = 1;
	s_brightness_slider.maxvalue = 20;
	s_brightness_slider.curvalue = vid_gamma->value * 10;

	s_contrast_slider.generic.type	= MTYPE_SLIDER;
	s_contrast_slider.generic.name	= "texture contrast";
	s_contrast_slider.generic.callback = ContrastCallback;
	s_contrast_slider.minvalue = 1;
	s_contrast_slider.maxvalue = 20;
	s_contrast_slider.curvalue = vid_contrast->value * 10;

	s_modulate_slider.generic.type = MTYPE_SLIDER;
	s_modulate_slider.generic.name = "lightmap brightness";
	s_modulate_slider.minvalue = 1;
	s_modulate_slider.maxvalue = 5;
	s_modulate_slider.generic.callback = ModulateCallback;
	s_modulate_slider.curvalue = gl_modulate->value;

	s_fs_box.generic.type = MTYPE_SPINCONTROL;
	s_fs_box.generic.name	= "fullscreen";
	s_fs_box.itemnames = yesno_names;
	s_fs_box.curvalue = vid_fullscreen->value;

	s_bloom_box.generic.type = MTYPE_SPINCONTROL;
	s_bloom_box.generic.name	= "light bloom";
	s_bloom_box.itemnames = onoff_names;
	s_bloom_box.generic.callback = BloomSetCallback;
	s_bloom_box.curvalue = r_bloom->value;

	s_bloom_slider.generic.type	= MTYPE_SLIDER;
	s_bloom_slider.generic.name	= "bloom intensity";
	s_bloom_slider.minvalue = 0;
	s_bloom_slider.maxvalue = 20;
	s_bloom_slider.generic.callback = BloomCallback;

	s_overbright_slider.generic.type	= MTYPE_SLIDER;
	s_overbright_slider.generic.name	= "overbright bits";
	s_overbright_slider.minvalue = 1;
	s_overbright_slider.maxvalue = 3;
	s_overbright_slider.curvalue =
		(r_overbrightbits->value == 4.0f) ? 3.0f : r_overbrightbits->value;

	s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.name	= "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3-gl_picmip->value;

	s_postprocess_box.generic.type	= MTYPE_SPINCONTROL;
	s_postprocess_box.generic.name	= "post process effects";
	s_postprocess_box.curvalue = gl_glsl_postprocess->value;
	s_postprocess_box.itemnames = yesno_names;
	s_postprocess_box.generic.callback = PostProcessCallback;

	s_glsl_box.generic.type	= MTYPE_SPINCONTROL;
	s_glsl_box.generic.name	= "GLSL shaders";
	s_glsl_box.curvalue = gl_glsl_shaders->value;
	s_glsl_box.itemnames = yesno_names;
	s_glsl_box.generic.callback = GlslCallback;

	s_vbo_box.generic.type	= MTYPE_SPINCONTROL;
	s_vbo_box.generic.name	= "vertex buffer objects";
	s_vbo_box.curvalue = gl_usevbo->value;
	s_vbo_box.itemnames = yesno_names;
	s_vbo_box.generic.statusbar = "increase performance - may be unstable";
	s_vbo_box.generic.callback = VboCallback;

	s_finish_box.generic.type = MTYPE_SPINCONTROL;
	s_finish_box.generic.name	= "draw frame completely";
	s_finish_box.curvalue = gl_finish->value;
	s_finish_box.itemnames = yesno_names;

	s_vsync_box.generic.type = MTYPE_SPINCONTROL;
	s_vsync_box.generic.name	   = "vertical sync";
	s_vsync_box.curvalue = gl_swapinterval->value;
	s_vsync_box.itemnames = onoff_names;

	s_windowed_mouse.generic.type = MTYPE_SPINCONTROL;
	s_windowed_mouse.generic.name   = "windowed mouse";
	s_windowed_mouse.curvalue = _windowed_mouse->value;
	s_windowed_mouse.itemnames = yesno_names;

	s_lowest_action.generic.type = MTYPE_ACTION;
	s_lowest_action.generic.name = "high compatibility";
	s_lowest_action.generic.callback = SetCompatibility;
	s_lowest_action.generic.statusbar = "use when all other modes fail or run slowly";

	s_low_action.generic.type = MTYPE_ACTION;
	s_low_action.generic.name = "high performance";
	s_low_action.generic.callback = SetMaxPerformance;
	s_low_action.generic.statusbar = "fast rendering, many effects disabled";

	s_medium_action.generic.type = MTYPE_ACTION;
	s_medium_action.generic.name = "performance";
	s_medium_action.generic.callback = SetPerformance;
	s_medium_action.generic.statusbar = "GLSL per-pixel lighting and postprocess";

	s_high_action.generic.type = MTYPE_ACTION;
	s_high_action.generic.name = "quality";
	s_high_action.generic.callback = SetQuality;
	s_high_action.generic.statusbar = "GLSL per-pixel effects on all surfaces";

	s_highest_action.generic.type = MTYPE_ACTION;
	s_highest_action.generic.name = "high quality";
	s_highest_action.generic.callback = SetMaxQuality;
	s_highest_action.generic.statusbar = "GLSL, shadows, light shafts from sun";

	s_apply_action.generic.type = MTYPE_ACTION;
	s_apply_action.generic.name = "apply changes";
	s_apply_action.generic.callback = ApplyChanges;
	
	s_separator_presets.generic.type = s_separator_apply.generic.type = MTYPE_TEXT;

	Menu_AddItem( &s_opengl_menu, &s_mode_list);
	Menu_AddItem( &s_opengl_menu, &s_width_field);
	Menu_AddItem( &s_opengl_menu, &s_height_field);
	Menu_AddItem( &s_opengl_menu, &s_brightness_slider);
	Menu_AddItem( &s_opengl_menu, &s_contrast_slider);
	Menu_AddItem( &s_opengl_menu, &s_modulate_slider);
	Menu_AddItem( &s_opengl_menu, &s_fs_box);
	Menu_AddItem( &s_opengl_menu, &s_bloom_box );
	Menu_AddItem( &s_opengl_menu, &s_bloom_slider);
	Menu_AddItem( &s_opengl_menu, &s_overbright_slider );
	Menu_AddItem( &s_opengl_menu, &s_tq_slider );
	Menu_AddItem( &s_opengl_menu, &s_postprocess_box );
	Menu_AddItem( &s_opengl_menu, &s_glsl_box );
	Menu_AddItem( &s_opengl_menu, &s_vbo_box );
	Menu_AddItem( &s_opengl_menu, &s_finish_box );
	Menu_AddItem( &s_opengl_menu, &s_vsync_box );
	Menu_AddItem( &s_opengl_menu, &s_windowed_mouse );

	Menu_AddItem( &s_opengl_menu, &s_separator_presets);
	
	Menu_AddItem( &s_opengl_menu, &s_lowest_action);
	Menu_AddItem( &s_opengl_menu, &s_low_action);
	Menu_AddItem( &s_opengl_menu, &s_medium_action);
	Menu_AddItem( &s_opengl_menu, &s_high_action);
	Menu_AddItem( &s_opengl_menu, &s_highest_action);
	
	Menu_AddItem( &s_opengl_menu, &s_separator_apply);
	
	Menu_AddItem( &s_opengl_menu, &s_apply_action);
	
	Menu_AddItem (&s_opengl_screen, &s_opengl_menu);
	Menu_AutoArrange (&s_opengl_screen);
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	s_current_menu = &s_opengl_menu;
	
	Draw_Fill(global_menu_xoffset, 0, viddef.width, viddef.height, 0);
	
	Menu_AdjustCursor( s_current_menu, 1 );
	Menu_Draw( &s_opengl_screen );
}

/*
================
VID_MenuKey
================
*/
const char *VID_MenuKey (menuframework_s *screen, int key)
{
	extern void M_PopMenu( void );

	menucommon_s *item;
	menuframework_s *m = &s_opengl_menu;
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
		return NULL;
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		return NULL;
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
		Menu_SelectItem( m );
		break;
	}

	return sound;
}
