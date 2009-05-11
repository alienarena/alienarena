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
/*
 * snd_wav.c
 *
 * .WAV file support for OpenAL version
 */

#include "client.h"

typedef struct
{
	int rate;
	int width;
	int channels;
	int loopstart;
	int samples;
	int dataofs; // chunk starts this many bytes from file start
} wavinfo_t;

/*
 ==
 .WAV file parsing
 ==
 */

byte *data_p;
byte *iff_end;
byte *last_chunk;
byte *iff_data;
int iff_chunk_len;

short GetLittleShort( void )
{
	short val = 0;
	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	data_p += 2;
	return val;
}

int GetLittleLong( void )
{
	int val = 0;
	val = *data_p;
	val = val + ( *( data_p + 1 ) << 8 );
	val = val + ( *( data_p + 2 ) << 16 );
	val = val + ( *( data_p + 3 ) << 24 );
	data_p += 4;
	return val;
}

void FindNextChunk( char *name )
{
	while( 1 )
	{
		data_p = last_chunk;

		if( data_p >= iff_end )
		{ // didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if( iff_chunk_len < 0 )
		{
			data_p = NULL;
			return;
		}
		//		if (iff_chunk_len > 1024*1024)
		//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ( ( iff_chunk_len + 1 ) & ~1 );
		if( !strncmp( (const char *)data_p, name, 4 ) )
			return;
	}
}

void FindChunk( char *name )
{
	last_chunk = iff_data;
	FindNextChunk( name );
}

/*
 ============
 GetWavinfo
 ============
 */
qboolean GetWavinfo( const char *name, byte *wav, int wavlength,
        wavinfo_t *info )
{
	//wavinfo_t info;
	int i;
	int format;
	int samples;

	memset( info, 0, sizeof( info ) );

	if( !wav )
		return false;

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk( "RIFF" );
	if( !( data_p && !strncmp( (const char *)data_p + 8, "WAVE", 4 ) ) )
	{
		Com_DPrintf( "%s: Missing RIFF/WAVE chunks\n", name );
		return false;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;

	FindChunk( "fmt " );
	if( !data_p )
	{
		Com_DPrintf( "%s: Missing fmt chunk\n", name );
		return false;
	}

	data_p += 8;
	format = GetLittleShort();
	if( format != 1 ) // format 1 is PCM data,
	{
		Com_DPrintf( "%s: audio format %i not supported\n", name, format );
		return false;
	}

	info->channels = GetLittleShort();
	info->rate = GetLittleLong();
	data_p += 4 + 2;

	info->width = GetLittleShort();
	if( info->width != 8 && info->width != 16 )
	{
		Com_DPrintf( "%s: %i bits-per-sample not supported\n", name, info->width );
		return false;
	}
	info->width /= 8;

	// get cue chunk. for files with "intro" + "loop"
	// 	TODO: figure out how to get this to work with OpenAL
	FindChunk( "cue " );
	if( data_p )
	{
		data_p += 32;
		info->loopstart = GetLittleLong();
		Com_DPrintf( "%s: loopstart=%d\n", name, info->loopstart );

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk( "LIST" );
		if( data_p )
		{
			if( !strncmp( (const char *)data_p + 28, "mark", 4 ) )
			{ // this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong(); // samples in loop
				info->samples = info->loopstart + i;
				Com_DPrintf( "%s: looped length: %i\n", name, i );
			}
		}
	}
	else
	{
		info->loopstart = -1; // no loop
	}

	// find data chunk
	FindChunk( "data" );
	if( !data_p )
	{
		Com_DPrintf( "%s: Missing data chunk\n", name );
		return false;
	}
	data_p += 4;

	samples = GetLittleLong() / info->width;
	if( info->loopstart != -1 && samples < info->samples )
	{ // from "cue " chunk
		Com_DPrintf( "%s: Bad loop length\n", name );
		info->loopstart = -1;
	}
	info->samples = samples;

	info->dataofs = data_p - wav;

	return true;
}

/*
 ==
 S_LoadSound()

 creates a buffer containing PCM data, 8- or 16-bit, mono or stereo.
 stereo samples are left-channel first

 OUTPUTS:
 readbfr : the whole file, caller must FS_FileFree() this
 pcmdata : start of PCM data, for transfer to OpenAL buffer
 channels : 1=mono, 2=stereo
 bytewidth : 1=8-bit, 2=16-bit
 samplerate : samples-per-second (aka, frequency)
 byte_count : number of bytes of PCM data
 loop_start : offset in <?> to start of looping section, from "cue " chunk
 ==
 */
qboolean S_LoadSound( const char *filename, // in
        void** filebfr, // out
        void** pcmbfr, // out
        int *bytewidth, // out
        int *channels, // out
        int *samplerate, // out
        size_t *byte_count, // out
        int *loop_offset // out
)
{
	byte *data; // for buffer allocated by FS_LoadFile
	int size;
	wavinfo_t info;
	qboolean success;

	// clear return values
	*filebfr = NULL;
	*pcmbfr = NULL;
	*channels = 0;
	*bytewidth = 0;
	*samplerate = 0;
	*byte_count = 0;
	*loop_offset = -1;

	// load sound file from game directory
	size = FS_LoadFile( (char*)filename, (void **) &data );
	if( !data )
	{
		Com_DPrintf( "%s: Couldn't load\n", filename );
		return false;
	}

	// parse the .wav file
	success = GetWavinfo( filename, data, size, &info );
	if( !success )
	{
		FS_FreeFile( data );
		return false;
	}

	// set return values
	*filebfr = data;
	*pcmbfr = data + info.dataofs;
	*channels = info.channels;
	*bytewidth = info.width;
	*samplerate = info.rate;
	*byte_count = info.samples * info.width;
	*loop_offset = info.loopstart;

	return true;
}

