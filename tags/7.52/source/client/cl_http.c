/*
* Copyright (C) 1997-2001 Id Software, Inc.
* Copyright (C) 2002 The Quakeforge Project.
* Copyright (C) 2006 Quake2World.
* Copyright (C) 2010 COR Entertainment, LLC.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or(at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined HAVE_UNLINK && !defined HAVE__UNLINK
#define _unlink unlink
#endif

#include "curl/curl.h"
CURLM *curlm;
CURL *curl;

// generic encapsulation for common http response codes
typedef struct response_s {
        int code;
        char *text;
} response_t;

response_t responses[] = {
        {400, "Bad request"}, {401, "Unauthorized"}, {403, "Forbidden"},
	{404, "Not found"}, {500, "Internal server error"}, {0, NULL}
};

char curlerr[MAX_STRING_CHARS];  // curl's error buffer

char url[MAX_OSPATH];  // remote url to fetch from
char dnld_file[MAX_OSPATH];  // local path to save to

long status, length;  // for current transfer
qboolean success;

/*
CL_HttpDownloadRecv
*/
size_t CL_HttpDownloadRecv(void *buffer, size_t size, size_t nmemb, void *p){
       return fwrite(buffer, size, nmemb, cls.download);
}

/*
CL_HttpDownload

Queue up an http download.  The url is resolved from cls.downloadurl and
the current gamedir.  We use cURL's multi interface, even tho we only ever
perform one download at a time, because it is non-blocking.
*/
qboolean CL_HttpDownload(void){
        char game[64];

        if(!curlm)
                return false;

        if(!curl)
                return false;

        memset(dnld_file, 0, sizeof(dnld_file));  // resolve local file name
        Com_sprintf(dnld_file, sizeof(dnld_file) - 1, "%s/%s", FS_Gamedir(), cls.downloadname);

        FS_CreatePath(dnld_file);  // create the directory

        if(!(cls.download = fopen(dnld_file, "wb"))){
                Com_Printf("Failed to open %s.\n", dnld_file);
                return false;  // and open the file
        }

		cls.downloadhttp = true;

        memset(game, 0, sizeof(game));  // resolve gamedir
        strncpy(game, Cvar_VariableString("game"), sizeof(game) - 1);

        if(!strlen(game))  // use default if not set
                strcpy(game, "arena");

        memset(url, 0, sizeof(url));  // construct url
        Com_sprintf(url, sizeof(url) - 1, "%s/%s/%s", cls.downloadurl, game, cls.downloadname);
	    // set handle to default state
        curl_easy_reset(curl);

        // set url from which to retrieve the file
        curl_easy_setopt(curl, CURLOPT_URL, url);

		// time out in 5s
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);

        // set error buffer so we may print meaningful messages
        memset(curlerr, 0, sizeof(curlerr));
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerr);

        // set the callback for when data is received
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CL_HttpDownloadRecv);

        curl_multi_add_handle(curlm, curl);

		return true;
}


/*
CL_HttpResponseCode
*/
char *CL_HttpResponseCode(long code){
        int i = 0;
        while(responses[i].code){
                if(responses[i].code == code)
                        return responses[i].text;
                i++;
        }
        return "Unknown";
}


/*
CL_HttpDownloadCleanup

If a download is currently taking place, clean it up.  This is called
both to finalize completed downloads as well as abort incomplete ones.
*/
void CL_HttpDownloadCleanup(){
        char *c;

        if(!cls.download || !cls.downloadhttp)
                return;

        curl_multi_remove_handle(curlm, curl);  // cleanup curl

        fclose(cls.download);  // always close the file
        cls.download = NULL;

		if(success){
		        cls.downloadname[0] = 0;
        }
        else {  // retry via legacy udp download

                c = strlen(curlerr) ? curlerr : CL_HttpResponseCode(status);

                Com_DPrintf("Failed to download %s via HTTP: %s.\n"
                                "Trying UDP..", cls.downloadname, c);

                MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
                MSG_WriteString(&cls.netchan.message, va("download %s", cls.downloadname));

                _unlink(dnld_file);  // delete partial or empty file
        }

        cls.downloadpercent = 0;
        cls.downloadhttp = false;

        status = length = 0;
        success = false;
}



/*
CL_HttpDownloadThink

Process the pending download by giving cURL some time to think.
Poll it for feedback on the transfer to determine what action to take.
If a transfer fails, stuff a stringcmd to download it via UDP.  Since
we leave cls.download.tempname in tact during our cleanup, when the
download is parsed back in the client, it will fopen and begin.
*/
void CL_HttpDownloadThink(void){
        CURLMsg *msg;
        int i;
		int runt;

        if(!cls.downloadurl[0] || !cls.download)
                return;  // nothing to do

        // process the download as long as data is avaialble
        while(curl_multi_perform(curlm, &i) == CURLM_CALL_MULTI_PERFORM){}

        // fail fast on any curl error
        if(strlen(curlerr)){
                CL_HttpDownloadCleanup();
                return;
        }

        // check for http status code
        if(!status){
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
                if(status > 0 && status != 200){  // 404, 403, etc..
                        CL_HttpDownloadCleanup();
                        return;
                }
        }

        // check for completion
        while((msg = curl_multi_info_read(curlm, &i))){
                if(msg->msg == CURLMSG_DONE){
						//not so fast, curl gives false positives sometimes
						runt = FS_LoadFile (cls.downloadname, NULL);
						if(runt > 2048)  { //the curl bug produces a 2kb chunk of data
							success = true;
							CL_HttpDownloadCleanup();
							CL_RequestNextDownload();
						}
						else {
							success = false;
							CL_HttpDownloadCleanup();
						}
                        return;
                }
        }
}


/*
CL_InitHttpDownload
*/
void CL_InitHttpDownload(void){
        if(!(curlm = curl_multi_init()))
                return;

        if(!(curl = curl_easy_init()))
                return;
}


/*
CL_ShutdownHttpDownload
*/
void CL_ShutdownHttpDownload(void){
        CL_HttpDownloadCleanup();

        curl_easy_cleanup(curl);
        curl = NULL;

        curl_multi_cleanup(curlm);
        curlm = NULL;
}
