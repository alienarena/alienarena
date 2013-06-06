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
#ifndef __QMENU_H__
#define __QMENU_H__

// TODO: This stuff could be useful outside the menu as well.

typedef enum 
{
	linkstatus_literal,
	linkstatus_linktarget,
	linkstatus_link
} linkable_status_t;

// NOTE: if the compiler complains that this is defined but not used, that is
// GOOD! It means the optimizing compiler has detected that the code in the
// ASSERTFAILLINK macro never runs, optimized that code out of the binary, 
// and as a result there's no code that uses this array. But we still need
// the ASSERTFAILLINK macro so that invalid code will be detected as soon as
// it's tested. So DON'T DELETE THIS!
static const char *LINK_STATUS_NAMES[] = {
	"literal",
	"linktarget", 
	"link"
};

#define LINKABLE(type) \
struct \
{ \
	/*For determining whether an encapsulated variable has been linked to 
	  another, and if so, how. */ \
	linkable_status_t status; \
	/*Contents do not matter if status == linkstatus_link*/\
	type val; \
	/*points to val within another struct of this type */ \
	/*Contents do not matter unless status == linkstatus_link*/\
	type *ptr; \
}

#define ASSERTFAILLINK(a,b)\
	Com_Error (ERR_FATAL, "CANNOT CREATE LINK!\n%s (%p) type %s\n%s (%p) type %s", \
	#a, &a, LINK_STATUS_NAMES[a.status], #b, &b, LINK_STATUS_NAMES[b.status]);

#define _ASSERTLINK(a,b)\
{\
	if (b.status == linkstatus_link)\
	{\
		if (a.ptr != b.ptr)\
			ASSERTFAILLINK (a,b)\
	}\
	else if (b.status != linkstatus_linktarget || a.ptr != &(b.val))\
		ASSERTFAILLINK (a,b)\
}

#define ASSERTLINK(a,b)\
{\
	if (a.status == linkstatus_link)\
		_ASSERTLINK(a,b)\
	else if (b.status == linkstatus_link)\
		_ASSERTLINK(b,a)\
	else\
		ASSERTFAILLINK (a,b)\
}

#define _LINK(a,b) \
{\
	b.status = linkstatus_link;\
	if (a.status == linkstatus_link)\
	{\
		b.ptr = a.ptr;\
	}\
	else\
	{\
		a.status = linkstatus_linktarget;\
		b.ptr = &(a.val);\
	}\
}

#define LINK(a,b) \
{\
	if (&(a) != &(b)) \
	{\
		if ((a).status != linkstatus_literal && (b).status != linkstatus_literal)\
			ASSERTLINK ((a),(b))\
		else if ((b).status == linkstatus_literal)\
			_LINK((a),(b))\
		else\
			_LINK((b),(a))\
	}\
}

// The reason this macro is so odd is because you need to be able to do 
// CHASELINK(var) = value directly.
#define CHASELINK(l) \
(*\
	(\
		((l).status == linkstatus_link)?\
		(	/*	Macro evaluates to (*((l).ptr)) */ \
			(l).ptr\
		)\
		:\
		(	/*	Macro evaluates to (*(&((l).val))) which simplifies to (l).val
				The compiler should figure that out and come up with something
				you can assign directly.
			*/ \
			&((l).val)\
		)\
	)\
)

#define RESETLINK(l,v) \
{\
	if ((l).status != linkstatus_link) \
		(l).val = v; \
}

#define INCREASELINK(l,v) \
{\
	if ((l).status == linkstatus_link) \
	{\
		if (*((l).ptr) < v) \
			*((l).ptr) = v; \
		else \
			v = *((l).ptr); \
	}\
	else \
	{\
		(l).val = v; \
	}\
}



#define MAXMENUITEMS	64

typedef enum {
	MTYPE_SLIDER,
	MTYPE_ACTION,
	MTYPE_FIELD,
	MTYPE_NOT_INTERACTIVE,
	MTYPE_SPINCONTROL,
	MTYPE_SUBMENU
} menutype_t ;
#define MTYPE_TEXT MTYPE_NOT_INTERACTIVE

#define	K_TAB			9
#define	K_ENTER			13
#define	K_ESCAPE		27
#define	K_SPACE			32

//menu mouse
#define RCOLUMN_OFFSET  12	// was 16
#define LCOLUMN_OFFSET -12	// was -16
#define SLIDER_RANGE 10

#define NUM_CURSOR_FRAMES 15
#define FONTSCALE 1.5

//menu mouse
#define MOUSEBUTTON1 0
#define MOUSEBUTTON2 1

// normal keys should be passed as lowercased ascii

#define	K_BACKSPACE		127
#define	K_UPARROW		128
#define	K_DOWNARROW		129
#define	K_LEFTARROW		130
#define	K_RIGHTARROW	131

#define QMF_LEFT_JUSTIFY	0x00000001
#define QMF_NUMBERSONLY		0x00000002
#define QMF_ALLOW_WRAP		0x00000004
#define QMF_STRIP_COLOR		0x00000008
#define QMF_SNUG_LEFT		0x00000010

typedef struct
{
	int x, y;
} menuvec2_t;

typedef struct
{
	menutype_t type;
	const char *name;
	LINKABLE(int) x, y;
	int	visible_length;
	struct _tag_menuframework *parent;
	int	localints[3];
	const char *localstrings[1];
	void *localptrs[1];
	unsigned flags;
	
	const char *statusbar;

	const char *tooltip;
	
	// action callbacks
	void		(*callback) (void *self); // clicked on/activated
	void		(*cursorcallback) (void *self, FNT_font_t font); // moused over
	
	// layout callbacks for each column
	menuvec2_t	(*namesizecallback) (void *self, FNT_font_t font); // left
	menuvec2_t	(*itemsizecallback) (void *self, FNT_font_t font); // right
	
	// rendering callbacks
	void		(*itemdraw) (void *self, FNT_font_t font);
	void		(*namedraw) (void *self, FNT_font_t font);
	void		(*cursordraw) (void *self, FNT_font_t font);
	
	// Each menu item may draw different things in the left column and/or the
	// right column. The size of whatever gets drawn in each column is tracked
	// separately.
	LINKABLE(menuvec2_t) lsize, rsize;
} menucommon_s;

typedef struct _tag_menuframework
{
	menucommon_s generic;
	
	int x, y;
	LINKABLE(int) lwidth, rwidth, height;
	int maxwidth, maxheight; // 0 for no limit
	int maxlines; // for generating maxheight automatically
	int xscroll, yscroll;
	qboolean horizontal;
	
	qboolean navagable;
	
	qboolean enable_highlight;
	
	float borderalpha;

	int	nitems;
	void *items[64];

	const char *statusbar;
	const char *tooltip;
	const char *bordertitle;
	const char *bordertexture;
	
	int natural_width;

	void (*cursordraw)( struct _tag_menuframework *m );
} menuframework_s;

typedef struct
{
	menucommon_s generic;

	char		buffer[80];
	int			cursor;
	int			length;
} menufield_s;

typedef struct 
{
	menucommon_s generic;
	
	// slider only
	float minvalue;
	float maxvalue;
	float range;
	int	  size;
	
	// slider and list
	int curvalue;
	
	// list only
	const char **itemnames;
} menumultival_s;

typedef menumultival_s menuslider_s;
typedef menumultival_s menulist_s;

typedef struct 
{
	menucommon_s generic;
} menuitem_s;

typedef menuitem_s menuaction_s;
typedef menuitem_s menutxt_s;

void refreshCursorLink (void);

qboolean Field_Key( menufield_s *field, int key );

void	_Menu_AddItem( menuframework_s *menu, menucommon_s *item );

// Hushes up the incompatible pointer compiler warnings while still preventing
// a genuinely incorrect value from being used:
#define Menu_AddItem(menu,item)\
	_Menu_AddItem((menu), &((item)->generic))


qboolean Menu_SelectMenu (menuframework_s *menu);
void	Menu_AdvanceCursor (int dir);
void	Menu_Center( menuframework_s *menu );
void	Menu_AutoArrange( menuframework_s *menu );
void	Menu_Draw( menuframework_s *menu );
void	Menu_AssignCursor (menuframework_s *menu, int layernum);
void	Menu_DrawHighlight (void);
void	Menu_SelectItem (void);
void	Menu_SetStatusBar( menuframework_s *s, const char *string );
void	Menu_SlideItem (int dir);

void	Menu_DrawString( int, int, const char * );
void	Menu_DrawBox (int x, int y, int w, int h, float alpha, const char *title, const char *prefix);
int		Menu_PredictSize (const char *str);

// utility layout functions
void	Menu_MakeTable (menuframework_s *menu, int nrows, int ncolumns, size_t *celltype_size, menuframework_s *header, menuframework_s *rows, void *columns, const char **contents);

#if !defined min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

#if !defined max
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

#if !defined clamp
#define clamp(x,low,high) (max(min(low,x),high))
#endif

int global_menu_xoffset;
int global_menu_xoffset_progress;
int global_menu_xoffset_target;

#define Item_GetHeight(i) \
	(max(CHASELINK((i).generic.lsize).y, CHASELINK((i).generic.rsize).y))

#define Menu_GetCtrX(m) ((m).x + CHASELINK((m).lwidth))
#define Item_GetX(i) (CHASELINK((i).generic.x) + Menu_GetCtrX(*((i).generic.parent)))

#define Menu_TrueHeight(m) \
	((m).maxheight!=0?min((m).maxheight,CHASELINK((m).height)):CHASELINK((m).height))

#define Menu_TrueWidth(m) \
	(CHASELINK((m).lwidth) + CHASELINK((m).rwidth))

#define Menu_NaturalWidth(m) \
	((m).natural_width == 0 ? Menu_TrueWidth(m) : (m).natural_width)

#define Menu_GetBaseY(m) ((m).y - (m).yscroll)
#define Item_GetY(i) (CHASELINK((i).generic.y) + Menu_GetBaseY(*((i).generic.parent)))

#define Menu_ContainsCursor(m) \
	(	cursor.y > (m).y && cursor.y < (m).y+Menu_TrueHeight(m) && \
		cursor.x > (m).x && cursor.x < (m).x+CHASELINK((m).rwidth)+CHASELINK((m).lwidth) \
	)

struct FNT_window_s	menu_box;

#endif
