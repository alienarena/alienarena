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

#include <string.h>
#include <ctype.h>

#include "client.h"
#include "qmenu.h"

static void	 Action_DoEnter( menuaction_s *a );
static void	 Action_Draw( menuaction_s *a );
static void  Menu_DrawStatusBar( const char *string );
static void  Menu_DrawToolTip( const char *string );
static void	 MenuList_Draw( menulist_s *l );
static void	 Separator_Draw( menuseparator_s *s );
static void	 Separator2_Draw( menuseparator_s *s );
static void  ColorTxt_Draw( menutxt_s *s );
static void	 Slider_DoSlide( menuslider_s *s, int dir );
static void	 Slider_Draw( menuslider_s *s );
static void  VertSlider_Draw( menuslider_s *s );
static void	 SpinControl_Draw( menulist_s *s );
static void	 SpinControl_DoSlide( menulist_s *s, int dir );

int color_offset;

//#define RCOLUMN_OFFSET  16 //for menu mouse
//#define LCOLUMN_OFFSET -16

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

void Action_DoEnter( menuaction_s *a )
{
	if ( a->generic.callback )
		a->generic.callback( a );
}

void Action_Draw( menuaction_s *a )
{
	if ( a->generic.flags & QMF_LEFT_JUSTIFY )
	{
		if ( !(a->generic.flags & QMF_GRAYED) )
			Menu_DrawStringDark( a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name );
		else
			Menu_DrawString( a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name );
	}
	else
	{
		if ( !(a->generic.flags & QMF_GRAYED) )
			Menu_DrawStringR2LDark( a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name );
		else
			Menu_DrawStringR2L( a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name );
	}
	if ( a->generic.ownerdraw )
		a->generic.ownerdraw( a );
}
void ColorAction_Draw( menuaction_s *a )
{
	if ( a->generic.flags & QMF_GRAYED )
		Menu_DrawStringDark( a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name );
	else
		Menu_DrawColorString( a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name );

	if ( a->generic.ownerdraw )
		a->generic.ownerdraw( a );
}
qboolean Field_DoEnter( menufield_s *f )
{
	if ( f->generic.callback )
	{
		f->generic.callback( f );
		return true;
	}
	return false;
}

void Field_Draw( menufield_s *f )
{
	int i;
	char tempbuffer[128]="";
	int charscale;

	charscale = (float)(viddef.height)*16/600;

	color_offset = 0;

	if ( f->generic.name )
		Menu_DrawStringR2LDark( f->generic.x + f->generic.parent->x + LCOLUMN_OFFSET, f->generic.y + f->generic.parent->y, f->generic.name );

	strncpy( tempbuffer, f->buffer + f->visible_offset, f->visible_length);

	Draw_ScaledChar( f->generic.x + f->generic.parent->x + 8, f->generic.y + f->generic.parent->y - 4, 18, charscale, true );
	Draw_ScaledChar( f->generic.x + f->generic.parent->x + 8, f->generic.y + f->generic.parent->y + 4, 24, charscale, true );

	Draw_ScaledChar( f->generic.x + f->generic.parent->x + 16 + f->visible_length * charscale, f->generic.y + f->generic.parent->y - 4, 20, charscale, true );
	Draw_ScaledChar( f->generic.x + f->generic.parent->x + 16 + f->visible_length * charscale, f->generic.y + f->generic.parent->y + 4, 26, charscale, true );

	for ( i = 0; i < f->visible_length; i++ )
	{
		Draw_ScaledChar( f->generic.x + f->generic.parent->x + 16 + i * charscale, f->generic.y + f->generic.parent->y - 4, 19, charscale, true );
		Draw_ScaledChar( f->generic.x + f->generic.parent->x + 16 + i * charscale, f->generic.y + f->generic.parent->y + 4, 25, charscale, true);
	}

	Menu_DrawColorString( f->generic.x + f->generic.parent->x + 1*charscale, f->generic.y + f->generic.parent->y, tempbuffer );

	if ( Menu_ItemAtCursor( f->generic.parent ) == f )
	{
		int offset;

		if ( f->visible_offset )
			offset = f->visible_length;
		else
			offset = f->cursor - color_offset;

		if ( ( ( int ) ( Sys_Milliseconds() / 250 ) ) & 1 )
		{
			Draw_ScaledChar( f->generic.x + f->generic.parent->x + offset * charscale + charscale,
					   f->generic.y + f->generic.parent->y,
					   11, charscale, true );
		}
		else
		{
			Draw_ScaledChar( f->generic.x + f->generic.parent->x + offset * charscale + charscale,
					   f->generic.y + f->generic.parent->y,
					   ' ', charscale, true );
		}
	}
}

qboolean Field_Key( menufield_s *f, int key )
{
	extern int keydown[];

	switch ( key )
	{
	case K_KP_SLASH:
		key = '/';
		break;
	case K_KP_MINUS:
		key = '-';
		break;
	case K_KP_PLUS:
		key = '+';
		break;
	case K_KP_HOME:
		key = '7';
		break;
	case K_KP_UPARROW:
		key = '8';
		break;
	case K_KP_PGUP:
		key = '9';
		break;
	case K_KP_LEFTARROW:
		key = '4';
		break;
	case K_KP_5:
		key = '5';
		break;
	case K_KP_RIGHTARROW:
		key = '6';
		break;
	case K_KP_END:
		key = '1';
		break;
	case K_KP_DOWNARROW:
		key = '2';
		break;
	case K_KP_PGDN:
		key = '3';
		break;
	case K_KP_INS:
		key = '0';
		break;
	case K_KP_DEL:
		key = '.';
		break;
	}

	if ( key > 127 )
	{
		switch ( key )
		{
		case K_DEL:
		default:
			return false;
		}
	}

	/*
	** support pasting from the clipboard
	*/
	if ( ( toupper( key ) == 'V' && keydown[K_CTRL] ) ||
		 ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && keydown[K_SHIFT] ) )
	{
		char *cbd;

		if ( ( cbd = Sys_GetClipboardData() ) != 0 )
		{
			strtok( cbd, "\n\r\b" );

			strncpy( f->buffer, cbd, f->length - 1 );
			f->cursor = strlen( f->buffer );
			f->visible_offset = f->cursor - f->visible_length;
			if ( f->visible_offset < 0 )
				f->visible_offset = 0;

			free( cbd );
		}
		return true;
	}

	switch ( key )
	{
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
	case K_BACKSPACE:
		if ( f->cursor > 0 )
		{
			memmove( &f->buffer[f->cursor-1], &f->buffer[f->cursor], strlen( &f->buffer[f->cursor] ) + 1 );
			f->cursor--;

			if ( f->visible_offset )
			{
				f->visible_offset--;
			}
		}
		break;

	case K_KP_DEL:
	case K_DEL:
		memmove( &f->buffer[f->cursor], &f->buffer[f->cursor+1], strlen( &f->buffer[f->cursor+1] ) + 1 );
		break;

	case K_KP_ENTER:
	case K_ENTER:
	case K_ESCAPE:
	case K_TAB:
		return false;

	case K_SPACE:
	default:
		if ( !isdigit( key ) && ( f->generic.flags & QMF_NUMBERSONLY ) )
			return false;

		if ( f->cursor < f->length )
		{
			f->buffer[f->cursor++] = key;
			f->buffer[f->cursor] = 0;

			if ( f->cursor > f->visible_length )
			{
				f->visible_offset++;
			}
		}
	}

	return true;
}

void Menu_AddItem( menuframework_s *menu, void *item )
{
	if ( menu->nitems == 0 )
		menu->nslots = 0;

	if ( menu->nitems < MAXMENUITEMS )
	{
		menu->items[menu->nitems] = item;
		( ( menucommon_s * ) menu->items[menu->nitems] )->parent = menu;
		menu->nitems++;
	}

	menu->nslots = Menu_TallySlots( menu );
}

/*
** Menu_AdjustCursor
**
** This function takes the given menu, the direction, and attempts
** to adjust the menu's cursor so that it's at the next available
** slot.
*/
void Menu_AdjustCursor( menuframework_s *m, int dir )
{
	menucommon_s *citem;

	/*
	** see if it's in a valid spot
	*/
	if ( m->cursor >= 0 && m->cursor < m->nitems )
	{
		if ( ( citem = Menu_ItemAtCursor( m ) ) != 0 )
		{
			if ( citem->type != MTYPE_SEPARATOR && citem->type != MTYPE_COLORTXT )
				return;
		}
	}

	/*
	** it's not in a valid spot, so crawl in the direction indicated until we
	** find a valid spot
	*/
	if ( dir == 1 )
	{
		while ( 1 )
		{
			citem = Menu_ItemAtCursor( m );
			if ( citem )
				if ( citem->type != MTYPE_SEPARATOR && citem->type != MTYPE_COLORTXT )
					break;
			m->cursor += dir;
			if ( m->cursor >= m->nitems )
				m->cursor = 0;
		}
	}
	else
	{
		while ( 1 )
		{
			citem = Menu_ItemAtCursor( m );
			if ( citem )
				if ( citem->type != MTYPE_SEPARATOR && citem->type != MTYPE_COLORTXT)
					break;
			m->cursor += dir;
			if ( m->cursor < 0 )
				m->cursor = m->nitems - 1;
		}
	}
}

void Menu_Center( menuframework_s *menu )
{
	int height;

	height = ( ( menucommon_s * ) menu->items[menu->nitems-1])->y;
	height += 10;

	menu->y = ( VID_HEIGHT - height ) / 2;
}

void Menu_Draw( menuframework_s *menu )
{
	int i;
	menucommon_s *item;
	menuslider_s *slider;
	int charscale;

	charscale = (float)(viddef.height)*16/600;

	/*
	** draw contents
	*/
	for ( i = 0; i < menu->nitems; i++ )
	{
		switch ( ( ( menucommon_s * ) menu->items[i] )->type )
		{
		case MTYPE_FIELD:
			Field_Draw( ( menufield_s * ) menu->items[i] );
			break;
		case MTYPE_SLIDER:
			Slider_Draw( ( menuslider_s * ) menu->items[i] );
			break;
		case MTYPE_LIST:
			MenuList_Draw( ( menulist_s * ) menu->items[i] );
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw( ( menulist_s * ) menu->items[i] );
			break;
		case MTYPE_ACTION:
			Action_Draw( ( menuaction_s * ) menu->items[i] );
			break;
		case MTYPE_SEPARATOR:
			Separator_Draw( ( menuseparator_s * ) menu->items[i] );
			break;
		case MTYPE_SEPARATOR2:
			Separator2_Draw( ( menuseparator_s * ) menu->items[i] );
			break;
		case MTYPE_VERTSLIDER:
			VertSlider_Draw( ( menuslider_s * ) menu->items[i] );
			break;
		case MTYPE_COLORTXT:
			ColorTxt_Draw ( ( menutxt_s * ) menu->items[i] );
			break;
		case MTYPE_COLORACTION:
			ColorAction_Draw( ( menuaction_s * ) menu->items[i] );
			break;
		}
	}

	// Knightmare- added Psychspaz's mouse support
	/*
	** now check mouseovers - psychospaz
	*/
	cursor.menu = menu;

	if (cursor.mouseaction)
	{
		menucommon_s *lastitem = cursor.menuitem;
		refreshCursorLink();

		for ( i = menu->nitems; i >= 0 ; i-- )
		{
			int type;
			int len;
			int min[2], max[2];

			item = ((menucommon_s * )menu->items[i]);

			if (!item || item->type == MTYPE_SEPARATOR || item->type == MTYPE_COLORTXT)
				continue;

			max[0] = min[0] = menu->x + (item->x + RCOLUMN_OFFSET); //+ 2 chars for space + cursor
			max[1] = min[1] = menu->y + (item->y);
			max[1] += (charscale);

			switch ( item->type )
			{
				case MTYPE_ACTION:
				case MTYPE_COLORACTION:
					{
						len = strlen(item->name);

						if (item->flags & QMF_LEFT_JUSTIFY || item->type == MTYPE_COLORACTION)
						{
							min[0] += (LCOLUMN_OFFSET*2);
							max[0] = min[0] + (len*charscale);
						}
						else
						{
							min[0] -= (len*charscale + charscale*3);
						}

						type = MENUITEM_ACTION;
					}
					break;
				case MTYPE_SLIDER:
					{
						min[0] -= (16);
						max[0] += ((SLIDER_RANGE + 4)*MENU_FONT_SIZE);
						type = MENUITEM_SLIDER;
					}
					break;
				case MTYPE_VERTSLIDER:
					{
						slider = ((menuslider_s * )menu->items[i]);
						min[0] -= (2*charscale);
						max[0] += (2*charscale);
						min[1] -= (charscale);
						max[1] += ((slider->size)*MENU_FONT_SIZE - charscale);
						type = MENUITEM_VERTSLIDER;
					}
					break;
				case MTYPE_LIST:
				case MTYPE_SPINCONTROL:
					{
						int len;
						menulist_s *spin = menu->items[i];


						if (item->name)
						{
							len = strlen(item->name);
							min[0] -= (len*charscale - LCOLUMN_OFFSET*2);
						}

						len = strlen(spin->itemnames[spin->curvalue]);
						max[0] += (len*charscale);

						type = MENUITEM_ROTATE;
					}
					break;
				case MTYPE_FIELD:
					{
						menufield_s *text = menu->items[i];

						len = text->visible_length + 2;

						max[0] += (len*charscale);
						type = MENUITEM_TEXT;
					}
					break;
				default:
					continue;
			}

			if (cursor.x>=min[0] &&
				cursor.x<=max[0] &&
				cursor.y>=min[1] &&
				cursor.y<=max[1])
			{
				//new item
				if (lastitem!=item)
				{
					int j;

					for (j=0;j<MENU_CURSOR_BUTTON_MAX;j++)
					{
						cursor.buttonclicks[j] = 0;
						cursor.buttontime[j] = 0;
					}
				}

				cursor.menuitem = item;
				cursor.menuitemtype = type;

				menu->cursor = i;

				break;
			}
		}
		
	}
	
	cursor.mouseaction = false;
	// end Knightmare

	item = Menu_ItemAtCursor( menu );

	if ( item && item->cursordraw )
	{
		item->cursordraw( item );
	}
	else if ( menu->cursordraw )
	{
		menu->cursordraw( menu );
	}
	else if ( item && (item->type == MTYPE_ACTION || item->type == MTYPE_COLORACTION) ) //change to a "highlite"
	{
		if ( item->flags & QMF_LEFT_JUSTIFY || item->type == MTYPE_COLORACTION)
		{
			if(item->name && item->type == MTYPE_COLORACTION)
				Menu_DrawFilteredString(menu->x + item->x - 24, menu->y + item->y, item->name);
			else if(item->name)
				Menu_DrawStringDark(menu->x + item->x - 24, menu->y + item->y, item->name);
		}
		else
		{
			if(item->name)
				Menu_DrawStringR2L(menu->x + item->x - 24, menu->y + item->y, item->name);
		}
	}
	else if ( item && item->type != MTYPE_FIELD ) //change to a "highlite"
	{
		if ( item->flags & QMF_LEFT_JUSTIFY )
		{
			if(item->name)
				Menu_DrawStringDark(menu->x + item->x - 24, menu->y + item->y, item->name);
		}
		else
		{
			if(item->name)
				Menu_DrawStringR2L(menu->x + item->x - 24, menu->y + item->y, item->name);
		}
	}

	if ( item )
	{
		if ( item->statusbarfunc )
			item->statusbarfunc( ( void * ) item );
		else if ( item->statusbar )
			Menu_DrawStatusBar( item->statusbar );
		else
			Menu_DrawStatusBar( menu->statusbar );

	}
	else
	{
		Menu_DrawStatusBar( menu->statusbar );
	}

	if ( item  && cursor.menuitem)
	{
		if ( item->tooltip )
			Menu_DrawToolTip( item->tooltip );
	}	
}

void Menu_DrawStatusBar( const char *string )
{
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	if ( string )
	{
		int l = strlen( string );
		// int maxrow = VID_HEIGHT / charscale; // unused
		int maxcol = VID_WIDTH / charscale;
		int col = maxcol / 2 - l / 2;

		Draw_Fill( 0, VID_HEIGHT-charscale, VID_WIDTH, charscale, 4 );
		Menu_DrawString( col*charscale, VID_HEIGHT - charscale, string );
	}
	else
	{
		Draw_Fill( 0, VID_HEIGHT-charscale, VID_WIDTH, charscale, 0 );
	}
}

void Menu_DrawToolTip( const char *string )
{
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	if ( string )
	{
		int i;
		int colorChars = 0;
		//position at cursor
		//remove length of color escape chars
		for(i = 0; i < strlen(string); i++)
			if(string[i] == '^' && i < strlen(string) - 1)
				if(string[i+1] != '^')
					colorChars+=2;
		Draw_Fill( cursor.x, cursor.y-charscale, (strlen(string)-colorChars)*charscale, charscale, 4 );
		Menu_DrawColorString( cursor.x, cursor.y - charscale, string );
	}
}

void Menu_DrawString( int x, int y, const char *string )
{
	unsigned i;
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_ScaledChar( ( x + i*charscale ), y, string[i], charscale, true );
	}
}
vec4_t	Color_Table[8] =
{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
};
void Menu_DrawColorString ( int x, int y, const char *str )
{
	int		num;
	vec4_t	scolor;
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	scolor[0] = 0;
	scolor[1] = 1;
	scolor[2] = 0;
	scolor[3] = 1;

	while (*str) {
		if ( Q_IsColorString( str ) ) {
			VectorCopy ( Color_Table[ColorIndex(str[1])], scolor );
			str += 2;
			color_offset +=2;
			continue;
		}

		Draw_ScaledColorChar (x, y, *str, scolor, charscale, true); //this is only ever used for names.

		num = *str++;
		num &= 255;

		if ( (num&127) == 32 ) { //spaces reset colors
			scolor[0] = 0;
			scolor[1] = 1;
			scolor[2] = 0;
			scolor[3] = 1;
		}

		x += charscale;
	}
}
void Menu_DrawColorStringL2R ( int x, int y, const char *str )
{
	int		num, i;
	vec4_t	scolor;
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	scolor[0] = 0;
	scolor[1] = 1;
	scolor[2] = 0;
	scolor[3] = 1;

	//need to know just how many color chars there are before hand, so that it will draw in
	//correct place
	for ( i = 0; i < strlen( str ); i++ )
	{
		if(str[i] == '^' && i < strlen( str )-1) {
			if(str[i+1] != '^')
				x += 2*charscale;
		}
	}

	while (*str) {
		if ( Q_IsColorString( str ) ) {
			VectorCopy ( Color_Table[ColorIndex(str[1])], scolor );
			str += 2;
			color_offset +=2;
			continue;
		}

		Draw_ScaledColorChar (x, y, *str, scolor, charscale, true); //this is only ever used for names.

		num = *str++;
		num &= 255;

		if ( (num&127) == 32 ) { //spaces reset colors
			scolor[0] = 0;
			scolor[1] = 1;
			scolor[2] = 0;
			scolor[3] = 1;
		}

		x += charscale;
	}
}

void Menu_DrawFilteredString ( int x, int y, const char *str )
{
	int		num;
	vec4_t	scolor;
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	while (*str) {
		if ( Q_IsColorString( str ) ) {
			str += 2;
			color_offset +=2;
			continue;
		}

		Draw_ScaledChar (x, y, *str, charscale, true);

		num = *str++;
		num &= 255;

		if ( (num&127) == 32 ) { //spaces reset colors
			scolor[0] = 0;
			scolor[1] = 1;
			scolor[2] = 0;
			scolor[3] = 1;
		}

		x += charscale;
	}
}

void Menu_DrawStringDark( int x, int y, const char *string )
{
	unsigned i;
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_ScaledChar( ( x + i*charscale ), y, string[i] + 128, charscale, true );
	}
}

void Menu_DrawStringR2L( int x, int y, const char *string )
{
	unsigned i;
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_ScaledChar( ( x - i*charscale ), y, string[strlen(string)-i-1], charscale, true );
	}
}

void Menu_DrawStringR2LDark( int x, int y, const char *string )
{
	unsigned i;
	int	charscale;

	charscale = (float)(viddef.height)*16/600;

	for ( i = 0; i < strlen( string ); i++ )
	{
		Draw_ScaledChar( ( x - i*charscale ), y, string[strlen(string)-i-1]+128, charscale, true );
	}
}

void *Menu_ItemAtCursor( menuframework_s *m )
{
	if ( m->cursor < 0 || m->cursor >= m->nitems )
		return 0;

	return m->items[m->cursor];
}

qboolean Menu_SelectItem( menuframework_s *s )
{
	menucommon_s *item = ( menucommon_s * ) Menu_ItemAtCursor( s );

	if ( item )
	{
		switch ( item->type )
		{
		case MTYPE_FIELD:
			return Field_DoEnter( ( menufield_s * ) item ) ;
		case MTYPE_ACTION:
		case MTYPE_COLORACTION:
			Action_DoEnter( ( menuaction_s * ) item );
			return true;
		case MTYPE_LIST:
//			Menulist_DoEnter( ( menulist_s * ) item );
			return false;
		case MTYPE_SPINCONTROL:
//			SpinControl_DoEnter( ( menulist_s * ) item );
			return false;
		}
	}
	return false;
}
qboolean Menu_MouseSelectItem( menucommon_s *item )
{
	if ( item )
	{
		switch ( item->type )
		{
		case MTYPE_FIELD:
			return Field_DoEnter( ( menufield_s * ) item ) ;
		case MTYPE_ACTION:
		case MTYPE_COLORACTION:
			Action_DoEnter( ( menuaction_s * ) item );
			return true;
		case MTYPE_LIST:
		case MTYPE_SPINCONTROL:
			return false;
		}
	}
	return false;
}
void Menu_SetStatusBar( menuframework_s *m, const char *string )
{
	m->statusbar = string;
}

void Menu_SlideItem( menuframework_s *s, int dir )
{
	menucommon_s *item = ( menucommon_s * ) Menu_ItemAtCursor( s );

	if ( item )
	{
		switch ( item->type )
		{
		case MTYPE_SLIDER:
			Slider_DoSlide( ( menuslider_s * ) item, dir );
			break;
		case MTYPE_VERTSLIDER:
			Slider_DoSlide( ( menuslider_s * ) item, dir );
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide( ( menulist_s * ) item, dir );
			break;
		}
	}
}

int Menu_TallySlots( menuframework_s *menu )
{
	int i;
	int total = 0;

	for ( i = 0; i < menu->nitems; i++ )
	{
		if ( ( ( menucommon_s * ) menu->items[i] )->type == MTYPE_LIST )
		{
			int nitems = 0;
			const char **n = ( ( menulist_s * ) menu->items[i] )->itemnames;

			while (*n)
				nitems++, n++;

			total += nitems;
		}
		else
		{
			total++;
		}
	}

	return total;
}

#if 0
// unused
void Menulist_DoEnter( menulist_s *l )
{
	int start;

	start = l->generic.y / 10 + 1;

	l->curvalue = l->generic.parent->cursor - start;

	if ( l->generic.callback )
		l->generic.callback( l );
}
#endif

void MenuList_Draw( menulist_s *l )
{
	const char **n;
	int y = 0;

	Menu_DrawStringR2LDark( l->generic.x + l->generic.parent->x + LCOLUMN_OFFSET, l->generic.y + l->generic.parent->y, l->generic.name );

	n = l->itemnames;

  	Draw_Fill( l->generic.x - 112 + l->generic.parent->x, l->generic.parent->y + l->generic.y + l->curvalue*10 + 10, 128, 10, 16 );
	while ( *n )
	{
		Menu_DrawStringR2LDark( l->generic.x + l->generic.parent->x + LCOLUMN_OFFSET, l->generic.y + l->generic.parent->y + y + 10, *n );

		n++;
		y += 10;
	}
}

void Separator_Draw( menuseparator_s *s )
{
	if ( s->generic.name )
		Menu_DrawStringR2LDark( s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->generic.name );
}
void Separator2_Draw( menuseparator_s *s )
{
	if ( s->generic.name )
		Menu_DrawStringR2L( s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->generic.name );
}
void ColorTxt_Draw( menutxt_s *s )
{
	if ( s->generic.name )
		Menu_DrawColorStringL2R( s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->generic.name );
}
void Slider_DoSlide( menuslider_s *s, int dir )
{
	s->curvalue += dir;

	if ( s->curvalue > s->maxvalue )
		s->curvalue = s->maxvalue;
	else if ( s->curvalue < s->minvalue )
		s->curvalue = s->minvalue;

	if ( s->generic.callback )
		s->generic.callback( s );
}

void Slider_Draw( menuslider_s *s )
{
	int	i;
	int charscale;

	charscale = (float)(viddef.height)*8/600;


	Menu_DrawStringR2LDark( s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET,
		                s->generic.y + s->generic.parent->y,
						s->generic.name );

	s->range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if ( s->range < 0)
		s->range = 0;
	if ( s->range > 1)
		s->range = 1;
	Draw_ScaledChar( s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET,
		s->generic.y + s->generic.parent->y + charscale/2, 128, charscale, true);
	for ( i = 0; i < SLIDER_RANGE; i++ )
		Draw_ScaledChar( RCOLUMN_OFFSET + s->generic.x + i*charscale + s->generic.parent->x + charscale,
		s->generic.y + s->generic.parent->y + charscale/2, 129, charscale, true);
	Draw_ScaledChar( RCOLUMN_OFFSET + s->generic.x + i*charscale + s->generic.parent->x + charscale,
		s->generic.y + s->generic.parent->y + charscale/2, 130, charscale, true);

	Draw_ScaledChar( ( int ) ( charscale + RCOLUMN_OFFSET + s->generic.parent->x + s->generic.x + (SLIDER_RANGE-1)*charscale * s->range ),
		s->generic.y + s->generic.parent->y + charscale/2, 139, charscale, true);
}

void VertSlider_Draw( menuslider_s *s )
{
	int	i;
	int charscale;

	charscale = (float)(viddef.height)*8/600;

	s->range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if ( s->range < 0)
		s->range = 0;
	if ( s->range > 1)
		s->range = 1;
	//top
	Draw_ScaledChar( s->generic.x + s->generic.parent->x + charscale,
		s->generic.y + s->generic.parent->y - charscale, 18, charscale, true);
	Draw_ScaledChar( s->generic.x + s->generic.parent->x + 2*charscale,
		s->generic.y + s->generic.parent->y - charscale, 20, charscale, true);

	for ( i = 0; i <= s->size; i++ ) {
		Draw_ScaledChar( s->generic.x + s->generic.parent->x + charscale,
			s->generic.y + i*charscale + s->generic.parent->y, 21, charscale, true);
		Draw_ScaledChar( s->generic.x + s->generic.parent->x + 2*charscale,
			s->generic.y + i*charscale + s->generic.parent->y, 23, charscale, true);
	}
	//bottom
	Draw_ScaledChar( s->generic.parent->x + s->generic.x + charscale,
		(s->size+1)*charscale + s->generic.y + s->generic.parent->y, 24, charscale, true);
	Draw_ScaledChar( s->generic.parent->x + s->generic.x + 2*charscale,
		(s->size+1)*charscale + s->generic.y + s->generic.parent->y, 26, charscale, true);

	//cursor
	Draw_ScaledChar( (int)( s->generic.parent->x + s->generic.x + 1.5*charscale),
		( int ) (s->generic.y + s->generic.parent->y + (s->size)*charscale * s->range), 11, charscale, true);

}

#if 0
// unused
void SpinControl_DoEnter( menulist_s *s )
{
	s->curvalue++;
	if ( s->itemnames[s->curvalue] == 0 )
		s->curvalue = 0;

	if ( s->generic.callback )
		s->generic.callback( s );
}
#endif

void SpinControl_DoSlide( menulist_s *s, int dir )
{
	s->curvalue += dir;

	if ( s->curvalue < 0 )
		s->curvalue = 0;
	else if ( s->itemnames[s->curvalue] == 0 )
		s->curvalue--;

	if ( s->generic.callback )
		s->generic.callback( s );
}

void SpinControl_Draw( menulist_s *s )
{
	char buffer[100];

	if ( s->generic.name )
	{
		Menu_DrawStringR2LDark( s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET,
							s->generic.y + s->generic.parent->y,
							s->generic.name );
	}
	if ( !strchr( s->itemnames[s->curvalue], '\n' ) )
	{
		Menu_DrawString( RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->itemnames[s->curvalue] );
	}
	else
	{
		strcpy( buffer, s->itemnames[s->curvalue] );
		*strchr( buffer, '\n' ) = 0;
		Menu_DrawString( RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, buffer );
		strcpy( buffer, strchr( s->itemnames[s->curvalue], '\n' ) + 1 );
		Menu_DrawString( RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y + 18, buffer );
	}
}

