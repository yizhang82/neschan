/* Implements a simple square wave generator as would be in a sound
chip emulator */

#include "blip_buf.h"
#include "wave_writer.h"
#include <stdlib.h>

static const int   sample_rate =   44100;    /* 44.1 kHz sample rate*/
static const double clock_rate = 3579545.45; /* 3.58 MHz clock rate */

static blip_buffer_t* blip;

/* Square wave state */
static int time;        /* clock time of next delta */
static int period = 1;  /* clocks between deltas */
static int phase  = +1; /* +1 or -1 */
static int volume;
static int amp;         /* current amplitude in delta buffer */

static void run_wave( int clocks )
{
	/* Add deltas that fall before end time */
	for ( ; time < clocks; time += period )
	{
		int delta = phase * volume - amp;
		amp += delta;
		blip_add_delta( blip, time, delta );
		phase = -phase;
	}
}

static void flush_samples( void )
{
	/* If we only wanted 512-sample chunks, never smaller, we would
	do >= 512 instead of > 0. Any remaining samples would be left
	in buffer for next time. */
	while ( blip_samples_avail( blip ) > 0 )
	{
		enum { temp_size = 512 };
		short temp [temp_size];
		
		/* count is number of samples actually read (in case there
		were fewer than temp_size samples actually available) */
		int count = blip_read_samples( blip, temp, temp_size, 0 );
		wave_write( temp, count );
	}
}

static void init_sound( void )
{
	blip = blip_new( sample_rate / 10 );
	if ( blip == NULL )
		exit( EXIT_FAILURE ); /* out of memory */
	
	blip_set_rates( blip, clock_rate, sample_rate );
}

int main( void )
{
	init_sound();
	
	/* Record some output to wave file */
	wave_open( sample_rate, "out.wav" );
	while ( wave_sample_count() < 2 * sample_rate )
	{
		/* Generate 1/60 second each time through loop */
		int clocks = (int) (clock_rate / 60);
		
		/* We could instead run however many clocks are needed to get a fixed number
		of samples per frame:
		int samples_needed = sample_rate / 60;
		clocks = blip_clocks_needed( blip, samples_needed );
		*/
		
		run_wave( clocks );
		blip_end_frame( blip, clocks );
		time -= clocks; /* adjust for new time frame */
		
		flush_samples();
		
		/* Slowly increase volume and lower pitch */
		volume += 100;
		period += period / 28 + 3;
	}
	wave_close();
	
	blip_delete( blip );
	
	return 0;
}
