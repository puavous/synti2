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
synti2_fill_patches_from(synti2_synth *s, const unsigned char *data);

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
 * Not size-limited. (This is static and should not get compiled in
 * the standalone synth...)
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

/**
 * Adds an event to its correct location; event data is of fixed size,
 * and this will copy the maximum number of bytes from the
 * source. Assumes that the pre-existing events are ordered.
 *
 * Note: Worst case for inserting n events is O(n^2), but this happens
 * only if the insloc pointer is reset before calling this function
 * for each new event. This *is* linear time, if the pointers are not
 * updated in the middle of insertion. Of course, this requires that
 * the events are added in their natural (time) order.
 *
 * This can be static for playback-mode, but in real-time/compose mode
 * the MIDI interface module needs to see this.
 *
 */
#ifndef USE_MIDI_INPUT
static
#endif
void
synti2_player_event_add(synti2_synth *s, 
                        unsigned int frame, 
                        const byte_t *src){
  synti2_player_ev *ev_new;
  byte_t *msg;
  unsigned int i;
  
  while((s->seq.insloc->next != NULL)
        && (s->seq.insloc->next->frame <= frame)){
    s->seq.insloc = s->seq.insloc->next;
  }
  ev_new = s->seq.freeloc++;
  
#ifndef ULTRASMALL
  /* Compose-mode checks for storage size. */
  if (ev_new > (s->seq.evpool+SYNTI2_MAX_SONGEVENTS)){
    ev_new = --(s->seq.freeloc);
    s->seq.last_error_frame = frame;
    s->seq.last_error_type = SYNTI2_ERROR_OUT_OF_EVENT_SPACE;
    s->seq.last_error_info = *(unsigned int*)src;
    return; /* No need to proceed.. */
  } 
#endif
  
  msg = ev_new->data;
  ev_new->next = s->seq.insloc->next;
  ev_new->frame = frame;
  s->seq.insloc->next = ev_new;
  
  /* Actual copying of the data. Always max size.*/
  for(i=0;i<SYNTI2_MAX_EVDATA;i++){
    *msg++ = *src++;
  }
}

/* TODO: This too much of a hack? */
static
#include "synti2_fdec.c"

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
  unsigned int frame, tickdelta, intval;
  const byte_t *par;
  byte_t msg[SYNTI2_MAX_EVDATA];
  
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
      synti2_player_event_add(s, frame, msg);
    }
#ifndef NO_CC
    else if (type == MISSS_LAYER_CONTROLLER_RAMPS) {
      msg[0] = MISSS_MSG_RAMP;
      msg[1] = chan;
      msg[2] = par[0]; /*cont_index*/
      
      /* Hmm.. Come to think of it.. All along, I'm sort of assuming
         byte is 8 bits and float is 4 bytes.. Will there be trouble
         if this is not the case.. Hmmm... and what effects would
         there be from alignment.. */

      r += varlength(r, &intval); /* time */
      *((float*)(msg+3)) = synti2_decode_f(intval);
      r += varlength(r, &intval); /* dest. value */
      *(((float*)(msg+3))+1) = synti2_decode_f(intval);

      /*printf("Adding ramp: Time: %7.3fs Target: %7.3f\n",
       *((float*)(&msg[3])), *((float*)(&msg[3])+1));*/
      
      /* Then two native floats float out, and the header piece:. */
      synti2_player_event_add(s, frame, msg);
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
    / (1000000.0f / s->sr); /* frames-per-tick */
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
  float deltaf;
  
  memset(s, 0, sizeof(s));     /* zero */
  
  /* Initialize the player module. (Not much to be done...) */
  s->sr = sr;
  
#ifndef ULTRASMALL
  if (songdata != NULL) 
    synti2_player_init_from_misss(s, songdata);
  
  if (patchdata != NULL)
    synti2_fill_patches_from(s, patchdata);
#else
  /* In "Ultrasmall" mode, we trust the user to provide all data.*/
#ifndef EXTREME_NO_SEQUENCER
  /* Extreme hackers may have made a smaller sequencer/music generator:)*/
  synti2_player_init_from_misss(s, songdata);
#endif
  /* Patches are to be made with some patch editor, though.*/
  synti2_fill_patches_from(s, patchdata);
#endif
  
  /* Initialize the rest of the synth. */
  
  /* Create a note-to-frequency look-up table (cents need interpolation). 
   * TODO: Which is more efficient / small code: linear interpolation 
   * on every eval or make a lookup table on the cent scale...
   *
   * TODO: I could have negative notes for LFO's and upper octaves to
   * the ultrasound for weird aliasing/noise kinda stuff... just make
   * a bigger table here and let the pitch envelopes go beyond the
   * beyond!
   *
   * Notes: The iterative multiplication produces slightly inaccurate
   * notes but without powf(). Many bytes shorter exe. My ears find
   * the result very tolerable; I don't know about others... Compute
   * counter deltas directly; no need to know frequencies while
   * playing.
   */
  deltaf = 8.175798915643707f;
  for(ii=0;ii<128;ii++){
    s->note2freq[ii] = deltaf;
    s->note2delta[ii] = ((MAX_COUNTER) / s->sr) * deltaf ;
    deltaf *= 1.0594630943592953f;
  }

  /* Accurate, but need to link powf():
   *   for(ii=0; ii<128; ii++){
   *     s->note2freq[ii] = 440.0f * powf(2.0f, ((float)ii - 69.0f) / 12.0f );
   *   }
   */
  
  for(ii=0; ii<WAVETABLE_SIZE; ii++){
    /*s->wave[ii] = sin(2*M_PI * ii/(WAVETABLE_SIZE-1));*/
    
    t = (float)ii/(WAVETABLE_SIZE-1);
    s->wave[0][ii] = sin((float)(2*M_PI)*t);
#ifndef NO_EXTRA_WAVETABLES
    t = (float)ii/(WAVETABLE_SIZE-1);
    for(wt=1; wt<NHARM; wt++){
      s->wave[wt][ii] = s->wave[wt-1][ii] 
        + 1.f/(wt+1) * sinf((float)(2*M_PI) * (wt+1) * t);
    }
#endif
  }
}


/** 
 * Note on OR note off (upon velocity == 0).
 *
 * Note or velocity must not jump at a note-off, when the parameters
 * are both likely to be different from what they were at note-on.
 *
 * Velocity is MIDI-like, i.e., range 0-127, and 0 means note-off
 * instead of note-on with zero velocity. Velocity handling is
 * optional code, but the zero/non-zero encoding is always used here
 * for note on/note off.
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
  s->voi[voice].sustain = vel;
#endif
  
#ifndef NO_NOTEOFF
  if (vel==0){
    for (ie=0; ie<=NENVPERVOICE; ie++){
      s->voi[voice].estage[ie] = RELEASESTAGE;
      s->voi[voice].eprog[ie].delta = 0;
    }
    return; /* Note off done. */
  }
#endif
  
  /* note on */
#ifndef NO_VELOCITY
  s->voi[voice].velocity = vel;
#endif
  
  s->voi[voice].note = note;
  
#ifndef NO_LEGATO
  synti2_counter_retarget(&(s->voi[voice].pitch),
                          s->voi[voice].patch.fpar[SYNTI2_F_LEGLEN],
                          note, s->sr);
#endif
  
  /* Trigger all envelopes. Just give a hint to the evaluator function.. */
  for (ie=0; ie<=NENVPERVOICE; ie++){
    s->voi[voice].estage[ie] = TRIGGERSTAGE;
    s->voi[voice].eprog[ie].delta = 0;
  }
  
#ifdef DO_RESET_PHASE
  /* Adds to code size and produces clicks when note tail is still
   * sounding, but makes each note start with deterministic (zero)
   * phase difference between oscillators.
   */
  for(ie=0;ie<NOSCILLATORS;ie++) {
    s->voi[voice].c[ie].val = 0;
  }
#endif

}

/** Reads patches (can be many) from off-line data, generated by the
 *  tool programs. This is not compatible with the MIDI SysEx
 *  world. 
 */
static
void
synti2_fill_patches_from(synti2_synth *s, const unsigned char *data)
{
  int ir;
  unsigned int intval;
  size_t nbytes;
  unsigned int ipat;
  synti2_patch *pat;
  
  for(ipat=0; *data != 0xf7; ipat++){
    pat = &(s->voi[ipat].patch);
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
 * TODO: Do I want SysEx to be stored in the playback data? Maybe...
 * I don't find many reasons why it shouldn't be possible. No big bulk
 * messages, but for convenience, some simple "set one parameter"-type
 * of messages would make the stand-alone synth a lot more
 * flexible.. Maybe in some cases some song data size optimization
 * could be done by means of SysEx (?). But I deem this a very low
 * priority now. To be thought about later if I happen to have the
 * time and interest.
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
    s->voi[ipat].patch.ipar3[ir] = *data;
  } else if (opcode==MISSS_SYSEX_SET_F){
    /* Receive one float parameter at location (patch,fpar_index) */
    decode7b4(data, &encoded_fval);
    s->voi[ipat].patch.fpar[ir] = synti2_decode_f(encoded_fval);
  } 
#ifndef ULTRASMALL
  else {
    s->last_error_frame = s->framecount;
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
      synti2_counter_retarget(&(s->voi[msgbuf[1]].contr[msgbuf[2]]),
                              (*((float*)(msgbuf+3))) /*in given time */,
                              (*((float*)(msgbuf+3+sizeof(float)))) /*to next*/,
                              s->sr);
#endif
      
#ifndef NO_SYSEX_RECEIVE
    } else if (msgbuf[0] == MISSS_MSG_DATA){
      /* Used only in compose mode (patch editor requires this) */
      synti2_do_receiveSysexData(s, msgbuf+1);
#endif
    
#ifndef ULTRASMALL
    } else {
      s->last_error_frame = s->framecount;
      s->last_error_type = SYNTI2_ERROR_UNKNOWN_MESSAGE;
      s->last_error_info = msgbuf[0];
#endif
    }
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
 */
static 
void
synti2_evalCounters(synti2_voice *v){
  counter *c;
  unsigned int ic, icmax;
#ifndef NO_LEGATO
  icmax = 1 + NOSCILLATORS + NENVPERVOICE+1 + NCONTROLLERS;
#else
  icmax = NOSCILLATORS + NENVPERVOICE+1 + NCONTROLLERS;
#endif
  for(ic=0;ic<icmax;ic++){
    c = v->c+ic;
    if (c->delta == 0) {continue;}  /* stopped.. not running*/
    c->detect = c->val; 
    c->val += c->delta;  /* Count 0 to MAX*/
    
    if (ic > NOSCILLATORS) { /* Clamp only latter half of counters. */
      if (c->val < c->detect){         /* Detect overflow*/
        c->val = MAX_COUNTER;          /* Clamp. */
        c->delta = 0;                  /* Stop after cycle*/
      }
    }
    
    c->fr = c->val / ((float)MAX_COUNTER);
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
 */
static
void
synti2_updateEnvelopeStages(synti2_synth *s,
                            synti2_voice *v, 
                            synti2_patch *pat){
  int ie, ipastend;
  float nextgoal, nexttime;
  
#ifndef NO_SAFETY
  /* Looping over all-zero times produces an infinite iteration which
   * is not nice, especially in a real-time audio system. So, there is
   * an optional safety mechanism for real-time work. In a
   * pre-determined 4k setting you'd know that the patches contain no
   * zero-time loops, and turn off this watchdog.
   */
  int iter_watch;
#endif
  
  /* Envelope 0 is never touched. Therefore it yields a constant
   * 0. I don't know if this hack is very useful, but it is in its
   * own way very beautiful, so I leave it untouched. That's why the
   * loop goes through indices 1..NENVPERVOICE (in reverse order):
   */
  for (ie=NENVPERVOICE; ie>0; ie--){
    if (v->estage[ie] == 0) continue; /* skip untriggered. */
    
    /* reverse order in the stages, so a bit awkward indexing. */
    ipastend = SYNTI2_F_ENVS + ie * SYNTI2_NENVD;
    
#ifndef NO_SAFETY
    iter_watch = 0;
#endif
    /* Find next non-zero-timed knee (or end). delta==0 on a
       triggered envelope means endclamp/noteon. */
    while ((v->eprog[ie].delta == 0) && ((--v->estage[ie]) > 0)){
#ifndef NO_LOOPING_ENVELOPES
      if ((v->estage[ie] == 1) && (v->sustain != 0)){
        v->estage[ie] += pat->ipar3[(SYNTI2_I3_ELOOP1-1)+ie]; /*-1*/
#ifndef NO_SAFETY
        if ((iter_watch++) > 5){
          v->sustain = 0; /* stop ifinite loop and mark error. */
          s->seq.last_error_frame = s->framecount;
          s->seq.last_error_type = SYNTI2_ERROR_INVALID_LOOPING_ENV;
          s->seq.last_error_info = ie;
        }
#endif
      }
#endif
      
      /* No time -> skip envelope knee. Force value to the new level
       * (next goal). Delta remains at 0, so we may skip many. There
       * could be clicks and stuff.. More in the docs.
       */
      nexttime = pat->fpar[ipastend - v->estage[ie] * 2 + 0];
      nextgoal = pat->fpar[ipastend - v->estage[ie] * 2 + 1];
      synti2_counter_retarget(&(v->eprog[ie]), nexttime, nextgoal, s->sr);
    }
  } 
}

static
float
interpolate_note(const float* lookup, float value){
  int basenote;
  float interm;
  basenote = value; /* should make a floor (does it? check spec)*/
  interm = (1.0f + 0.05946f * (value - basenote)); /* +cents.. */
  return interm * lookup[basenote];
}

/** Converts note values to counter deltas. */
static
void
synti2_updateFrequencies(const synti2_synth *s,
                         synti2_voice *v,
                         const synti2_patch *pat){
  int iosc;
  float notemod;
  
  /* Frequency computation... where to put it after all? Well.. it
   * seems to have ended up in this spot.
   */
  
  for (iosc=0; iosc<NOSCILLATORS; iosc++){
    /* Pitch either from legato counter or directly from note: */
#ifndef NO_LEGATO
    notemod = v->pitch.f;
#else
    notemod = v->note;
#endif
    
#ifndef NO_PITCH_SCALING
    /* Optional pitch scale */
    notemod *= (1.f + pat->fpar[SYNTI2_F_PSCALE]);
#endif
    
#ifndef NO_PITCH_ENV
    /* Optional pitch envelope */
    notemod += v->eprog[pat->ipar3[SYNTI2_I3_EPIT1+iosc]].f;
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
    
    /* Interpolate between two notes (deltas) in a look-up table: */
    v->c[iosc].delta = 
      interpolate_note(s->note2delta,notemod);
    
#ifndef NO_FILTER_PITCH_FOLLOW
    /* Store effective note for filter pitch follow: */
    v->effnote[iosc] = notemod;
#endif
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
 *    cutoff = cutoff freq in Hz  (parameter fenv)
 *    fs = sampling frequency //(e.g. 44100Hz)
 *    f = 2*sin(M_PI*cutoff / fs)
 *    q = resonance/bandwidth [0 < q <= 1]  most res: q=1, less: q=0
 *    scale = q  OR scale = sqrt(q)?? 
 *
 *  Algorithm:
 *    f = frqHz / sampleRate*4   (so Hz depends on sample rate!
 *                                should check on 44.1kHz systems
 *                                to see if songs sound too bad..
 *                                so far all tests have been 48kHz)
 *    low = low + f * band;
 *    high = scale * input - low - q*band;   was scale=sqrt(q) 
 *    band = f * high + band;
 *    (save low and band for next round.)
 */
static void apply_filter(synti2_synth *s, 
                         float fenv,
                         float renv,
                         float *store){
#define FIL_IN 0
#define FIL_LP 1
#define FIL_BP 2
#define FIL_HP 3
#define FIL_NF 4
  
  float f,q;
  
  f = 2.f*sinf(M_PI*(fenv) / s->sr);
  
  q = 1.0f - renv;
  
  store[FIL_LP] += f * store[FIL_BP];
  store[FIL_HP]  = q * store[FIL_IN] - store[FIL_LP] - (q*q) * store[FIL_BP];
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
  int iframe, ii, iv, id, iosc;
  int iframeL;
  float interm;
  int wtoffs;
  synti2_patch *pat;
  synti2_voice *voi;
  counter *eprog;
  
  float *sigin;  /* Input signal table during computation. */
  float *signal; /* Output signal destination during computation. */
  
#ifndef NO_DELAY
  float dlev;
#endif
#ifndef NO_CC
  int ccdest;
#endif
#ifndef NO_FILTER
  float fenv, renv;
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
    synti2_handleInput(s, s->framecount + iframe);
#endif
    
    s->framecount++;
    
    /* Sound output begins. Mix channels additively to a cleared frame:*/
    buffer[iframeL] = 0.0f;
#ifndef NO_STEREO
    buffer[iframeR] = 0.0f;
#endif
    
    for(iv=0;iv<NPARTS;iv++){
      voi = &(s->voi[iv]);
      pat = &(voi->patch);
      eprog = voi->eprog;
      
      /* Update status of everything on this voice. */
      synti2_updateEnvelopeStages(s, voi, pat);
      synti2_updateFrequencies(s, voi, pat);
      synti2_evalCounters(voi);
      
#ifndef NO_CC
      for (ii=0;ii<NCONTROLLERS;ii++){
        ccdest = pat->fpar[SYNTI2_F_CDST1+ii];
        pat->fpar[ccdest] = voi->contr[ii].f;
      }
#endif

      /* Start creating the sound from this voice. */
      sigin  = signal = &(voi->outp[0]);
      
#ifndef NO_DELAY
      /* Additive mix from delay lines. */
      interm = 0.0f;
      dsamp = s->framecount;
      for (id = 0; id < NDELAYS; id++){
        if ((dlev = (pat->fpar[SYNTI2_F_DINLV1+id])) == 0.0f) continue;
        interm += s->delay[id][dsamp % DELAYSAMPLES] * dlev;
      }
      sigin[NOSCILLATORS+1] = interm;
#endif
      
      /* FM / Phase modulation synthesis */
      for(iosc = 0; iosc < NOSCILLATORS; iosc++){
        signal++; /* Go to next output slot. */
        wtoffs = (unsigned int)
          ((voi->c[iosc].fr 
            + sigin[pat->ipar3[SYNTI2_I3_FMTO1+iosc]])  /* phase modulator */
           * WAVETABLE_SIZE) & WAVETABLE_BITMASK;
        
#ifndef NO_EXTRA_WAVETABLES
        interm  = s->wave[pat->ipar3[SYNTI2_I3_HARM1+iosc]][wtoffs];
#else
        interm  = s->wave[0][wtoffs];
#endif
        
        /* parallel mix could be optional? Actually also FM could be? */
        interm *= (eprog[pat->ipar3[SYNTI2_I3_EAMP1+iosc]].f);
        interm += sigin[pat->ipar3[SYNTI2_I3_ADDTO1+iosc]]; /* parallel */
        interm *= pat->fpar[SYNTI2_F_LV1+iosc]; /* level/gain */
#ifndef NO_VELOCITY
        /* Optional velocity sensitivity. */
        if (pat->ipar3[SYNTI2_I3_VS1+iosc]){
          interm *= voi->velocity / 127.f;
        }
#endif
        *signal = interm;
      }
      
      /* Optional additive noise after FM operator synthesis: */
#ifndef NO_NOISE
      RandSeed *= 16807;
      interm += eprog[pat->ipar3[SYNTI2_I3_EAMPN]].f 
#ifndef NO_VELOCITY
        * ((pat->ipar3[SYNTI2_I3_VSN])?(voi->velocity / 127.f) : 1.f)
#endif
        * pat->fpar[SYNTI2_F_LVN]       /*noise gain*/
        * (float)RandSeed * 4.6566129e-010f; /*noise*/
#endif
      
#ifndef NO_DELAY
      /* Additive mix from delay lines. */
      signal++; /* Go to next output slot (=delay mix). */
      interm += *(signal) * pat->fpar[SYNTI2_F_LVD];  /*delay mix gain*/
#endif
      
#ifndef NO_FILTER
      /* Skip for faster computation. Should do the same for delays! */
      if(pat->ipar3[SYNTI2_I3_FILT]) {
        signal[0] = interm;
        
        /* Base frequency as note value from parameter. */
        fenv = pat->fpar[SYNTI2_F_FFREQ];
        renv = pat->fpar[SYNTI2_F_FRESO];
        
        /* Follow the pitch of an oscillator, if requested: */
#ifndef NO_FILTER_PITCH_FOLLOW
        if (pat->ipar3[SYNTI2_I3_FFOLL] > 0){
          fenv += voi->effnote[pat->ipar3[SYNTI2_I3_FFOLL]-1];
        }
#endif
        
        /* Convert to frequency at this point. */
        fenv = interpolate_note(s->note2freq,fenv);
        
#ifndef NO_FILTER_CUT_ENVELOPE
        /* Optionally _multiply_ cutoff frequency by an envelope. */
        if (pat->ipar3[SYNTI2_I3_EFILT]>0){
          fenv *= eprog[pat->ipar3[SYNTI2_I3_EFILT]].f;
        }
#endif
        
#ifndef NO_FILTER_RESO_ENVELOPE
        /* Optionally _multiply_ also resonance by an envelope. */
        if (pat->ipar3[SYNTI2_I3_EFILR]>0){
          renv *= eprog[pat->ipar3[SYNTI2_I3_EFILR]].f;
        }
#endif
        
        apply_filter(s, fenv, renv, signal);
        interm = signal[pat->ipar3[SYNTI2_I3_FILT]]; /*choose output*/
      }
#endif
      
#ifndef NO_DELAY
      /* mix also to the delay lines.*/
      for (id = 0; id < NDELAYS; id++){
        if ((dlev = (pat->fpar[SYNTI2_F_DLEV1+id])) == 0.0f) continue;
        /* Unit of the delay length parameter is now millisecond
           unless otherwise specified. WARNING: Sample rates not
           divisible by 1000 might behave badly! Floating point
           division would solve the problem, but cost about 10 bytes
           in code size. */
#ifndef NO_MILLISECOND_DELAY
        dsamp = s->framecount
          + (int)(pat->fpar[SYNTI2_F_DLEN1+id] * (s->sr / 1000));
#else
        dsamp = s->framecount
          + (int)(pat->fpar[SYNTI2_F_DLEN1+id] * s->sr);
#endif
        s->delay[id][dsamp % DELAYSAMPLES] += dlev * interm;
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
      pan += eprog[pat->ipar3[SYNTI2_I3_EPAN]].f;
#endif
      buffer[iframeL] += pat->fpar[SYNTI2_F_MIXLEV] * interm * (1.f-pan);
      buffer[iframeR] += pat->fpar[SYNTI2_F_MIXLEV] * interm * (1.f+pan);
#endif
    }
    
#ifndef NO_OUTPUT_SQUASH
    buffer[iframeL] = sin(buffer[iframeL]); /*Hack, but sounds nice*/
#ifndef NO_STEREO
    buffer[iframeR] = sin(buffer[iframeR]); /*Hack, but sounds nice*/
#endif
#endif
    
#ifndef NO_DELAY
    /* "erase the delay tape" so it can be used again. Must be done
       after each channel has read the current contents. */
    for(id=0; id<NDELAYS; id++){
      s->delay[id][s->framecount % DELAYSAMPLES] = 0.f; 
    }
#endif
  }
}
