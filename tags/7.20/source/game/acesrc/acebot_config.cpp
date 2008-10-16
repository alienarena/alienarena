//A C++ filestream module for reading bot config files

#include  <stdio.h>
#include  <stdlib.h>
#include  <math.h>
#include  <ctype.h>
#include  <string.h>
#include  <iostream.h>
#include  <iomanip> 
#include  <fstream>

using namespace std;

extern "C" struct botvals_s
{
	int skill;
	char faveweap[64];
	float weapacc[10];
	float awareness;
	char		chatmsg1[128];
	char		chatmsg2[128];
	char		chatmsg3[128];
	char		chatmsg4[128];
	char		chatmsg5[128];
	char		chatmsg6[128];
	char		chatmsg7[128];
	char		chatmsg8[128];
} botvals;

extern "C" void ACECO_ReadConfig(char config_file[128]);

void ACECO_ReadConfig(char config_file[128])
{
	ifstream infile;
	char a_string[128];
	int k;

	infile.open(config_file);

	//set bot defaults(in case no bot config file is present for that bot)
	botvals.skill = 1; //medium
	strcpy(botvals.faveweap, "None");
	for(k = 1; k < 10; k++) 
		botvals.weapacc[k] = 1.0;
	botvals.awareness = (float)0.7;

	strcpy( botvals.chatmsg1, "%s: I am the destroyer %s, not you."); 
	strcpy( botvals.chatmsg2, "%s: Are you using a bot %s?"); 
	strcpy( botvals.chatmsg3, "%s: %s is a dead man." ); 
	strcpy( botvals.chatmsg4, "%s: You will pay dearly %s!"); 
	strcpy( botvals.chatmsg5, "%s: Ackity Ack Ack!"); 
	strcpy( botvals.chatmsg6, "%s: Die %s!"); 
	strcpy( botvals.chatmsg7, "%s: My blood is your blood %s." ); 
	strcpy( botvals.chatmsg8, "%s: I will own you %s!"); 

	if(infile.good()) {
	
		infile.getline(a_string, 128); 
		botvals.skill = atoi(a_string);
		infile.getline(botvals.faveweap, 128); 

		for(k = 1; k < 10; k++) {
			infile.getline(a_string, 128);
			botvals.weapacc[k] = atof(a_string);
		}
		infile.getline(a_string, 128);
		botvals.awareness = atof(a_string); 

		infile.getline(botvals.chatmsg1, 128);
		infile.getline(botvals.chatmsg2, 128);
		infile.getline(botvals.chatmsg3, 128);
		infile.getline(botvals.chatmsg4, 128);
		infile.getline(botvals.chatmsg5, 128);
		infile.getline(botvals.chatmsg6, 128);
		infile.getline(botvals.chatmsg7, 128);
		infile.getline(botvals.chatmsg8, 128);

		infile.close();
	}

	return;
}
