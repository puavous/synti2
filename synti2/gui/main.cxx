/**
 * A re-implementation of the synti2 GUI. I hope this will be better
 * than the first piece of crap.
 *
 */

/* jack headers */
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/control.h>

/* stdlib headers */
#include <iostream>

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

#if 0
  /* Set up process callback */
  jack_set_process_callback (client, process, 0);

  /* Try to create midi ports */
  inmidi_port = jack_port_register (client, "iportti", 
                                    JACK_DEFAULT_MIDI_TYPE, 
                                    JackPortIsInput, 0);
  outmidi_port = jack_port_register (client, "oportti", 
                                     JACK_DEFAULT_MIDI_TYPE, 
                                     JackPortIsOutput, 0);

  /* Set up a ringbuffer for communication from GUI to process().*/
  if ((global_rb = jack_ringbuffer_create(RINGBUFSZ))==NULL){
    std::cerr << "Could not allocate ringbuffer. " << std::endl;
    exit(2);
  }
  printf("Ringbuffer created with write space %d.\n",
         jack_ringbuffer_write_space (global_rb));
  

  /* Activate client. */
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
#endif
  jack_is_ok = true;

}

int main(int argc, char **argv) {
  int retval;

  init_jack(default_name);

#if 0
  pt = new synti2::Patchtool(patchdes_fname);
  pbank = pt->makePatchBank(16);
  midimap = new synti2::MidiMap();
 
  init_jack_or_die();

  /* Make an edit window for our custom kind of patches. */
  (pt->exposePatchDescr())->headerFileForC(std::cout);
  Fl_Window *window = build_main_window(pt->exposePatchDescr());

  widgets_to_reflect_reality();

  window->show(argc, argv);

  retval = Fl::run();

  jack_ringbuffer_free(global_rb);
  
  if (pbank != NULL) free(pbank);
  if (pt != NULL) free(pt);
#endif

  if (weHaveJack()) {jack_client_close(my_jack.client);}

  return 0;
}
