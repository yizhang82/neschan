/* http://www.slack.net/~ant/ */

#include "wave_writer.h"

#include <stdlib.h>
#include <stdio.h>

/* Copyright (C) 2003-2009 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

enum { sample_size = 2 };

static FILE* file;
static int   sample_count;
static int   sample_rate;
static int   chan_count;

static void write_header( void );

static void fatal_error( char const str [] )
{
	fprintf( stderr, "Error: %s\n", str );
	exit( EXIT_FAILURE );
}

void wave_open( int new_sample_rate, char const filename [] )
{
	wave_close();
	
	file = fopen( filename, "wb" );
	if ( !file )
		fatal_error( "Couldn't open WAVE file for writing" );
	
	sample_rate = new_sample_rate;
	write_header();
}

void wave_close( void )
{
	if ( file )
	{
		if ( fflush( file ) )
			fatal_error( "Couldn't write WAVE data" );
		
		rewind( file );
		write_header();
		
		if ( fclose( file ) )
			fatal_error( "Couldn't write WAVE data" );
		file = NULL;
	}
	
	sample_count = 0;
	chan_count   = 1;
}

static void write_data( void const* in, unsigned size )
{
	if ( !fwrite( in, size, 1, file ) )
		fatal_error( "Couldn't write WAVE data" );
}

static void set_le32( unsigned char p [4], unsigned n )
{
	p [0] = (unsigned char) (n      );
	p [1] = (unsigned char) (n >>  8);
	p [2] = (unsigned char) (n >> 16);
	p [3] = (unsigned char) (n >> 24);
}

static void write_header( void )
{
	int data_size  = sample_size * sample_count;
	int frame_size = sample_size * chan_count;
	unsigned char h [0x2C] =
	{
		'R','I','F','F',
		0,0,0,0,        /* length of rest of file */
		'W','A','V','E',
		'f','m','t',' ',
		16,0,0,0,       /* size of fmt chunk */
		1,0,            /* uncompressed format */
		0,0,            /* channel count */
		0,0,0,0,        /* sample rate */
		0,0,0,0,        /* bytes per second */
		0,0,            /* bytes per sample frame */
		sample_size*8,0,/* bits per sample */
		'd','a','t','a',
		0,0,0,0         /* size of sample data */
		/* ... */       /* sample data */
	};
	
	set_le32( h + 0x04, sizeof h - 8 + data_size );
	h [0x16] = chan_count;
	set_le32( h + 0x18, sample_rate );
	set_le32( h + 0x1C, sample_rate * frame_size );
	h [0x20] = frame_size;
	set_le32( h + 0x28, data_size );
	
	write_data( h, sizeof h );
}

void wave_enable_stereo( void )
{
	chan_count = 2;
}

void wave_write( short const in [], int remain )
{
	sample_count += remain;
	
	while ( remain )
	{
		unsigned char buf [4096];
		
		int n = sizeof buf / sample_size;
		if ( n > remain )
			n = remain;
		remain -= n;
		
		/* Convert to little-endian */
		{
			unsigned char* out = buf;
			short const* end = in + n;
			do
			{
				unsigned s = *in++;
				out [0] = (unsigned char) (s     );
				out [1] = (unsigned char) (s >> 8);
				out += sample_size;
			}
			while ( in != end );
			
			write_data( buf, out - buf );
		}
	}
}

int wave_sample_count( void )
{
	return sample_count;
}
