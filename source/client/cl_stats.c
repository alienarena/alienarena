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

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

#ifdef _WIN32 // All CURL libs are distributed in Windows.
#define HAVE_CURL
#endif

#ifdef __unix__
#include <unistd.h>
#endif

#ifdef HAVE_CURL
#include "curl/curl.h"
CURLM *curlm;
CURL *curl;
#endif

extern cvar_t  *cl_stats_server;

static FILE* statsdb_open( const char* mode )
{
	FILE* file;
	char pathbfr[MAX_OSPATH];

	Com_sprintf (pathbfr, sizeof(pathbfr)-1, "%s/%s", FS_Gamedir(), "stats.db");
	file = fopen( pathbfr, mode );

	return file;
}

size_t write_data(const void *buffer, size_t size, size_t nmemb, void *userp)
{
	FILE* file;
	size_t bytecount;

	file = statsdb_open( "a" ); //append, don't rewrite

	if(file) {
		//write buffer to file
		bytecount = fwrite( buffer, size, nmemb, file );
		fclose(file);
	}
	return bytecount;
}

//get the stats database
void getStatsDB( void )
{
	FILE* file;
	char statserver[128];

	CURL* easyhandle = curl_easy_init() ;

	file = statsdb_open( "w" ); //create new, blank file for writing
	if(file)
		fclose(file);

	Com_sprintf(statserver, sizeof(statserver), "%s%s", cl_stats_server->string, "/playerrank.db");

	curl_easy_setopt( easyhandle, CURLOPT_URL, statserver ) ;

	curl_easy_setopt( easyhandle, CURLOPT_WRITEFUNCTION, write_data ) ;

	curl_easy_perform( easyhandle );

	curl_easy_cleanup( easyhandle );
}

//parse the stats database, looking for player match
PLAYERSTATS getPlayerRanking ( PLAYERSTATS player )
{
	FILE* file;
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[32], poll[16], remote_address[21];
	int foundplayer = false;

	//open file,
	file = statsdb_open( "r" ) ;

	if(file != NULL) {

		//parse it, and compare to player name
		while(player.ranking < 1000) {

			//name
			fgets(name, sizeof(name), file);
			name[strlen(name) - 1] = 0; //truncate garbage byte
			//remote address
			fgets(remote_address, sizeof(remote_address), file);
			//points
			fgets(points, sizeof(points), file);
			//frags
			fgets(frags, sizeof(frags), file);
			//total frags
			fgets(totalfrags, sizeof(totalfrags), file);
			if(!strcmp(player.playername, name))
				player.totalfrags = atoi(totalfrags);
			//current time in poll
			fgets(time, sizeof(time), file);
			//total time
			fgets(totaltime, sizeof(totaltime), file);
			if(!strcmp(player.playername, name))
				player.totaltime = atof(totaltime);
			//last server.ip
			fgets(ip, sizeof(ip), file);
			//what poll
			fgets(poll, sizeof(poll), file);

			player.ranking++;

			if(!strcmp(player.playername, name)) {
				foundplayer = true;
				break; //get out we are done
			}
		}
		fclose(file);
	}

	if(!foundplayer) {
		player.totalfrags = 0;
		player.totaltime = 1;
	}

	return player;
}

//get player info by rank
PLAYERSTATS getPlayerByRank ( int rank, PLAYERSTATS player )
{
	FILE* file;
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[32], poll[16], remote_address[21];
	int foundplayer = false;

	//open file,
	file = statsdb_open( "r" ) ;

	if(file != NULL) {

		//parse it, and compare to player name
		while(player.ranking < 1000) {

			//name
			fgets(name, sizeof(name), file);
			strcpy(player.playername, name);
			player.playername[strlen(player.playername)-1] = 0; //remove line feed
			//remote address
			fgets(remote_address, sizeof(remote_address), file);
			//points
			fgets(points, sizeof(points), file);
			//frags
			fgets(frags, sizeof(frags), file);
			//total frags
			fgets(totalfrags, sizeof(totalfrags), file);
			player.totalfrags = atoi(totalfrags);
			//current time in poll
			fgets(time, sizeof(time), file);
			//total time
			fgets(totaltime, sizeof(totaltime), file);
			player.totaltime = atof(totaltime);
			//last server.ip
			fgets(ip, sizeof(ip), file);
			//what poll
			fgets(poll, sizeof(poll), file);

			player.ranking++;

			if(player.ranking == rank) {
				foundplayer = true;
				break; //get out we are done
			}
		}
		fclose(file);
	}

	if(!foundplayer) {
		player.totalfrags = 0;
		player.totaltime = 1;
		strcpy(player.playername, "unknown");
	}

	return player;
}
