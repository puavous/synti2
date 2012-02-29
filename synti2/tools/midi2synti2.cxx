/** @file midi2synti2.cxx
 *
 * A converter program that filters and transforms standard midi
 * messages (both on-line, and from SMF0 and SMF1 sequence files) to
 * messages that synti2 can receive (also both on-line, and in
 * stand-alone sequence playback mode). A graphical user interface is
 * provided for the on-line mode, and a non-graphical command-line
 * mode is available for the SMF conversion stage. (FIXME: Not all is
 * implemented as of yet)
 *
 * This is made with some more rigor than earlier proof-of-concept
 * hacks, but there is a hard calendar time limitation before
 * Instanssi 2012, and thus many kludges and awkwardness are likely to
 * persist.
 *
 */

/* Includes needed by fltk. */
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>

/* Includes needed by jack */
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/control.h>
#include <errno.h>


#include <iostream>


/** Class for jack client setup. Just the earlier dirty code wrapped
 *  inside a class. Query isOK() after attempted creation.
 */
class MyJackClient{
private:
  static const int RINGBUFSZ = 30000;
  jack_ringbuffer_t* rb_in;

  jack_client_t *client;
  jack_port_t *inmidi_port;
  jack_port_t *outmidi_port;
  unsigned long sr;
  bool _isOK;

  /** Initializes jack stuff. Exits upon failure. */
  void init_jack_or_die(const char *client_name, JackProcessCallback process){
    jack_status_t status;

    _isOK = false; /* It's not OK until it's OK. */

    /* Initial Jack setup. */
    if ((client = jack_client_open (client_name, 
                                    JackNoStartServer, 
                                    &status)) == NULL) {
      std::cerr << "jack server not running?" << std::endl; 
      return;
    }

    /* Set up process callback. Arg will be a ptr to this client instance. */
    if (jack_set_process_callback (client, process, this) != 0){
      std::cerr << "Could not set the jack process callback" << std::endl;
      return;
    };

    /* Try to create midi ports */
    inmidi_port = jack_port_register (client, "in", 
                                      JACK_DEFAULT_MIDI_TYPE, 
                                      JackPortIsInput, 0);

    outmidi_port = jack_port_register (client, "out", 
                                       JACK_DEFAULT_MIDI_TYPE, 
                                       JackPortIsOutput, 0);

    /* Set up a ringbuffer for communication from GUI to process().*/
    if ((rb_in = jack_ringbuffer_create(RINGBUFSZ))==NULL){
      std::cerr << "Could not allocate ringbuffer. " << std::endl;
      return;
    }

    /* Activate client. */
    if (jack_activate (client)) {
      std::cerr << "cannot activate client" << std::endl;
      return;
    }

    _isOK = true;
}


public:
  MyJackClient(const char *cname, JackProcessCallback cbf){
    init_jack_or_die(cname, cbf);
  }
  ~MyJackClient(){
    std::cerr << "releasing jack" << std::endl;

    if (rb_in != NULL) jack_ringbuffer_free(rb_in);
    if (client != NULL) jack_client_close(client);
  }
  bool isOK(){return this->_isOK;}

  void * getMidiInBuffer(jack_nframes_t nframes){
    return (void*)jack_port_get_buffer (inmidi_port, nframes);
  }
  void * getMidiOutBuffer(jack_nframes_t nframes){
    return (void*)jack_port_get_buffer (outmidi_port, nframes);
  }
};





/* Thanks to http://seriss.com/people/erco/fltk for nice and helpful
 * Fltk examples.
 */

class MainWin : public Fl_Window {
private:

public:  

  static void cb_exit(Fl_Widget *w, void *data){
    ((Fl_Window*)data)->hide();
  }

  MainWin(int X, int Y, 
          int W=100, int H=140,
          const char *L=0) : Fl_Window(X,Y,W,H,L)
  {
    Fl_Button *b = new Fl_Button(10,10,50,20,"&Quit");
    b->callback(cb_exit); b->argument((long)this);
    end();
  }
};


/** send data to synth, if there is new stuff from the GUI. */
static int
process (jack_nframes_t nframes, void *arg)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  jack_midi_data_t *msg;

  jack_midi_data_t *outdata;

  MyJackClient *jc = (MyJackClient*) arg;

  void *midi_in_buffer = jc->getMidiInBuffer(nframes);
  void *midi_out_buffer = jc->getMidiOutBuffer(nframes);

  size_t sz;


  jack_midi_clear_buffer(midi_out_buffer);
  nev = jack_midi_get_event_count(midi_in_buffer);

  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) == ENODATA) break;

    /*debug_print_ev(&ev);*/
    /*channel(&ev);*/
    /*debug_print_ev(&ev);*/
    
    /*if (!rotate_notes(&ev)) continue;*/
    /* TODO: duplicate_controllers() -- 
       requires creation of additional events!!!
       maybe put the event writing inside of the functions 
       (and refactor names)
     */

    jack_midi_event_write(midi_out_buffer, 
                          ev.time, ev.buffer, ev.size);
  }
  return 0;
}


int main(int argc, char **argv){
  MyJackClient mjc("midi2synti2", process);
  if (!mjc.isOK()) exit(1);

  MainWin *mw = new MainWin(10,10);
  mw->show(argc, argv);
  int retval = Fl::run();

  return retval;
}
