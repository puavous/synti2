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
 */
#define NPARTS 16

/* Sound bank size equals the number of parts. Also this decision
 * stems from the 4k no-frills approach. This is no restriction for
 * more general use, because, in principle, you could have thousands
 * of patches off-line, and then load one of them for each of the 16
 * channels on-demand. For 4k, we expect to use only max 16 patches
 * and 16 channels, and everything is "hard-coded" (automatically,
 * though, as of now) at compile time.
 */
#define NPATCHES 16

/* Total number of "counters", i.e., oscillators/operators. */
#define NCOUNTERS (NVOICES * NOSCILLATORS)

/* Total number of envelopes */
#define NENVS (NVOICES * (NENVPERVOICE+1))

/* Maximum value of the counter type depends on C implementation, so
 * use limits.h -- TODO: Actually should probably use C99 and stdint.h !
 */
#define MAX_COUNTER UINT_MAX
/* The wavetable sizes and divisor-bitshift must be consistent. */
#define WAVETABLE_SIZE 0x10000
#define WAVETABLE_BITMASK 0xffff
/* Hmm... is there some way to get the implementation-dependent
   bit-count?*/
#define COUNTER_TO_TABLE_SHIFT 16

/* Storage size */
#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGEVENTS 15000


/** I finally realized that unsigned ints will nicely loop around
 * (overflow) and as such they model an oscillator's phase pretty
 * nicely. Can I use them for other stuff as well? Seem fit for
 * envelopes with only a little extra (clamping and stop flag). Could
 * I use just one huge counter bank for everything? Looks that
 * way. There seems to be a 30% speed hit for computing all the
 * redundant stuff. But a little bit smaller code, and the source
 * looks pretty nice too. TODO: think if it is more efficient to put
 * values and deltas in separate arrays. Probably, if you think of a
 * possible assembler implementation with multimedia vector
 * instructions(?). Or maybe not? Look at the compiler output...
 *
 * TODO: Think. Signed integers would overflow just as nicely. Could
 * it be useful to model things in -1..1 range instead of 0..1 ?
 *
 * TODO: Maybe separate functions for oscillators and envelopes could
 * still be used if they would compress nicely...
 */
typedef struct {
  unsigned int detect;
  unsigned int val;
  unsigned int delta;
  float f;  /* current output value (interpolant) */
  float ff;  /* current output value (1..0) */
  float fr;  /* current output value (0..1) */
  float aa; /* for interpolation start */
  float bb; /* for interpolation end */
} counter;

/* Events shall form a singly linked list. TODO: Is the list code too
 * complicated? Use just tables with O(n^2) pre-ordering instead??
 * Hmm.. after some futile attempts, I was unable to squeeze a smaller
 * code from any other approach, so I'm leaving the list as it is now.
 */
typedef struct synti2_player_ev synti2_player_ev;

struct synti2_player_ev {
  const byte_t *data;
  synti2_player_ev *next;
  unsigned int frame;
  int len;
};

struct synti2_player {
  synti2_player_ev *playloc; /* could we use just one loc for play and ins? */
  synti2_player_ev *insloc;  /* (one loc yielded larger exe on my first try)*/
  int fpt;          /* Frames per tick. integer => tempos inexact, sry! */
                    /* Hmm: Could I use some "automagic" tick counter? */
  int tpq;          /* Ticks per quarter (no support for SMPTE). */
  synti2_player_ev *freeloc; /*pointer to next free event structure*/
  int frames_done;  /* Runs continuously. Breaks after 12 hrs @ 48000fps !*/
  int sr;           /* Sample rate. */

  int idata;        /* index of next free data location */

  /* Only playable events. The first will be at evpool[0]! */
  synti2_player_ev evpool[SYNTI2_MAX_SONGEVENTS];
  /* Data for events. */
  byte_t data[SYNTI2_MAX_SONGBYTES];
};


/* The patch. The way things sound. */
typedef struct synti2_patch {
  int ipar3[SYNTI2_I3_NPARS];
  int ipar7[SYNTI2_I7_NPARS];
  float fenvpar[10]; /* FIXME: This zero-env hack OK? */
  float fpar[SYNTI2_F_NPARS];
} synti2_patch;

/** TODO: Not much is inside the part structure. Is it necessary at
 *  all? It will have controller values, though..
 */
typedef struct synti2_part {
  int voiceofkey[128];  /* Which note has triggered which voice */
                        /* TODO: disable multiple triggering(?) */
  int patch; /* Which patch is selected for this part. */
} synti2_part;

struct synti2_synth {
  /* I'll actually put the player inside the synthesizer. Should
   * probably call it "sequencer" instead of "player"... ? Seems to
   * yield smallest compressed code (with current function
   * implementations) when the player is dynamically allocated and the
   * pointer is stored as the very first field of the synth structure.
   */
  synti2_player *pl;
  unsigned long sr; /* Better for code size to have indiv. attrib 1st?*/

  float infranotes[128]; /* TODO: This space could be used for LFO's */
  float note2freq[128];  /* pre-computed frequencies of notes... Tuning
			    systems would be easy to change - just
			    compute a different table here (?)..*/
  float ultranotes[128]; /* TODO: This space could be used for noises? */

  float wave[WAVETABLE_SIZE];
  float rise[WAVETABLE_SIZE];
  float fall[WAVETABLE_SIZE];
  /*float noise[WAVETABLE_SIZE]; Maybe?? */

  /* Oscillators are now modeled as integer counters (phase). */
  /* Envelope progression also modeled by integer counters. Not much
   * difference between oscillators and envelopes!!
   */
  /* Must be in this order and next to each other exactly!! Impl. specif?*/
  counter c[NCOUNTERS];
  counter eprog[NVOICES][NENVPERVOICE+1];
  counter framecount;

  /* Envelope stages just a table? TODO: think.*/
  int estage[NVOICES][NENVPERVOICE+1];
  int sustain[NVOICES];

  int partofvoice[NVOICES];  /* which part has triggered each "voice";
                                -1 (should we use zero instead?) means
                                that the voice is free to re-occupy. */

  synti2_patch *patchofvoice[NVOICES];  /* which patch is sounding; */


  int note[NVOICES];
  int velocity[NVOICES];

  float outp[NVOICES][NOSCILLATORS+1+1]; /*"zero", oscillator outputs, noise*/

  /* The parts. Sixteen as in the MIDI standard. TODO: Could have more? */
  synti2_part part[NPARTS];   /* FIXME: I want to call this channel!!!*/
  synti2_patch patch[NPATCHES];   /* The sound parameters per part*/
};


/* Jack interface needs to have this exposed. */
#ifndef JACK_MIDI
static
#endif
void
synti2_player_event_add(synti2_player *pl, 
                        int frame, 
                        const byte_t *src, 
                        size_t n);


#endif
