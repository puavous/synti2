/** Synti-kaksi interface */
#ifndef SYNTI2_INCLUDED
#define SYNTI2_INCLUDED

typedef float synti2_smp_t;

typedef struct synti2_synth synti2_synth;
typedef struct synti2_conts synti2_conts;

/* Realtime synth interface */
/** Create a synth instance */

synti2_synth *
synti2_create(unsigned long sr); /* Sample rate could be variable ??*/

/** Render some (stereo-interleaved) frames of audio to the output
 *  buffer.
 */
void
synti2_render(synti2_synth *s, 
	      synti2_conts *control, 
	      synti2_smp_t *buffer,
	      int nframes);

/* Sequencer interface */
/** Load and initialize a song */
/** Render some frames of control data for the synth */


/* Realtime control interface */
synti2_conts * 
synti2_conts_create();
/** Get ready to store new controls. Must be called after reading all
    messages for a block, and before writing new ones. */
void 
synti2_conts_reset(synti2_conts *control);
/** Must be called once before calling any gets */
void 
synti2_conts_start(synti2_conts *control);

/** Store a raw midi command */
void 
synti2_conts_store(synti2_conts *control,
		   int frame,
		   unsigned char *midibuf,
		   int len);


#endif
