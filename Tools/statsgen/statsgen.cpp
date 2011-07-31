// statsgen.cpp : Defines the entry point for the console application.
//
#define WIN32_LEAN_AND_MEAN
#include "stdafx.h"
#include "stdio.h"
#include <malloc.h>
#include <memory.h>
#include <winsock.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <iomanip> 
#include <fstream>
#include <time.h>
#include <cassert>
#include "fce.h"
#include "keycode.h"

#define NOVERIFY 0

/* With TEST_LOG defined
 *  - lots of info is logged in 'testlog' file
 *    ('testlog' is not opened with append)
 *
 * NO_UPLOAD used to disable FTP upload
 */
#define TEST_LOG 1
#define NO_UPLOAD 1
#define NO_CLANSTATS 1

using namespace std;

#if defined TEST_LOG
/* test/debug log file */
ofstream testlog;
#endif

typedef struct _PLAYERINFO {
	char playername[32];
	int ping;
	int score;
	int rank;
	double points;
	int frags;
	int totalfrags;
	double time;
	double totaltime;
	char ip[16];
	char remote_address[21];
	int poll;
} PLAYERINFO;

typedef struct _SERVERINFO {
	unsigned int ip;
	//char host[256];
	unsigned short port;
	char szHostName[256];
	char szMapName[256];
	int curClients;
	int maxClients;
	DWORD startPing;
	int	ping;
	PLAYERINFO players[64];
	int temporary;
} SERVERINFO;

typedef	struct _CLANINFO {
	char clanname[32];
	PLAYERINFO players[32];
	double totalpoints;
	double totaltime;
	double totalfrags;
	int clannumber;
	int members;
} CLANINFO;

SERVERINFO servers[128];
CLANINFO clans[32];

int numServers = 0;
int numLiveServers = 0;
int currentpoll = 0;
int gracePeriod = 0;
int total_players = 0;
int real_players = 0;
int realDelay = 0;
double playerTally = 0;
double pollTally = 0;
double OLplayerTally = 0;

SOCKET master;

/*
 * Strings for HTML Color
 */
const char* ColorPrefix[] =
	{
		"<font color=\"#000000\">",  // BLACK
		"<font color=\"#FF0000\">",  // RED
		"<font color=\"#00FF00\">",  // GREEN
		"<font color=\"#FFFF00\">",  // YELLOW
		"<font color=\"#0000FF\">",  // BLUE
		"<font color=\"#00FFFF\">",  // CYAN
		"<font color=\"#FF0000\">",  // MAGENTA
		"<font color=\"#FFFFFF\">"   // WHITE
	};

const char ColorSuffix[] = "</font>";


int verify_player(char name[32])
{
	ifstream infile;
	char vName[32] = "go";

#ifdef NOVERIFY
	return true;
#endif

	infile.open("validated");

	if(infile.good())
	{
		while(strlen(vName))
		{
			infile.getline(vName, 32);
			if(!strcmp(name, vName))
			{
				return true;
			}
		}
	}

	return false;	
}

/**
 * Calculate points for sorting by rank
 *
 * totalpoints  accumulated points
 * totaltime    accumulated game time
 * returns      points for ranking 
 */
double RankingPoints( char* playername, double totalpoints, double totaltime )
{
	if ( totalpoints < 0.0 || totaltime < 1.0 )
	{
#if TEST_LOG
		testlog << "db error: " << playername << " ("
			<< totalpoints << ", " << totaltime << ')' << endl;
#endif
		return 0.49; // slightly less than minimum
	}
	// Calculation derived from:
	//  y = mx + b, with m=0.5, and (x,y) = (1,1)
	// "compresses" actualpoints range and favors higher t
	// effectively (points/hour)/2 after ~10 hours
	return (((totalpoints - 1.0)/(2.0 * totaltime)) + 1.0);
}

PLAYERINFO LoadPlayerInfo(PLAYERINFO player)
{	
	ifstream infile;
	ofstream outfile;
	bool foundplayer, spoofed;
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];

	//find this playername in the database, if he doesn't exist, insert him at the bottom
	//of the rankings

	infile.open("playerrank.db");

	player.rank = 1;
	foundplayer = false;
	spoofed = false;
	
	if(verify_player(player.playername))
	{
		if(infile.good()) 
		{
			strcpy(name, "go");

			while(strlen(name)) { //compare name and get stats

				infile.getline(name, 32); //name			
				infile.getline(remote_address, 21); //remote address
				infile.getline(points, 32); //points
				if(!strcmp(player.playername, name))
					player.points = atof(points);
				infile.getline(frags, 32); //frags
				if(!strcmp(player.playername, name))
					player.frags = atoi(frags);
				infile.getline(totalfrags, 32); //total frags
				if(!strcmp(player.playername, name))
					player.totalfrags = atoi(totalfrags);
				infile.getline(time, 16); //current time in poll
				if(!strcmp(player.playername, name)) 
				{
					player.time = atof(time);
					if(player.time > 200) //either a bot, or idling zombie
						player.time = .008; 
				}
				infile.getline(totaltime, 16);
				if(!strcmp(player.playername, name)) 
					player.totaltime = atof(totaltime);
				infile.getline(ip, 16); //last server.ip
				if(!strcmp(player.playername, name)) 
					strcpy(player.ip, ip);
				infile.getline(poll, 16); //what poll was our last?
				if(!strcmp(player.playername, name)) 
				{ 
					player.poll = atoi(poll);
					foundplayer = true;
					break; //get out we are done
				}
				player.rank++;
			}
		}
	}
	else 
	{ 
		spoofed = true;
	}

	infile.close();

	if(!foundplayer) 
	{
		//we didn't find this player, so give him some defaults and insert him
		if(spoofed)
			outfile.open("spoofed.db", ios::app);
		else
			outfile.open("playerrank.db", ios::app);
		if(strlen(player.playername)) 
		{	
			//don't write out blank player names
			outfile << player.playername << endl;
			outfile << "127.0.0.1" << endl;
			outfile << "0" << endl;
			outfile << "0" << endl;
			outfile << "0" << endl;
			outfile << ".008" << endl;
			outfile << "1.0" << endl;
			outfile << "my ip" << endl; 
			outfile << currentpoll << endl;
		}

		outfile.close();
		player.points = 0;
		player.frags = 0;
		player.totalfrags = 0;
		player.totaltime = 1.0;
		player.time = .008;
		player.poll = currentpoll;
		strcpy(player.ip, "my ip");
		strcpy(player.remote_address, "127.0.0.1");
	}
	
	return player;
}
void ReInsertPlayer(PLAYERINFO player)
{
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];
	int inserted;
	ifstream infile;
	ofstream outfile;
	int total_players;
	double rankpoints_player;
	double rankpoints_other;

#if 0
	// don't do this here, player could be in game
	//  moved to CullDatabase()
	if(!strlen(player.playername) || !(verify_player(player.playername)))
		return; //don't bother with assholes who put in blank names or don't have accounts
#endif

	infile.open("playerrank.db");

	outfile.open("temp.db");

	rankpoints_player = RankingPoints( player.playername, player.points, player.totaltime );

	if(infile.good()) {

		strcpy(name, "go");
		inserted = false;
		total_players = 0;
		//the idea of this is, when rebuilding this file, keep it to a reasonable size
		//we only be tracking the top 1000 players then.

		while(strlen(name) && total_players < (1000 + gracePeriod)) {

			infile.getline(name, 32); //name
			infile.getline(remote_address, 21); //remote ip
			infile.getline(points, 32); //points
			infile.getline(frags, 32); //frags
			infile.getline(totalfrags, 32); //total frags
			infile.getline(time, 16); //time
			infile.getline(totaltime, 16); //totaltime
			infile.getline(ip, 16); //ip
			infile.getline(poll, 16); //poll number

			rankpoints_other = RankingPoints( name, atof(points), atof(totaltime) );
			if ( !inserted  && rankpoints_other < rankpoints_player )
			{ //this will add new ranking
				outfile << player.playername << endl;
				outfile << player.remote_address << endl;
				outfile << player.points << endl;
				outfile << player.frags << endl;
				outfile << player.totalfrags << endl;
				outfile << player.time << endl;
				outfile << player.totaltime << endl;
				outfile << player.ip << endl;
				outfile << player.poll << endl;
				inserted = true;
			}
			if(strcmp(name, player.playername) && strlen(name)) 
			{ //this will remove old ranking
				outfile << name << endl;
				outfile << remote_address << endl;
				outfile << points << endl;
				outfile << frags << endl;
				outfile << totalfrags << endl;
				outfile << time << endl;
				outfile << totaltime << endl;
				outfile << ip << endl;
				outfile << poll << endl;
			}
			total_players++;
		}
		if(!inserted) { //worst player ever!
			if(strlen(player.playername)) {
				outfile << player.playername << endl;
				outfile << player.remote_address << endl;
				outfile << player.points << endl;
				outfile << player.frags << endl;
				outfile << player.totalfrags << endl;
				outfile << player.time << endl;
				outfile << player.totaltime << endl;
				outfile << player.ip << endl;
				outfile << player.poll << endl;
			}
		}
		infile.close();
		outfile.close();
	}
	else {
		//couldn't open, don't blow away anything!
		infile.close();
		outfile.close();
		return;
	}
	
	remove("playerrank.db");
	rename("temp.db", "playerrank.db"); //copy new db in to old db's name

	//give brand new players a chance to remain in the top 1000
	gracePeriod++;
	if(gracePeriod > 100)
		gracePeriod = 0; 

	return;
}

void ProcessPlayers(SERVERINFO *server)
{
	static const double points_epsilon = 0.00001;
	int i, j;
	double total_score;
	SOCKADDR_IN addr;
	char server_ip[16];
	int realpeeps;
	double prop_score[64];
	double sum_prop_score;
	double total_score_factor;

	//convert the ip to a string
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.S_un.S_addr = server->ip;
	sprintf (server_ip, "%s", inet_ntoa(addr.sin_addr));

	/*
	 * load the player information for this server's current poll
	 */
	realpeeps   = 0;
	total_score = 0.0;
	for ( j = 0, i = 0; i < server->curClients ; i++ )
	{
		PLAYERINFO player_info;
		double max_time = 0.0;

		if ( server->players[i].ping == 0 )
		{ // cull bots and connecting players early.
			// LoadPlayerInfo() would insert them in database.
			continue;
		}

		// local copy of server's PLAYERINFO record
		player_info = LoadPlayerInfo( server->players[i] );

		if ( player_info.poll != currentpoll-1
			|| strcmp( player_info.ip, server_ip )	)
		{ // first poll on this server for this player
			// set time to minimum
			player_info.time = .008;
		}

		if ( player_info.score > 0 )
		{ // add active, real player to the player list
			// note: overwrites record at or before current 'i'
			server->players[j++] = player_info;
			++realpeeps;
			total_score += (double)player_info.score;
		}
		else if ( player_info.time != 0.008 )
		{ // possibly downloading, idling, or just doing poorly.
			// need to initialize time and reinsert, otherwise
			// time could be 0, screwing up prop_score calc
			// might amount to a do-over when score goes positive.
#if defined TEST_LOG
			testlog << player_info.playername 
				<< " reinsert (" << player_info.time
				<< ',' << player_info.score << ')' << endl;
#endif
			player_info.time  = .008;
			ReInsertPlayer( player_info );
		}
	}
	assert( j == realpeeps );
	if ( realpeeps == 0 )
	{ // early cull if no active, real players in list
		// note: with just 1 player, we still need to update the
		//  database with the player's current frags
		return;
	}
	assert( total_score > 0.0 );

	/*
	 * a factor used to compensate for low total score
	 */
	total_score_factor = 1.0 / total_score;

	/*
	 * Now we can process the list of active, real players
	 * at this point: any player in the list will have score >= 1
	 *   the total score will be >= 2
	 */
	/*
	 * approximate comparable scoring with per player proportional score
	 * effectively, an average over the time player has been in game
	 * then, normalization compensates for scoring differences between
	 *  different game modes
	 * note: the expression: ceil(player.time/.008) is effectively
	 *  the number of polls. The small 0.05 fudge factor compensates for
	 *  small number of polls.
	 */
	sum_prop_score = 0.0;
	for ( i = 0; i < realpeeps ; i++ )
	{
		prop_score[i] = server->players[i].score;
		assert( server->players[i].time >= .008 );
		prop_score[i] *= 1.0 / ( (ceil( server->players[i].time / .008)) + 0.05);
		sum_prop_score += prop_score[i];
	}
	for ( i = 0; i < realpeeps ; i++ )
	{ // normalize. sum of prop_score's will be 1.0
		prop_score[i] /= sum_prop_score;
	}

	/*
	 * calculate points for each player as
	 *  sum of points versus each other player
	 */
	server->players[i].points = 0.0;
	for ( i = 0; i < realpeeps ; i++ )
	{ // for each player in the server's filtered list
#if defined TEST_LOG
		testlog << server->players[i].playername
			<< " (" << prop_score[i] << ")\n";
#endif
		for ( j = 0; j < realpeeps ; j++ )
		{
			double new_points = 0.0;

			if ( i == j )
			{ // self. new_points would be 0 anyway.
				continue;
			}
			if ( server->players[i].time < 0.015 
				|| server->players[j].time < 0.015 )
			{ // require some time in game before getting points
				// prevents inaccuracies and discourages gaming
				// the system.
				new_points = 0.0;
			}
			else if ( server->players[i].rank > server->players[j].rank )
			{ // ranking is 1 highest to 1000 lowest.
				/*
				 * lower ranked player always gets >0 points,
				 * but does not amount to much if close in rank
				 */
				double rank_factor;
				double score_factor;

				rank_factor = (server->players[i].rank - server->players[j].rank)
								/ 1001.0 ;
				if ( rank_factor > 1.0 )
				{ // in case .rank can be > 1000
					rank_factor = 1.0;
				}
				assert( rank_factor >= 0.0 && rank_factor <= 1.0 );

				// score difference adjusted from (-1,1) to (0,2)
				//  total score factor compensates for low scoring games
				score_factor = ( 1.0 + prop_score[i] - prop_score[j] ) - total_score_factor;
				if ( score_factor < 0.0 )
				{ // probably total score too low to be significant
					score_factor = 0.0;
				}
				assert( score_factor >= 0.0 && score_factor <= 2.0 );

				new_points = rank_factor * score_factor;
				if ( new_points < points_epsilon )
				{ // use an epsilon to avoid floating point instability
					new_points = 0.0;
				}
#if defined TEST_LOG
				testlog << "  " << server->players[i].playername << " rf*sf: " << new_points
						<< " vs. " << server->players[j].playername 
						<< "\n";
#endif
				// note: new_points close to 2.0 are extremely improbable
				assert( new_points >= 0.0 && new_points <= 2.0 );
			}
			else if ( prop_score[i] > prop_score[j] )
			{
				new_points = (prop_score[i] - prop_score[j]) - total_score_factor;
				if ( new_points < points_epsilon )
				{ // use an epsilon to avoid floating point instability
					new_points = 0.0;
				}
				assert( new_points >= 0.0 && new_points <= 1.0 );
#if defined TEST_LOG
				testlog << "  " << server->players[i].playername << " ps-ps: " << new_points
						<< " vs. " << server->players[j].playername 
						<< "\n";
#endif
			}

			if ( new_points > 0.0 )
			{ // accumulate new points
				server->players[i].points += new_points;
			}
		} // for each other player
	} // for each player
	
	for ( i = 0; i < realpeeps ; i++ )
	{ // for each player in the server's filtered list, re-insert in database
		// separate loop to be (paranoid) sure that changes don't affect scoring

		//first tally up total frags by comparing with our last poll's number
		// (only if player is currently in consecutive polls)
		if(server->players[i].poll == currentpoll-1) 
		{ 
			if((server->players[i].score - server->players[i].frags) > 0)
			{ // record only positive increment in frags
				// score is current frags for game, frags is previously tallied frags
				server->players[i].totalfrags += (server->players[i].score - server->players[i].frags);
			}
		}
		// update to current frags, poll, time and server ip
		//  and reinsert player into database with new values
		server->players[i].frags = server->players[i].score;
		server->players[i].poll = currentpoll;
		if(realDelay > 30) {
			server->players[i].time += (realDelay * .008/30); 
			server->players[i].totaltime += (realDelay * .008/30); //small increment, real time hours
		}
		else {
			server->players[i].time += (.008); 
			server->players[i].totaltime += (.008); //small increment, real time hours
		}
		strcpy(server->players[i].ip, server_ip); //we are in this server now
#if defined TEST_LOG
		testlog << "reinsert: " << server->players[i].playername 
			<< " (" << server->players[i].points << ")" << endl;
#endif
		ReInsertPlayer(server->players[i]);
	} // for each player
}

void GetServerList (void) {
	
	HOSTENT *hp;
	int i, result;
	struct timeval delay;
	fd_set stoc;
	struct sockaddr_in dgFrom;
	char recvBuff[0xFFFF], *p;

	hp = gethostbyname ("master.corservers.com");
	if (!hp) {
		printf("couldn't resolve master server");
		return; //couldn't resolve master server
	}
	
	memset (recvBuff, 0, sizeof(recvBuff));

	for (i = 0; i < 3; i++) {
		dgFrom.sin_family = AF_INET;
		dgFrom.sin_port = htons (27900);
		memset (&dgFrom.sin_zero, 0, sizeof(dgFrom.sin_zero));
		memcpy (&dgFrom.sin_addr, hp->h_addr_list[0], sizeof(dgFrom.sin_addr));

		result = sendto (master, "query", 5, 0, (const struct sockaddr *)&dgFrom, sizeof (dgFrom));
		if (result == SOCKET_ERROR) {
			return; //couldn't contact master server
		}
		memset (&stoc, 0, sizeof(stoc));
		FD_SET (master, &stoc);
		delay.tv_sec = 2;
		delay.tv_usec = 0;
		result = select (0, &stoc, NULL, NULL, &delay);
		if (result) {
			int fromlen = sizeof(dgFrom);
			result = recvfrom (master, recvBuff, sizeof(recvBuff), 0, (struct sockaddr *)&dgFrom, &fromlen);
			if (result >= 0) {
				break;
			} else if (result == -1) {
				return; //couldn't contact master server
			}
		} 
	}

	if (!result) {
		return; //couldn't contact master server
	}

	p = recvBuff + 12;

	result -=12;

	numServers = 0;

	while (result) {
		servers[numServers].temporary = 1;
		memcpy (&servers[numServers].ip, p, sizeof (servers[numServers].ip));
		p += 4;
		memcpy (&servers[numServers].port, p, sizeof (servers[numServers].port));
		servers[numServers].port = ntohs(servers[numServers].port);
		p += 2;
		result -= 6;

		if (++numServers == 128)
			break;
	}
}

char *GetLine (char **contents, int *len)
{
	int num;
	int i;
	char line[2048];
	char *ret;

	num = 0;
	line[0] = '\0';

	if (*len <= 0)
		return NULL;

	for (i = 0; i < *len; i++) {
		if ((*contents)[i] == '\n') {
			*contents += (num + 1);
			*len -= (num + 1);
			line[num] = '\0';
			ret = (char *)malloc (sizeof(line));
			strcpy (ret, line);
			return ret;
		} else {
			line[num] = (*contents)[i];
			num++;
		}
	}

	ret = (char *)malloc (sizeof(line));
	strcpy (ret, line);
	return ret;
}

void PingServers (SERVERINFO *server)
{
	char *p, *rLine;
	char lasttoken[256];
	char recvBuff[4096];
	fd_set stoc;
	int i;
	int result;
	int fromlen;
	struct sockaddr dgFrom;
	TIMEVAL delay;
	SOCKADDR_IN addr;
	SOCKET	s;
	char request[] = { "\xFF\xFF\xFF\xFFstatus\n" };
	char seps[]   = "\\";
	char *token;
	int players = 0;

	memset(&addr, 0, sizeof(addr));
	memset(&recvBuff, 0, sizeof(recvBuff));
	addr.sin_family = AF_INET;
	addr.sin_port = 0;

	addr.sin_addr.S_un.S_addr = server->ip;
	addr.sin_port = htons(server->port);

	s = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	for (i = 0; i < 2; i++) {
		result = sendto (s, request, sizeof(request), 0, (struct sockaddr *)&addr, sizeof(addr));
		if (result == SOCKET_ERROR) {
			printf ("Can't send: error %d", WSAGetLastError());
		}
		else {
			
			memset (&stoc, 0, sizeof(stoc));
			FD_SET (s, &stoc);
			delay.tv_sec = 1;
			delay.tv_usec = 0;
			result = select (0, &stoc, NULL, NULL, &delay);
			if (result) {
				fromlen = sizeof(dgFrom);
				result = recvfrom (s, recvBuff, sizeof(recvBuff), 0, &dgFrom, &fromlen);
			} else {
				result = -1;
			}
		}
	}
	if (result >= 0) {

		p = recvBuff;

		//discard print
		rLine = GetLine (&p, &result);
		free (rLine);

		//serverinfo
		rLine = GetLine (&p, &result);

		/* Establish string and get the first token: */
		token = strtok( rLine, seps );
		while( token != NULL ) {
			/* While there are tokens in "string" */
			if (!_stricmp (lasttoken, "mapname"))
				strcpy (server->szMapName, token);
			else if (!_stricmp (lasttoken, "maxclients"))
				server->maxClients = atoi(token);
			else if (!_stricmp (lasttoken, "hostname"))
				strcpy (server->szHostName, token);

			/* Get next token: */
			strcpy (lasttoken, token);
			token = strtok( NULL, seps );
		}
#if defined TEST_LOG
		testlog << "\nSERVER: " << inet_ntoa (addr.sin_addr) << endl;
#endif
		free (rLine);

		//playerinfo
		strcpy (seps, " ");
		while (rLine = GetLine (&p, &result)) {
#if defined TEST_LOG
			testlog << rLine << endl;
#endif
			/* Establish string and get the first token: */
			token = strtok( rLine, seps);
			server->players[players].score = atoi(token);

			token = strtok( NULL, seps);
			server->players[players].ping = atoi(token);
			if(server->players[players].ping > 0)
			{
				real_players++;
			}

			token = strtok( NULL, "\"");
			if (token)
				strncpy (server->players[players].playername, token, sizeof(server->players[players].playername)-1);
			else
				server->players[players].playername[0] = '\0';

			token = strtok( NULL, "\"");
			token = strtok( NULL, "\"");
			if (token) 
				strncpy (server->players[players].remote_address, token, sizeof(server->players[players].remote_address)-1);
			else
				strncpy (server->players[players].remote_address, "127.0.0.1", sizeof(server->players[players].remote_address)-1);

			players++;
			free (rLine);
		}

		server->curClients = players;
		total_players += players;
		numLiveServers++;

		//process this server's players
		ProcessPlayers(server);
	
	} else {
		//printf ("No response from %s:%d", inet_ntoa (addr.sin_addr), server->port);
	}
	if (s)
        closesocket(s);

	return;
}

/**
 * Filter the database by a culling criteria
 *  Note that records are not removed while player is active in game.
 *  Also, the html generation needs to apply similar criteria
 *   to filter out such records from the stats html pages.
 */
void CullDatabase( void )
{
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];
	ifstream infile;
	ofstream outfile;
	int input_count;
	int output_count;

	outfile.open( "temp.db", ofstream::out );
	if ( outfile.fail() )
	{
#if defined TEST_LOG
		testlog << "\nCullDatabase: temp.db open failed" << endl;
#endif
		return;
	}
	infile.open("playerrank.db", ifstream::in );
	if ( infile.fail() )
	{
#if defined TEST_LOG
		testlog << "\nCullDatabase: playerrank.db open failed" << endl;
#endif
		outfile.close();
		return;
	}
	outfile.clear();
	infile.clear();

	output_count = input_count = 0;

#if defined TEST_LOG
	testlog << "\nDATABASE CULL" << endl;
#endif
	// keep only player records that are significant
	for(;;)
	{
		// get each line of a player record
		infile.getline(name, 32);
		infile.getline(remote_address, 21);
		infile.getline(points, 32);
		infile.getline(frags, 32);
		infile.getline(totalfrags, 32);
		infile.getline(time, 16);
		infile.getline(totaltime, 16);
		infile.getline(ip, 16);
		infile.getline(poll, 16);
		if ( infile.eof() )
		{ // end of input file, all done
			infile.clear();
			break;
		}
		++input_count;

		// Culling Criteria
		if ( atoi(poll) != currentpoll )
		{ // not currently in game. (that would screw up point calcs)
			if ( !strlen( name) )
			{
#if defined TEST_LOG
				testlog << "asshole culled for blank name" << endl;
#endif
				continue;
			}
			if ( !verify_player( name ) )
			{
#if defined TEST_LOG
				testlog << "  " << name << " culled for verify failure" << endl;
#endif
				continue;
			}
			if ( !_strnicmp( name, "Player", 6 ) )
			{ // using a default name
				/* technically, this should check for digits
				 * following "Player" and be case-sensitive.
				 * But, usually, any name starting with "Player" in
				 * any form is just clutter and screws up ranking.
				 */
#if defined TEST_LOG
				testlog << "  " << name << " culled for name." << endl;
#endif
				continue;
			}
			if ( atof(points) <= 0.0  )
			{ // has no points
				/* so no point in being in the database
				**  often single-player only players
				**  or, so far, only played online against bots
				*/
#if defined TEST_LOG
				testlog << "  " << name << " culled for points: " << points << endl;
#endif
				continue;
			}
		}

		// keep this player record
		outfile << name << '\n';
		outfile << remote_address << '\n';
		outfile << points << '\n';
		outfile << frags << '\n';
		outfile << totalfrags << '\n';
		outfile << time << '\n';
		outfile << totaltime << '\n';
		outfile << ip << '\n';
		outfile << poll << endl;
		++output_count;

	}
	if ( !infile.bad() && !outfile.bad() )
	{ // close and update
		infile.close();
		outfile.close();
		remove("playerrank.bak");
		rename("playerrank.db", "playerrank.bak" );
		rename("temp.db", "playerrank.db");
#if defined TEST_LOG
		testlog << "Database cull complete."  
			<< "( " << input_count << ", "
			<< output_count << " )" << endl;
#endif
	}
	else
	{ // just close
		infile.close();
		outfile.close();
#if defined TEST_LOG
		testlog << "Database cull failed." << endl;
#endif
	}
}

/**
 * Quake Color Escape Codes
 */
inline bool IsColorString( const char* s )
{
	return (   ( s     != NULL)
			&& (*s     == '^' )
			&& (*(s+1) != '\0')
			&& (*(s+1) != '^' ));
}

inline int ColorIndex( const char* c )
{
	return (int)( (*c - '0') & 7 );

}

/**
 * Translate Quake Color Escapes to HTML tags
 */
bool
ColorizePlayerName ( const char *src, char *dst, size_t dstsize )
{
	int icolor;
	int jcolor;
	size_t char_count    = 0;
	size_t substring_len = 0;
	bool prefixed        = false;
	char *prefix_begin   = dst;
	size_t prefix_len    = 0;
	size_t suffix_len    = strlen( ColorSuffix );

	icolor = jcolor = 7; // white
	do
	{
		if ( IsColorString( src ) )
		{
			++src;
			jcolor = ColorIndex( src );
			++src;
			if ( jcolor != icolor )
			{ // color change.
				if ( prefixed )
				{ // suffix for previous colored substring
					if ( substring_len == 0 )
					{ // pointless color change, back up.
						char_count -= prefix_len;
						dst = prefix_begin;
						prefixed = false;
					}
					else
					{ // append suffix
						char_count += suffix_len;
						if ( char_count < dstsize )
						{
							strncpy( dst, ColorSuffix, suffix_len );
							dst += suffix_len;
						}
						else
						{
							return false;
						}
					}
				}
				char_count += strlen( ColorPrefix[jcolor] );
				if ( char_count < dstsize )
				{ // append prefix for new color
					prefixed = true;
					prefix_begin = dst;
					prefix_len = strlen( ColorPrefix[jcolor] );
					strncpy( dst, ColorPrefix[jcolor], prefix_len );
					icolor = jcolor;
					dst += prefix_len;
					substring_len = 0;
				}
				else
				{
					return false;
				}
			}
			// else redundant color change, do nothing
		}
		else
		{ // visible character
			if ( ++char_count < dstsize )
			{
				*dst++ = *src++;
				++substring_len;
			}
			else
			{
				return false;
			}
		}
	} while ( *src );
	if ( prefixed )
	{ // trailing suffix
		if ( substring_len > 0 )
		{
			char_count += suffix_len;
			if ( char_count < dstsize )
			{
				strncpy( dst, ColorSuffix, suffix_len );
				dst += suffix_len;
			}
			else
			{
				return false;
			}
		}
		else
		{ // pointless trailing color change
			dst = prefix_begin;
		}
	}
	if ( char_count >= dstsize )
	{
		return false;
	}
	*dst = '\0';

	return true;
}

void GeneratePlayerRankingHtml(void) //Top 1000 players
{
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];
	ifstream infile;
	ofstream outfile;
	int rank;
	double npoints;
	double ntotalfrags;
	double ntotaltime;
	double fragrate, actualtime, actualpoints;
	char a_string[1024];
	int page, currentPos;
	int	closed = false;
	SYSTEMTIME stime;

	printf("Generating html.\n");

	GetSystemTime(&stime);

	infile.open("playerrank.db");

	if(infile.good()) {

		strcpy(name, "go");
		rank = 1;
		currentPos = 1;

		while(strlen(name) && rank <= currentPos) {

			page = rank/100 + 1; //will automatically truncate
			
			if(rank == currentPos) {
				currentPos += 100;
				if(currentPos == 1101) //done
					currentPos = 0;

				//open a new file
				sprintf(a_string, "stats%i.html", page);
				outfile.open(a_string);

				//build header
				outfile << "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">" << endl;
				outfile << "<html>" << endl;
				outfile << "<head>" << endl;
				outfile << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">" << endl;
				outfile << "<meta name=\"Author\" content=\"John Diamond\">" << endl;
				outfile << "<meta name=\"GENERATOR\" content=\"Mozilla/4.78 [en] (Windows NT 5.0; U) [Netscape]\">" << endl;
				outfile << "<title>Alien Arena Global Stats</title>" << endl;
				outfile << "</head>" << endl;
				outfile << "<body style=\"color: rgb(255, 255, 255); background-color: rgb(153, 153, 153); background-image: url(images/default_r4_c5.jpg);\" alink=\"#000099\" link=\"#ffcc33\" vlink=\"#33ff33\">" << endl;
				outfile << "<center><b><font face=\"Arial,Helvetica\">ALIEN ARENA GLOBAL STATS</font></b>" << endl;
				sprintf(a_string, "<p><b><font face=\"Arial,Helvetica\"><font size=-2>Updated %i/%i/%i %i:%i GMT</font></font></b></center>", stime.wMonth, stime.wDay, stime.wYear, stime.wHour, stime.wMinute);
				outfile << a_string << endl;
				outfile << "<p><br>" << endl;
				outfile << "<center><table BORDER CELLSPACING=0 WIDTH=\"400\" >" << endl;
				outfile << "<tr>" << endl;
				outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>RANK</font></font></td>" << endl;
				outfile << "<td WIDTH=\"100\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>NAME</font></font></td>" << endl;
				outfile << "<td WIDTH=\"20\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>POINTS</font></font></td>" << endl;
				outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGS</font></font></td>" << endl;
				outfile << "<td WIDTH=\"20\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>TIME</font></font></td>" << endl;
				outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGRATE</font></font></td>" << endl;
				outfile << "</tr>" << endl;

			}

			infile.getline(name, 32); //name
			infile.getline(remote_address, 21); //remote address
			infile.getline(points, 32); //points
			infile.getline(frags, 32); //frags
			infile.getline(totalfrags, 32); //total frags
			infile.getline(time, 16); //current time in poll
			infile.getline(totaltime, 16);
			infile.getline(ip, 16); //last server.ip
			infile.getline(poll, 16); //what poll was our last?

			ntotaltime  = atof( totaltime );
			ntotalfrags = atof( totalfrags );
			npoints     = atof( points );
			actualtime  = ntotaltime - 1.0;
			if ( actualtime < 0.015 || ntotalfrags <= 0.0 || npoints < 0.00001 )
			{ // cull out minimum time, zero frags, minimal points
				// to help keep floating point math stable
				continue;
			}
			if ( !strlen( name)  
				|| !verify_player( name ) 
				|| !_strnicmp( name, "Player", 6 ) )
			{ // cull out blank name, verify fail, default name
				// things in database only while player is in game
				continue;
			}

			//build row for this player
			//  with calculated points and fragrate
			actualpoints = RankingPoints( name, npoints, ntotaltime );
			fragrate     = ntotalfrags / actualtime;
			outfile << "<tr>" << endl;
			sprintf(a_string, "<td>%i</td>", rank);
			outfile << a_string << endl;

			outfile << "<td>" ;
			if ( ColorizePlayerName( name, a_string, sizeof(a_string) ))
			{
				outfile << a_string ;
			}
			else
			{ // colorize error
				outfile << name ;
			}
			outfile << "</td>" ;

			// TODO: verify format.
			//  see RankingPoints()
			sprintf(a_string, "<td>%3.3f</td>", actualpoints);

			outfile << a_string << endl;
			sprintf(a_string, "<td>%s</td>", totalfrags);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%4.2f hours</td>", actualtime);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%4.2f frags/hr</td>", fragrate);
			outfile << a_string << endl;
			outfile << "</tr>" << endl;

			rank++;
			if(rank == currentPos) {
				outfile << "</table>" << endl;
				sprintf(a_string, "<p><font face=\"Arial,Helvetica\"><font size=-1><a href=\"stats%i.html\">Page %i</a></font></font></center>", page, page);
				outfile << a_string << endl; 
				outfile << "</body>" << endl;
				outfile << "</html>" << endl;

				outfile.close();

				closed = true;
			}
		}
	}

	if(!closed) { //in other words, we don't have the full 1000 names in the db yet
		outfile << "</table>" << endl;
		sprintf(a_string, "<p><font face=\"Arial,Helvetica\"><font size=-1><a href=\"stats%i.html\">Page %i</a></font></font></center>", page, page);
		outfile << a_string << endl; 
		outfile << "</body>" << endl;
		outfile << "</html>" << endl;

		outfile.close();
	}
	infile.close();

	return;
}
void BuildClanDb(void) //take our clan information and build a database in memory
{
	char name[32];
	ifstream infile;
	int i, j;

	//initialize data struct
	for(i=0; i<32; i++) {
		clans[i].clanname[0] = 0;
		for(j=0; j<32; j++) {
			clans[i].players[j].playername[0] = 0;
		}
		clans[i].totalpoints = 0;
		clans[i].totalfrags = 0;
		clans[i].totaltime = 0;
		clans[i].members = 0;
		
	}
	i = 0;

	infile.open("clans.db");

	if(infile.good()) { //read in the clan db into a data structure

		strcpy(name, "go");

		while(strlen(name)) {

			infile.getline(name, 32);

			if(!strcmp(name, "clan")) {
				infile.getline(clans[i].clanname, 32);
				j = 0;
				while(strcmp(name, "end")) {
					infile.getline(name, 16);
					if(strcmp(name, "end")) {
						strcpy(clans[i].players[j].playername, name);
					}
					j++;
					if(j>31)
						break;
				}
			}
			i++;
		}
	}
	infile.close();
	return;
}

void GenerateClanHtml(int i) {
	
	ofstream outfile;
	char filename[32];
	char a_string[1024];
	SYSTEMTIME stime;
	int j;
	
	GetSystemTime(&stime);

	sprintf(filename, "clan%i.html", i);
	outfile.open(filename);

	//build header
	outfile << "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">" << endl;
	outfile << "<html>" << endl;
	outfile << "<head>" << endl;
	outfile << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">" << endl;
	outfile << "<meta name=\"Author\" content=\"John Diamond\">" << endl;
	outfile << "<meta name=\"GENERATOR\" content=\"Mozilla/4.78 [en] (Windows NT 5.0; U) [Netscape]\">" << endl;
	outfile << "<title>Alien Arena Global Stats</title>" << endl;
	outfile << "</head>" << endl;
	outfile << "<body style=\"color: rgb(255, 255, 255); background-color: rgb(153, 153, 153); background-image: url(images/default_r4_c5.jpg);\" alink=\"#000099\" link=\"#ffcc33\" vlink=\"#33ff33\">" << endl;
	sprintf(a_string, "<center><b><font face=\"Arial,Helvetica\">ALIEN ARENA GLOBAL %s</font></b>", clans[i].clanname);
	outfile << a_string << endl;
	sprintf(a_string, "<p><b><font face=\"Arial,Helvetica\"><font size=-2>Updated %i/%i/%i %i:%i GMT</font></font></b></center>", stime.wMonth, stime.wDay, stime.wYear, stime.wHour, stime.wMinute);
	outfile << a_string << endl;
	outfile << "<p><br>" << endl;
	outfile << "<center><table BORDER CELLSPACING=0 WIDTH=\"400\" >" << endl;
	outfile << "<tr>" << endl;
	outfile << "<td WIDTH=\"100\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>NAME</font></font></td>" << endl;
	outfile << "<td WIDTH=\"20\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>POINTS</font></font></td>" << endl;
	outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGS</font></font></td>" << endl;
	outfile << "<td WIDTH=\"20\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>TIME</font></font></td>" << endl;
	outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGRATE</font></font></td>" << endl;
	outfile << "</tr>" << endl;

	for(j=0; j<32; j++) {
		if(strlen(clans[i].players[j].playername)) {
									
			//build row for this player
			sprintf(a_string, "<td>%s</td>", clans[i].players[j].playername);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%4.2f</td>", 100*clans[i].players[j].points/clans[i].players[j].totaltime);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%i</td>", clans[i].players[j].totalfrags);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%4.2f hours</td>", clans[i].players[j].totaltime - 1);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%4.2f frags/hr</td>", clans[i].players[j].totalfrags/(clans[i].players[j].totaltime-1));
			outfile << a_string << endl;
			outfile << "</tr>" << endl;
		}
	}
	outfile << "</table>" << endl;
	outfile << "</body>" << endl;
	outfile << "</html>" << endl;

	outfile.close();
	return;
}
	
void InsertClanIntoDb(int i) { //this will give us a top 32 clans database in order of ranking

	char name[32], totalfrags[32], totalpoints[32], totaltime[32], clannumber[16], members[16];
	ifstream infile;
	ofstream outfile;
	int inserted;

	infile.open("sorted.db");

	outfile.open("temp.db");

	if(infile.good()) {

		strcpy(name, "go");
		inserted = false;
	
		while(strlen(name)) {

			infile.getline(name, 32); //clanname
			infile.getline(totalpoints, 32); //points
			infile.getline(totaltime, 32); //time
			infile.getline(totalfrags, 32); //total frags
			infile.getline(clannumber, 16); //clan number
			infile.getline(members, 16); //how many members
		
			if((clans[i].totalpoints/clans[i].totaltime >= atof(totalpoints)/atof(totaltime)) && !inserted) { //this will add new ranking
				outfile << clans[i].clanname << endl;
				outfile << clans[i].totalpoints << endl;
				outfile << clans[i].totaltime << endl;
				outfile << clans[i].totalfrags << endl;
				outfile << i << endl;
				outfile << clans[i].members << endl;
				inserted = true;
			}
			if(strcmp(name, clans[i].clanname) && strlen(name)) { //this will remove old ranking
				outfile << name << endl;
				outfile << totalpoints << endl;
				outfile << totaltime << endl;
				outfile << totalfrags << endl;
				outfile << clannumber << endl;
				outfile << members << endl;
			}
		}
		if(!inserted) { //bottom feeder!
			if(strlen(clans[i].clanname)) {
				outfile << clans[i].clanname << endl;
				outfile << clans[i].totalpoints << endl;
				outfile << clans[i].totaltime << endl;
				outfile << clans[i].totalfrags << endl;
				outfile << i << endl;
				outfile << clans[i].members << endl;
			}
		}
	}
	infile.close();
	outfile.close();

	remove("sorted.db");
	rename("temp.db", "sorted.db"); //copy new db in to old db's name

	return;
}
void GetClanMemberInfo(void) 
{
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];
	ifstream infile;
	int i, j;

	infile.open("playerrank.db");

	strcpy(name, "go");
	
	while(strlen(name)) {

		infile.getline(name, 32); //name
		infile.getline(remote_address, 21); //remote address
		infile.getline(points, 32); //points
		infile.getline(frags, 32); //frags
		infile.getline(totalfrags, 32); //total frags
		infile.getline(time, 16); //current time in poll
		infile.getline(totaltime, 16);
		infile.getline(ip, 16); //last server.ip
		infile.getline(poll, 16); //what poll was our last?

		//find it in the clan db and add in that player's data
		for(i=0; i<32; i++) {
			if(strlen(clans[i].clanname)) {
				for(j=0; j<32; j++) {		
					if(!strcmp(name, clans[i].players[j].playername)) {
						clans[i].totalfrags += atof(totalfrags);
						clans[i].totaltime += atof(totaltime);
						if(clans[i].totaltime <= 0)
							clans[i].totaltime = 1;
						clans[i].totalpoints += atof(points);
						//be sure to insert the actual players data too
						clans[i].players[j].points = atof(points);
						clans[i].players[j].totalfrags = atoi(totalfrags);
						clans[i].players[j].totaltime = atof(totaltime);
						if(clans[i].players[j].totaltime <= 0)
							clans[i].players[j].totaltime = 1;
					}
				}		
			}
		}
	}
	infile.close();
	return;
}

void GenerateClanRankingHtml(void) //Clan Rankings
{
	char name[32], points[32], totalfrags[32], totaltime[16], clannumber[16], members[16];
	ifstream infile;
	ofstream outfile;
	char a_string[1024];
	SYSTEMTIME stime;
	int i;
	int j;
	int rank;
	double actualtime, actualpoints, fragrate;

	printf("Generating clan html.\n");

	j = 0;
	i = 0;

	GetSystemTime(&stime);

	BuildClanDb();

	GetClanMemberInfo();

	//find valid players with valid time, count members
	for(i=0; i<32; i++){
		if(strlen(clans[i].clanname)) {
			for(j=0; j<32; j++) {
				if(clans[i].players[j].totaltime > 1)
					clans[i].members++;
			}
		}
	}

	for(i=0; i<32; i++) {
		if(strlen(clans[i].clanname)) {
			InsertClanIntoDb(i);
			GenerateClanHtml(i);
		}
	}
	
	//generate the html page

	//open a new file for top 32 clans
	outfile.open("clanrank.html");

	//build header
	outfile << "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">" << endl;
	outfile << "<html>" << endl;
	outfile << "<head>" << endl;
	outfile << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">" << endl;
	outfile << "<meta name=\"Author\" content=\"John Diamond\">" << endl;
	outfile << "<meta name=\"GENERATOR\" content=\"Mozilla/4.78 [en] (Windows NT 5.0; U) [Netscape]\">" << endl;
	outfile << "<title>Alien Arena Global Stats</title>" << endl;
	outfile << "</head>" << endl;
	outfile << "<body style=\"color: rgb(255, 255, 255); background-color: rgb(153, 153, 153); background-image: url(images/default_r4_c5.jpg);\" alink=\"#000099\" link=\"#ffcc33\" vlink=\"#33ff33\">" << endl;
	outfile << "<center><b><font face=\"Arial,Helvetica\">ALIEN ARENA GLOBAL CLAN STATS</font></b>" << endl;
	sprintf(a_string, "<p><b><font face=\"Arial,Helvetica\"><font size=-2>Updated %i/%i/%i %i:%i GMT</font></font></b></center>", stime.wMonth, stime.wDay, stime.wYear, stime.wHour, stime.wMinute);
	outfile << a_string << endl;
	outfile << "<p><br>" << endl;
	outfile << "<center><table BORDER CELLSPACING=0 WIDTH=\"400\" >" << endl;
	outfile << "<tr>" << endl;
	outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>RANK</font></font></td>" << endl;
	outfile << "<td WIDTH=\"100\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>NAME</font></font></td>" << endl;
	outfile << "<td WIDTH=\"20\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>POINTS</font></font></td>" << endl;
	outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGS</font></font></td>" << endl;
	outfile << "<td WIDTH=\"20\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>TIME</font></font></td>" << endl;
	outfile << "<td WIDTH=\"10\" BGCOLOR=\"#CCCCCC\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGRATE</font></font></td>" << endl;
	outfile << "</tr>" << endl;

	infile.open("sorted.db");

	if(infile.good()) {

		strcpy(name, "go");
		rank = 1;

		while(strlen(name)) {
			
			infile.getline(name, 32); //name
			infile.getline(points, 32); //points
			infile.getline(totaltime, 16);
			infile.getline(totalfrags, 32); //total frags
			infile.getline(clannumber, 16); //clan number
			infile.getline(members, 16); //found clan members
			
			actualtime = atof(totaltime);
			fragrate = atof(totalfrags)/actualtime;
			actualpoints = 100*atof(points)/atof(totaltime);

			if(strlen(name)) {
				//build row for this clan
				outfile << "<tr>" << endl;
				sprintf(a_string, "<td>%i</td>", rank);
				outfile << a_string << endl;
				sprintf(a_string, "<td><a href=\"clan%s.html\">%s</a></td>", clannumber, name);
				outfile << a_string << endl;
				sprintf(a_string, "<td>%4.2f</td>", actualpoints);
				outfile << a_string << endl;
				sprintf(a_string, "<td>%s</td>", totalfrags);
				outfile << a_string << endl;
				sprintf(a_string, "<td>%4.2f hours</td>", actualtime  -  atof(members));
				outfile << a_string << endl;
				sprintf(a_string, "<td>%4.2f frags/hr</td>", fragrate);
				outfile << a_string << endl;
				outfile << "</tr>" << endl;
			}
			rank++;
		}
		outfile << "</table>" << endl;
		outfile << "</body>" << endl;
		outfile << "</html>" << endl;

		outfile.close();

	}
	infile.close();

	return;

}
void BackupStats(void) { //backup main player database

	char backupfile[32];
	char line[32];
	ofstream outfile;
	ifstream infile;
	SYSTEMTIME st;

	GetSystemTime(&st);

	sprintf(backupfile, "backup/day%ihour%i.db", st.wDayOfWeek, st.wHour);
	infile.open("playerrank.db");
	outfile.open(backupfile);
	strcpy(line, "go");
	while(strlen(line)) {
		infile.getline(line, 32);
		outfile << line << endl;
	}
	infile.close();
	outfile.close();

	return;
}
void UploadStats(void) { //put all updated files on the server

	int error;
	int i;
	char a_string[16];
	FILE *filename;

#if defined NO_UPLOAD
	cout << "\n!No FTP Upload!" << endl;
#endif

#if defined TEST_LOG
	// being extra cautious. should not occur.
	testlog << "Not uploading to ftp server!" << endl;
	return;
#endif

	printf("Uploading to ftp server.\n");

	fceSetInteger(0, FCE_SET_PASSIVE, 1);
	fceSetInteger(0, FCE_SET_CONNECT_WAIT, 1000);
    fceSetInteger(0, FCE_SET_MAX_RESPONSE_WAIT, 1000);
	
	// Connect to FTP server
	error = fceConnect(0,"ftp.martianbackup.com","user","password");
	if(error < 0) {
		printf("Error connecting to host!\n");
		return;
	}
	else 
		printf("Connected Succesfully!\n");

	//change to correct dir if needed
	/*error = fceSetServerDir (0, "/public_html");
	if(error < 0) {
		printf("Error changing directory!\n");
		fceClose(0);
		return;
	}*/	

	error = fcePutFile(0, "playerrank.db");
	if(error < 0) {
		printf("Error uploading player database file!\n");
		fceClose(0);
		return;
	}
	else
		printf("Player database successfully uploaded!\n");
	
	error = fceSetServerDir (0, "aastats");
	if(error < 0) {
		printf("Error changing directory!\n");
		fceClose(0);
		return;
	}

	for(i = 1; i < 11; i++) {
		sprintf(a_string, "stats%i.html", i);
		error = fcePutFile(0, a_string);
		if(error < 0) {
			printf("Error uploading stats file: %i!\n", i);
			fceClose(0);
			return;
		}
	}
	printf("Player stats successfully uploaded!\n");

	error = fcePutFile(0, "clanrank.html");
	if(error < 0) {
		printf("Error uploading clan ranking file!\n", i);
		fceClose(0);
		return;
	}
	else
		printf("Clan ranking successfully uploaded!\n");

	for(i = 0; i < 32; i++) {
		sprintf(a_string, "clan%i.html", i);

		filename = fopen(a_string, "r");
		if(filename) {
			error = fcePutFile(0, a_string);
			if(error < 0) {
				printf("Error uploading clan file: %i!\n", i);
				fceClose(0);
				fclose(filename);
				return;
			}
		}
		else
			continue;
		fclose(filename);
	}
	printf("Clan stats successfully uploaded!\n");

	fceClose(0);

}

void LoadPollNumber(void) 
{
	ifstream infile;
	char poll[16];

// TODO: if poll.db not opened. scan playerrank.db for last poll
//  and create poll.db from there
	
	infile.open("poll.db");

	if(infile.good()) {
		infile.getline(poll, 16);
		currentpoll = atoi(poll);
	}
	infile.close();

	return;
}

void RecordPollNumber(void) 
{
	ofstream outfile;

	outfile.open("poll.db");
	outfile << currentpoll <<endl;
	outfile.close();

	return;
}

void CheckInactivePlayers(void)
{
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];
	ifstream infile;
	ofstream outfile;
	int diff;
	double pointnumber;
	PLAYERINFO player[1101]; //very unlikely to ever use all of this
	int i, j;
	int player_poll;

	i = 0;

	infile.open("playerrank.db");

	if(infile.good()) {
		
		strcpy(name, "go");
	
		while(strlen(name)) {

			infile.getline(name, 32); //name
			infile.getline(remote_address, 21); //remote address
			infile.getline(points, 32); //points
			pointnumber = atof(points); //so we can manipulate this
			infile.getline(frags, 32); //frags
			infile.getline(totalfrags, 32); //total frags
			infile.getline(time, 16); //current time in poll
			infile.getline(totaltime, 16);
			infile.getline(ip, 16); //last server.ip
			infile.getline(poll, 16); //what poll was our last?

			player_poll = atoi(poll);
			diff = currentpoll - player_poll;
			if ( diff < 0 )
			{ // currentpoll wrapped around at 100000
				if ( player_poll < 100001 )
				{
					diff += 100000;
				}
			}
			if ( player_poll > 100000 || diff > 20160 )
			{ // start aging after a week with no play
				pointnumber *= .90; //start dropping each day
				if ( pointnumber < 1.0 )
				{ // cutoff point, avoid aging almost forever.
					pointnumber = 0.0;
				}
				//add to data list
				strcpy(player[i].playername, name);
				strcpy(player[i].remote_address, remote_address);
				player[i].points = pointnumber;
				player[i].frags = atoi(frags);
				player[i].totalfrags = atoi(totalfrags);
				player[i].time = atof(time);
				player[i].totaltime = atof(totaltime);
				strcpy(player[i].ip, ip);
				// TODO: Following Trick To Be Tested
				player[i].poll = 100001; // age until player plays again
				i++;
#if defined TEST_LOG
				testlog << "aging " << player[i].playername  
					<< " (" << player[i].points << ")" << endl;
#endif
			}
		}
		infile.close();
		for(j=0; j<i; j++) { //reinsert each player into the db, so that they are ordered
			ReInsertPlayer(player[j]);
		}

	}

	return;
}
void RepairDb(void) {
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16];
	ifstream infile;
	ofstream outfile;

	infile.open("playerrank.db");
	outfile.open("newdb.db");

	if(infile.good()) {
		
		strcpy(name, "go");
	
		while(strlen(name)) {

			infile.getline(name, 32); //name
			infile.getline(points, 32); //points
			infile.getline(frags, 32); //frags
			infile.getline(totalfrags, 32); //total frags
			infile.getline(time, 16); //current time in poll
			infile.getline(totaltime, 16);
			infile.getline(ip, 16); //last server.ip
			infile.getline(poll, 16); //what poll was our last?

			outfile << name << endl;
			outfile << "127.0.0.1" << endl;
			outfile << points << endl;
			outfile << frags << endl;
			outfile << totalfrags << endl;
			outfile << time << endl;
			outfile << totaltime << endl;
			outfile << ip << endl;
			outfile << poll << endl; 

		}
		infile.close();
		outfile.close();
	}
}

int main(int argc, char* argv[])
{
	WSADATA ws;
	int error, i;
	SYSTEMTIME st_poll;
	SYSTEMTIME st_tally;
	SYSTEMTIME st_generate;
	SYSTEMTIME st_decay;
	SYSTEMTIME st_backup;
	DWORD loop_t0;
	DWORD loop_t1;
	DWORD loop_msecs;

	//just stick this here

	//RepairDb();
	//return 0;

#if defined TEST_LOG
	testlog.open("testlog", ofstream::out );
	testlog << "STATSGEN TESTLOG" << endl;
#endif

	WSACleanup(); //Cleanup Winsock - not sure if this needed, but can it hurt?
	
	//initialize Winsock
	error = WSAStartup ((WORD)MAKEWORD (1,1), &ws);

	if (error) {
		printf("Couldn't load Winsock!");
		return 0;
	}
	
	//open a socket for polling the master
	master = socket (AF_INET, SOCK_DGRAM, 0);

#if !defined NO_UPLOAD
	// Initialize FCE (look in KEYCODE.H for FCE_KEY_CODE)
	fceAttach(1, FCE_KEY_CODE);
#endif

	//load the current poll
	LoadPollNumber();

	// initialize time and timing variables
	GetSystemTime( &st_poll );
	st_tally = st_generate = st_backup = st_decay = st_poll;
	loop_t0 = GetTickCount();
	while(1) {

		printf("Polling master.  Current poll: %i\n", currentpoll);
		GetServerList(); //poll the master
		currentpoll++;   // advance the poll sequence number
		if(currentpoll > 100000)
			currentpoll = 1;
		pollTally++;

		/*
		 * Collect data from servers at nominal 30 second intervals
		 * TODO: !currently running at > 30 second intervals!
		 * Appears to be mostly due to waiting for server responses
		 * Some ideas:
		 *   Send the next server status request right
		 *    after receiving current server data.
		 *   Or change poll interval to 60sec nominal.
		 */
		loop_t1 = GetTickCount();
		if ( loop_t0 > loop_t1 )
		{ // 49 day wrap, just re-init
			loop_t1 = loop_t0 = GetTickCount();
		}
		assert( loop_t1 >= loop_t0 );
		loop_msecs = loop_t1 - loop_t0;
		if ( loop_msecs < 28800 )
		{
			Sleep( 28800 - loop_msecs );
			loop_msecs = 30000;
		}
		realDelay = ((int)loop_msecs) / 1000; // poll interval in seconds
		loop_t0 = loop_t1;
#if defined TEST_LOG
		if ( realDelay > 60 )
			testlog << "poll interval: " << realDelay << endl;
#endif

		// get UTC wall clock time (used below for timed actions)
		GetSystemTime( &st_poll );
		cout << "UTC: " 
			<< st_poll.wHour << ':' << st_poll.wMinute << ':' << st_poll.wSecond 
			<< endl;
#if defined TEST_LOG
		testlog << "\nTIMESTAMP: " 
			<< st_poll.wHour << ':' << st_poll.wMinute << ':' << st_poll.wSecond 
			<< endl;
#endif
		// poll the servers and process player info
		for( i = 0; i < numServers; i++) 
		{
			PingServers(&servers[i]);
		}

		// remove players from database who do not meet certain criteria
		CullDatabase();

		if ( st_poll.wDayOfWeek != st_tally.wDayOfWeek && st_poll.wHour == 5 )
		{ // daily edge trigger at midnight EST
			// reset averages
			playerTally = OLplayerTally = pollTally = 0;
			st_tally = st_poll;
		}
		playerTally += (real_players+(numServers - numLiveServers - 5)); 
		OLplayerTally += real_players;
		//total players:
		//this consists of all players currently in a server with a ping, and also counts
		//each unresponsive listen server as 1 player.  This is the truest measure of game 
		//activity.
		printf("Servers: %i Players: %i Real Players: %i Online: %i Daily Avg: %4.2f/%4.2f\n",
			numServers, total_players, (real_players+(numServers - numLiveServers - 5)), 
			real_players, playerTally/pollTally, OLplayerTally/pollTally);
		total_players = 0;
		real_players = 0;
		numLiveServers = 0;
		
		// generate the html stats pages
		if ( (st_poll.wMinute > st_generate.wMinute 
			&& (st_poll.wMinute - st_generate.wMinute > 15))
			|| ( st_poll.wMinute < st_generate.wMinute
			&& (st_generate.wMinute - st_poll.wMinute < 45)) )
		{ // at 15 minute intervals
			GeneratePlayerRankingHtml();
#if defined NO_CLANSTATS
			cout << "!Not generating Clan Stats" << endl;
#else
			GenerateClanRankingHtml();
#endif
#if !defined NO_UPLOAD
			UploadStats();
#endif
			st_generate = st_poll;
		}

		// each day, check for inactive players, and decay their point totals
		if ( st_poll.wDayOfWeek != st_decay.wDayOfWeek )
		{ // daily edge trigger
			cout << "Checking for inactive players..." << endl;
			CheckInactivePlayers();
			st_decay = st_poll;
		}

		// backup database every hour
		if ( st_poll.wHour != st_backup.wHour )
		{ // hourly edge trigger
			cout << "Backing up databases..." << endl;
			BackupStats();
			st_backup = st_poll;
		}

		//write out the completed poll number
		RecordPollNumber();
	}
	
#if !defined NO_UPLOAD
	fceRelease();
#endif

#if defined TEST_LOG
	testlog << "\nEXIT" << endl;
	testlog.close();
#endif

	return 0;
}
