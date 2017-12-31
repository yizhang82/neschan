/* Simple wave sound file writer for use in demo programs. */

#ifndef WAVE_WRITER_H
#define WAVE_WRITER_H

#ifdef __cplusplus
	extern "C" {
#endif

/* If error occurs (out of memory, disk full, etc.), functions print cause
then exit program. */

/* Creates and opens sound file of given sample rate and filename. */
void wave_open( int sample_rate, const char filename [] );

/* Enables stereo output. */
void wave_enable_stereo( void );

/* Appends count samples to file. */
void wave_write( const short in [], int count );

/* Number of samples written so far. */
int wave_sample_count( void );

/* Finishes writing sound file and closes it. */
void wave_close( void );

#ifdef __cplusplus
	}
#endif

#ifdef __cplusplus

/* C++ interface */
class Wave_Writer {
public:
	typedef short sample_t;
	Wave_Writer( int rate, const char file [] = "out.wav" ) { wave_open( rate, file ); }
	void enable_stereo()                                    { wave_enable_stereo(); }
	void write( const sample_t in [], int n )               { wave_write( in, n ); }
	int sample_count() const                                { return wave_sample_count(); }
	void close()                                            { wave_close(); }
	~Wave_Writer()                                          { wave_close(); }
};
#endif

#endif
