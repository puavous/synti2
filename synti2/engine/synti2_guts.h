#ifndef SYNTI2_INTERNAL_STRUCTURES_H
#define SYNTI2_INTERNAL_STRUCTURES_H

/* Public interface is used also internally */
#include "synti2.h"

/* More or less architecture dependent values: */
#include "synti2_archdep.h"

/* Universally constant values: */
#include "synti2_limits.h"

/* The following will be custom configurable for the 4k target: */
#ifdef COMPOSE_MODE
#include "synti2_cap_full.h"
#else
#include "synti2_cap_custom.h"
#endif

#ifdef USE_MIDI_INPUT
#include "synti2_midi.h"
#include "synti2_midi_guts.h"
#endif

#ifdef FEAT_EXTRA_WAVETABLES
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
 * TODO: (later, maybe) think if it is more efficient to put values
 * and deltas in separate arrays. Probably, if you think of a possible
 * assembler implementation with multimedia vector instructions(?). Or
 * maybe not? Look at the compiler output...
 *
 * TODO: (don't, actually) Signed integers would overflow just as
 * nicely in some implementation, but the standard says signed
 * overflow is undefined. So let's not think about this further.
 *
 * TODO: (later, maybe) Maybe separate functions for oscillators and
 * envelopes could still be used if they would compress nicely...
 *
 * TODO: (to try in later projects altogether) Now I convert the
 * counter value to a float before computations even though fixed
 * point arithmetics could admittedly yield shorter code (maybe...)
 * Fixed point synth remains to be tried in a later project, because
 * now I didn't have the time to think about accuracies and
 * stuff. Floats are so easy. In the end, I suppose everything should
 * use floats, including oscillators. As in 'updated :=
 * decimal_part_of(old+delta)'.
 */
typedef struct {
  unsigned int val;
  unsigned int detect;
  unsigned int delta;
  float f;   /* current output value (interpolant) */
  float fr;  /* current output value (0..1) */
  float aa;  /* for interpolation start */
  float bb;  /* for interpolation end */
} counter;

/* Events shall form a singly linked list. TODO: (or not) Is the list
 * code too complicated? Use just tables with O(n^2) pre-ordering
 * instead??  Hmm.. after some futile attempts, I was unable to
 * squeeze a smaller code from any other approach, so I'm leaving the
 * list as it is now.
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

#ifdef DO_SAFETY_CHECKS
  unsigned int last_error_frame; /* Errors can be monitored by tools. */
  unsigned int last_error_type;  /* Error position. */
  unsigned int last_error_info;  /* Some key information about the error. */
#endif
};


/** The synthesizer patch. The way things sound. */
typedef struct synti2_patch {
  float fpar[NUM_FPARS+NUM_IPARS]; /*replicated for simpler reader code */
  int ipar3[NUM_IPARS+NUM_FPARS];
} synti2_patch;

/** Call it voice. (TODO: rename everywhere) It could be channel,
 *  part, source, ... but none of these is an accurate word for what
 *  it stands for. So just call it voice.
 */
typedef struct synti2_voice {
  /* Counters in one array (so that they can be iterated in one loop):
   * Operator phases, envelope timers, modulators, pitch.
   */
#define CI_OPERS     0
#define CI_ENVS      (CI_OPERS + NUM_OPERATORS)
#define CI_MODS      (CI_ENVS  + (1 + NUM_ENVS))
#define CI_PITCH     (CI_MODS  + NUM_MODULATORS)
#define NUM_COUNTERS (CI_PITCH + 1)
  counter c[NUM_COUNTERS];
#ifdef FEAT_FILTER_FOLLOW_PITCH
  float effnote[NUM_OPERATORS];
#endif

  /* Envelope stages just a table? TODO: think.*/
  unsigned int estage[NUM_ENVS+1];
#ifdef FEAT_LOOPING_ENVELOPES
  unsigned int sustain;
#endif

  unsigned int note;

#ifdef FEAT_VELOCITY_SENSITIVITY
  unsigned int velocity;
#endif

  /* Computation area for "zero", operator outputs, delay bus: */
  float outp[1+NUM_MAX_OPERATORS+1];

#ifdef FEAT_FILTER
/* Filter playground in + (lp,bp,hp,notch) */
#define FILTER_PLAYGROUND_SIZE (1+4)
  float filtp[FILTER_PLAYGROUND_SIZE];
#endif

  synti2_patch patch;   /* The sound parameters per part*/
} synti2_voice;

struct synti2_synth {
  /* I'll actually put the player inside the synthesizer. Somewhat
   * less 'modular', but seems to be a bit smaller size.
   */
  unsigned int sr; /* Better for code size to have indiv. attrib 1st?*/
  /* The simple random number generator was posted on musicdsp.org by
   * Dominik Ries. Thanks a lot.
   */
  int RandSeed;

  synti2_player seq;

  /* TODO: This space could be used for LFO's. But I suppose memory
   * layout is implementation-dependent!
   */
  float infranotes[128]; 
  float note2delta[128];  /* pre-computed frequencies of notes... Tuning
			    systems would be easy to change - just
			    compute a different table here (?)..*/
  float ultranotes[128]; /* TODO: This space could be used for noises? */
  float note2freq[256];  /* For filter.. */

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
