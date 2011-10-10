/** Synti-kaksi interface */
#ifndef SYNTI2_INCLUDED
#define SYNTI2_INCLUDED

typedef float synti2_smp_t;

typedef struct synti2_synth synti2_synth;
typedef struct synti2_conts synti2_conts;
typedef struct synti2_player synti2_player;

/* ------- Realtime synth interface ------- */

/** Create a synth instance */
synti2_synth *
synti2_create(unsigned long sr, 
              const unsigned char * patch_sysex);

/** Render some (stereo-interleaved) frames of audio to the output
 *  buffer.
 */
void
synti2_render(synti2_synth *s, 
	      synti2_player *pl, 
	      synti2_smp_t *buffer,
	      int nframes);

/** Immediately transfers data with SysEx. This is a hack for making
 *  default instrument sounds for testing, and this will go away later
 *  (FIXME: make this go away...)
 */
//void
//synti2_do_receiveSysEx(synti2_synth *s, unsigned char * data);


/* ------- Sequencer interface for a pre-composed song -------- */

/** Load and initialize a song. */
synti2_player *
synti2_player_create(unsigned char * songdata, 
		     int samplerate);

/** Render some frames of control data for the synth, keeping track of
 * song position. This will do the store()s as if the song was played
 * live.
 */
void
synti2_player_render(synti2_player *player, 
		    synti2_conts *control,
		    int frames);


/* ------- Realtime control interface ------- */

#ifdef JACK_MIDI
#include <jack/jack.h>
#include <jack/midiport.h>
/** Creates a future for the player object that will repeat the next
 *  nframes of midi data from a jack audio connection kit midi port.
 */
void
synti2_player_init_from_jack_midi(synti2_player *player,
                                  jack_port_t *inmidi_port,
                                  jack_nframes_t nframes);
#endif



/* Defines for indices and stuff. Should be autogenerated from some
 * nicer description!! 
 */
/*TODO: Design the patch format properly. */
#define SYNTI2_IENVS 0
#define SYNTI2_IENVLOOP 40

#define SYNTI2_NPARAMS 41


/* Length of envelope data block (K1T&L K2T&L K3T&L K4T&L K5T&L) */
/* (order of knees might be different??) FIXME: Think about this .. make envs simpler?*/
#define SYNTI2_NENVD 10

#endif
