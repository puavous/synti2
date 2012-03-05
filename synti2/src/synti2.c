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
#include "synti2_guts.h"
#include "synti2_misss.h"

/* Synti2 can be compiled as a Jack-MIDI compatible real-time soft
 * synth. Jack headers are not necessary otherwise...
 */
#ifdef JACK_MIDI
#include "synti2_jack.h"
/* If any foreign MIDI protocol is used, note-offs should be converted
 * to our own protocol (also a well-known MIDI variant) in which
 * note-on with zero velocity means a note-off.
 * */
#define DO_CONVERT_OFFS
#endif



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
 * how to make significant improvements to code size that way.
 */
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
  ev_new = pl->freeloc++;

  /* Fill in the node: */
  ev_new->data = src;         /* SHALLOW COPY here.*/
  ev_new->next = pl->insloc->next;
  ev_new->frame = frame;
  ev_new->len = n;
  pl->insloc->next = ev_new;
  /*printf("stored frame %d data %02x %02x %02x\n", frame, src[0], src[1], src[2]); fflush(stdout);*/
  
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
      /* Not yet implemented. FIXME: implement? Controller reset==fast ramp!*/
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



/** Allocate and initialize a new synth instance. */
synti2_synth *
synti2_create(unsigned long sr, 
              const byte_t * patchdata, 
              const byte_t * songdata)
{
  synti2_synth * s;
  int ii, wt;
  float t;

  s = calloc (1, sizeof(synti2_synth));

#ifndef ULTRASMALL
  if (s == NULL) return NULL;
#endif

  s->pl = calloc (1, sizeof(synti2_player));

#ifndef ULTRASMALL
  if (s->pl == NULL) {free(s); return NULL;}
#endif

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
  synti2_player_init_from_misss(s->pl, songdata);
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
  return s;
}


/** Note on OR note off (upon velocity == 0) */
static
void
synti2_do_noteon(synti2_synth *s, int voice, int note, int vel)
{
  int ie;

  /* note off */
  if (vel==0){
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
  s->note[voice] = note;
  s->velocity[voice] = vel;
  s->sustain[voice] = 1;    /* FIXME: Needed only if loop env is used?*/
 
  /* TODO: trigger all envelopes according to patch data..  Just give
     a hint to the evaluator function.. */
  for (ie=0; ie<=NENVPERVOICE; ie++){
    s->estage[voice][ie] = TRIGGERSTAGE;
    s->eprog[voice][ie].delta = 0;
  }
}


static
void
synti2_fill_patches_from(synti2_patch *pat, const unsigned char *data)
{
  int a,b,c,ir;
  float decoded;
  for(; *data != 0xf7; pat++){
    for(ir=0;ir<SYNTI2_I3_NPARS; ir+=2){
      pat->ipar3[ir] = *data >> 3;
      pat->ipar3[ir+1] = (*data++) & 0x7;
    }

    for (ir=0; ir<SYNTI2_F_NPARS; ir++){
      /* TODO: Try different approaches to data storage? */
      a = *data;
      b = *(data+SYNTI2_F_NPARS);
      data++;      

      decoded = ((a & 0x03) << 7) + b;   /* 2 + 7 bits accuracy*/
      decoded = (a & 0x40) ? -decoded : decoded;  /* sign */
      decoded *= .001f;                           /* default e-3 */
      for (c=0; c < ((a & 0x0c) >> 2); c++) decoded *= 10.f; /* can be more */
      /*use of powf() seems to yield longer exe..*/
      /*decoded *= powf(10.f, ((a & 0x0c) >> 2)-3);*/
      /*pat->fpar[ir] = decoded * powf(10.f, ((a & 0x0c) >> 2)-3.f);*/
      pat->fpar[ir] = decoded;
    }
    data += SYNTI2_F_NPARS;
  }
}


#ifdef USE_MIDI_INPUT
/** Receive a MIDI SysEx. (Convenient for sound editing, but not
 *  strictly necessary for stand-alone 4k synth.)
 *
 * FIXME: Manufacturer ID check.. Should also check that there is F7
 *  in the end :) length check; checksums :) could (and should) have
 *  checks now that this code is moved outside the stand-alone synth
 *  and thus is not size critical anymore..
 */

static
void
synti2_do_receiveSysEx(synti2_synth *s, const byte_t * data){
  int opcode, offset, ir;
  int a, adjust_byte = 0;
  synti2_patch *pat;
  float decoded;  
  /*static int stride = SYNTI2_F_NPARS;*/ /* Constant, how to do?*/
  
  /* Sysex header: */
  data += 4; /* skip Manufacturer IDs n stuff TODO: think about this */
  opcode = *data++; opcode <<= 7; opcode += *data++;  /* what to do */
  offset = *data++; offset <<= 7; offset += *data++;  /* in where */

  /* Sysex data: */
  /* As of yet, offset==patch index. TODO: More elaborate addressing? */

  if (opcode==0){
    /* Opcode 0: fill in patch memory (one or more patches at a
     * time). Data must be complete and without errors; no checks are
     * made in here.
     */
    synti2_fill_patches_from(s->patch + (offset & 0x7f), data);

#ifndef NO_EXTRA_SYSEX
  } else if (opcode==1) {
    /* Receive one 3-bit parameter at location (patch,i3par_index) */
    pat = s->patch + (offset & 0x7f); 
    ir = offset >> 7;
    pat->ipar3[ir] = *data;
    /*jack_info("Rcv patch %d I3 param %d: %d", offset & 0x7f, ir, *data);*/
  } else if (opcode==2) {
    /* Receive one 7-bit parameter at location (patch,i7par_index) */
    /* (These were removed from the synth. Opdoce 2 no longer used!) */
    /*pat = s->patch + (offset & 0x7f); ir = offset >> 7;
      pat->ipar7[ir] = *data; */
  } else if (opcode==3) {
    /* Receive one fixed point parameter at location (patch,fpar_index) */
    pat = s->patch + (offset & 0x7f); 
    ir = offset >> 7;
    /* FIXME: Decoding should be a static function instead of copy-paste:*/
    decoded = ((data[0] & 0x03) << 7) + data[1];   /* 2 + 7 bits accuracy*/
    decoded = ((data[0] & 0x40)>0) ? -decoded : decoded;  /* sign */
    decoded *= .001f;                            /* default e-3 */
    for (a=0; a < ((data[0] & 0x0c)>>2); a++) decoded *= 10.f;  /* or more */
    pat->fpar[ir] = (adjust_byte >> 4) ? -decoded : decoded; /* sign.*/
    /*jack_info("Rcv patch %d F param %d: %f", offset & 0x7f, ir, decoded);*/
#endif
  } else {
    /* Unknown opcode - should be an error. */
  }
}
#endif

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
                   unsigned int upto_frames)
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
      /* FIXME: Could I use only 3 bits for the message type? Then I
       * could have 32 individually addressable channels!
       */
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
  unsigned int ind;
  double ff;
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


/** Updates envelope stages as the patch data dictates. 

  FIXME: just simple LFO stuff instead of this hack of a looping envelope?

  FIXME: Or make looping optional by a simple sound parameter... 

*/
static
void
synti2_updateEnvelopeStages(synti2_synth *s){
  int iv, ie, ipastend;
  float nextgoal;
  float nexttime;
  synti2_patch *pat;

  /* remember that voice == part under the new design decisions.. */
  for(iv=0; iv<NPARTS; iv++){
    /* FIXME: See if this needs some "completely finished sound" logic*/

    pat = s->patch + iv;

    for (ie=1; ie<=NENVPERVOICE; ie++){
      /*printf("At %d voice %d env %d\n", s->framecount.val, iv,ie); fflush(stdout);*/
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
        nexttime = pat->fenvpar[ipastend - s->estage[iv][ie] * 2 + 0];
        nextgoal = pat->fenvpar[ipastend - s->estage[iv][ie] * 2 + 1];
        s->eprog[iv][ie].aa = s->eprog[iv][ie].f;
        s->eprog[iv][ie].bb = nextgoal;
        if (nexttime <= 0.0f) {
          /*No time -> skip envelope knee. Force value to the new
            level (next goal). Delta remains at 0, and we may skip many.*/
          s->eprog[iv][ie].f = s->eprog[iv][ie].bb;
        } else {
          s->eprog[iv][ie].val = 0; /*FIXME: Is it necessary to reset val?*/
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
  for (iv=0; iv<NPARTS; iv++){
    pat = s->patch + iv;
    if (pat==NULL) continue;

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
  unsigned int dsamp;
  int iframe, ii, iv;
  int iosc;
  float interm;
  int wtoffs;
  synti2_player *pl;
  synti2_patch *pat;

  float *sigin;  /* Input signal table during computation. */
  float *signal; /* Output signal destination during computation. */

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
      
      /* Sound output. Getting more realistic as we speak... */
      buffer[iframe+ii] = 0.0f;
      
      for(iv=0;iv<NPARTS;iv++){

        pat = s->patch + iv;
        /* FIXME: Need logic for "unsounding"? */

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

          /*interm *= (s->velocity[iv]/128.0f);*/  /* Velocity sensitivity */
          /* could reorganize I3 parameters for shorter code here(?): */
          interm *= (s->eprog[iv][pat->ipar3[SYNTI2_I3_EAMP1+iosc]].f);
          interm += sigin[pat->ipar3[SYNTI2_I3_ADDTO1+iosc]]; /* parallel */
          interm *= pat->fpar[SYNTI2_F_LV1+iosc]; /* level/gain */
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
        /* mix also to a delay line. FIXME: s->delaypos is a counter
           like frame? FIXME: Was that everything-is-a-counter thing a
           good idea in the first place?*/
        dsamp += (int)(pat->fpar[SYNTI2_F_DLEN] * s->sr);
        //dsamp %= DELAYSAMPLES;
        s->delay[pat->ipar3[SYNTI2_I3_DNUM]][dsamp % DELAYSAMPLES] 
                 += pat->fpar[SYNTI2_F_DLEV] * interm;
#endif

        /* result is both in *signal and in interm (like before). 
         * Mix (no stereo as of yet) 
         */
        buffer[iframe+ii] += interm;

      }

#ifndef NO_OUTPUT_SQUASH
      buffer[iframe+ii] = sin(buffer[iframe+ii]); /*Hack, but sounds nice*/
#endif

#ifndef NO_DELAY
      for(dsamp=0; dsamp<NDELAYS; dsamp++){
        s->delay[dsamp][s->framecount.val % DELAYSAMPLES] = 0.f; 
      }
#endif
      /*buffer[iframe+ii] = sin(32*buffer[iframe+ii]);*/ /* Hack! beautiful too!*/
      /*buffer[iframe+ii] = tanh(16*buffer[iframe+ii]);*/ /* Hack! beautiful too!*/
    }
  }
}
