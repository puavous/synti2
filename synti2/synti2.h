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
synti2_create(unsigned long sr); /* Sample rate could be variable ??*/

/** Render some (stereo-interleaved) frames of audio to the output
 *  buffer.
 */
void
synti2_render(synti2_synth *s, 
	      synti2_conts *control, 
	      synti2_smp_t *buffer,
	      int nframes);



/* ------- Sequencer interface for a pre-composed song -------- */

/** Load and initialize a song. */
synti2_player *
synti2_player_create(unsigned char * songdata, int samplerate);

/** Render some frames of control data for the synth, keeping track of
 * song position. This will do the store()s as if the song was played
 * live.
 */
void
synti2_player_render(synti2_player *player, 
		    synti2_conts *control,
		    int frames);



/* ------- Realtime control interface ------- */

/** Create a MIDI control mediator */
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



/* Defines for indices and stuff. Should be autogenerated from some
 * nicer description!! 
 */
/*TODO: Design the patch format properly. */
#define SYNTI2_IENVS 0
#define SYNTI2_NPARAMS 40

/* Length of envelope data block (K1T&L K2T&L K3T&L K4T&L K5T&L) */
/* (order of knees might be different??) FIXME: Think about this .. make envs simpler?*/
#define SYNTI2_NENVD 10

#endif
