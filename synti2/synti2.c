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

/* Total number of "counters", i.e., oscillators/operators. */
#define NCOUNTERS 64

/* Maximum value of the counter type */
#define MAX_COUNTER UINT_MAX

/* Total number of envelopes */
#define NENVS 128

/* Number of inner loop iterations (frame evaluations) between
 * evaluating "slow-motion stuff" such as MIDI input and
 * envelopes. Must be a divisor of the buffer length. Example: 48000/8
 * would yield 6000 Hz (or faster than midi wire rate) responsiveness.
 * Hmm.. will there be audible problems with "jagged volumes" if I
 * put the volume envelope outside of the inner loop... So I'll keep at
 * least the envelope progression counter code inside.
 */
#define NINNERLOOP 8

/** I just realized that unsigned ints will nicely loop around
 * (overflow) and as such they model an oscillator's phase pretty
 * nicely. Can I use them for other stuff as well? Seem fit for
 * envelopes with only a little extra (clamping and stop flag)
 */
typedef struct counter {
  unsigned int val;
  unsigned int delta;
} counter;

#define SYNTI2_NPARAMS 128

#define NVOICES (NCOUNTERS/2)

typedef struct synti2_part {
  int par[SYNTI2_NPARAMS];  /* The sound parameters*/
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
  counter eprog[NENVS];
  float feprog[NCOUNTERS];   /* store the values as floats right away. */

  /* Envelope stages just a table? TODO: think.*/
  int estage[NENVS];

  int partofvoice[NVOICES];  /* which part has triggered each "voice";
				-1 (should we use zero instead?) means
				that the voice is not alive. */

  int note[NVOICES];
  int velocity[NVOICES];

  /* The parts. Sixteen as in the MIDI standard. */
  synti2_part part[16];   /* FIXME: I want to call this channel!!!*/

  unsigned long sr;

  /* Throw-away test stuff:*/
  long int global_framesdone;
};

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
  int inote;

  s = calloc (1, sizeof(synti2_synth));
  if (s == NULL) return s;
  s->sr = sr;

  /* Create a note-to-frequency look-up table (cents need interpolation). 
   * TODO: Which is more efficient / small code: linear interpolation 
   * on every eval or make a lookup table on the cent scale...
   */
  for(inote=0; inote<128; inote++){
    s->note2freq[inote] = 440.0 * pow(2.0, ((float)inote - 69.0) / 12.0 );
  }

  for (inote=0; inote<NVOICES; inote++){
    s->partofvoice[inote] = -1;
  }
  return s;
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
  unsigned char midibuf[1024];
  int part, note, vel, voice;
  int freevoice;

  while ((0 <= control->nextframe) && (control->nextframe <= uptoframe)){
    synti2_conts_get(control, midibuf);
    if ((midibuf[0] & 0xf0) == 0x90){
      for(freevoice=0; freevoice <= NVOICES; freevoice++){
	if (s->partofvoice[freevoice] < 0) break;
      }
      if (freevoice==NVOICES) continue; /* Cannot play new note! */
      /* (Could actually force the last voice to play anyway!!) */

      /* note on */
      part = midibuf[0] & 0x0f;
      note = midibuf[1];
      vel  = midibuf[2];
      
      s->part[part].voiceofkey[note] = freevoice;
      s->partofvoice[freevoice] = part;
      s->note[freevoice] = note;
      s->velocity[freevoice] = vel;

      /* TODO: trigger all envelopes according to patch data.. */
      s->eprog[freevoice*4+0].delta = (MAX_COUNTER / s->sr) / 7 * 1;
      s->eprog[freevoice*4+1].delta = (MAX_COUNTER / s->sr) / 1 * 1;
      s->eprog[freevoice*4+2].delta = (MAX_COUNTER / s->sr) * 3 * 1;
      s->eprog[freevoice*4+3].delta = (MAX_COUNTER / s->sr) * 3 * 1;
    } else if ((midibuf[0] & 0xf0) == 0x80) {
      /* note off */
      part = midibuf[0] & 0x0f;
      note = midibuf[1];
      vel  = midibuf[2];

      freevoice = s->part[part].voiceofkey[note];
      //s->part[part].voiceofkey[note] = 0; /*FIXME: think*/
      s->partofvoice[freevoice] = -1; /* FIXME: (f1) Voice should be
					 freed no earlier than when
					 the envelope release stage
					 ends!! */
      /* TODO: release all envelopes.. */
      s->eprog[freevoice*4+0].delta = (MAX_COUNTER / s->sr) * 20 * 1;
      s->eprog[freevoice*4+1].delta = (MAX_COUNTER / s->sr) * 20 * 1;
      s->eprog[freevoice*4+2].delta = (MAX_COUNTER / s->sr) * 20 * 1;
      s->eprog[freevoice*4+3].delta = (MAX_COUNTER / s->sr) * 20 * 1;
      //      if (s->eprog[0].delta != 0)
      //        s->eprog[2].delta = s->eprog[0].delta = (MAX_COUNTER / s->sr) * 20 * 1; //NINNERLOOP;

    }else {
    }
  }
}

/** TODO: Implement better? */
static
void
synti2_evalEnvelopes(synti2_synth *s){  
  int ic;
  unsigned int prev;
  /* TODO: Implement. */

  /* A counter is triggered just by setting delta positive. */
  for(ic=0;ic<NENVS; ic++){
    prev = s->eprog[ic].val;
    s->eprog[ic].val += s->eprog[ic].delta;  /* counts from 0 to MAX */
    if (s->eprog[ic].val <= prev){
      s->eprog[ic].delta = 0;  /* Counter stops after full circle   */
      s->feprog[ic] = 1.0;     /* and the floating value is clamped */ 
    } else {
      s->feprog[ic] = (float) s->eprog[ic].val / MAX_COUNTER;
    }
  }
  /* When delta==0, the counter has gone a full circle and stopped at
   * maximum floating value (1.0)
   */

}

/** Converts note values to counter deltas. */
static
void
synti2_updateFrequencies(synti2_synth *s){
  int iv, note;
  float interm, freq;
  /* Frequency computation... where to put it after all? */
  for (iv=0; iv<NVOICES; iv++){
    note = s->note[iv];  /**/
    interm = (1.0 + 0.05946 * ((s->note[iv]) - note)); /* +cents.. */
    freq = interm * s->note2freq[note];
    
    s->c[iv*2].delta = freq / s->sr * MAX_COUNTER;
    s->c[iv*2+1].delta = (2*freq) / s->sr * MAX_COUNTER; /*hack test*/
    //s->c[2].delta = freq / s->sr * MAX_COUNTER; /* hack test */
  }
}

/** Updates envelope stages as the patch data dictates. */
static
void
synti2_updateEnvelopeStages(synti2_synth *s){
  int iv;
  for(iv=0; iv<NVOICES; iv++){
    
  }
}


void
synti2_render(synti2_synth *s, 
	      synti2_conts *control, 
	      synti2_smp_t *buffer,
	      int nframes)
{  
  int iframe, ii, ic, iv;
  float interm;
  
  for (iframe=0; iframe<nframes; iframe += NINNERLOOP){
    /* Outer loop for tihngs that are allowed some jitter: */
    /* Envelopes couldn't be evaluated here because vol envelope makes
     * evil (=very audible) jig jag 
     */

    /* Handle MIDI-ish controls if there are more of them waiting. */
    synti2_handleInput(s, control, iframe + NINNERLOOP);
    synti2_updateFrequencies(s);    /* frequency upd. */
    synti2_updateEnvelopeStages(s); /* move on if need be. */
    
    /* Inner loop runs the oscillators and audio generation. */
    for(ii=0;ii<NINNERLOOP;ii++){

      /* Could I put all counter upds in one big awesomely parallel loop */
      synti2_evalEnvelopes(s);

      for(ic=0;ic<NCOUNTERS;ic++){
	s->c[ic].val += s->c[ic].delta;
	s->fc[ic] = (float) s->c[ic].val / MAX_COUNTER;
      }
      
      /* Getting more realistic soon: */
      buffer[iframe+ii] = 0.0;

      for(iv=0;iv<NVOICES;iv++){
	if (s->partofvoice[iv] < 0) continue; /* FIXME: (f1) */
	/* Produce sound :) First modulator, then carrier*/
	interm = sin(2*M_PI* s->fc[iv*2+1]);
	//interm *= (1.0-s->feprog[iv*4+1]); /* Could have this here...*/
	interm = sin(2*M_PI* (s->fc[iv*2+0] 
			      + (1.5*s->velocity[iv]/128.0)*(1-s->feprog[iv*4+1]) * interm));
	interm *= (1.0-s->feprog[iv*4+0]);
	buffer[iframe+ii] += interm;
      }
      buffer[iframe+ii] /= NVOICES;
    }
  }
  s->global_framesdone += nframes;
}
