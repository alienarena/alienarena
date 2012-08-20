/*
Copyright (C) 2011 COR Entertainment, LLC.

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

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "curl/curl.h"
static CURL *curl;

extern cvar_t  *cl_latest_game_version_url;


static char *versionstr;
static size_t versionstr_sz;

static char* extend_versionstr ( size_t bytecount )
{
	char *new_versionstr;
	size_t cur_sz = versionstr_sz;
	if ( cur_sz ){
	    versionstr_sz += bytecount;
	    new_versionstr = realloc ( versionstr, versionstr_sz );
	    if (new_versionstr == NULL) {
	    	free (versionstr);
	    	Com_Printf ("WARN: SYSTEM MEMORY EXHAUSTION!\n");
	    	versionstr_sz = 0;
	        return NULL;
	    }
	    versionstr = new_versionstr;
	    return versionstr+cur_sz;
	}
	versionstr_sz = bytecount;
	versionstr = malloc ( versionstr_sz );
	return versionstr;
}

static size_t write_data(const void *buffer, size_t size, size_t nmemb, void *userp)
{
	char *buffer_pos;
	size_t bytecount = size*nmemb;

	buffer_pos = extend_versionstr ( bytecount );
	if (!buffer_pos)
	    return 0;

	memcpy ( buffer_pos, buffer, bytecount );
	buffer_pos[bytecount] = 0;

	return bytecount;
}

void getLatestGameVersion( void )
{
	char url[128];
	CURL* easyhandle;

	easyhandle = curl_easy_init() ;

    versionstr_sz = 0;

	Com_sprintf(url, sizeof(url), "%s", cl_latest_game_version_url->string);

	if (curl_easy_setopt( easyhandle, CURLOPT_URL, url ) != CURLE_OK) return ;

	// time out in 5s
	if (curl_easy_setopt(easyhandle, CURLOPT_CONNECTTIMEOUT, 5) != CURLE_OK) return ;

	if (curl_easy_setopt( easyhandle, CURLOPT_WRITEFUNCTION, write_data ) != CURLE_OK) return ;

	if (curl_easy_perform( easyhandle ) != CURLE_OK) return;

	(void)curl_easy_cleanup( easyhandle );

    if ( versionstr ){
        if ( atof ( versionstr ) )
            Cvar_SetValue ( "cl_latest_game_version", atof ( versionstr ) );
        free ( versionstr );
    }

}
