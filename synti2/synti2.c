#include "synti2.h"
#include <math.h>
#include <stdlib.h>

struct synti2_conts {
  int i;
  int ibuf;
  int nextframe;
  int frame[20000];        /*FIXME: think about limits */
  int msglen[20000];        /*FIXME: think about limits */
  unsigned char buf[20000]; /*FIXME: think about limits */
};


struct synti2_synth {
  unsigned long sr;
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
  if (s == NULL) return;
  s->sr = sr;
}


void
synti2_render(synti2_synth *s, 
	      synti2_conts *control, 
	      synti2_smp_t *buffer,
	      int nframes)
{
  
  static double global_freq = 440;
  static double global_amp = 0.0;
  static double global_phase = 0.0;
  static double global_dphase;
  static double global_phase2 = 0.0;
  static double global_dphase2;
  static double global_fmi = 0.0;
  static long int global_framesdone = 0;
  global_dphase =  global_freq / s->sr;
  unsigned char midibuf[1024];
  
  int i;
  
  for (i=0; i<nframes; i++){
    
    /* Handle MIDI-ish controls if there are more of them waiting. */
    /* TODO: sane implementation */
    while (control->nextframe == i){
      synti2_conts_get(control, midibuf);
      if ((midibuf[0] & 0xf0) == 0x90){
        /* note on */
        global_freq = 440.0 * pow(2.0, ((float)midibuf[1] - 69.0) / 12.0 );
	global_amp = 0.5;
	global_dphase =  global_freq / s->sr;
        global_fmi = 1.0 * midibuf[2] / 128.0;
      } else if ((midibuf[0] & 0xf0) == 0x80) {
        /* note off */
	global_amp = 0.0;
      }else {
      }
    }

    //global_amp = 0.5;

    /* Produce sound :) */
    //    buffer[i] = global_amp * .5 * sin(2*M_PI*global_phase);
    buffer[i] = sin(2*M_PI* (2.1*global_phase)); /* modulator osc*/ 
    // final:
    buffer[i] = global_amp * .5 * sin(2*M_PI* (global_phase + global_fmi * buffer[i]));
    global_phase += global_dphase;
    if (global_phase > 1.0) global_phase -= floor(global_phase);
  }
  global_framesdone += nframes;
}
