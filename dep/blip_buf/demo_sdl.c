/* Plays two square waves with mouse control of frequency,
using SDL multimedia library */

#include "blip_buf.h"
#include <stdlib.h>
#include "SDL.h"

enum { sample_rate = 44100 };
static const double clock_rate = (double) sample_rate * blip_max_ratio;

static blip_buffer_t* blip;

typedef struct wave_t
{
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
		blip_add_delta( blip, w->time, delta );
		w->phase = -w->phase;
	}
	w->time -= clocks;
}

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

static void start_audio( void );

int main( int argc, char* argv [] )
{
	SDL_Event e;
	
	waves [0].frequency = 1000;
	waves [0].volume    = 0.2;
	waves [0].phase     = 1;
	
	waves [1].frequency = 1000;
	waves [1].volume    = 0.2;
	waves [1].phase     = 1;
	
	init_sound();
	start_audio();
	
	/* run until mouse or keyboard is pressed */
	while ( !SDL_PollEvent( &e ) || (e.type != SDL_MOUSEBUTTONDOWN &&
				e.type != SDL_KEYDOWN && e.type != SDL_QUIT) )
	{
		/* mouse controls frequency and volume */
		int ix, iy;
		SDL_GetMouseState( &ix, &iy );
		waves [0].frequency = ix / 511.0 * 2000 + 100;
		waves [1].frequency = iy / 511.0 * 2000 + 100;
	}
	
	SDL_PauseAudio( 1 );
	blip_delete( blip );
	
	return 0;
}

static void fill_buffer( void* unused, Uint8* out, int byte_count )
{
	gen_samples( (short*) out, byte_count / sizeof (short) );
}

static void start_audio( void )
{
	static SDL_AudioSpec as = { sample_rate, AUDIO_S16SYS, 1 };
	
	/* initialize SDL */
	if ( SDL_Init( SDL_INIT_AUDIO | SDL_INIT_VIDEO ) < 0 )
		exit( EXIT_FAILURE );
	atexit( SDL_Quit );
	SDL_SetVideoMode( 512, 512, 0, 0 );
	
	/* start audio */
	as.callback = fill_buffer;
	as.samples = 1024;
	if ( SDL_OpenAudio( &as, 0 ) < 0 )
		exit( EXIT_FAILURE );
	SDL_PauseAudio( 0 );
}
