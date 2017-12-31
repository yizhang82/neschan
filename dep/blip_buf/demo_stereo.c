/* Generates left and right channels and interleaves them together */

#include "blip_buf.h"
#include "wave_writer.h"
#include <stdlib.h>

enum { sample_rate = 44100 };
static const double clock_rate = (double) sample_rate * blip_max_ratio;

/* Delta buffers for left and right channels */
static blip_buffer_t* blips [2];

typedef struct wave_t
{
	blip_buffer_t* blip; /* delta buffer to output to */
	double frequency;
	double volume;
	int phase;
	int time;
	int amp;
} wave_t;

static wave_t waves [2];

static void run_wave( wave_t* w, int clocks )
{
	int period = (int) (clock_rate / w->frequency / 2 + 0.5);
	int volume = (int) (w->volume * 65536 / 2 + 0.5);
	for ( ; w->time < clocks; w->time += period )
	{
		int delta = w->phase * volume - w->amp;
		w->amp += delta;
		blip_add_delta( w->blip, w->time, delta );
		w->phase = -w->phase;
	}
	w->time -= clocks;
}

static void gen_samples( short out [], int samples )
{
	int pairs = samples / 2; /* number of stereo sample pairs */
	int clocks = blip_clocks_needed( blips [0], pairs );
	int i;
	
	run_wave( &waves [0], clocks );
	run_wave( &waves [1], clocks );
	
	/* Generate left and right channels, interleaved into out */
	for ( i = 0; i < 2; ++i )
	{
		blip_end_frame( blips [i], clocks );
		blip_read_samples( blips [i], out + i, pairs, 1 );
	}
}

static void init_sound( void )
{
	/* Create left and right delta buffers */
	int i;
	for ( i = 0; i < 2; ++i )
	{
		blips [i] = blip_new( sample_rate / 10 );
		if ( blips [i] == NULL )
			exit( EXIT_FAILURE ); /* out of memory */
		
		blip_set_rates( blips [i], clock_rate, sample_rate );
	}
}

int main( void )
{
	init_sound();
	
	waves [0].blip      = blips [0];
	waves [0].phase     = 1;
	waves [0].volume    = 0.0;
	waves [0].frequency = 16000;
	
	waves [1].blip      = blips [1];
	waves [1].phase     = 1;
	waves [1].volume    = 0.5;
	waves [1].frequency = 1000;
	
	wave_open( sample_rate, "out.wav" );
	wave_enable_stereo();
	while ( wave_sample_count() < 2 * sample_rate * 2 )
	{
		enum { samples = 2048 };
		short temp [samples];
		gen_samples( temp, samples );
		wave_write( temp, samples );

		/* Slowly increase volume and lower pitch */
		waves [0].volume    += 0.005;
		waves [0].frequency *= 0.950;
		
		/* Slowly decrease volume and raise pitch */
		waves [1].volume    -= 0.002;
		waves [1].frequency *= 1.010;
	}
	wave_close();
	
	blip_delete( blips [0] );
	blip_delete( blips [1] );
	
	return 0;
}
