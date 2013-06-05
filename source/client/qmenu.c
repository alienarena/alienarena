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

static void	 Action_Draw (menuaction_s *a, FNT_font_t font);
static void  Menu_DrawStatusBar( const char *string );
static void  Menu_DrawToolTip( const char *string );
static void  Text_Draw (menutxt_s *s, FNT_font_t font);
static void	 Slider_DoSlide( menuslider_s *s, int dir );
static void	 Slider_Draw (menuslider_s *s, FNT_font_t font);
static void	 SpinControl_Draw (menulist_s *s, FNT_font_t font);
static void	 SpinControl_DoSlide( menulist_s *s, int dir );
static qboolean	SubMenu_Draw (menuframework_s *s, FNT_font_t font);
static void  Menu_DrawString_Core (int x, int y, const char *string, unsigned int cmode, unsigned int align, const float *color);

static void Menu_DrawHorizBar (const char *pathbase, float x, float y, float w, float base_size);
static void Menu_DrawVertBar (const char *pathbase, float x, float y, float h, float base_size);

void Menu_DrawStringDark( int x, int y, const char *string );
void Menu_DrawStringR2L( int x, int y, const char *string );
void Menu_DrawStringR2LDark( int x, int y, const char *string );

static menuvec2_t Menu_GetBorderSize (menuframework_s *s);

// some potential color schemes.

// dark color = same blue as icons, light color = white
const float dark_color[4] = {0, 1, 135.0f/255.0f, 1};
const float light_color[4] = {1, 1, 1, 1};

// dark color = grey, light color = white
/*const float dark_color[4] = {0.9, 0.9, 0.9, 1};*/
/*const float light_color[4] = {1, 1, 1, 1};*/

// old color scheme
/*#define dark_color FNT_colors[2]*/
/*#define light_color FNT_colors[7*/

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

int Menu_PredictSize (const char *str)
{
	FNT_font_t		font;
	
	font = FNT_AutoGet( CL_menuFont );
	
	return FNT_PredictSize (font, str, true);
}

void Action_Draw (menuaction_s *a, FNT_font_t font)
{
	const float *color;
	unsigned int align, cmode;
	
	if ( a->generic.flags & QMF_LEFT_JUSTIFY )
		align = FNT_ALIGN_LEFT;
	else
		align = FNT_ALIGN_RIGHT;
	
	color = dark_color;
	
	if ( a->generic.flags & QMF_STRIP_COLOR )
		cmode = FNT_CMODE_TWO;
	else
		cmode = FNT_CMODE_QUAKE_SRS;
	
	Menu_DrawString_Core (
		Item_GetX (*a) + LCOLUMN_OFFSET,
		Item_GetY (*a),
		a->generic.name, cmode, align, color
	);
	
	if ( a->generic.itemdraw )
		a->generic.itemdraw( a, font );
}

void Field_Draw (menufield_s *f, FNT_font_t font)
{
	char tempbuffer[128]="";
	int x, y;
	
	Action_Draw ((menuaction_s *)f, font);

	y = Item_GetY (*f);
	x = Item_GetX (*f) + RCOLUMN_OFFSET;
	
	strncpy( tempbuffer, f->buffer, f->generic.visible_length);
	
	Menu_DrawHorizBar ("menu/slide_border", (float)x, (float)y-2.0, (float)(f->generic.visible_length*font->size)+4.0, (float)(font->size)+4.0);
	
	menu_box.x = x+4;
	menu_box.y = y;
	menu_box.height = 0;
	menu_box.width = f->generic.visible_length*font->width;
	FNT_BoundedPrint (font, f->buffer, FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT, &menu_box, dark_color);
	
	if ( Menu_ItemAtCursor( f->generic.parent ) == f )
	{
		if ( ( ( int ) ( Sys_Milliseconds() / 300 ) ) & 1 )
			Draw_StretchPic (menu_box.x + menu_box.width - font->size / 8, menu_box.y-1, font->size, font->size+4, "menu/field_cursor");
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
		return false;

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

		{
			int maxlength = f->length;
			if (f->length > sizeof(f->buffer)-2 || f->length == 0)
			{
				maxlength = sizeof(f->buffer)-2;
			}
			if ( f->cursor < maxlength )
			{
				f->buffer[f->cursor++] = key;
				f->buffer[f->cursor] = 0;
			}
		}
	}

	return true;
}

void _Menu_AddItem( menuframework_s *menu, menucommon_s *item )
{
	if ( menu->nitems < MAXMENUITEMS )
	{
		menu->items[menu->nitems] = item;
		( ( menucommon_s * ) menu->items[menu->nitems] )->parent = menu;
		menu->nitems++;
	}
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
			if ( citem->type != MTYPE_NOT_INTERACTIVE )
				return;
		}
	}
	
	/*
	** it's not in a valid spot, so crawl in the direction indicated until we
	** find a valid spot
	*/
	if ( dir == 1 )
	{
		while (1 )
		{
			citem = Menu_ItemAtCursor( m );
			if ( citem )
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
				break;
			m->cursor += dir;
			if ( m->cursor < 0 )
				m->cursor = m->nitems - 1;
		}
	}
}

void Menu_Center( menuframework_s *menu )
{
	menu->y = ( (int)VID_HEIGHT - Menu_TrueHeight(*menu) ) / 2;
}

int SpinControl_MaxLines (menulist_s *s)
{
	int i;
	
	for (i = 0; s->itemnames[i]; i++)
	{
		if (strchr( s->itemnames[i], '\n' ))
			return 2;
	}
	
	return 1;
}

int SpinControl_MaxWidth (menulist_s *s)
{
	char buffer[100];
	int i;
	int ret = 0;
	
	for (i = 0; s->itemnames[i]; i++)
	{
		if ( !strchr( s->itemnames[i], '\n' ) )
		{
			int npix = Menu_PredictSize (s->itemnames[i]);
			if (npix > ret)
				ret = npix;
		}
		else
		{
			int npix;
			strcpy( buffer, s->itemnames[i] );
			*strchr( buffer, '\n' ) = 0;
			npix = Menu_PredictSize (buffer);
			if (npix > ret)
				ret = npix;
			strcpy( buffer, strchr( s->itemnames[i], '\n' ) + 1 );
			npix = Menu_PredictSize (buffer);
			if (npix > ret)
				ret = npix;
		}
	}
	
	return ret;
}

static inline menuvec2_t Menu_Item_LeftSize (menucommon_s *self, FNT_font_t font)
{
	menuvec2_t ret;
	ret.x = ret.y = 0;
	
	if (self->type == MTYPE_SUBMENU)
	{
		ret = Menu_GetBorderSize ((menuframework_s *)self);
		if (self->flags & QMF_SNUG_LEFT)
			ret.x += CHASELINK(self->parent->lwidth);
		return ret;
	}
	
	if (self->name && !(self->flags & QMF_LEFT_JUSTIFY))
	{
		ret.x = Menu_PredictSize (self->name);
		if (self->type != MTYPE_TEXT)
			ret.x -= LCOLUMN_OFFSET;
	} 
	ret.y = font->height;
	return ret;
}

static inline menuvec2_t Menu_Item_RightSize (menucommon_s *self, FNT_font_t font)
{
	menuvec2_t ret;
	ret.x = ret.y = 0;
	
	ret.y = font->height;
	
	if (self->name && (self->flags & QMF_LEFT_JUSTIFY))
	{
		ret.x = Menu_PredictSize (self->name);
		if (self->type != MTYPE_TEXT)
			ret.x += RCOLUMN_OFFSET;
	} 
	
	if (self->visible_length != 0)
		ret.x += font->width * self->visible_length + RCOLUMN_OFFSET;
	
	switch ( self->type )
	{
		case MTYPE_SLIDER:
			ret.x = font->width * (SLIDER_RANGE+1) + RCOLUMN_OFFSET;
			break;
		case MTYPE_FIELD:
			ret.y += 2;
			break;
		case MTYPE_SPINCONTROL:
			ret.x += SpinControl_MaxWidth ((menulist_s *)self);
			ret.y = SpinControl_MaxLines ((menulist_s *)self)*(font->height);
			break;
		case MTYPE_SUBMENU:
			ret.y = Menu_TrueHeight(*(menuframework_s *)self);
			ret.x = CHASELINK(((menuframework_s *)self)->lwidth) + 
					CHASELINK(((menuframework_s *)self)->rwidth);
			if (self->flags & QMF_SNUG_LEFT)
				ret.x -= CHASELINK(self->parent->lwidth);
			{
				menuvec2_t border = Menu_GetBorderSize ((menuframework_s*)self);
				if (ret.x < border.x + border.x/2)
					ret.x = border.x + border.x/2;
				ret.x += border.x;
				ret.y += border.y;
			}
			break;
		case MTYPE_ACTION:
		case MTYPE_TEXT:
			break;
	}
	
	return ret;
}

/*
=============
Menu_AutoArrange

This section of code is responsible for taking a menu tree and arranging all 
the submenus and menu items on screen, in such a way that they fit the 
constraints imposed on them. I'm afraid this is complicated enough to justify
a small essay.

The reason this is so complicated is because of those constraints. Menu width,
menu item x, and menu item y are all "linkable" attributes, meaning they can
be linked together so they always have the same value. If updated in one
place, the updates show up everywhere else. Here's the problem: these linkable
attributes depend on each other, sometimes in complex ways. 

For example, take a table with two rows:
ITEMA	ITEMB	ITEMC
ITEMD	ITEME	ITEMF
ITEMA and ITEMD's x-coordinates are linked, ITEMB and ITEME's x-coordinates
are linked, etc.

Now the easy way to do this would be to lay out the entirety of row 1, then 
the entirety of row 2. But suppose you have a case where ITEMA and ITEME are
narrow, while ITEMD and ITEMB are wide? Here's what the table looks like 
before it's been laid out at all:
[ITEMA][=ITEMB=][ITEMC]
[=ITEMD=][ITEME][ITEMF]

Laying out the first row goes well. The required amount of space is inserted
between ITEMA and ITEMB, and because ITEMB and ITEME are linked, that change
is propagated to the second row automatically. Likewise for the space between
ITEMB and ITEMC, and the propagation from ITEMC to ITEMF.
[ITEMA]---[=ITEMB=]---[ITEMC]
[=ITEMD=]-[ITEME]-----[ITEMF]

Then the second row is laid out. Since ITEMD is wider, the space between it 
and ITEME isn't enough, so ITEME has to be moved for the space to be 
sufficient. Because ITEMF is still far enough away from ITEME, it doesn't get
moved, and as a result, ITEMC doesn't get moved either! ITEMC's position is no
longer valid, so the first row has to be laid out again.
[ITEMA]-----[=ITEMB=]-[ITEMC]
[=ITEMD=]---[ITEME]---[ITEMF]

To insure a proper layout, Menu_Arrange is simply called over and over again
until no more movement is detected. Actually, the number of calls is limited
at 5 to prevent things from getting too out of hand.
=============
*/

static int changes = 0;
#define MENU_INCREASELINK(l,v) \
{\
	if ((l).status == linkstatus_link) \
	{\
		if (*((l).ptr) < v) \
		{ \
			changes++; \
			*((l).ptr) = v; \
		} \
		else \
			v = *((l).ptr); \
	}\
	else if (reset || (l).val < v) \
	{\
		changes++;\
		(l).val = v; \
	}\
	else \
	{\
		v = (l).val;\
	}\
}

void Menu_Arrange (menuframework_s *menu, qboolean reset, FNT_font_t font)
{
	int i, x, y;
	int itemheight, horiz_height;
	menucommon_s *item;
	
	x = y = 0;
	
	// add some extra space to make things easier to click
	if (menu->horizontal && menu->navagable)
		y += 2*font->size - font->height;
	
	horiz_height = 0;
	
	if (reset && !menu->freeform)
	{
		RESETLINK (menu->rwidth, 0);
		RESETLINK (menu->lwidth, 0);
		RESETLINK (menu->height, 0);
	}

	for (i = 0; i < menu->nitems; i++ )
	{
		int lwidth;
		int rwidth;
		
		item = ((menucommon_s * )menu->items[i]);
		
		if (item->type == MTYPE_SUBMENU)
		{
			menuvec2_t border;
			int ntries;
			int old_nchanges = changes;
			
			border = Menu_GetBorderSize ((menuframework_s*)item);
			
			y += border.y;
			MENU_INCREASELINK (item->y, y);
			y -= border.y;
			
			Menu_Arrange ((menuframework_s *)item, reset, font);
			
			ntries = 0;
			// attempt to keep repetition localized where possible
			while (old_nchanges != changes && ++ntries < 3)
			{
				old_nchanges = changes;
				Menu_Arrange ((menuframework_s *)item, false, font);
			}
		}
		else if (!menu->freeform)
			MENU_INCREASELINK (item->y, y);
		
		if (item->namesizecallback)
			CHASELINK(item->lsize) = item->namesizecallback (item, font);
		else
			CHASELINK(item->lsize) = Menu_Item_LeftSize (item, font);
		if (item->itemsizecallback)
			CHASELINK(item->rsize) = item->itemsizecallback (item, font);
		else
			CHASELINK(item->rsize) = Menu_Item_RightSize (item, font);
		
		// TODO remove freeform
		if (menu->freeform)
		{
			// add some margins to make things easier to click
			CHASELINK(item->y) += 6;
			CHASELINK(item->lsize).x += 6;
			CHASELINK(item->lsize).y += 12;
			CHASELINK(item->rsize).x += 6;
			CHASELINK(item->rsize).y += 12;
			continue;
		}
		
		itemheight = max (CHASELINK(item->lsize).y, CHASELINK(item->rsize).y);
		lwidth = CHASELINK(item->lsize).x;
		rwidth = CHASELINK(item->rsize).x;
		
		if (menu->horizontal)
		{
			if (horiz_height < itemheight)
				horiz_height = itemheight;
		}
		else
			y += itemheight;
			
		if (menu->horizontal)
		{
			if (i == 0)
				MENU_INCREASELINK (menu->lwidth, lwidth)
			else
				x += lwidth + RCOLUMN_OFFSET - LCOLUMN_OFFSET;
			MENU_INCREASELINK (item->x, x)
			x += rwidth;
		}
		else
		{
			MENU_INCREASELINK (menu->lwidth, lwidth);
			MENU_INCREASELINK (menu->rwidth, rwidth);
		}
	}
	
	if (menu->freeform)
		return;
	
	// add some extra space to make things easier to click
	if (menu->horizontal && menu->navagable)
		horiz_height += 2*y;
	
	if (menu->horizontal)
	{
		MENU_INCREASELINK (menu->rwidth, x);
		MENU_INCREASELINK (menu->height, horiz_height);
	}
	else
		MENU_INCREASELINK (menu->height, y);
	
	if (!menu->horizontal && menu->maxlines != 0 && menu->maxlines < menu->nitems)
		menu->maxheight = CHASELINK (((menucommon_s*)menu->items[menu->maxlines])->y);
}

void Menu_AutoArrange (menuframework_s *menu)
{
	int ntries;
	FNT_font_t		font;
	
	font = FNT_AutoGet( CL_menuFont );
	
	Menu_Arrange (menu, true, font);
	ntries = 0;
	while (changes && ++ntries < 5)
	{
		changes = 0;
		Menu_Arrange (menu, false, font);
	}
}

void _Menu_Draw (menuframework_s *menu, FNT_font_t font)
{
	int i;
	menuitem_s *item;
	qboolean submenu_has_mouse = false;
	
	if (menu->bordertexture != NULL)
		Menu_DrawBorder (menu, menu->bordertitle, menu->bordertexture);
	
	// may get covered up later by a higher-priority status bar
	Menu_DrawStatusBar( menu->statusbar );
	
	/*
	** draw contents
	*/
	for ( i = 0; i < menu->nitems; i++ )
	{
		item = ((menuitem_s * )menu->items[i]);
		if (menu->maxheight != 0)
		{
			if (CHASELINK(item->generic.y)-menu->yscroll < 0)
				continue;
			if (CHASELINK(item->generic.y)-menu->yscroll + Item_GetHeight(*item) > menu->maxheight)
				continue;
		}
		
		// TODO: handle these properly in in the following draw methods
		if (item->generic.type == MTYPE_NOT_INTERACTIVE)
		{
			qboolean skip_rest = false;
			if (item->generic.namedraw != NULL)
			{
				item->generic.namedraw (item, font);
				skip_rest = true;
			}
			if (item->generic.itemdraw != NULL)
			{
				item->generic.itemdraw (item, font);
				skip_rest = true;
			}
			if (skip_rest)
				continue;
		}
		
		switch ( item->generic.type )
		{
		case MTYPE_FIELD:
			Field_Draw ((menufield_s *) menu->items[i], font);
			break;
		case MTYPE_SLIDER:
			Slider_Draw ((menuslider_s *) menu->items[i], font);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw ((menulist_s *) menu->items[i], font);
			break;
		case MTYPE_ACTION:
			Action_Draw ((menuaction_s *) menu->items[i], font);
			break;
		case MTYPE_TEXT:
			Text_Draw ((menutxt_s *) menu->items[i], font);
			break;
		case MTYPE_SUBMENU:
			if (SubMenu_Draw ((menuframework_s *) menu->items[i], font))
				// Prevent anything from being highlighted in this menu if a
				// submenu is being navigated. Also, prevent cursor.menu from
				// being overwritten, allowing mouse clicks to be forwarded
				// to the submenu.
				submenu_has_mouse = true;
			break;
		}
	}
	
	if (!menu->navagable)
		return;

	if (Menu_ContainsCursor (*menu) && !submenu_has_mouse)
	{
		cursor.menu = menu;

		if (cursor.mouseaction)
		{
			menuitem_s *lastitem = cursor.menuitem;
			refreshCursorLink();

			for ( i = menu->nitems; i >= 0 ; i-- )
			{
				int min[2], max[2];

				item = ((menuitem_s * )menu->items[i]);

				if (!item || item->generic.type == MTYPE_TEXT)
					continue;

				if (menu->freeform)
				{
					// TODO REMOVE
					max[0] = min[0] = Item_GetX (*item); 
					max[0] += CHASELINK(item->generic.rsize).x;
					min[0] -= CHASELINK(item->generic.lsize).x;
					max[1] = min[1] = Item_GetY (*item);
					max[1] += max (CHASELINK(item->generic.lsize).y, CHASELINK(item->generic.rsize).y);
				}
				else if (menu->horizontal)
				{
					max[0] = min[0] = Item_GetX (*item); 
					max[0] += CHASELINK(item->generic.rsize).x;
					min[0] -= CHASELINK(item->generic.lsize).x;
					min[1] = 0;
					max[1] = VID_HEIGHT;
				}
				else
				{
					max[1] = min[1] = Item_GetY (*item);
					max[1] += max (CHASELINK(item->generic.lsize).y, CHASELINK(item->generic.rsize).y);
					min[0] = 0;
					max[0] = VID_WIDTH;
				}
				
				if (cursor.x>=min[0] &&
					cursor.x<=max[0] &&
					cursor.y>=min[1] &&
					cursor.y<=max[1])
				{
					// Selected a new item-- reset double-click count
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
					cursor.menuitemtype = item->generic.type;

					menu->cursor = i;

					break;
				}
			}
		
		}

		cursor.mouseaction = false;
	}
	else if (!menu->force_highlight)
		return;

	item = Menu_ItemAtCursor( menu );
	
	// Scrolling - make sure the selected item is entirely on screen if
	// possible. TODO: add smoothing?
	if (item && menu->maxheight != 0)
	{
		int y = CHASELINK(item->generic.y);
		menu->yscroll = clamp (menu->yscroll, y, y + Item_GetHeight(*item) - menu->maxheight);
	}
	
	// no actions you can do with these types, so use them only for scrolling
	// and then return
	if (!item || item->generic.type == MTYPE_TEXT)
		return;

	if (item && item->generic.cursorcallback)
		item->generic.cursorcallback (item, font);

	// highlighting 
	if ( item && item->generic.cursordraw )
	{
		item->generic.cursordraw( item, font );
	}
	else if ( menu->cursordraw )
	{
		menu->cursordraw( menu );
	}
	else if ( item && item->generic.type == MTYPE_SUBMENU && ((menuframework_s *)item)->enable_highlight)
	{
		menuframework_s *sm = (menuframework_s *)item;
		// FIXME HACK: draw a solid background, then draw the submenu again
		// on top of the background
		Draw_Fill (Item_GetX (*item), Item_GetY (*item), CHASELINK(sm->lwidth) + CHASELINK(sm->rwidth), Menu_TrueHeight (*sm), 4);
		SubMenu_Draw (sm, font);
	}
	else if ( item && (item->generic.type == MTYPE_SPINCONTROL) )
	{
		unsigned int align;
		int x = Item_GetX (*item);
		if ( item->generic.flags & QMF_LEFT_JUSTIFY)
		{
			align = FNT_ALIGN_LEFT;
		}
		else
		{
			align = FNT_ALIGN_RIGHT;
			x += LCOLUMN_OFFSET;
		}
		Menu_DrawString_Core (
			x, Item_GetY (*item), item->generic.name,
			FNT_CMODE_TWO, align, light_color
		);
	}
	else if ( item && item->generic.type == MTYPE_ACTION ) //change to a "highlite"
	{
		unsigned int align;
		if ( item->generic.flags & QMF_LEFT_JUSTIFY)
			align = FNT_ALIGN_LEFT;
		else
			align = FNT_ALIGN_RIGHT;
		if(item->generic.name)
			Menu_DrawString_Core (
				Item_GetX (*item) + LCOLUMN_OFFSET, Item_GetY (*item), item->generic.name,
				FNT_CMODE_TWO, align, light_color
			);
	}
	else if ( item ) //change to a "highlite"
	{
		if ( item->generic.flags & QMF_LEFT_JUSTIFY )
		{
			if(item->generic.name)
				Menu_DrawStringDark(Item_GetX (*item) + LCOLUMN_OFFSET, Item_GetY (*item), item->generic.name);
		}
		else
		{
			if(item->generic.name)
				Menu_DrawStringR2L(Item_GetX (*item) + LCOLUMN_OFFSET, Item_GetY (*item), item->generic.name);
		}
	}

	if ( item )
	{
		if ( item->generic.statusbar )
			Menu_DrawStatusBar( item->generic.statusbar );
	}

	if ( item  && cursor.menuitem)
	{
		if ( item->generic.tooltip )
			Menu_DrawToolTip( item->generic.tooltip );
	}	
}

// needed because global_menu_xoffset must be added to only the top level of
// any menu tree.
void Menu_Draw( menuframework_s *menu )
{
	FNT_font_t font = FNT_AutoGet (CL_menuFont);
	menu->navagable = true;
	menu->x += global_menu_xoffset;
	_Menu_Draw (menu, font);
	menu->x -= global_menu_xoffset;
}

static menuvec2_t Menu_GetBorderSize (menuframework_s *s)
{
	char topcorner_name[MAX_QPATH];
	menuvec2_t ret;
	FNT_font_t		font;

	font = FNT_AutoGet( CL_menuFont );
	
	ret.x = ret.y = 0;
	
	if (s->bordertexture != NULL)
	{
		Com_sprintf (topcorner_name, MAX_QPATH, "%stopcorner.tga", s->bordertexture);
		Draw_GetPicSize (&ret.x, &ret.y, topcorner_name );
		ret.x = (int)((float)ret.x/64.0*(float)font->size*4.0);
		ret.y = (int)((float)ret.y/64.0*(float)font->size*4.0);
		ret.x -= font->size;
		ret.y -= font->size;
	}
	
	return ret;
}
	
void Menu_DrawBox (int x, int y, int w, int h, float alpha, const char *title, const char *prefix)
{
	char topcorner_name[MAX_QPATH];
	char bottomcorner_name[MAX_QPATH];
	char top_name[MAX_QPATH];
	char bottom_name[MAX_QPATH];
	char side_name[MAX_QPATH];
	char background_name[MAX_QPATH];
	int _tile_w, _tile_h;
	float tile_w, tile_h;
	FNT_font_t		font;

	font = FNT_AutoGet( CL_menuFont );
	
	Com_sprintf (topcorner_name, MAX_QPATH, "%stopcorner.tga", prefix);
	Com_sprintf (bottomcorner_name, MAX_QPATH, "%sbottomcorner.tga", prefix);
	Com_sprintf (top_name, MAX_QPATH, "%stop.tga", prefix);
	Com_sprintf (bottom_name, MAX_QPATH, "%sbottom.tga", prefix);
	Com_sprintf (side_name, MAX_QPATH, "%sside.tga", prefix);
	Com_sprintf (background_name, MAX_QPATH, "%sbackground.tga", prefix);
	
	// assume all tiles are the same size
	Draw_GetPicSize (&_tile_w, &_tile_h, topcorner_name );
	
	tile_w = (float)_tile_w/64.0*(float)font->size*4.0;
	tile_h = (float)_tile_h/64.0*(float)font->size*4.0;
	
	// make room for the scrollbar
	if (!strcmp (prefix, "menu/sm_"))
	{
		w += font->size;
		x -= font->size/2;
	}
	
	if (w < tile_w)
	{
		w = tile_w;
	}
	
	if (h < tile_h)
	{
		h = tile_h;
	}
	
	x -= tile_w/2;
	w += tile_w;
	
	if (!strcmp (prefix, "menu/m_"))
	{
		y -= tile_h/8;
		if (h > tile_h)
			h += tile_h/4;
	}
	
	
	Draw_AlphaStretchTilingPic( x-tile_w/2, y-tile_h/2, tile_w, tile_h, topcorner_name, alpha );
	Draw_AlphaStretchTilingPic( x+w+tile_w/2, y-tile_h/2, -tile_w, tile_h, topcorner_name, alpha );
	Draw_AlphaStretchTilingPic( x-tile_w/2, y+h-tile_h/2, tile_w, tile_h, bottomcorner_name, alpha );
	Draw_AlphaStretchTilingPic( x+w+tile_w/2, y+h-tile_h/2, -tile_w, tile_h, bottomcorner_name, alpha );
	if (w > tile_w)
	{
		Draw_AlphaStretchTilingPic( x+tile_w/2, y-tile_h/2, w-tile_w, tile_h, top_name, alpha );
		Draw_AlphaStretchTilingPic( x+tile_w/2, y+h-tile_h/2, w-tile_w, tile_h, bottom_name, alpha );
	}
	
	if (h > tile_h)
	{
		Draw_AlphaStretchTilingPic( x-tile_w/2, y+tile_h/2, tile_w, h-tile_h, side_name, alpha );
		Draw_AlphaStretchTilingPic( x+w+tile_w/2, y+tile_h/2, -tile_w, h-tile_h, side_name, alpha );
		if (w > tile_w)
			Draw_AlphaStretchTilingPic( x+tile_w/2, y+tile_h/2, w-tile_w, h-tile_h, background_name, alpha );
	}
	
	if (title != NULL)
	{
		int textwidth = Menu_PredictSize (title);
		Menu_DrawHorizBar ("menu/slide_border", x+w/2-textwidth/2-2, y-font->size-2, textwidth+4, font->size+4);
		Menu_DrawString_Core (x+w/2, y-font->size, title, FNT_CMODE_QUAKE, FNT_ALIGN_CENTER, light_color);
	}
}


void Menu_DrawVertBar (const char *pathbase, float x, float y, float h, float base_size)
{
	char scratch[MAX_QPATH];
	
	Com_sprintf( scratch, sizeof( scratch ), "%s%s", pathbase, "_end");
	
	Draw_AlphaStretchTilingPic (x, y-base_size/2.0, base_size, base_size, scratch, 1);
	Draw_AlphaStretchTilingPic (x, y+h+base_size/2.0, base_size, -base_size, scratch, 1);
	if (h > base_size)
		Draw_AlphaStretchTilingPic (x, y+base_size/2.0, base_size, h-base_size, pathbase, 1);
}

void Menu_DrawHorizBar (const char *pathbase, float x, float y, float w, float base_size)
{
	char scratch[MAX_QPATH];
	
	Com_sprintf( scratch, sizeof( scratch ), "%s%s", pathbase, "_end");
	
	Draw_AlphaStretchTilingPic (x-base_size/2.0, y, base_size, base_size, scratch, 1);
	Draw_AlphaStretchTilingPic (x+w+base_size/2.0, y, -base_size, base_size, scratch, 1);
	if (w > base_size)
		Draw_AlphaStretchTilingPic (x+base_size/2.0, y, w-base_size, base_size, pathbase, 1);
}

void Menu_DrawScrollbar (menuframework_s *menu)
{
	float		maxscroll, scroll_range, scroll_top;
	float		scrollbar_size, scrollbar_pos;
	float		charscale, right;
	FNT_font_t	font;
	
	font = FNT_AutoGet( CL_menuFont );
	charscale = font->size;
	
	if (menu->maxheight == 0 || CHASELINK(menu->height) <= menu->maxheight)
		return;
	
	right = menu->x + CHASELINK(menu->rwidth) + CHASELINK(menu->lwidth);
	
	maxscroll = CHASELINK(menu->height) - menu->maxheight;
	scroll_range = (float)menu->maxheight-charscale/2.0;
	scrollbar_size = scroll_range-maxscroll;
	if (scrollbar_size < charscale)
		scrollbar_size = charscale;
	
	scrollbar_pos = (float)menu->yscroll*(scroll_range-scrollbar_size)/maxscroll;
	scroll_top = (float)menu->y+charscale/4.0;
	
	Menu_DrawVertBar ("menu/scroll_border", right, menu->y, menu->maxheight, charscale);
	Menu_DrawVertBar ("menu/scroll_cursor", right, scroll_top+scrollbar_pos, scrollbar_size, charscale);
}

void Menu_DrawBorder (menuframework_s *menu, const char *title, const char *prefix)
{
	int height, width;
	
	height = CHASELINK(menu->height);
	
	if (menu->maxheight != 0 && height > menu->maxheight)
		height = menu->maxheight;
	
	if (strcmp (prefix, "menu/m_"))
	{
		menu->borderalpha = 0.50;
	}
	else if (Menu_ContainsCursor (*menu))
	{
		menu->borderalpha += cls.frametime*2;
		if (menu->borderalpha > 1.0)
			menu->borderalpha = 1.0;
	}
	else
	{
		menu->borderalpha -= cls.frametime*2;
		if (menu->borderalpha < 0.5)
			menu->borderalpha = 0.5;
	}
	
	width = CHASELINK(menu->lwidth) + CHASELINK(menu->rwidth);
	Menu_DrawBox (menu->x, menu->y, width, height, menu->borderalpha, title, prefix);
	
	Menu_DrawScrollbar (menu);
}

void Menu_DrawToolTip( const char *string )
{
	FNT_font_t		font;
	
	font = FNT_AutoGet( CL_menuFont );

	if ( string )
	{
		Menu_DrawHorizBar ("menu/slide_border", cursor.x-2, cursor.y-font->size-4, Menu_PredictSize (string)+4, font->size+4);
		Menu_DrawString_Core (
			cursor.x, cursor.y - font->size - 4, 
			string, FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT,
			light_color
		);
	}
}

void Menu_DrawString_Core (int x, int y, const char *string, unsigned int cmode, unsigned int align, const float *color)
{
	FNT_font_t		font;
	
	font = FNT_AutoGet( CL_menuFont );
	
	if (align == FNT_ALIGN_RIGHT)
	{
		menu_box.x = 0;
		menu_box.width = x;
	}
	else if (align == FNT_ALIGN_CENTER)
	{
		//width only needs to be an upper bound
		int width = strlen(string)*font->size; 
		menu_box.x = x-width/2;
		menu_box.width = width;
	}
	else
	{
		menu_box.x = x;
		menu_box.width = 0;
	}
	
	menu_box.y = y;
	menu_box.height = 0;
	
	FNT_BoundedPrint (font, string, cmode, align, &menu_box, color);
	
	if (align == FNT_ALIGN_RIGHT)
		menu_box.x = x-menu_box.width;
	else if (align == FNT_ALIGN_CENTER)
		menu_box.x = x-menu_box.width/2;
}

void Menu_DrawStatusBar( const char *string )
{
	FNT_font_t		font;
		
	font = FNT_AutoGet( CL_menuFont );

	if ( string )
	{
		Draw_Fill( 0, VID_HEIGHT-font->size-10, VID_WIDTH, font->size+10, 4 );
		Menu_DrawString_Core (VID_WIDTH/2, VID_HEIGHT-font->size-5, string, FNT_CMODE_QUAKE, FNT_ALIGN_CENTER, light_color);
	}
}

void Menu_DrawString( int x, int y, const char *string )
{
	Menu_DrawString_Core (x, y, string, FNT_CMODE_NONE, FNT_ALIGN_LEFT, light_color);
}
void Menu_DrawStringDark( int x, int y, const char *string )
{
	Menu_DrawString_Core (x, y, string, FNT_CMODE_NONE, FNT_ALIGN_LEFT, dark_color);
}
void Menu_DrawStringR2L( int x, int y, const char *string )
{
	Menu_DrawString_Core (x, y, string, FNT_CMODE_NONE, FNT_ALIGN_RIGHT, light_color);
}
void Menu_DrawStringR2LDark( int x, int y, const char *string )
{
	Menu_DrawString_Core (x, y, string, FNT_CMODE_NONE, FNT_ALIGN_RIGHT, dark_color);
}
void Menu_DrawColorString ( int x, int y, const char *str )
{
	Menu_DrawString_Core (x, y, str, FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT, dark_color);
}

void *Menu_ItemAtCursor( menuframework_s *m )
{
	if ( m->cursor < 0 || m->cursor >= m->nitems )
		return 0;

	return m->items[m->cursor];
}

void Menu_SelectItem( menuframework_s *s )
{
	menucommon_s *item = ( menucommon_s * ) Menu_ItemAtCursor( s );
	if (item->callback != NULL)
		item->callback( item );
}
void Menu_MouseSelectItem( menucommon_s *item )
{
	if (item->callback != NULL)
		item->callback( item );
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
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide( ( menulist_s * ) item, dir );
			break;
		}
	}
}

void Text_Draw (menutxt_s *s, FNT_font_t font)
{
	unsigned int align;
	
	if ( s->generic.name == NULL)
		return;
	
	if ( s->generic.flags & QMF_LEFT_JUSTIFY )
		align = FNT_ALIGN_LEFT;
	else
		align = FNT_ALIGN_RIGHT;
	
	Menu_DrawString_Core (
		Item_GetX (*s), Item_GetY (*s),
		s->generic.name, FNT_CMODE_QUAKE_SRS, align, dark_color
	);
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

void Slider_Draw (menuslider_s *s, FNT_font_t font)
{
	float		maxscroll, curscroll, scroll_range, cursor_size, x, width;
	float 		charscale;

	charscale = font->size;
	
	Menu_DrawStringR2LDark( Item_GetX (*s) + LCOLUMN_OFFSET,
					Item_GetY (*s),
					s->generic.name );
	
	x = Item_GetX (*s) + RCOLUMN_OFFSET;
	
	curscroll = s->curvalue - s->minvalue;
	maxscroll = s->maxvalue - s->minvalue;
	
	s->range = (float) curscroll / ( float ) maxscroll;
	
	width = charscale * (float)SLIDER_RANGE;
	scroll_range = width-charscale/2.0;
	cursor_size = charscale;
	
	Menu_DrawHorizBar ("menu/slide_border", x, Item_GetY (*s), width, charscale);
	Menu_DrawHorizBar ("menu/slide_cursor", x+charscale/4.0+s->range*(scroll_range-cursor_size), Item_GetY (*s), cursor_size, charscale);
}

void SpinControl_DoSlide( menulist_s *s, int dir )
{
	int i;
	s->curvalue += dir;

	if ( s->curvalue < 0 )
	{
		if (s->generic.flags & QMF_ALLOW_WRAP)
		{
			for (i = 0; s->itemnames[i]; i++)
				continue;
			s->curvalue += i;
		}
		else
			s->curvalue = 0;
	}
	else if ( s->itemnames[s->curvalue] == 0 )
	{
		if (s->generic.flags & QMF_ALLOW_WRAP)
			s->curvalue = 0;
		else
			s->curvalue--;
	}

	if ( s->generic.callback )
		s->generic.callback( s );
}

void SpinControl_Draw (menulist_s *s, FNT_font_t font)
{
	char buffer[100];
	
	int x = Item_GetX (*s);

	if (s->generic.namedraw != NULL)
	{
		s->generic.namedraw (s, font);
	}
	else if ( s->generic.name )
	{
		unsigned int align;
		int text_x = x;
		
		if (s->generic.flags & QMF_LEFT_JUSTIFY)
		{
			align = FNT_ALIGN_LEFT;
			x += Menu_PredictSize (s->generic.name);
		}
		else
		{
			align = FNT_ALIGN_RIGHT;
			text_x += LCOLUMN_OFFSET;
		}
			
		Menu_DrawString_Core (
			text_x, Item_GetY (*s), 
			s->generic.name, FNT_CMODE_QUAKE_SRS, align,
			dark_color
		);
	}
	if (s->generic.itemdraw != NULL)
	{
		s->generic.itemdraw (s, font);
	}
	else if ( !strchr( s->itemnames[s->curvalue], '\n' ) )
	{
		Menu_DrawString_Core (
			RCOLUMN_OFFSET + x, Item_GetY (*s), 
			s->itemnames[s->curvalue], FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT,
			light_color
		);
	}
	else
	{
		strcpy( buffer, s->itemnames[s->curvalue] );
		*strchr( buffer, '\n' ) = 0;
		Menu_DrawString_Core (
			RCOLUMN_OFFSET + x, Item_GetY (*s), 
			buffer, FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT,
			light_color
		);
		strcpy( buffer, strchr( s->itemnames[s->curvalue], '\n' ) + 1 );
		Menu_DrawString_Core (
			menu_box.x, menu_box.y+menu_box.height, 
			buffer, FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT,
			light_color
		);
	}
}

qboolean SubMenu_Draw (menuframework_s *sm, FNT_font_t font)
{
	sm->x = Item_GetX (*sm);
	if (sm->generic.flags & QMF_SNUG_LEFT)
		sm->x -= CHASELINK(sm->generic.parent->lwidth);
	sm->y = Item_GetY (*sm);
	if (sm->navagable)
		Menu_DrawScrollbar (sm);
	_Menu_Draw (sm, font);
	return (sm->navagable && Menu_ContainsCursor (*sm));
}

// utility functions

void Menu_MakeTable (menuframework_s *menu, int nrows, int ncolumns, size_t *celltype_size, menuframework_s *header, menuframework_s *rows, void *columns, const char **contents)
{
	int i, j;
	menuframework_s *cur_row = rows;
	// char because the measurements in celltype_size are in bytes
	char *cur_cell_p = columns; 
	
	menu->nitems = 0;
	for (i = 0; i < nrows; i++)
	{
		cur_row->nitems = 0;
		cur_row->generic.type = MTYPE_SUBMENU;
		cur_row->horizontal = true;
		cur_row->navagable = false;
		cur_row->enable_highlight = true;
		if (cur_row != header)
		{
			LINK(header->lwidth, cur_row->lwidth);
			LINK(header->rwidth, cur_row->rwidth);
		}
		for (j = 0; j < ncolumns; j++)
		{
			menuitem_s *cur_cell = (menuitem_s *)cur_cell_p;
			if (cur_row != header)
			{
				menucommon_s *header_cell = (menucommon_s *)header->items[j];
				memcpy (cur_cell, header_cell, sizeof (menucommon_s));
				//reset after memcpy
				cur_cell->generic.x.status = linkstatus_literal;
				LINK(header_cell->x, cur_cell->generic.x);
			}
			cur_cell->generic.name = contents[i*ncolumns+j];
			Menu_AddItem (cur_row, cur_cell);
			cur_cell_p += celltype_size[j];
		}
		Menu_AddItem (menu, cur_row);
		cur_row++;
	}
	Menu_AutoArrange (menu);
}
