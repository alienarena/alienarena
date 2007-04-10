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
#include "../client/client.h"
#include "../client/qmenu.h"

#define REF_OPENGL	0
#define REF_3DFX	1
#define REF_POWERVR	2
#define REF_VERITE	3

extern cvar_t *vid_ref;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_gamma;
extern cvar_t *scr_viewsize;
extern cvar_t *gl_modulate;

extern cvar_t *r_bloom_intensity;
extern cvar_t *gl_detailtextures;
extern cvar_t *gl_normalmaps;

extern cvar_t *vid_width;
extern cvar_t *vid_height;

static cvar_t *gl_mode;
static cvar_t *gl_driver;
static cvar_t *gl_picmip;
static cvar_t *gl_finish;
static cvar_t *gl_swapinterval;
static cvar_t *gl_vlights;
static cvar_t *gl_texres;
static cvar_t *r_bloom;
static cvar_t *gl_reflection;
static cvar_t *r_overbrightbits;
static cvar_t *gl_ext_mtexcombine;

static cvar_t *sw_mode;
static cvar_t *sw_stipplealpha;

extern void M_ForceMenuOff( void );

/*
====================================================================

MENU INTERACTION

====================================================================
*/

static menuframework_s  s_software_menu;
static menuframework_s	s_opengl_menu;
static menuframework_s *s_current_menu;
static int				s_current_menu_index;

static menulist_s		s_mode_list;
static menulist_s		s_ref_list;
static menuslider_s		s_tq_slider;
static menuslider_s		s_screensize_slider;
static menuslider_s		s_brightness_slider;
static menulist_s  		s_fs_box;
static menulist_s  		s_finish_box;
static menulist_s		s_vsync_box;
static menuaction_s		s_apply_action;
static menuaction_s		s_defaults_action;
static menulist_s		s_texres_box;
static menulist_s		s_bloom_box;
static menuslider_s		s_bloom_slider;
static menulist_s		s_reflect_box;
static menulist_s		s_texcombine_box;
static menuslider_s		s_overbright_slider;
static menuslider_s		s_detailtex_slider;
static menuslider_s		s_modulate_slider;
static menufield_s		s_height_field;
static menufield_s		s_width_field;
static menulist_s		s_normalmaps_box;

static void DriverCallback( void *unused )
{

	s_current_menu = &s_opengl_menu;

}

static void ScreenSizeCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "viewsize", slider->curvalue * 10 );
}

static void BloomCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "r_bloom_intensity", slider->curvalue/10);
}

static void DetailtexCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "gl_detailtextures", slider->curvalue);
}

static void ModulateCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	Cvar_SetValue( "gl_modulate", slider->curvalue);
}

static void BrightnessCallback( void *s )
{
	menuslider_s *slider = ( menuslider_s * ) s;

	float gamma = ( 0.8 - ( slider->curvalue/10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
}

static void ResetDefaults( void *unused )
{
	//umm why wasn't this here before??
	Cvar_SetValue("gl_texres", 1);
	Cvar_SetValue("gl_reflection", 0);
	Cvar_SetValue("r_bloom", 0);
	Cvar_SetValue("r_bloom_intensity", 0.5);
	Cvar_SetValue("gl_ext_mtexcombine", 1);
	Cvar_SetValue("r_overbrightbits", 2);
	Cvar_SetValue("gl_detailtextures", 0);
	Cvar_SetValue("gl_modulate", 2);
	Cvar_SetValue("gl_picmip", 0);
	Cvar_SetValue("vid_gamma", 1);
	Cvar_SetValue("gl_normalmaps", 0);

	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
	float gamma;

	gamma = ( 0.8 - ( s_brightness_slider.curvalue/10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "vid_gamma", gamma );
	Cvar_SetValue( "gl_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValue( "vid_fullscreen", s_fs_box.curvalue );
	Cvar_SetValue( "gl_finish", s_finish_box.curvalue );
	Cvar_SetValue( "gl_swapinterval", s_vsync_box.curvalue);
	if(s_mode_list.curvalue == 0) {
		Cvar_SetValue( "gl_mode", -1);
		//set custom width and height only in this case
		//check for sane values
		if(atoi(s_width_field.buffer) > 2048)
			strcpy(s_width_field.buffer, "2048");
		if(atoi(s_height_field.buffer) > 1536)
			strcpy(s_height_field.buffer, "1536");
		if(atoi(s_width_field.buffer) < 512)
			strcpy(s_width_field.buffer, "512");
		if(atoi(s_height_field.buffer) < 384)
			strcpy(s_height_field.buffer, "384");

		Cvar_SetValue("vid_width", atoi( s_width_field.buffer ));
		Cvar_SetValue("vid_height", atoi( s_height_field.buffer ));
	}
	else
		Cvar_SetValue( "gl_mode", s_mode_list.curvalue + 2 ); //offset added back 
	Cvar_SetValue( "gl_texres", s_texres_box.curvalue );
	Cvar_SetValue( "gl_reflection", s_reflect_box.curvalue);
	Cvar_SetValue( "r_bloom", s_bloom_box.curvalue);
	Cvar_SetValue( "r_bloom_intensity", s_bloom_slider.curvalue/10);
	Cvar_SetValue( "gl_ext_mtexcombine", s_texcombine_box.curvalue);
	Cvar_SetValue( "r_overbrightbits", s_overbright_slider.curvalue);
	Cvar_SetValue( "gl_detailtextures", s_detailtex_slider.curvalue);
	Cvar_SetValue( "gl_modulate", s_modulate_slider.curvalue);
	Cvar_SetValue("gl_normalmaps", s_normalmaps_box.curvalue);
	
	switch ( s_ref_list.curvalue )
	{
	
	case REF_OPENGL:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "opengl32" );
		break;
	case REF_3DFX:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "3dfxgl" );
		break;
	case REF_POWERVR:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "pvrgl" );
		break;
	case REF_VERITE:
		Cvar_Set( "vid_ref", "gl" );
		Cvar_Set( "gl_driver", "veritegl" );
		break;
	}

	/*
	** update appropriate stuff if we're running OpenGL and gamma
	** has been modified
	*/
	if ( stricmp( vid_ref->string, "gl" ) == 0 )
	{
		if ( vid_gamma->modified )
		{
			vid_ref->modified = true;
			if ( stricmp( gl_driver->string, "3dfxgl" ) == 0 )
			{
				char envbuffer[1024];
				float g;

				vid_ref->modified = true;

				g = 2.00 * ( 0.8 - ( vid_gamma->value - 0.5 ) ) + 1.0F;
				Com_sprintf( envbuffer, sizeof(envbuffer), "SSTV2_GAMMA=%f", g );
				putenv( envbuffer );
				Com_sprintf( envbuffer, sizeof(envbuffer), "SST_GAMMA=%f", g );
				putenv( envbuffer );

				vid_gamma->modified = false;
			}
		}

		if ( gl_driver->modified )
			vid_ref->modified = true;
	}
	
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
		"[custom   ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 960 ]",
		"[1600 1200]",
		"[2048 1536]",
		0
	};
	static const char *refs[] =
	{
		"[default OpenGL]",
		"[3Dfx OpenGL   ]",
		"[PowerVR OpenGL]",
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

	if ( !gl_driver )
		gl_driver = Cvar_Get( "gl_driver", "opengl32", 0 );
	if ( !gl_picmip )
		gl_picmip = Cvar_Get( "gl_picmip", "0", CVAR_ARCHIVE);
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "3", 0 );
	if ( !sw_mode )
		sw_mode = Cvar_Get( "sw_mode", "0", 0 );
	if ( !gl_finish )
		gl_finish = Cvar_Get( "gl_finish", "0", CVAR_ARCHIVE );
	if ( !gl_swapinterval )
                gl_swapinterval = Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );
	if ( !gl_texres )
		gl_texres = Cvar_Get( "gl_texres", "1", CVAR_ARCHIVE );
	if ( !gl_reflection)
		gl_reflection = Cvar_Get( "gl_reflection", "0", CVAR_ARCHIVE );
	if ( !r_bloom )
		r_bloom = Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	if ( !r_bloom_intensity )
		r_bloom_intensity = Cvar_Get( "r_bloom_intensity", "0.5", CVAR_ARCHIVE);
	if ( !gl_ext_mtexcombine )
		gl_ext_mtexcombine = Cvar_Get( "gl_ext_mtexcombine", "1", CVAR_ARCHIVE);
	if ( !r_overbrightbits )
		r_overbrightbits = Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE);
	if ( !gl_detailtextures )
		gl_detailtextures = Cvar_Get( "gl_detailtextures", "0", CVAR_ARCHIVE);
	if ( !gl_modulate )
		gl_modulate = Cvar_Get( "gl_modulate", "2", CVAR_ARCHIVE);
	if ( !vid_width )
		vid_width = Cvar_Get( "vid_width", "640", CVAR_ARCHIVE);
	if ( !vid_height )
		vid_height = Cvar_Get( "vid_height", "400", CVAR_ARCHIVE);
	if (!gl_normalmaps)
		gl_normalmaps = Cvar_Get( "gl_normalmaps", "0", CVAR_ARCHIVE);

	s_mode_list.curvalue = gl_mode->value - 2; //done because of the removed resolutions hack

	if ( s_mode_list.curvalue < 0 )
		s_mode_list.curvalue = 0;

	if ( !scr_viewsize )
		scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);

	s_screensize_slider.curvalue = scr_viewsize->value/10;
	s_bloom_slider.curvalue = r_bloom_intensity->value*10;
	s_detailtex_slider.curvalue = gl_detailtextures->value;
	
	if ( strcmp( gl_driver->string, "3dfxgl" ) == 0 )
		s_ref_list.curvalue = REF_3DFX;
	else if ( strcmp( gl_driver->string, "pvrgl" ) == 0 )
		s_ref_list.curvalue = REF_POWERVR;
	else if ( strcmp( gl_driver->string, "opengl32" ) == 0 )
		s_ref_list.curvalue = REF_OPENGL;
	else
		s_ref_list.curvalue = REF_OPENGL;
	
	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;

	s_ref_list.generic.type = MTYPE_SPINCONTROL;
	s_ref_list.generic.name = "driver";
	s_ref_list.generic.x = 0;
	s_ref_list.generic.y = 0;
	s_ref_list.generic.callback = DriverCallback;
	s_ref_list.itemnames = refs;

	s_mode_list.generic.type = MTYPE_SPINCONTROL;
	s_mode_list.generic.name = "video mode";
	s_mode_list.generic.x = 0;
	s_mode_list.generic.y = 10;
	s_mode_list.itemnames = resolutions;

	s_width_field.generic.type = MTYPE_FIELD;
	s_width_field.generic.name = "custom width";
	s_width_field.generic.flags = QMF_NUMBERSONLY;
	s_width_field.generic.x	= 8;
	s_width_field.generic.y	= 30;
	s_width_field.generic.statusbar = "set custom width";
	s_width_field.length = 4;
	s_width_field.visible_length = 4;
	strcpy(s_width_field.buffer, Cvar_VariableString("vid_width"));

	s_height_field.generic.type = MTYPE_FIELD;
	s_height_field.generic.name = "custom height";
	s_height_field.generic.flags = QMF_NUMBERSONLY;
	s_height_field.generic.x	= 8;
	s_height_field.generic.y	= 50;
	s_height_field.generic.statusbar = "set custom height";
	s_height_field.length = 4;
	s_height_field.visible_length = 4;
	strcpy(s_height_field.buffer, Cvar_VariableString("vid_height"));

	s_screensize_slider.generic.type	= MTYPE_SLIDER;
	s_screensize_slider.generic.x		= 0;
	s_screensize_slider.generic.y		= 60;
	s_screensize_slider.generic.name	= "screen size";
	s_screensize_slider.minvalue = 3;
	s_screensize_slider.maxvalue = 12;
	s_screensize_slider.generic.callback = ScreenSizeCallback;

	s_brightness_slider.generic.type	= MTYPE_SLIDER;
	s_brightness_slider.generic.x	= 0;
	s_brightness_slider.generic.y	= 70;
	s_brightness_slider.generic.name	= "texture brightness";
	s_brightness_slider.generic.callback = BrightnessCallback;
	s_brightness_slider.minvalue = 5;
	s_brightness_slider.maxvalue = 13;
	s_brightness_slider.curvalue = ( 1.3 - vid_gamma->value + 0.5 ) * 10;

	s_modulate_slider.generic.type = MTYPE_SLIDER;
	s_modulate_slider.generic.x	= 0;
	s_modulate_slider.generic.y	= 80;
	s_modulate_slider.generic.name = "lightmap brightness";
	s_modulate_slider.minvalue = 1;
	s_modulate_slider.maxvalue = 5;
	s_modulate_slider.generic.callback = ModulateCallback;
	s_modulate_slider.curvalue = gl_modulate->value;

	s_fs_box.generic.type = MTYPE_SPINCONTROL;
	s_fs_box.generic.x	= 0;
	s_fs_box.generic.y	= 90;
	s_fs_box.generic.name	= "fullscreen";
	s_fs_box.itemnames = yesno_names;
	s_fs_box.curvalue = vid_fullscreen->value;

	s_texres_box.generic.type = MTYPE_SPINCONTROL;
	s_texres_box.generic.x	= 0;
	s_texres_box.generic.y	= 110;
	s_texres_box.generic.name	= "hires textures";
	s_texres_box.itemnames = onoff_names;
	s_texres_box.curvalue = gl_texres->value;

	s_detailtex_slider.generic.type = MTYPE_SLIDER;
	s_detailtex_slider.generic.x	= 0;
	s_detailtex_slider.generic.y	= 120;
	s_detailtex_slider.generic.name = "detail texture level";
	s_detailtex_slider.minvalue = 0;
	s_detailtex_slider.maxvalue = 9;
	s_detailtex_slider.generic.callback = DetailtexCallback;

	s_bloom_box.generic.type = MTYPE_SPINCONTROL;
	s_bloom_box.generic.x	= 0;
	s_bloom_box.generic.y	= 130;
	s_bloom_box.generic.name	= "light bloom";
	s_bloom_box.itemnames = onoff_names;
	s_bloom_box.curvalue = r_bloom->value;

	s_bloom_slider.generic.type	= MTYPE_SLIDER;
	s_bloom_slider.generic.x		= 0;
	s_bloom_slider.generic.y		= 140;
	s_bloom_slider.generic.name	= "bloom intensity";
	s_bloom_slider.minvalue = 0;
	s_bloom_slider.maxvalue = 20;
	s_bloom_slider.generic.callback = BloomCallback;
	
	s_reflect_box.generic.type = MTYPE_SPINCONTROL;
	s_reflect_box.generic.x	= 0;
	s_reflect_box.generic.y	= 150;
	s_reflect_box.generic.name	= "reflective water";
	s_reflect_box.itemnames = onoff_names;
	s_reflect_box.curvalue = gl_reflection->value;

	s_texcombine_box.generic.type = MTYPE_SPINCONTROL;
	s_texcombine_box.generic.x	= 0;
	s_texcombine_box.generic.y	= 160;
	s_texcombine_box.generic.name	= "multitexture combine";
	s_texcombine_box.itemnames = onoff_names;
	s_texcombine_box.curvalue = gl_ext_mtexcombine->value;

	s_overbright_slider.generic.type	= MTYPE_SLIDER;
	s_overbright_slider.generic.x		= 0;
	s_overbright_slider.generic.y		= 170;
	s_overbright_slider.generic.name	= "overbright bits";
	s_overbright_slider.minvalue = 2;
	s_overbright_slider.maxvalue = 5;
	s_overbright_slider.curvalue = r_overbrightbits->value;

	s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.x		= 0;
	s_tq_slider.generic.y		= 180;
	s_tq_slider.generic.name	= "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3-gl_picmip->value;

	s_normalmaps_box.generic.type	= MTYPE_SPINCONTROL;
	s_normalmaps_box.generic.x		= 0;
	s_normalmaps_box.generic.y		= 190;
	s_normalmaps_box.generic.name	= "bumpmapping";
	s_normalmaps_box.curvalue = gl_normalmaps->value;
	s_normalmaps_box.itemnames = yesno_names;

	s_finish_box.generic.type = MTYPE_SPINCONTROL;
	s_finish_box.generic.x	= 0;
	s_finish_box.generic.y	= 200;
	s_finish_box.generic.name	= "draw frame completely";
	s_finish_box.curvalue = gl_finish->value;
	s_finish_box.itemnames = yesno_names;

    s_vsync_box.generic.type = MTYPE_SPINCONTROL;
    s_vsync_box.generic.x  = 0;
    s_vsync_box.generic.y  = 210;
    s_vsync_box.generic.name       = "vertical sync";
    s_vsync_box.curvalue = gl_swapinterval->value;
    s_vsync_box.itemnames = onoff_names;

	s_defaults_action.generic.type = MTYPE_ACTION;
	s_defaults_action.generic.name = "reset to defaults";
	s_defaults_action.generic.x    = 0;
	s_defaults_action.generic.y    = 230;
	s_defaults_action.generic.callback = ResetDefaults;

	s_apply_action.generic.type = MTYPE_ACTION;
	s_apply_action.generic.name = "apply changes";
	s_apply_action.generic.x    = 0;
	s_apply_action.generic.y    = 240;
	s_apply_action.generic.callback = ApplyChanges;

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_ref_list);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_width_field);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_height_field);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_screensize_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_modulate_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_fs_box);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_texres_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_detailtex_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_bloom_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_bloom_slider);
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_reflect_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_texcombine_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_overbright_slider );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_tq_slider );	
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_normalmaps_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_finish_box );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_vsync_box );

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_defaults_action);
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

	s_current_menu = &s_opengl_menu;

	/*
	** draw the banner
	*/
	Draw_StretchPic(0, 0, viddef.width, viddef.height, "conback");
	Draw_GetPicSize( &w, &h, "m_banner_main" );
	Draw_Pic( viddef.width / 2 - w / 2, viddef.height /2 - 250, "m_banner_main" );

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


