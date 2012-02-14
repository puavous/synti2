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
#include <jack/control.h>

#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <signal.h>

#define RINGBUFSZ 0x10000

/** Internal format for messages. */
typedef struct {
  int type;     /* see code for meaning 1 = 3 bits, 2 = 7 bits, 3 = float*/
  int location; /* offset into the respective (3/7/float) parameter table */
  float value;  /* value (internal) */
  float actual; /* value (truncated to type and synti2 transmission format.)*/
} s2ed_msg_t;

jack_ringbuffer_t* global_rb;

jack_client_t *client;
jack_port_t *inmidi_port;
jack_port_t *outmidi_port;
unsigned long sr;
char * client_name = "synti2editor";

/* A small buffer for building one message... */
unsigned char sysex_build_buf[] = {0xF0, 0x00, 0x00, 0x00,
                                   0x00, 0x00,
                                   0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00,
                                   0xf7,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static void signal_handler(int sig)
{
  jack_client_close(client);
  std::cerr << "stop" << std::endl;
  exit(0);
}

/** Decode a "floating point" parameter. */
float synti2_decode_f(const jack_midi_data_t *buf){
  int i;
  float res;
  //jack_info("Decoding %x %x", buf[0], buf[1]);
  res = ((buf[0] & 0x03) << 7) + buf[1];   /* 2 + 7 bits accuracy*/
  res = ((buf[0] & 0x40)>0) ? -res : res;  /* sign */
  res *= .001f;                            /* default e-3 */
  for (i=0; i < ((buf[0] & 0x0c)>>2); i++) res *= 10.f;  /* can be more */
  return res;
}

/* Encode a "floating point" parameter into 7 bit parts. */
float synti2_encode_f(float val, jack_midi_data_t * buf){
  int high = 0;
  int low = 0;
  int intval = 0;
  int timestimes10 = 0;
  if (val < 0){ high |= 0x40; val = -val; } /* handle sign bit */
  /* maximum precision strategy: */
  /* TODO: check decimals first, and try less precise if possible */
  if (val <= 0.511) {
    timestimes10 = 0; intval = val * 1000;
  } else if (val <= 5.11) {
    timestimes10 = 1; intval = val * 100;
  } else if (val <= 51.1) {
    timestimes10 = 2; intval = val * 10;
  } else if (val <= 511) {
    timestimes10 = 3; intval = val * 1;
  } else if (val <= 5110.f) {
    timestimes10 = 4; intval = val * .1;
  } else if (val <= 51100.f) {
    timestimes10 = 5; intval = val * .01;
  } else if (val <= 511000.f) {
    timestimes10 = 6; intval = val * .001;
  } else if (val <= 5110000.f){
    timestimes10 = 7; intval = val * .0001;
  } else {
    jack_error("Too large f value %f", val);
  }
  high |= (timestimes10 << 2); /* The powers of 10*/
  high |= (intval >> 7);
  low = intval & 0x7f;
  buf[0] = high;
  buf[1] = low;
  // jack_info("%02x %02x", high, low);
  return synti2_decode_f(buf);
}


int synti2_encode(s2ed_msg_t *sm, jack_midi_data_t * buf){
  int intval;
  switch (sm->type){
  case 0:
    jack_error("Cannot do opcode 0 from here.");
    return 0;
  case 1:
    intval = sm->value;
    if (intval > 0x07) jack_error("Excess 3bit value! %d", intval);
    sm->actual = (*buf = (intval &= 0x07));
    return 1;
  case 2:
    intval = sm->value;
    if (intval > 0x7f) jack_error("Excess 7bit value! %d", intval);
    sm->actual = (*buf = (intval &= 0x7f));
    return 1;
  case 3:
    sm->actual = synti2_encode_f(sm->value, buf);
    return 2;
  default:
    /* An error.*/
    jack_error("Unknown parameter type.");
  }
  return 0;
}

/** 
 * Builds a synti2 compatible jack MIDI message from our internal
 * representation. The "actual value" field of the structure pointed
 * to by the parameter will be updated. Returns the length of the
 * complete jack MIDI message, headers and footers included.
 */
int build_sysex(s2ed_msg_t *sm, jack_midi_data_t * buf){
  int payload_len;
  buf[0] = 0xF0; buf[1] = 0x00; buf[2] = 0x00; buf[3] = 0x00;
  buf[4] = sm->type >> 7; buf[5] = sm->type & 0x7f;
  buf[6] = (sm->location >> 8) & 0x7f; buf[7] = sm->location & 0x7f;
  payload_len = synti2_encode(sm, &(buf[8]));
  buf[8+payload_len] = 0xF7;
  return 8+1+payload_len;
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

  void *midi_in_buffer = (void*)jack_port_get_buffer (inmidi_port, nframes);
  void *midi_out_buffer = (void*)jack_port_get_buffer (outmidi_port, nframes);

  s2ed_msg_t s2m;
  size_t sz;

  jack_midi_clear_buffer(midi_out_buffer); 
  nev = jack_midi_get_event_count(midi_in_buffer);
  /* Read from UI thread. FIXME: synchronization issues? */
  while (jack_ringbuffer_read_space (global_rb) >= sizeof(s2ed_msg_t)) {
    jack_ringbuffer_read (global_rb, (char*)&s2m, sizeof(s2ed_msg_t));
    sz = build_sysex(&s2m,sysex_build_buf);
    jack_midi_event_write(midi_out_buffer, 0, sysex_build_buf, sz);
    /*
    jack_info("msg %d %d, %8.4f, %8.4f", 
              s2m.type, s2m.location,
              s2m.value, s2m.actual);
    */
  }
  
  /* Handle incoming. */
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {
      jack_info("k ");
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
  s2ed_msg_t msg = {3,0x030a,-3.14159265f,4.5f};
  std::cout << "ja tuota. FIXME: implement this and others" << std::endl;

  size_t nwrit = jack_ringbuffer_write (global_rb, (char*)(&msg), sizeof(s2ed_msg_t));
}



int main(int argc, char **argv) {
  int retval;
  jack_status_t status;

  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, 
                                  JackNoStartServer, 
                                  &status)) == 0) {
    std::cerr << "jack server not running?" << std::endl; return 1;
  }
  
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

  retval = Fl::run();

  jack_ringbuffer_free(global_rb);
  jack_client_close(client);
  
 error:
  exit (0);

}
