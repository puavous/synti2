/**
 * A re-implementation of the synti2 GUI. I hope this will be better
 * than the first piece of crap.
 *
 */

#define MIDI_INTERFACE_IS_JACK


/* fltk headers */
#include <FL/Fl.H>
#include <FL/Fl_Window.H>

/* Our own GUI windows and widgets */
#include "MainWindow.hpp"

/* Application logic */
#include "PatchBankHandler.hpp"
using synti2gui::PatchBankHandler;

/* jack headers */
#ifdef MIDI_INTERFACE_IS_JACK
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/control.h>
#endif

/* stdlib headers */
#include <iostream>

/* C headers */
#include <errno.h>
#include <signal.h>


using synti2gui::MainWindow;


/* ------------------------------- Jack interface -- */

#ifdef MIDI_INTERFACE_IS_JACK

#define MAX_EVENTS_IN_SLICE 80
#define RINGBUFSZ 0x10000

/** Global stuff for Jack connection kit use */
struct my_jack_T {
  jack_ringbuffer_t* rb;
  jack_client_t *client;
  jack_port_t *inmidi_port;
  jack_port_t *outmidi_port;
  unsigned long sr;
  const char * client_name;
} my_jack;
const char * default_name = "synti2editor";

bool jack_is_ok = false;
bool weHaveJack(){
  return jack_is_ok;
}

/** Signal handler that shuts down jack upon forced exit. */
static void signal_handler_for_jack_shutdown(int sig)
{
  jack_client_close(my_jack.client);
  std::cerr << "stop" << std::endl;
  exit(0);
}

/** send data to synth, if there is new stuff from the GUI. */
static int
process_with_jack(jack_nframes_t nframes, void *arg)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  //jack_midi_data_t *msg;
  //jack_midi_data_t *outdata;

  void *midi_in_buffer = (void*)jack_port_get_buffer (my_jack.inmidi_port, nframes);
  void *midi_out_buffer = (void*)jack_port_get_buffer (my_jack.outmidi_port, nframes);

  unsigned char sysex_buf[2000];
  size_t sz;

  int nsent;

  jack_midi_clear_buffer(midi_out_buffer); 
  nev = jack_midi_get_event_count(midi_in_buffer);

  /* Read from UI thread. FIXME: think if synchronization issues persist? */

  nsent = 0;
  //while (jack_ringbuffer_read_space (global_rb) >= sizeof(s2ed_msg_t)) {
  for(;;){
    /* This should be a call to a "has_message()" */
    if (jack_ringbuffer_read_space (my_jack.rb) < sizeof(size_t)) break;
    jack_ringbuffer_peek (my_jack.rb, (char*)&sz, sizeof(size_t));
    if (jack_ringbuffer_read_space (my_jack.rb) < sz) break;

    /* If there is no space to forward the event, we'll have to
     * wait. Leave the message untouched in the ringbuffer front.
     */
    if (jack_midi_max_event_size(midi_out_buffer) < sz) break;

    /* Read size and contents. */
    jack_ringbuffer_read (my_jack.rb, (char*)&sz, sizeof(size_t));
    jack_ringbuffer_read (my_jack.rb, (char*)&sysex_buf, sz);

    /*printf("jack event size %d\n", 
      jack_midi_max_event_size(midi_out_buffer));*/

    if (jack_midi_event_write(midi_out_buffer, 0, sysex_buf, sz) != 0){
      std::cerr << "Error writing event " << nsent << std::endl;
      break;
    }

    /* The current synth engine cannot handle very many events in
       real-time. There is a crashing bug (git branch send_all_hang0
       for trying to sort it out): sending few patches at once works
       but with more patches at once the receiving synth crashes. It
       is currently fixed by this kludge that only allows a hard-coded
       number of events further from the ringbuffer on each process()
       call. The real reason of the crash is not known to me as of
       yet. Should make some tests to see if the data is transmitted
       completely at all or if the problem lies deeper in the
       processing. Anyway it seems to be a synth problem that must be
       considered a limitation as of now. The evil thing is that I
       don't really know why there needs to be this limitation.. */
    nsent ++;
    if (nsent > MAX_EVENTS_IN_SLICE) break;
  }

  /* Handle incoming. TODO: The idea was that a midi control surface
     could be used for real-time editing... Same message structure
     could be used for both directions, but need another ringbuffer
     for the incoming messages.
  */
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {
      /*jack_info("k ");*/
      /*FIXME: zadaadaadaa. */
    } else {
      break;
    }
  }
  return 0;
}


void init_jack(const char * client_name){
  jack_status_t status;
  my_jack.client_name = client_name;

  /* Initial Jack setup. Open (=create?) a client. */
  if ((my_jack.client = jack_client_open (
      my_jack.client_name, JackNoStartServer, &status)) == 0)
  {
    std::cerr << "jack server not running?" << std::endl; 
    jack_is_ok = false;
    return;
  }

  /* Set up process callback */
  jack_set_process_callback (my_jack.client, process_with_jack, 0);

  /* Try to create midi ports */
  my_jack.inmidi_port = jack_port_register (my_jack.client, "iportti", 
                                    JACK_DEFAULT_MIDI_TYPE, 
                                    JackPortIsInput, 0);
  my_jack.outmidi_port = jack_port_register (my_jack.client, "oportti", 
                                     JACK_DEFAULT_MIDI_TYPE, 
                                     JackPortIsOutput, 0);

  /* Set up a ringbuffer for communication from GUI to process().*/
  if ((my_jack.rb = jack_ringbuffer_create(RINGBUFSZ))==NULL){
    std::cerr << "Could not allocate ringbuffer. " << std::endl;
    exit(2);
  }
  std::cerr << "Ringbuffer created with write space " 
            << jack_ringbuffer_write_space (my_jack.rb)
            << std::endl;

  /* Activate client. */
  if (jack_activate (my_jack.client)) {
    std::cerr << "cannot activate client" << std::endl;
    jack_is_ok = false;
    return;
  }
  
  /* install a signal handler to properly quit jack client */
#ifdef WIN32
  signal(SIGINT, signal_handler_for_jack_shutdown);
  signal(SIGABRT, signal_handler_for_jack_shutdown);
  signal(SIGTERM, signal_handler_for_jack_shutdown);
#else
  signal(SIGQUIT, signal_handler_for_jack_shutdown);
  signal(SIGTERM, signal_handler_for_jack_shutdown);
  signal(SIGHUP, signal_handler_for_jack_shutdown);
  signal(SIGINT, signal_handler_for_jack_shutdown);
#endif

  jack_is_ok = true;

}


/* --------------------------- End Jack interface -- */
#endif


#ifdef MIDI_INTERFACE_IS_JACK
int main(int argc, char **argv) {
  int retval = 2;

  if (argc < 2) {return 1;}
  char *patchdes_fname = argv[1];

  init_jack(default_name);

  std::cerr << "synti2gui main(): note: Jack init " 
            << (weHaveJack()?"succeeded":"failed") << "." << std::endl;

  /* FIXME: Better implementation of these: */
  synti2::Patchtool *pt = new synti2::Patchtool(patchdes_fname);
  synti2::PatchBank *pbank = pt->makePatchBank(14);

  PatchBankHandler h(pbank, NULL, NULL);
  Fl_Window *window = new MainWindow(1000,740,&h);

  window->show(argc, argv);

#if 0
  midimap = new synti2::MidiMap();
  widgets_to_reflect_reality();
#endif

  retval = Fl::run();

  if (weHaveJack()) {jack_ringbuffer_free(my_jack.rb);}
  if (weHaveJack()) {jack_client_close(my_jack.client);}

  if (pbank != NULL) free(pbank);
  if (pt != NULL) free(pt);
  
  return retval;
}
#else
#error "Only Jack Midi is supported as of now."
#endif
