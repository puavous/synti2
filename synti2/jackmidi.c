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
synti2_player *global_player;

/* FIXME: remove, after this works properly in the SDL test*/
int global_hack_playeronly = 0;
extern unsigned char hacksong_data[];
extern unsigned int hacksong_length;


synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

/* Test patch from the hack script: */
extern unsigned char hack_patch_sysex[];
extern int hack_patch_sysex_length;

static void signal_handler(int sig)
{
  jack_client_close(client);
  fprintf(stderr, "signal received, exiting ...\n");
  exit(0);
}


static void
process_audio (jack_nframes_t nframes) 
{
  int i;
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
    synti2_conts_store(global_cont, 0, hack_patch_sysex, hack_patch_sysex_length);
  }
  
  if (global_hack_playeronly == 0) {
    nev = jack_midi_get_event_count(midi_in_buffer);
    for (i=0;i<nev;i++){
      if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {
	synti2_conts_store(global_cont, ev.time, ev.buffer, ev.size);
      } else {
	break;
      }
    }
  } else {
    /* FIXME: Only for testing; to be removed.. */
    
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

  if ((argc >= 2) && (strcmp(argv[1],"-p")==0)){
    global_hack_playeronly = 1;
  }

  /*printf("Length = %d",hack_patch_sysex_length); fflush(stdout);
  printf("Buf = %d %d %d %d",hack_patch_sysex[0],
	 hack_patch_sysex[1],hack_patch_sysex[2],hack_patch_sysex[3]); fflush(stdout);
  printf("First = %x",*((int*)hack_patch_sysex)); fflush(stdout);
  */


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

  /*hack. FIXME: remove.*/
  global_player = synti2_player_create(hacksong_data, hacksong_length, sr);
  if (global_player == NULL){
    fprintf(stderr, "Haist \n");
    goto error;
  }
  
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
