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
#include <iostream.h>
#include <iomanip> 
#include <fstream>
#include <time.h>
#include "fce.h"
#include "keycode.h"

using namespace std;

typedef struct _PLAYERINFO {
	char playername[16];
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

int verify_address(char db_address[21], char address[21])
{
	int i, j;
	char add1[21], add2[21]; 
	char *add, *dbadd;
	//here will determine whether or not this is a valid player, and whether or not we want to
	//update his stats.  

	//if the person in the db has an address of 127.0.0.1, then no matter what, we update it
	//if the person has an address in the db that is not 127.0.0.1, then only return valid if
	//they match.

	//since some people still use dynamic ip's, we will limit the comparison to the first two 
	strcpy(add1, address);
	strcpy(add2, db_address);
	add = add1;
	dbadd = add2;

	//trim off the addies
	j = 0;
	for(i = 0; i<strlen(dbadd); i++) {
		if(dbadd[i] == '.')
			j++;
		if(j == 2)
			dbadd[i] = 0;
	}
	j = 0;
	for(i = 0; i<strlen(add); i++) {
		if(add[i] == '.')
			j++;
		if(j == 2)
			add[i] = 0;
	}
return true; //implement this once all servers up updated
	if(!strcmp(dbadd, "127.0"))
		return true;
//	else if(!strcmp(add, "192.168")) //home server, don't count towards stats
//		return false; 
	
	else if(!strcmp(add, dbadd))
		return true;
	else
		return false;
}

PLAYERINFO LoadPlayerInfo(PLAYERINFO player)
{	
	ifstream infile;
	ofstream outfile;
	int foundplayer, spoofed;
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];

	//find this playername in the database, if he doesn't exist, insert him at the bottom
	//of the rankings

	infile.open("playerrank.db");

	player.rank = 1;
	foundplayer = false;
	spoofed = false;

	if(infile.good()) {

		strcpy(name, "go");

		while(strlen(name)) { //compare ip's AND name
			infile.getline(name, 32); //name
			infile.getline(remote_address, 21); //remote address
			infile.getline(points, 32); //points
			if(!strcmp(player.playername, name) && verify_address(remote_address, player.remote_address))
				player.points = atof(points);
			infile.getline(frags, 32); //frags
			if(!strcmp(player.playername, name) && verify_address(remote_address, player.remote_address))
				player.frags = atoi(frags);
			infile.getline(totalfrags, 32); //total frags
			if(!strcmp(player.playername, name) && verify_address(remote_address, player.remote_address))
				player.totalfrags = atoi(totalfrags);
			infile.getline(time, 16); //current time in poll
			if(!strcmp(player.playername, name) && verify_address(remote_address, player.remote_address)) {
				player.time = atof(time);
				if(player.time > 200) //either a bot, or idling zombie
					player.time = .008; 
			}
			infile.getline(totaltime, 16);
			if(!strcmp(player.playername, name) && verify_address(remote_address, player.remote_address)) 
				player.totaltime = atof(totaltime);
			infile.getline(ip, 16); //last server.ip
			if(!strcmp(player.playername, name) && verify_address(remote_address, player.remote_address)) 
				strcpy(player.ip, ip);
			infile.getline(poll, 16); //what poll was our last?
			if(!strcmp(player.playername, name) && verify_address(remote_address, player.remote_address)) { 
				player.poll = atoi(poll);
				foundplayer = true;
				break; //get out we are done
			}
			if(!strcmp(player.playername, name) && !verify_address(remote_address, player.remote_address)) { 
				spoofed = true;
				foundplayer = false;
				break;
			}
			player.rank++;
		}
	}

	infile.close();

	if(!foundplayer) {
		//we didn't find this player, so give him some defaults and insert him
		outfile.open("playerrank.db", ios::app);
		if(strlen(player.playername)) { //don't write out blank player names
			if(spoofed)
				outfile << "Spoofer" << endl; //this way we can catch, and ban cheaters
			else
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

	if(!strlen(player.playername))
		return; //don't bother with assholes who put in blank names

	infile.open("playerrank.db");

	outfile.open("temp.db");

	if(infile.good()) {

		strcpy(name, "go");
		inserted = false;
		total_players = 0;
		//the idea of this is, when rebuilding this file, keep it to a reasonable size
		//we we only be tracking the top 1000 players then.

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

			if(((atof(points)/atof(totaltime)) < (player.points/player.totaltime)) && !inserted) { //this will add new ranking
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
			if(strcmp(name, player.playername) && strlen(name)) { //this will remove old ranking
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
	int i, j;
	double total_score;
	SOCKADDR_IN addr;
	char server_ip[16];
	int realpeeps;

	//convert the ip to a string
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.S_un.S_addr = server->ip;
	sprintf (server_ip, "%s", inet_ntoa(addr.sin_addr));

	for(i = 0; i < server->curClients; i++) {

		//load player info
		server->players[i] = LoadPlayerInfo(server->players[i]);

		//reset your time if polls or ip do not match properly
		if(server->players[i].poll != currentpoll-1 || 
			(strcmp(server->players[i].playername, "Player") && strcmp(server->players[i].ip, server_ip)))
			server->players[i].time = .008;

		//get the total amount of points in the game, we need to add percentages, in order to
		//take into account CTF and Deathball games skewing scoring
		total_score = 1;
		for ( j = 0; j < server->curClients; j++) 
		{
			if(server->players[j].score > 0) //don't let idiots or bots ruin the total
				total_score += server->players[j].score;
		}

		//check against scores of other players in this game and add points if appropriate
		realpeeps = 0;
		for( j = 0; j < server->curClients; j++) {
			if(j == i) {
				if (j < server->curClients)
					j++; //don't add in your own score!
			}
			server->players[j] = LoadPlayerInfo(server->players[j]);
			if(server->players[j].ping !=0 && ((server->players[i].score/server->players[i].time)
				>= (server->players[j].score/server->players[j].time))) { //better fragrate
				//add points for players below this player(only if a positive score and not a bot)
				if(server->players[i].score >= 0 && server->players[i].ping != 0 && total_score >= 0
					&& server->players[j].score >= 0) {
					if((server->players[j].score/total_score)/((server->players[j].rank + 1) * 5.0) > 0) //just to be 100% sure we aren't subtracting
						server->players[i].points += (server->players[j].score/total_score)/((server->players[j].rank + 1) * 5.0);
				}
			}
			if(server->players[j].ping !=0)
				realpeeps++;
		}
		//add in a frag percentage bonus here - rewards players who have large percentage
		//of total frags in the game
		if( realpeeps && server->curClients > 1 && server->players[i].score >= 0 && total_score > 0 && server->players[i].ping != 0)
			server->players[i].points += server->players[i].score/(50 * total_score);

		//re-insert this player into the database
		//first tally up total frags by comparing with our last poll's number(only if player is 
		//currently in consecutive polls
		if(server->players[i].poll == currentpoll-1) { 
			if((server->players[i].score - server->players[i].frags) > 0)
				server->players[i].totalfrags += (server->players[i].score - server->players[i].frags);
		}
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
		ReInsertPlayer(server->players[i]);
	}

	return;
}

void GetServerList (void) {
	
	HOSTENT *hp;
	int i, result;
	struct timeval delay;
	fd_set stoc;
	struct sockaddr_in dgFrom;
	char recvBuff[0xFFFF], *p;

	hp = gethostbyname ("192.168.0.2");	

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
	char request[] = "ÿÿÿÿstatus\n";
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

		free (rLine);

		//playerinfo
		strcpy (seps, " ");
		while (rLine = GetLine (&p, &result)) {

			/* Establish string and get the first token: */
			token = strtok( rLine, seps);
			server->players[players].score = atoi(token);

			token = strtok( NULL, seps);
			server->players[players].ping = atoi(token);
			if(server->players[players].ping > 0)
				real_players++;

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

void GeneratePlayerRankingHtml(void) //Top 1000 players
{
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];
	ifstream infile;
	ofstream outfile;
	int rank;
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

			actualtime = atof(totaltime) - 1.0;
			fragrate = atof(totalfrags)/actualtime;
			actualpoints = 100*atof(points)/atof(totaltime);

			//build row for this player
			outfile << "<tr>" << endl;
			sprintf(a_string, "<td>%i</td>", rank);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%s</td>", name);
			outfile << a_string << endl;
			sprintf(a_string, "<td>%4.2f</td>", actualpoints);
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
						clans[i].players[j].totalfrags = atof(totalfrags);
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

	printf("Uploading to ftp server.\n");

	fceSetInteger(0, FCE_SET_PASSIVE, 1);
	fceSetInteger(0, FCE_SET_CONNECT_WAIT, 1000);
    fceSetInteger(0, FCE_SET_MAX_RESPONSE_WAIT, 1000);
	
	// Connect to FTP server
	error = fceConnect(0,"ftp.mysite.com","user","password");
	if(error < 0) {
		printf("Error connecting to host!\n");
		return;
	}

	//change to correct dir
	error = fceSetServerDir (0, "cor.planetquake.gamespy.com/stats");
	if(error < 0) {
		printf("Error changing directory!\n");
		fceClose(0);
		return;
	}
	else
		printf("Connected Succesfully!\n");
	
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

	error = fcePutFile(0, "playerrank.db");
	if(error < 0) {
		printf("Error uploading player database file!\n", i);
		fceClose(0);
		return;
	}
	else
		printf("Player database successfully uploaded!\n");


	fceClose(0);

}

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
	char name[16], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16], remote_address[21];
	ifstream infile;
	ofstream outfile;
	int diff;
	double pointnumber;
	PLAYERINFO player[1101]; //very unlikely to ever use all of this
	int i, j;

	i = 0;

	infile.open("playerrank.db");

	if(infile.good()) {
		
		strcpy(name, "go");
	
		while(strlen(name)) {

			infile.getline(name, 16); //name
			infile.getline(remote_address, 21); //remote address
			infile.getline(points, 32); //points
			pointnumber = atof(points); //so we can manipulate this
			infile.getline(frags, 32); //frags
			infile.getline(totalfrags, 32); //total frags
			infile.getline(time, 16); //current time in poll
			infile.getline(totaltime, 16);
			infile.getline(ip, 16); //last server.ip
			infile.getline(poll, 16); //what poll was our last?

			//check for inactivity of 1 week
			diff = currentpoll - atoi(poll);
			if(diff < 0) //cycled around
				diff += 100000;
			
			if(diff > 20160)  //1 week since player was polled
			{
				pointnumber *= .90; //start dropping each day
				//add to data list
				strcpy(player[i].playername, name);
				strcpy(player[i].remote_address, remote_address);
				player[i].points = pointnumber;
				player[i].frags = atof(frags);
				player[i].totalfrags = atof(totalfrags);
				player[i].time = atof(time);
				player[i].totaltime = atof(totaltime);
				strcpy(player[i].ip, ip);
				player[i].poll = atoi(poll);
				i++;

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
	char name[16], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[16], poll[16];
	ifstream infile;
	ofstream outfile;

	infile.open("playerrank.db");
	outfile.open("newdb.db");

	if(infile.good()) {
		
		strcpy(name, "go");
	
		while(strlen(name)) {

			infile.getline(name, 16); //name
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
	double generate_time, backup_time, check_decay_time;
	SYSTEMTIME st;
	int oldsecond, newsecond;
	int adjustedDelay = 0;

	//just stick this here

	//RepairDb();
	//return 0;
	
	WSACleanup(); //Cleanup Winsock - not sure if this needed, but can it hurt?
	
	//initialize Winsock
	error = WSAStartup ((WORD)MAKEWORD (1,1), &ws);

	if (error) {
		printf("Couldn't load Winsock!");
		return 0;
	}
	
	//open a socket for polling the master
	master = socket (AF_INET, SOCK_DGRAM, 0);

	// Initialize FCE (look in KEYCODE.H for FCE_KEY_CODE)
	fceAttach(1, FCE_KEY_CODE);
	
	generate_time = 0;
	backup_time = 0;
	check_decay_time = 0;

	//load the current poll
	LoadPollNumber();

	while(1) {

		printf("Polling master.  Current poll: %i\n", currentpoll);
		GetServerList(); //poll the master
		currentpoll++;
		if(currentpoll > 100000)
			currentpoll = 0;

		pollTally++;

		if(adjustedDelay >= 30)
			adjustedDelay = 29;

		Sleep(30000-(adjustedDelay*1000));
			
		//now ping each server and process game and player data
		GetSystemTime(&st); //time before processing
		oldsecond = st.wSecond;

		for( i = 0; i < numServers; i++) {
			PingServers(&servers[i]);
		}

		if(st.wHour == 5 && st.wMinute < 5) 
			playerTally = OLplayerTally = pollTally = 0; //reset the average at midnite EST
		playerTally += (real_players+(numServers - numLiveServers - 5)); 
		OLplayerTally += real_players;
		//total players:
		//this consists of all players currently in a server with a ping, and also counts
		//each uresponsive listen server as 1 player.  This is the truest measure of game 
		//activity.
		printf("Servers: %i Players: %i Real Players: %i Online: %i Daily Avg: %4.2f/%4.2f\n", numServers, total_players, (real_players+(numServers - numLiveServers - 5)), real_players, playerTally/pollTally, OLplayerTally/pollTally);
		total_players = 0;
		real_players = 0;
		numLiveServers = 0;
		
		//generate an html output file
		if(generate_time > 30) { //120 is one hour
			generate_time = 0;
			GeneratePlayerRankingHtml();
			GenerateClanRankingHtml();
			UploadStats();
		}
		generate_time++;

		//backup database every hour
		if(backup_time > 120) {
			printf("Backing up databases...\n");
			backup_time = 0;
			BackupStats();
		}
		backup_time++;

		//each day, check for inactive players, and decay their point totals
		if(check_decay_time > 2880) {
			printf("Checking for inactive players...\n");
			check_decay_time = 0;
			CheckInactivePlayers();
		}
		check_decay_time++;

		//write out the poll number
		RecordPollNumber();

		GetSystemTime(&st);
		newsecond = st.wSecond;

		adjustedDelay = newsecond - oldsecond; 
		if (adjustedDelay <= 0)
			adjustedDelay+=60;

		realDelay = adjustedDelay; //prior to adjusting it

	}
	
	fceRelease();
	
	return 0;
}
