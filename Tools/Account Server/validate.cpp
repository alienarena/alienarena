#include  <ctype.h>
#include  <string.h>
#include  <iostream>
#include  <fstream>
#include  <time.h>
#include  <Windows.h>

#include "accountserv.h"

using namespace std;

extern SYSTEMTIME st;

bool ValidatePlayer(char name[32], char password[32])
{
	ifstream playerProfileFile;
	ofstream newPlayerProfileFile;
	char szPath[256];
	char svPass[32];
	char svTime[32];

	sprintf(szPath, "playerprofiles/%s", name);
	
	//open file
	playerProfileFile.open(szPath);

	//if no file, create one and return true
	if(!playerProfileFile)
	{
		printf("creating new profile for %s\n", name);

		GetSystemTime(&st);
		sprintf(svTime, "%i-%i-%i-%i", st.wYear, st.wMonth, st.wDay, st.wHour);
		newPlayerProfileFile.open(szPath);
		newPlayerProfileFile << password<<endl;
		newPlayerProfileFile << svTime<<endl;
		newPlayerProfileFile.close();

		return true;
	}
	else
	{
		printf("reading profile for %s\n", name);

		playerProfileFile.getline(svPass, 32);
		playerProfileFile.close();

		if(!_stricmp(svPass, password))
		{
			//matched!
			return true;
		}
		else
			return false;
	}	

	return false;
}

void DumpValidPlayersToFile(void)
{
	ofstream	currPlayers;
	player_t	*player = &players;

	currPlayers.open("validated");
	while (player->next)
	{
		player = player->next;
		currPlayers << player->name<<endl;		
	}
	currPlayers.close();
}
