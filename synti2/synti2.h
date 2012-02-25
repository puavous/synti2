/** Synti-kaksi interface */
#ifndef SYNTI2_INCLUDED
#define SYNTI2_INCLUDED

#include "synti2_params.h"

typedef float synti2_smp_t;

typedef unsigned char byte_t; 

typedef struct synti2_synth synti2_synth;
typedef struct synti2_player synti2_player;

/* ------- Realtime / pre-sequenced synth interface ------- */

/** Create a synth instance */
synti2_synth *
synti2_create(unsigned long sr, 
              const unsigned char * patch_sysex, 
              const unsigned char * songdata);

/** Render some (stereo-interleaved) frames of audio to the output
 *  buffer.
 */
void
synti2_render(synti2_synth *s, 
	      synti2_smp_t *buffer,
	      int nframes);


/* ------- Realtime control interface ------- */

/* So far only jack MIDI is supported. It would be easy to support
 * others (at least as of yet, when the synth listens to plain
 * MIDI:)).
 */

#ifdef JACK_MIDI
#include <jack/jack.h>
#include <jack/midiport.h>


/** Creates a future for the player object that will repeat the next
 *  nframes of midi data from a jack audio connection kit midi port.
 */
void
synti2_read_jack_midi(synti2_synth *s,
                      jack_port_t *inmidi_port,
                      jack_nframes_t nframes);
#endif



/* Sound structure. Must be consistent with all the parameters! */
#define NENVPERVOICE 6
#define NOSCILLATORS 4

/* TODO: Think about the whole envelope madness... use LFOs instead of
 * looping envelopes?
 */
#define TRIGGERSTAGE 6

/* Remain in the MIDI world - it is 7 bits per SysEx data bit. */

/* Length of envelope data block (K1T&L K2T&L K3T&L K4T&L K5T&L) */
#define SYNTI2_NENVD 10
/* (order of knees might be better reversed ??) 
  FIXME: Think about this .. make envs simpler?
*/


#endif
