/* Of course, I start with an example program... which is metro.c from
   jack examples this time :). Mutilated to an unrecognizable
   state. Thanks to Anthony Van Groningen for the nice and helpful
   example.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <math.h>
#include <signal.h>
#include <string.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "synti2.h"

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *output_portL;
jack_port_t *output_portR;
jack_port_t *inmidi_port;
unsigned long sr;
char * client_name = "beeper";

synti2_conts *global_cont;
synti2_synth *global_synth;

synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

#define PATCHMSGSIZE (3 + 16 * (SYNTI2_NPARAMS * 4))
unsigned char hack_patchbuffer[PATCHMSGSIZE];
int hack_patchbuflen;

const float hack_examplesound[16* SYNTI2_NPARAMS] = 
  {
    /* Four envelopes for 16 channels: */
   0.01, 1.00,   0.10, 0.50,   1.27, 0.05,   0.00, 0.05,   0.20, 0.0,
   0.00, 1.27,   0.02, 0.10,   0.03, 0.00,   0.00, 0.00,   0.02, 0.0,
   0.00, 36.00,  0.02, 24.0,   0.07, 0.00,   0.00, 0.00,   0.02, 0.0,
   0.00, 1.27,   0.2, 0.4,   0.4, 0.1,   0.40, 0.2,   0.20, 0.1, 

   0.01, 1.00,   0.10, 0.50,   3.00, 0.00,   0.00, 0.00,   0.10, 0.0,
   0.00, 2.27,   0.60, 0.80,   1.25, 0.05,   3.00, 0.00,   2.02, 0.0,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,

   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,   0.00, 0.00,
};

void hack_toSysEx()
{
  int i, par, pt;
  int d;
  float val;
  float decim;
  i=0;
  hack_patchbuffer[i++]=0xf0; /* sysex*/
  hack_patchbuffer[i++]=0x00; /* manuf. id*/
  /* FIXME: proper*/
  for (pt=0; pt<16; pt++){
    for (par=0;par<SYNTI2_NPARAMS;par++){
      /* TODO: A function to convert floats to the (as of yet
	 unplanned) synti2 representation easily */
      val = hack_examplesound[pt*SYNTI2_NPARAMS+par];
      if ((0.0<=val) && (val<=1.27)){
	hack_patchbuffer[i] = val * 100;
	hack_patchbuffer[i+1*(16*SYNTI2_NPARAMS)] = 0x00;
	hack_patchbuffer[i+2*(16*SYNTI2_NPARAMS)] = 0x00;
	hack_patchbuffer[i+3*(16*SYNTI2_NPARAMS)] = 0x00;
      } else if ((1.27<=val) && (val<=127.99)){
	d = val;
	decim = val - d;
	hack_patchbuffer[i] = decim * 100;
	hack_patchbuffer[i+1*(16*SYNTI2_NPARAMS)] = d;
	hack_patchbuffer[i+2*(16*SYNTI2_NPARAMS)] = 0x00;
	hack_patchbuffer[i+3*(16*SYNTI2_NPARAMS)] = 0x00;
      } else {
	hack_patchbuffer[i] = val * 100;
	hack_patchbuffer[i+1*(16*SYNTI2_NPARAMS)] = 0x00;
	hack_patchbuffer[i+2*(16*SYNTI2_NPARAMS)] = 0x00;
	hack_patchbuffer[i+3*(16*SYNTI2_NPARAMS)] = 0x00;
      }
      i++;
    }
  }
  
  hack_patchbuffer[PATCHMSGSIZE-1] = 0xf7;
  hack_patchbuflen = PATCHMSGSIZE;
}


static void signal_handler(int sig)
{
  jack_client_close(client);
  fprintf(stderr, "signal received, exiting ...\n");
  exit(0);
}


static void
process_audio (jack_nframes_t nframes) 
{
  int i, par;
  sample_t *bufferL = (sample_t *) jack_port_get_buffer (output_portL, nframes);
  sample_t *bufferR = (sample_t *) jack_port_get_buffer (output_portR, nframes);

  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);
  
  jack_midi_event_t ev;
  jack_nframes_t nev;

  static int hack_first = 1;

  /* Transform MIDI messages from native type (jack) to our own format */
  synti2_conts_reset(global_cont);

  if (hack_first != 0) {
    hack_first = 0;
    synti2_conts_store(global_cont, 0, hack_patchbuffer, hack_patchbuflen);
  }
  
  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {
      synti2_conts_store(global_cont, ev.time, ev.buffer, ev.size);
    } else {
      break;
    }
  }

  synti2_conts_start(global_cont);

  /* Call our own synth engine and convert samples to native type (jack) */
  synti2_render(global_synth, global_cont,
		global_buffer, nframes); 
  for (i=0;i<nframes;i++){
    bufferL[i] = global_buffer[i];
    bufferR[i] = global_buffer[i];
  }
}

static int
process (jack_nframes_t nframes, void *arg)
{
  /* FIXME: Should have our own code here, arg could be our data?*/
  process_audio (nframes);
  return 0;
}

int
main (int argc, char *argv[])
{
  jack_status_t status;

  /* hack. */
  hack_toSysEx();
  
  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, JackNoStartServer, &status)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    return 1;
  }
  /* Set up process callback */
  jack_set_process_callback (client, process, 0);
  
  /* Create an output port */
  output_portL = jack_port_register (client, "bportL", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  output_portR = jack_port_register (client, "bportR", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  /* Try to create a midi port */
  inmidi_port = jack_port_register (client, "iportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

  /* Get sample rate. */
  sr = jack_get_sample_rate (client);

  /* My own soft synth to be created. */
  global_synth = synti2_create(sr);
  global_cont = synti2_conts_create();
  if ((global_synth == NULL) || (global_cont == NULL)){
    fprintf (stderr, "Couldn't allocate synti-kaksi \n");
    goto error;
  };

  
  /* Now we activate our new client. */
  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client\n");
    goto error;
  }
  
  /* install a signal handler to properly quits jack client */
#ifdef WIN32
  signal(SIGINT, signal_handler);
  signal(SIGABRT, signal_handler);
  signal(SIGTERM, signal_handler);
#else
  signal(SIGQUIT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
#endif
  
  /* run until interrupted */
  while (1) {
#ifdef WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
  };
  
  jack_client_close(client);
  
 error:
  exit (0);
}
