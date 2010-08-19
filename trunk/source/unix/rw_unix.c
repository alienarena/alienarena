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

#include "../client/client.h"

extern int mx, my;
extern qboolean mouse_active;
extern cursor_t cursor;
static qboolean	mouse_avail;
static int old_mouse_x, old_mouse_y;

static qboolean	mlooking;

cvar_t	*m_filter;
cvar_t	*in_mouse;
cvar_t	*in_dgamouse;
cvar_t	*in_joystick;
cvar_t	*m_accel;

void IN_MLookDown (void)
{
	mlooking = true;
}

void IN_MLookUp (void)
{
	mlooking = false;
	if (!freelook->integer && lookspring->integer)
	IN_CenterView ();
}

void IN_Init(void)
{

	// mouse variables
	m_filter = Cvar_Get ("m_filter", "0", 0);
	in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	in_dgamouse = Cvar_Get ("in_dgamouse", "1", CVAR_ARCHIVE);
	in_joystick     = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);

	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	mx = my = 0.0;
	mouse_avail = true;

	// Knightmare- added Psychospaz's menu mouse support
	refreshCursorMenu();
	refreshCursorLink();
}

void IN_Shutdown(void)
{
	if (!mouse_avail)
		return;

	IN_Activate(false);

	mouse_avail = false;

	Cmd_RemoveCommand ("+mlook");
	Cmd_RemoveCommand ("-mlook");
}


/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
}

/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd)
{
	if (!mouse_avail)
		return;

	if (m_filter->integer)
	{
		mx = (mx + old_mouse_x) * 0.5;
		my = (my + old_mouse_y) * 0.5;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	//now to set the menu cursor
	if (cls.key_dest == key_menu)
	{
		cursor.oldx = cursor.x;
		cursor.oldy = cursor.y;

		if ( mouse_active )
		{
			cursor.x += mx * menu_sensitivity->value/10;
			cursor.y += my * menu_sensitivity->value/10;
			mx = my = 0;
		}
		else
		{
			cursor.x = mx;
			cursor.y = my;
		}

		if (cursor.x!=cursor.oldx || cursor.y!=cursor.oldy)
			cursor.mouseaction = true;

		if (cursor.x < 0)
			cursor.x = 0;
		else if (cursor.x > viddef.width)
			cursor.x = viddef.width;
		if (cursor.y < 0)
			cursor.y = 0;
		else if (cursor.y > viddef.height)
		       	cursor.y = viddef.height;
		M_Think_MouseCursor();
		return;
	}

	if (cls.key_dest != key_console)
	{ // game mouse. don't do mouse movement when in console

		if ( m_smoothing->value )
		{   // reduce sensitivity when frames per sec is below maximum setting
			// cvar mouse sensitivity * ( current measured fps / cvar set maximum fps )
			float adjustment;
			extern cvar_t* cl_maxfps;
			adjustment = sensitivity->value / (cls.frametime * cl_maxfps->value);
			mx *= adjustment;
			my *= adjustment;
		}
		else
		{
			mx *= sensitivity->value;
			my *= sensitivity->value;
		}

		// add mouse X/Y movement to cmd
		if ((in_strafe.state & 1) || (lookstrafe->integer && mlooking))
			cmd->sidemove += m_side->value * mx;
		else
			cl.viewangles[YAW] -= m_yaw->value * mx;

		//if ((mlooking || freelook->integer) && !(in_strafe.state & 1))
		if ((mlooking || freelook->value) && !(in_strafe.state & 1))
			cl.viewangles[PITCH] += m_pitch->value * my;
		else
			cmd->forwardmove -= m_forward->value * my;

		mx = my = 0;
	}

}

void IN_Frame (void)
{
	if (!mouse_avail)
		return;

	if ( !cl.refresh_prepped || cls.key_dest == key_console || cls.key_dest == key_menu)
		IN_Activate(false);
	else
		IN_Activate(true);

}

