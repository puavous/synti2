#ifndef SYNTI2_INTERNAL_STRUCTURES_H
#define SYNTI2_INTERNAL_STRUCTURES_H

#include "synti2.h"
#include "synti2_params.h"

/* Multitimbrality. This equals polyphonic capacity, too. This
 * decision makes the code simpler and resultingly smaller for the 4k
 * intro use case, because no code needs to be written for dynamically
 * allocating a "voice". If there is need for more polyphony, it would
 * be easy enough to create several synth instances running in
 * parallel, each giving a maximum of 16 new "voices" to the whole.
 *
 * (And I would still like to call these "channels" rather than
 * "parts", but let's still do it in the Roland way as of now)
 */
#define NPARTS 16

/* Sound bank size equals the number of parts. Also this decision
 * stems from the 4k intro needs. But this is no restriction for more
 * general use either, because, in principle, you could have thousands
 * of patches off-line, and then load one of them for each of the 16
 * channels on-demand. But, for 4k purposes, we expect to need only
 * max 16 patches and 16 channels, and everything will be "hard-coded"
 * (automatically, though, nowadays) at compile time. Adjacent copies
 * of exactly the same patch data should compress nicely, too.
 *
 * FIXME: For 4k, a sound bank should contain no unused patches (even
 * with zero parameters). Every saved byte matters :).
 *
 * FIXME: The sysex_receive() could be hardwired to receive all the
 * patches at once (when compiled as stand-alone), starting from 0,
 * saving the bytes for the offset handling.
 *
 * Remain in the MIDI world - it is 7 bits per SysEx data bit. Or
 * should we move to our own world altogether? No... SysEx is nice :).
 */

/* Sound structure. Must be consistent with all the parameters! */
#define NENVPERVOICE 6
#define NOSCILLATORS 4
#define NCONTROLLERS 4

/* TODO: Think about the whole envelope madness... use LFOs instead of
 * looping envelopes?
 */
#define TRIGGERSTAGE 6
#define RELEASESTAGE 2


/* Length of envelope data block (K1T&L K2T&L K3T&L K4T&L K5T&L) */
#define SYNTI2_NENVD 10
/* (order of knees might be better reversed ??) 
  FIXME: Think about this .. make envs simpler?
*/

/* Total number of "counters", i.e., oscillators/operators. */
#define NCOUNTERS (NPARTS * NOSCILLATORS)

/* Total number of envelopes. There is the magical "zero-envelope"
 * which is technically not operational. Hmm.. FIXME: Is the
 * zero-envelope needed anymore, when we are actually using separate
 * operator gains? It seems to be necessary only for pitch
 * envelopes. Would it make a shorter code if we used a "pitch gain"
 * and not use a zero-envelope?
 */
#define NENVS (NPARTS * (NENVPERVOICE+1))

/* Maximum value of the counter type depends on C implementation, so
 * use limits.h -- TODO: Actually should probably use C99 and stdint.h !
 */
#define MAX_COUNTER UINT_MAX

/* The wavetable sizes and divisor-bitshift must be consistent. */
#define WAVETABLE_SIZE 0x10000
#define WAVETABLE_BITMASK 0xffff
/* FIXME: This is implementation dependent! Hmm... is there some way
   to get the implementation-dependent bit-count here? */
#define COUNTER_TO_TABLE_SHIFT 16

/* Storage size. TODO: Would be nice to check bounds unless
 * -DULTRASMALL is set.
 */
#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000


/* Audio delay storage. Less delays means less computation, faster synth.*/
#define NDELAYS 4
#define DELAYSAMPLES 0x10000


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
 * TODO: Think. Signed integers would overflow just as nicely. Could
 * it be useful to model things in -1..1 range instead of 0..1 ?
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
  float fr;  /* current output value (0..1) */ /* (only f needed?) */
  float aa;  /* for interpolation start */
  float bb;  /* for interpolation end */
} counter;

/* Events shall form a singly linked list. TODO: Is the list code too
 * complicated? Use just tables with O(n^2) pre-ordering instead??
 * Hmm.. after some futile attempts, I was unable to squeeze a smaller
 * code from any other approach, so I'm leaving the list as it is now.
 */
typedef struct synti2_player_ev synti2_player_ev;

struct synti2_player_ev {
  const byte_t *data;       /* Event data */
  synti2_player_ev *next;   /* link to next event */
  unsigned int frame;       /* Time of the event in frames */
  int len;                  /* length of the event data (hard-codable?) */
  /* FIXME: Think something like {*next, frame, databytes[MAX_EVENT_DATA_SIZE]}? */
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
  int sr;           /* Sample rate. */

  int idata;        /* index of next free data location */

  /* Playable events. The first will be at evpool[0]! */
  synti2_player_ev evpool[SYNTI2_MAX_SONGEVENTS];
  /* Data for events. */
  byte_t data[SYNTI2_MAX_SONGBYTES];

#ifndef ULTRASMALL
  unsigned int last_error_frame; /* Errors can be monitored by tools. */
  unsigned int last_error_type;  /* Error position. */
  unsigned int last_error_info;  /* Some key information about the error. */
#endif
};


/** The synthesizer patch. The way things sound. */
typedef struct synti2_patch {
  float fenvpar[10]; /* FIXME: This zero-env hack OK? */
  float fpar[SYNTI2_F_NPARS];
  int ipar3[SYNTI2_I3_NPARS];
} synti2_patch;

struct synti2_synth {
  /* I'll actually put the player inside the synthesizer. Should
   * probably call it "sequencer" instead of "player"... ? Seems to
   * yield smallest compressed code (with current function
   * implementations) when the player is dynamically allocated and the
   * pointer is stored as the very first field of the synth structure.
   * FIXME: I already let go of all dynamic allocations for the sake
   * 4k, so now maybe should replace s->pl with &s->_actual_player all
   * around the code. Or just move the few attributes of the player to
   * be attributes of the synth itself.. Yes, the latter would be the
   * most lean (and mean) option for a 4k synth, I guess.
   */
  synti2_player *pl;
  unsigned long sr; /* Better for code size to have indiv. attrib 1st?*/
  synti2_player _actual_player;

  float infranotes[128]; /* TODO: This space could be used for LFO's */
  float note2freq[128];  /* pre-computed frequencies of notes... Tuning
			    systems would be easy to change - just
			    compute a different table here (?)..*/
  float ultranotes[128]; /* TODO: This space could be used for noises? */

#ifndef NO_EXTRA_WAVETABLES
#define NHARM 8
#else
#define NHARM 1
#endif

  float rise[WAVETABLE_SIZE];
  float wave[NHARM][WAVETABLE_SIZE];
  /*float noise[WAVETABLE_SIZE]; Maybe?? */

  /* Oscillators are now modeled as integer counters (phase). */
  /* Envelope progression also modeled by integer counters. Not much
   * difference between oscillators and envelopes!!
   */
  /* Must be in this order and next to each other exactly!! Impl. specif?*/
  counter c[NCOUNTERS];
  counter eprog[NPARTS][NENVPERVOICE+1];
  counter contr[NPARTS][NCONTROLLERS];
  counter framecount;

  /* Envelope stages just a table? TODO: think.*/
  unsigned int estage[NPARTS][NENVPERVOICE+1];
  unsigned int sustain[NPARTS];

  unsigned int note[NPARTS];
  unsigned int velocity[NPARTS];

  float outp[NPARTS][1+NOSCILLATORS+4]; /*"zero", oscillator outputs, 
                                          filter storage.
                                        FIXME: could be a struct?*/

  synti2_patch patch[NPARTS];   /* The sound parameters per part*/

  float delay[NDELAYS][DELAYSAMPLES]; /* Use of delays is optional,
                                         but the space costs nothing..*/

#ifndef ULTRASMALL
  /* Errors could be monitored by tools. Ultrasmall exe assumes correct data.*/
  unsigned int last_error_frame; /* Error position. */
  unsigned int last_error_type;  /* Error type */
  unsigned int last_error_info;  /* Some key information about the error. */
#endif
};


/* Jack interface needs to have this exposed. Otherwise can be static. */
#ifndef JACK_MIDI
static
#endif
void
synti2_player_event_add(synti2_player *pl, 
                        unsigned int frame, 
                        const byte_t *src, 
                        size_t n);


#endif
