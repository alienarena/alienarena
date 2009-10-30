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

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	FILE* file;

	file = fopen( "stats.db", "a" ); //append, don't rewrite
	
	if(file) {
		//write buffer to file
		fprintf(file, buffer);
		fclose(file);
	}
	return strlen(buffer);
}

//get the stats database
void getStatsDB( void )
{  	
	FILE* file;

	CURL* easyhandle = curl_easy_init() ;  

	file = fopen( "stats.db", "w" ); //create new, blank file for writing
	if(file)
		fclose(file);
	
	//to do - make this a cvar
	curl_easy_setopt( easyhandle, CURLOPT_URL, "http://alienenforcer.com/alienarena/stats/playerrank.db" ) ;   
	
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
	file = fopen( "stats.db", "r" ) ;

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
	file = fopen( "stats.db", "r" ) ;

	if(file != NULL) {

		//parse it, and compare to player name
		while(player.ranking < 1000) {
			
			//name
			fgets(name, sizeof(name), file);
			strcpy(player.playername, name);
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