#include "synti2.h"
#include <math.h>
#include <limits.h>
#include <stdlib.h>

struct synti2_conts {
  int i;
  int ibuf;
  int nextframe;
  int frame[20000];        /*FIXME: think about limits */
  int msglen[20000];        /*FIXME: think about limits */
  unsigned char buf[20000]; /*FIXME: think about limits */
};


/** I just realized that unsigned ints will nicely loop around
 * (overflow) and as such they model an oscillator's phase pretty
 * nicely. Can I use them for other stuff as well.
 */
typedef struct counter {
  unsigned int val;
  unsigned int delta;
} counter;

/* Total number of "counters", i.e., oscillators/operators. */
#define NCOUNTERS 64

/* Total number of envelopes */
#define NENVS 128


/* Number of inner loop iterations (frame evaluations) between
 * evaluating "slow-motion stuff" such as MIDI input and
 * envelopes. Must be a divisor of the buffer length. Example: 48000/8
 * would yield 6000 Hz (or faster than midi wire rate) responsiveness.
 * Hmm.. will there be audible problems with "jagged volumes"...
 */
#define NINNERLOOP 8

#define MAX_COUNTER UINT_MAX

struct synti2_synth {
  float note2freq[128]; /* pre-computed frequencies of notes... */

  /* Oscillators are now modeled by integer counters (phase). */
  counter c[NCOUNTERS];
  float fc[NCOUNTERS];   /* store the values as floats right away. */

  /* Envelope progression also modeled by integer counters. */
  counter eprog[NENVS];
  float feprog[NCOUNTERS];   /* store the values as floats right away. */

  /* Envelope stages just a table? TODO: think.*/
  int estage[NENVS];

  unsigned long sr;

  /* Throw-away test stuff:*/
  double global_note;
  double global_amp;
  double global_fmi;
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
  double freq;

  while ((0 <= control->nextframe) && (control->nextframe <= uptoframe)){
    synti2_conts_get(control, midibuf);
    if ((midibuf[0] & 0xf0) == 0x90){
      /* note on */
      s->global_note = (float)midibuf[1];
      //s->global_amp = 0.5;
      /* TODO: trigger all envelopes.. */
      s->eprog[0].delta = (MAX_COUNTER / s->sr) / 2 * 1;
      s->eprog[1].delta = (MAX_COUNTER / s->sr) * 8 * 1; //NINNERLOOP;
      s->global_fmi = 2.0 * midibuf[2] / 128.0;
     
    } else if ((midibuf[0] & 0xf0) == 0x80) {
      /* note off */
      //s->global_amp = 0.0;
      if (s->eprog[0].delta != 0)
        s->eprog[0].delta = (MAX_COUNTER / s->sr) * 20 * 1; //NINNERLOOP;
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


void
synti2_render(synti2_synth *s, 
	      synti2_conts *control, 
	      synti2_smp_t *buffer,
	      int nframes)
{  
  int iframe, iouter, ii, ic;
  int note;
  float interm;
  float freq;
  
  for (iframe=0; iframe<nframes; iframe += NINNERLOOP){
    /* Handle MIDI-ish controls if there are more of them waiting. */
    synti2_handleInput(s, control, iframe + NINNERLOOP);

    /* Envelopes to be evaluated here. */
    //synti2_evalEnvelopes(s);   // vol envelope makes evil jig jag here

    /* Frequency computation... where to put it after all? */
    for (ic=0; ic<NCOUNTERS; ic++){
      //freq = 440.0 * pow(2.0, ((float)midibuf[1] - 69.0) / 12.0 );
      //freq = 440.0 * pow(2.0, (s->global_note - 69.0) / 12.0 );
      note = s->global_note; /* should be a floor!? */
      interm = (1.0 + 0.05946 * (s->global_note - note)); /* +cents.. */
      freq = interm * s->note2freq[note];

      s->c[0].delta = freq / s->sr * MAX_COUNTER;
      s->c[1].delta = (3*freq) / s->sr * MAX_COUNTER;
      s->c[2].delta = freq / s->sr * MAX_COUNTER; /* hack test */
    }

    
    /* Inner loop runs the oscillators and audio generation. */
    for(ii=0;ii<NINNERLOOP;ii++){

      synti2_evalEnvelopes(s);

      for(ic=0;ic<NCOUNTERS;ic++){
	s->c[ic].val += s->c[ic].delta;
	s->fc[ic] = (float) s->c[ic].val / MAX_COUNTER;
      }
      
      /* Just a hack for testing: */

      /* Produce sound :) First modulator, then carrier*/
      interm = sin(2*M_PI* s->fc[1]);
      //interm = sin(2*M_PI* (s->fc[0] + s->global_fmi * interm));
      interm = sin(2*M_PI* (s->fc[0] + s->global_fmi*(1-s->feprog[1]) * interm));
      /* These could be chained, couldn't they: */
      //      interm = sin(2*M_PI* (s->fc[2] + s->global_fmi * interm));
      interm *= (1.0-s->feprog[0]); //s->global_amp * .5;
      buffer[iframe+ii] = interm;
      
    }
  }
  s->global_framesdone += nframes;
}
