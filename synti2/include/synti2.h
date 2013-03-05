/** Synti-kaksi interface */
#ifndef SYNTI2_INCLUDED
#define SYNTI2_INCLUDED

/* Some conditional compile logic to make things (maybe) easier. */
#ifdef JACK_MIDI
#define USE_MIDI_INPUT
#endif

/* If they don't want no nothin' then so be it. */
#ifdef NO_NOTHING
#define NO_NOTEOFF
#define NO_NOISE
#define NO_LOOPING_ENVELOPES
#define NO_SYSEX_RECEIVE
#define NO_LEGATO
#define NO_PITCH_SCALING
#define NO_PITCH_ENV
#define NO_DETUNE
#define NO_EXTRA_WAVETABLES
#define NO_OUTPUT_SQUASH
#define NO_DELAY
#define NO_FILTER
#define NO_VELOCITY
#define NO_CC
#define NO_STEREO
#endif

#if defined (NO_FILTER) || defined (NO_FILTER_FRILLS)
#ifndef NO_FILTER_PITCH_FOLLOW
#define NO_FILTER_PITCH_FOLLOW
#endif
#ifndef NO_FILTER_CUT_ENVELOPE
#define NO_FILTER_CUT_ENVELOPE
#endif
#ifndef NO_FILTER_RESO_ENVELOPE
#define NO_FILTER_RESO_ENVELOPE
#endif
#endif

/* don't know if this is nice, but doing it still: */
#ifdef BUT_USE_DETUNE
#undef NO_DETUNE
#endif

#ifdef BUT_USE_NOISE
#undef NO_NOISE
#endif

#ifdef BUT_USE_PITCH_ENV
#undef NO_PITCH_ENV
#endif

#ifdef BUT_USE_OUTPUT_SQUASH
#undef NO_OUTPUT_SQUASH
#endif


#define SYNTI2_ERROR_UNKNOWN_LAYER        1
#define SYNTI2_ERROR_UNKNOWN_MESSAGE      2
#define SYNTI2_ERROR_UNKNOWN_OPCODE       3
#define SYNTI2_ERROR_OUT_OF_EVENT_SPACE   4
#define SYNTI2_ERROR_OUT_OF_MESSAGE_SPACE 5
#define SYNTI2_ERROR_INVALID_LOOPING_ENV  6


typedef float synti2_smp_t;

typedef struct synti2_synth synti2_synth;
typedef struct synti2_player synti2_player;

/* ------- Realtime / pre-sequenced synth interface ------- */

/** Create a synth instance */
void
synti2_init(synti2_synth * s,
            unsigned long sr, 
            const unsigned char * patch_sysex, 
            const unsigned char * songdata);

/** Render some (stereo-interleaved) frames of audio to the output
 *  buffer.
 */
void
synti2_render(synti2_synth *s, 
	      synti2_smp_t *buffer,
	      int nframes);

#ifdef EXTREME_NO_SEQUENCER
/** If importing of MIDI sequences is not your itch, then why not set
 *  -DEXTREME_NO_SEQUENCER and jam with generated calls to noteon().
 */
void
synti2_do_noteon(synti2_synth *s, unsigned int voice, 
                 unsigned int note, unsigned int vel);
#endif


/* ------- Realtime control interface ------- */

/* So far only jack MIDI is supported. It would be easy to support
 * others.
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


#endif
