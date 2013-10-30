#ifndef SYNTI2_INTERNAL_STRUCTURES_H
#define SYNTI2_INTERNAL_STRUCTURES_H

/* Public interface is used also internally */
#include "synti2.h"

/* More or less architecture dependent values: */
#include "synti2_archdep.h"

/* The following will be custom configurable for the 4k target: */
#ifdef COMPOSE_MODE
#include "synti2_cap_full.h"
/* separate parameters? Not really necessary? */
#endif

#ifdef ULTRASMALL
#include "synti2_cap.h"
#include "synti2_params.h"
/* FIXME: TBD: 
#include "synti2_cap_custom.h"
#include "synti2_par_custom.h"
 */
#endif

#ifdef USE_MIDI_INPUT
#include "synti2_midi.h"
#include "synti2_midi_guts.h"

#endif

#ifndef NO_EXTRA_WAVETABLES
#define NHARM 8
#else
#define NHARM 1
#endif



/** Oscillator/counter structure. I don't know if they usually do this
 * kind of stuff with synths... (I know I should read other peoples
 * code, I know, I know, I will, I will... later... ) but I did it
 * this way... 
 * 
 * The idea came about when I realized that unsigned ints will nicely
 * loop around (overflow) through zero and as such they model an
 * oscillator's phase pretty nicely. Could I use them for other stuff
 * as well? They seem fit for envelopes with only a little extra logic
 * (clamping and a stop flag). Could I use just one huge counter bank
 * for everything? It pretty much looks that way. There seems to be a
 * 30% speed hit for computing all the redundant stuff... but as a
 * result, the executable is a little bit smaller, and the source
 * looks pretty nice too.
 * 
 * TODO: think if it is more efficient to put values and deltas in
 * separate arrays. Probably, if you think of a possible assembler
 * implementation with multimedia vector instructions(?). Or maybe
 * not? Look at the compiler output...
 *
 * TODO: Think. Signed integers would overflow just as nicely (oops.. 
 * check the standard again.. I hear it could be undefined behavior). 
 * Could it be useful to model things in -1..1 range instead of 0..1 ?
 *
 * TODO: Maybe separate functions for oscillators and envelopes could
 * still be used if they would compress nicely...
 *
 * TODO: Now I convert the counter value to a float before
 * computations even though fixed point arithmetics could admittedly
 * yield shorter code (maybe...) Fixed point synth remains to be tried
 * in a later project, because now I didn't have the time to think
 * about accuracies and stuff. Floats are so easy.
 */
typedef struct {
  unsigned int val;
  unsigned int detect;
  unsigned int delta;
  float f;   /* current output value (interpolant) */
  float fr;  /* current output value (0..1) */ /* (FIXME: only f needed?) */
  float aa;  /* for interpolation start */
  float bb;  /* for interpolation end */
} counter;

/* Events shall form a singly linked list. TODO: Is the list code too
 * complicated? Use just tables with O(n^2) pre-ordering instead??
 * Hmm.. after some futile attempts, I was unable to squeeze a smaller
 * code from any other approach, so I'm leaving the list as it is now.
 */
typedef struct synti2_player_ev synti2_player_ev;

#define SYNTI2_MAX_EVDATA (1+3+2*sizeof(float))

/* Events are small (I have now excluded bulk sysexes for patch data).*/
struct synti2_player_ev {
  synti2_player_ev *next;   /* link to next event */
  unsigned int frame;       /* Time of the event in frames */
  byte_t data[SYNTI2_MAX_EVDATA];       /* Event data */
};

/** Player is the MIDI-like sequence playback engine. */
struct synti2_player {
  synti2_player_ev *playloc; /* could we use just one loc for play and ins? */
  synti2_player_ev *insloc;  /* (one loc yielded larger exe on my first try)*/
  unsigned int fpt; /* Frames per tick. integer => tempos inexact, sry! */
                    /* Hmm: Could I use some "automagic" tick counter? */
  unsigned int tpq; /* Ticks per quarter (no support for SMPTE). */
  synti2_player_ev *freeloc; /*pointer to next free event structure*/
  int frames_done;  /* Runs continuously. Breaks after 12 hrs @ 48000fps !*/

  /* Playable events. The first will be at evpool[0]! */
  synti2_player_ev evpool[SYNTI2_MAX_SONGEVENTS];

#ifndef NO_SAFETY
  unsigned int last_error_frame; /* Errors can be monitored by tools. */
  unsigned int last_error_type;  /* Error position. */
  unsigned int last_error_info;  /* Some key information about the error. */
#endif
};


/** The synthesizer patch. The way things sound. */
typedef struct synti2_patch {
  float fpar[NUM_FPARS];
  int ipar3[NUM_IPARS];
} synti2_patch;

/** Call it voice. (TODO: rename everywhere) It could be channel,
 *  part, source, ... but none of these is an accurate word for what
 *  it stands for. So just call it voice.
 */
typedef struct synti2_voice {
  /* Must be in this order and next to each other exactly!! Impl. specif?*/
  counter c[NUM_OPERATORS];
  counter eprog[NUM_ENVS+1];
  counter contr[NUM_MODULATORS];
#ifndef NO_LEGATO
  counter pitch;
#endif
#ifndef NO_FILTER_PITCH_FOLLOW
  float effnote[NUM_OPERATORS];
#endif

  /* Envelope stages just a table? TODO: think.*/
  unsigned int estage[NUM_ENVS+1];
#ifndef NO_LOOPING_ENVELOPES
  unsigned int sustain;
#endif

  unsigned int note;

#ifndef NO_VELOCITY
  unsigned int velocity;
#endif

  float outp[1+NUM_MAX_OPERATORS+1]; /*"zero", operator outputs, delay
                                        bus FIXME: could be a struct?
                                        should?*/
#ifdef FEAT_FILTER
  float filtp[1+4];     /* Filter playground in + (lp,bp,hp,notch) */
#endif

  synti2_patch patch;   /* The sound parameters per part*/
} synti2_voice;

struct synti2_synth {
  /* I'll actually put the player inside the synthesizer. Somewhat
   * less 'modular', but seems to be a bit smaller size.
   */
  unsigned int sr; /* Better for code size to have indiv. attrib 1st?*/
  synti2_player seq;

  float infranotes[128]; /* TODO: This space could be used for LFO's */
  float note2delta[128];  /* pre-computed frequencies of notes... Tuning
			    systems would be easy to change - just
			    compute a different table here (?)..*/
  float ultranotes[128]; /* TODO: This space could be used for noises? */
  float note2freq[256]; /* For filter.. */

  float wave[NHARM][WAVETABLE_SIZE];
  /*float noise[WAVETABLE_SIZE]; Maybe?? */

  /* Oscillators are now modeled as integer counters (phase). */
  /* Envelope progression also modeled by integer counters. Not much
   * difference between oscillators and envelopes!!
   */
  
  synti2_voice voi[NUM_CHANNELS];
  unsigned int framecount;

  float delay[NUM_DELAY_LINES][SYNTI2_DELAYSAMPLES]; /* Use of delays is optional,
                                         but the space costs nothing..*/
#ifdef USE_MIDI_INPUT
  synti2_midi_map midimap;
  synti2_midi_state midistate;
#endif

#ifndef ULTRASMALL
  /* Errors could be monitored by tools. Ultrasmall exe assumes correct data.*/
  unsigned int last_error_frame; /* Error position. */
  unsigned int last_error_type;  /* Error type */
  unsigned int last_error_info;  /* Some key information about the error. */
#endif
};


/* A MIDI interface needs to have this exposed. Otherwise can be static. */
#ifndef USE_MIDI_INPUT
static
#endif
void
synti2_player_event_add(synti2_synth *s, 
                        unsigned int frame, 
                        const byte_t *src);


#endif
