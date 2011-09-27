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

/* Number of "counters", i.e., oscillators/operators */
#define NCOUNTERS 64

#define MAX_COUNTER UINT_MAX

struct synti2_synth {
  unsigned long sr;

  counter c[NCOUNTERS];
  /* Throw-away test stuff:*/
  double global_freq;
  double global_amp;
  double global_phase;
  double global_dphase;
  double global_phase2;
  double global_dphase2;
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
  for (i=0;i < control->msglen[control->i];i++){
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
  s = calloc (1, sizeof(synti2_synth));
  if (s == NULL) return s;
  s->sr = sr;
  return s;
}


void
synti2_render(synti2_synth *s, 
	      synti2_conts *control, 
	      synti2_smp_t *buffer,
	      int nframes)
{
  //s->global_dphase =  s->global_freq / s->sr;
  unsigned char midibuf[1024];
  double freq;
  
  int i, ic;
  
  for (i=0; i<nframes; i++){
    
    /* Handle MIDI-ish controls if there are more of them waiting. */
    /* TODO: sane implementation */
    while (control->nextframe == i){
      synti2_conts_get(control, midibuf);
      if ((midibuf[0] & 0xf0) == 0x90){
        /* note on */
        //s->global_freq = 440.0 * pow(2.0, ((float)midibuf[1] - 69.0) / 12.0 );
	freq = 440.0 * pow(2.0, ((float)midibuf[1] - 69.0) / 12.0 );
	s->global_amp = 0.5;
	s->c[0].delta = freq / s->sr * MAX_COUNTER;
	s->c[1].delta = (3*freq) / s->sr * MAX_COUNTER;
	//s->global_dphase =  s->global_freq / s->sr;
        s->global_fmi = 2.0 * midibuf[2] / 128.0;
      } else if ((midibuf[0] & 0xf0) == 0x80) {
        /* note off */
	s->global_amp = 0.0;
      }else {
      }
    }

    //    global_amp = 0.

    for(ic=0;ic<NCOUNTERS;ic++){
      s->c[ic].val += s->c[ic].delta;
    }

    /* Produce sound :) */
    //    buffer[i] = global_amp * .5 * sin(2*M_PI*global_phase);
    //buffer[i] = sin(2*M_PI* (1.0*s->global_phase)); /* modulator osc*/ 
    // final:
    //buffer[i] = s->global_amp * .5 * sin(2*M_PI* (s->global_phase + s->global_fmi * buffer[i]));
    //s->global_phase += s->global_dphase;
    //if (s->global_phase > 1.0) s->global_phase -= floor(s->global_phase);

    buffer[i] = sin(2*M_PI* ((float) s->c[1].val / MAX_COUNTER)); /* modulator osc*/ 
    buffer[i] = sin(2*M_PI* ((float) s->c[0].val / MAX_COUNTER)
		    + s->global_fmi * buffer[i]);
    buffer[i] *= s->global_amp * .5;

  }
  s->global_framesdone += nframes;
}
