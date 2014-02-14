/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (c) 2014 COR Entertainment, LLC.

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
// common.c -- misc functions used in client and server

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "com_std.h"

#if HAVE_SETJMP_H
#include <setjmp.h>
#endif

#ifdef DEDICATED_ONLY
#undef HAVE_ZLIB
#endif 

#if HAVE_ZLIB
#include <zlib.h>
#endif

#if HAVE_SETJMP_H
#include <setjmp.h>
#endif

#include "qcommon.h"

/* compressed normal vector set for net packets */
vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
#include "client/anorms.h"
};

#define COM_MSGSIZE 384
#define	MAXPRINTMSG	8192

#define MAX_NUM_ARGVS	50

#ifndef DEDICATED_ONLY
extern void S_StartMenuMusic( void );
#endif
extern void CL_Disconnect(void);


int		com_argc;
char	*com_argv[MAX_NUM_ARGVS+1];

int		realtime;

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame

cvar_t	*developer;
cvar_t	*timescale;
cvar_t	*fixedtime;
cvar_t	*showtrace;
cvar_t	*dedicated;

int			server_state;

// -jjb- forward reference
char *Com_xsprintf( const char *fmt, ... );

/*--------------------------------------------------------- RCON Redirection */

static int	rd_target;
static char	*rd_buffer;
static int	rd_buffersize;
static void	(*rd_flush)(int target, char *buffer);

void Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush))
{
	if (!target || !buffer || !buffersize || !flush)
		return;
	rd_target = target;
	rd_buffer = buffer;
	rd_buffersize = buffersize;
	rd_flush = flush;  // -jjb-

	*rd_buffer = 0;
}

void Com_EndRedirect (void)
{
	rd_flush(rd_target, rd_buffer);

	rd_target = 0;
	rd_buffer = NULL;
	rd_buffersize = 0;
	rd_flush = NULL;
}

void Com_PrintRedirect( const char* msg )
{
	if ((strlen (msg) + strlen(rd_buffer)) > (rd_buffersize - 1))
	{
		rd_flush( rd_target, rd_buffer );
		*rd_buffer = 0;
	}
	strcat( rd_buffer, msg );
}


/*----------------------------------------------------------- Error Reporting */

/**
 * @brief Error message output for standard errno errors
 *
 */
void
Com_ErrnoMsg( int errnum, const char *msg )
{
	char *errmsg;

	errno = errnum;
	perror( msg ); /* to stderr */
	
	/* Com_xsprintf allocates a sufficient buffer */
	errmsg = Com_xsprintf( "\n%s: %s\n", msg, strerror(errnum) );

	if ( rd_target )
	{
		Com_PrintRedirect( errmsg );
	}
	else
	{
		CON_Print( errmsg );
		Sys_ConsoleOutput( errmsg );
	}

	FS_WriteConsoleLog( errmsg );

	Com_xfree( errmsg );
}

/*------------------------------------------------- Checked Memory Allocation */

void*
Com_xmalloc( size_t size )
{
	void *p;

	p = malloc( size );
	if ( !p )
	{
		Com_ErrnoMsg( errno, "Com_xmalloc" );
		abort();
	}

	return p;
}

void*
Com_xmalloc0( size_t size )
{
	void *p;

	p = calloc( size, sizeof(char) );
	if ( !p )
	{
		Com_ErrnoMsg( errno, "Com_xmalloc0" );
		abort();
	}

	return p;
}

void*
Com_xcalloc( size_t count, size_t elemsize )
{
	void *p;
	p = calloc( count, elemsize );
	if ( !p )
	{
		Com_ErrnoMsg( errno, "Com_xcalloc" );
		abort();
	}

	return p;
}

void*
Com_xrealloc( void *p, size_t newsize )
{
	void *newp;

	newp = realloc( p, newsize );
	if ( !newp )
	{
		Com_ErrnoMsg( errno, "Com_xrealloc" );
		abort();
	}

	return newp;
}

void
Com_xfree( void *p )
{
	free( p );
}


/**
 * @brief String printf implemented using vsnprintf
 */
void Com_sprintf (char *dest, int size, char *fmt, ...)
{
	int     len;
	va_list argptr;
	char    *buf;

	va_start( argptr, fmt );
	len = vsnprintf( dest, size, fmt, argptr );
	va_end( argptr );
	if ( len >= size )
	{
		/* caller did not supply a big enough destination buffer.
		 * the output was truncated without a terminating null
		 */
		dest[size-1] = '\0';
		Com_ErrnoMsg( errno, "Com_sprintf" );
	}
}

/*
 * @brief Alternate string printf which returns an allocated buffer
 *        of at least the size of the generated string.
 */
char*
Com_xsprintf( const char* fmt, ... )
{
	char    *p, *np;
	int     size = 128;
	va_list ap;
	int     len;
	int     errnum;

	p = Com_xmalloc0( size );
	
	va_start( ap, fmt );
	len = vsnprintf( p, size, fmt, ap );
	va_end( ap );
	if ( len < 0 )
	{
		errnum = errno;
		Com_xfree( p );
		Com_ErrnoMsg( errnum, "Com_xsprintf" );
		return NULL;
	}
	if ( len >= size )
	{
		size = len + 1;
		p = Com_xrealloc( p, size );

		va_start( ap, fmt );
		len = vsnprintf( np, size, fmt, ap );
		va_end( ap );
	}

	return p;
}

/**
 * @brief Printf-style output message output
 *
 *
 */
void Com_Printf( char *fmt, ... )
{
	va_list	argptr;
	char*   msg;
	int     msglen;
	int     msgsize;

	msg = Com_xmalloc( COM_MSGSIZE  );

	va_start( argptr, fmt );
	msglen = vsnprintf( msg, COM_MSGSIZE, fmt, argptr ); // -jjb- c99
	va_end( argptr );

	if ( msglen >= COM_MSGSIZE )
	{
		if ( msglen >= MAXPRINTMSG )
		{
			Com_ErrnoMsg( errno, "Com_Printf" );
			Com_xfree( msg );
			return;
		}

		msgsize = msglen + 2;
		msg = Com_xrealloc( msg, msgsize );

		va_start( argptr, fmt );
		msglen = vsnprintf( msg, MAXPRINTMSG, fmt, argptr );
		va_end( argptr );
	}
	if ( msglen >= MAXPRINTMSG )
	{
		Com_ErrnoMsg( 0, "Com_Printf: message too long\n" );
		Com_xfree( msg );
		return;
	}

	if ( rd_target )
	{
		Com_PrintRedirect( msg );
	}
	else
	{
		CON_Print( msg );
		Sys_ConsoleOutput( msg );
	}

	FS_WriteConsoleLog( msg );

	Com_xfree( msg );

}

void Com_DPrintf( char *fmt, ... )
{
// -jjb- redundancy
	if ( developer && developer->integer )
	{
		va_list argptr;
		char    msg[MAXPRINTMSG];

		va_start (argptr,fmt);
		vsnprintf(msg, sizeof(msg), fmt, argptr);
		va_end (argptr);

		Com_Printf ("%s", msg);
	}
}

/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error( int code, char *fmt, ... )
{
	va_list          argptr;
	static char      msg[MAXPRINTMSG];
	static qboolean	recursive = false;
	bool do_reconnect   = false;
	bool do_abort_frame = true;;

	if (recursive)
	{
		Sys_Warn( "\nMultiple errors. Com_Error re-entered\n" );
		return;
	}
	recursive = true;

	va_start (argptr,fmt);
	vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	Sys_Warn( msg );

	switch ( code )
	{

	case ERR_DISCONNECT:
		CL_Drop();
		break;

	case ERR_DROP:
		SV_Shutdown( "\nServer stopped\n", do_reconnect );
		CL_Drop ();
		break;

	default:
		SV_Shutdown ("\nServer fatal crashed\n", do_reconnect );
		CL_Shutdown ();
		do_abort_frame = false; // -jjb- ???
		break;

	}

//	FS_CloseLogFiles(); -jjb-

	recursive = false;
	if ( do_abort_frame )
		longjmp( abortframe, -1 ); // -jjb-

}


/*
=============
Com_Quit

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit (void)
{
	bool do_reconnect = false;

	SV_Shutdown ("Server quit\n", do_reconnect );
	CL_Shutdown ();

// -jjb-
//	FS_CloseLogFiles();

	Sys_Quit ();
}


/*
==================
Com_ServerState
==================
*/
int Com_ServerState (void)
{
	return server_state;
}

/*
==================
Com_SetServerState
==================
*/
void Com_SetServerState (int state)
{
	server_state = state;
}


/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < -128 || c > 127)
	{
		// Com_Printf( "MSG_WriteChar: range error: %i\n", c);
		Com_Error (ERR_FATAL, "MSG_WriteChar: range error");
	}
#endif

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte	*buf;

#ifdef PARANOID
	if (c < 0 || c > 255) {
		//Com_Printf( "MSG_WriteByte: range error: %i\n", c );
		Com_Error (ERR_FATAL, "MSG_WriteByte: range error");
	}
#endif

	buf = SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	short *buf;

	buf  = (short*)SZ_GetSpace( sb , 2 );
	*buf = LittleShort( c );

}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	int *buf;

	buf  = (int*)SZ_GetSpace( sb, 4 );
	*buf = LittleLong( c );

}

void MSG_WriteSizeInt (sizebuf_t *sb, int bytes, int c)
{
	byte		*buf;
	int			i;
	
	// manually construct two's complement encoding with appropriate bit depth
	if (c < 0 && bytes != sizeof(c))
		c += 1<<(8*bytes);
	
	buf = SZ_GetSpace (sb, bytes);
	for (i = 0; i < bytes; i++)
		buf[i] = (byte)((c>>(8*i))&0xff);
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union
	{
		float	f;
		int	l;
	} dat;

	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, char *s)
{
	if (!s)
		SZ_Write (sb, "", 1);
	else
		SZ_Write (sb, s, strlen(s)+1);
}

void MSG_WriteCoord (sizebuf_t *sb, float f)
{
	MSG_WriteSizeInt (sb, coord_bytes, (int)(f*8));
}

void MSG_WritePos (sizebuf_t *sb, vec3_t pos)
{
	MSG_WriteSizeInt (sb, coord_bytes, (int)(pos[0]*8));
	MSG_WriteSizeInt (sb, coord_bytes, (int)(pos[1]*8));
	MSG_WriteSizeInt (sb, coord_bytes, (int)(pos[2]*8));
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, (int)(f*256/360) & 255);
}

void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, ANGLE2SHORT(f));
}


void MSG_WriteDeltaUsercmd (sizebuf_t *buf, usercmd_t *from, usercmd_t *cmd)
{
	int		bits;

//
// send the movement message
//
	bits = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

    MSG_WriteByte (buf, bits);

	if (bits & CM_ANGLE1)
		MSG_WriteShort (buf, cmd->angles[0]);
	if (bits & CM_ANGLE2)
		MSG_WriteShort (buf, cmd->angles[1]);
	if (bits & CM_ANGLE3)
		MSG_WriteShort (buf, cmd->angles[2]);

	if (bits & CM_FORWARD)
		MSG_WriteShort (buf, cmd->forwardmove);
	if (bits & CM_SIDE)
	  	MSG_WriteShort (buf, cmd->sidemove);
	if (bits & CM_UP)
		MSG_WriteShort (buf, cmd->upmove);

 	if (bits & CM_BUTTONS)
	  	MSG_WriteByte (buf, cmd->buttons);
 	if (bits & CM_IMPULSE)
	    MSG_WriteByte (buf, cmd->impulse);

    MSG_WriteByte (buf, cmd->msec);
	MSG_WriteByte (buf, cmd->lightlevel);
}


void MSG_WriteDir (sizebuf_t *sb, vec3_t dir)
{
	int		i, best;
	float	d,x,y,z;

	if (!dir)
	{
		MSG_WriteByte (sb, 0);
		return;
	}

	x = dir[0];
	y = dir[1];
	z = dir[2];
	best = 0;

	d = (x*x) + (y*y) + (z*z);
	// sometimes dir is {0,0,0}
	if ( d == 0.0f )
	{
		MSG_WriteByte( sb, 0 );
		return;
	}

// -jjb- to separate function ?
	// sometimes dir is not normalized
	if ( d < 0.999f || d > 1.001f )
	{
		d = 1.0 / sqrt( d );
		x *= d;
		y *= d;
		z *= d;
	}
	// frequently occurring axial cases, use known best index
	if ( x == 0.0f && y == 0.0f )
	{
		best = ( z >= 0.999f ) ? 5  : 84;
	}
	else if ( x == 0.0f && z == 0.0f )
	{
		best = ( y >= 0.999f ) ? 32 : 104;
	}
	else if ( y == 0.0f && z == 0.0f )
	{
		best = ( x >= 0.999f ) ? 52 : 143;
	}
	else
	{ // general case
		float bestd = 0.0f;
		for ( i = 0 ; i < NUMVERTEXNORMALS ; i++ )
		{ // search for closest match
			d = (x*bytedirs[i][0]) + (y*bytedirs[i][1]) + (z*bytedirs[i][2]);
			if ( d > 0.998f )
			{ // no other entry could be a closer match
				//  0.9679495 is max dot product between anorm.h entries
				best = i;
				break;
			}
			if ( d > bestd )
			{
				bestd = d;
				best = i;
			}
		}
	}

	MSG_WriteByte (sb, best);
}


void MSG_ReadDir (sizebuf_t *sb, vec3_t dir)
{
	int		b;

	b = MSG_ReadByte (sb);
	if (b >= NUMVERTEXNORMALS)
		Com_Error (ERR_DROP, "MSG_ReadDir: out of range");
	VectorCopy (bytedirs[b], dir);
}


/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void MSG_WriteDeltaEntity (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force, qboolean newentity)
{
	int		bits;

	if (!to->number)
		Com_Error (ERR_FATAL, "Unset entity number");
	if (to->number >= MAX_EDICTS)
		Com_Error (ERR_FATAL, "Entity number >= MAX_EDICTS");

// send an update
	bits = 0;

	if (to->number >= 256)
		bits |= U_NUMBER16;		// number8 is implicit otherwise

	if (to->origin[0] != from->origin[0])
		bits |= U_ORIGIN1;
	if (to->origin[1] != from->origin[1])
		bits |= U_ORIGIN2;
	if (to->origin[2] != from->origin[2])
		bits |= U_ORIGIN3;

	if ( to->angles[0] != from->angles[0] )
		bits |= U_ANGLE1;
	if ( to->angles[1] != from->angles[1] )
		bits |= U_ANGLE2;
	if ( to->angles[2] != from->angles[2] )
		bits |= U_ANGLE3;

	if ( to->skinnum != from->skinnum )
	{
		if ((unsigned)to->skinnum < 256)
			bits |= U_SKIN8;
		else if ((unsigned)to->skinnum < 0x10000)
			bits |= U_SKIN16;
		else
			bits |= (U_SKIN8|U_SKIN16);
	}

	if ( to->frame != from->frame )
	{
		if (to->frame < 256)
			bits |= U_FRAME8;
		else
			bits |= U_FRAME16;
	}

	if ( to->effects != from->effects )
	{
		if (to->effects < 256)
			bits |= U_EFFECTS8;
		else if (to->effects < 0x8000)
			bits |= U_EFFECTS16;
		else
			bits |= U_EFFECTS8|U_EFFECTS16;
	}

	if ( to->renderfx != from->renderfx )
	{
		if (to->renderfx < 256)
			bits |= U_RENDERFX8;
		else if (to->renderfx < 0x8000)
			bits |= U_RENDERFX16;
		else
			bits |= U_RENDERFX8|U_RENDERFX16;
	}

	if ( to->solid != from->solid )
		bits |= U_SOLID;

	// event is not delta compressed, just 0 compressed
	if ( to->event  )
		bits |= U_EVENT;

	if ( to->modelindex != from->modelindex )
		bits |= U_MODEL;
	if ( to->modelindex2 != from->modelindex2 )
		bits |= U_MODEL2;
	if ( to->modelindex3 != from->modelindex3 )
		bits |= U_MODEL3;
	if ( to->modelindex4 != from->modelindex4 )
		bits |= U_MODEL4;

	if ( to->sound != from->sound )
		bits |= U_SOUND;

	if (newentity || (to->renderfx & RF_BEAM))
		bits |= U_OLDORIGIN;

	//
	// write the message
	//
	if (!bits && !force)
		return;		// nothing to send!

	//----------

	if (bits & 0xff000000)
		bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x00ff0000)
		bits |= U_MOREBITS2 | U_MOREBITS1;
	else if (bits & 0x0000ff00)
		bits |= U_MOREBITS1;

	MSG_WriteByte (msg,	bits&255 );

	if (bits & 0xff000000)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
		MSG_WriteByte (msg,	(bits>>16)&255 );
		MSG_WriteByte (msg,	(bits>>24)&255 );
	}
	else if (bits & 0x00ff0000)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
		MSG_WriteByte (msg,	(bits>>16)&255 );
	}
	else if (bits & 0x0000ff00)
	{
		MSG_WriteByte (msg,	(bits>>8)&255 );
	}

	//----------

	if (bits & U_NUMBER16)
		MSG_WriteShort (msg, to->number);
	else
		MSG_WriteByte (msg,	to->number);

	if (bits & U_MODEL)
		MSG_WriteByte (msg,	to->modelindex);
	if (bits & U_MODEL2)
		MSG_WriteByte (msg,	to->modelindex2);
	if (bits & U_MODEL3)
		MSG_WriteByte (msg,	to->modelindex3);
	if (bits & U_MODEL4)
		MSG_WriteByte (msg,	to->modelindex4);

	if (bits & U_FRAME8)
		MSG_WriteByte (msg, to->frame);
	if (bits & U_FRAME16)
		MSG_WriteShort (msg, to->frame);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))		//used for laser colors
		MSG_WriteLong (msg, to->skinnum);
	else if (bits & U_SKIN8)
		MSG_WriteByte (msg, to->skinnum);
	else if (bits & U_SKIN16)
		MSG_WriteShort (msg, to->skinnum);


	if ( (bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16) )
		MSG_WriteLong (msg, to->effects);
	else if (bits & U_EFFECTS8)
		MSG_WriteByte (msg, to->effects);
	else if (bits & U_EFFECTS16)
		MSG_WriteShort (msg, to->effects);

	if ( (bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16) )
		MSG_WriteLong (msg, to->renderfx);
	else if (bits & U_RENDERFX8)
		MSG_WriteByte (msg, to->renderfx);
	else if (bits & U_RENDERFX16)
		MSG_WriteShort (msg, to->renderfx);

	if (bits & U_ORIGIN1)
		MSG_WriteCoord (msg, to->origin[0]);
	if (bits & U_ORIGIN2)
		MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ORIGIN3)
		MSG_WriteCoord (msg, to->origin[2]);

	if (bits & U_ANGLE1)
		MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ANGLE2)
		MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ANGLE3)
		MSG_WriteAngle(msg, to->angles[2]);

	if (bits & U_OLDORIGIN)
	{
		MSG_WriteCoord (msg, to->old_origin[0]);
		MSG_WriteCoord (msg, to->old_origin[1]);
		MSG_WriteCoord (msg, to->old_origin[2]);
	}

	if (bits & U_SOUND)
		MSG_WriteByte (msg, to->sound);
	if (bits & U_EVENT)
		MSG_WriteByte (msg, to->event);
	if (bits & U_SOLID)
		MSG_WriteShort (msg, to->solid);
}


//============================================================

//
// reading functions
//

void MSG_BeginReading (sizebuf_t *msg)
{
	msg->readcount = 0;
}

// returns -1 if no more characters are available
int MSG_ReadChar (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount+1 > msg_read->cursize)
		c = -1;
	else
		c = (signed char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;

	return c;
}

int MSG_ReadByte (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount+1 > msg_read->cursize)
		c = -1;
	else
		c = (unsigned char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;

	return c;
}

int MSG_ReadShort (sizebuf_t *msg_read)
{
	int c     = -1;
	int rc    = msg_read->readcount;
	short *pi = (short*)(&msg_read->data[ rc ]);

	rc += 2;
	if ( rc <= msg_read->cursize )
	{
		c = LittleShort( *pi );
		msg_read->readcount = rc;
	}

	return c;
}

int MSG_ReadLong (sizebuf_t *msg_read)
{
	int c   = -1;
	int rc  = msg_read->readcount;
	int *pi = (int*)(&msg_read->data[rc]);

	rc += 4;
	if ( rc <= msg_read->cursize )
	{
		c = LittleLong( *pi );
		msg_read->readcount = rc;
	}

	return c;
}

int MSG_ReadSizeInt (sizebuf_t *msg_read, int bytes)
{
	int c, i;
	
	if (msg_read->readcount+bytes > msg_read->cursize)
		c = -1;
	else
	{
		c = 0;
		for (i = 0; i < bytes; i++)
			c += msg_read->data[msg_read->readcount++] << (8*i);
		// check sign bit and sign-extend to 32 bits if needed
		if (bytes != sizeof(c) && (c & 1<<(8*bytes-1)))
			for (; i < sizeof(c); i++)
				c += 0xff<<(8*i);
	}
	
	return c;
}

float MSG_ReadFloat (sizebuf_t *msg_read)
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;

	if (msg_read->readcount+4 > msg_read->cursize)
		dat.f = -1;
	else
	{
		dat.b[0] =	msg_read->data[msg_read->readcount];
		dat.b[1] =	msg_read->data[msg_read->readcount+1];
		dat.b[2] =	msg_read->data[msg_read->readcount+2];
		dat.b[3] =	msg_read->data[msg_read->readcount+3];
	}
	msg_read->readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

char *MSG_ReadString (sizebuf_t *msg_read)
{
	static char	string[2048];
	int		l,c;

	l = 0;
	do
	{
		// sku - replaced MSG_ReadChar with MSG_ReadByte to avoid
		// potentional vulnerability
		c = MSG_ReadByte (msg_read);
		if (c == -1 || c == 0)
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);

	string[l] = 0;

	return string;
}

char *MSG_ReadStringLine (sizebuf_t *msg_read)
{
	static char	string[2048];
	int		l,c;

	l = 0;
	do
	{
		// sku - replaced MSG_ReadChar with MSG_ReadByte to avoid
		// potentional vulnerability
		c = MSG_ReadByte (msg_read);
		if (c == -1 || c == 0 || c == '\n')
			break;
		string[l] = c;
		l++;
	} while (l < sizeof(string)-1);

	string[l] = 0;

	return string;
}

float MSG_ReadCoord (sizebuf_t *msg_read)
{
	return MSG_ReadSizeInt(msg_read, coord_bytes) * (1.0/8);
}

void MSG_ReadPos (sizebuf_t *msg_read, vec3_t pos)
{
	pos[0] = MSG_ReadSizeInt(msg_read, coord_bytes) * (1.0/8);
	pos[1] = MSG_ReadSizeInt(msg_read, coord_bytes) * (1.0/8);
	pos[2] = MSG_ReadSizeInt(msg_read, coord_bytes) * (1.0/8);
}

float MSG_ReadAngle (sizebuf_t *msg_read)
{
	return MSG_ReadChar(msg_read) * (360.0/256);
}

float MSG_ReadAngle16 (sizebuf_t *msg_read)
{
	return SHORT2ANGLE(MSG_ReadShort(msg_read));
}

void MSG_ReadDeltaUsercmd (sizebuf_t *msg_read, usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = MSG_ReadByte (msg_read);

// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE2)
		move->angles[1] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE3)
		move->angles[2] = MSG_ReadShort (msg_read);

// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = MSG_ReadShort (msg_read);
	if (bits & CM_SIDE)
		move->sidemove = MSG_ReadShort (msg_read);
	if (bits & CM_UP)
		move->upmove = MSG_ReadShort (msg_read);

// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = MSG_ReadByte (msg_read);

	if (bits & CM_IMPULSE)
		move->impulse = MSG_ReadByte (msg_read);

// read time to run command
	move->msec = MSG_ReadByte (msg_read);

// read the light level
	move->lightlevel = MSG_ReadByte (msg_read);
}


void MSG_ReadData (sizebuf_t *msg_read, void *data, int len)
{
	int		i;

	for (i=0 ; i<len ; i++)
		((byte *)data)[i] = MSG_ReadByte (msg_read);
}


//===========================================================================

void SZ_Init (sizebuf_t *buf, byte *data, int length)
{
	memset (buf, 0, sizeof(*buf));
	buf->data = data;
	buf->maxsize = length;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	void	*data;

	if (buf->cursize + length > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Com_Error (ERR_FATAL, "SZ_GetSpace: overflow without allowoverflow set");

		if (length > buf->maxsize)
			Com_Error (ERR_FATAL, "SZ_GetSpace: %i is > full buffer size", length);

		Com_Printf ("SZ_GetSpace: overflow\n"); // -jjb-

		SZ_Clear (buf);
		buf->overflowed = true;
	}

	data = buf->data + buf->cursize;
	buf->cursize += length;

	return data;
}

void SZ_Write (sizebuf_t *buf, void *data, int length)
{
	memcpy (SZ_GetSpace(buf,length),data,length);
}

void SZ_Print (sizebuf_t *buf, char *data)
{
	int		len;

	len = strlen(data)+1;

	if (buf->cursize)
	{
		if (buf->data[buf->cursize-1])
			memcpy ((byte *)SZ_GetSpace(buf, len),data,len); // no trailing 0
		else
			memcpy ((byte *)SZ_GetSpace(buf, len-1)-1,data,len); // write over trailing 0
	}
	else
		memcpy ((byte *)SZ_GetSpace(buf, len),data,len);
}


//============================================================================


/*
================
COM_CheckParm

Returns the position (1 to argc-1) in the program's argument list
where the given parameter apears, or 0 if not present
================
*/
int COM_CheckParm (char *parm)
{
	int		i;

	for (i=1 ; i<com_argc ; i++)
	{
		if (!strcmp (parm,com_argv[i]))
			return i;
	}

	return 0;
}

int COM_Argc (void)
{
	return com_argc;
}

char *COM_Argv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return "";
	return com_argv[arg];
}

void COM_ClearArgv (int arg)
{
	if (arg < 0 || arg >= com_argc || !com_argv[arg])
		return;
	com_argv[arg] = "";
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv (int argc, char **argv)
{
	int		i;

	if (argc > MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "argc > MAX_NUM_ARGVS");
	com_argc = argc;
	for (i=0 ; i<argc ; i++)
	{
		if (!argv[i] || strlen(argv[i]) >= MAX_TOKEN_CHARS )
			com_argv[i] = "";
		else
			com_argv[i] = argv[i];
	}
}

/*
================
COM_AddParm

Adds the given string at the end of the current argument list
================
*/
void COM_AddParm (char *parm)
{
	if (com_argc == MAX_NUM_ARGVS)
		Com_Error (ERR_FATAL, "COM_AddParm: MAX_NUM)ARGS");
	com_argv[com_argc++] = parm;
}


/// just for debugging
int	memsearch (byte *start, int count, int search)
{
	int		i;

	for (i=0 ; i<count ; i++)
		if (start[i] == search)
			return i;
	return -1;
}


char *CopyString (const char *in)
{
	char	*out;

	out = Z_Malloc (strlen(in)+1);
	strcpy (out, in);
	return out;
}



void Info_Print (char *s)
{
	char	key[512];
	char	value[512];
	char	*o;
	int		l, i;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		i = 512;
		while (--i && *s && *s != '\\')
			*o++ = *s++;
		if (*s && *s != '\\')
		{
			Com_Printf ("KEY TOO LONG\n");
			return;
		}

		l = o - key;
		if (l < 20)
		{
			memset (o, ' ', 20-l);
			key[20] = 0;
		}
		else
			*o = 0;
		Com_Printf ("%s", key);

		if (!*s)
		{
			Com_Printf ("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		i = 512;
		while (--i && *s && *s != '\\')
			*o++ = *s++;
		if (*s && *s != '\\')
		{
			Com_Printf ("VALUE TOO LONG\n");
			return;
		}
		*o = 0;

		if (*s)
			s++;
		Com_Printf ("%s\n", value);
	}
}


/*
==============================================================================

						ZONE MEMORY ALLOCATION

just cleared malloc with counters now...

==============================================================================
*/

#define	Z_MAGIC		0x1d1d


typedef struct zhead_s
{
	struct zhead_s	*prev, *next;
	short	magic;
	short	tag;			// for group free
	int		size;
} zhead_t;

zhead_t		z_chain;
int		z_count, z_bytes;

/*
========================
Z_Free
========================
*/
void Z_Free (void *ptr)
{
	zhead_t	*z;

	z = ((zhead_t *)ptr) - 1;

	if (z->magic != Z_MAGIC)
		Com_Error (ERR_FATAL, "Z_Free: bad magic");

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	Com_xfree (z); // -jjb-
}


/*
========================
Z_Stats_f
========================
*/
void Z_Stats_f (void)
{
	Com_Printf ("%i bytes in %i blocks\n", z_bytes, z_count);
}

/*
========================
Z_FreeTags
========================
*/
void Z_FreeTags (int tag)
{
	zhead_t	*z, *next;

	for (z=z_chain.next ; z != &z_chain ; z=next)
	{
		next = z->next;
		if (z->tag == tag)
			Z_Free ((void *)(z+1));
	}
}

/*
========================
Z_TagMalloc
========================
*/
void *Z_TagMalloc (int size, int tag)
{
	zhead_t	*z;

	size = size + sizeof(zhead_t);
#if 0
	z = malloc(size);
	if (!z)
		Com_Error (ERR_FATAL, "Z_Malloc: failed on allocation of %i bytes",size);
	memset (z, 0, size);
#else 
// -jjb-
	z = Com_xmalloc0( size );
#endif
	z_count++;
	z_bytes += size;
	z->magic = Z_MAGIC;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return (void *)(z+1);
}

/*
========================
Z_Malloc
========================
*/
void *Z_Malloc (int size)
{
	return Z_TagMalloc (size, 0);
}

static byte chktbl[1024] = {
0x84, 0x47, 0x51, 0xc1, 0x93, 0x22, 0x21, 0x24, 0x2f, 0x66, 0x60, 0x4d, 0xb0, 0x7c, 0xda,
0x88, 0x54, 0x15, 0x2b, 0xc6, 0x6c, 0x89, 0xc5, 0x9d, 0x48, 0xee, 0xe6, 0x8a, 0xb5, 0xf4,
0xcb, 0xfb, 0xf1, 0x0c, 0x2e, 0xa0, 0xd7, 0xc9, 0x1f, 0xd6, 0x06, 0x9a, 0x09, 0x41, 0x54,
0x67, 0x46, 0xc7, 0x74, 0xe3, 0xc8, 0xb6, 0x5d, 0xa6, 0x36, 0xc4, 0xab, 0x2c, 0x7e, 0x85,
0xa8, 0xa4, 0xa6, 0x4d, 0x96, 0x19, 0x19, 0x9a, 0xcc, 0xd8, 0xac, 0x39, 0x5e, 0x3c, 0xf2,
0xf5, 0x5a, 0x72, 0xe5, 0xa9, 0xd1, 0xb3, 0x23, 0x82, 0x6f, 0x29, 0xcb, 0xd1, 0xcc, 0x71,
0xfb, 0xea, 0x92, 0xeb, 0x1c, 0xca, 0x4c, 0x70, 0xfe, 0x4d, 0xc9, 0x67, 0x43, 0x47, 0x94,
0xb9, 0x47, 0xbc, 0x3f, 0x01, 0xab, 0x7b, 0xa6, 0xe2, 0x76, 0xef, 0x5a, 0x7a, 0x29, 0x0b,
0x51, 0x54, 0x67, 0xd8, 0x1c, 0x14, 0x3e, 0x29, 0xec, 0xe9, 0x2d, 0x48, 0x67, 0xff, 0xed,
0x54, 0x4f, 0x48, 0xc0, 0xaa, 0x61, 0xf7, 0x78, 0x12, 0x03, 0x7a, 0x9e, 0x8b, 0xcf, 0x83,
0x7b, 0xae, 0xca, 0x7b, 0xd9, 0xe9, 0x53, 0x2a, 0xeb, 0xd2, 0xd8, 0xcd, 0xa3, 0x10, 0x25,
0x78, 0x5a, 0xb5, 0x23, 0x06, 0x93, 0xb7, 0x84, 0xd2, 0xbd, 0x96, 0x75, 0xa5, 0x5e, 0xcf,
0x4e, 0xe9, 0x50, 0xa1, 0xe6, 0x9d, 0xb1, 0xe3, 0x85, 0x66, 0x28, 0x4e, 0x43, 0xdc, 0x6e,
0xbb, 0x33, 0x9e, 0xf3, 0x0d, 0x00, 0xc1, 0xcf, 0x67, 0x34, 0x06, 0x7c, 0x71, 0xe3, 0x63,
0xb7, 0xb7, 0xdf, 0x92, 0xc4, 0xc2, 0x25, 0x5c, 0xff, 0xc3, 0x6e, 0xfc, 0xaa, 0x1e, 0x2a,
0x48, 0x11, 0x1c, 0x36, 0x68, 0x78, 0x86, 0x79, 0x30, 0xc3, 0xd6, 0xde, 0xbc, 0x3a, 0x2a,
0x6d, 0x1e, 0x46, 0xdd, 0xe0, 0x80, 0x1e, 0x44, 0x3b, 0x6f, 0xaf, 0x31, 0xda, 0xa2, 0xbd,
0x77, 0x06, 0x56, 0xc0, 0xb7, 0x92, 0x4b, 0x37, 0xc0, 0xfc, 0xc2, 0xd5, 0xfb, 0xa8, 0xda,
0xf5, 0x57, 0xa8, 0x18, 0xc0, 0xdf, 0xe7, 0xaa, 0x2a, 0xe0, 0x7c, 0x6f, 0x77, 0xb1, 0x26,
0xba, 0xf9, 0x2e, 0x1d, 0x16, 0xcb, 0xb8, 0xa2, 0x44, 0xd5, 0x2f, 0x1a, 0x79, 0x74, 0x87,
0x4b, 0x00, 0xc9, 0x4a, 0x3a, 0x65, 0x8f, 0xe6, 0x5d, 0xe5, 0x0a, 0x77, 0xd8, 0x1a, 0x14,
0x41, 0x75, 0xb1, 0xe2, 0x50, 0x2c, 0x93, 0x38, 0x2b, 0x6d, 0xf3, 0xf6, 0xdb, 0x1f, 0xcd,
0xff, 0x14, 0x70, 0xe7, 0x16, 0xe8, 0x3d, 0xf0, 0xe3, 0xbc, 0x5e, 0xb6, 0x3f, 0xcc, 0x81,
0x24, 0x67, 0xf3, 0x97, 0x3b, 0xfe, 0x3a, 0x96, 0x85, 0xdf, 0xe4, 0x6e, 0x3c, 0x85, 0x05,
0x0e, 0xa3, 0x2b, 0x07, 0xc8, 0xbf, 0xe5, 0x13, 0x82, 0x62, 0x08, 0x61, 0x69, 0x4b, 0x47,
0x62, 0x73, 0x44, 0x64, 0x8e, 0xe2, 0x91, 0xa6, 0x9a, 0xb7, 0xe9, 0x04, 0xb6, 0x54, 0x0c,
0xc5, 0xa9, 0x47, 0xa6, 0xc9, 0x08, 0xfe, 0x4e, 0xa6, 0xcc, 0x8a, 0x5b, 0x90, 0x6f, 0x2b,
0x3f, 0xb6, 0x0a, 0x96, 0xc0, 0x78, 0x58, 0x3c, 0x76, 0x6d, 0x94, 0x1a, 0xe4, 0x4e, 0xb8,
0x38, 0xbb, 0xf5, 0xeb, 0x29, 0xd8, 0xb0, 0xf3, 0x15, 0x1e, 0x99, 0x96, 0x3c, 0x5d, 0x63,
0xd5, 0xb1, 0xad, 0x52, 0xb8, 0x55, 0x70, 0x75, 0x3e, 0x1a, 0xd5, 0xda, 0xf6, 0x7a, 0x48,
0x7d, 0x44, 0x41, 0xf9, 0x11, 0xce, 0xd7, 0xca, 0xa5, 0x3d, 0x7a, 0x79, 0x7e, 0x7d, 0x25,
0x1b, 0x77, 0xbc, 0xf7, 0xc7, 0x0f, 0x84, 0x95, 0x10, 0x92, 0x67, 0x15, 0x11, 0x5a, 0x5e,
0x41, 0x66, 0x0f, 0x38, 0x03, 0xb2, 0xf1, 0x5d, 0xf8, 0xab, 0xc0, 0x02, 0x76, 0x84, 0x28,
0xf4, 0x9d, 0x56, 0x46, 0x60, 0x20, 0xdb, 0x68, 0xa7, 0xbb, 0xee, 0xac, 0x15, 0x01, 0x2f,
0x20, 0x09, 0xdb, 0xc0, 0x16, 0xa1, 0x89, 0xf9, 0x94, 0x59, 0x00, 0xc1, 0x76, 0xbf, 0xc1,
0x4d, 0x5d, 0x2d, 0xa9, 0x85, 0x2c, 0xd6, 0xd3, 0x14, 0xcc, 0x02, 0xc3, 0xc2, 0xfa, 0x6b,
0xb7, 0xa6, 0xef, 0xdd, 0x12, 0x26, 0xa4, 0x63, 0xe3, 0x62, 0xbd, 0x56, 0x8a, 0x52, 0x2b,
0xb9, 0xdf, 0x09, 0xbc, 0x0e, 0x97, 0xa9, 0xb0, 0x82, 0x46, 0x08, 0xd5, 0x1a, 0x8e, 0x1b,
0xa7, 0x90, 0x98, 0xb9, 0xbb, 0x3c, 0x17, 0x9a, 0xf2, 0x82, 0xba, 0x64, 0x0a, 0x7f, 0xca,
0x5a, 0x8c, 0x7c, 0xd3, 0x79, 0x09, 0x5b, 0x26, 0xbb, 0xbd, 0x25, 0xdf, 0x3d, 0x6f, 0x9a,
0x8f, 0xee, 0x21, 0x66, 0xb0, 0x8d, 0x84, 0x4c, 0x91, 0x45, 0xd4, 0x77, 0x4f, 0xb3, 0x8c,
0xbc, 0xa8, 0x99, 0xaa, 0x19, 0x53, 0x7c, 0x02, 0x87, 0xbb, 0x0b, 0x7c, 0x1a, 0x2d, 0xdf,
0x48, 0x44, 0x06, 0xd6, 0x7d, 0x0c, 0x2d, 0x35, 0x76, 0xae, 0xc4, 0x5f, 0x71, 0x85, 0x97,
0xc4, 0x3d, 0xef, 0x52, 0xbe, 0x00, 0xe4, 0xcd, 0x49, 0xd1, 0xd1, 0x1c, 0x3c, 0xd0, 0x1c,
0x42, 0xaf, 0xd4, 0xbd, 0x58, 0x34, 0x07, 0x32, 0xee, 0xb9, 0xb5, 0xea, 0xff, 0xd7, 0x8c,
0x0d, 0x2e, 0x2f, 0xaf, 0x87, 0xbb, 0xe6, 0x52, 0x71, 0x22, 0xf5, 0x25, 0x17, 0xa1, 0x82,
0x04, 0xc2, 0x4a, 0xbd, 0x57, 0xc6, 0xab, 0xc8, 0x35, 0x0c, 0x3c, 0xd9, 0xc2, 0x43, 0xdb,
0x27, 0x92, 0xcf, 0xb8, 0x25, 0x60, 0xfa, 0x21, 0x3b, 0x04, 0x52, 0xc8, 0x96, 0xba, 0x74,
0xe3, 0x67, 0x3e, 0x8e, 0x8d, 0x61, 0x90, 0x92, 0x59, 0xb6, 0x1a, 0x1c, 0x5e, 0x21, 0xc1,
0x65, 0xe5, 0xa6, 0x34, 0x05, 0x6f, 0xc5, 0x60, 0xb1, 0x83, 0xc1, 0xd5, 0xd5, 0xed, 0xd9,
0xc7, 0x11, 0x7b, 0x49, 0x7a, 0xf9, 0xf9, 0x84, 0x47, 0x9b, 0xe2, 0xa5, 0x82, 0xe0, 0xc2,
0x88, 0xd0, 0xb2, 0x58, 0x88, 0x7f, 0x45, 0x09, 0x67, 0x74, 0x61, 0xbf, 0xe6, 0x40, 0xe2,
0x9d, 0xc2, 0x47, 0x05, 0x89, 0xed, 0xcb, 0xbb, 0xb7, 0x27, 0xe7, 0xdc, 0x7a, 0xfd, 0xbf,
0xa8, 0xd0, 0xaa, 0x10, 0x39, 0x3c, 0x20, 0xf0, 0xd3, 0x6e, 0xb1, 0x72, 0xf8, 0xe6, 0x0f,
0xef, 0x37, 0xe5, 0x09, 0x33, 0x5a, 0x83, 0x43, 0x80, 0x4f, 0x65, 0x2f, 0x7c, 0x8c, 0x6a,
0xa0, 0x82, 0x0c, 0xd4, 0xd4, 0xfa, 0x81, 0x60, 0x3d, 0xdf, 0x06, 0xf1, 0x5f, 0x08, 0x0d,
0x6d, 0x43, 0xf2, 0xe3, 0x11, 0x7d, 0x80, 0x32, 0xc5, 0xfb, 0xc5, 0xd9, 0x27, 0xec, 0xc6,
0x4e, 0x65, 0x27, 0x76, 0x87, 0xa6, 0xee, 0xee, 0xd7, 0x8b, 0xd1, 0xa0, 0x5c, 0xb0, 0x42,
0x13, 0x0e, 0x95, 0x4a, 0xf2, 0x06, 0xc6, 0x43, 0x33, 0xf4, 0xc7, 0xf8, 0xe7, 0x1f, 0xdd,
0xe4, 0x46, 0x4a, 0x70, 0x39, 0x6c, 0xd0, 0xed, 0xca, 0xbe, 0x60, 0x3b, 0xd1, 0x7b, 0x57,
0x48, 0xe5, 0x3a, 0x79, 0xc1, 0x69, 0x33, 0x53, 0x1b, 0x80, 0xb8, 0x91, 0x7d, 0xb4, 0xf6,
0x17, 0x1a, 0x1d, 0x5a, 0x32, 0xd6, 0xcc, 0x71, 0x29, 0x3f, 0x28, 0xbb, 0xf3, 0x5e, 0x71,
0xb8, 0x43, 0xaf, 0xf8, 0xb9, 0x64, 0xef, 0xc4, 0xa5, 0x6c, 0x08, 0x53, 0xc7, 0x00, 0x10,
0x39, 0x4f, 0xdd, 0xe4, 0xb6, 0x19, 0x27, 0xfb, 0xb8, 0xf5, 0x32, 0x73, 0xe5, 0xcb, 0x32
};

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
byte	COM_BlockSequenceCRCByte (byte *base, int length, int sequence)
{
	int		n;
	byte	*p;
	int		x;
	byte chkb[60 + 4];
	unsigned short crc;


	if (sequence < 0)
		Sys_Error("sequence < 0, this shouldn't happen\n");

	p = chktbl + (sequence % (sizeof(chktbl) - 4));

	if (length > 60)
		length = 60;
	memcpy (chkb, base, length);

	chkb[length] = p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CRC_Block(chkb, length);

	for (x=0, n=0; n<length; n++)
		x += chkb[n];

	crc = (crc ^ x) & 0xff;

	return crc;
}

void Key_Init (void);
void SCR_EndLoadingPlaque (void);

/*
 * target of 'error' console command
 *   for testing error handling 
 */
void Com_Error_f (void)
{
	Com_Error( ERR_FATAL, "%s", Cmd_Argv(1) );
}

/*--------------------------------------------------------------- Random Math */

static float rndtbl1[ 1024 ];
static float *fr1;
static float const *fr1_end = rndtbl1 + sizeof(rndtbl1)/sizeof(float);
static float rndtbl2[ 1024 ];
static float *fr2;
static float const *fr2_end = rndtbl2 + sizeof(rndtbl2)/sizeof(float);

void
Com_random_table_init( void )
{
	float *pr;
	int    count;
	float  rmax;

	rmax = (float)( RAND_MAX );
	pr   = rndtbl1;
	count = sizeof(rndtbl1)/sizeof(float);
	srand( Sys_Milliseconds() ); /* somewhat indeterminate seed */
	while ( count-- )
		*pr++ = ((float)rand() / rmax);

	rmax = (float)( RAND_MAX >> 1 );
	pr   = rndtbl2;
	count = sizeof(rndtbl2)/sizeof(float);
	srand( Sys_Milliseconds() );
	while ( count-- )
		*pr++ = ((float)rand() / rmax) - 1.0f;

	fr1 = rndtbl1;
	fr2 = rndtbl2;
}

/**
 * [0.0f, 1.0f]
 */
float
frand( void )
{
	if ( ++fr1 == fr1_end )
		fr1 = rndtbl1;
	return *fr1;
}

/**
 * [-1.0f, 1.0f]
 */
float
crand( void )
{
	if ( ++fr2 == fr2_end )
		fr2 = rndtbl2;
	return *fr2;
}


/*
=================
Qcommon_Init
=================
*/
void Qcommon_Init (int argc, char **argv)
{
	char	*s;

	if (setjmp (abortframe) )
		Sys_Error ("Error during initialization");

	Com_random_table_init();

	z_chain.next = z_chain.prev = &z_chain;

	// prepare enough of the subsystems to handle
	// cvar and command buffer management
	COM_InitArgv (argc, argv);

	Swap_Init ();
	Cbuf_Init ();

	Cmd_Init ();
	Cvar_Init ();

	Key_Init ();

	// we need to add the early commands twice, because
	// a basedir needs to be set before execing
	// config files, but we want other parms to override
	// the settings of the config files
	Cbuf_AddEarlyCommands (false);
	Cbuf_Execute ();

	FS_InitFilesystem ();

	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_AddText ("exec config.cfg\n");
	Cbuf_AddText ("exec profile.cfg\n");

	Cbuf_AddEarlyCommands (true);
	Cbuf_Execute ();

	//
	// init commands and vars
	//
    Cmd_AddCommand ("z_stats", Z_Stats_f);
    Cmd_AddCommand ("error", Com_Error_f);

	developer = Cvar_Get ("developer", "0", 0);
	timescale = Cvar_Get ("timescale", "1", 0);
	fixedtime = Cvar_Get ("fixedtime", "0", 0);
	showtrace = Cvar_Get ("showtrace", "0", 0);
#ifdef DEDICATED_ONLY
	dedicated = Cvar_Get ("dedicated", "1", CVAR_NOSET);
#else
	dedicated = Cvar_Get ("dedicated", "0", CVAR_LATCH);
#endif

	s = va("%s %s %s %s", VERSION, CPUSTRING, __DATE__, BUILDSTRING);
	Cvar_Get ("version", s, CVAR_SERVERINFO|CVAR_NOSET);

	if ( dedicated->integer )
		Cmd_AddCommand ("quit", Com_Quit);

	Sys_Init ();

	NET_Init ();
	Netchan_Init ();

	SV_Init ();

	dedicated->modified = false;

	if(!dedicated->value)
		CL_Init ();

	// add + commands from command line
	if (!Cbuf_AddLateCommands ())
	{	// if the user didn't give any commands, run default action
		if (!dedicated->value)
			Cbuf_AddText("menu_main\n");
		else
			Cbuf_AddText ("dedicated_start\n");
		Cbuf_Execute ();
	}
	else
	{	// the user asked for something explicit
		// so drop the loading plaque
		SCR_EndLoadingPlaque ();
	}

	//drop console, to get it into menu
	SCR_EndLoadingPlaque ();
	// clear any lines of console text
	Cbuf_AddText ("clear\n");
#ifndef DEDICATED_ONLY
	//play music
	if (!dedicated->value)
		S_StartMenuMusic();
#endif
	Com_Printf ("======== Alien Arena Initialized ========\n\n");
}

/*
=================
Qcommon_Frame
=================
*/
void Qcommon_Frame (int msec)
{
	float fmsec;

	/*
	 * for events that require aborting the current frame
	 */
	if ( setjmp( abortframe ) )
	{		
		return;
	}

// -jjb- new perf log?

	/* for testing */
	fmsec = (float)msec;
	if ( fixedtime->value >= 1.0f )
	{
		fmsec = fixedtime->value;
	}
	else if ( timescale->value > 0.0f )
	{
		fmsec *= timescale->value;
	}
	msec = (int)ceilf( fmsec );
	if ( msec < 1 )
		msec = 1;


#if defined UNIX_VARIANT

	/* all console input */
	for (;;)
	{
		char *cmd;

		cmd = Sys_ConsoleInput();
		if ( cmd )
		{
			Cbuf_AddText( cmd );
			Cbuf_AddText( "\n" );
		}
		else
		{
			break;
		}
	};

#else

	// Get input from dedicated server console
	if ( dedicated->integer)
	{
		char	*cmd;

		cmd = Sys_GetCommand();
		if (cmd){
			Cbuf_AddText(cmd);
			Cbuf_AddText("\n");
		}
	}
#endif

	Cbuf_Execute ();

	SV_Frame( msec );

#if defined WIN32_VARIANT
	/*
	 * switch to dedicated server mode
	 * see Host Server menu
	 */
	if(dedicated->modified) {
		dedicated->modified = false;
		if ( dedicated->integer ) {
			//shutdown the client
			CL_Shutdown();
		}
	}
#endif

	CL_Frame( msec );

}

/*
=================
Qcommon_Shutdown
=================
*/
void Qcommon_Shutdown (void)
{
}

/*-------------------------------------------------------- ZLib Decompression */

#ifdef HAVE_ZLIB
//This compression code was originally from the Lua branch. More of it will
//eventually be merged when packet compression is added, but currently we 
//are only decompressing zlib data and only need it on the client side.

//Following function ripped off from R1Q2, then modified to use sizebufs
int ZLibDecompress (sizebuf_t *in, sizebuf_t *out, int wbits) {
	z_stream zs;
	char *err_summary;
	int result;

	memset (&zs, 0, sizeof(zs));


	zs.next_in = in->data + in->readcount;
	zs.avail_in = 0;

	zs.next_out = out->data + out->cursize;
	zs.avail_out = out->maxsize - out->cursize;

	result = inflateInit2(&zs, wbits);
	if (result != Z_OK)
	{
		switch (result)
		{
			default: err_summary = "unknown"; break;
			case Z_MEM_ERROR: err_summary = "system memory exhausted"; break;
			case Z_STREAM_ERROR: err_summary = "invalid z_stream"; break;
			case Z_VERSION_ERROR: err_summary = "incompatible zlib version"; break;
		}
		Com_Printf ("ZLib data error! Error %d on inflateInit.\nMessage: %s\n", result, zs.msg);
		return 0;
	}

	zs.avail_in = in->cursize;

	result = inflate(&zs, Z_FINISH);
	if (result != Z_STREAM_END)
	{
	    switch (result)
		{
			default: err_summary = "unknown"; break;
			case Z_STREAM_END: err_summary = "premature end of stream"; break;
			case Z_NEED_DICT: err_summary = "missing preset dictionary"; break;
			case Z_STREAM_ERROR: err_summary = "invalid z_stream"; break;
			case Z_DATA_ERROR: err_summary = "corrupted data"; break;
			case Z_BUF_ERROR: err_summary = "too much compressed data"; break;
			case Z_OK: err_summary = "shouldn't happen with Z_FINISH"; break;
		}
		Com_Printf ("ZLib data error! Error %d (%s) on inflate.\nMessage: %s\n", result, err_summary, zs.msg);
		return 0;
	}

	result = inflateEnd(&zs);
	if (result != Z_OK)
	{
		switch (result)
		{
			default: err_summary = "unknown"; break;
			case Z_STREAM_ERROR: err_summary = "invalid z_stream"; break;
			case Z_DATA_ERROR: err_summary = "prematurely freed z_stream"; break;
		}
		Com_Printf ("ZLib data error! Error %d (%s) on inflateEnd.\nMessage: %s\n", result, err_summary, zs.msg);
		return 0;
	}

	return zs.total_out;
}
#endif //HAVE_ZLIB

void qdecompress (sizebuf_t *src, sizebuf_t *dst, int type){
	int newsize = 0;

	switch (type) {
#ifdef HAVE_ZLIB
		case compression_zlib_raw:
			//raw DEFLATE data, no header/trailer
			newsize = ZLibDecompress (src, dst, -15);
			break;
#endif
#if 0 
        case compression_lzo:
            newsize = LzoDecompress (src, dst);
            break;
#endif
#ifdef HAVE_ZLIB
		case compression_zlib_header: 
			//automatically detect gzip/zlib header/trailer
			newsize = ZLibDecompress (src, dst, 47); 
			break;
#endif
	} 

	if ( newsize == 0)
		dst->cursize = 0;
	else
		dst->cursize += newsize;
	
}



