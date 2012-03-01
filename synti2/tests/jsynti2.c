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

synti2_synth *global_synth;
synti2_player *global_player;

/* FIXME: remove, after this works properly in the SDL test.. maybe? */
int global_hack_playeronly = 0;
extern unsigned char hacksong_data[];

synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

/* Test patch */
extern unsigned char patch_sysex[];
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

  /* Transform MIDI messages from native type (jack) to our own format */
  if (global_hack_playeronly == 0){
    synti2_read_jack_midi(global_synth, inmidi_port, nframes);
  }
  /* Call our own synth engine and convert samples to native type (jack) */
  synti2_render(global_synth, global_buffer, nframes); 

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
  /*global_synth = synti2_create(sr, patch_sysex, hacksong_data);*/
  global_synth = synti2_create(sr, patch_sysex, hacksong_data);

  if (global_synth == NULL){
    fprintf (stderr, "Couldn't allocate synti-kaksi \n");
    goto error;
  };
  
  /* Now we activate our new client. */
  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client\n");
    goto error;
  }
  
  /* install a signal handler to properly quit jack client */
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
