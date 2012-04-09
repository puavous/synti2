/** @file synti2.c
 *
 * Core functionality of Synti2 ("synti-kaksi"), a miniature realtime
 * software synthesizer with a sequence playback engine. Released at
 * Instanssi 2012;
 * 
 * @author Paavo Nieminen <paavo.j.nieminen@jyu.fi>
 *
 * @copyright 2012. MIT License, see LICENSE.txt
 */
#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* For debug prints: */
#include <stdio.h>

#include "synti2.h"
#include "synti2_guts.h"
#include "synti2_misss.h"


/* Number of inner loop iterations (audio frame evaluations) between
 * evaluating "slow-motion stuff" such as MIDI input and
 * envelopes. Must be a divisor of the buffer length. Example: 48000/8
 * would yield 6000 Hz (or faster than midi wire rate) responsiveness.
 * Hmm.. there is an audible problem with "jagged volume" (producing a
 * pitched artefact) if the amplitude envelope is outside of the inner
 * loop... So I'll keep at least the envelope progression counter code
 * inside. Same must be true for panning which is essentially an amplitude
 * env. Could it be that pitch or filter envelopes could be outside?
 * TODO: test..
 *
 * FIXME: As a second thought, I think now that for 4k/tiny stuff it
 * would be best to not separate the inner loop at all. The
 * performance hit could be compensated by other means in composition
 * mode, and in playback mode we can have a longer output buffer in
 * any case. So the actual to-do is to merge the inner loop to the
 * outer one, maintaining real-time capacity in composition mode.
 */
#define NINNERLOOP 16


/* local subr. declared here */
static
void
synti2_fill_patches_from(synti2_patch *pat, const unsigned char *data);


/* The simple random number generator was posted on musicdsp.org by
 * Dominik Ries. Thanks a lot.
 */
static int RandSeed = 1;

/** Reads a MIDI variable length number. "Varlengths" are used in my
 * native song format. I was thinking about re-using the scheme for
 * other stuff, like patch parameters, but I was unable to figure out
 * how to make significant improvements to code size that way. I may
 * have overlooked something, though, as always.
 */
static 
int
__attribute__ ((noinline))
varlength(const byte_t * source, unsigned int * dest){
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
#ifndef ULTRASMALL
  /* Longer than 4 bytes! Actually unexpected input which should be
     caught as a bug. */
  return 0;
#endif
}



/** Adds an event to its correct location; makes no checks for empty
 * messages, i.e., assumes n >= 1. Also assumes that the pre-existing
 * events are ordered. This can be static for playback-mode, but in
 * real-time mode the MIDI interface module needs to see this. 
 *
 * TODO: relevant to any MIDI interface, not only jack, so rename the
 * macro...
 *
 * FIXME: Now that I'm using an internal event format in any case,
 * could I fix the length? I suppose I could... there are not so many
 * different messages, and the bulk data message (which is the only
 * variable-length event) could contain a native pointer to a memory
 * area... maybe?
 */
#ifndef JACK_MIDI
static
#endif
void
synti2_player_event_add(synti2_player *pl, 
                        unsigned int frame, 
                        const byte_t *src, 
                        size_t n){
  synti2_player_ev *ev_new;
  while((pl->insloc->next != NULL) && (pl->insloc->next->frame <= frame)){
    pl->insloc = pl->insloc->next;
  }
  ev_new = pl->freeloc++;  /*FIXME: Will spill over lethally! */

  /* Fill in the node: */
  ev_new->data = src;         /* SHALLOW COPY here.*/
  ev_new->next = pl->insloc->next;
  ev_new->frame = frame;
  ev_new->len = n;
  pl->insloc->next = ev_new;
  /*printf("stored frame %d data %02x %02x %02x\n", frame, src[0], src[1], src[2]); fflush(stdout);*/
  
}

/** Merges a chunk, aka layer, to the list of events to be
 *  played. Returns a pointer to one past end of read in input data,
 *  i.e., next byte.
 */
static
byte_t *
synti2_player_merge_chunk(synti2_player *pl, 
                          const byte_t *r, 
                          int n_events)
{
  char chan, type;
  int ii;
  unsigned int frame, tickdelta;
  const byte_t *par;
  byte_t *msg;

  chan = *r++;
  type = *r++;
  par = r;
  frame = 0;
  pl->insloc = pl->evpool; /* Re-start merging from frame 0. */
  r += 2; /* add number of parameters to r! */
  /* Always two parameters.. makes reader code simpler with not too
   * much storage overhead.
   */

  for(ii=0; ii<n_events; ii++){
    r += varlength(r, &tickdelta);
    frame += pl->fpt * tickdelta;

    msg = pl->data + pl->idata; /* Get next available data pool spot */

    if (type <= MISSS_LAYER_NOTES_CVEL_CPITCH){
      /* Produce a 'Note on' message in our internal midi-like format. */
      msg[0] = MISSS_MSG_NOTE;
      msg[1] = chan;
      msg[2] = (par[0]==0xff) ? *r++ : par[0];
      msg[3] = (par[1]==0xff) ? *r++ : par[1];
      synti2_player_event_add(pl, frame, msg, 4);
      pl->idata += 4; /* Update the data pool top */
    } else {
      switch (type){
      case MISSS_LAYER_CONTROLLER_RAMPS:
	/* Not yet implemented. FIXME: implement? Controller reset==fast ramp!*/
	/* FIXME: Is it possible to use the counter logic for these? */
	break;
#if 0
      case MISSS_LAYER_SYSEX_OR_ARBITRARY:
        /* Not yet implemented. FIXME: not really necessary for 4k?
	   I'm getting the feeling that in 4k playback mode this is
	   definitely not needed. So I really only have two types of
	   layers?*/
	break;
#endif
#ifndef ULTRASMALL
      default:
        /* Unexpected input; maybe handle somehow as an error.. */
        break;
#endif
      }
    }
  }
  return (byte_t*) r;
}

/** Load and initialize a song. Assumes a freshly created player object!*/
static
void
synti2_player_init_from_misss(synti2_player *pl, const byte_t *r)
{
  unsigned int chunksize;
  unsigned int uspq;
  /* Initialize an "empty" "head event": (really necessary?) */
  pl->freeloc = pl->evpool + 1;

  pl->playloc = pl->evpool; /* This would "rewind" the song */

  /* TODO: Now we use the standard MIDI tempo unit of MSPQN. For our
   * 4k purposes, we might not need tpq at all. Just give us
   * microseconds per tick, and we'll be quite happy here and try to
   * match it approximately by framecounts... TODO: Think about
   * accuracy vs. code size once more in here(?).
   */
  r += varlength(r, &(pl->tpq));  /* Ticks per quarter note */
  r += varlength(r, &uspq);       /* Microseconds per quarter note */
  pl->fpt = ((float)uspq / pl->tpq) / (1000000.0f / pl->sr); /* frames-per-tick */
  for(r += varlength(r, &chunksize); chunksize > 0; r += varlength(r, &chunksize)){
    r = synti2_player_merge_chunk(pl, r, chunksize); /* read a chunk... */
  }
}



/** Initialize a new synth instance. To make the code smaller, we
 *  assume that somebody else has made the allocation. It can be a
 *  static chunk of global data, for example.
 */
void
synti2_init(synti2_synth * s,
            unsigned long sr, 
            const byte_t * patchdata, 
            const byte_t * songdata)
{
  int ii, wt;
  float t;

  memset(s, 0, sizeof(s));     /* zero */

  s->pl = &s->_actual_player;  /* no more necessary (FIXME everywh.)*/

  /* Initialize the player module. (Not much to be done...) */
  s->pl->sr = sr;
  s->sr = sr;
  s->framecount.delta = 1;

#ifndef ULTRASMALL
  if (songdata != NULL) 
    synti2_player_init_from_misss(s->pl, songdata);

  if (patchdata != NULL)
    synti2_fill_patches_from(s->patch, patchdata+8);
#else
  /* In "Ultrasmall" mode, we trust the user to provide all data.*/
#ifndef EXTREME_NO_SEQUENCER
  /* Extreme hackers may have made a smaller sequencer/music generator:)*/
  synti2_player_init_from_misss(s->pl, songdata);
#endif
  /* Patches are to be made with some patch editor, though.*/
  synti2_fill_patches_from(s->patch, patchdata+8);
#endif


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

  for(ii=0; ii<WAVETABLE_SIZE; ii++){
    /*s->wave[ii] = sin(2*M_PI * ii/(WAVETABLE_SIZE-1));*/

    t = (float)ii/(WAVETABLE_SIZE-1);
    s->rise[ii] = t; 
    s->wave[0][ii] += sinf(2*M_PI * t);

#ifndef NO_EXTRA_WAVETABLES
    for(wt=1; wt<NHARM; wt++){
      s->wave[wt][ii] = s->wave[wt-1][ii] 
        + 1.f/(wt+1) * sinf(2*M_PI * (wt+1) * t);
    }
#endif

  }
}


/** 
 * Note on OR note off (upon velocity == 0).
 *
 * Note or velocity must not jump at a note-off, when the parameters are
 * likely both different from what they were at note-on.
 *
 * NOTE: I tried if memset could be economical in zeroing the
 * counters. It might be, but the address computation compiles into so
 * many instructions.. maybe some real savings could come from
 * destroying the nice struct layout of things, if the pointers to
 * things could be more easily remembered. Or, actually, if the
 * resettable values were arranged together part-wise and only one
 * memset(p,0,sz) would be required to perform note-on/off.
 */
#ifndef EXTREME_NO_SEQUENCER /* convenience for sequencer-less conduct */
static
#endif
void
synti2_do_noteon(synti2_synth *s, 
                 unsigned int voice, 
                 unsigned int note, 
                 unsigned int vel)
{
  int ie;

#ifndef NO_LOOPING_ENVELOPES
  s->sustain[voice] = vel;
#endif

#ifndef NO_NOTEOFF
  if (vel==0){
    for (ie=0; ie<=NENVPERVOICE; ie++){
      s->estage[voice][ie] = RELEASESTAGE; /
      s->eprog[voice][ie].delta = 0;
      s->eprog[voice][ie].val = 0;   /* must skip also value!? FIXME: think(?)*/
    }
    return; /* Note off done. */
  }
#endif
 
  /* note on */
#ifndef NO_VELOCITY
  s->velocity[voice] = vel;
#endif

  s->note[voice] = note;
  /* Trigger all envelopes. Just give a hint to the evaluator function.. */
  for (ie=0; ie<=NENVPERVOICE; ie++){
    s->estage[voice][ie] = TRIGGERSTAGE;
    s->eprog[voice][ie].delta = 0;
  }

#ifdef DO_RESET_PHASE
  /* Adds to code size and produces clicks when note tail is still
   * sounding, but makes each note start with deterministic (zero)
   * phase difference between oscillators.
   */
  for(ie=0;ie<NOSCILLATORS;ie++) {
    s->c[NOSCILLATORS*voice+ie].val = 0;
  }
#endif

}

/** Decodes a "floating point" synthesis parameter. TODO: Maybe try
 * yet more different approaches to data storage?
 */
static
float
/*__attribute__ ((noinline))*/ /* longer code with an actual call..*/
synti2_decode_fpar(unsigned char a, unsigned char b){
  float decoded;
  decoded = ((a & 0x03) << 7) + b;   /* 2 + 7 bits accuracy*/
  decoded = (a & 0x40) ? -decoded : decoded;  /* sign */
  for (a=((a & 0x0c) >> 2); a < 3; a++) decoded *= .1f; /* e-N; N=0..2) */
  return decoded;
}


static
void
synti2_fill_patches_from(synti2_patch *pat, const unsigned char *data)
{
  int ir;
  for(; *data != 0xf7; pat++){
    for(ir=0;ir<SYNTI2_I3_NPARS; ir+=2){
      pat->ipar3[ir] = *data >> 3;
      pat->ipar3[ir+1] = (*data++) & 0x7;
    }

    for (ir=0; ir<SYNTI2_F_NPARS; ir++){
      pat->fpar[ir] = synti2_decode_fpar(data[ir],
                                         data[ir+SYNTI2_F_NPARS]); 
    }
    data += 2*SYNTI2_F_NPARS;
  }
}


#ifndef NO_RECEIVE_SYSEX
/** Receive arbitrary data (one instance of which can be the contents
 *  of a "MIDI SysEx"). (Convenient for sound editing, but not
 *  strictly necessary for stand-alone 4k synth. Therefore optional
 *  compilation). The actual SysEx things have been dealt with prior
 *  to entry, and this can receive just the bulk data.
 *
 * FIXME: Verify the sanity of this function... In particular (at
 * least): This is unlikely to be used in 4k mode, so there's no limit
 * in code size of this function. This is not visible to the outside,
 * so the 7 bit restriction from the MIDI sysex world is not necessary
 * if the 8th bit can be used for something more useful.
 *
 * FIXME: Verify the necessity of this function. There will be the
 * MIDI translator module in any case, so see if it could deal
 * directly with handleInput()
 *
 * FIXME: The 7bit restriction is as invalid for all the other parts
 * of the synth core, now that the MIDI interface is cleaned away from
 * here. And thus the 3bit parameters could be extended to as much as
 * 4 bits?
 */
static
void
synti2_do_receiveData(synti2_synth *s, const byte_t * data){
  int opcode, offset, ir;
  synti2_patch *pat;

  /* Data header: */
  opcode = *data++; opcode <<= 7; opcode += *data++;  /* what to do */
  offset = *data++; offset <<= 7; offset += *data++;  /* in where */

  /* "Sysex" data: */
  /* As of yet, offset==patch index. TODO: More elaborate addressing? */

  if (opcode==MISSS_OP_FILL_PATCHES){
    synti2_fill_patches_from(s->patch + (offset & 0x7f), data);
  } else if (opcode==MISSS_OP_SET_3BIT) {
    /* Receive one (supposedly 3-bit) parameter at location
     * (patch,i3par_index) */
    pat = s->patch + (offset & 0x7f); 
    ir = offset >> 7;
    pat->ipar3[ir] = *data;
  } else if (opcode==MISSS_OP_SET_F){
    /* Receive one float parameter at location (patch,fpar_index) */
    pat = s->patch + (offset & 0x7f); 
    ir = offset >> 7;
    pat->fpar[ir] = synti2_decode_fpar(data[0], data[1]);
  } else {
    /* Unknown opcode - should be an error. */
  }
}
#endif

#ifndef EXTREME_NO_SEQUENCER
/** 
 * Handles input that comes from the stored list of song events;
 * forwards control data for the synth and keeps track of song
 * position. All necessary conversion and event timing work has been
 * done by either a real-time midi adapter or the sequencer's song
 * loader, and we can just go with the pre-determined flow of our own
 * internal messages here.
 *
 * Upon entry (and all times): pl->next points to the next event to be
 * played. pl->frames_done indicates how far the sequence has been
 * played.
 *
 * This is part of the "sequencer module" which can be left out, if
 * you want to control the engine without it.
 *
 */
static
void
synti2_handleInput(synti2_synth *s, 
                   unsigned int upto_frames)
{
  const byte_t *midibuf;
  synti2_player *pl;

  pl = s->pl;  /* TODO: Think about the role of the player structure...*/

  while((pl->playloc->next != NULL) 
        && (pl->playloc->next->frame < upto_frames )) {
    pl->playloc = pl->playloc->next;

    midibuf = pl->playloc->data;
    if (midibuf[0] == MISSS_MSG_NOTE){
      synti2_do_noteon(s, midibuf[1], midibuf[2], midibuf[3]);

#ifndef NO_CC
    } else if (midibuf[0] == MISSS_MSG_SETF){
      /* A native float format is provided for our convenience: */
      s->patch[midibuf[1]].fpar[midibuf[2]] = *((float*)(midibuf+3));
      /* or synti2_do_setf(midibuf[1], midibuf[2], (float*)(midibuf+3)?*/
#endif

#ifndef NO_SYSEX_RECEIVE
    } else if (midibuf[0] == MISSS_MSG_DATA){
      /* Receiving SysEx is nice, but not strictly necessary if the
       * initial patch data is given upon creation.
       */
      synti2_do_receiveData(s, midibuf+1);
#endif

    } else {
      /* Other messages are actually an error. */
      /* FIXME: Handle somehow?
       */
    }
  }
  pl->frames_done = upto_frames;
}
#endif


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
  unsigned int ind;
  /* for(ic=0;ic<NCOUNTERS+NPARTS*NENVPERVOICE+1;ic++){*/
  for(c = s->c; c <= &s->framecount; c++){
    if (c->delta == 0) {continue;}  /* stopped.. not running*/
    c->detect = c->val; 
    c->val += c->delta;  /* Count 0 to MAX*/

    if (c >= (counter*) (s->eprog)) { /* Clamp only latter half of counters. */
      if (c->val < c->detect){         /* Detect overflow*/
        c->val = MAX_COUNTER;          /* Clamp. */
        c->delta = 0;                  /* Stop after cycle*/
      }
    }

    /* Linear interpolation using pre-computed "fall" and "rise"
     * tables. Also, the oscillator phases will be the rise values.
     * Phew.. shaved off a byte by letting go of "fall" table...
     * FIXME: See if there is a significant performance hit.. likely not(?)
     */
    ind = c->val >> COUNTER_TO_TABLE_SHIFT;
    c->fr = s->rise[ind];
    c->f = (1.0f-c->fr) * c->aa + c->fr * c->bb; 
  }
}


/** 
 * Updates envelope stages as the patch data dictates. A looping
 * 5-knee envelope is used, and this is my final choice for the synti2
 * synth, mostly because I feel that it is simple in a
 * musician-friendly way, which is one leading design principle for
 * synti2, even though it conflicts with the size constraints. Another
 * reason is that it was in the original plan, and I want to fix the
 * feature set of synti2 very soon.
 *
 * TODO: (In some later, re-designed project) the envelope code could
 * be made simpler by letting go of the whole loop idea, and, instead,
 * using some kind of LFOs or inter-channel patching for modulation
 * effects.
 *
 * TODO: (In some other later project) the envelopes could be made
 * more interesting by combining more parts than just 5 linear
 * interpolation knees. Consider a freely editable list of arbitrary
 * combinations of constant+anyWaveTable at editable frequencies. I
 * think that such code could (somehow, maybe) support both compact
 * storage for 4k and, at the same time, unlimited flexibility for
 * unconstrained productions.
 */
static
void
synti2_updateEnvelopeStages(synti2_synth *s){
  int iv, ie, ipastend;
  float nextgoal;
  float nexttime;
  synti2_patch *pat;
#ifndef NO_SAFETY
  /* Looping over all-zero times produces an infinite iteration which
   * is not nice, especially in a real-time audio system. So, there is
   * an optional safety mechanism for real-time work. In a
   * pre-determined 4k setting you'd know that the patches contain no
   * zero-time loops, and turn off this watchdog.
   */
  int iter_watch;
#endif

  /* remember that voice == part under the new design decisions.. */
  for(iv=0; iv<NPARTS; iv++){
    pat = s->patch + iv;

    /* Envelope 0 is never touched. Therefore it yields a constant
     * 0. I don't know if this hack is very useful, but it is in its
     * own way very beautiful, so I leave it untouched. That's why the
     * loop begins from envelope 1:
     */
    for (ie=1; ie<=NENVPERVOICE; ie++){
      if (s->estage[iv][ie] == 0) continue; /* skip untriggered. */

      /* reverse order in the stages, so a bit awkward indexing. */
      ipastend = SYNTI2_F_ENVS + (ie+1) * SYNTI2_NENVD;

#ifndef NO_SAFETY
      iter_watch = 0;
#endif
      /* Find next non-zero-timed knee (or end). delta==0 on a
	 triggered envelope means endclamp/noteon. */
      while ((s->eprog[iv][ie].delta == 0) && ((--s->estage[iv][ie]) > 0)){
#ifndef NO_LOOPING_ENVELOPES
        /* The loop logic seems to yield 55 bytes of compressed code!!
	 * Whyyy so much?  Hmm... the address computation becomes
	 * filthy long. FIXME: Consider some tricks? Pre-computation?
	 * Re-ordering of storage?
	 */
        if ((s->estage[iv][ie] == 1) && (s->sustain[iv] != 0)){
          s->estage[iv][ie] += pat->ipar3[(SYNTI2_I3_ELOOP1-1)+ie]; /*-1*/
#ifndef NO_SAFETY
          if ((iter_watch++) > 5) s->sustain[iv] = 0; /* stop ifinite loop. */
#endif
        }
#endif
        nexttime = pat->fenvpar[ipastend - s->estage[iv][ie] * 2 + 0];
        nextgoal = pat->fenvpar[ipastend - s->estage[iv][ie] * 2 + 1];
        s->eprog[iv][ie].aa = s->eprog[iv][ie].f;
        s->eprog[iv][ie].bb = nextgoal;
        if (nexttime <= 0.0f) {
          /* No time -> skip envelope knee. Force value to the new
           * level (next goal). Delta remains at 0, so we may skip many.
           */
          s->eprog[iv][ie].f = s->eprog[iv][ie].bb;
        } else {
          s->eprog[iv][ie].val = 0;
          s->eprog[iv][ie].delta = MAX_COUNTER / s->sr / nexttime;
        }
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
  for (iv=0; iv<NPARTS; iv++){
    pat = s->patch + iv;

    /* FIXME: See if "completely finished sound" logic is required? I
       think the following neverhappen was an early placeholder for
       such... */
    /* if (pat==NULL) continue; */

    for (iosc=0; iosc<NOSCILLATORS; iosc++){
      /* TODO: Pitch-note follow ratio .. */

      notemod = s->note[iv];
#ifndef NO_PITCH_ENV
      notemod += s->eprog[iv][pat->ipar3[SYNTI2_I3_EPIT1+iosc]].f;
#endif

#ifndef NO_DETUNE
      notemod += pat->fpar[SYNTI2_F_DT1 + iosc];    /* "coarse" */
      notemod += pat->fpar[SYNTI2_F_DT1F + iosc];   /* "fine"   */
#endif

#ifndef NO_PITCH_BEND
      /* Pitch bends for all oscillators. FIXME: Allow weird effects by
	 [SYNTI2_F_PBAM+iosc]? In code, it is only one plus; in patch
	 data it is number of channels times number of oscillators
	 fpars, which is a lot. Unless I come up with a new, sparse,
	 storage for the patch data. Hmm.. why not, indeed.. */
      notemod += pat->fpar[SYNTI2_F_PBVAL] * pat->fpar[SYNTI2_F_PBAM];
#endif

      note = notemod; /* should make a floor (does it? check spec)*/
      interm = (1.0f + 0.05946f * (notemod - note)); /* +cents.. */
      freq = interm * s->note2freq[note]; /* could be note2delta[] */
      s->c[iv*NOSCILLATORS+iosc].delta = freq / s->sr * MAX_COUNTER;
    }
  }
}


#ifndef NO_FILTER
/** A filter. Taken from
 *  http://www.musicdsp.org/showArchiveComment.php?ArchiveID=23
 *
 *     Type : 12db resonant low, high or bandpass
 *
 *     References : Effect Design Part 1, Jon Dattorro, J. Audio
 *     Eng. Soc., Vol 45, No. 9, 1997 September
 *
 *     Notes : Digital approximation of Chamberlin two-pole low
 *     pass. Easy to calculate coefficients, easy to process
 *     algorithm.
 *
 *    cutoff = cutoff freq in Hz
 *    fs = sampling frequency //(e.g. 44100Hz)
 *    f = frqHz / sampleRate*4.;  hmm what it means..
 *    q = resonance/bandwidth [0 < q <= 1]  most res: q=1, less: q=0
 *    scale = q  OR scale = sqrt(q)?? 
 *
 *  Algorithm:
 *    f = frqHz / sampleRate*4   (mine comes as 0-1 sound param.)
 *                               (so Hz depends on sample rate!
 *                                should check on 44.1kHz systems
 *                                to see if songs sound too bad..)
 *    low = low + f * band;
 *    high = scale * input - low - q*band;   was scale=sqrt(q) 
 *    band = f * high + band;
 *    (save low and band for next round.)
 */
static void apply_filter(synti2_synth *s, 
                         synti2_patch *pat, 
                         float *store){
#define FIL_IN 0
#define FIL_BP 1
#define FIL_LP 2
#define FIL_HP 3
#define FIL_NF 4

  float f,q;
  /* At first use only a static filter frequency. TODO: Maybe use an
     envelope later? FIXME: Check the value range once again..? */

  f = 1000.f * pat->fpar[SYNTI2_F_FFREQ] / s->sr;
  q = 1.0f - pat->fpar[SYNTI2_F_FRESO];

  store[FIL_LP] += f * store[FIL_BP];
  store[FIL_HP]  = q * store[FIL_IN] - store[FIL_LP] - q * store[FIL_BP];
  store[FIL_BP] += f * store[FIL_HP];
#ifndef NO_NOTCH_FILTER
  store[FIL_NF] = store[FIL_LP] + store[FIL_HP];
#endif
}
#endif  /*NO_FILTER*/



void
synti2_render(synti2_synth *s,
              synti2_smp_t *buffer,
              int nframes)
{
  unsigned int dsamp;
  int iframe, ii, iv;
  int iosc;
  float interm;
  int wtoffs;
  synti2_patch *pat;

  float *sigin;  /* Input signal table during computation. */
  float *signal; /* Output signal destination during computation. */

  for (iframe=0; iframe<nframes; iframe += NINNERLOOP){
    /* Outer loop for things that are allowed some jitter. (further
     * elaboration in comments near the definition of NINNERLOOP).
     */
    
#ifndef EXTREME_NO_SEQUENCER
    /* If they wish to generate do_note_on() without my beautiful
       sequencer interface, by all means let them!! FIXME:
       do_note_on() needs to be non-static in that case, though. */
    synti2_handleInput(s, s->framecount.val + iframe + NINNERLOOP);
#endif
    synti2_updateEnvelopeStages(s); /* move on if need be. */
    synti2_updateFrequencies(s);    /* frequency upd. */
    
    /* Inner loop runs the oscillators and audio generation. */
    for(ii=0;ii<NINNERLOOP;ii++){
      
      /* Could I put all counter upds in one big awesomely parallel loop? */
      synti2_evalCounters(s);  /* .. apparently yes ..*/
      
      /* Sound output. Getting more realistic as we speak... */
      buffer[iframe+ii] = 0.0f;
      
      for(iv=0;iv<NPARTS;iv++){

        pat = s->patch + iv;
        /* FIXME: Need logic for "unsounding"? Yes, an #ifndef NO_SKIP_UNPLAYING 
	 but then what is the rule? op4amp? how about delay tricks then? */

        sigin  = signal = &(s->outp[iv][0]);
  
        for(iosc = 0; iosc < NOSCILLATORS; iosc++){
          signal++; /* Go to next output slot. */
          wtoffs = (unsigned int)
            ((s->c[iv*NOSCILLATORS+iosc].fr 
              + sigin[pat->ipar3[SYNTI2_I3_FMTO1+iosc]])  /* FM modulator */
              * WAVETABLE_SIZE) & WAVETABLE_BITMASK;

#ifndef NO_EXTRA_WAVETABLES
          interm  = s->wave[pat->ipar3[SYNTI2_I3_HARM1+iosc]][wtoffs];
#else
          interm  = s->wave[0][wtoffs];
#endif

	  /* parallel mix could be optional? Actually also FM could be? */
          /* could reorganize I3 parameters for shorter code here(?): */
          interm *= (s->eprog[iv][pat->ipar3[SYNTI2_I3_EAMP1+iosc]].f);
          interm += sigin[pat->ipar3[SYNTI2_I3_ADDTO1+iosc]]; /* parallel */
          interm *= pat->fpar[SYNTI2_F_LV1+iosc]; /* level/gain */
#ifndef NO_VELOCITY
          /* Optional velocity FIXME: think. Table lookup? Hmm.. */
          if (pat->ipar3[SYNTI2_I3_VS1+iosc]){
            interm *= s->velocity[iv] / 127.f; /* Now just linear. */
          }
#endif
          *signal = interm;
        }

        /* Optional additive noise after FM operator synthesis:
           (FIXME: could/should use wavetable for noise?) */
#ifndef NO_NOISE
        RandSeed *= 16807;
        interm += s->eprog[iv][pat->ipar3[SYNTI2_I3_EAMPN]].f 
          * pat->fpar[SYNTI2_F_LVN]       /*noise gain*/
          * (float)RandSeed * 4.6566129e-010f; /*noise*/
#endif

#ifndef NO_DELAY
        /* Additive mix from a delay line. FIXME: Could have wild
           results from modulating with a delayed mix.. sort of like a
           "feedback" operator. But the oscillator and envelope code
           may need some rethinking in that case (?) ... */
        dsamp = s->framecount.val;
        interm += s->delay[pat->ipar3[SYNTI2_I3_DIN]][dsamp % DELAYSAMPLES]
          * pat->fpar[SYNTI2_F_DINLV];
#endif

#ifndef NO_FILTER
        /* Skip for faster computation. Should do the same for delays! */
        if(pat->ipar3[SYNTI2_I3_FILT]) {
          signal[0] = interm;
          apply_filter(s, pat, signal);
          interm = signal[pat->ipar3[SYNTI2_I3_FILT]]; /*choose output*/
        }
#endif

#ifndef NO_DELAY
        /* mix also to a delay line. FIXME: s->delaypos is a counter
           like frame? FIXME: Was that everything-is-a-counter thing a
           good idea in the first place?*/
        dsamp += (int)(pat->fpar[SYNTI2_F_DLEN] * s->sr);
        s->delay[pat->ipar3[SYNTI2_I3_DNUM]][dsamp % DELAYSAMPLES] 
                 += pat->fpar[SYNTI2_F_DLEV] * interm;
#endif

        /* result is both in *signal and in interm (like before). Mix
         * (no stereo as of yet) FIXME: stereo mix, pan/panenv?.
         */
        buffer[iframe+ii] += interm;

      }

#ifndef NO_OUTPUT_SQUASH
      buffer[iframe+ii] = sin(buffer[iframe+ii]); /*Hack, but sounds nice*/
#endif

#ifndef NO_DELAY
      /* "erase the delay tape" so it can be used again. */
      for(dsamp=0; dsamp<NDELAYS; dsamp++){
        s->delay[dsamp][s->framecount.val % DELAYSAMPLES] = 0.f; 
      }
#endif
    }
  }
}
