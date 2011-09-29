#include "synti2.h"
#include <math.h>
#include <limits.h>
#include <stdlib.h>

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

/* Maximum value of the counter type */
#define MAX_COUNTER UINT_MAX

/* Number of inner loop iterations (frame evaluations) between
 * evaluating "slow-motion stuff" such as MIDI input and
 * envelopes. Must be a divisor of the buffer length. Example: 48000/8
 * would yield 6000 Hz (or faster than midi wire rate) responsiveness.
 * Hmm.. will there be audible problems with "jagged volumes" if I
 * put the volume envelope outside of the inner loop... So I'll keep at
 * least the envelope progression counter code inside.
 */
#define NINNERLOOP 8

/* TODO: Think about the whole envelope madness... */
#define TRIGGERSTAGE 6



/** I just realized that unsigned ints will nicely loop around
 * (overflow) and as such they model an oscillator's phase pretty
 * nicely. Can I use them for other stuff as well? Seem fit for
 * envelopes with only a little extra (clamping and stop flag)
 */
typedef struct counter {
  unsigned int val;
  unsigned int delta;
} counter;

typedef struct synti2_part {
  int voiceofkey[128];  /* Which note has triggered which voice*/
} synti2_part;

struct synti2_synth {
  float note2freq[128]; /* pre-computed frequencies of notes... Tuning
			   systems would be easy to change - just
			   compute a different table here (?)..*/

  /* TODO: Reduce number of constants, i.e., make 2*VOICE etc. */

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
				that the voice is not alive. */

  int note[NVOICES];
  int velocity[NVOICES];

  /* The parts. Sixteen as in the MIDI standard. */
  synti2_part part[NPARTS];   /* FIXME: I want to call this channel!!!*/
  float patch[NPARTS * SYNTI2_NPARAMS];  /* The sound parameters per part*/

  unsigned long sr;

  /* Throw-away test stuff:*/
  long int global_framesdone;
};

/** Creates a controller "object". Could be integrated within the synth itself?*/
synti2_conts * synti2_conts_create(){
  return calloc(1, sizeof(synti2_conts));
}

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

/** Get ready to store new controls. Must be called after reading all
    messages for a block, and before writing new ones. */
void 
synti2_conts_reset(synti2_conts *control)
{
  control->i = 0;
  control->ibuf = 0;
  control->nextframe = -1;
}

/** Must be called once before calling any gets */
void 
synti2_conts_start(synti2_conts *control)
{
  control->i = 0;
  control->ibuf = 0;
}

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
   */
  for(ii=0; ii<128; ii++){
    s->note2freq[ii] = 440.0 * pow(2.0, ((float)ii - 69.0) / 12.0 );
  }

  for(ii=0; ii<NVOICES; ii++){
    s->partofvoice[ii] = -1;
  }

  /* Hack with the example sound
  //  for(ic=0; ic<NPARTS; ic++){
  for(ic=0; ic<1; ic++){
    for(ii=0; ii<SYNTI2_NPARAMS; ii++){
      s->patch[ic*SYNTI2_NPARAMS + ii] = hack_examplesound[ii];
    }
  }
*/

  return s;
}


static
void
synti2_do_noteon(synti2_synth *s, int part, int note, int vel)
{
  int freevoice, ie;
  /* note on */
  for(freevoice=0; freevoice < NVOICES; freevoice++){
    if (s->partofvoice[freevoice] < 0) break;
  }
  if (freevoice==NVOICES) return; /* Cannot play new note! */
  /* (Could actually force the last voice to play anyway!!) */
  
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
    s->sustain[voice] = 0;               /* don't renew loop. */
  }
}


/** FIXME: Think about data model.. aim at maximal sparsity but enough
    expressive range. Maximum obtainable with below is 
    (127+1.27+0.0127)*63 = 8 081.8101 

    SysEx format (planned; FIXME: implement!) 

    F0 ?? [storeAddrMSB] [storeAddr2LSB] [inputStride] ... data ... F7


    
*/
static
void
synti2_do_receiveSysEx(synti2_synth *s, unsigned char * data){
  int ii, ir;
  ir = 0;
  /* Most common: range +0.00 to +1.27 */
  for (ii = 0; ii<NPARTS * SYNTI2_NPARAMS; ii++){
    s->patch[ii] = data[ir++] / 100.0;
  }
  /* If needed: add integer value in +0 to +127 */
  for (ii = 0; ii<NPARTS*SYNTI2_NPARAMS; ii++){
    s->patch[ii] += data[ir++];
  }
  /* If ever needed: add fractional in +0.0000 to +0.0127 */
  for (ii = 0; ii<NPARTS*SYNTI2_NPARAMS; ii++){
    s->patch[ii] += data[ir++] / 10000.0;
  }
  /* If needed: multiply by integer value in -64 to +63 */
  for (ii = 0; ii<NPARTS*SYNTI2_NPARAMS; ii++){
    // FIXME: Format of this needs some consideration!!
    // s->patch[ii] *= (data[ir++] - 64);
    //    jack_info("%04d: %6.2f",ii, s->patch[ii]);
  }
}



/** Things may happen late or ahead of time. I don't know if that is
 *  what they call "jitter", but if it is, then this is where I jit
 *  and jat...
 */
static
void
synti2_handleInput(synti2_synth *s, 
                   synti2_conts *control,
                   int uptoframe){
    /* TODO: sane implementation */
  unsigned char midibuf[102400];/*FIXME: THINK about limits and make checks!!*/

  while ((0 <= control->nextframe) && (control->nextframe <= uptoframe)){
    synti2_conts_get(control, midibuf);
    if ((midibuf[0] & 0xf0) == 0x90){
      synti2_do_noteon(s, midibuf[0] & 0x0f, midibuf[1], midibuf[2]);
    } else if ((midibuf[0] & 0xf0) == 0x80) {
      synti2_do_noteoff(s, midibuf[0] & 0x0f, midibuf[1], midibuf[2]);
    } else if (midibuf[0] == 0xf0){
      synti2_do_receiveSysEx(s, midibuf+2);
    }else {
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
    /* When delta==0, the counter has gone a full cycle and stopped at
     * the maximum floating value, 1.0 and the envelope output stands
     * at the goal value ("set point"). */
  }
}


/* Advance the oscillator counters. Consider using the xmms assembly for these? */
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
    /* Outer loop for tihngs that are allowed some jitter: */
    /* Envelopes couldn't be evaluated here because vol envelope makes
     * evil (=very audible) jig jag 
     */

    /* Handle MIDI-ish controls if there are more of them waiting. */
    synti2_handleInput(s, control, iframe + NINNERLOOP);
    synti2_updateEnvelopeStages(s); /* move on if need be. */
    synti2_updateFrequencies(s);    /* frequency upd. */
    
    /* Inner loop runs the oscillators and audio generation. */
    for(ii=0;ii<NINNERLOOP;ii++){

      /* Could I put all counter upds in one big awesomely parallel loop */
      synti2_evalEnvelopes(s);
      synti2_evalCounters(s);
      
      /* Getting more realistic soon: */
      buffer[iframe+ii] = 0.0;

      for(iv=0;iv<NVOICES;iv++){
	// if (s->partofvoice[iv] < 0) continue; /* Unsounding. FIXME: (f1) */
	/* Produce sound :) First modulator, then carrier*/
        /* Worth using a wavetable? Probably.. Could do the first just
	   by bit-shifting the counter... */
	interm  = sin(2*M_PI* s->fc[iv*2+1]);
        interm *= (s->velocity[iv]/128.0) * (s->fenv[iv][1]);
	interm  = sin(2*M_PI* (s->fc[iv*2+0] + interm));
	interm *= s->fenv[iv][0];
	buffer[iframe+ii] += interm;
      }
      buffer[iframe+ii] /= NVOICES;
    }
  }
  s->global_framesdone += nframes;
}
