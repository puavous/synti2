#include <math.h>
#include <limits.h>
#include <stdlib.h>

/* For debug prints: */
#include <stdio.h>

#include "synti2.h"

struct synti2_conts {
  int i;
  int ibuf;
  int nextframe;            /* TODO: Necessary field? */
  int frame[20000];         /*FIXME: think about limits */
  int msglen[20000];        /*FIXME: think about limits */
  unsigned char buf[20000]; /*FIXME: think about limits */
};

/* Polyphony */
#define NVOICES 32

/* Multitimbrality */
#define NPARTS 16

/* Total number of "counters", i.e., oscillators/operators. */
#define NCOUNTERS (NVOICES * 2)

/* Multitimbrality */
#define NENVPERVOICE 4

/* Total number of envelopes */
#define NENVS (NVOICES * NENVPERVOICE)

/* Maximum value of the counter type. Depends on C implementation, so use limits.h */
#define MAX_COUNTER UINT_MAX

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

/* TODO: Think about the whole envelope madness... use LFOs instead of
 * looping envelopes?
 */
#define TRIGGERSTAGE 6



/** I finally realized that unsigned ints will nicely loop around
 * (overflow) and as such they model an oscillator's phase pretty
 * nicely. Can I use them for other stuff as well? Seem fit for
 * envelopes with only a little extra (clamping and stop flag). Could
 * I use just one huge counter bank for everything? TODO: Try out, and
 * also think if it is more efficient to put values and deltas in
 * different arrays. Probably, if you think of a possible assembler
 * implementation with multimedia vector instructions(?).
 */
typedef struct counter {
  unsigned int val;
  unsigned int delta;
} counter;

/** TODO: Not much is inside the part structure. Is it necessary at
 *  all? It will have controller values, though..
 */
typedef struct synti2_part {
  int voiceofkey[128];  /* Which note has triggered which voice*/
} synti2_part;

struct synti2_synth {
  float infranotes[128]; /* TODO: This space could be used for LFO's */
  float note2freq[128];  /* pre-computed frequencies of notes... Tuning
			    systems would be easy to change - just
			    compute a different table here (?)..*/
  float ultranotes[128]; /* TODO: This space could be used for noises? */

  /* Oscillators are now modeled by integer counters (phase). */
  counter c[NCOUNTERS];
  float fc[NCOUNTERS];   /* store the values as floats right away. */

  /* Envelope progression also modeled by integer counters. Not much
   * difference between oscillators and envelopes!!
   */
  counter eprog[NVOICES][NENVPERVOICE];
  float feprog[NVOICES][NENVPERVOICE];   /* store the values as floats right away. */
  float feprev[NVOICES][NENVPERVOICE];   /* previous value (for lin. interp.) */
  float fegoal[NVOICES][NENVPERVOICE];   /* goal value (for lin. interp.) */
  float fenv[NVOICES][NENVPERVOICE];     /* current output (of lin. interp.) */

  /* Envelope stages just a table? TODO: think.*/
  int estage[NVOICES][NENVPERVOICE];
  int sustain[NVOICES];

  int partofvoice[NVOICES];  /* which part has triggered each "voice";
				-1 (should we use zero instead?) means
				that the voice is free to re-occupy. */

  int note[NVOICES];
  int velocity[NVOICES];

  /* The parts. Sixteen as in the MIDI standard. TODO: Could have more? */
  synti2_part part[NPARTS];   /* FIXME: I want to call this channel!!!*/
  float patch[NPARTS * SYNTI2_NPARAMS];  /* The sound parameters per part*/

  unsigned long sr;

  /* Throw-away test stuff:*/
  long int global_framesdone;  /*NOTE: This is basically just a counter, too! */
};


#define SYNTI2_MAX_SONGBYTES 30000
#define SYNTI2_MAX_SONGTRACKS 32

struct synti2_player {
  int sr;       /* Sample rate. */
  //int bpm;      /* Tempo (may vary). */
  int fpt;      /* Frames per tick. Tempos will be inexact, sry! */
  int ntracks;  /* Number of tracks. */
  int tpq;      /* Ticks per quarter (no support for SMPTE). */
  int deltaticks[SYNTI2_MAX_SONGTRACKS];
  int frames_to_next_tick;
  unsigned char *track [SYNTI2_MAX_SONGTRACKS];
  unsigned char data[SYNTI2_MAX_SONGBYTES];
};

/** Reads a MIDI variable length number. */
static 
int
varlength(const unsigned char * source, int * dest){
  int nread;
  unsigned char byte;
  *dest = 0;
  for (nread=1; nread<=4; nread++){
    byte = *source++;
    *dest += (byte & 0x7f);
    if ((byte >> 7) == 0) return nread;
  }
  return 0; /* Longer than 4 bytes! Wrong input! FIXME: die. */
}

static 
int 
twobyte(unsigned char *r){
  return (r[0] * 0x100) + r[1];
}

static 
int 
fourbyte(unsigned char *r){
  return (r[0] * 0x1000000) + (r[1] * 0x10000) + (r[2] * 0x100) + r[3];
}


/** Load and initialize a song. */
synti2_player *
synti2_player_create(unsigned char * songdata, int datalen, int samplerate){
  unsigned char *r;
  int format, ii, it, chunksize;
  synti2_player *pl;

  pl = calloc(1, sizeof(synti2_player));
  if (pl == NULL) return pl;

  /* Default BPM is 120. Convert to frames-per-tick - losing accuracy.*/
  pl->sr = samplerate;
  pl->frames_to_next_tick = 0; /* There will be a tick right away. */

  /* TODO: Should checks limits and file format!! (unless compiling for 4k) */
  /* Read and initialize a zong. (make a copy of the data - not necessary in 4k.) */
  for (ii=0;ii<datalen;ii++){
    pl->data[ii] = songdata[ii];
  }

  r = pl->data;
  r += 8; /*Just skip header; TODO: I want at least a midi "distiller"
	    to rid of what I don't need!*/
  format = twobyte(r); r += 2;
  pl->ntracks = twobyte(r); r += 2;
  pl->tpq = twobyte(r); r += 2;
  //if ((pl->tpq & 0x8000) != 0) {free(pl); return NULL; /*no support for SMPTE*/};
  /* Default BPM is 120. Convert to frames-per-tick - losing accuracy.*/
  pl->fpt = (pl->sr * 60) / (120 * pl->tpq);

  for (it=0; it < pl->ntracks; it++){
    r += 4; /* skip header */
    chunksize = fourbyte(r); r += 4; /* Fixme: chunk includes header? */     
    pl->track[it] = r;
    r += chunksize;
  }

  /* The first deltas: */
  for (it=0; it < pl->ntracks; it++){
    /* Store delta and go directly to start of event data: */
    pl->track[it] += varlength(pl->track[it], &(pl->deltaticks[it]));
  }
  
  return pl;
}

/** .. returns the effective length of this event. */
static
int
synti2_player_do_event(synti2_conts *control, int frame, unsigned char *midibuf)
{
  int length = 0;  /* length must be determined for all possible
		      events, including those that are skipped. F*k
		      there's going to be too much code in here!! I'm
		      going to have to rewrite the sequencer again,
		      and do a midi importer to my own local
		      format! */
  int vlenlen;

  /* FIXME: The events can be also running statuses!! Must handle
     those (or filter out with a tool program)! */
  /* The easy ones.. normal midi control.. */
  //printf("Next up: %02x %02x %02x %02x \n", midibuf[0], midibuf[1], midibuf[2], midibuf[3]); fflush(stdout);

  if ((0xc0 <= midibuf[0]) && (midibuf[0] < 0xe0)){
    length = 2; /* ... or is the third parameter sent even if not used? */
    synti2_conts_store(control, frame, midibuf, length);
  } else if (/* (0x80 <= midibuf[0]) && */ (midibuf[0] < 0xf0)){
    length = 3;
    synti2_conts_store(control, frame, midibuf, length);
  } else if (midibuf[0] >= 0xf0) {
    switch (midibuf[0]){
    case 0xf0:
      vlenlen = varlength(midibuf + 2, &length);
      *(midibuf+1+vlenlen) = 0xf7; /*FIXME: terrible hacks appear from the void.*/
      synti2_conts_store(control, frame, midibuf+1+vlenlen, length);
      length += vlenlen + 1;
      break;
    case 0xff:
      /* Meta-event. Affects just our player's internal state. */
      switch (midibuf[1]){
      case 0x27: /* Set tempo */
	/* FIXME: Implement!.. */
	vlenlen = varlength(midibuf + 2, &length);
	length += vlenlen + 2;
	break;
      case 0x2f: /* End of Track*/
	/* FIXME: Should stop playback.. */
	length = -1; /* This way, the same event will keep on appearing... hack. */
	break;
      default:
	vlenlen = varlength(midibuf + 2, &length);
	length += vlenlen + 2;
      }
      break;
      //default:
      /* Should die, in fact. */
    }
  } else {
    /* Should die, in fact. */
  }
  return length;
}


/** render some frames of control data for the synth, keeping track of
 * song position. This will do the store()s as if the song was played
 * live.
 *
 * Upon entry: each track's delta is known. each track's pointer
 * points to the first byte of actual event data.
 *
 * Upon exit: each track's delta has been updated, if necessary. each
 * track's pointer points to the first byte of actual event data.
 */
void
synti2_player_render(synti2_player *pl, 
		     synti2_conts *control,
		     int framestodo)
{
  int it;
  //  int minframes = frames;
  int efflen;
  int move, frame;
  frame = 0;

  while (framestodo > 0) {
    //printf("todo: %d  to next: %d \n",framestodo,pl->frames_to_next_tick);fflush(stdout);
    move = (framestodo < pl->frames_to_next_tick)?framestodo:pl->frames_to_next_tick;
    frame += move; /* Is it possible to lose data on buffer boundary, btw..?*/
    framestodo -= move;
    pl->frames_to_next_tick -= move;
  
    /* When we hit a tick, process it: */
    if (pl->frames_to_next_tick == 0) /*FIXME: think. && (framestodo>0) ??? */ {
      pl->frames_to_next_tick += pl->fpt;  /* remember to update.. */
      for (it=0; it < pl->ntracks; it++){
	/* Everything that happens right now on this track: */
	while (pl->deltaticks[it] == 0) {
	  /* Process an event, and read next delta.*/
	  //printf("track %d\n", it);
	  efflen = synti2_player_do_event(control, frame, pl->track[it]);
	  if (efflen > 0){
	    pl->track[it] += efflen;
	    pl->track[it] += varlength(pl->track[it],&(pl->deltaticks[it]));
	  } else {
	    /* Track has ended. */
	    //printf("End of track %d reached.\n",it);fflush(stdout);
	    pl->deltaticks[it] = -1; /*FIXME: What does this imply?*/
	  }
	}
	if (pl->deltaticks[it] > 0) pl->deltaticks[it]--;
      }
    }
  }
}
/** Creates a controller "object". Could be integrated within the
 * synth itself?  For the final 4k version (synti-kaksinen:)),
 * everything will probably have to be crushed into the same unit
 * anyway, created at once, and glued together with holy spirit.
 */
synti2_conts * synti2_conts_create(){
  return calloc(1, sizeof(synti2_conts));
}

/** Get the next MIDI command */
static void 
synti2_conts_get(synti2_conts *control, 
		 unsigned char *midibuf)
{
  int i;
  for (i=0;i < control->msglen[control->i]; i++){
    midibuf[i] = control->buf[control->ibuf++];
  }
  control->i++;
  control->nextframe = control->frame[control->i];
}

/** Get ready to store new events. Must be called after each audio
 * block, before writing new events.
 */
void 
synti2_conts_reset(synti2_conts *control)
{
  control->i = 0;
  control->ibuf = 0;
  control->nextframe = -1;
}

/** Must be called once before calling any gets (rewinds internal
 * pointers).
 */
void 
synti2_conts_start(synti2_conts *control)
{
  control->i = 0;
  control->ibuf = 0;
}

/** Store a MIDI event. Raw MIDI data, like in the jack MIDI system.*/
void 
synti2_conts_store(synti2_conts *control,
		   int frame,
		   unsigned char *midibuf,
		   int len)
{
  int i;
  if (control->nextframe < 0){
    control->nextframe = frame;
  }
  control->frame[control->i] = frame;
  control->msglen[control->i] = len;
  for (i=0;i<len;i++){
    control->buf[control->ibuf++] = midibuf[i];
  }
  control->i++;
  control->frame[control->i] = -1; /* Marks the (new) end. */
}


/** Allocate and initialize a new synth instance. */
synti2_synth *
synti2_create(unsigned long sr)
{
  synti2_synth * s;
  int ii;

  s = calloc (1, sizeof(synti2_synth));
  if (s == NULL) return s;
  s->sr = sr;

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
    s->note2freq[ii] = 440.0 * pow(2.0, ((float)ii - 69.0) / 12.0 );
  }

  for(ii=0; ii<NVOICES; ii++){
    s->partofvoice[ii] = -1;     /* Could I make this 0 somehow? */
  }

  return s;
}


static
void
synti2_do_noteon(synti2_synth *s, int part, int note, int vel)
{
  int freevoice, ie;
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
  for(freevoice=0; freevoice < NVOICES; freevoice++){
    if (s->partofvoice[freevoice] < 0) break;
  }
  if (freevoice==NVOICES) return; /* Cannot play new note! */
  /* (Could actually force the last voice to play anyway!?) */
  
  s->part[part].voiceofkey[note] = freevoice;
  s->partofvoice[freevoice] = part;
  s->note[freevoice] = note;
  s->velocity[freevoice] = vel;
  s->sustain[freevoice] = 1;  
 
  /* TODO: trigger all envelopes according to patch data..  Just give
     a hint to the evaluator function.. */
  for (ie=0; ie<NENVPERVOICE; ie++){
    s->estage[freevoice][ie] = TRIGGERSTAGE;
    s->eprog[freevoice][ie].delta = 0;
  }
}

static
void
synti2_do_noteoff(synti2_synth *s, int part, int note, int vel){
  int voice, ie;
  voice = s->part[part].voiceofkey[note];
  //s->part[part].voiceofkey[note] = -1;  /* key should no longer map to ... no... */
  //if (voice < 0) return; /* FIXME: think.. */
  /* TODO: release all envelopes.. */
  for (ie=0; ie<NENVPERVOICE; ie++){
    s->estage[voice][ie] = 2;      /* skip to end */
    s->eprog[voice][ie].delta = 0; /* skip to end */
    s->eprog[voice][ie].val = 0;   /* must skip also value!! FIXME: think(?)*/
    s->sustain[voice] = 0;         /* don't renew loop. */
  }
}


/** FIXME: Think about data model.. aim at maximal sparsity but enough
    expressive range. If it turns out that the 1/1000 accuracy is very
    seldom required, then it could be worthwhile to store everything
    in the much simpler format of 4*7 = 28-bit signed integer
    representing (decimal) hundredths. Even then most parameters would
    only have the least-significant 7bit set?

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
*/
static
void
synti2_do_receiveSysEx(synti2_synth *s, unsigned char * data){
  int offset, stride;
  unsigned char adjust_byte, adjust_nib, sign_nib;

  int ir;
  float decoded;

  /* FIXME: Manufacturer ID check.. length check; checksums :) could
   * have checks.. :) but checks are for chicks?
   */

  /* Process header: */
  offset = data[4]; offset <<= 7; offset += data[5];
  stride = data[6]; stride <<= 7; stride += data[7];
  data += 8;
  
  //jack_info("Receiving! Offset %d Length %d", offset, stride);
  /* Process data: */
  for (ir=0; ir<stride; ir++){
    decoded  = (*(data + 0*stride)) * 0.01;  /* hundredths*/
    decoded += (*(data + 1*stride));         /* wholes */
    decoded += (*(data + 2*stride)) * 100.0; /* hundreds*/
    adjust_byte = *(data + 3*stride);
    adjust_nib = adjust_byte & 0x0f;
    sign_nib = adjust_byte >> 4;
    decoded += adjust_nib * 0.001;           /* thousandths*/
    if (sign_nib>0) decoded = -decoded;      /* sign. */

    s->patch[offset++] = decoded;
    data++;
  }
  /* Could check that there is F7 in the end :)*/
}



/** Things may happen late or ahead of time. I don't know if that is
 *  what they call "jitter", but if it is, then this is where I jit
 *  and jat...
 */
static
void
synti2_handleInput(synti2_synth *s, 
                   synti2_conts *control,
                   int uptoframe)
{
  /* TODO: sane implementation */
  unsigned char midibuf[20000];/*FIXME: THINK about limits and make checks!!*/

  while ((0 <= control->nextframe) && (control->nextframe <= uptoframe)){
    synti2_conts_get(control, midibuf);
    if ((midibuf[0] & 0xf0) == 0x90){
      synti2_do_noteon(s, midibuf[0] & 0x0f, midibuf[1], midibuf[2]);
    } else if ((midibuf[0] & 0xf0) == 0x80) {
      synti2_do_noteoff(s, midibuf[0] & 0x0f, midibuf[1], midibuf[2]);
    } else if (midibuf[0] == 0xf0){
      synti2_do_receiveSysEx(s, midibuf);
    }else {
      /* Other MIDI messages are silently ignored. */
    }
  }
}

/** TODO: Implement better? */
static
void
synti2_evalEnvelopes(synti2_synth *s){  
  int ie, iv;
  unsigned int detect;
  /* A counter is triggered just by setting delta positive. */
  /* FIXME: Here could be a good place for some pointer arithmetics
   * instead of this index madness [iv][ie] ... Also see if this and
   * the code for the main oscillators could be combined.
   */
  for(iv=0;iv<NVOICES; iv++){
    for(ie=0;ie<NENVPERVOICE; ie++){
      if (s->eprog[iv][ie].delta == 0) continue;  /* stopped.. not running*/
      detect = s->eprog[iv][ie].val;
      s->eprog[iv][ie].val += s->eprog[iv][ie].delta; /* counts from 0 to MAX */
      if (s->eprog[iv][ie].val < detect){  /* Detect overflow */
	s->eprog[iv][ie].delta = 0;  /* Counter stops after full cycle   */
	s->feprog[iv][ie] = 1.0;     /* and the floating value is clamped */ 
	s->eprog[iv][ie].val = 0;    /* FIXME: Is it necessary to reset val? */
      } else {
	s->feprog[iv][ie] = ((float) s->eprog[iv][ie].val) / MAX_COUNTER;
      }
      s->fenv[iv][ie] = (1.0 - s->feprog[iv][ie]) * s->feprev[iv][ie] 
	+ s->feprog[iv][ie] * s->fegoal[iv][ie];
    }
    /* TODO: Observe that for this kind of rotating counter c
       interpreted as x \in [0,1], it is easy to get 1-x since it is
       just -c in the integer domain (?). Useable fact? */
    /* When delta==0, the counter has gone a full cycle and stopped at
     * the maximum floating value, 1.0, and the envelope output stands
     * at the goal value ("set point").
     */
  }
}


/* Advance the oscillator counters. Consider using the xmms assembly
 * for these? Try to combine with the envelope counter code somehow..
 */
static 
void
synti2_evalCounters(synti2_synth *s){
  int ic;
  for(ic=0;ic<NCOUNTERS;ic++){
    s->c[ic].val += s->c[ic].delta;
    s->fc[ic] = (float) s->c[ic].val / MAX_COUNTER;
  }
}


/** Updates envelope stages as the patch data dictates. */
static
void
synti2_updateEnvelopeStages(synti2_synth *s){
  int iv, ie, ipastend;
  int part;
  float nextgoal;
  float nexttime;
  int stagesum;

  for(iv=0; iv<NVOICES; iv++){
    part = s->partofvoice[iv];
    if (part<0) continue;
    stagesum = 0;
    for (ie=0; ie<NENVPERVOICE; ie++){
      ipastend = part * SYNTI2_NPARAMS + SYNTI2_IENVS + (ie+1) * SYNTI2_NENVD;
      if (s->estage[iv][ie] == 0) continue; /* skip untriggered envs. */
      /* Think... delta==0 on a triggered envelope means endclamp?? 
         NOTE: Need to set delta=0 upon note on!!
         and estage == NSTAGES+1 or so (=6?) means go to attack.. */
      if (s->eprog[iv][ie].delta == 0){
        s->estage[iv][ie]--; /* Go to next stage (one smaller ind) */
        if (s->estage[iv][ie] == 0) continue; /* Newly ended one.. no more knees. */
	if ((s->estage[iv][ie] == 1) && (s->sustain[iv] != 0)) s->estage[iv][ie] = 3;
        nexttime = s->patch[ipastend - s->estage[iv][ie] * 2 + 0];
        nextgoal = s->patch[ipastend - s->estage[iv][ie] * 2 + 1];
        s->feprev[iv][ie] = s->fenv[iv][ie];
        s->fegoal[iv][ie] = nextgoal;
	if (nexttime <= 0.0){
	  s->eprog[iv][ie].val = 1; /* hack for "no time". will be 0 after an eval. */
	  s->eprog[iv][ie].delta = MAX_COUNTER;
	} else {
	  s->eprog[iv][ie].delta = MAX_COUNTER / s->sr / nexttime;
	}
	/*
	  if (ie==0)
	  jack_info("v%02de%02d(Rx%02d): stage %d at %.2f to %.2f in %.2fs (d=%d) ", 
	  iv, ie, part, s->estage[iv][ie], s->feprev[iv][ie], 
	  s->fegoal[iv][ie], 
	  nexttime, s->eprog[iv][ie].delta);
	*/
      }
      /* FIXME: just simple LFO stuff instead of this hack of a looping envelope?*/
      stagesum += s->estage[iv][ie];
    }
    if (stagesum == 0){
      /* Consider note instance "completely finished" when all envelopes are over: */
      s->partofvoice[iv] = -1;
    }
  }
}


/** Converts note values to counter deltas. */
static
void
synti2_updateFrequencies(synti2_synth *s){
  int iv, note;
  float notemod, interm, freq;
  /* Frequency computation... where to put it after all? */
  for (iv=0; iv<NVOICES; iv++){
    notemod = s->note[iv] + s->fenv[iv][2];   // HACK!!
    /* should make a floor (does it? check spec)*/
    note = notemod;
    interm = (1.0 + 0.05946 * (notemod - note)); /* +cents.. */
    freq = interm * s->note2freq[note];
    
    s->c[iv*2].delta = freq / s->sr * MAX_COUNTER;

    /* modulator pitch; Hack. FIXME: */
    notemod = s->note[iv] + s->fenv[iv][3];   // HACK!!
    /* should make a floor (does it? check spec)*/
    note = notemod;
    interm = (1.0 + 0.05946 * (notemod - note)); /* +cents.. */
    freq = interm * s->note2freq[note];

    s->c[iv*2+1].delta = (freq) / s->sr * MAX_COUNTER; /*hack test*/
    //s->c[2].delta = freq / s->sr * MAX_COUNTER; /* hack test */
  }
}


void
synti2_render(synti2_synth *s, 
	      synti2_conts *control, 
	      synti2_smp_t *buffer,
	      int nframes)
{
  int iframe, ii, iv;
  float interm;
  
  for (iframe=0; iframe<nframes; iframe += NINNERLOOP){
    /* Outer loop for things that are allowed some jitter. (further
     * elaboration in comments near the definition of NINNERLOOP).
     */

    /* Handle MIDI-ish controls if there are more of them waiting. */
    synti2_handleInput(s, control, iframe + NINNERLOOP);
    synti2_updateEnvelopeStages(s); /* move on if need be. */
    synti2_updateFrequencies(s);    /* frequency upd. */
    
    /* Inner loop runs the oscillators and audio generation. */
    for(ii=0;ii<NINNERLOOP;ii++){

      /* Could I put all counter upds in one big awesomely parallel loop? */
      synti2_evalEnvelopes(s);
      synti2_evalCounters(s);
      
      /* Getting more realistic soon: */
      buffer[iframe+ii] = 0.0;

      for(iv=0;iv<NVOICES;iv++){
	// if (s->partofvoice[iv] < 0) continue; /* Unsounding. FIXME: (f1) */
	/* Produce sound :) First modulator, then carrier*/
        /* Worth using a wavetable? Probably.. Could do the first just
	   by bit-shifting the counter... */
	interm  = sin(2*M_PI * s->fc[iv*2+1]);
        interm *= (s->velocity[iv]/128.0) * (s->fenv[iv][1]);
	interm  = sin(2*M_PI * (s->fc[iv*2+0] + interm));
	interm *= s->fenv[iv][0];
	buffer[iframe+ii] += interm;
      }
      buffer[iframe+ii] /= NVOICES;
    }
  }
  s->global_framesdone += nframes;
}
