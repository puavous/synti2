/** @file synti2.c
 *
 * Core functionality of Synti2 ("synti-kaksi"), a miniature realtime
 * software synthesizer with a sequence playback engine.
 * 
 * @author Paavo Nieminen <paavo.j.nieminen@jyu.fi>
 *
 * @copyright TODO: license to be determined upon release at Instanssi
 * 2012; definitely open source... maybe 'copyleft' if I get too
 * idealistic.
 */
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* For debug prints: */
#include <stdio.h>

#include "synti2.h"
#include "misss.h"

typedef unsigned char byte_t; 

/* Synti2 can be compiled as a Jack-MIDI compatible real-time soft
 * synth. Jack headers are not necessary otherwise...
 */
#ifdef JACK_MIDI
#include "jack/jack.h"
#include "jack/midiport.h"
#include <errno.h>
#endif

/* If any foreign MIDI protocol is used, note-offs should be converted
 * to our own protocol (also a well-known MIDI variant) in which
 * note-on with zero velocity means a note-off.
 */
#ifdef JACK_MIDI
#define DO_CONVERT_OFFS
#endif

/* Polyphony */
#define NVOICES 32

/* Multitimbrality */
#define NPARTS 16

/* Sound bank size (FIXME: separate the concept of patch and part!)*/
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

/* Number of inner loop iterations (audio frame evaluations) between
 * evaluating "slow-motion stuff" such as MIDI input and
 * envelopes. Must be a divisor of the buffer length. Example: 48000/8
 * would yield 6000 Hz (or faster than midi wire rate) responsiveness.
 * Hmm.. there is an audible problem with "jagged volume" (producing a
 * pitched artefact) if the volume envelope is outside of the inner
 * loop... So I'll keep at least the envelope progression counter code
 * inside. Same must be true for panning which is essentially a volume
 * env. Could it be that pitch or filter envelopes could be outside?
 * TODO: test..
 */
#define NINNERLOOP 8

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

  /* The parts. Sixteen as in the MIDI standard. TODO: Could have more? */
  synti2_part part[NPARTS];   /* FIXME: I want to call this channel!!!*/
  synti2_patch patch[NPATCHES];   /* The sound parameters per part*/
};

/* Random number generator was posted on musicdsp.org by Dominik
 * Ries. Thanks a lot; saves me a call to rand(). I use the seed as a
 * global because it is used by both the wavetable code and the
 * non-wavetable one.
 */
static  int RandSeed = 1;


/** Reads a MIDI variable length number. */
static 
int
__attribute__ ((noinline))
varlength(const byte_t * source, int * dest){
  int nread;
  byte_t byte;
  *dest = 0;
  for (nread=1; nread<=4; nread++){
    byte = *source++;
    *dest += (byte & 0x7f);
    if ((byte & 0x80) == 0)
      return nread; 
    else *dest <<= 7;
  }
  return 0; /* Longer than 4 bytes! Wrong input! FIXME: die. */
}



/** Adds an event to its correct location. Assumes that the
 * pre-existing events are ordered.
 */
static
void
synti2_player_event_add(synti2_player *pl, 
                        int frame, 
                        const byte_t *src, 
                        size_t n){
  synti2_player_ev *ev_new;
  while((pl->insloc->next != NULL) && (pl->insloc->next->frame <= frame)){
    pl->insloc = pl->insloc->next;
  }
  ev_new = pl->freeloc++;

  /* Fill in the node: */
  ev_new->data = src;         /* SHALLOW COPY here.*/
  ev_new->next = pl->insloc->next;
  ev_new->frame = frame;
  ev_new->len = n;
  pl->insloc->next = ev_new;
}

/** Returns a pointer to one past end of read in input data, i.e., next byte. */
static
byte_t *
synti2_player_merge_chunk(synti2_player *pl, 
                          const byte_t *r, 
                          int n_events)
{
  char chan, type;
  int ii;
  int frame, tickdelta;
  const byte_t *par;
  byte_t *msg;

  chan = *r++;
  type = *r++;
  par = r;
  frame = 0;
  pl->insloc = pl->evpool; /* Re-start merging from frame 0. */

  /* Always two parameters.. makes reader code simpler with not too
   * much storage overhead.
   */
  r += 2;   /* add number of parameters to r! */

  for(ii=0; ii<n_events; ii++){
    r += varlength(r, &tickdelta);
    frame += pl->fpt * tickdelta;

    msg = pl->data + pl->idata; /* Get next available data pool spot */

    if (type <= MISSS_LAYER_NOTES_CVEL_CPITCH){
      msg[0] = 0x90; /* MIDI Note on. */
      msg[1]= (par[0]==0xff) ? *r++ : par[0];
      msg[2]= (par[1]==0xff) ? *r++ : par[1];
      /* Now it is a complete msg. */
      synti2_player_event_add(pl, frame, msg, 3); 
      pl->idata += 3; /*Update the data pool top*/
    } else {
      /* Not yet implemented. FIXME: implement? */
      switch (type){
      case MISSS_LAYER_CONTROLLER_RESETS:
      case MISSS_LAYER_CONTROLLER_RAMPS:
      case MISSS_LAYER_SYSEX_OR_ARBITRARY:
      case MISSS_LAYER_NOTHING_AS_OF_YET:
      default:
        break;
      }
    }
  }
  return (byte_t*) r;
}

#ifdef JACK_MIDI
/** Creates a future for the player object that will repeat the next
 *  nframes of midi data from a jack audio connection kit midi port.
 *
 * ... If need be.. this could be made into merging function on top of
 * the sequence being played.. but I won't bother for today's
 * purposes...
 */
static
void
synti2_player_init_from_jack_midi(synti2_player *pl,
                                  jack_port_t *inmidi_port,
                                  jack_nframes_t nframes)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  byte_t *msg;
  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);

  /* Re-initialize, and overwrite any former data. */
  pl->playloc = pl->evpool; /* This would "rewind" the song */
  pl->insloc = pl->evpool; /* This would "rewind" the song */
  pl->freeloc = pl->evpool+1;
  pl->idata = 0;
  pl->playloc->next = NULL; /* This effects the deletion of previous msgs.*/
  
  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {

      /* Get next available spot from the data pool */
      msg = pl->data + pl->idata;
      memcpy(msg,ev.buffer,ev.size);  /* deep copy here */
      pl->idata += ev.size; /*Update the data pool top*/

      synti2_player_event_add(pl, 
                              pl->frames_done + ev.time, 
                              msg, 
                              ev.size);
    } else {
      break;
    }
  }
}


/** Interface. */
void
synti2_read_jack_midi(synti2_synth *s,
                      jack_port_t *inmidi_port,
                      jack_nframes_t nframes)
{
  synti2_player_init_from_jack_midi(s->pl, inmidi_port, nframes);
}
#endif

/** Load and initialize a song. Assumes a freshly created player object!*/
static
void
synti2_player_init_from_misss(synti2_player *pl, const byte_t *r)
{
  int chunksize;
  int uspq;
  /* Initialize an "empty" "head event": (really necessary?) */
  pl->freeloc = pl->evpool + 1;

  pl->playloc = pl->evpool; /* This would "rewind" the song */

  r += varlength(r, &(pl->tpq));  /* Ticks per quarter note */
  r += varlength(r, &uspq);       /* Microseconds per quarter note */
  pl->fpt = ((float)uspq / pl->tpq) * (pl->sr / 1000000.0f); /* frames-per-tick */
  /* TODO: Think about accuracy vs. code size */
  
  for(r += varlength(r, &chunksize); chunksize > 0; r += varlength(r, &chunksize)){
    r = synti2_player_merge_chunk(pl, r, chunksize); /* read a chunk... */
  }
}



static
void
synti2_do_receiveSysEx(synti2_synth *s, const byte_t * data);


/** Allocate and initialize a new synth instance. */
synti2_synth *
synti2_create(unsigned long sr, 
              const byte_t * patch_sysex, 
              const byte_t * songdata)
{
  synti2_synth * s;
  int ii;
  float t;

  s = calloc (1, sizeof(synti2_synth));

#ifndef ULTRASMALL
  if (s == NULL) return NULL;
#endif

  s->pl = calloc (1, sizeof(synti2_player));

#ifndef ULTRASMALL
  if (s->pl == NULL) {free(s); return NULL;}
#endif

  /* Initialize the player part. (Not much to be done...) */
  s->pl->sr = sr;
  s->sr = sr;
  if (songdata != NULL) synti2_player_init_from_misss(s->pl, songdata);

  s->framecount.delta = 1;

  if (patch_sysex != NULL)
    synti2_do_receiveSysEx(s, patch_sysex);


  /* Initialize the rest of the synth. */

  /* Create a note-to-frequency look-up table (cents need interpolation). 
   * TODO: Which is more efficient / small code: linear interpolation 
   * on every eval or make a lookup table on the cent scale...
   *
   * Idea: I could have negative notes for LFO's and upper octaves to
   * the ultrasound for weird aliasing/noise kinda stuff... just make
   * a bigger table here and let the pitch envelopes go beyond the
   * beyond!
   */
  for(ii=0; ii<128; ii++){
    s->note2freq[ii] = 440.0f * powf(2.0f, ((float)ii - 69.0f) / 12.0f );
  }

  for(ii=0; ii<NVOICES; ii++){
    s->partofvoice[ii]--;     /* == -1 .. Could I make this 0 somehow? */
  }

  for(ii=0; ii<WAVETABLE_SIZE; ii++){
    /*s->wave[ii] = sin(2*M_PI * ii/(WAVETABLE_SIZE-1));*/
    t = (float)ii/(WAVETABLE_SIZE-1);
    s->wave[ii] = sinf(2*M_PI * t);
    s->rise[ii] = t; 
    s->fall[ii] = 1.0f-t;
  }

  return s;
}


/** Note on OR note off (upon velocity == 0) */
static
void
synti2_do_noteon(synti2_synth *s, int part, int note, int vel)
{
  int voice, ie;

  /* note off */
  if (vel==0){
    voice = s->part[part].voiceofkey[note];
    //if (voice < 0) return; /* FIXME: think.. */
    /* TODO: release all envelopes.. */
    for (ie=0; ie<=NENVPERVOICE; ie++){
      s->estage[voice][ie] = 2;      /* skip to end */
      s->eprog[voice][ie].delta = 0; /* skip to end */
      s->eprog[voice][ie].val = 0;   /* must skip also value!! FIXME: think(?)*/
      s->sustain[voice] = 0;         /* don't renew loop. FIXME: necessary only if loop is used.*/
    }
    return; /* Note off is now handled. Otherwise do note on. */
  }

 
  /* note on */

  /* FIXME: Unimplemented plan: if patch is monophonic, always use the
   * voice corresponding to the part number. Otherwise, if that
   * primary voice is occupied, find a free voice starting from index
   * 17. This way, there is always one voice available per channel for
   * mono patches, and it will become more deterministic to know where
   * things happen, for possible visualization needs (but there is
   * evil resource wasting when the song uses less than 16 parts!
   * maybe some kind of free voice stack could be implemented with not
   * too much code...)
   */
  for(voice=0; voice < NVOICES-1; voice++){
    if (s->partofvoice[voice] < 0) break;
  }
  //if (voice==NVOICES) return; /* Cannot play new note! */
  /* (Could actually force the last voice to play anyway!?) */

  s->part[part].voiceofkey[note] = voice;
  /* How much code for always referencing through [voice]? voice.note better?*/
  s->partofvoice[voice] = part;
  s->patchofvoice[voice] = s->patch + part; // FIXME: s->part[part].patch
  s->note[voice] = note;
  s->velocity[voice] = vel;
  s->sustain[voice] = 1;  
 
  /* TODO: trigger all envelopes according to patch data..  Just give
     a hint to the evaluator function.. */
  for (ie=0; ie<=NENVPERVOICE; ie++){
    s->estage[voice][ie] = TRIGGERSTAGE;
    s->eprog[voice][ie].delta = 0;
  }
}


/** FIXME: Think about data model.. aim at maximal
    sparsity/compressibility but sufficient expressive range. If it
    turns out that the 1/1000 accuracy is very seldom required, then
    it could be worthwhile to store everything in the much simpler
    format of 4*7 = 28-bit signed integer representing (decimal)
    hundredths. Even then most parameters would only have the
    least-significant 7bit set?

    FIXME: If the song sequence data is finally read from SMF, then
    there will already be a subroutine that reads variable length
    values (the time deltas) which could be re-used here. But what
    about accuracy then?

    FIXME: Think about nonlinear parameter range. For example x^2 -
    usually accuracy is critical for the smaller parameter values. But
    no... pitch envelopes need to be accurate on a wide range!!

    SysEx format (planned; FIXME: implement!) 

    F0 00 00 00 [storeAddrMSB] [storeAddr2LSB] [inputLengthMSB] [inputLengthLSB]
    ... data LLSBs... 
    ... data LMSBs... 
    ... data MLSBs... 
    ... data MMSBs... 
    F7

    Length is also the stride for value encoding.

   FIXME: Manufacturer ID check.. length check; checksums :) could
   have checks.. :) but checks are for chicks? Could also check that
   there is F7 in the end :)

*/

static
void
synti2_do_receiveSysEx(synti2_synth *s, const byte_t * data){
  int opcode, offset, ir;
  int a, b, c, adjust_byte;
  synti2_patch *pat;
  float decoded;  
  float *dst; 
  static int stride = SYNTI2_F_NPARS; /* Constant, how to do?*/
  const byte_t *rptr;
  
  /* Sysex header: */
  data += 4; /* skip Manufacturer IDs n stuff*/
  /* FIXME: For 4k stuff these could be hardcoded!!?:*/
  /* Overall, these could be streamlined a bit*/
  opcode = *data++; opcode <<= 7; opcode += *data++;  /* what to do */
  offset = *data++; offset <<= 7; offset += *data++;  /* in where */

  /* Sysex data: */
  /* As of yet, offset==patch index. FIXME: Maybe more elaborate addressing? */

  if (opcode==0){
    /* Opcode 0: fill in patch memory (one or more patches at a
     * time). Data must be complete and without errors; no checks are
     * made in here.
     */
    for(pat = s->patch + offset; *data != 0xf7; pat++){
      
      for(ir=0;ir<SYNTI2_I3_NPARS; ir+=2){
        pat->ipar3[ir] = *data >> 3;
        pat->ipar3[ir+1] = (*data++) & 0x7;
      }

      for(ir=0;ir<SYNTI2_I7_NPARS; ir++){
        pat->ipar7[ir] = *data++;
      }

      for (ir=0; ir<stride; ir++){
        rptr = data++;
        a = *rptr;
        b = *(rptr += stride); 
        c = *(rptr += stride);
        adjust_byte = *(rptr += stride);
        decoded = 0.01f * a + b + 100*c;
      
#ifdef SUPER_ACCURATE_PATCHES
        /* Sigh.. I wanted this, but letting go by default. */
        decoded += (adjust_byte & 0x0f) * 0.001f;
#endif
      
        pat->fpar[ir+10] = (adjust_byte >> 4) ? -decoded : decoded; /* sign.*/
      }
      data += 3*stride;
    }
#ifndef NO_EXTRA_SYSEX
  } else if (opcode==1) {
    /* Receive one 3-bit parameter at location (patch,i3par_index) */
    pat = s->patch + (offset & 0x7f); 
    ir = offset >> 7;
    pat->ipar3[ir] = *data;
  } else if (opcode==2) {
    /* Receive one 7-bit parameter at location (patch,i7par_index) */
    pat = s->patch + (offset & 0x7f); 
    ir = offset >> 7;
    pat->ipar7[ir] = *data;
  } else if (opcode==3) {
    /* Receive one fixed point parameter at location (patch,fpar_index) */
    pat = s->patch + (offset & 0x7f); 
    ir = offset >> 7;
    /* FIXME: Decoding should be a static function instead of copy-paste:*/
    rptr = data++;
    a = *data++;
    b = *data++;
    c = *data++;
    adjust_byte = *data++;
    decoded = 0.01f * a + b + 100*c;
#ifdef SUPER_ACCURATE_PATCHES
    decoded += (adjust_byte & 0x0f) * 0.001f;
#endif
    pat->fpar[ir+10] = (adjust_byte >> 4) ? -decoded : decoded; /* sign.*/
#endif
  } else {
    /* Unknown opcode - should be an error. */
  }
}


/** Handles input that comes from the stored list of song events.
 *
 * Renders some frames of control data for the synth, keeping track
 * of song position. This will do the store()s as if the song was
 * played live. The dirty work of figuring out event timing has been
 * done by the song loader, so we just float in here.
 *
 * Upon entry (and all times): pl->next points to the next event to be
 * played. pl->frames_done indicates how far the sequence has been
 * played.
 *
 *  Things may happen late or ahead of time. I don't know if that is
 *  what they call "jitter", but if it is, then this is where I jit
 *  and jat...
 */
static
void
synti2_handleInput(synti2_synth *s, 
                   int upto_frames)
{
  /* TODO: sane implementation */
  const byte_t *midibuf;
  synti2_player *pl;

  pl = s->pl;

  while((pl->playloc->next != NULL) 
        && (pl->playloc->next->frame < upto_frames )) {
    pl->playloc = pl->playloc->next;

    midibuf = pl->playloc->data;
    if ((midibuf[0] & 0xf0) == 0x90){
      synti2_do_noteon(s, midibuf[0] & 0x0f, midibuf[1], midibuf[2]);

#ifdef DO_CONVERT_OFFS
    } else if ((midibuf[0] & 0xf0) == 0x80) {
      /* Convert any note-off to a note-on with velocity 0 here.
       * Necessary only if you don't have control over the note-off
       * protocol of the sender.
       */
      synti2_do_noteon(s, midibuf[0] & 0x0f, midibuf[1], 0);
#endif

#ifndef NO_SYSEX_RECEIVE
    } else if (midibuf[0] == 0xf0){
      /* Receiving SysEx is nice, but not strictly necessary if the
       * initial patch data is given upon creation.
       */
      synti2_do_receiveSysEx(s, midibuf);
#endif

    } else {
      /* Other MIDI messages are silently ignored. */
    }
  }
  pl->frames_done = upto_frames;
}



/* Advance the oscillator counters. Consider using the xmms assembly
 * for these? Try to combine with the envelope counter code somehow..
 * Would be simple: Only difference is the detection and clamping!!
 * counters = {framecount, osc1, osc2, ..., oscN, ev1, ev2, ..., evM}
 * and an "if i>N".
 */
static 
void
synti2_evalCounters(synti2_synth *s){
  counter *c;
  unsigned int detect;
  //  for(ic=0;ic<NCOUNTERS+NVOICES*NENVPERVOICE+1;ic++){
  for(c = s->c; c <= &s->framecount; c++){
    if (c->delta == 0) {continue;}  /* stopped.. not running*/
    c->detect = c->val; 
    c->val += c->delta;  /* Count 0 to MAX*/

    if (c >= s->eprog) { /* Clamp only latter half of counters. */
      if (c->val < c->detect){         /* Detect overflow*/
        c->val = MAX_COUNTER;          /* Clamp. */
        c->delta = 0;                  /* Stop after cycle*/
      }
    }

    /* Linear interpolation using pre-computed "fall" and "rise"
     * tables. Also, the oscillator phases will be the rise values.
     */
    c->fr = s->rise[c->val >> COUNTER_TO_TABLE_SHIFT];
    c->ff = s->fall[c->val >> COUNTER_TO_TABLE_SHIFT];
    c->f = c->ff * c->aa + c->fr * c->bb; 
  }
}


/** Updates envelope stages as the patch data dictates. 

  FIXME: just simple LFO stuff instead of this hack of a looping envelope?

  FIXME: Or make looping optional by a simple sound parameter... 

*/
static
void
synti2_updateEnvelopeStages(synti2_synth *s){
  int iv, ie, ipastend;
  int part;
  float nextgoal;
  float nexttime;
  synti2_patch *pat;

  for(iv=0; iv<NVOICES; iv++){
    /* Consider note instance "completely finished" when envelope 1 is
       over: */
    if (s->estage[iv][1] == 0) {
      s->partofvoice[iv] = -1;
    }

    part = s->partofvoice[iv];
    if (part<0) continue;
    pat = s->patchofvoice[iv]; /* Oh, the levels of indirection!(TODO:?)*/

    for (ie=1; ie<=NENVPERVOICE; ie++){
      //printf("At %d voice %d env %d\n", s->framecount.val, iv,ie); fflush(stdout);
      if (s->estage[iv][ie] == 0) continue; /* skip untriggered envs.FIXME: needed?*/
      /* Think... delta==0 on a triggered envelope means endclamp??
         NOTE: Need to set delta=0 upon note on!! and estage ==
         NSTAGES+1 or so (=6?) means go to attack.. */
      ipastend = SYNTI2_F_ENVS + (ie+1) * SYNTI2_NENVD;

      /* Find next non-zero-timed knee (or end.) */
      while ((s->eprog[iv][ie].delta == 0) && ((--s->estage[iv][ie]) > 0)){

#ifndef NO_LOOPING_ENVELOPES
        /* Seems to yield 55 bytes of compressed code!! Whyyy so much? */
        if ((s->estage[iv][ie] == 1) && (s->sustain[iv] != 0)){
          /*jack_info("Part %d: Env %d Reached stage %d, looping to %d", 
                    part, ie, s->estage[iv][ie], 
                    (int)s->patch[part*SYNTI2_NPARAMS 
                    + SYNTI2_IENVLOOP+ie]);*/
          s->estage[iv][ie] += pat->ipar3[SYNTI2_I3_ELOOP1+ie];
        }
#endif

        nexttime = pat->fpar[ipastend - s->estage[iv][ie] * 2 + 0];
        nextgoal = pat->fpar[ipastend - s->estage[iv][ie] * 2 + 1];
        s->eprog[iv][ie].aa = s->eprog[iv][ie].f;
        s->eprog[iv][ie].bb = nextgoal;
        if (nexttime <= 0.0f) {
          /*No time -> skip envelope knee. Force value to the new
            level (next goal). Delta remains at 0, and we may skip many.*/
          s->eprog[iv][ie].f = s->eprog[iv][ie].bb;
        } else {
          s->eprog[iv][ie].val = 0;    /* FIXME: Is it necessary to reset val? */
          s->eprog[iv][ie].delta = MAX_COUNTER / s->sr / nexttime;
        }
        /*if ((iv==0) && (ie<2))
          printf("v%02de%02d(Rx%02d): stage %d at %.2f to %.2f in %.2fs (d=%d) \n", 
                    iv, ie, part, s->estage[iv][ie], s->eprog[iv][ie].aa, 
                    s->eprog[iv][ie].bb, nexttime, s->eprog[iv][ie].delta);*/
      }
    }
  }
}


/** Converts note values to counter deltas. */
static
void
synti2_updateFrequencies(synti2_synth *s){
  int iv, note;
  int iosc;
  float notemod, interm, freq;
  synti2_patch *pat;

  /* Frequency computation... where to put it after all? */
  for (iv=0; iv<NVOICES; iv++){
    pat = s->patchofvoice[iv]; /* Oh, the levels of indirection!(TODO:?)*/
    if (pat==NULL) continue;

    for (iosc=0; iosc<NOSCILLATORS; iosc++){
      /* TODO: Pitch-note follow ratio .. */
      /* TODO: How much size gain from absolutely hard-coding envelopes n stuff?*/
      notemod = s->note[iv] 
        + s->eprog[iv][pat->ipar3[SYNTI2_I3_EPIT1+iosc]].f
        + pat->fpar[SYNTI2_F_DT1+iosc];
      note = notemod; /* should make a floor (does it? check spec)*/
      interm = (1.0f + 0.05946f * (notemod - note)); /* +cents.. */
      freq = interm * s->note2freq[note];
      s->c[iv*NOSCILLATORS+iosc].delta = freq / s->sr * MAX_COUNTER;
    }
  }
}


void
synti2_render(synti2_synth *s,
              synti2_smp_t *buffer,
              int nframes)
{
  int iframe, ii, iv;
  float interm;
  int wtoffs;
  synti2_player *pl;
  synti2_patch *pat;

  pl = s->pl;
  
  for (iframe=0; iframe<nframes; iframe += NINNERLOOP){
    /* Outer loop for things that are allowed some jitter. (further
     * elaboration in comments near the definition of NINNERLOOP).
     */
    
    synti2_handleInput(s, s->framecount.val + iframe + NINNERLOOP);
    synti2_updateEnvelopeStages(s); /* move on if need be. */
    synti2_updateFrequencies(s);    /* frequency upd. */
    
    /* Inner loop runs the oscillators and audio generation. */
    for(ii=0;ii<NINNERLOOP;ii++){
      
      /* Could I put all counter upds in one big awesomely parallel loop? */
      synti2_evalCounters(s);  /* .. apparently yes ..*/
      
      /* Getting more realistic soon: */
      buffer[iframe+ii] = 0.0f;
      
      for(iv=0;iv<NVOICES;iv++){

        /* Hmm.. these not needed if we hardcode the patch algorithm!! */
        pat = s->patchofvoice[iv];
        if (pat==NULL) continue;
        /* if (s->partofvoice[iv] < 0) continue; Unsounding. FIXME: how? */
        /* Produce sound :) First modulator, then carrier*/
        /* Wavetable definitely! Could bit-shift the counter... */
        wtoffs = (unsigned int)(s->c[iv*NOSCILLATORS+1].fr * WAVETABLE_SIZE) & WAVETABLE_BITMASK;
        interm  = s->wave[wtoffs];
        //interm *= interm * interm; /* Hack!! BEAUTIFUL!!*/
        interm *= (s->velocity[iv]/128.0f);  /* Velocity sensitivity */
        interm *= (s->eprog[iv][pat->ipar3[SYNTI2_I3_EAMP2]].f);
        wtoffs = (unsigned int)((s->c[iv*NOSCILLATORS+0].fr + interm) * WAVETABLE_SIZE) & WAVETABLE_BITMASK;
        interm  = s->wave[wtoffs];
        interm *= s->eprog[iv][pat->ipar3[SYNTI2_I3_EAMP1]].f;

        RandSeed *= 16807;

        interm += s->eprog[iv][pat->ipar3[SYNTI2_I3_EAMPN]].f 
          * (float)RandSeed * 4.6566129e-010f; /*noise*/

        buffer[iframe+ii] += interm;
      }      
      buffer[iframe+ii] /= NVOICES;
      
      //buffer[iframe+ii] = sin(32*buffer[iframe+ii]); /* Hack! beautiful too!*/
      //buffer[iframe+ii] = tanh(16*buffer[iframe+ii]); /* Hack! beautiful too!*/
    }
  }
}
