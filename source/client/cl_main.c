/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2011 COR Entertainment, LLC.

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
// cl_main.c  -- client main loop

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#if defined WIN32_VARIANT
#include <winsock.h>
#endif

#if defined UNIX_VARIANT
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#endif

#if defined HAVE_PUTENV && !defined HAVE__PUTENV
#define _putenv putenv
#endif

/*
 * Temporary test data collection for packet rate limiting
 * and framerate_cap jitter experiment
 */
#define PKT_TEST 1

#if PKT_TEST
cvar_t *cl_packet_test;
cvar_t *cl_frcjitter;
#endif

cvar_t	*freelook;

cvar_t	*adr0;
cvar_t	*adr1;
cvar_t	*adr2;
cvar_t	*adr3;
cvar_t	*adr4;
cvar_t	*adr5;
cvar_t	*adr6;
cvar_t	*adr7;
cvar_t	*adr8;

cvar_t	*cl_stereo_separation;
cvar_t	*cl_stereo;

cvar_t	*rcon_client_password;
cvar_t	*rcon_address;

cvar_t	*cl_noskins;
cvar_t	*cl_autoskins;
cvar_t	*cl_footsteps;
cvar_t	*cl_timeout;
cvar_t	*cl_predict;
cvar_t	*cl_maxfps;
cvar_t	*cl_gun;
cvar_t  *cl_brass;
cvar_t  *cl_showPlayerNames;
cvar_t	*cl_playtaunts;
cvar_t	*cl_centerprint;
cvar_t	*cl_precachecustom;

cvar_t	*cl_paindist;
cvar_t	*cl_explosiondist;

cvar_t	*cl_add_particles;
cvar_t	*cl_add_lights;
cvar_t	*cl_add_entities;
cvar_t	*cl_add_blend;

cvar_t	*cl_shownet;
cvar_t	*cl_showmiss;
cvar_t	*cl_showclamp;

cvar_t	*cl_paused;
cvar_t	*cl_timedemo;

cvar_t	*lookspring;
cvar_t	*lookstrafe;
cvar_t	*sensitivity;
cvar_t	*menu_sensitivity;

cvar_t	*m_smoothing;
cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;

//
// userinfo
//
cvar_t	*info_password;
cvar_t	*info_spectator;
cvar_t	*name;
cvar_t	*password;
cvar_t	*pw_hashed;
cvar_t	*skin;
cvar_t	*rate;
cvar_t	*fov;
cvar_t	*msg;
cvar_t	*hand;
cvar_t	*gender;
cvar_t	*gender_auto;

cvar_t	*cl_vwep;
cvar_t	*cl_vehicle_huds;

cvar_t  *background_music;
cvar_t	*background_music_vol;

cvar_t	*scriptsloaded;

//master server
cvar_t  *cl_master;
cvar_t  *cl_master2;

//custom huds
cvar_t	*cl_hudimage1;
cvar_t	*cl_hudimage2;

//health aura
cvar_t	*cl_healthaura;

//blood
cvar_t  *cl_noblood;

//beam color for disruptor
cvar_t  *cl_disbeamclr;

//Stats
cvar_t  *cl_stats_server;

//latest version of the game available
cvar_t	*cl_latest_game_version;
cvar_t	*cl_latest_game_version_url;

client_static_t	cls;
client_state_t	cl;

centity_t		cl_entities[MAX_EDICTS];

entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

extern	cvar_t *allow_download;
extern	cvar_t *allow_download_players;
extern	cvar_t *allow_download_models;
extern	cvar_t *allow_download_sounds;
extern	cvar_t *allow_download_maps;

extern	char map_music[128];

extern void RS_FreeAllScripts(void);

typedef struct _PLAYERINFO {
	char playername[PLAYERNAME_SIZE];
	int ping;
	int score;
} PLAYERINFO;

typedef struct _SERVERINFO {
	char ip[32];
	int port;
	char szHostName[256];
	char szMapName[256];
	int curClients;
	int maxClients;
	int	ping;
	PLAYERINFO players[64];
	int temporary;
} SERVERINFO;

SERVERINFO servers[64];
int numServers = 0;
extern unsigned int starttime;

static size_t szr; // just for unused result warnings

//
// Fonts
//
FNT_auto_t			CL_gameFont;
static struct FNT_auto_s	_CL_gameFont;
FNT_auto_t			CL_centerFont;
static struct FNT_auto_s	_CL_centerFont;
FNT_auto_t			CL_consoleFont;
static struct FNT_auto_s	_CL_consoleFont;


//======================================================================


/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage (void)
{
	int		len, swlen;

	// the first eight bytes are just packet sequencing stuff
	len = net_message.cursize-8;
	swlen = LittleLong(len);
	if (!swlen)
		return;

	szr = fwrite (&swlen, 4, 1, cls.demofile);
	szr = fwrite (net_message.data+8,	len, 1, cls.demofile);
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	int		len;

	if (!cls.demorecording)
	{
		Com_Printf ("Not recording a demo.\n");
		return;
	}

// finish up
	len = -1;
	szr = fwrite (&len, 4, 1, cls.demofile);
	fclose (cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	Com_Printf ("Stopped demo.\n");
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void CL_Record_f (void)
{
	char	name[MAX_OSPATH];
	char	buf_data[MAX_MSGLEN];
	sizebuf_t	buf;
	int		i;
	int		len;
	entity_state_t	*ent;
	entity_state_t	nullstate;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("record <demoname>\n");
		return;
	}

	if (cls.demorecording)
	{
		Com_Printf ("Already recording.\n");
		return;
	}

	if (cls.state != ca_active)
	{
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	Com_sprintf (name, sizeof(name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("recording to %s.\n", name);
	FS_CreatePath (name);
	cls.demofile = fopen (name, "wb");
	if (!cls.demofile)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}
	cls.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.demowaiting = true;

	//
	// write out messages to hold the startup information
	//
	SZ_Init (&buf, (byte *)buf_data, sizeof(buf_data));
	SZ_SetName ( &buf, "CL_Record_f", false );

	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, 0x10000 + cl.servercount);
	MSG_WriteByte (&buf, 1);	// demos are always attract loops
	MSG_WriteString (&buf, cl.gamedir);
	MSG_WriteShort (&buf, cl.playernum);

	MSG_WriteString (&buf, cl.configstrings[CS_NAME]);

	// configstrings
	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++)
	{
		if (cl.configstrings[i][0])
		{
			if (buf.cursize + strlen (cl.configstrings[i]) + 32 > buf.maxsize)
			{	// write it out
				len = LittleLong (buf.cursize);
				szr = fwrite (&len, 4, 1, cls.demofile);
				szr = fwrite (buf.data, buf.cursize, 1, cls.demofile);
				buf.cursize = 0;
			}

			MSG_WriteByte (&buf, svc_configstring);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, cl.configstrings[i]);
		}

	}

	// baselines
	memset (&nullstate, 0, sizeof(nullstate));
	for (i=0; i<MAX_EDICTS ; i++)
	{
		ent = &cl_entities[i].baseline;
		if (!ent->modelindex)
			continue;

		if (buf.cursize + 64 > buf.maxsize)
		{	// write it out
			len = LittleLong (buf.cursize);
			szr = fwrite (&len, 4, 1, cls.demofile);
			szr = fwrite (buf.data, buf.cursize, 1, cls.demofile);
			buf.cursize = 0;
		}

		MSG_WriteByte (&buf, svc_spawnbaseline);
		MSG_WriteDeltaEntity (&nullstate, &cl_entities[i].baseline, &buf, true, true);
	}

	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, "precache\n");

	// write it to the demo file

	len = LittleLong (buf.cursize);
	szr = fwrite (&len, 4, 1, cls.demofile);
	szr = fwrite (buf.data, buf.cursize, 1, cls.demofile);

	// the rest of the demo file will be individual frames
}

//======================================================================

/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void)
{
	char	*cmd;

	cmd = Cmd_Argv(0);
	if (cls.state <= ca_connected || *cmd == '-' || *cmd == '+')
	{
		Com_Printf ("Unknown command \"%s\"\n", cmd);
		return;
	}

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, cmd);
	if (Cmd_Argc() > 1)
	{
		SZ_Print (&cls.netchan.message, " ");
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}
}

void CL_Setenv_f( void )
{
	int argc = Cmd_Argc();

	if ( argc > 2 )
	{
		char buffer[1000];
		int i;

		strcpy( buffer, Cmd_Argv(1) );
		strcat( buffer, "=" );

		for ( i = 2; i < argc; i++ )
		{
			strcat( buffer, Cmd_Argv( i ) );
			strcat( buffer, " " );
		}

		_putenv( buffer );
	}
	else if ( argc == 2 )
	{
		char *env = getenv( Cmd_Argv(1) );

		if ( env )
		{
			Com_Printf( "%s=%s\n", Cmd_Argv(1), env );
		}
		else
		{
			Com_Printf( "%s undefined\n", Cmd_Argv(1), env );
		}
	}
}


/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f (void)
{
	if (cls.state != ca_connected && cls.state != ca_active)
	{
		Com_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}
}


/*
==================
CL_Pause_f
==================
*/
void CL_Pause_f (void)
{
	// never pause in multiplayer
	if (Cvar_VariableValue ("maxclients") > 1 || !Com_ServerState ())
	{
		Cvar_SetValue ("paused", 0);
		return;
	}

	Cvar_SetValue ("paused", !cl_paused->value);
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)
{
	S_StopAllSounds ();
	CL_Disconnect ();
	Com_Quit ();
}

/*
================
CL_Drop

Called after an ERR_DROP was thrown
================
*/
void CL_Drop (void)
{
	if (cls.state == ca_uninitialized)
		return;
	if (cls.state != ca_disconnected)
		CL_Disconnect ();

	// drop loading plaque unless this is the initial game start
	if (cls.disable_servercount != -1)
		SCR_EndLoadingPlaque ();	// get rid of loading plaque
}


/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and
connect.
======================
*/
void CL_SendConnectPacket (void)
{
	netadr_t	adr;
	int		port;

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Com_Printf ("Bad server address\n");
		cls.connect_time = 0;
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	port = Cvar_VariableValue ("qport");
	userinfo_modified = false;

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo() );
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend (void)
{
	netadr_t	adr;

	// if the local server is running and we aren't
	// then connect
	if (cls.state == ca_disconnected && Com_ServerState() )
	{
		cls.state = ca_connecting;
		strncpy (cls.servername, "localhost", sizeof(cls.servername)-1);
		// we don't need a challenge on the localhost
		CL_SendConnectPacket ();
		return;
//		cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != ca_connecting)
		return;

	if (cls.realtime - cls.connect_time < 3000)
		return;

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Com_Printf ("Bad server address\n");
		cls.state = ca_disconnected;
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	cls.connect_time = cls.realtime;	// for retransmit requests

	Com_Printf ("Connecting to %s...\n", cls.servername);

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "getchallenge\n");
}


/*
================
CL_Connect_f

================
*/
void CL_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: connect <server>\n");
		return;
	}

	if (Com_ServerState ())
	{	// if running a local server, kill it and reissue
		SV_Shutdown (va("Server quit\n", msg), false);
	}
	else
	{
		CL_Disconnect ();
	}

	server = Cmd_Argv (1);

	NET_Config (true);		// allow remote

	CL_Disconnect ();

	cls.state = ca_connecting;
	strncpy (cls.servername, server, sizeof(cls.servername)-1);
	cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately

	STATS_RequestVerification();
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
{
	char		message[1024];
	sizebuf_t	buffer;
	int		i;
	netadr_t	to;

	if ( !(rcon_client_password->string && rcon_client_password->string[0]) && Cmd_Argc() < 3)
	{
		Com_Printf ("You must set 'rcon_password' before issuing an rcon command.\n");
		return;
	}

	NET_Config (true);		// allow remote

	SZ_Init( &buffer, (byte *) message, 1024 );
	buffer.allowoverflow = true;
	SZ_SetName( &buffer, "RCon buffer", false );

	SZ_Print (&buffer, "\xff\xff\xff\xffrcon ");
	if ( rcon_client_password->string && rcon_client_password->string[0] )
	{
		SZ_Print (&buffer, "\"");
		SZ_Print (&buffer, rcon_client_password->string);
		SZ_Print (&buffer, "\" ");
	}

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		SZ_Print (&buffer, "\"");
		SZ_Print (&buffer, Cmd_Argv(i));
		SZ_Print (&buffer, "\" ");
	}

	if ( buffer.overflowed )
	{
		Com_Printf ("Rcon command too long\n");
		return;
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address->string))
		{
			Com_Printf ("You must either be connected, or set the 'rcon_address' cvar to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rcon_address->string, &to);
		if (to.port == 0)
			to.port = BigShort (PORT_SERVER);
	}

	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, to);
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	S_StopAllSounds ();
	CL_ClearEffects ();
	CL_ClearTEnts ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));
	memset (cl_entities, 0, sizeof(cl_entities));

	SZ_Clear (&cls.netchan.message);

}

/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	byte	final[32];
	int repeat;

	if (cls.state == ca_disconnected)
		return;

	if (cl_timedemo && cl_timedemo->value)
	{
		int	time;

		time = Sys_Milliseconds () - cl.timedemo_start;
		if (time > 0)
			Com_Printf ("%i frames, %3.1f seconds: %3.1f fps\n", cl.timedemo_frames,
			time/1000.0, cl.timedemo_frames*1000.0 / time);
	}

	VectorClear (cl.refdef.blend);
	R_SetPalette(NULL);

	M_ForceMenuOff ();

	cls.connect_time = 0;

	if (cls.demorecording)
		CL_Stop_f ();

	// send a disconnect message to the server
	final[0] = clc_stringcmd;
	strcpy ((char *)final+1, "disconnect");
	for ( repeat = 3 ; repeat-- ;)
		Netchan_Transmit( &cls.netchan, (int)strlen( (char*)final ), final );

	CL_ClearState ();

	// stop download
	if(cls.download){
		if(cls.downloadhttp)  // clean up http downloads
			CL_HttpDownloadCleanup();
		else  // or just stop legacy ones
			fclose(cls.download);
		cls.download = NULL;
	}

	cls.state = ca_disconnected;

/*
** From Max's timedemo benchmarks:
**	For automated test runs using demos, can exit here by calling CL_Quit_f()
*/

}

void CL_Disconnect_f (void)
{
	Com_Error (ERR_DROP, "Disconnected from server");
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f (void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	SCR_BeginLoadingPlaque ();
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Com_Printf ("\nChanging map...\n");
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	S_StopAllSounds ();
	if (cls.state == ca_connected) {
		Com_Printf ("reconnecting...\n");
		cls.state = ca_connected;
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		return;
	}

	if (*cls.servername) {
		if (cls.state >= ca_connected) {
			CL_Disconnect();
			cls.connect_time = cls.realtime - 1500;
		} else
			cls.connect_time = -99999; // fire immediately

		cls.state = ca_connecting;
		Com_Printf ("reconnecting...\n");
	}
}

/*
=================
CL_ParseStatusMessage

Handle a reply from a status request
=================
*/
void CL_ParseFullStatusMessage (void)
{
	char	*s;

	s = MSG_ReadString(&net_message);

	Com_Printf ("TESTING!!!!%s\n", s);
	M_AddToServerList (net_from, s);
}

/*
===================
GetServerList

Get a list of servers from the master
===================
*/
void GetServerList (void) {

	char *requeststring;
	netadr_t adr;

	// this function may block => sound channels might not be updated
	// while it does so => prevent 'looping sound effect' while waiting
	// -JR / 20050731 / 2
	S_StopAllSounds ();

	requeststring = va( "query" );

	// send a broadcast packet
	Com_Printf( "querying %s...\n", cl_master->string );

	if( NET_StringToAdr( cl_master->string, &adr ) ) {
		if( !adr.port )
			adr.port = BigShort( PORT_MASTER );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master->string);
	}
}
/*
=================
CL_ParseGetServersResponse

Handle a reply from getservers message to master server
=================
*/
void CL_ParseGetServersResponse()
{
	cvar_t		*noudp;
	cvar_t		*noipx;
	netadr_t	adr;
	char		adrString[32];
	byte		addr[4];

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);	// skip the -1

	noudp = Cvar_Get ("noudp", "0", CVAR_NOSET);
	if (!noudp->value)
	{
		adr.type = NA_BROADCAST;
	}
	noipx = Cvar_Get ("noipx", "0", CVAR_NOSET);
	if (!noipx->value)
	{
		adr.type = NA_BROADCAST_IPX;
	}
	numServers = 0;

	if(net_message.readcount +8 < net_message.cursize)
	net_message.readcount += 8;

	while( net_message.readcount +6 <= net_message.cursize ) {
		MSG_ReadData( &net_message, addr, 4 );
		servers[numServers].port = MSG_ReadShort( &net_message );

		Com_sprintf( adrString, sizeof( adrString ), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);

		memcpy (&servers[numServers].ip, adrString, sizeof (servers[numServers].ip));

		Com_Printf ("pinging %s...\n", servers[numServers].ip);
		if (!NET_StringToAdr (servers[numServers].ip, &adr))
		{
			Com_Printf ("Bad address: %s\n", servers[numServers].ip);
			continue;
		}
		adr.port = servers[numServers].port;
		if (!adr.port) {
			adr.port = BigShort(PORT_SERVER);
		}
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));

		if (++numServers == 64)
			break;
	}
	//set the start time for pings
	starttime = Sys_Milliseconds();

}

/*
=================
CL_PingServers_f
=================
*/
void CL_PingServers_f (void)
{

	int			i;
	netadr_t	adr;
	char		name[32];
	char		*adrstring;
	cvar_t		*noudp;
	cvar_t		*noipx;

	NET_Config (true);		// allow remote

	GetServerList();//get list from COR master server

	// send a broadcast packet
	Com_Printf ("pinging broadcast...\n");

	noudp = Cvar_Get ("noudp", "0", CVAR_NOSET);
	if (!noudp->value)
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));

	}
	noipx = Cvar_Get ("noipx", "0", CVAR_NOSET);
	if (!noipx->value)
	{
		adr.type = NA_BROADCAST_IPX;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));
	}

	// send a packet to each address book entry
	for (i=0 ; i<16 ; i++)
	{
		Com_sprintf (name, sizeof(name), "adr%i", i);
		adrstring = Cvar_VariableString (name);
		if (!adrstring || !adrstring[0])
			continue;

		Com_Printf ("pinging %s...\n", adrstring);
		if (!NET_StringToAdr (adrstring, &adr))
		{
			Com_Printf ("Bad address: %s\n", adrstring);
			continue;
		}
		if (!adr.port)
			adr.port = BigShort(PORT_SERVER);

		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));

	}

	// -JD restart the menu music that was stopped during this procedure
	S_StartMenuMusic();
}


/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f (void)
{
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		Com_Printf ("client %i: %s\n", i, cl.configstrings[CS_PLAYERSKINS+i]);
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
	}
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);	// skip the -1

	s = MSG_ReadStringLine (&net_message);

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);

	Com_Printf ("%s: %s\n", NET_AdrToString (net_from), c);

	if(!strncmp(c, "servers", 7))
	{
		CL_ParseGetServersResponse();
		return;
	}

	if(!strncmp(c, "vstring", 7))
	{
		s = Cmd_Argv(1);
		switch(currLoginState.requestType)
		{
			case STATSLOGIN:
				STATS_AuthenticateStats(s);
				break;
			case STATSPWCHANGE:
				STATS_ChangePassword(s);
				break;
		}
		return;
	}

	if(!strncmp(c, "validated", 9))
	{
		//in cases of changed passwords
		if(currLoginState.requestType == STATSPWCHANGE)
		{
			//make sure the password is stored for future use
			Q_strncpyz2(currLoginState.old_password, password->string, sizeof(currLoginState.old_password));

			Com_Printf("Password change successful!\n");
		}
		else
		{
			Com_Printf("Account validated!\n");
			currLoginState.validated = true;
		}
		return;
	}

	// server connection
	if (!strcmp(c, "client_connect"))
	{
		if (cls.state == ca_connected)
		{
			Com_Printf ("Dup connect received.  Ignored.\n");
			return;
		}
		Netchan_Setup (NS_CLIENT, &cls.netchan, net_from, cls.quakePort);
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		cls.state = ca_connected;

		memset(cls.downloadurl, 0, sizeof(cls.downloadurl));
		if(Cmd_Argc() == 2)  // http download url
		{
			strncpy(cls.downloadurl, Cmd_Argv(1), sizeof(cls.downloadurl) - 1);
		}
		return;
	}

	// remote command from gui front end
	if (!strcmp(c, "cmd"))
	{
		if (!NET_IsLocalAddress(net_from))
		{
			Com_Printf ("Command packet from remote host.  Ignored.\n");
			return;
		}
		Sys_AppActivate ();
		s = MSG_ReadString (&net_message);
		Cbuf_AddText (s);
		Cbuf_AddText ("\n");
		return;
	}
	// print command from somewhere
	if (!strcmp(c, "print"))
	{
		s = MSG_ReadString (&net_message);
		Com_Printf ("%s", s);
		M_AddToServerList (net_from, s); //add net_from so we have the addy
		return;
	}

	// ping from somewhere
	if (!strcmp(c, "ping"))
	{
		Netchan_OutOfBandPrint (NS_CLIENT, net_from, "ack");
		return;
	}

	// challenge from the server we are connecting to
	if (!strcmp(c, "challenge"))
	{
		cls.challenge = atoi(Cmd_Argv(1));
		CL_SendConnectPacket ();
		return;
	}

	// echo request from server
	if (!strcmp(c, "echo"))
	{
		Netchan_OutOfBandPrint (NS_CLIENT, net_from, "%s", Cmd_Argv(1) );
		return;
	}

	Com_Printf ("Unknown command.\n");
}


/*
=================
CL_DumpPackets

A vain attempt to help bad TCP stacks that cause problems
when they overflow
=================
*/
void CL_DumpPackets (void)
{
	while (NET_GetPacket (NS_CLIENT, &net_from, &net_message))
	{
		Com_Printf ("dumping a packet\n");
	}
}

/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void)
{
	while (NET_GetPacket (NS_CLIENT, &net_from, &net_message))
	{

		//
		// remote command packet
		//
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket ();
			continue;
		}

		if (cls.state == ca_disconnected || cls.state == ca_connecting)
			continue;		// dump it if not connected

		if (net_message.cursize < 8)
		{
			Com_Printf ("%s: Runt packet\n",NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!NET_CompareAdr (net_from, cls.netchan.remote_address))
		{
			Com_DPrintf ("%s:sequenced packet without connection\n"
				,NET_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&cls.netchan, &net_message))
			continue;		// wasn't accepted for some reason
		CL_ParseServerMessage ();
	}

	//
	// check timeout
	//
	if (cls.state >= ca_connected
	 && cls.realtime - cls.netchan.last_received > cl_timeout->value*1000)
	{
		if (++cl.timeoutcount > 5)	// timeoutcount saves debugger
		{
			Com_Printf ("\nServer connection timed out.\n");
			CL_Disconnect ();
			return;
		}
	}
	else
		cl.timeoutcount = 0;

}


//=============================================================================

/*
==============
CL_FixUpGender_f
==============
*/
void CL_FixUpGender(void)
{
	char *p;
	char sk[80];

	if (gender_auto->value)
	{
		if (gender->modified)
		{
			// was set directly, don't override the user
			gender->modified = false;
			return;
		}

		strncpy(sk, skin->string, sizeof(sk) - 1);
		if ((p = strchr(sk, '/')) != NULL)
			*p = 0;
		if (Q_strcasecmp(sk, "male") == 0 || Q_strcasecmp(sk, "cyborg") == 0)
			Cvar_Set ("gender", "male");
		else if (Q_strcasecmp(sk, "female") == 0 || Q_strcasecmp(sk, "crackhor") == 0)
			Cvar_Set ("gender", "female");
		else
			Cvar_Set ("gender", "none");
		gender->modified = false;
	}
}

/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f (void)
{
	Com_Printf ("User info settings:\n");
	Info_Print (Cvar_Userinfo());
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem so it can pick up
new parameters and flush all sounds
=================
*/
void CL_Snd_Restart_f (void)
{
	S_Shutdown ();
	S_Init ();
	CL_RegisterSounds ();
}

int precache_check; // for autodownload of precache items
int precache_spawncount;
int precache_tex;
int precache_model_skin;

byte *precache_model; // used for skin checking in alias models

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)
#define SCRIPT_CNT (TEXTURE_CNT+999)

static const char *env_suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

void CL_RequestNextDownload (void)
{
	unsigned	map_checksum;		// for detecting cheater maps
	char fn[MAX_OSPATH];
	char map[MAX_OSPATH];
	char script[MAX_OSPATH];
	dmdl_t *pheader;
	int i, j;

	if (cls.state != ca_connected)
		return;

	if (!allow_download->value && precache_check < ENV_CNT)
		precache_check = ENV_CNT;

	if (precache_check == CS_MODELS) { // confirm map
		precache_check = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->value)
			if (!CL_CheckOrDownloadFile(cl.configstrings[CS_MODELS+1]))
				return; // started a download
	}

redoSkins:
	if (precache_check >= CS_MODELS && precache_check < CS_MODELS+MAX_MODELS)
	{
		if (allow_download_models->value)
		{
			while (precache_check < CS_MODELS+MAX_MODELS &&
				cl.configstrings[precache_check][0])
			{
				if (cl.configstrings[precache_check][0] == '*' ||
					cl.configstrings[precache_check][0] == '#')
				{
					precache_check++;
					continue;
				}
				//do not download player models from this section
				else if (cl.configstrings[precache_check][0] == 'p' &&
					cl.configstrings[precache_check][1] == 'l' &&
					cl.configstrings[precache_check][2] == 'a' &&
					cl.configstrings[precache_check][3] == 'y' &&
					cl.configstrings[precache_check][4] == 'e' &&
					cl.configstrings[precache_check][5] == 'r' &&
					cl.configstrings[precache_check][6] == 's')
				{
					precache_check++;
					continue;
				}
				if (precache_model_skin == 0)
				{
					if (!CL_CheckOrDownloadFile(cl.configstrings[precache_check]))
					{
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model)
				{
					FS_LoadFile (cl.configstrings[precache_check], (void **)&precache_model);
					if (!precache_model)
					{
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					if (LittleLong(*(unsigned *)precache_model) != IDALIASHEADER)
					{
						// not an alias model
						FS_FreeFile(precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
					pheader = (dmdl_t *)precache_model;
					if (LittleLong (pheader->version) != ALIAS_VERSION)
					{
						precache_check++;
						precache_model_skin = 0;
						continue; // couldn't load it
					}
				}

				pheader = (dmdl_t *)precache_model;

				while (precache_model_skin - 1 < LittleLong(pheader->num_skins))
				{
					if (!CL_CheckOrDownloadFile((char *)precache_model +
						LittleLong(pheader->ofs_skins) +
						(precache_model_skin - 1)*MAX_SKINNAME))
					{
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}
				if (precache_model)
				{
					FS_FreeFile(precache_model);
					precache_model = 0;
				}
				precache_model_skin = 0;
				precache_check++;
			}
		}
		if (precache_model)
		{
			precache_check = CS_MODELS + 2;
			precache_model_skin = 0;

			goto redoSkins;
		}
		precache_check = CS_SOUNDS;
	}

	if (precache_check >= CS_SOUNDS && precache_check < CS_SOUNDS+MAX_SOUNDS)
	{
		if (allow_download_sounds->value)
		{
			if (precache_check == CS_SOUNDS)
				precache_check++; // zero is blank
			while (precache_check < CS_SOUNDS+MAX_SOUNDS &&
				cl.configstrings[precache_check][0])
			{
				if (cl.configstrings[precache_check][0] == '*')
				{
					precache_check++;
					continue;
				}
				Com_sprintf(fn, sizeof(fn), "sound/%s", cl.configstrings[precache_check++]);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}
	if (precache_check >= CS_IMAGES && precache_check < CS_IMAGES+MAX_IMAGES)
	{
		if (precache_check == CS_IMAGES)
			precache_check++; // zero is blank
		while (precache_check < CS_IMAGES+MAX_IMAGES &&
			cl.configstrings[precache_check][0])
		{
			Com_sprintf(fn, sizeof(fn), "pics/%s.tga", cl.configstrings[precache_check++]);
			if (!CL_CheckOrDownloadFile(fn))
				return; // started a download
		}
		precache_check = CS_PLAYERSKINS;
	}
	if (precache_check >= CS_PLAYERSKINS && precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
	{
		if (allow_download_players->value)
		{
			while (precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
			{
				int i, n;
				char model[MAX_QPATH], skin[MAX_QPATH], *p;

				i = (precache_check - CS_PLAYERSKINS)/PLAYER_MULT;
				n = (precache_check - CS_PLAYERSKINS)%PLAYER_MULT;

				if (!cl.configstrings[CS_PLAYERSKINS+i][0])
				{
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configstrings[CS_PLAYERSKINS+i], '\\')) != NULL)
					p++;
				else
					p = cl.configstrings[CS_PLAYERSKINS+i];
				strcpy(model, p);
				p = strchr(model, '/');
				if (!p)
					p = strchr(model, '\\');
				if (p)
				{
					*p++ = 0;
					strcpy(skin, p);
				} else
					*skin = 0;

				switch (n)
				{
					case 0: // model
						Com_sprintf(fn, sizeof(fn), "players/%s/tris.iqm", model);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 1: // weapon model
						Com_sprintf(fn, sizeof(fn), "players/%s/weapon.iqm", model);
						if (!CL_CheckOrDownloadFile(fn)) {
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 2: // weapon skin
						Com_sprintf(fn, sizeof(fn), "players/%s/weapon.jpg", model);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 3: // skin
						Com_sprintf(fn, sizeof(fn), "players/%s/%s.jpg", model, skin);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 4: // skin_i
						Com_sprintf(fn, sizeof(fn), "players/%s/%s_i.jpg", model, skin);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
							return; // started a download
						}
						// move on to next model
						precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		precache_check = ENV_CNT;
	}

	if (precache_check == ENV_CNT)
	{
		precache_check = ENV_CNT + 1;

		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);

		if (map_checksum != atoi(cl.configstrings[CS_MAPCHECKSUM]))
		{
			Com_Error (ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.configstrings[CS_MAPCHECKSUM]);
			return;
		}
	}

	if (precache_check > ENV_CNT && precache_check < TEXTURE_CNT)
	{
			if (allow_download->value && allow_download_maps->value)
			{
				while (precache_check < TEXTURE_CNT)
				{
					int n = precache_check++ - ENV_CNT - 1;

					Com_sprintf(fn, sizeof(fn), "env/%s%s.tga",
							cl.configstrings[CS_SKY], env_suf[n/2]);
					if (!CL_CheckOrDownloadFile(fn))
						return; // started a download
				}
			}
			precache_check = TEXTURE_CNT;
	}
	if (precache_check == TEXTURE_CNT)
	{
		precache_check = TEXTURE_CNT+1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT+1)
	{
		// from qcommon/cmodel.c
		extern int			numtexinfo;
		extern mapsurface_t	map_surfaces[];

		if (allow_download->value && allow_download_maps->value)
		{
			while (precache_tex < numtexinfo)
			{
				char fn[MAX_OSPATH];

				sprintf(fn, "textures/%s.tga", map_surfaces[precache_tex++].rname);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT+999;
	}

	//get map related scripts
	if (precache_check == SCRIPT_CNT)
	{
		precache_check = SCRIPT_CNT+1;
		if (allow_download_maps->value)
		{
			//get fog files
			COM_StripExtension ( cl.configstrings[CS_MODELS+1], fn );

			//remove "maps/" from string
			for(i = 5, j = 0; i < strlen(fn); i++, j++)
			{
				map[j] = fn[i];
			}
			map[i-5] = 0;

			Com_sprintf(script, sizeof(script), "maps/scripts/%s.fog", map);
			if (!CL_CheckOrDownloadFile(script))
				return; // started a download
		}
	}

	if (precache_check == SCRIPT_CNT+1)
	{
		precache_check = SCRIPT_CNT+2;
		if (allow_download_maps->value)
		{
			//get mus files
			COM_StripExtension ( cl.configstrings[CS_MODELS+1], fn );

			//remove "maps/" from string
			for(i = 5, j = 0; i < strlen(fn); i++, j++)
				map[j] = fn[i];
			map[i-5] = 0;

			Com_sprintf(script, sizeof(script), "maps/scripts/%s.mus", map);
			if (!CL_CheckOrDownloadFile(script))
				return; // started a download
		}
	}

	if (precache_check == SCRIPT_CNT+2)
	{
		precache_check = SCRIPT_CNT+3;
		if (allow_download_maps->value)
		{
			//get rscript files
			COM_StripExtension ( cl.configstrings[CS_MODELS+1], fn );

			//remove "maps/" from string
			for(i = 5, j = 0; i < strlen(fn); i++, j++)
				map[j] = fn[i];
			map[i-5] = 0;

			Com_sprintf(script, sizeof(script), "scripts/maps/%s.rscript", map);
			if (!CL_CheckOrDownloadFile(script))
				return; // started a download
		}
	}

//ZOID
	CL_RegisterSounds ();
	CL_PrepRefresh ();

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("begin %i\n", precache_spawncount) );
}


/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f (void)
{
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2)
	{
		unsigned	map_checksum;		// for detecting cheater maps

		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);
		CL_RegisterSounds ();
		CL_PrepRefresh ();
		return;
	}

	precache_check = CS_MODELS;
	precache_spawncount = atoi(Cmd_Argv(1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload();
}

/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal (void)
{
	cls.state = ca_disconnected;
	cls.realtime = Sys_Milliseconds ();

	CL_InitInput ();

	CL_InitHttpDownload();

	CL_IRCSetup( );

	adr0 = Cvar_Get( "adr0", "", CVAR_ARCHIVE );
	adr1 = Cvar_Get( "adr1", "", CVAR_ARCHIVE );
	adr2 = Cvar_Get( "adr2", "", CVAR_ARCHIVE );
	adr3 = Cvar_Get( "adr3", "", CVAR_ARCHIVE );
	adr4 = Cvar_Get( "adr4", "", CVAR_ARCHIVE );
	adr5 = Cvar_Get( "adr5", "", CVAR_ARCHIVE );
	adr6 = Cvar_Get( "adr6", "", CVAR_ARCHIVE );
	adr7 = Cvar_Get( "adr7", "", CVAR_ARCHIVE );
	adr8 = Cvar_Get( "adr8", "", CVAR_ARCHIVE );

#if PKT_TEST
	cl_packet_test = Cvar_Get( "cl_packet_test", "0", 0 );
	cl_frcjitter = Cvar_Get( "cl_frcjitter", "1", 0 );
#endif

//
// register our variables
//
	cl_stereo_separation = Cvar_Get( "cl_stereo_separation", "0.4", CVAR_ARCHIVE );
	cl_stereo = Cvar_Get( "cl_stereo", "0", 0 );
	background_music = Cvar_Get("background_music", "1", CVAR_ARCHIVE);
	background_music_vol = Cvar_Get("background_music_vol", "0.5", CVAR_ARCHIVE);

	cl_add_blend = Cvar_Get ("cl_blend", "1", 0);
	cl_add_lights = Cvar_Get ("cl_lights", "1", 0);
	cl_add_particles = Cvar_Get ("cl_particles", "1", 0);
	cl_add_entities = Cvar_Get ("cl_entities", "1", 0);
	cl_gun = Cvar_Get ("cl_gun", "1", CVAR_ARCHIVE);
	cl_footsteps = Cvar_Get ("cl_footsteps", "1", 0);
	cl_noskins = Cvar_Get ("cl_noskins", "0", CVAR_ARCHIVE);
	cl_autoskins = Cvar_Get ("cl_autoskins", "0", 0);
	cl_predict = Cvar_Get ("cl_predict", "1", 0);
	cl_maxfps = Cvar_Get ("cl_maxfps", "90", CVAR_ARCHIVE);
	cl_showPlayerNames = Cvar_Get ("cl_showplayernames", "0", CVAR_ARCHIVE);
	cl_healthaura = Cvar_Get ("cl_healthaura", "1", CVAR_ARCHIVE);
	cl_noblood = Cvar_Get ("cl_noblood", "0", CVAR_ARCHIVE);
	cl_disbeamclr = Cvar_Get("cl_disbeamclr", "0", CVAR_ARCHIVE);
	cl_brass = Cvar_Get ("cl_brass", "1", CVAR_ARCHIVE);
	cl_playtaunts = Cvar_Get ("cl_playtaunts", "1", CVAR_ARCHIVE);
	cl_centerprint = Cvar_Get ("cl_centerprint", "1", CVAR_ARCHIVE);
	cl_precachecustom = Cvar_Get ("cl_precachecustom", "0", CVAR_ARCHIVE);

	cl_paindist = Cvar_Get ("cl_paindist", "1", CVAR_ARCHIVE);
	cl_explosiondist = Cvar_Get ("cl_explosiondist", "1", CVAR_ARCHIVE);

	cl_upspeed = Cvar_Get ("cl_upspeed", "200", 0);
	cl_forwardspeed = Cvar_Get ("cl_forwardspeed", "200", 0);
	cl_sidespeed = Cvar_Get ("cl_sidespeed", "200", 0);
	cl_yawspeed = Cvar_Get ("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get ("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get ("cl_anglespeedkey", "1.5", 0);

	cl_run = Cvar_Get ("cl_run", "0", CVAR_ARCHIVE);
	freelook = Cvar_Get( "freelook", "0", CVAR_ARCHIVE );
	lookspring = Cvar_Get ("lookspring", "0", CVAR_ARCHIVE);
	lookstrafe = Cvar_Get ("lookstrafe", "0", CVAR_ARCHIVE);
	m_accel	= Cvar_Get ("m_accel", "1",	CVAR_ARCHIVE);
	sensitivity = Cvar_Get ("sensitivity", "3", CVAR_ARCHIVE);
	menu_sensitivity = Cvar_Get("menu_sensitivity", "3", CVAR_ARCHIVE);

	m_smoothing = Cvar_Get("m_smoothing", "0", CVAR_ARCHIVE);
	m_pitch = Cvar_Get ("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get ("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get ("m_forward", "1", 0);
	m_side = Cvar_Get ("m_side", "1", 0);

	cl_shownet = Cvar_Get ("cl_shownet", "0", 0);
	cl_showmiss = Cvar_Get ("cl_showmiss", "0", 0);
	cl_showclamp = Cvar_Get ("showclamp", "0", 0);
	cl_timeout = Cvar_Get ("cl_timeout", "120", 0);
	cl_paused = Cvar_Get ("paused", "0", 0);
	cl_timedemo = Cvar_Get ("timedemo", "0", 0);

	rcon_client_password = Cvar_Get ("rcon_password", "", 0);
	rcon_address = Cvar_Get ("rcon_address", "", 0);

	//
	// userinfo
	//
	info_password = Cvar_Get ("password", "", CVAR_USERINFO);
	info_spectator = Cvar_Get ("spectator", "0", CVAR_USERINFO);
	name = Cvar_Get ("name", "unnamed", CVAR_USERINFO | CVAR_ARCHIVE);
	password = Cvar_Get("stats_password", "password", CVAR_PROFILE);
	Q_strncpyz2(currLoginState.old_password, password->string, sizeof(currLoginState.old_password));
	pw_hashed = Cvar_Get("stats_pw_hashed", "0", CVAR_PROFILE);
	/* */
	ValidatePlayerName( name->string, (strlen(name->string)+1) );
	/* */
	skin = Cvar_Get ("skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE);
	rate = Cvar_Get ("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE);	// FIXME
	msg = Cvar_Get ("msg", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	hand = Cvar_Get ("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	fov = Cvar_Get ("fov", "90", CVAR_USERINFO | CVAR_ARCHIVE);
	gender = Cvar_Get ("gender", "male", CVAR_USERINFO | CVAR_ARCHIVE);
	gender_auto = Cvar_Get ("gender_auto", "1", CVAR_ARCHIVE);
	gender->modified = false; // clear this so we know when user sets it manually

	cl_vwep = Cvar_Get ("cl_vwep", "1", CVAR_ARCHIVE);
	cl_vehicle_huds = Cvar_Get ("cl_vehicle_huds", "1", CVAR_ARCHIVE);

	cl_master = Cvar_Get ("cl_master", "master.corservers.com", CVAR_ARCHIVE);
	cl_master2 = Cvar_Get ("cl_master2", "master2.corservers.com", CVAR_ARCHIVE);

	//custom huds
	cl_hudimage1 = Cvar_Get("cl_hudimage1", "pics/i_health.tga", CVAR_ARCHIVE);
	cl_hudimage2 = Cvar_Get("cl_hudimage2", "pics/i_score.tga", CVAR_ARCHIVE);

	//stats server
	cl_stats_server = Cvar_Get("cl_stats_server", "http://stats.planetarena.org", CVAR_ARCHIVE);

	//update checker
	cl_latest_game_version = Cvar_Get("cl_latest_game_version", VERSION, CVAR_ARCHIVE);
	cl_latest_game_version_url = Cvar_Get("cl_latest_game_version_server", "http://red.planetarena.org/version/crx_version", CVAR_ARCHIVE);

	//
	// register our commands
	//
	Cmd_AddCommand ("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand ("pause", CL_Pause_f);
	Cmd_AddCommand ("pingservers", CL_PingServers_f);
	Cmd_AddCommand ("skins", CL_Skins_f);

	Cmd_AddCommand ("userinfo", CL_Userinfo_f);
	Cmd_AddCommand ("snd_restart", CL_Snd_Restart_f);

	Cmd_AddCommand ("changing", CL_Changing_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);

	Cmd_AddCommand ("quit", CL_Quit_f);

	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);

	Cmd_AddCommand ("rcon", CL_Rcon_f);

	Cmd_AddCommand ("setenv", CL_Setenv_f );

	Cmd_AddCommand ("precache", CL_Precache_f);

	Cmd_AddCommand ("download", CL_Download_f);

	Cmd_AddCommand ("irc_connect", CL_InitIRC);
	Cmd_AddCommand ("irc_quit", CL_IRCInitiateShutdown);
	Cmd_AddCommand ("irc_say", CL_IRCSay);

	//
	// forward to server commands
	//
	// the only thing this does is allow command completion
	// to work -- all unknown commands are automatically
	// forwarded to the server. It also prevents the commands
	// from being ignored if they are issued in 'forced' mode.
	Cmd_AddCommand ("wave", NULL);
	Cmd_AddCommand ("inven", NULL);
	Cmd_AddCommand ("kill", NULL);
	Cmd_AddCommand ("use", NULL);
	Cmd_AddCommand ("drop", NULL);
	Cmd_AddCommand ("say", NULL);
	Cmd_AddCommand ("say_team", NULL);
	Cmd_AddCommand ("info", NULL);
	Cmd_AddCommand ("prog", NULL);
	Cmd_AddCommand ("give", NULL);
	Cmd_AddCommand ("god", NULL);
	Cmd_AddCommand ("notarget", NULL);
	Cmd_AddCommand ("noclip", NULL);
	Cmd_AddCommand ("invuse", NULL);
	Cmd_AddCommand ("invprevw", NULL);
	Cmd_AddCommand ("invnextw", NULL);
	Cmd_AddCommand ("invprevp", NULL);
	Cmd_AddCommand ("invnextp", NULL);
	Cmd_AddCommand ("invprev", NULL);
	Cmd_AddCommand ("invnext", NULL);
	Cmd_AddCommand ("invdrop", NULL);
	Cmd_AddCommand ("weapnext", NULL);
	Cmd_AddCommand ("weapprev", NULL);
	Cmd_AddCommand ("weaplast", NULL);
	Cmd_AddCommand ("chatbubble", NULL);
	Cmd_AddCommand ("players", NULL);
	Cmd_AddCommand ("score", NULL);
	Cmd_AddCommand ("help", NULL);
	Cmd_AddCommand ("vote", NULL);
	Cmd_AddCommand ("putaway", NULL);
	Cmd_AddCommand ("playerlist", NULL);

	Cvar_SetValue("scriptsloaded", 0);

	strcpy(map_music, "music/none.wav");

	//register all our menu gfx
	R_RegisterPic("m_main");
	R_RegisterPic("m_options");
	R_RegisterPic("menu_back");
	R_RegisterPic("m_video");
	R_RegisterPic("m_controls");
	R_RegisterPic("m_player");
	R_RegisterPic("m_bots");
	R_RegisterPic("m_startserver");
	R_RegisterPic("m_dmoptions");
	R_RegisterPic("m_mutators");
	R_RegisterPic("m_single");
	R_RegisterPic("m_quit");
	R_RegisterPic("m_main_mont0");
	R_RegisterPic("m_main_mont1");
	R_RegisterPic("m_main_mont2");
	R_RegisterPic("m_main_mont3");
	R_RegisterPic("m_main_mont4");
	R_RegisterPic("m_main_mont5");
	R_RegisterPic("hud_bomber");
	R_RegisterPic("hud_strafer");
	R_RegisterPic("hud_hover");
	R_RegisterPic("blood_ring");
}



/*
===============
CL_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void CL_WriteConfiguration (void)
{
	FILE	*f;
	char	path[MAX_OSPATH];

	if (cls.state == ca_uninitialized)
		return;

	FS_FullWritePath( path, sizeof(path), "config.cfg");
	f = fopen (path, "w");
	if (!f)
	{
		Com_Printf ("Couldn't write config.cfg.\n");
		return;
	}

	fprintf (f, "// generated by Alien Arena. Use autoexec.cfg for custom settings.\n");
	Key_WriteBindings (f);
	fclose (f);

	Cvar_WriteVariables (path);
}

/*
===============
CL_WriteProfile

Writes profile information to profile.cfg
===============
*/
void CL_WriteProfile (void)
{
	FILE	*f;
	char	path[MAX_OSPATH];

	if (cls.state == ca_uninitialized)
		return;

	FS_FullWritePath( path, sizeof(path), "profile.cfg");
	f = fopen (path, "w");
	if (!f)
	{
		Com_Printf ("Couldn't write profile.cfg.\n");
		return;
	}

	fprintf (f, "// generated by Alien Arena.\n");
	fprintf (f, "set stats_password %s\n", password->string);
	fprintf (f, "set stats_pw_hashed %i\n", pw_hashed->integer);
	fclose (f);
}



/*
==================
CL_FixCvarCheats

==================
*/

typedef struct
{
	char	*name;
	char	*value;
	cvar_t	*var;
} cheatvar_t;

cheatvar_t cheatvars[] =
{
	{"timescale",     "1", NULL},
	{"timedemo",      "0", NULL},
	{"r_drawworld",   "1", NULL},
	{"cl_testlights", "0", NULL},
	{"r_fullbright",  "0", NULL},
	{"r_drawflat",    "0", NULL},
	{"paused",        "0", NULL},
	{"fixedtime",     "0", NULL},
	{"gl_lightmap",   "0", NULL},
	{"gl_showpolys",  "0", NULL},
	{NULL, NULL, NULL}
};

void CL_FixCvarCheats (void)
{
	static int numcheatvars = 0;
	int			i;
	cheatvar_t	*var;

	if ( !strcmp(cl.configstrings[CS_MAXCLIENTS], "1")
		|| !cl.configstrings[CS_MAXCLIENTS][0] )
		return;		// single player can cheat

	// find all the cvars if we haven't done it yet
	if (!numcheatvars)
	{
		while (cheatvars[numcheatvars].name)
		{
			cheatvars[numcheatvars].var = Cvar_Get (cheatvars[numcheatvars].name,
					cheatvars[numcheatvars].value, 0);
			numcheatvars++;
		}
	}

	// make sure they are all set to the proper values
	for (i=0, var = cheatvars ; i<numcheatvars ; i++, var++)
	{
		if ( cl.attractloop &&
			(!strcmp( var->name, "timedemo" )
			||
			!strcmp( var->name, "timescale"))
			)
			continue; // allow these when running a .dm2 demo

		if ( strcmp (var->var->string, var->value) )
		{
			Cvar_Set (var->name, var->value);
		}
	}
}

//============================================================================

/*
 * Temporary test data collection for packet rate limiting
 * cvar: cl_packet_test
 * Dumps collected packets per server frame and packet timer info
 *  at 1024 server frame intervals
 * This is TEMPORARY. To be removed after initial testing.
 *  'cause the data format is technical, arcane and crude.
 */
#if PKT_TEST

#define PPSF_SIZE 16
int svframe_count;
int pkts_per_svframe[PPSF_SIZE];
int ppsfi;

#define PKTTIME_SIZE 30
int pkttime_ix;
int pkttime[PKTTIME_SIZE];

#endif

qboolean send_packet_now = false; // instant packets. used during downloads
extern float    r_frametime;      // TODO: move to appropriate .h
extern unsigned sys_frame_time;   // TODO: ditto

/**
 * @brief Top level client-side routine for main loop.
 *
 * @param msec  the time since the previous CL_Frame.
 */
/*
 * Notes on time variables:
 * cls.frametime :
 *  float seconds since last render
 *  clamped for packet processing, now unclamped for rendering
 *  used in client-to-server move message
 *  in CL_AddClEntities(): used in bouncing entities (brass) calculations
 *
 * r_frametime :
 *  unclamped float seconds since last render. Used in particle rendering.
 *
 * cl.time :
 *  critical global timer used in various places.
 *  clamped to [cl.frame.servertime-100, cl.frame.servertime] in CL_ParseFrame()
 *
 * cls.realtime :
 *   set to time at start of frame. Updated anywhere else?
 *
 *  cl.frame.servertime:
 *    equivalent to server frame number times 100msecs. see CL_ParseFrame()
 */

/* Packet Rate Limiting Cap in milliseconds
 *  msecs=PPS :: 12=83, 13=76, 14=71, 15=66, 16=62
 * Current choice is 12msec/83PPS nominal.
 *  This matches the default cl_maxfps setting.
 *  Which is 90, of course, but because of msec rounding, the result is 83
 *  This results in 8 packets per server frame, mostly.
 * Packet rate limiting is not invoked unless the PPS is set higher than the FPS.
 * Seems like a good idea not to invoke packet rate limiting unless the
 *  cl_maxfps is set higher than the default.
 */
#define PKTRATE_CAP 12

void CL_Frame( int msec )
{
	// static int lasttimecalled = 0; // TODO: see below, obsolete logging?

#if PKT_TEST
	static int frcjitter[4]   = { 0, -1, 0, 1 };
	static int frcjitter_ix   = 0;
#endif
	static int framerate_cap  = 0;
	static int packetrate_cap = 0;
	static int packet_timer   = 0;
	static int render_timer   = 0;
	static int render_counter = 0;
	static int packet_delay = 0;
	int render_trigger;
	int packet_trigger;

	// test variables
	static int prev_server_frame = 0;
	static int packets_per_server_frame = 0;

	if ( dedicated->integer )
	{ // crx running as dedicated server crashes without this.
		return;
	}

	cls.realtime  = curtime; // time at start of Qcommon_Frame()
	cls.frametime = 0.0f;    // zero here for debug purposes, set below
	cl.time += msec; // clamped to [cl.frame.servertime-100,cl.frame.servertime]

	/* local timers for decoupling frame rate from packet rate */
	packet_timer += msec;
	render_timer += msec;

	/* periodically override certain test and cheat cvars unless single player.*/
	if ( (cl.time & 0xfff) == 0 )
	{
		CL_FixCvarCheats ();
	}

	/*
	 * update maximum FPS.
	 * framerate_cap is in msecs/frame and is user specified.
	 * Note: Quake2World sets a hard lower limit of 30. Not sure if
	 *   that is needed; to be determined if that is a good thing to do.
	 */
	if ( cl_maxfps->modified  )
	{
		if ( cl_maxfps->value < 30.0f )
		{
			Com_Printf("Warning: cl_maxfps set to less than 30.\n");
			if ( cl_maxfps->value < 1.0f )
			{
				Cvar_ForceSet( "cl_maxfps", "1" );
			}
		}
		cl_maxfps->modified = false;
		framerate_cap = 0; // force a recalculation
	}
	if ( framerate_cap < 1 )
	{
		framerate_cap = (int)(ceil( 1000.0f / cl_maxfps->value ));
		if ( framerate_cap < 1 )
		{
			framerate_cap = 1;
		}
		Com_DPrintf("framerate_cap set to %i msec\n", framerate_cap );
	}

	/*
	 * Set nominal milliseconds-per-packet for client-to-server messages.
	 * Idea is to be timely in getting and transmitting player input without
	 *   congesting the server.
	 * Plan is to not implement a user setting for this unless a need for that
	 *   is discovered.
	 * If the cl_maxfps is set low, then FPS will limit PPS, and
	 *  packet rate limiting is bypassed.
	 */
	if ( cls.state == ca_connected )
	{ // receiving configstrings from the server, run at nominal 10PPS
		// avoids unnecessary load on the server
		if ( packetrate_cap != 100 )
			Com_DPrintf("packet rate change: 10 PPS\n");
		packetrate_cap = 100;
	}
	else if ( framerate_cap >= PKTRATE_CAP )
	{ // do not to throttle packet sending, run in sync with render
		if ( packetrate_cap != -1)
			Com_DPrintf("packetrate change: framerate\n");
		packetrate_cap = -1;
	}
	else
	{ // packet rate limiting
		if ( packetrate_cap != PKTRATE_CAP )
			Com_DPrintf("packetrate change: %iPPS\n", 1000/PKTRATE_CAP);
		packetrate_cap = PKTRATE_CAP;
	}

	/* local triggers for decoupling framerate from packet rate  */
	render_trigger = 0;
	packet_trigger = 0;

	if ( cl_timedemo->integer == 1 )
	{ /* accumulate timed demo statistics, free run both packet and render */
		if ( !cl.timedemo_start )
		{
			cl.timedemo_start = cls.realtime;
		}
		cl.timedemo_frames++;
		render_trigger = 1;
		packet_trigger = 1;
	}
	else
	{ /* normal operation. */
#if PKT_TEST
		if ( cl_frcjitter->integer )
		{ // the cvar is temporary for test, to allow disabling this
			/* Sometimes, the packetrate_cap can be "in phase" with
			 *  the frame rate affecting the average packets-per-server-frame.
			 *  A little jitter in the framerate_cap counteracts that.
			 */
			if ( render_timer >= (framerate_cap + frcjitter[frcjitter_ix]) )
			{
				if ( ++frcjitter_ix > 3 ) frcjitter_ix = 0;
				render_trigger = 1;
			}
		}
		else
		{
			if ( render_timer >= framerate_cap )
			{
				render_trigger = 1;
			}

		}
#else
		if ( render_timer >= framerate_cap )
		{
			render_trigger = 1;
		}
#endif
		if ( packetrate_cap == -1 )
		{ // flagged to run same as framerate_cap
			packet_trigger = render_trigger;
		}
		else if ( packet_timer >= packetrate_cap )
		{ // normal packet trigger
			packet_trigger = 1;
		}
		else if ( packet_delay > 0 )
		{ // packet sent in previous loop was delayed
			if ( packet_timer >= 9 )
			{ // should be ok to send a packet early
				/* idea is to try to maintain a steady number of
				 * client-to-server packets per server frame.
				 * If render is triggered, it is good to poll input and
				 *  send a packet to avoid more delay.
				 * If adding the delay to the timer reaches the cap then
				 *  try to catch up
				 * Otherwise, do nothing, the next loop should occur soon.
				 */
				if ( render_trigger
					|| ((packet_timer + packet_delay) >= packetrate_cap) )
				{
					packet_trigger = 1;
				}
			}
		}
	}

	if ( packet_trigger || send_packet_now )
	{
		send_packet_now = false; // used during downloads

		if ( packetrate_cap > 0 && packet_timer > packetrate_cap )
		{ // difference between cap and timer, a delayed packet
			packet_delay = packet_timer - packetrate_cap;
		}
		else
		{
			packet_delay = 0;
		}

		render_counter = 0; // for counting renders since last packet

		/* let the mouse activate or deactivate */
		IN_Frame();

		/*
		 * calculate frametime in seconds for packet procedures
		 * cls.frametime is source for the cmd.msecs byte
		 *  in the client-to-server move message.
		 */
		cls.frametime  = ((float)packet_timer) / 1000.0f;
		if ( cls.frametime >= 0.250f )
		{ /* very long delay */
			/*
			 * server checks for cmd.msecs to be <= 250
			 */
			cls.frametime = 0.24999f ;
			Com_DPrintf("CL_Frame(): cls.frametime clamped from %0.8f to 0.24999\n",
					cls.frametime );
			/*
			 * try to throttle the video frame rate by overriding the
			 * render trigger.
			 */
			render_trigger = false;
			render_timer = 0;
		}

		/* process server-to-client packets */
		CL_ReadPackets();

		/* execute pending commands */
		Cbuf_Execute();

		/* run cURL downloads */
		CL_HttpDownloadThink();

		/* system dependent keyboard and mouse input event polling */
		Sys_SendKeyEvents();

		/* joystick input. may or may not be working. */
		IN_Commands();

		/* execute pending commands */
		Cbuf_Execute();

		/* send client commands to server */
		CL_SendCmd();

		/* resend a connection request if necessary */
		CL_CheckForResend();

		/*
		 * predict movement for un-acked client-to-server packets
		 * [The Quake trick that keeps players view smooth in on-line play.]
		 */
		CL_PredictMovement();

#if PKT_TEST

		if ( packetrate_cap > 0 && cl_packet_test->integer )
		{
		/* developer test for checking packets per serverframe, packet delays
		 *  histogram data collection
		 */
			/*
			 * collect packet timer histogram
			 */
			pkttime_ix = packet_timer;
			if ( pkttime_ix >= PKTTIME_SIZE-1 )
			{
				++pkttime[PKTTIME_SIZE-1];
			}
			else
			{
				++pkttime[pkttime_ix];
			}

			if ( prev_server_frame != cl.frame.serverframe )
			{ // new server frame
				++svframe_count;
				if ( packets_per_server_frame > 0
						&& packets_per_server_frame < PPSF_SIZE-1 )
				{
					++pkts_per_svframe[packets_per_server_frame];
				}
				else
				{
					++pkts_per_svframe[PPSF_SIZE-1];
				}

				if ( (svframe_count & 0x3FF) == 0 )
				{ // dump to console and clear every 1024 server frames, ~100 secs
					/*--- PACKETS PER SERVER FRAME ---*/
					Com_Printf( "ppsf(%i):", svframe_count );
					for ( ppsfi = 0; ppsfi < PPSF_SIZE-1; ppsfi++ )
					{
						if ( pkts_per_svframe[ppsfi] != 0 )
						{
							Com_Printf( "[%i:%i]", ppsfi,
							        pkts_per_svframe[ppsfi] );
							pkts_per_svframe[ppsfi] = 0;
						}
					}
					Com_Printf( "\n" );
					/*--- PACKET TIMER HISTOGRAM ---*/
					Com_Printf( "pkttime:");
					for ( pkttime_ix = 0; pkttime_ix < PKTTIME_SIZE-1; pkttime_ix++ )
					{
						if ( pkttime[pkttime_ix] != 0 )
						{
							Com_Printf( "[%i:%i]",
									pkttime_ix, pkttime[pkttime_ix] );
							pkttime[pkttime_ix] = 0;
						}
					}
					Com_Printf( "\n" );
				}
				prev_server_frame = cl.frame.serverframe;
				packets_per_server_frame = 1;
			}
			else
			{
				++packets_per_server_frame;
			}
		}
#endif

		/* retrigger packet send timer */
		packet_timer = 0;
	}

	/*
	 * refresh can occur on different frames than client-to-server messages.
	 *  when packet rate limiting is in effect
	 */
	if ( render_trigger )
	{
		++render_counter; // counting renders since last packet

		/*
		 * calculate cls.frametime in seconds for render procedures.
		 *
		 * May not need to clamp for rendering.
		 * Only would affect things if framerate went below 4 FPS.
		 *
		 * Using a simple lowpass filter, to smooth out irregular timing.
		 */
		cls.frametime = (float)(render_timer) / 1000.0f;
		r_frametime = (r_frametime + cls.frametime + cls.frametime) / 3.0f;
		cls.frametime = r_frametime;

		//  Update the display
		VID_CheckChanges();
		if ( !cl.refresh_prepped && cls.state == ca_active )
		{ // re-initialize video configuration
			CL_PrepRefresh();
		}
		else
		{ // regular screen update
			if ( host_speeds->value )
				time_before_ref = Sys_Milliseconds(); // TODO: obsolete test?
			SCR_UpdateScreen();
			if ( host_speeds->value )
				time_after_ref = Sys_Milliseconds();
		}

		// update audio.
		S_Update( cl.refdef.vieworg, cl.v_forward, cl.v_right, cl.v_up );

		// advance local effects for next frame
		CL_RunDLights();
		CL_RunLightStyles();
		SCR_RunConsole ();

		++cls.framecount; // does not appear to be used anywhere

		/* developer test for observing timers */
		// Com_DPrintf("rt: %i cft: %f\n", render_timer, cls.frametime );

		// retrigger render timing
		render_timer = 0;

#if 0
		/* TODO: Check if this still works and/or is useful.
		 */
		if ( log_stats->value )
		{
			if ( cls.state == ca_active )
			{
				if ( !lasttimecalled )
				{
					lasttimecalled = Sys_Milliseconds();
					if ( log_stats_file )
						fprintf( log_stats_file, "0\n" );
				}
				else
				{
					int now = Sys_Milliseconds();

					if ( log_stats_file )
						fprintf( log_stats_file, "%d\n", now - lasttimecalled );
					lasttimecalled = now;
				}
			}
		}
#endif

	}

}

//============================================================================

/*
====================
CL_Init
====================
*/
void CL_Init (void)
{
	if (dedicated->value)
		return;		// nothing running on the client

	// Initialise fonts
	CL_gameFont = &_CL_gameFont;
	FNT_AutoInit( CL_gameFont , "default" , 0 , 65 , 8 , 48 );
	CL_gameFont->faceVar = Cvar_Get( "fnt_game" , "creativeblock" , CVAR_ARCHIVE );
	CL_gameFont->sizeVar = Cvar_Get( "fnt_game_size" , "0" , CVAR_ARCHIVE );
	FNT_AutoRegister( CL_gameFont );

	CL_centerFont = &_CL_centerFont;
	FNT_AutoInit( CL_centerFont , "default" , 0 , 45 , 16 , 64 );
	CL_centerFont->faceVar = CL_gameFont->faceVar;
	CL_centerFont->sizeVar = Cvar_Get( "fnt_center_size" , "0" , CVAR_ARCHIVE );
	FNT_AutoRegister( CL_centerFont );

	CL_consoleFont = &_CL_consoleFont;
	FNT_AutoInit( CL_consoleFont , "default" , 0 , 52 , 8 , 48 );
	CL_consoleFont->faceVar = Cvar_Get( "fnt_console" , "freemono" , CVAR_ARCHIVE );
	CL_consoleFont->sizeVar = Cvar_Get( "fnt_console_size" , "0" , CVAR_ARCHIVE );
	FNT_AutoRegister( CL_consoleFont );

	// all archived variables will now be loaded

	CON_Initialise( );

	VID_Init ();
	S_Init ();

	V_Init ();

	net_message.data = net_message_buffer;
	net_message.maxsize = sizeof(net_message_buffer);

	M_Init ();

	SCR_Init ();
	cls.disable_screen = true;	// don't draw yet

	CL_InitLocal ();
	IN_Init ();

	FS_ExecAutoexec (); // add commands from autoexec.cfg
	Cbuf_Execute ();

	if ( name && name->string[0] )
	{
		ValidatePlayerName( name->string, (strlen(name->string)+1) );
	}
}


/*
===============
CL_Shutdown

FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

	STATS_Logout();

	CL_IRCInitiateShutdown();
	CL_ShutdownHttpDownload();
	CL_WriteConfiguration ();
	CL_WriteProfile();

	S_Shutdown();
	IN_Shutdown ();
	VID_Shutdown();
	CL_IRCWaitShutdown( );

	NET_Shutdown();

	RS_FreeAllScripts();
}


