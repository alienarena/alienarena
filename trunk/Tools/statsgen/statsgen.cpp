/**
 * Alien Arena Game Stats Program
 *
 * Copyright (C) 2006,2011 by COR Entertainment
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Compile Options
 *
 * -DNO_UPLOAD to disable FTP upload to web server
 *
 * -DNO_VERIFY to disable verification.
 *
 * -DCLANSTATS to include clan related html page generation.
 *   (Clan related code not yet updated)
 *
 * -DTEST_LOG for logging test/debug info to 'testlog' file
 *   (lots of data, but much or all of this could be removed after testing)
 *
 */

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <cstring>
#include <cassert>

#include <WinSock.h>
#include "fce.h"
#include "keycode.h"

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

#if defined CLANSTATS
typedef	struct _CLANINFO {
	char clanname[32];
	PLAYERINFO players[32];
	double totalpoints;
	double totaltime;
	double totalfrags;
	int clannumber;
	int members;
} CLANINFO;

CLANINFO clans[32];
#endif

SERVERINFO servers[128];

#define VALIDATED_SIZE 1024
struct validated_s
{
	int count;
	char name[VALIDATED_SIZE][32];
} validated;

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

//Error flags.
// TODO: either complete this, or throw it out
const unsigned db_ifs  = 0x01;
const unsigned db_ofs  = 0x02;
const unsigned db_open = 0x04;
unsigned global_fail = 0;

/*
 * Strings for HTML Color
 */
const char* ColorPrefix[] =
	{
		"<font color=\"#000000\">",  // BLACK
		"<font color=\"#ff0000\">",  // RED
		"<font color=\"#00ff00\">",  // GREEN
		"<font color=\"#ffff00\">",  // YELLOW
		"<font color=\"#0000ff\">",  // BLUE
		"<font color=\"#00ffff\">",  // CYAN
		"<font color=\"#ff0000\">",  // MAGENTA
		"<font color=\"#ffffff\">"   // WHITE
	};

const char ColorSuffix[] = "</font>";

/**
 * HTML Special Chars
 */
const char* SpecialChar( char ch )
{
	if ( ch > 0x7f || ch < 0x20 )
	{
		return ("?");
	}
	switch ( ch )
	{
	case '<': return ("&lt;");
	case '>': return ("&gt;");
	case '&': return ("&amp;");
	case '"': return ("&quot;");
	case '\x27' : return ("&#39;"); // apostrophe
	case ' ': return ("&nbsp;");
	default: return ( NULL );
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
 * Check for name containing visible characters
 *
 * return  true if name has visible chars, false otherwise
 */
bool VisibleName( char *namestr )
{
	char *pch = namestr;
	while ( isspace( *pch ) )
	{
		++pch;
	}
	while ( IsColorString( pch ) )
	{
		pch += 2;
		while ( isspace( *pch ) ) ++pch;
	}
	return ( *pch != '\0' );
}

/**
 *  Null playername string safe getline()
 *
 *  Prevents bogus eof when player name is a null string.
 *  Also filters out names with no displayable characters.
 *
 * infile    ifstream reference to the source file
 * dst_name  pointer to string storage
 * dstsize   max size of string storage
 */
template< class FstreamType >
void GetPlayerName( FstreamType& infile, char *dst_name, size_t dstsize )
{
	if ( infile.peek() != 0 )
	{
		infile.getline( dst_name, dstsize );
		if ( !VisibleName( dst_name ) )
		{ // nothing but whitespace or color escapes
			dst_name[0] = 0;
			if ( infile.peek() == '\n')
			{
				infile.get();
			}
		}
	}
	else
	{ // empty name string or eof
		dst_name[0] = infile.get(); // nul
		if ( infile.peek() == '\n')
		{
			infile.get();
		}
	}
}

/**
 * Search the list of currently validated players from AccountServ
 * 
 * name    the player's name string to check
 * return  true if player is logged in, false otherwise
 */
bool VerifyPlayer (char *name)
{
	int i;

#if defined NO_VERIFY
	return true;
#endif

	for ( i = 0 ; i < validated.count ; i++ )
	{
		if ( !strcmp( name, validated.name[i] ) )
		{
			return true;
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
double RankingPoints (char* playername, double totalpoints, double totaltime)
{

	if ( totalpoints < 0.0 || totaltime < 1.0 )
	{
		return 0.49; //slightly less than minimum
	}
	// Calculation derived from:
	//  y = mx + b, with m=0.5, with a point (x,y) = (1,1)
	// "compresses" actualpoints range and favors higher totaltime
	// approx equal to points-per-1/2-hour, after ~10 hours
	return (((totalpoints - 1.0)/(2.0 * totaltime)) + 1.0);

}

/**
 * Get player info for a real person player actively in the game.
 * Add to the database, if not already there.
 *
 * Bots and non-validated players should never get here.
 *
 *  player  PLAYERINFO record with name
 *  returns PLAYERINFO record with rest of data, including current rank
 *
 */
PLAYERINFO LoadPlayerInfo (PLAYERINFO player)
{
	char name[32],
		points[32],
		frags[32],
		totalfrags[32],
		time[16],
		totaltime[16],
		ip[16],
		poll[16],
		remote_address[21];
	bool foundplayer;

	fstream dbfile( "playerrank.db" );
	if ( !dbfile )
	{
		global_fail |= db_open;
		cout << "[E] LoadPlayerInfo: playerrank.db open failed." << endl;
		name[0] = '\0';
		return player;
	}

	player.rank = 1;
	foundplayer = false;
	for (;;)
	{
		//get the next record
		GetPlayerName( dbfile, name, 32 );
		dbfile.getline( remote_address, 21 ); //remote address
		dbfile.getline( points, 32 ); //points
		dbfile.getline( frags, 32 ); //frags
		dbfile.getline( totalfrags, 32 ); //total frags
		dbfile.getline( time, 16 ); //current time in poll
		dbfile.getline( totaltime, 16 );
		dbfile.getline( ip, 16 ); //last server.ip
		dbfile.getline( poll, 16 ); //what poll was our last?
		if ( dbfile.eof() )
		{
			break;
		}

		if ( !strcmp( player.playername, name ) )
		{ //found this player
			player.points = atof( points );
			player.frags = atoi( frags );
			player.totalfrags = atoi( totalfrags );
			player.time = atof( time );
			player.totaltime = atof( totaltime );
			strcpy( player.ip, ip );
			player.poll = atoi( poll );
			foundplayer = true;
			break; //get out we are done
		}
		player.rank++ ;
	}

	if ( !foundplayer && strlen( player.playername ) )
	{ //player not already in database, add to end with defaults
		//don't write out blank player names, should not get here anyway

		dbfile.seekg( 0, ios::end );

		dbfile << player.playername << '\n';
		dbfile << "127.0.0.1" << '\n';
		dbfile << "0" << '\n';
		dbfile << "0" << '\n';
		dbfile << "0" << '\n';
		dbfile << ".008" << '\n';
		dbfile << "1.0" << '\n';
		dbfile << "my ip" << '\n';
		dbfile << currentpoll << endl;

		strcpy( player.remote_address, "127.0.0.1" );
		player.points     = 0;
		player.frags      = 0;
		player.totalfrags = 0;
		player.totaltime  = 1.0;
		player.time       = .008;
		strcpy( player.ip, "my ip" );
		player.poll       = currentpoll;
	}
	dbfile.close();

	return player;
}

/**
 * Put player's current data into database at current ranking position
 *
 * player   the record for the player to be re-inserted
 */
void ReInsertPlayer (PLAYERINFO player )
{
	char name[32],
		points[32],
		frags[32],
		totalfrags[32],
		time[16],
		totaltime[16],
		ip[16],
		poll[16],
		remote_address[21];
	int inserted;
	int total_players;
	double rankpoints_player;
	double rankpoints_other;

	if ( strlen(player.playername) > 0 )
	{
		rankpoints_player = RankingPoints( player.playername, player.points, player.totaltime );
	}
	else
	{ //should not happen, but if it does...
		return;
	}

	ifstream infile( "playerrank.db" );
	if ( infile.peek() == EOF )
	{ // empty file, add player
		infile.close();
		ofstream initdb( "playerrank.db", ios::trunc );
		initdb << player.playername << '\n';
		initdb << player.remote_address << '\n';
		initdb << player.points << '\n';
		initdb << player.frags << '\n';
		initdb << player.totalfrags << '\n';
		initdb << player.time << '\n';
		initdb << player.totaltime << '\n';
		initdb << player.ip << '\n';
		initdb << player.poll << endl;
		initdb.close();
		return;
	}

	if ( !infile )
	{
		cout << "[E] ReInsertPlayer: playerrank.db open failed" << endl;
		return;
	}
	ofstream outfile("temp.db", ios::trunc );
	if ( !outfile )
	{
		infile.close();
		cout << "[E] ReInsertPlayer: temp.db open failed" << endl;
		return;
	}

	if( infile.good() && outfile.good() )
	{
		inserted = false;
		total_players = 0;

		//the idea of this is, when rebuilding this file, keep it to a reasonable size
		//we only be tracking the top 1000 players then.
		//with an extra 50 slots to handle overflow. Scoring requires that
		//active players are in the database.
		while ( total_players < 1050 )
		{
			GetPlayerName( infile, name, 32 );
			infile.getline(remote_address, 21);
			infile.getline(points, 32);
			infile.getline(frags, 32);
			infile.getline(totalfrags, 32);
			infile.getline(time, 16);
			infile.getline(totaltime, 16);
			infile.getline(ip, 16);
			infile.getline(poll, 16);
			if ( infile.eof() )
			{
				break;
			}

			if ( !inserted && strlen(name) && strcmp( player.playername, name ))
			{ //not in yet, somebody is, and its not me.
				rankpoints_other = RankingPoints(name, atof( points ), atof( totaltime ));
				if ( rankpoints_other < rankpoints_player )
				{ //this will add new ranking
					//and I win!
					outfile << player.playername << '\n';
					outfile << player.remote_address << '\n';
					outfile << player.points << '\n';
					outfile << player.frags << '\n';
					outfile << player.totalfrags << '\n';
					outfile << player.time << '\n';
					outfile << player.totaltime << '\n';
					outfile << player.ip << '\n';
					outfile << player.poll << endl;
					inserted = true;
				}
			}

			if ( strcmp(name, player.playername) && strlen(name) )
			{ //output other player records, this will remove old ranking
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
		if( !inserted && total_players < 1050 )
		{ //worst player ever!
			if(strlen(player.playername))
			{
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
	else
	{
		infile.close();
		outfile.close();
		global_fail |= db_ifs | db_ofs;
		cout << "[E] ReInsertPlayer: good() file checks failed" << endl;
		return;
	}

	// swap in the new playerrank.db
	if ( remove("playerrank.db") != 0 )
	{
		cout << "[E] ReInsertPlayer: remove of playerrank.db failed. " << strerror(errno) << endl;
	}
	if ( rename("temp.db", "playerrank.db") != 0 )
	{
		cout << "[E] ReInsertPlayer: rename from temp.db to playerrank.db failed." << endl;
	}

}

/**
 * Update scoring from server status response
 *
 */
void ProcessPlayers (SERVERINFO *server)
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
	PLAYERINFO player_info;

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
		//check early for bots, and equivalents
		//  LoadPlayerInfo() would insert them in database.
		if ( server->players[i].ping == 0 )
		{ //bots and connecting players
			continue;
		}
		if ( strlen( server->players[i].playername ) == 0  )
		{ //empty name string
			//same as bots, no effect on scoring and ranking
			continue;
		}
		if ( !VerifyPlayer( server->players[i].playername ) )
		{ //spoofed or non-registered
			//same as bots, no effect on scoring and ranking

			// TODO: log this ?

			continue;
		}
		if ( !strcmp( server->players[i].playername, "Player" ) )
		{ //exact match to default player name
			//not unusual for multiple players online simultaneously with this name
			//which would mess up scoring badly
			continue;
		}

		//complete record generated from server info and database
		player_info = LoadPlayerInfo( server->players[i] );
		if ( strlen( player_info.playername) == 0 )
		{ // this should not happen
			cout << "[E] ProcessPlayers: LoadPlayerInfo() null string program error" << endl;
			continue;
		}

		if ( player_info.poll != currentpoll-1
			|| strcmp( player_info.ip, server_ip )	)
		{ //first poll on this server for this player
			//set time to minimum
			if(realDelay > 30)
			{
				player_info.time = (realDelay * .008/30);
			}
			else
			{
				player_info.time  = .008;
			}
		}

		if ( player_info.score > 0 )
		{ //add active, real player to the player list
			//note: overwrites record at or before current 'i'
			server->players[j++] = player_info;
			++realpeeps;
			//accumulate total score for qualified players
			total_score += (double)player_info.score;
		}
		else
		{ //possibly downloading, idling, or just doing poorly.
			// need to initialize time and reinsert, otherwise
			// time could be 0, screwing up prop_score calc
			if(realDelay > 30)
			{
				player_info.time = (realDelay * .008/30);
			}
			else
			{
				player_info.time  = .008;
			}
			player_info.frags = 0;
			// TODO: not too sure setting about setting frags.
			//  question is, when player becomes active,
			//  what should stored values be?
			ReInsertPlayer( player_info );
		}
	}
	assert( j == realpeeps );
	if ( realpeeps == 0 )
	{ //early return if no active, real players in list
		return;
	}
	else if ( realpeeps == 1 )
	{ //only 1, nothing counts. init time and frags increments, and reinsert
		player_info.time  = .008;
		player_info.frags = 0;
		ReInsertPlayer( player_info );
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
	 *  the number of polls.
	 * note: The 0.05 fudge factor compensates for small number of polls.
	 */
	sum_prop_score = 0.0;
	for ( i = 0; i < realpeeps ; i++ )
	{
		assert( server->players[i].time >= .008 );
		prop_score[i] = server->players[i].score;
		prop_score[i] *= 1.0 / ( (ceil( server->players[i].time / .008)) + 0.05);
		sum_prop_score += prop_score[i];
	}
	for ( i = 0; i < realpeeps ; i++ )
	{ //normalize. sum of prop_score's will be 1.0
		prop_score[i] /= sum_prop_score;
	}

	//calculate points for each player as sum of points versus each other player
#if defined TEST_LOG
	testlog << "GAME POLL ( " << currentpoll <<  " )\n";
#endif
	server->players[i].points = 0.0;
	for ( i = 0; i < realpeeps ; i++ )
	{ //for each player in the server's filtered list
#if defined TEST_LOG
		testlog << server->players[i].playername
			<< " prop_score: " << prop_score[i] << "\n";
#endif
		for ( j = 0; j < realpeeps ; j++ )
		{
			double new_points = 0.0;

			if ( i == j )
			{ //self. new_points would be 0 anyway.
				continue;
			}
			if ( server->players[i].time < 0.015
				|| server->players[j].time < 0.015 )
			{ //require some time in game before getting points
				// prevents inaccuracies and discourages gaming the system.
				new_points = 0.0;
			}
			else if ( server->players[i].rank > server->players[j].rank )
			{ //ranking is 1 highest to 1000 lowest.
				//lower ranked player always gets >0 points,
				//but does not amount to much if close in rank
				double rank_factor;
				double score_factor;

				rank_factor = (server->players[i].rank - server->players[j].rank)
								/ 1001.0 ;
				if ( rank_factor > 1.0 )
				{ //in case .rank is >1000
					rank_factor = 1.0;
				}
				assert( rank_factor >= 0.0 && rank_factor <= 1.0 );

				//score difference adjusted from (-1,1) to (0,2)
				//  total score factor compensates for low scoring games
				score_factor = ( 1.0 + prop_score[i] - prop_score[j] ) - total_score_factor;
				if ( score_factor < 0.0 )
				{ //probably total score too low to be significant
					score_factor = 0.0;
				}
				assert( score_factor >= 0.0 && score_factor <= 2.0 );

				new_points = rank_factor * score_factor;
				if ( new_points < points_epsilon )
				{ //use an epsilon to avoid floating point instability
					new_points = 0.0;
				}
#if defined TEST_LOG
				testlog << "  " << server->players[i].playername
						<< " rf*sf: " << new_points
						<< " vs. " << server->players[j].playername
						<< "\n";
#endif
				//new_points close to 2.0 are extremely improbable
				//but good players should move up rank quickly at first
				assert( new_points >= 0.0 && new_points <= 2.0 );
			}
			else if ( prop_score[i] > prop_score[j] )
			{
				new_points = (prop_score[i] - prop_score[j]) - total_score_factor;
				if ( new_points < points_epsilon )
				{ //use an epsilon to avoid floating point instability
					new_points = 0.0;
				}
				assert( new_points >= 0.0 && new_points <= 1.0 );
#if defined TEST_LOG
				testlog << "  " << server->players[i].playername
						<< " ps-ps: " << new_points
						<< " vs. " << server->players[j].playername
						<< "\n";
#endif
			}

			if ( new_points > 0.0 )
			{ //accumulate new points
				server->players[i].points += new_points;
			}
		} //for each other player
	} //for each player

	for ( i = 0; i < realpeeps ; i++ )
	{ //for each player in the server's filtered list, re-insert in database
		//separate loop to be (paranoid) sure that changes don't affect scoring

		//first tally up total frags by comparing with our last poll's number
		//(only if player is currently in consecutive polls)
		if(server->players[i].poll == currentpoll-1)
		{
			if((server->players[i].score - server->players[i].frags) > 0)
			{ //record only positive increment in frags
				//score is current frags for game, frags is previously tallied frags
				server->players[i].totalfrags +=
					(server->players[i].score - server->players[i].frags);
			}
		}
		//update to current frags, poll, time and server ip
		//and reinsert player into database with new values
		//the time increment can adjust to the polling interval
		server->players[i].frags = server->players[i].score;
		server->players[i].poll = currentpoll;
		if(realDelay > 30) {
			server->players[i].time      += (realDelay * .008/30);
			server->players[i].totaltime += (realDelay * .008/30);
		}
		else {
			server->players[i].time      += (.008);
			server->players[i].totaltime += (.008);
		}
		strcpy(server->players[i].ip, server_ip); //we are in this server now
		ReInsertPlayer(server->players[i]);
#if defined TEST_LOG
		testlog << "  " << server->players[i].playername
				<< " .time: " << server->players[i].time
				<< " .points: " << server->players[i].points << '\n';
#endif
	} //for each player
#if defined TEST_LOG
	testlog << endl;
#endif
}

/**
 * Query the master for current servers
 */
void GetServerList (void)
{

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

/**
 * Read the logged in players list from Account Server
 */
void GetValidatedPlayers (void)
{
	int index = 0;

	validated.count = 0;

	ifstream infile( "validated" );
	if ( !infile )
	{
		cout << "[E] GetValidatedPlayers: validated file failed open." << endl;
		return;
	}

	for (;;)
	{
		GetPlayerName( infile, validated.name[index++], 32 );
		if ( infile.eof() )
		{
			break;
		}
		++validated.count;
		if ( index >= VALIDATED_SIZE )
		{
			cout << "[E] GetValidatedPlayers: overflow." << endl;
		}
	}
	infile.close();

}

/**
 * Read a line from server's status response
 */
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

/**
 * Request and receive status from a server
 */
void PingServers (SERVERINFO *server)
{
	char *p, *rLine;
	char lasttoken[256];
	char recvBuff[4096];
	fd_set stoc;
	int i;
	int result;
	int fromlen;
	SOCKADDR_IN dgFrom;
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

	for (i = 0; i < 2; i++)
	{
		result = sendto (s, request, sizeof(request), 0, (struct sockaddr *)&addr, sizeof(addr));
		if (result == SOCKET_ERROR)
		{
			printf ("Can't send: error %d", WSAGetLastError());
		}
		else
		{
			memset (&stoc, 0, sizeof(stoc));
			FD_SET (s, &stoc);

			//time out was 1 sec, shorter now to maintain 30sec loop time
			delay.tv_sec  = 0;
			delay.tv_usec = 600000L;

			result = select( 0, &stoc, NULL, NULL, &delay );
			if ( result != SOCKET_ERROR && result > 0 )
			{
				fromlen = sizeof(dgFrom);
				result = recvfrom (s, recvBuff, sizeof(recvBuff), 0, (SOCKADDR*)&dgFrom, &fromlen);
				break; //got a response, exit the loop
			}
			else
			{
				result = -1;
			}
		}
	}

	if (result > 0)
	{

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
		free (rLine);

		//playerinfo
#if defined TEST_LOG
			testlog << "--- " << server->szHostName << " ---\n";
#endif
		strcpy (seps, " ");
		while (rLine = GetLine (&p, &result))
		{
#if defined TEST_LOG
			testlog << rLine << '\n';
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
				strncpy( server->players[players].playername, token,
							sizeof(server->players[players].playername)-1);
			else
				server->players[players].playername[0] = '\0';

			token = strtok( NULL, "\"");
			token = strtok( NULL, "\"");
			if (token)
				strncpy (server->players[players].remote_address, token,
						sizeof(server->players[players].remote_address)-1);
			else
				strncpy (server->players[players].remote_address, "127.0.0.1",
						sizeof(server->players[players].remote_address)-1);

			players++;
			free (rLine);
		}
#if defined TEST_LOG
			testlog << endl;
#endif
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
}

/**
 * Filter the player database by a culling criteria
 *
 *  Note that records are not removed while player is active in game.
 *  Also, the html generation needs to apply similar criteria
 *   to filter out such records from the stats html pages.
 * Requires playerrank.db to exist.
 */
void CullDatabase  (void)
{
	char name[32],
		points[32],
		frags[32],
		totalfrags[32],
		time[16],
		totaltime[16],
		ip[16],
		poll[16],
		remote_address[21];
	int input_count;
	int output_count;

	ofstream outfile( "temp.db" );
	if ( !outfile )
	{
		global_fail |= db_ofs;
		cout << "[E] CullDatabase: temp.db open fail." << endl;
		return;
	}
	ifstream infile( "playerrank.db" );
	if ( !infile )
	{
		outfile.close();
		global_fail = db_ifs;
		cout << "[E] CullDatabase: playerrank.db open fail." << endl;
		return;
	}

	output_count = input_count = 0;
#if defined TEST_LOG
	testlog << "\nDatabase cull:" << endl;
#endif
	//keep only player records that are significant
	for (;;)
	{
		//get each line of a player record
		GetPlayerName( infile, name, 32 );
		infile.getline(remote_address, 21);
		infile.getline(points, 32);
		infile.getline(frags, 32);
		infile.getline(totalfrags, 32);
		infile.getline(time, 16);
		infile.getline(totaltime, 16);
		infile.getline(ip, 16);
		infile.getline(poll, 16);
		if ( infile.eof() || infile.bad() )
		{
			break;
		}
		++input_count;

		//Culling Criteria
		if ( atoi(poll) != currentpoll )
		{ //not currently in game. (that would screw up point calcs)
			// NOTE: non-verified players and players with empty name strings
			//  should not be in the database and have no effect on scoring.
			if ( strlen( name ) == 0 )
			{
#if defined TEST_LOG
				testlog << "  culled for null name string @ " << input_count << endl;
#endif
				continue;
			}
			if ( !_strnicmp( name, "Player", 6 ) )
			{ //using a default name or a variation thereof
#if defined TEST_LOG
				testlog << "  " << name << " culled for name." << endl;
#endif
				continue;
			}
			if ( atof(points) <= 0.0  )
			{ //has no points, so no point in being in the database
				//single-player only player
				//or, so far, only played online against bots
				//or points have decayed to nothing.
#if defined TEST_LOG
				testlog << "  " << name << " culled for points: " << points << endl;
#endif
				continue;
			}
		}

		if ( strlen( name ) > 0 )
		{
			//keep this player record
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
		else
		{
#if defined TEST_LOG
			testlog << "  culled for null name string @ " << input_count << endl;
#endif
		}
	}

	if ( !infile.bad() && !outfile.bad() )
	{
		infile.close();
		outfile.close();
		remove("playerrank.db");
		rename("temp.db", "playerrank.db");
#if defined TEST_LOG
		testlog << "Database cull complete."
			<< "( " << input_count << ", " << output_count << " )" << endl;
#endif
	}
	else
	{
		infile.close();
		outfile.close();
		cout << "[E] CullDatabase: failed, playerrank.db not updated." << endl;
#if defined TEST_LOG
		testlog << "Database cull failed."
			<< "( " << input_count << ", " << output_count << " )" << endl;
#endif
	}
}

/**
 * Translate Quake Color Escapes to HTML tags.
 * Also, translate html reserved characters.
 *
 */
bool ColorizePlayerName (const char *src, char *dst, size_t dstsize)
{
	int icolor;
	int jcolor;
	size_t char_count    = 0;
	size_t substring_len = 0;
	bool prefixed        = false;
	char *prefix_begin   = dst;
	size_t prefix_len    = 0;
	size_t suffix_len    = strlen( ColorSuffix );

	icolor = jcolor = 7; //white
	do
	{
		if ( IsColorString( src ) )
		{
			++src;
			jcolor = ColorIndex( src );
			++src;
			if ( jcolor != icolor )
			{ //color change.
				if ( prefixed )
				{ //suffix for previous colored substring
					if ( substring_len == 0 )
					{ //pointless color change, back up.
						char_count -= prefix_len;
						dst = prefix_begin;
						prefixed = false;
					}
					else
					{ //append suffix
						char_count += suffix_len;
						if ( char_count < dstsize )
						{
							strncpy( dst, ColorSuffix, suffix_len );
							dst += suffix_len;
						}
						else
						{ //no room
							return false;
						}
					}
				}
				char_count += strlen( ColorPrefix[jcolor] );
				if ( char_count < dstsize )
				{ //append prefix for new color
					prefixed = true;
					prefix_begin = dst;
					prefix_len = strlen( ColorPrefix[jcolor] );
					strncpy( dst, ColorPrefix[jcolor], prefix_len );
					icolor = jcolor;
					dst += prefix_len;
					substring_len = 0;
				}
				else
				{ //no room
					return false;
				}
			}
			//else redundant color change, do nothing
		}
		else
		{ //visible character
			if ( ++char_count < dstsize )
			{
				const char* htmlchar = SpecialChar( *src );
				if ( htmlchar != NULL )
				{ //translate html reserved character
					int htmllength = strlen( htmlchar );
					if ( (char_count -1 + htmllength) < dstsize )
					{
						strcpy( dst, htmlchar );
						dst += htmllength;
						++src;
						char_count    += htmllength - 1;
						substring_len += htmllength;
					}
					else
					{ //no room
						*dst++ = '?';
						++src ;
					}
				}
				else
				{ //regular char
					*dst++ = *src++;
					++substring_len;
				}
			}
			else
			{ //no room
				return false;
			}
		}
	} while ( *src );
	if ( prefixed )
	{ //trailing suffix
		if ( substring_len > 0 )
		{
			char_count += suffix_len;
			if ( char_count < dstsize )
			{ //add </td>
				strncpy( dst, ColorSuffix, suffix_len );
				dst += suffix_len;
			}
			else
			{ //no room
				return false;
			}
		}
		else
		{ //pointless trailing color change
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

/**
 * Create web pages from player database
 */
void GeneratePlayerRankingHtml (void)
{
	char name[32],
		points[32],
		frags[32],
		totalfrags[32],
		time[16],
		totaltime[16],
		ip[16],
		poll[16],
		remote_address[21];
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

	ifstream infile( "playerrank.db" );
	if ( !infile )
	{
		cout << "[E] GeneratePlayerRankingHtml: playerrank.db open failed." << endl;
		return;
	}

	GetSystemTime(&stime);

	if(infile.good())
	{
		rank = 1;
		currentPos = 1;
		while( rank <= currentPos )
		{
			page = rank/100 + 1; //[1,99]=>1, [100,199]=>2,...
			if(rank == currentPos)
			{ // new page
				currentPos += 100; // 101,201,...
				if (currentPos >= 1101 )
				{
					currentPos = 0;
				}

				//open a new file
				sprintf(a_string, "stats%i.html", page);
				outfile.open(a_string);

				//build header
				outfile << "<!doctype html public \"-//w3c//dtd html 4.0 transitional//en\">\n";
				outfile << "<html>\n";
				outfile << "<head>\n";
				outfile << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n";
				outfile << "<meta name=\"Author\" content=\"John Diamond\">\n";
				outfile << "<meta name=\"GENERATOR\" content=\"Mozilla/4.78 [en] (Windows NT 5.0; U) [Netscape]\">\n";
				outfile << "<title>Alien Arena Global Stats</title>\n";
				outfile << "</head>\n";
				outfile << "<body style=\"color: rgb(255, 255, 255); background-color: rgb(153, 153, 153); background-image: url(images/default_r4_c5.jpg);\" alink=\"#000099\" link=\"#ffcc33\" vlink=\"#33ff33\">\n";
				outfile << "<center><b><font face=\"Arial,Helvetica\">ALIEN ARENA GLOBAL STATS</font></b>\n";
				sprintf(a_string, "<p><b><font face=\"Arial,Helvetica\"><font size=-2>Updated %i/%i/%i %i:%i GMT</font></b></center>", stime.wMonth, stime.wDay, stime.wYear, stime.wHour, stime.wMinute);
				outfile << a_string << endl;
				outfile << "<p><br>\n";
				outfile << "<center><table border=\"2\" cellspacing=\"0\" cellpadding=\"4\">\n";
				outfile << "<tr>\n";
				outfile << "<td align=\"center\" bgcolor=\"#cccccc\"><font face=\"Arial,Helvetica\"><font size=-1>RANK</font></td>\n";
				outfile << "<td align=\"center\" bgcolor=\"#cccccc\"><font face=\"Arial,Helvetica\"><font size=-1>NAME</font></td>\n";
				outfile << "<td align=\"center\" bgcolor=\"#cccccc\"><font face=\"Arial,Helvetica\"><font size=-1>POINTS</font></td>\n";
				outfile << "<td align=\"center\" bgcolor=\"#cccccc\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGS</font></td>\n";
				outfile << "<td align=\"center\" bgcolor=\"#cccccc\"><font face=\"Arial,Helvetica\"><font size=-1>TIME</font></td>\n";
				outfile << "<td align=\"center\" bgcolor=\"#cccccc\"><font face=\"Arial,Helvetica\"><font size=-1>FRAGRATE</font></td>\n";
				outfile << "</tr>" << endl;

			}

			GetPlayerName( infile, name, 32 );
			infile.getline(remote_address, 21);
			infile.getline(points, 32);
			infile.getline(frags, 32);
			infile.getline(totalfrags, 32);
			infile.getline(time, 16);
			infile.getline(totaltime, 16);
			infile.getline(ip, 16);
			infile.getline(poll, 16);
			if ( infile.eof() )
			{ // all done, for less than 1000
				break;
			}

			if ( strlen( name) == 0
				|| !_strnicmp( name, "Player", 6 ) )
			{ //cull out blank name, default name
#if defined TEST_LOG
				testlog << "  html cull "
					<< (strlen(name)==0?"blank":name) << endl;
#endif
				continue;
			}

			ntotaltime  = atof( totaltime );
			ntotalfrags = atof( totalfrags );
			npoints     = atof( points );
			actualtime  = ntotaltime - 1.0;
			if ( actualtime < 0.015 || ntotalfrags <= 0.0 || npoints < 0.00001 )
			{ //cull out minimum time, zero frags, minimal points
				//to help keep floating point math stable
#if defined TEST_LOG
				testlog << "  html cull "
					<< name << " ( "
					<< actualtime << ','
					<< ntotalfrags << ','
					<< npoints << " )" << endl;
#endif
				continue;
			}

			//build row for this player
			// with calculated points and fragrate
			//Scaling displayed points by 10, for psych. reasons.
			actualpoints = RankingPoints( name, npoints, ntotaltime );
			actualpoints *= 10.0;
			fragrate     = ntotalfrags / actualtime;
			outfile << "<tr>\n";
			sprintf(a_string, "<td align=\"right\">%i</td>", rank);
			outfile << a_string << endl;

			outfile << "<td align=\"center\">" ;
			if ( ColorizePlayerName( name, a_string, sizeof(a_string) ))
			{
				outfile << a_string;
			}
			else
			{ //colorize error
				outfile << name ;
			}
			outfile << "</td>\n" ;

			//POINTS, FRAGS, TIME, FRAGRATE
			sprintf(a_string, "<td align=\"right\">%4.2f</td>", actualpoints);
			outfile << a_string << '\n';
			sprintf(a_string, "<td align=\"right\">%s</td>", totalfrags);
			outfile << a_string << '\n';
			sprintf(a_string, "<td align=\"right\">%4.2f&nbsp;hours</td>", actualtime);
			outfile << a_string << '\n';
			sprintf(a_string, "<td align=\"right\">%4.2f&nbsp;frags/hr</td>", fragrate);
			outfile << a_string << '\n';
			outfile << "</tr>" << endl;

			rank++;
			if ( rank == currentPos )
			{ // this page is full  ( rank == 100,200,... )
				outfile << "</table>\n";
				sprintf(a_string,
					"<p><font face=\"Arial,Helvetica\"><font size=-1><a href=\"stats%i.html\">Page %i</a></font></center>",
					page, page);
				outfile << a_string << '\n';
				outfile << "</body>\n";
				outfile << "</html>" << endl;

				outfile.close();

				closed = true;
			}
			else
			{
				closed = false;
			}
		}
	}
	infile.close();

	if(!closed)
	{ // last page was partial. less than 1000 qualified players
		outfile << "</table>\n";
		sprintf(a_string, "<p><font face=\"Arial,Helvetica\"><font size=-1><a href=\"stats%i.html\">Page %i</a></font></font></center>", 1, 1);
		outfile << a_string << '\n';
		outfile << "</body>\n";
		outfile << "</html>" << endl;

		outfile.close();
	}
}

#if defined CLANSTATS

// TODO: decide what to do about clan stats. not much point in updating
//   this code unless clan.db is updated.

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

#endif // CLANSTATS

/**
 * Hourly copy of player database to backup directory
 */
void BackupStats (void)
{

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

#if !defined NO_UPLOAD

void UploadStats(void) { //put all updated files on the server

	int error;
	int i;
	char a_string[16];
	FILE *filename;

	printf("Uploading to ftp server.\n");

	fceSetInteger(0, FCE_SET_PASSIVE, 1);
	fceSetInteger(0, FCE_SET_CONNECT_WAIT, 1000);
	fceSetInteger(0, FCE_SET_MAX_RESPONSE_WAIT, 1000);

	//Connect to FTP server
	error = fceConnect(0,"ftp.martianbackup.com","user","password");
	if(error < 0) {
		printf("Error connecting to host!\n");
		return;
	}
	else
		printf("Connected Successfully!\n");

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

#if defined CLANSTATS
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
#endif

	fceClose(0);

}
#endif // NO_UPLOAD

void LoadPollNumber(void)
{
	ifstream infile;
	char poll[16];

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
	char name[32],
		points[32],
		frags[32],
		totalfrags[32],
		time[16],
		totaltime[16],
		ip[16],
		poll[16],
		remote_address[21];
	PLAYERINFO player[1101]; //very unlikely to ever use all of this
	int diff;
	double pointnumber;
	int i, j;
	int player_poll;

	ifstream infile( "playerrank.db" );
	if ( !infile )
	{
		cout << "[E] CheckInactivePlayers: playerrank.db failed open." << endl;
		return;
	}

	i = 0;
	for (;;)
	{
		GetPlayerName( infile, name, 32 );
		infile.getline(remote_address, 21); //remote address
		infile.getline(points, 32); //points
		pointnumber = atof(points); //so we can manipulate this
		infile.getline(frags, 32); //frags
		infile.getline(totalfrags, 32); //total frags
		infile.getline(time, 16); //current time in poll
		infile.getline(totaltime, 16);
		infile.getline(ip, 16); //last server.ip
		infile.getline(poll, 16); //what poll was our last?
		if ( infile.eof() )
		{
			break;
		}

		// decay player's points while inactive > inactive limit
		//   if points >= 10, decay 10%/day
		//   if points >=2 and points < 10, decay 1pt/day
		//   if points < 2 , set points to zero
		//
		// examples for days to zero points after inactive decay begins
		//  2000pts:60days, 1000pts:53days, 500pts:47days,
		//  200pts:38days, 100pts:31days
		//
		// once a player passes inactive limit, their poll
		//  is set to 100001. And their points are decayed until they
		//  play again or are removed from the database.
		//  This prevents suspending decay after the
		//  current poll wraps and passes player's last poll.
		//
		// setting the intactive limit is a compromise between
		//  * giving a player some room for a "vacation".
		//  * preventing the database from getting overloaded with inactive
		//     and abandoned names,
		//
		//  For a player name with lots of points, it takes a long time for
		//   the name to be removed ( inactive limit + >60 days ). This keeps
		//   new players out of the database.
		//  Even if an active player gets decayed some, they will be able to
		//   re-establish higher rank fairly quickly, because they will be
		//   outscoring higher ranked players.
		//  An inactive name in the database cannot be "challenged" by other
		//    players, so it is not good for an inactive name to be highly
		//    ranked.
		//  For a player name with a low score after the inactive limit point,
		//    it is likely that this is not an active player, so it is good
		//    to eliminate these from the database.

		player_poll = atoi(poll);
		diff = currentpoll - player_poll;
		if ( diff < 0 )
		{ //currentpoll wrapped around at 100000. (every ~35 days)
			if ( player_poll < 100001 )
			{
				diff += 100000;
			}
		}

		// inactive interval set to approx 2 weeks (40320 polls)
		//  1 week too short, 4 weeks too long (see above).
		if ( player_poll > 100000 || diff > 40320 )
		{ //start decay after the inactive interval with no play
			//and decay each day
#if defined TEST_LOG
			testlog << "decay " << name
				<< "[ " << currentpoll << ", " << player_poll << " ]"
				<< " ( " << pointnumber ;
#endif
			if ( pointnumber >= 10.0 )
			{ //by 10% per day
				pointnumber *= .90;
#if defined TEST_LOG
				testlog << " to " << pointnumber
					<< " )" << endl;
#endif
			}
			else if ( pointnumber >= 2.0 )
			{ //by 1 point per day
				pointnumber -= 1.0;
#if defined TEST_LOG
				testlog << " to " << pointnumber
					<< " )" << endl;
#endif
			}
			else
			{ //zero if would be < 1.0, after -1.0
				pointnumber = 0.0;
#if defined TEST_LOG
				testlog << " to zero )" << endl;
#endif
			}
			//add to player with decayed points data list
			strcpy(player[i].playername, name);
			strcpy(player[i].remote_address, remote_address);
			player[i].points = pointnumber;
			player[i].frags = atoi(frags);
			player[i].totalfrags = atoi(totalfrags);
			player[i].time = atof(time);
			player[i].totaltime = atof(totaltime);
			strcpy(player[i].ip, ip);
			player[i].poll = 100001; // always >100000
			i++;
		}
	}
	infile.close();

	if ( i != 0 )
	{
		ofstream decaylog( "decay.log", ios::app );
		if ( decaylog )
		{
			decaylog << "POLL: " << currentpoll << endl;
		}
		else
		{
			cout << "DECAY. POLL: " << currentpoll << endl;
		}
		for ( j = 0; j < i ; j++ )
		{ //for each player with decayed points, reinsert to update rank
			//players with 0 points will be culled by CullDatabase
			ReInsertPlayer( player[j] );
			if ( decaylog )
			{
				decaylog << player[j].playername
						<< " points: " << player[j].points << endl;
			}
			else
			{
				cout << player[j].playername
						<< " points: " << player[j].points << endl;
			}
	#if defined TEST_LOG
			testlog << "decay reinsert " << player[j].playername
				<< " points: " << player[j].points << endl;
	#endif
		}
		if ( decaylog )
		{
			decaylog.close();
		}
	}

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

/**
 * Clean playerrank.db on startup
 *
 * To re-sort the file in case of a change in the
 *  ranking calculation, or other rank affecting change.
 * To filter out various unwanted records.
 */
bool InitDatabase( void )
{
	PLAYERINFO player;
	char name[32],
		remote_address[21],
		points[32],
		frags[32],
		totalfrags[32],
		time[16],
		totaltime[16],
		server_ip[16],
		poll[16];
	int new_record_count = 0;

	cout << "Initializing database..." << endl;

	ifstream newdbchk( "playerrank.db" );
	if ( newdbchk.peek() == EOF )
	{
		cout << "playerrank.db is empty or does not exist" << endl;
		newdbchk.close();
		ofstream createdb("playerrank.db", ios::trunc );
		if ( !createdb )
		{
			cout << "  file error. initialization failed." << endl;
			return false;
		}
		else
		{
			createdb.close();
			cout << "  empty file exists. initializaton complete." << endl;
			return true;
		}
	}
	newdbchk.close();

	cout << "  running CullDatabase()..." << endl;
	CullDatabase();
	if ( global_fail )
	{
		cout << "[E] CullDatabase failed." << endl;
		return false;
	}

	cout << "  re-inserting all records..." << endl;
	ifstream dbfile( "playerrank.db" );
	if ( !dbfile )
	{
		cout << "[E] unexpected playerrank.db error." << endl;
		return false;
	}
	if ( dbfile.peek() == EOF )
	{
		cout << "  nothing to re-insert. playerrank.db is empty." << endl;
		dbfile.close();
		return true;
	}
	fstream dbcopy("initrank.db", ios::in|ios::out|ios::trunc );
	if ( !dbcopy )
	{
		dbfile.close();
		cout << "[E] could not create initrank.db." << endl;
		return false;
	}

	dbcopy << dbfile.rdbuf();
	dbfile.close();
	cout << "    copied playerrank.db to initrank.db." << endl;

	int record_no = 0;
	dbcopy.seekg( 0, ios::beg );
	for (;;)
	{
		GetPlayerName( dbcopy, name, sizeof(name) );
		dbcopy.getline(remote_address, sizeof(remote_address));
		dbcopy.getline(points, sizeof(points));
		dbcopy.getline(frags, sizeof(frags));
		dbcopy.getline(totalfrags, sizeof(totalfrags));
		dbcopy.getline(time, sizeof(time));
		dbcopy.getline(totaltime, sizeof(totaltime));
		dbcopy.getline(server_ip, sizeof(server_ip));
		dbcopy.getline(poll, sizeof(poll));
		if ( dbcopy.eof() )
		{
			break;
		}
		++record_no;

		if ( strlen( name ) == 0 )
		{ //should not happen
			continue;
		}
		strncpy_s( player.playername, sizeof(player.playername),
			name, _TRUNCATE);
		strncpy_s( player.remote_address, sizeof(player.remote_address),
			remote_address, _TRUNCATE);
		player.points = atof( points );
		player.frags      = 0;
		player.totalfrags = atoi( totalfrags );
		player.time       = 0.008;
		player.totaltime  = atof( totaltime );
		strncpy_s( player.ip, sizeof(player.ip), server_ip, _TRUNCATE);
		player.poll       = atoi( poll );
		// non-database properties
		player.score      = 0;
		player.ping       = 1;
		player.rank       = 1001;

		ReInsertPlayer( player );
		++new_record_count;
		if ( global_fail )
		{
			dbcopy.close();
			cout << "[E] ReInsertPlayer() failed at record no "
				<< record_no << " for " << name << endl;
			return false;
		}
	}
	dbcopy.close();
	cout << "  Initialize complete with "
		<< new_record_count << " records in database." << endl;

	return true;
}


int main( int argc, char** argv )
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
	testlog.open( "testlog" );
	if ( !testlog.bad() )
	{
		testlog << "STATSGEN TESTLOG" << endl;
	}
	else
	{
		cout << "[E] testlog failed open." << endl;
		return 1;
	}
#endif

	if ( !InitDatabase() )
	{
		cout << "[E] Database initial clean failed" << endl;
		return 1;
	}

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
	//Initialize FCE (look in KEYCODE.H for FCE_KEY_CODE)
	fceAttach(1, FCE_KEY_CODE);
#endif

	//load the current poll
	LoadPollNumber();

	//initialize time and timing variables
	GetSystemTime( &st_poll );
	st_tally = st_generate = st_backup = st_decay = st_poll;
	loop_t0 = GetTickCount();

	//initialize validated players list
	validated.count = 0;
	memset( validated.name, 0, sizeof( validated.name ) );

	while(1) {

		printf("Polling master.  Current poll: %i\n", currentpoll);
		GetServerList(); //poll the master
		currentpoll++;   //advance the poll sequence number
		if(currentpoll > 100000)
			currentpoll = 1;
		pollTally++;


		/*
		 * Collect data from servers at nominal 30 second intervals
		 *  select() timeout at .6 secs. (was 1 sec) seems to
		 *  keep loop interval at ~30 secs.
		 */
		loop_t1 = GetTickCount(); //monotonic, millisecs since system start
		if ( loop_t0 > loop_t1 )
		{ //wraps every 49 days, just re-init
			loop_t1 = loop_t0 = GetTickCount();
		}
		assert( loop_t1 >= loop_t0 );
		loop_msecs = loop_t1 - loop_t0;
		if ( loop_msecs < 28800 )
		{
			Sleep( 28800 - loop_msecs );
			loop_msecs = 30000;
		}
		realDelay = ((int)loop_msecs) / 1000; //poll interval in seconds
		loop_t0 = loop_t1;
#if defined TEST_LOG
		if ( realDelay > 35 )
			testlog << "poll interval: " << realDelay << endl;
#endif

		//get UTC wall clock time (used below for timed actions)
		GetSystemTime( &st_poll );
		cout << "UTC: "
			<< st_poll.wHour << ':' << st_poll.wMinute << ':' << st_poll.wSecond
			<< endl;
#if defined TEST_LOG
		testlog << "\nTIMESTAMP: "
			<< st_poll.wHour << ':' << st_poll.wMinute << ':' << st_poll.wSecond
			<< endl;
#endif
		//load list of currently validated players
		 GetValidatedPlayers();		

		//poll the servers and process player info
		for( i = 0; i < numServers; i++)
		{
			PingServers(&servers[i]);
		}

		//update daily averages
		if ( st_poll.wDayOfWeek != st_tally.wDayOfWeek && st_poll.wHour == 5 )
		{ //daily edge trigger at midnight EST
			//reset averages
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

		ifstream emptycheck( "playerrank.db" );
		if ( emptycheck.peek() == EOF )
		{ //empty db. branch around pointless file processing
			emptycheck.close();
			cout << "WARNING: playerrank.db is empty" << endl;
			goto emptybranch;
		}
		emptycheck.close();

		//generate the html stats pages at 15 minute intervals
		if ( (st_poll.wMinute > st_generate.wMinute
			&& (st_poll.wMinute - st_generate.wMinute > 15))
			|| ( st_poll.wMinute < st_generate.wMinute
			&& (st_generate.wMinute - st_poll.wMinute < 45)) )
		{
			GeneratePlayerRankingHtml();
#if defined CLANSTATS
			GenerateClanRankingHtml();
#else
			cout << "NOT Generating Clan Stats." << endl;
#endif
#if defined NO_UPLOAD
			cout << "NOT Uploading Stats." << endl;
#else
			UploadStats();
#endif
			st_generate = st_poll;
		}

		//each day, check for inactive players, and decay their point totals
		if ( st_poll.wDayOfWeek != st_decay.wDayOfWeek )
		{ //daily edge trigger
			cout << "Checking for inactive players..." << endl;
			CheckInactivePlayers();
			st_decay = st_poll;
		}

		//backup database every hour
		if ( st_poll.wHour != st_backup.wHour )
		{ //hourly edge trigger
			cout << "Backing up databases..." << endl;
			BackupStats();
			st_backup = st_poll;
		}

		//remove players from database who do not meet certain criteria
		CullDatabase();

emptybranch:
		//write out the completed poll number
		RecordPollNumber();

		//**** UGLY HACK ALERT ****
		//exit cleanly hack, create a file called 'exit_statsgen"
		//and delete it before restarting
		//proper way would be to catch abort signal
		ifstream exithack( "exit_statsgen" );
		if ( exithack )
		{
			exithack.close();
			break;
		}

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
