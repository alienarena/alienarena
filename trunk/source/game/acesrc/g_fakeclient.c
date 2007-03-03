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

#ifdef __linux__
#include "../g_local.h"
#else
#include "..\g_local.h"
#endif
#include "acebot.h"
#include "g_fakeclient.h"



//===============================================================
//
//				FAKECLIENTS
//
//===============================================================


//==========================================
//  G_FindFakeClientbyState
// 
//==========================================
fakeclient_t *G_FindFakeClientbyState (int state)
{
	int i;
	fakeclient_t *fakeclient;
	
	for (i = 0; i < MAX_BOTS+1; i++)
	{
		fakeclient = fakeClients + i + 1;
		if (fakeclient->state == state)	//found
			return fakeclient;
	}
	return NULL;
}


//==========================================
// G_FindFakeClientbyEdict
// Only tracks fakeClients in use!
//==========================================
fakeclient_t *G_FindFakeClientbyEdict (edict_t *ent)
{
	int i;
	fakeclient_t *fakeclient;

	if (!(ent->svflags & SVF_FAKECLIENT))
		return NULL;

	for (i = 0; i < MAX_BOTS+1; i++)
	{
		fakeclient = fakeClients + i + 1;
		if (ent == fakeclient->ent && fakeclient->state == FAKECLIENT_STATE_INUSE )
			return fakeclient;
	}
	return NULL;
}


//==========================================
// G_FakeClientEmptyTrash
// Make sure the fakeClients list is sane
//==========================================
void G_FakeClientEmptyTrash( void )
{
	fakeclient_t *fakeclient;
	int i;

	for (i = 0; i < MAX_BOTS+1; i++)
	{
		fakeclient = fakeClients + i + 1;
		if (fakeclient->state < FAKECLIENT_STATE_INUSE )
		{
			if (fakeclient->state == FAKECLIENT_STATE_CONNECTED && fakeclient->ent)
			{
				//trap_DropClient(fakeclient->ent);
			}

			fakeclient->state = FAKECLIENT_STATE_FREE;
			fakeclient->ent = NULL;
			fakeclient->respawn = NULL;
		}
	}
}

//==========================================
// G_FakeClientDisconnect
// called from ClientDisconnect
//==========================================
void G_FakeClientDisconnect (edict_t *ent)
{
	fakeclient_t *fakeclient;

	if ( !(ent->svflags & SVF_FAKECLIENT) )
		return;

	fakeclient = G_FindFakeClientbyEdict (ent);
	if (!fakeclient){
		return;
	}

	if ( fakeclient->state == FAKECLIENT_STATE_INUSE )
	{
		fakeclient->state = FAKECLIENT_STATE_FREE;
		fakeclient->ent->svflags &= ~SVF_FAKECLIENT;
		fakeclient->ent = NULL;
		fakeclient->respawn = NULL;
		return;
	}

}


//==========================================
// G_FakeClientBeginConnection
// called from ClientConnect
//==========================================
qboolean G_FakeClientBeginConnection (edict_t *ent)
{
	fakeclient_t *fakeclient;

	fakeclient = G_FindFakeClientbyState(FAKECLIENT_STATE_FREE);
	if (!fakeclient)
	{		
		return false;
	}

	//init
	fakeclient->ent = ent;
	fakeclient->respawn = NULL;
	fakeclient->state = FAKECLIENT_STATE_CONNECTING;
	return true;
}


//==========================================
// G_SpawnFakeClient
// returns the fakeclient connected as spectator
//==========================================
fakeclient_t *G_SpawnFakeClient ( char *fakeUserinfo, char *fakeIP )
{
	edict_t		*ent;
	fakeclient_t	*fakeClient;
	int			count=0;
	int			i;

	G_FakeClientEmptyTrash();

	//count spawned fakeClients
	for (i = 0; i < game.maxclients+1; i++)
	{
		ent = g_edicts + i + 1;
		if (ent->svflags & SVF_FAKECLIENT)
			count++;
	}
	ent = NULL;

	//always keep a free server slot for a player
	if (count+2 > game.maxclients)//|| count+1>sv_maxbots->integer)
	{
		return NULL;
	}

	gi.FakeClientConnect (fakeUserinfo, fakeIP);

	//find our client
	fakeClient = G_FindFakeClientbyState(FAKECLIENT_STATE_CONNECTING);
	if (!fakeClient) {
		G_FakeClientEmptyTrash();
		return NULL;
	}

	fakeClient->state = FAKECLIENT_STATE_CONNECTED;
	return fakeClient;
}

//==========================================
// G_FakeClientStartAsSpectator
// Only makes effect when connecting
//==========================================
qboolean G_FakeClientStartAsSpectator (edict_t *ent)
{
	fakeclient_t	*fakeClient;
	
	if ( !(ent->svflags & SVF_FAKECLIENT) )
		return false;
	
	//put as spectator only when it's connecting
	fakeClient = G_FindFakeClientbyState (FAKECLIENT_STATE_CONNECTING);
	if (!fakeClient || fakeClient->ent != ent)
		return false;
	
	// start as 'observer'
	ent->movetype = MOVETYPE_NOCLIP;
	ent->solid = SOLID_NOT;
	ent->svflags |= SVF_NOCLIENT;
	ent->dmteam = NO_TEAM;
	ent->client->ps.gunindex = 0;
	gi.linkentity (ent);
	
	return true;
}

//==========================================
// G_FakeClientRespawn
// called at finishing standard respawn
//==========================================
void G_FakeClientRespawn (edict_t *ent)
{
	fakeclient_t	*fakeclient;

	if ( !(ent->svflags & SVF_FAKECLIENT) )
		return;

	fakeclient = G_FindFakeClientbyEdict (ent);

	if (fakeclient && fakeclient->respawn)
		fakeclient->respawn(ent);
}

