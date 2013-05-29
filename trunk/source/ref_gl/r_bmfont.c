/*
Copyright (C) 2010 COR Entertainment, LLC.

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

/* r_bmfont.c
 *
 * Displays text using bitmap fonts loaded from pictures.
 * This code is meant to replace the old Draw_*Char routines and the various
 * parts of the code that use them.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "r_local.h"
#include "r_text.h"
#include "r_bmfont.h"


/*****************************************************************************
 * LOCAL FUNCTIONS ENCAPSULATED BY FONT/FACE STRUCTURES                      *
 *****************************************************************************/

/* Destroy a bitmap font face */
static void _BMF_DestroyFace( FNT_face_t face );

/* Initialise a bitmap font */
static qboolean _BMF_GetFont( FNT_font_t font );

/* Raw printing function */
static void _BMF_RawPrint(
	FNT_font_t	font ,
	const char *	text ,
	unsigned int	text_length ,
	qboolean	r2l ,
	float		x ,
	float		y ,
	const float	color[4] );

/* Bounded printing function */
static void _BMF_BoundedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		FNT_window_t	box ,
		const float *	color );

/* Wrapped printing function */
static void _BMF_WrappedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		unsigned int	indent ,
		FNT_window_t	box ,
		const float *	color
	);

/* Size prediction function */
static int _BMF_PredictSize(
		FNT_font_t	font ,
		const char *	text
	);


/*****************************************************************************
 * LOCAL TYPE DEFINITIONS                                                    *
 *****************************************************************************/

/*
 * This structure "replaces" the generic structure; it is used to store the
 * GL list base identifier.
 */
struct _BMF_font_s
{
	/* The font's look-up key */
	char			lookup[ FNT_FONT_KEY_MAX + 1 ];

	/* The font's face */
	FNT_face_t		face;

	/* The font's height in pixels */
	unsigned int		size;

	/* Printing functions */
	FNT_RawPrint_funct	RawPrint;
	FNT_BoundedPrint_funct	BoundedPrint;
	FNT_WrappedPrint_funct	WrappedPrint;
	FNT_PredictSize_funct	PredictSize;

	/* Destructor */
	FNT_DestroyFont_funct	Destroy;

	/* Amount of "unfreed" GetFont's for this font */
	unsigned int		used;

	/* Internal data used by the actual font renderer - replaces the void* pointer */
	GLuint			listBase;
};
typedef struct _BMF_font_s * _BMF_font_t;


/*****************************************************************************
 * FONT FRONT-END INTERFACE FUNCTION                                         *
 *****************************************************************************/

qboolean BMF_InitFace( FNT_face_t face )
{
	char		fileName[ FNT_FACE_NAME_MAX + 11 ];

	Com_sprintf( fileName , sizeof( fileName ) , "fonts/%s.tga" , face->name );
	face->internal = GL_FindImage( fileName , it_pic );
	if ( face->internal == NULL )
		return false;

	face->Destroy = _BMF_DestroyFace;
	face->GetFont = _BMF_GetFont;

	return ( face->internal != NULL );
}



/*****************************************************************************
 * FONT MANAGEMENT FUNCTIONS                                                 *
 *****************************************************************************/

/* Face destruction function */
static void _BMF_DestroyFace( FNT_face_t face )
{
	GL_FreeImage( (image_t *) face->internal );
}


/*
 * Destroys a font's data structure
 */
static void _BMF_Destroy( FNT_font_t font )
{
	_BMF_font_t	_font = (_BMF_font_t) font;
	qglDeleteLists( _font->listBase , 255 );
}


/* Initialise a bitmap font */
static qboolean _BMF_GetFont( FNT_font_t font )
{
	_BMF_font_t	_font = (_BMF_font_t) font;
	GLuint		glListBase;
	unsigned int	i;

	assert( sizeof( struct _BMF_font_s ) <= sizeof( struct FNT_font_s ) );

	// Initialise GL lists
	//
	// XXX: make sure there's no pending error first; this should not be
	// needed, but since the rest of the GL code doesn't check for errors
	// that often...
	qglGetError( );
	glListBase = qglGenLists( 255 );
	if ( qglGetError( ) != GL_NO_ERROR ) {
		Com_Printf( "BMF: could not create OpenGL lists\n" );
		return false;
	}
	_font->listBase = glListBase;

	// Set up GL lists from the font
	for ( i = 0 ; i < 256 ; i ++ ) {
		unsigned int	row , col;
		float		fRow , fCol;

		// Don't bother with spaces
		if ( i == ' ' ) {
			continue;
		}

		// Grid coordinates
		row = i >> 4;
		col = i & 0xf;

		// Texture coordinates
		fRow = row / 16.0;
		fCol = col / 16.0;

		// Initialise list
		qglNewList( glListBase , GL_COMPILE );
		qglBegin( GL_QUADS );
		qglTexCoord2f( fCol , fRow );
		qglVertex2f( 0 , 0 );
		qglTexCoord2f( fCol + 1.0 / 16.0 , fRow );
		qglVertex2f( font->size , 0 );
		qglTexCoord2f( fCol + 1.0 / 16.0 , fRow + 1.0 / 16.0 );
		qglVertex2f( font->size , font->size );
		qglTexCoord2f( fCol , fRow + 1.0 / 16.0 );
		qglVertex2f( 0 , font->size );
		qglEnd ();
		qglEndList( );

		glListBase ++;
	}

	font->Destroy = _BMF_Destroy;
	font->RawPrint = _BMF_RawPrint;
	font->BoundedPrint = _BMF_BoundedPrint;
	font->WrappedPrint = _BMF_WrappedPrint;
	font->PredictSize = _BMF_PredictSize;

	return true;
}



/*****************************************************************************
 * PRINTING FUNCTIONS - INTERNALS                                            *
 *****************************************************************************/

/*
 * Set up the environment so bitmap font text can be drawn.
 */
static void _BMF_PrepareToDraw( FNT_face_t face )
{
	// Save current context
	qglPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT );
	qglMatrixMode( GL_MODELVIEW );
	qglPushMatrix( );

	// Prepare texture
	GL_Bind( ( (image_t *) face->internal )->texnum );
	GL_TexEnv( GL_MODULATE );

	// Set blending function
	qglDisable( GL_ALPHA_TEST );
	qglEnable( GL_BLEND );
	qglBlendFunc ( GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA );
}


/*
 * Restores the environment as if was before _BMF_PrepareToDraw was called
 */
static void _BMF_RestoreEnvironment( )
{
	qglPopMatrix( );
	qglPopAttrib( );
}


/*
 * Print a wrapped line.
 */
static unsigned int _BMF_PrintWrappedLine(
		FNT_font_t			font ,
		const _FNT_render_info_t	renderInfo ,
		unsigned int *			riIndex ,
		unsigned int			riLength ,
		unsigned int			align ,
		unsigned int			indent ,
		const FNT_window_t		box ,
		unsigned int			cHeight )
{
	_BMF_font_t	_font		= (_BMF_font_t) font;
	unsigned int	width		= 0;
	unsigned int	wsIndex		= *riIndex;
	const float *	curColor	= NULL;
	unsigned int	drawFrom	= wsIndex;
	unsigned int	drawTo;
	unsigned int	weIndex;
	unsigned int	x;

	while ( wsIndex < riLength ) {
		unsigned int nWidth;

		// Find end of word
		weIndex = wsIndex + 1;
		while ( weIndex < riLength && renderInfo[ weIndex ].toDraw != ' ' ) {
			weIndex ++;
		}

		// Compute word width
		nWidth = ( weIndex - wsIndex ) * font->size;
		if ( width != 0 ) {
			nWidth += font->size;
		}

		// Is there enough space?
		if ( width + nWidth < box->width - indent ) {
			// No problem, we can draw that
			drawTo = wsIndex = weIndex;
			if ( wsIndex < riLength ) {
				wsIndex ++;
			}
		} else {
			if ( width == 0 ) {
				// Word is longer than maximal width, split it
				drawTo = wsIndex + ( ( box->width - indent ) / font->size );
				width = ( drawTo - drawFrom ) * font->size;
				assert( width < box->width - indent );
				wsIndex = drawTo;
			}

			// End line here.
			break;
		}
		width += nWidth;
	}
	*riIndex = wsIndex;

	// Compute coordinates from alignment
	switch ( align ) {
		case FNT_ALIGN_LEFT:
			x = box->x + indent;
			break;
		case FNT_ALIGN_RIGHT:
			x = box->x + box->width - indent - width;
			break;
		case FNT_ALIGN_CENTER:
			x = box->x + ( box->width - width ) / 2;
			break;
	}

	// Do not draw stuff outside of screen
	if ( box->y + cHeight >= viddef.height || box->y + cHeight + font->size < 0
			|| x + width < 0 || x > viddef.width ) {
		return width;
	}

	// Draw characters
	qglPushMatrix( );
	qglTranslatef( (float)x , (float)( box->y + cHeight ) , 0 );
	while ( drawFrom < drawTo ) {
		if ( renderInfo[ drawFrom ].toDraw != ' ' ) {
			int index = renderInfo[ drawFrom ].toDraw;
			if ( index > ' ' ) {
				index --;
			}

			if ( curColor != renderInfo[ drawFrom ].color ) {
				qglColor4fv( curColor = renderInfo[ drawFrom ].color );
			}
			qglCallList( _font->listBase + index );
		}
		qglTranslatef( (float) font->size , 0 , 0 );
		drawFrom ++;
	}
	qglPopMatrix( );

	return width;
}



/*****************************************************************************
 * PRINTING FUNCTIONS AVAILABLE THROUGH FRONT-END                            *
 *****************************************************************************/

/*
 * Raw printing function
 */
static void _BMF_RawPrint(
	FNT_font_t	font ,
	const char *	text ,
	unsigned int	text_length ,
	qboolean	r2l ,
	float		x ,
	float		y ,
	const float	color[4] )
{
	_BMF_font_t	_font = (_BMF_font_t) font;

	if ( y < -(float) font->size )
		return;

	_BMF_PrepareToDraw( font->face );

	qglColor4fv( color );
	qglTranslatef( x , y , 0 );
	while ( text_length )
	{
		if ( x >= -(float) font->size && *text != ' ' ) {
			int where = ( r2l ? -1 : 1 ) * font->size;
			int index = *text;
			if ( index > ' ' ) {
				index --;
			}
			index += _font->listBase;

			qglCallList( index );
			qglTranslatef( (float) where , 0 , 0 );
			x += where;
		}

		r2l ? ( text-- ) : ( text++ );
		text_length --;
	}

	_BMF_RestoreEnvironment( );
}


/*
 * Bounded printing function
 */
static void _BMF_BoundedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		FNT_window_t	box ,
		const float *	color
	)
{
	_BMF_font_t	_font		= (_BMF_font_t) font;
	const char *	ptr		= text;
	unsigned int	cHeight		= 0;
	unsigned int	maxWidth	= 0;
	qboolean	expectColor	= false;
	qboolean	colorChanged	= true;

	_BMF_PrepareToDraw( font->face );

	// Force left alignment if no width is specified
	if ( box->width == 0 ) {
		align = FNT_ALIGN_LEFT;
	}

	qglTranslatef( box->x , box->y , 0 );
	while ( box->height == 0 || cHeight + font->size < box->height ) {
		const char *	lineStart	= ptr;
		unsigned int	lineWidth	= 0;
		const float *	curColor	= color;
		unsigned int	nSkip		= 0;
		unsigned int	sx		= 0;
		unsigned int	nDisplay;

		while ( *ptr && *ptr != '\n' ) {
			// Skip color codes
			if ( ! _FNT_HandleColor( &ptr , cmode , color , &expectColor , &colorChanged , &curColor ) ) {
				continue;
			}

			lineWidth += font->size;
			ptr ++;
		}

		// If there is nothing to display in this line...
		if ( lineWidth == 0 ) {
			// End of string?
			if ( !*ptr ) {
				break;
			}

			// Nope, prepare for next line
			cHeight += font->size;
			qglTranslatef( 0 , (float) font->size , 0 );
			ptr ++;
			break;
		}

		// Get first character and amount of characters to display
		if ( box->width == 0 || lineWidth <= box->width ) {
			nDisplay = lineWidth / font->size;
			nSkip = 0;
		} else {
			nDisplay = box->width / font->size;
			switch ( align ) {
				case FNT_ALIGN_LEFT:
					nSkip = 0;
					break;
				case FNT_ALIGN_CENTER:
					nSkip = ( lineWidth - box->width ) / ( 2 * font->size );
					break;
				case FNT_ALIGN_RIGHT:
					nSkip = ( lineWidth - box->width ) / font->size;
					break;
			}
			lineWidth = nDisplay * font->size;
		}
		if ( lineWidth > maxWidth ) {
			maxWidth = lineWidth;
		}

		// Compute starting  location
		switch ( align ) {
			case FNT_ALIGN_LEFT:
				sx = 0;
				break;
			case FNT_ALIGN_CENTER:
				sx = ( box->width - lineWidth ) / 2;
				break;
			case FNT_ALIGN_RIGHT:
				sx = box->width - lineWidth;
				break;
		}

		// Display characters
		qglPushMatrix( );
		curColor = color;
		expectColor = false;
		colorChanged = true;
		qglTranslatef( (float)sx , 0 , 0 );
		while ( nDisplay ) {
			// Handle color codes
			if ( ! _FNT_HandleColor( &lineStart , cmode , color , &expectColor , &colorChanged , &curColor ) ) {
				continue;
			}

			// Found a displayable character
			if ( nSkip ) {
				// ... but it's not display time yet.
				nSkip --;
				lineStart ++;
				continue;
			}

			// Display character
			if ( *lineStart != ' ' ) {
				int index = *lineStart;
				if ( cmode == FNT_CMODE_TWO ) {
					index &= 0x7f;
				}
				if ( index > ' ' ) {
					index --;
				}

				if ( colorChanged ) {
					qglColor4fv( curColor );
				}
				qglCallList( _font->listBase + index );
			}
			qglTranslatef( (float) font->size , 0 , 0 );
			nDisplay --;
			lineStart ++;
		}
		qglPopMatrix( );
		qglTranslatef( 0 , (float) font->size , 0 );

		// Prepare for next line
		if ( ! *ptr ) {
			break;
		}
		ptr ++;
	}

	box->height = cHeight;
	box->width = maxWidth;

	_BMF_RestoreEnvironment( );
}


/*
 * Wrapped printing function
 */
static void _BMF_WrappedPrint(
		FNT_font_t	font ,
		const char *	text ,
		unsigned int	cmode ,
		unsigned int	align ,
		unsigned int	indent ,
		FNT_window_t	box ,
		const float *	color
	)
{
	const char *			curText		= text;
	unsigned int			cHeight		= 0;
	unsigned int			nHeight		= 0;
	unsigned int			maxWidth	= 0;
	qboolean			mustQuit	= false;

	_FNT_render_info_t	renderInfo;
	unsigned int			riLength;

	renderInfo = Z_Malloc( strlen( text ) * sizeof( struct _FNT_render_info_s ) );
	_BMF_PrepareToDraw( font->face );

	while ( !mustQuit && ( box->height == 0 || font->size + cHeight + nHeight < box->height ) ) {
		unsigned int	riIndex = 0;

		// Get next line
		mustQuit = ! _FNT_NextWrappedUnit( &curText , renderInfo , &riLength , cmode , color );

		// Empty line, skip space vertically
		if ( riLength == 0 ) {
			nHeight += font->size;
			continue;
		}

		// Add skipped space
		cHeight += nHeight;
		nHeight = 0;

		while ( riIndex < riLength && ( box->height == 0 || font->size + cHeight < box->height ) ) {
			unsigned int w = _BMF_PrintWrappedLine( font , renderInfo , &riIndex , riLength ,
				align , riIndex == 0 ? 0 : indent , box , cHeight );
			if ( w > maxWidth ) {
				maxWidth = w;
			}
			cHeight += font->size;
		}
	}

	_BMF_RestoreEnvironment( );
	Z_Free( renderInfo );

	box->width = maxWidth;
	box->height = cHeight;
}

/*
 * Size prediction function
 */
static int _BMF_PredictSize(
		FNT_font_t	font ,
		const char *	text
	)
{
	return font->size*strlen(text);
}
