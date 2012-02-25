/* A jack midi utility to make midi transformations on the fly.
 *
 * For experimenting with the realtime synth while it is being
 * developed.
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
jack_port_t *inmidi_port;
jack_port_t *outmidi_port;
unsigned long sr;
char * client_name = "jmiditrans";


int hack_channel_table[16][128] = {
  /*Chan 0  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 1  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 2  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 3  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 4  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 5  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 6  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 7  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 8  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan 9  Oct -5 */ {0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -4 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -3 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -2 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct -1 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  0 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  1 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  2 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  3 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  4 */  0,0,1,1,1,3,2,3,2,3,4,3,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan a  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan b  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan c  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan d  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan e  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},

  /*Chan f  Oct -5 */ {0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct -1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  0 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  1 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  2 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  3 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  4 */  0,0,0,0,0,0,0,0,0,0,0,0,
  /*        Oct  5 */  0,0,0,0,0,0,0,0},
};
//synti2_smp_t global_buffer[20000]; /* FIXME: limits? */

static void signal_handler(int sig)
{
  jack_client_close(client);
  fprintf(stderr, "stop\n");
  exit(0);
}


static void
debug_print_ev(jack_midi_event_t *ev){
  jack_info("Msg (len %d) at time %d:", ev->size, ev->time);
  if (ev->size==3) jack_info("  3byte: %02x %02x %02x", ev->buffer[0],
                             ev->buffer[1], ev->buffer[2]);
}

/** Just a dirty hack to spread drums out to different channels. */
static void
hack_channel(jack_midi_event_t *ev){
  int nib1, nib2, note;
  nib1 = ev->buffer[0] >> 4;
  nib2 = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  nib2 = nib2+hack_channel_table[nib2][note];

  ev->buffer[0] = (nib1<<4) + nib2;
}


/** MIDI filtering is the only thing this client is required to do. */
static void
process_audio (jack_nframes_t nframes) 
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  jack_midi_data_t *msg;

  jack_midi_data_t *outdata;
  /*
  outdata = jack_midi_event_reserve(void *midi_out_buffer,
		jack_nframes_t  	time,
		size_t  	data_size 
	) 	
  */
  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);
  void *midi_out_buffer  = (void *) jack_port_get_buffer (outmidi_port, nframes);


  jack_midi_clear_buffer(midi_out_buffer); 
  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {
      /*debug_print_ev(&ev);*/
      hack_channel(&ev);
      /*debug_print_ev(&ev);*/
      jack_midi_event_write(midi_out_buffer, ev.time, ev.buffer, ev.size);
    } else {
      break;
    }
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

  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, JackNoStartServer, &status)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    return 1;
  }
  /* Set up process callback */
  jack_set_process_callback (client, process, 0);
  

  /* Try to create a midi port */
  inmidi_port = jack_port_register (client, "iportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  outmidi_port = jack_port_register (client, "oportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

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
