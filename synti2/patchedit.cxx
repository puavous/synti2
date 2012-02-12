/**
 * A small program for creating synti2 patches, and sending them out
 * via jack MIDI.
 *
 * UI idea:
 *
 *  File: experiments.s2patch
 *  Patch: __  <-- can be 0-f
 *  [Copy from...]
 *
 *  Env1K1T K1L   K2T
 *  [0.00] [1.27] [0.20]  ... [101.19] <-- textboxes with float vals.
 *  Env2K1T K1L
 *  [0.00] [1.27] [0.20]  ... [101.19] <-- textboxes with float vals.
 *  ...
 *  Dlylen Dlyfd
 *  [0.00] [1.27] [0.20]  ... [101.19] <-- textboxes with float vals.
 *
 *  [Save]
 *
 *  Clicking on a value box opens a dialog that allows MIDI input to
 *  be mapped into the box:
 *
 *   (o)  Coarse CC=___  [learn] min=[0] max=[127]
 *   ( ) +Fine   CC=___  [learn] min=[0.00] max=[1.27]
 *   [OK] [Cancel]
 */

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Dial.H>

#include <iostream>

extern unsigned char hack_patch_sysex;

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <signal.h>

#define RINGBUFSZ 0x10000


jack_ringbuffer_t* global_rb;

jack_client_t *client;
jack_port_t *inmidi_port;
jack_port_t *outmidi_port;
unsigned long sr;
char * client_name = "synti2editor";

static void signal_handler(int sig)
{
  jack_client_close(client);
  std::cerr << "stop" << std::endl;
  exit(0);
}

/** MIDI filtering is the only thing this client is required to do. */
static int
process (jack_nframes_t nframes, void *arg)
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
      /*FIXME: zadaadaadaa. */
      /*debug_print_ev(&ev);*/
      //hack_channel(&ev);
      /*debug_print_ev(&ev);*/
      //jack_midi_event_write(midi_out_buffer, ev.time, ev.buffer, ev.size);
    } else {
      break;
    }
  }
  return 0;
}





/** Sends data to MIDI. (FIXME: when it's done) */
void cb_send(Fl_Widget*, void*){
  std::cout << "ja tuota." << std::endl;
  char* notuota = "notuota";

  /* FIXME: Implement actual writing to the buffer. */
  size_t nwrit = jack_ringbuffer_write (global_rb, notuota, sizeof(notuota));
}

int main(int argc, char **argv) {

  jack_status_t status;

  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, JackNoStartServer, &status)) == 0) {
    std::cerr << "jack server not running?" << std::endl; return 1;
  }
  /* Set up process callback */
  jack_set_process_callback (client, process, 0);

  /* Try to create a midi port */
  inmidi_port = jack_port_register (client, "iportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  outmidi_port = jack_port_register (client, "oportti", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

  /* Now we activate our new client. */
  if (jack_activate (client)) {
    std::cerr << "cannot activate client" << std::endl;
    exit(1);
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


  Fl_Window *window = new Fl_Window(600,180);
  Fl_Button *box = new Fl_Button(20,40,260,100,"S&end");
  box->callback(cb_send);
  box->box(FL_UP_BOX); box->labelsize(36); 
  box->labeltype(FL_SHADOW_LABEL);


  Fl_Dial *dial = new Fl_Dial(320,40,100,100,"Kissa123");
  dial->align(FL_ALIGN_CENTER);
  Fl_Dial *dial2 = new Fl_Dial(420,40,100,100,"@->| ja joo");
  dial2->type(FL_LINE_DIAL);
  window->end();
  window->show(argc, argv);

  if ((global_rb = jack_ringbuffer_create(RINGBUFSZ))==NULL){
    std::cerr << "Die." << std::endl;
    exit(2);
  }
  return Fl::run();
  jack_ringbuffer_free(global_rb);
  jack_client_close(client);
  
 error:
  exit (0);

}
