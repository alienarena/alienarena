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

//===============================================================
//
//				FAKECLIENTS
//
//===============================================================

#define MAX_FAKECLIENTS 64

enum {
	FAKECLIENT_STATE_FREE = 0,			
	FAKECLIENT_STATE_CONNECTING,
	FAKECLIENT_STATE_CONNECTED,
	FAKECLIENT_STATE_INUSE
};

typedef struct
{
	int			state;
	edict_t		*ent;
	void		(*respawn)(edict_t *self);	//optional
} fakeclient_t;

fakeclient_t fakeClients[MAX_FAKECLIENTS];

fakeclient_t *G_SpawnFakeClient ( char *fakeUserinfo, char *fakeIP );

