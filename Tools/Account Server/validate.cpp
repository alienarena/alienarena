#include  <ctype.h>
#include  <string.h>
#include  <iostream>
#include  <fstream>
#include  <time.h>
#include  <Windows.h>

#include "accountserv.h"

using namespace std;

extern SYSTEMTIME st;

void StripIllegalPathChars( char *name )
{
	int i = 0;

	while(name[i])
	{
		if(name[i] == ':' || name[i] == '*' || name[i] == '"' || name[i] == '/' ||
			name[i] == '?' || name[i] == '\\' || name[i] == '|' || name[i] == '<' ||
			name[i] == '>')
			name[i] = ' ';
		i++;
	} 
}

bool ValidatePlayer(char name[32], char password[256])
{
	ifstream playerProfileFile;
	ofstream newPlayerProfileFile;
	char szPath[256];
	char svPass[256];
	char svTime[32];

	StripIllegalPathChars(name);

	sprintf(szPath, "playerprofiles/%s", name);
	
	//open file
	playerProfileFile.open(szPath);

	//if no file, create one and return true
	if(!playerProfileFile)
	{
		printf("Creating new profile for %s\n", name);

		GetSystemTime(&st);
		sprintf(svTime, "%i-%i-%i-%i", st.wYear, st.wMonth, st.wDay, st.wHour);
		newPlayerProfileFile.open(szPath);
		newPlayerProfileFile << password <<endl;
		newPlayerProfileFile << svTime <<endl;
		newPlayerProfileFile.close();

		return true;
	}
	else
	{
		printf("Reading profile for %s\n", name);

		playerProfileFile.getline(svPass, 256);
		playerProfileFile.close();

		if(!_stricmp(svPass, password))
		{
			//matched!
			return true;
		}
		else
		{
			printf("[A]Invalid password for %s!\n", name);
			return false;
		}
	}	

	return false;
}

void ChangePlayerPassword(char name[32], char new_password[256])
{
	ofstream playerProfileFile;
	char szPath[256];
	char svTime[32];

	StripIllegalPathChars(name);

	sprintf(szPath, "playerprofiles/%s", name);
	
	remove(szPath);

	printf("Changing password for %s\n", name);

	GetSystemTime(&st);
	sprintf(svTime, "%i-%i-%i-%i", st.wYear, st.wMonth, st.wDay, st.wHour);
	playerProfileFile.open(szPath);
	playerProfileFile << new_password <<endl;
	playerProfileFile << svTime <<endl;
	playerProfileFile.close();
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
