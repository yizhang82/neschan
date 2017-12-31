/* Implements a simple square wave generator as might be used in an analog
waveform synthesizer. Runs two square waves together. */

#include "blip_buf.h"
#include "wave_writer.h"
#include <stdlib.h>

enum { sample_rate = 44100 };

/* Use the maximum supported clock rate, which is about one million times
the sample rate, 46 GHz in this case. This gives all the accuracy needed,
even for extremely fine frequency control. */
static const double clock_rate = (double) sample_rate * blip_max_ratio;

static blip_buffer_t* blip;

typedef struct wave_t
{
	double frequency;   /* cycles per second */
	double volume;      /* 0.0 to 1.0 */
	int phase;          /* +1 or -1 */
	int time;           /* clock time of next delta */
	int amp;            /* current amplitude in delta buffer */
} wave_t;

static wave_t waves [2];

static void run_wave( wave_t* w, int clocks )
{
	/* Clocks for each half of square wave cycle */
	int period = (int) (clock_rate / w->frequency / 2 + 0.5);
	
	/* Convert volume to 16-bit sample range (divided by 2 because it's bipolar) */
	int volume = (int) (w->volume * 65536 / 2 + 0.5);
	
	/* Add deltas that fall before end time */
	for ( ; w->time < clocks; w->time += period )
	{
		int delta = w->phase * volume - w->amp;
		w->amp += delta;
		blip_add_delta( blip, w->time, delta );
		w->phase = -w->phase;
	}
	
	w->time -= clocks; /* adjust for next time frame */
}

/* Generates enough samples to exactly fill out */
static void gen_samples( short out [], int samples )
{
	int clocks = blip_clocks_needed( blip, samples );
	run_wave( &waves [0], clocks );
	run_wave( &waves [1], clocks );
	blip_end_frame( blip, clocks );
	
	blip_read_samples( blip, out, samples, 0 );
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
	
	waves [0].phase     = 1;
	waves [0].volume    = 0.0;
	waves [0].frequency = 16000;
	
	waves [1].phase     = 1;
	waves [1].volume    = 0.5;
	waves [1].frequency = 1000;
	
	wave_open( sample_rate, "out.wav" );
	while ( wave_sample_count() < 2 * sample_rate )
	{
		enum { samples = 1024 };
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
	
	blip_delete( blip );
	
	return 0;
}
