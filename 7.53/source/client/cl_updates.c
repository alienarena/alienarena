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
CURLM *curlm;
CURL *curl;

extern cvar_t  *cl_latest_game_version_url;


static char *versionstr;
static size_t versionstr_sz;

static char* extend_versionstr ( size_t bytecount )
{
	size_t cur_sz = versionstr_sz;
	if ( cur_sz ){
	    versionstr_sz += bytecount;
	    versionstr = realloc ( versionstr, versionstr_sz );
	    if (!versionstr)
	        return versionstr;
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

	curl_easy_setopt( easyhandle, CURLOPT_URL, url ) ;

	// time out in 5s
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);

	curl_easy_setopt( easyhandle, CURLOPT_WRITEFUNCTION, write_data ) ;

	curl_easy_perform( easyhandle );

	curl_easy_cleanup( easyhandle );

    if ( versionstr ){
        if ( atof ( versionstr ) )
            Cvar_SetValue ( "cl_latest_game_version", atof ( versionstr ) );
        free ( versionstr );
    }

}
