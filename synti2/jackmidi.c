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

typedef jack_default_audio_sample_t sample_t;

jack_client_t *client;
jack_port_t *output_port;
jack_port_t *inmidi_port;
unsigned long sr;
char * client_name = "beeper";

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
  sample_t *buffer = (sample_t *) jack_port_get_buffer (output_port, nframes);

  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);
  
  static double global_freq = 440;
  static double global_amp = 0.0;
  static double global_phase = 0.0;
  static double global_dphase;
  static long int global_framesdone = 0;
  global_dphase =  global_freq / sr;

  jack_midi_event_t ev;
  jack_nframes_t nev;

  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {
      if ((ev.buffer[0] & 0xf0) == 0x90){
        /* note on */
        global_freq = 440.0 * pow(2.0, ((float)ev.buffer[1] - 69.0) / 12.0 );
	global_amp = 0.5;
      } else if ((ev.buffer[0] & 0xf0) == 0x80) {
        /* note off */
	global_amp = 0.0;
      }else {
      }
    } else {
      break;
    }
  }
  
  for (i=0; i<nframes; i++){
    buffer[i] = global_amp * .5 * sin(2*M_PI*global_phase);
    global_phase += global_dphase;
    if (global_phase > 1.0) global_phase -= floor(global_phase);
  }
  global_framesdone += nframes;
}

static int
process (jack_nframes_t nframes, void *arg)
{
  process_audio (nframes);
  return 0;
}

int
main (int argc, char *argv[])
{
  jack_status_t status;
  
  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, JackNoStartServer, &status)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    return 1;
  }
  /* Set up process callback */
  jack_set_process_callback (client, process, 0);
  
  /* Create an output port */
  output_port = jack_port_register (client, "bport", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  /* Try to create a midi port */
  inmidi_port = jack_port_register (client, "iportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

  /* Get sample rate. */
  sr = jack_get_sample_rate (client);
  
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
