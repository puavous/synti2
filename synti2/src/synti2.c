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


/* local subr. declared here */
static
void
synti2_fill_patches_from(synti2_patch *pat, const unsigned char *data);


/* The simple random number generator was posted on musicdsp.org by
 * Dominik Ries. Thanks a lot.
 */
static int RandSeed = 1;

/** Reads a MIDI variable length number. "Varlengths" are used in my
 * native song format for time deltas and fixed-point patch
 * parameters.
 */
static 
size_t
__attribute__ ((noinline))
varlength(const unsigned char * source, unsigned int * dest){
  size_t nread;
  unsigned char byte;
  *dest = 0;
  for (nread=1; nread<=4; nread++){
    byte = *source++;
    *dest += (byte & 0x7f);
    if ((byte & 0x80) == 0){
      return nread; 
    }
    else *dest <<= 7;
  }
#ifndef ULTRASMALL
  /* Longer than 4 bytes! Actually unexpected input which should be
     caught as a bug. */
  return 0;
#endif
}

/**
 * Assemble a 28-bit integer from 7 bit parts that were used
 * in SysEx transfer.
 *
 * (This is static and should not get compiled in the standalone
 * synth...)
 */
static
void
decode7b4(const unsigned char * source, unsigned int * dest){
  *dest = 
    ((source[0] & 0x7f) << 21)
    + ((source[1] & 0x7f) << 14)
    + ((source[2] & 0x7f) << 7)
    + ((source[3] & 0x7f) << 0);
}

static
void
__attribute__ ((noinline))
synti2_counter_retarget(counter *c, float nexttime, float nextgoal, unsigned int sr)
{
  c->aa = c->f;     /* init from current value */
  c->bb = nextgoal; /* aim to next goal*/
  if (nexttime <= 0.0f) {
    /* No time -> directly to nextgoal */
    c->f = nextgoal;
  } else {
    c->val = 0;
    c->delta = MAX_COUNTER / sr / nexttime;
  }
}

/** Adds an event to its correct location; makes no checks for empty
 * messages, i.e., assumes n >= 1. Also assumes that the pre-existing
 * events are ordered. This can be static for playback-mode, but in
 * real-time/compose mode the MIDI interface module needs to see this.
 *
 * Note: Worst case for inserting n events is O(n^2), but this happens
 * only if the insloc pointer is reset before calling this function
 * for each new event. This *is* linear time, if the pointers are not
 * updated in the middle of insertion. Naturally, this requires that
 * the events are added in their natural (time) order.
 *
 * This function makes a copy of the source data. The copy is stored
 * in the sequencer's own buffer.
 *
 * FIXME: Now that I'm using an internal event format in any case,
 * could I fix the length? Yes, I could... but would that be useful??
 * there are not so many different messages, and the bulk data message
 * (which is the only variable-length event) could contain a native
 * pointer to a memory area... maybe? This issue needs to be attended
 * while looking at the tool programs as well, and after the
 * implementation of controller ramps is finished, since that probably
 * dictates the maximum length of event data.
 */
#ifndef USE_MIDI_INPUT
static
#endif
void
synti2_player_event_add(synti2_synth *s, 
                        unsigned int frame, 
                        const byte_t *src, 
                        size_t n){
  synti2_player_ev *ev_new;
  byte_t *msg;

  while((s->seq.insloc->next != NULL)
        && (s->seq.insloc->next->frame <= frame)){
    s->seq.insloc = s->seq.insloc->next;
  }
  ev_new = s->seq.freeloc++;

#ifndef ULTRASMALL
  if (ev_new > (void*)s->seq.evpool+SYNTI2_MAX_SONGEVENTS){
    ev_new = --(s->seq.freeloc);
    s->seq.last_error_frame = frame;
    s->seq.last_error_type = SYNTI2_ERROR_OUT_OF_EVENT_SPACE;
    s->seq.last_error_info = *(unsigned int*)src;
  } 
#endif

  /*FIXME: Song data might get filled up and overflow lethally.. */
  msg = s->seq.data + s->seq.idata; /* Get next available data pool spot */

  /* Fill in the node: */
  ev_new->data = msg; /* We'll make a local copy here, see below.. */
  ev_new->next = s->seq.insloc->next;
  ev_new->frame = frame;
  ev_new->len = n;
  s->seq.insloc->next = ev_new;

  /* Actual copying of the data, and updating of the data pool top;
   * done this way to minimize code size (no local ints etc.)
   */
  for(;n>0;n--){
    *msg++ = *src++;
    s->seq.idata++;
  }
}

/** Merges a chunk, aka layer, to the list of events to be played,
 *  converting layers (stored) into messages (playable). Returns a
 *  pointer to one past end of read in input data, i.e., next byte.
 */
static
byte_t *
synti2_player_merge_chunk(synti2_synth *s, 
                          const byte_t *r, 
                          int n_events)
{
  char chan, type;
  int ii;
  unsigned int frame, tickdelta;
  const byte_t *par;
  byte_t msg[10]; /* FIXME: actual max. length!*/

  chan = *r++;
  type = *r++;
  par = r;
  frame = 0;
  s->seq.insloc = s->seq.evpool; /* Re-start merging from frame 0. */
  r += 2; /* add number of parameters to r! */
  /* Always two parameters.. makes reader code simpler with not too
   * much storage overhead.
   */

  for(ii=0; ii<n_events; ii++){
    r += varlength(r, &tickdelta);
    frame += s->seq.fpt * tickdelta;

    if (type == MISSS_LAYER_NOTES){
      /* Produce a 'Note on' message in our internal midi-like format. */
      msg[0] = MISSS_MSG_NOTE;
      msg[1] = chan;
      msg[2] = (par[0]==0xff) ? *r++ : par[0];
      msg[3] = (par[1]==0xff) ? *r++ : par[1];
      synti2_player_event_add(s, frame, msg, 4);
    }
#ifndef NO_CC
    /* FIXME: In fact, if NO_CC is used, then notes are the only layer
     * type.. Could optimize away the whole layer byte in that case,
     * maybe... but then again, that might be just excess...
     */
    else if (type == MISSS_LAYER_CONTROLLER_RAMPS) {
      /* Not yet implemented. FIXME: implement. Of course, this is
       * mostly a task for the tool programs, and the fpar data format
       * should be fixed before attending this one.
       */
    }
#endif
#ifndef ULTRASMALL
    else {
      s->seq.last_error_frame = frame;
      s->seq.last_error_type = SYNTI2_ERROR_UNKNOWN_LAYER;
      s->seq.last_error_info = type;
    }
#endif
  }
  return (byte_t*) r;
}

/** Load and initialize a song. Assumes a freshly created player object!*/
static
void
synti2_player_init_from_misss(synti2_synth *s, const byte_t *r)
{
  unsigned int chunksize;
  unsigned int uspq;
  /* Initialize an "empty" "head event": (really necessary?) */
  s->seq.freeloc = s->seq.evpool + 1;

  s->seq.playloc = s->seq.evpool; /* This would "rewind" the song */

  /* TODO: Now we use the standard MIDI tempo unit of MSPQN. For our
   * 4k purposes, we might not need tpq at all. Just give us
   * microseconds per tick, and we'll be quite happy here and try to
   * match it approximately by framecounts... TODO: Think about
   * accuracy vs. code size once more in here(?).
   */
  r += varlength(r, &(s->seq.tpq));  /* Ticks per quarter note */
  r += varlength(r, &uspq);       /* Microseconds per quarter note */
  s->seq.fpt = 
    ((float)uspq / s->seq.tpq) 
    / (1000000.0f / s->seq.sr); /* frames-per-tick */
  for(r += varlength(r, &chunksize); chunksize > 0; r += varlength(r, &chunksize)){
    r = synti2_player_merge_chunk(s, r, chunksize); /* read a chunk... */
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
  float freqf;

  memset(s, 0, sizeof(s));     /* zero */

  /* Initialize the player module. (Not much to be done...) */
  s->seq.sr = sr;
  s->sr = sr;
  s->framecount.delta = 1;

#ifndef ULTRASMALL
  if (songdata != NULL) 
    synti2_player_init_from_misss(s, songdata);

  if (patchdata != NULL)
    synti2_fill_patches_from(s->patch, patchdata);
#else
  /* In "Ultrasmall" mode, we trust the user to provide all data.*/
#ifndef EXTREME_NO_SEQUENCER
  /* Extreme hackers may have made a smaller sequencer/music generator:)*/
  synti2_player_init_from_misss(s, songdata);
#endif
  /* Patches are to be made with some patch editor, though.*/
  synti2_fill_patches_from(s->patch, patchdata);
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
  /* slightly inaccurate notes but without powf(). Many bytes shorter exe. */
  /* My ears find the result very tolerable; I don't know about others... */
  freqf = 8.175798915643707f;
  for(ii=0;ii<128;ii++){
    s->note2freq[ii] = freqf;
    freqf *= 1.0594630943592953f;
  }
#if 0
  for(ii=0; ii<128; ii++){
    s->note2freq[ii] = 440.0f * powf(2.0f, ((float)ii - 69.0f) / 12.0f );
  }
#endif

  for(ii=0; ii<WAVETABLE_SIZE; ii++){
    /*s->wave[ii] = sin(2*M_PI * ii/(WAVETABLE_SIZE-1));*/

    t = (float)ii/(WAVETABLE_SIZE-1);
    s->rise[ii] = t; 
    s->wave[0][ii] = sin(2*(float)M_PI*t);
#ifndef NO_EXTRA_WAVETABLES
    t = (float)ii/(WAVETABLE_SIZE-1);
    for(wt=1; wt<NHARM; wt++){
      s->wave[wt][ii] = s->wave[wt-1][ii] 
        + 1.f/(wt+1) * sinf(2*(float)M_PI * (wt+1) * t);
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
      s->estage[voice][ie] = RELEASESTAGE;
      s->eprog[voice][ie].delta = 0;
      /*s->eprog[voice][ie].val = 0;*/ /* I think this was unnecessary.. */
    }
    return; /* Note off done. */
  }
#endif
 
  /* note on */
#ifndef NO_VELOCITY
  s->velocity[voice] = vel;
#endif

  s->note[voice] = note;

#ifndef NO_LEGATO
  synti2_counter_retarget(&(s->pitch[voice]),
                          s->patch[voice].fpar[SYNTI2_F_LEGLEN],
                          note, s->sr);
#endif

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

/* TODO: This too much of a hack?*/
static
#include "synti2_fdec.c"


/** Reads patches (can be many) from off-line data, generated by the
 *  tool programs. This is not compatible with the MIDI SysEx
 *  world. 
 */
static
void
synti2_fill_patches_from(synti2_patch *pat, const unsigned char *data)
{
  int ir;
  unsigned int intval;
  size_t nbytes;
  for(; *data != 0xf7; pat++){
    for(ir=0;ir<SYNTI2_I3_NPARS; ir+=2){
      pat->ipar3[ir] = *data >> 4;
      pat->ipar3[ir+1] = (*data++) & 0xf;
    }

    for (ir=0; ir<SYNTI2_F_NPARS; ir++){
      /*printf("Reading value %08lx: (%02x %02x) ", data, data[0], data[1]);*/
      nbytes = varlength(data, &intval);
      data += nbytes;
      pat->fpar[ir] = synti2_decode_f(intval);
      /*printf("%04x len=%d %03d <- %f\n", intval, nbytes, ir, pat->fpar[ir]);*/
      /*fflush(stdout);*/
    }
  }
}

#ifndef NO_SYSEX_RECEIVE
/** 
 * Receive SysEx data. (Convenient for sound editing, but not
 * necessary for the stand-alone 4k synth. Hence, this is compiled
 * only when real-time midi input is used at compose time. Also, this
 * function is not space-restricted.)
 *
 * The MIDI SysEx header and footer things have been dealt with prior
 * to entry, and this receives just the bulk data.
 *
 * FIXME: Do I want SysEx to be stored in the playback data? At the
 * moment, I think not.. but be sure and make it a synti2 feature or
 * decide not to ever do that.
 *
 */
static
void
synti2_do_receiveSysexData(synti2_synth *s, const byte_t * data){
  int opcode, ipat, ir;
  unsigned int encoded_fval;

  /* Data header: what to do, for which parameter of which patch:*/
  opcode = *data++;
  ir = *data++;
  ipat = *data++;

  if (opcode==MISSS_SYSEX_SET_3BIT) {
    /* Receive one parameter at location (patch,i3par_index) */
    s->patch[ipat].ipar3[ir] = *data;
  } else if (opcode==MISSS_SYSEX_SET_F){
    /* Receive one float parameter at location (patch,fpar_index) */
    decode7b4(data, &encoded_fval);
    s->patch[ipat].fpar[ir] = synti2_decode_f(encoded_fval);
  } 
#ifndef ULTRASMALL
  else {
    s->last_error_frame = s->framecount.val;
    s->last_error_type = SYNTI2_ERROR_UNKNOWN_OPCODE;
    s->last_error_info = opcode;
  }
#endif
}
#endif

#ifndef EXTREME_NO_SEQUENCER
/** 
 * (Space-limited) Handles input that comes from the stored list of
 * song events; forwards control data for the synth and keeps track of
 * song position. All necessary conversion and event timing work has
 * been done by either a real-time midi adapter or the sequencer's
 * song loader, and we can just go with the pre-determined flow of our
 * own internal messages here.
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
  const byte_t *msgbuf;

  while((s->seq.playloc->next != NULL) 
        && (s->seq.playloc->next->frame <= upto_frames )) {
    s->seq.playloc = s->seq.playloc->next;

    msgbuf = s->seq.playloc->data;
    if (msgbuf[0] == MISSS_MSG_NOTE){
      synti2_do_noteon(s, msgbuf[1], msgbuf[2], msgbuf[3]);

#ifndef NO_CC
    } else if (msgbuf[0] == MISSS_MSG_RAMP){
      /* A ramp message contains controller number, time, and destination value: */
      synti2_counter_retarget(&(s->contr[msgbuf[1]][msgbuf[2]]),
                              (*((float*)(msgbuf+3))) /*in given time */,
                              (*((float*)(msgbuf+3+sizeof(float)))) /*to next*/,
                              s->sr);
#endif

#ifndef NO_SYSEX_RECEIVE
    } else if (msgbuf[0] == MISSS_MSG_DATA){
      /* Used only in compose mode (patch editor requires this) */
      synti2_do_receiveSysexData(s, msgbuf+1);
#endif
    } 

#ifndef ULTRASMALL
    else {
      s->last_error_frame = s->framecount.val;
      s->last_error_type = SYNTI2_ERROR_UNKNOWN_MESSAGE;
      s->last_error_info = msgbuf[0];
    }
#endif
  }
#ifndef ULTRASMALL
  s->seq.frames_done = upto_frames; /* So far used only by rt jack midi.. */
#endif
}
#endif


/* Advance the oscillator and envelope counters. Consider using the
 * xmms assembly for these? The only difference between oscillators
 * and envelope stages is the clamping. NOTE: I'm using the order of
 * the counters {framecount, osc1, osc2, ..., oscN, ev1, ev2, ...,
 * evM} to know which counters need clamping.
 *
 * FIXME: See if there would be size improvements from putting the
 * counters inside parts (and leave out what seems to be the only
 * global one, framecount)
 *
 * TODO: (Before future projects with re-designed synths) Evaluate if
 * this everything-is-a-counter thing was a good idea.
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
     * TODO: See if there was a significant performance hit (?).
     */
    ind = c->val >> COUNTER_TO_TABLE_SHIFT;
    c->fr = s->rise[ind];
    c->f = (1.0f-c->fr) * c->aa + c->fr * c->bb; 
  }
}


/** 
 * Updates envelope stages as the patch data dictates. 
 * 
 * A looping 5-knee envelope is used. This is my final choice for the
 * synti2 synth, mostly because I feel that it is simple in a
 * musician-friendly way, which is one leading design principle for
 * synti2, even though it might conflict with the size
 * constraints. Another reason is simply that it was in the original
 * plan, and I don't want to wander too much off from it, even if
 * better ideas may have been appearing during the project.
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
 * sound design, when size limitations need not to be considered.
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
         * Re-ordering of storage? Part-wise storage ordering seems to
         * appear in many of these remaining questions... so that may
         * be worth trying.
         */
        if ((s->estage[iv][ie] == 1) && (s->sustain[iv] != 0)){
          s->estage[iv][ie] += pat->ipar3[(SYNTI2_I3_ELOOP1-1)+ie]; /*-1*/
#ifndef NO_SAFETY
          if ((iter_watch++) > 5) s->sustain[iv] = 0; /* stop ifinite loop. */
#endif
        }
#endif

        /* Move these comments to a documentation file...*/
        /* No time -> skip envelope knee. Force value to the new
         * level (next goal). Delta remains at 0, so we may skip
         * many.
         */
        /* NOTE: There will be a value jump here, so be
         * careful when creating patches... The reason for this
         * whole thing was to make it possible to use less knees, if
         * 5 knees is not necessary. As an after-thought, the whole
         * envelope thing could have been made with less glitches,
         * but that remains as a to-do for some later project. This
         * envelope skip-and-jump is now a final feature of synti2.
         */
        nexttime = pat->fenvpar[ipastend - s->estage[iv][ie] * 2 + 0];
        nextgoal = pat->fenvpar[ipastend - s->estage[iv][ie] * 2 + 1];
        synti2_counter_retarget(&(s->eprog[iv][ie]), nexttime, nextgoal, s->sr);
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

    for (iosc=0; iosc<NOSCILLATORS; iosc++){
      /* Pitch either from legato counter or directly from note: */
#ifndef NO_LEGATO
      notemod = s->pitch[iv].f;
#else
      notemod = s->note[iv];
#endif

#ifndef NO_PITCH_SCALING
      /* Optional pitch scale */
      notemod *= (1.f + pat->fpar[SYNTI2_F_PSCALE]);
#endif

#ifndef NO_PITCH_ENV
      /* Optional pitch envelope */
      notemod += s->eprog[iv][pat->ipar3[SYNTI2_I3_EPIT1+iosc]].f;
#endif

#ifndef NO_DETUNE
      /* Optional detune */
      notemod += pat->fpar[SYNTI2_F_DT1 + iosc];    /* "coarse" */
      notemod += pat->fpar[SYNTI2_F_DT1F + iosc];   /* "fine"   */
#endif

#ifndef NO_PITCH_BEND
      /* Pitch bend for each oscillator. Different values between
       * oscillators allow some weird effects; normal synth default
       * would be +2 notes on each oscillator. The cube_thing 4k intro
       * actually got some bytes smaller after adding the 3 additional
       * parameters, so I think my pitch bend system is now final like
       * this.
       */
      notemod += pat->fpar[SYNTI2_F_PBVAL] * pat->fpar[SYNTI2_F_PBAM+iosc];
#endif

      note = notemod; /* should make a floor (does it? check spec)*/
      interm = (1.0f + 0.05946f * (notemod - note)); /* +cents.. */
      freq = interm * s->note2freq[note]; /* could be note2delta[] FIXME: think.*/
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
                         float fenv,
                         float *store){
#define FIL_IN 0
#define FIL_LP 1
#define FIL_BP 2
#define FIL_HP 3
#define FIL_NF 4

  float f,q;
  /* FIXME: Check the value range once again..? 
   *
   * FIXME: Could there be a nicer filter?
   *
   * FIXME: The computation becomes unstable with some combinations of
   * cutoff and resonance.
   */

  f = 1000.f * fenv / s->sr;
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
  int iframe, ii, iv, id;
  int iframeL;
  int iosc;
  float interm;
  int wtoffs;
  synti2_patch *pat;

  float *sigin;  /* Input signal table during computation. */
  float *signal; /* Output signal destination during computation. */

#ifndef NO_CC
  int ccdest;
#endif
#ifndef NO_STEREO
  float pan;
  int iframeR;
#endif

  for (iframe=0; iframe<nframes; iframe++){
    iframeL=2*iframe;
#ifndef NO_STEREO
    iframeR=iframeL+1;
#endif

#ifndef EXTREME_NO_SEQUENCER
    /* If they wish to generate do_note_on() without my beautiful
       sequencer interface, by all means let them ... */
    synti2_handleInput(s, s->framecount.val + iframe);
#endif
    synti2_updateEnvelopeStages(s); /* move on if need be. */
    synti2_updateFrequencies(s);    /* frequency upd. */
    
    /* Oscillators and audio generation. */
      
    /* Could I put all counter upds in one big awesomely parallel loop? */
    synti2_evalCounters(s);  /* .. apparently yes ..*/
      
    /* Sound output. Getting more realistic as we speak... */
    buffer[iframeL] = 0.0f;
#ifndef NO_STEREO
    buffer[iframeR] = 0.0f;
#endif
      
    for(iv=0;iv<NPARTS;iv++){
      pat = s->patch + iv;

#ifndef NO_CC
      for (ii=0;ii<NCONTROLLERS;ii++){
        ccdest = pat->fpar[SYNTI2_F_CDST1+ii];
        pat->fpar[ccdest] = s->contr[iv][ii].f;
      }
#endif
      
      sigin  = signal = &(s->outp[iv][0]);

#ifndef NO_FEEDBACK
      /* Sort of like a "feedback" operator. */

      /* TODO: OK.. this can do some pretty weird/nasty stuff, so I'll
         let it be here. Not my favorite feature, at least when it is
         crappy like this (i.e., modulates every oscillator by
         default..) Consider a different implementation, if this seems
         to become useful.. at all... But the oscillator and envelope
         code and sound parameters may need some rethinking in that
         case (?) ... should make the delay line contents available in
         sigin, and that's all, I guess.. looks easy enough?
         Yei. FIXME: Do it the easy way instead of this initial
         attempt. */
      id = pat->ipar3[SYNTI2_I3_FBACK];
      if (id>0){
        dsamp = s->framecount.val;
        *signal = s->delay[id-1][dsamp % DELAYSAMPLES]
          * pat->fpar[SYNTI2_F_DINLV1-1+id];
      }
#endif
      
      for(iosc = 0; iosc < NOSCILLATORS; iosc++){
        signal++; /* Go to next output slot. */
        wtoffs = (unsigned int)
          ((s->c[iv*NOSCILLATORS+iosc].fr 
            + sigin[pat->ipar3[SYNTI2_I3_FMTO1+iosc]])  /* phase modulator */
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
        /* Optional velocity sensitivity */
        if (pat->ipar3[SYNTI2_I3_VS1+iosc]){
          interm *= s->velocity[iv] / 127.f;
        }
#endif
        *signal = interm;
      }
      
      /* Optional additive noise after FM operator synthesis: */
      /* TODO: If noise was a wavetable like other oscillators, it
       * would not require additional code here. But elsewhere it
       * would.. worth trying or not? Probably not.
       */
#ifndef NO_NOISE
      RandSeed *= 16807;
      interm += s->eprog[iv][pat->ipar3[SYNTI2_I3_EAMPN]].f 
#ifndef NO_VELOCITY
        * ((pat->ipar3[SYNTI2_I3_VSN])?(s->velocity[iv] / 127.f) : 1.f)
#endif
        * pat->fpar[SYNTI2_F_LVN]       /*noise gain*/
        * (float)RandSeed * 4.6566129e-010f; /*noise*/
#endif
      
#ifndef NO_DELAY
      /* Additive mix from delay lines. */
      dsamp = s->framecount.val;
      for (id = 0; id < pat->ipar3[SYNTI2_I3_NDIN]; id++){
        interm += s->delay[id][dsamp % DELAYSAMPLES]
          * pat->fpar[SYNTI2_F_DINLV1+id];
      }
#endif
      
#ifndef NO_FILTER
      /* Skip for faster computation. Should do the same for delays! */
      if(pat->ipar3[SYNTI2_I3_FILT]) {
        signal[0] = interm;

        float fenv = pat->fpar[SYNTI2_F_FFREQ];

#ifndef NO_FILTER_ENVELOPE
        /* Optionally read cutoff frequency from an envelope. */
        fenv += s->eprog[iv][pat->ipar3[SYNTI2_I3_EFILT]].f;
#endif

        apply_filter(s, pat, fenv, signal);
        interm = signal[pat->ipar3[SYNTI2_I3_FILT]]; /*choose output*/
      }
#endif
      
#ifndef NO_DELAY
      /* mix also to the delay lines.*/
      for (id = 0; id < pat->ipar3[SYNTI2_I3_NDOUT]; id++){
        dsamp = s->framecount.val + (int)(pat->fpar[SYNTI2_F_DLEN1+id] * s->sr);
        s->delay[id][dsamp % DELAYSAMPLES] 
          += pat->fpar[SYNTI2_F_DLEV1+id] * interm;
      }
#endif
      
      /* result is both in *signal and in interm (like before). Main
       * mix in either mono or stereo. */
#ifdef NO_STEREO
      /* We only output to the left channel. */
      buffer[iframeL]   += pat->fpar[SYNTI2_F_MIXLEV] * interm;
#else
      /* To cut down computations, panning increases volume ([0,2]): */
      pan = pat->fpar[SYNTI2_F_MIXPAN];
#ifndef NO_PAN_ENVELOPE
      pan += s->eprog[iv][pat->ipar3[SYNTI2_I3_EPAN]].f;
#endif
      buffer[iframeL] += pat->fpar[SYNTI2_F_MIXLEV] * interm * (1.f-pan);
      buffer[iframeR] += pat->fpar[SYNTI2_F_MIXLEV] * interm * (1.f+pan);
#endif
    }
    
#ifndef NO_OUTPUT_SQUASH
    buffer[iframeL]   = sin(buffer[iframeL]); /*Hack, but sounds nice*/
#ifndef NO_STEREO
    buffer[iframeR] = sin(buffer[iframeR]); /*Hack, but sounds nice*/
#endif
#endif
    
#ifndef NO_DELAY
    /* "erase the delay tape" so it can be used again. Must be done
       after each channel has read the current contents. */
    for(id=0; id<NDELAYS; id++){
      s->delay[id][s->framecount.val % DELAYSAMPLES] = 0.f; 
    }
#endif
  }
}
