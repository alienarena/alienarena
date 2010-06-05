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
// cl_irc.c  -- irc client

#include "client.h"
#ifdef _WINDOWS
	#include <winsock.h>
	#include <process.h>
	typedef SOCKET irc_socket_t;
#else
	typedef int irc_socket_t;
#endif

#ifdef __unix__
	#include <unistd.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <sys/param.h>
	#include <sys/ioctl.h>
	#include <sys/uio.h>
	#include <errno.h>
	#include <pthread.h>
#endif

char Server[32];
char sMessage[1000];
char message[200];				   
user_t user;
irc_socket_t sock;               /* socket */
struct sockaddr_in address;      /* socket address stuff */
struct hostent * host;           /* host stuff */
char File_Buf[3000];			 /* file buffer */
char HostName[100];              /* host name from user */
int len;                         
int ichar;

#define IRC_SEND_BUF_SIZE 512
#define IRC_RECV_BUF_SIZE 1024

#ifdef _WINDOWS
void handle_error(void)
{
    switch ( WSAGetLastError() )
    {
      case WSANOTINITIALISED :
		Com_Printf("Unable to initialise socket.\n");
      break;
      case WSAEAFNOSUPPORT :
        Com_Printf("The specified address family is not supported.\n");
      break;
      case WSAEADDRNOTAVAIL :
        Com_Printf("Specified address is not available from the local machine.\n");
      break;
      case WSAECONNREFUSED :
        Com_Printf("The attempt to connect was forcefully rejected.\n"); 
        break; 
      case WSAEDESTADDRREQ : 
        Com_Printf("address destination address is required.\n");
      break;
      case WSAEFAULT :
        Com_Printf("The namelen argument is incorrect.\n");
      break;
      case WSAEINVAL :
        Com_Printf("The socket is not already bound to an address.\n");
      break;
      case WSAEISCONN :
        Com_Printf("The socket is already connected.\n");
      break;
      case WSAEADDRINUSE :
        Com_Printf("The specified address is already in use.\n");
      break;
      case WSAEMFILE : 
        Com_Printf("No more file descriptors are available.\n");
      break;
      case WSAENOBUFS :
        Com_Printf("No buffer space available. The socket cannot be created.\n");
      break;
      case WSAEPROTONOSUPPORT :
        Com_Printf("The specified protocol is not supported.\n");
        break; 
      case WSAEPROTOTYPE :
        Com_Printf("The specified protocol is the wrong type for this socket.\n");
      break;
      case WSAENETUNREACH : 
        Com_Printf("The network can't be reached from this host at this time.\n");
      break; 
      case WSAENOTSOCK :
         Com_Printf("The descriptor is not a socket.\n");
      break;
      case WSAETIMEDOUT :
        Com_Printf("Attempt timed out without establishing a connection.\n");
      break;
      case WSAESOCKTNOSUPPORT :
         Com_Printf("Socket type is not supported in this address family.\n");
      break;
      case WSAENETDOWN :
        Com_Printf("Network subsystem failure.\n");
      break;
      case WSAHOST_NOT_FOUND :
        Com_Printf("Authoritative Answer Host not found.\n");
      break;
      case WSATRY_AGAIN :
        Com_Printf("Non-Authoritative Host not found or SERVERFAIL.\n");
       break;
      case WSANO_RECOVERY :
         Com_Printf("Non recoverable errors, FORMERR, REFUSED, NOTIMP.\n");
      break;
      case WSANO_DATA :
        Com_Printf("Valid name, no data record of requested type.\n");
      break;
        case WSAEINPROGRESS :
        Com_Printf("address blocking Windows Sockets operation is in progress.\n");
      break;
      default :
        Com_Printf("Unknown connection error.\n");
       break;
	}
}
//#endif
#else
void handle_error( void )
{
	Com_Printf("IRC socket connection error.\n");
}
#endif

void sendData(char *msg)
{
	send(sock, msg,strlen(msg),0); 
}

IRCresponse_t SkipWords(char msg[250],int skip)
{
	IRCresponse_t res;
	int t;
	int ichar=0;
	int counter=0;

	memset((res.word),'\0',200*1);
	for (t=0; t<256;t++)
	{
		if (msg[t]==' ') counter++;
		if (counter>=skip) {
			res.word[0][ichar]=msg[t];
			ichar++;
		};
	}
	return res;
}

IRCresponse_t GetWords(char msg[200])
{
	IRCresponse_t res;
	int t;
	int word=0;
	int ichar=0;

	memset((res.word),'\0',200*50);

	for (t=0; t<200;t++)
	{
		res.word[word][ichar] = msg[t];
		ichar++;
		if (msg[t]==' ') {
			ichar=0;
			word++; 
		};
	}
	res.words = word;
	return res;
};

void analizeLine(char Line[1000])
{
	IRCresponse_t Response;
    char cmpmsg[10][200]; 
    char outputmsg[1000];
	char msgLine[101];
	int i, j, ti, lines;
	qboolean printed = false;

	outputmsg[0] = 0;
	msgLine[0] = 0;
	
	sprintf(cmpmsg[0],"353 %s", user.nick);
	if ((strstr(Line,cmpmsg[0]) != NULL))
    {
		memset((sMessage),'\0',1000);
		memset((Response.word[0]),'\0',1000);
		memset((outputmsg),'\0',1000);	
		for(ti=0;ti<40;ti++)
		{
			sMessage[ti]=Line[ti];
			if (Line[ti]==':') 
			{
				Response = SkipWords(Line,6);
						
				//this is the string we need to check length of, and if it's over the size of 
				//the window width, we need to split it into multiple lines.  Should not be too
				//difficult to implement.
				sprintf(outputmsg,"<%s> %s ",sMessage,Response.word[0]);
				outputmsg[strlen(outputmsg)-2] = 0; //don't want that linefeed showing up
							
				lines = strlen(outputmsg)/80 + 1; //how many lines do we have?
				for(i=0; i<lines; i++) {
					//get a segment of the total message
					memset(msgLine,'\0', 81);
					for(j=0; j<80; j++) 
						msgLine[j] = outputmsg[j+(80*i)];
					msgLine[80] = 0;

					SCR_IRCPrintf("^1IRC: %s\n", msgLine); //com_printf here for test

					printed = true;
				}
			}
		}
		
	 }

     sprintf(cmpmsg[0],"372 %s", user.nick);
     sprintf(cmpmsg[1],"251 %s", user.nick);
     sprintf(cmpmsg[2],"252 %s", user.nick);
     sprintf(cmpmsg[3],"253 %s", user.nick);
     sprintf(cmpmsg[4],"254 %s", user.nick);
     sprintf(cmpmsg[5],"255 %s", user.nick);
     sprintf(cmpmsg[6],"322 %s", user.nick); // <-- LIST
     sprintf(cmpmsg[7],"421 %s", user.nick); // <-- unkwown command, etc.
     sprintf(cmpmsg[8],"461 %s", user.nick); // <-- no enougth parameters, etc.
				    
   if ((strstr(Line,cmpmsg[0]) != NULL)||(strstr(Line,cmpmsg[1]) != NULL)||(strstr(Line,cmpmsg[2]) != NULL)||(strstr(Line,cmpmsg[3]) != NULL)||(strstr(Line,cmpmsg[4]) != NULL)||(strstr(Line,cmpmsg[5]) != NULL)||(strstr(Line,cmpmsg[6]) != NULL)||(strstr(Line,cmpmsg[7]) != NULL)||(strstr(Line,cmpmsg[8]) != NULL))
   {
	   memset((Response.word[0]),'\0',256);
	   Response = SkipWords(Line,3);
	   Response.word[0][strlen(Response.word[0])-2]= 0;
		
	   SCR_IRCPrintf("^1IRC: %s\n", msgLine); //com_printf here for test

	   printed = true;
	}


	sprintf(cmpmsg[0],"PRIVMSG #"); // <-- LIST
	if ((strstr(Line,cmpmsg[0]) != NULL))
	{
		memset((sMessage),'\0',1000);
		memset((Response.word[0]),'\0',1000);
		memset((outputmsg),'\0',1000);
					
		for(ti=0;ti<40;ti++)
		{
			sMessage[ti]=Line[ti];
			if (Line[ti]=='!') 
			{
				Response = SkipWords(Line,3);
						
				//this is the string we need to check length of, and if it's over the size of 
				//the window width, we need to split it into multiple lines.  Should not be too
				//difficult to implement.
				sprintf(outputmsg,"<%s> %s ",sMessage,Response.word[0]);
				outputmsg[strlen(outputmsg)-2] = 0; //don't want that linefeed showing up
							
				lines = strlen(outputmsg)/100 + 1; //how many lines do we have?
				for(i=0; i<lines; i++) {
					//get a segment of the total message
					memset(msgLine,'\0', 101);
					for(j=0; j<100; j++) 
						msgLine[j] = outputmsg[j+(100*i)];
					msgLine[100] = 0;

					SCR_IRCPrintf("^1IRC: %s\n", msgLine); //com_printf here for test

					printed = true;
				}
			}
		}
	}
	
	if(!printed) //users coming and going
	{
		memset((sMessage),'\0',1000);
		memset((Response.word[0]),'\0',1000);
		memset((outputmsg),'\0',1000);
			
		for(ti=0;ti<40;ti++)
		{
			sMessage[ti]=Line[ti];
			if (Line[ti]=='!') 
			{
				Response = SkipWords(Line,1);
						
				//this is the string we need to check length of, and if it's over the size of 
				//the window width, we need to split it into multiple lines.  Should not be too
				//difficult to implement.
				sprintf(outputmsg,"<%s> %s ",sMessage,Response.word[0]);
				outputmsg[strlen(outputmsg)-2] = 0; //don't want that linefeed showing up
							
				lines = strlen(outputmsg)/100 + 1; //how many lines do we have?
				for(i=0; i<lines; i++) {
					//get a segment of the total message
					memset(msgLine,'\0', 101);
					for(j=0; j<100; j++) 
						msgLine[j] = outputmsg[j+(100*i)];
					msgLine[100] = 0;

					SCR_IRCPrintf("^1IRC: %s\n", msgLine); 

				}
			}
		}
	}

}

void CL_GetIRCData(void) 
{
	int ichar =0;
	char line[IRC_RECV_BUF_SIZE];
	char prevLine[IRC_RECV_BUF_SIZE]; 
	int t;

	//to do - flag if connected or not

	len=0;	
	memset(File_Buf,0,IRC_RECV_BUF_SIZE);
	if((len=recv(sock,File_Buf,IRC_RECV_BUF_SIZE,0))>0) 
	{
	    // received a ping from server...
#ifdef _WINDOWS	    
		if (!strnicmp(File_Buf,"PING",4)) 
#else
		if (!strncasecmp(File_Buf,"PING",4)) 
#endif		
		{
			File_Buf[1]='O'; 
			sendData(File_Buf);
		};
	}

	//send the buffer to be analyzed
	if (len > 1) 
	{		
		memset(line,0,IRC_RECV_BUF_SIZE);
		for (t=0;t<strlen(File_Buf);t++)
		{
			line[ichar]= File_Buf[t];
			ichar++;
			if(File_Buf[t]==13) {
				ichar=0;
				if(!strcmp(line, prevLine)) { //don't print duplicate messages
					memset(line,0,IRC_RECV_BUF_SIZE);
					return;
				}
				analizeLine(line);
				strcpy(prevLine, line);
				memset(line,0,IRC_RECV_BUF_SIZE);
			};
		}
	}

}

void CL_IRCSay(void) 
{
	char msgLine[111];
	char tempstring[1024]; 
	int i, j, lines;
	char m_sendstring[1024];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: irc_say <text>\n");
		return;
	}

	strcpy(m_sendstring, Cmd_Argv (1));

	//check if it's an "action"
	if(m_sendstring[0] == '/' && m_sendstring[1] == 'm' && m_sendstring[2] == 'e') {
		//trim out the command
		for(i = 4; i < 1024; i++)
			tempstring[i-4] = m_sendstring[i];
		sprintf(message, "PRIVMSG #alienarena :\001ACTION %s\001\n\r", tempstring);

#ifdef _WINDOWS
		WSASetLastError(0);
#endif
		sendData(message);
		//send a junk string to clear the command
		sendData("PRIVMSG #alienarena :\n\r");
	}
	else {
		sprintf(message, "PRIVMSG #alienarena :%s\n\r", m_sendstring);
#ifdef _WINDOWS
		WSASetLastError(0);
#endif
		sendData(message);
	}

#ifdef _WINDOWS
	if(WSAGetLastError()) { //there was some error in connecting
			handle_error();
			Com_Printf("^1IRC: not connected to #alienarena");
	}
	else {
#endif
		//update the print buffer
		sprintf(message, "<%s> :%s", user.nick, m_sendstring);

		lines = 0;
		lines = strlen(message)/110 + 1; //how many lines do we have?
		for(i=0; i<lines; i++) {
			//get a segment of the total message
			memset(msgLine,'\0', 111);
			for(j=0; j<110; j++) 
				msgLine[j] = message[j+(110*i)];
			msgLine[110] = 0;

			SCR_IRCPrintf("^1IRC: %s\n", msgLine);
		}
#ifdef _WINDOWS
	}
#endif
}

#ifdef _WINDOWS
void RecvThreadProc(void *dummy)
{

	while(1) {

		//try not to eat up CPU
		Sleep(1000); //time to recieve packets

		CL_GetIRCData();	
	}
	return;
}
#endif

#ifdef __unix__
void *RecvThreadProc(void *dummy)
{

	while(1) {

		//try not to eat up CPU
		sleep(1); //time to recieve packets

		CL_GetIRCData();	
	}
	return;
}
#endif

void CL_InitIRC(void)
{

	char message[IRC_SEND_BUF_SIZE];
	char name[32];
	int i, j;
#ifdef __unix__
	pthread_t pth;
#endif

	if(cls.irc_connected)
		Com_Printf("...already connected to IRC\n");

	Com_Printf("...Initializing IRC client\n");

#ifdef _WINDOWS   
	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
#else
	if ( (sock = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
#endif
		return;	

	strcpy(name, Cvar_VariableString("name")); //we should force players to set name on startup

	if(!strcmp(name, "Player")) {
		Com_Printf("...IRC rejected due to unset player name\n");
		return;
	}

	//strip color chars
	j = 0;
	for (i = 0; i < 16; i++)
		user.nick[i] = 0;
		for (i = 0; i < strlen(name) && i < 32; i++) {
			if ( name[i] == '^' ) {
				i += 1;
				continue;
			}
		user.nick[j] = name[i];
		j++;
	}

	strcpy(user.email, "mymail@mail.com"); 

	address.sin_family=AF_INET;       // internet 
    address.sin_port = htons(6667);    

	sprintf(HostName, cl_IRC_server->string);

    if ( (host=gethostbyname(HostName)) == NULL ) {
#ifdef _WINDOWS
		closesocket(sock);
#else
		close(sock);
#endif
		handle_error();
 		return;
	}

    address.sin_addr.s_addr=*((unsigned long *) host->h_addr);

	if ( (connect(sock,(struct sockaddr *) &address, sizeof(address))) != 0) {
#ifdef _WINDOWS
		closesocket(sock);
#else
		close(sock);
#endif
		Com_Printf("...IRC connection refused.\n");
		return;
	}

	sprintf(message,"USER %s %s: %s %s  \n\r", user.nick , user.email , user.nick , user.nick );
	sendData(message);
	sprintf(message,"NICK %s\n\r", user.nick);
	sendData(message);

	sprintf(message,"JOIN %s\n\r", "#alienarena");
	sendData(message);

#ifdef _WINDOWS
	if(WSAGetLastError()) {

		closesocket(sock);
		handle_error();
		return; 
	}
#endif

	cls.irc_connected = true;

	Com_Printf("...Connected to IRC server\n");

	//we are in, start thread for data
#ifdef _WINDOWS
	_beginthread( RecvThreadProc, 0, NULL );
#endif
#ifdef __unix__
	pthread_create(&pth,NULL,RecvThreadProc,"dummy");
#endif

}

void CL_IRCShutdown(void)
{
	char message[IRC_SEND_BUF_SIZE];

	if(!cls.irc_connected)
		return;

	//logout
	sprintf(message,"QUIT\n\r");
	sendData(message);

	cls.irc_connected = false;

	Com_Printf("Disconnected from chat channel...");

#ifdef _WINDOWS
	closesocket(sock);
#else
	close(sock);
#endif
	return;
}

