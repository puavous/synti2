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
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Select_Browser.H>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/control.h>

#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <signal.h>

#include <iostream>

#include "patchtool.hpp"

#define RINGBUFSZ 0x10000

/** Internal format for messages. */
typedef struct {
  int type;     /* see code for meaning 1 = 3 bits, 2 = 7 bits, 3 = float*/
  int location; /* offset into the respective (3/7/float) parameter table */
  float value;  /* value (internal) */
  float actual; /* value (truncated to type and synti2 transmission format.)*/
} s2ed_msg_t;


/* FL things that need to be accessed globally */
Fl_Value_Slider *vs3, *vs7, *vsf;  /* The value sliders. */

/* Application logic that needs to be accessed globally */
int curr_patch = 0;
int curr_addr[3] = {0,0,0};

/* Patch data: */
synti2::PatchBank *pbank = NULL;
synti2::Patchtool *pt = NULL;

/* Jack stuff */
jack_ringbuffer_t* global_rb;

jack_client_t *client;
jack_port_t *inmidi_port;
jack_port_t *outmidi_port;
unsigned long sr;
const char * client_name = "synti2editor";

/* A small buffer for building one message... FIXME: local to the build func?*/
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


/** send data to synth, if there is new stuff from the GUI. */
static int
process (jack_nframes_t nframes, void *arg)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  jack_midi_data_t *msg;

  jack_midi_data_t *outdata;

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
  
  /* Handle incoming. TODO: jepijei. Same message structure could be used. */
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
void cb_send(Fl_Widget* w, void* p){
  s2ed_msg_t msg = {3,0x030a,-3.14159265f,4.5f};
  size_t nwrit = jack_ringbuffer_write (global_rb, (char*)(&msg), sizeof(s2ed_msg_t));
  std::cout << "ja tuota. FIXME: implement this and others" << std::endl;
}


/** Changes the send address. */
void cb_change_address(Fl_Widget* w, void* p){
  long i = (long) p;
  if (i==3){
    curr_addr[0] = ((Fl_Valuator*)w)->value();
    //vs3->value(patch[curr_patch][0][curr_addr[0]]);
    vs3->label(pt->getDescription("I3",((Fl_Valuator*)w)->value()).c_str());
    vs3->align(FL_ALIGN_RIGHT);
  }else if (i==7) {
    curr_addr[1] = ((Fl_Valuator*)w)->value();

    //vs7->value(patch[curr_patch][1][curr_addr[1]]);
    vs7->label(pt->getDescription("I7",((Fl_Valuator*)w)->value()).c_str());
    vs7->align(FL_ALIGN_RIGHT);
  } else if (i==14) {
    curr_addr[2] = ((Fl_Valuator*)w)->value();
    //vsf->value(patch[curr_patch][2][curr_addr[2]]);
    vsf->label(pt->getDescription("F",((Fl_Valuator*)w)->value()).c_str());
    vsf->align(FL_ALIGN_RIGHT);
  }
}

/** Changes a value to be sent to the current address. */
void cb_new_value(Fl_Widget* w, void* p){
  double val;
  int d;
  d = (long)p;
  val = ((Fl_Valuator*)w)->value();

  s2ed_msg_t msg = {0,0,0,0};

  if (d==3){
    //patch[curr_patch][0][curr_addr[0]] = val;
    std::cout << "Send " << val 
              << " to " << curr_addr[0] 
              << " of " << curr_patch << std::endl;
    msg.type = 1;
    msg.location = curr_addr[0] << 8 + curr_patch;
  } else if (d==7) {
    //patch[curr_patch][1][curr_addr[1]] = val;
    std::cout << "Send " << val 
              << " to " << curr_addr[1] 
              << " of " << curr_patch << std::endl;
    msg.type = 2;
    msg.location = curr_addr[1] << 8 + curr_patch;
  } else if (d==14) {
    //patch[curr_patch][2][curr_addr[2]] = val;
    std::cout << "Send " << val 
              << " to " << curr_addr[2] 
              << " of " << curr_patch << std::endl;
    msg.type = 3;
    msg.location = curr_addr[2] << 8 + curr_patch;
  } else {
    std::cerr << "Error.";
    return;
  }
  msg.value = val;
  size_t nwrit = jack_ringbuffer_write (global_rb, (char*)(&msg), sizeof(s2ed_msg_t));
}


/** Changes an "F" value to be sent to the current address. */
void cb_new_f_value(Fl_Widget* w, void* p){
  double val;
  int d;
  d = (long)p;
  val = ((Fl_Valuator*)w)->value();

  s2ed_msg_t msg = {0,0,0,0};

  if (d==3){
    //patch[curr_patch][0][curr_addr[0]] = val;
    std::cout << "Send " << val 
              << " to " << curr_addr[0] 
              << " of " << curr_patch << std::endl;
    msg.type = 1;
    msg.location = curr_addr[0] << 8 + curr_patch;
  } else if (d==7) {
    //patch[curr_patch][1][curr_addr[1]] = val;
    std::cout << "Send " << val 
              << " to " << curr_addr[1] 
              << " of " << curr_patch << std::endl;
    msg.type = 2;
    msg.location = curr_addr[1] << 8 + curr_patch;
  } else if (d==14) {
    //patch[curr_patch][2][curr_addr[2]] = val;
    std::cout << "Send " << val 
              << " to " << curr_addr[2] 
              << " of " << curr_patch << std::endl;
    msg.type = 3;
    msg.location = curr_addr[2] << 8 + curr_patch;
  } else {
    std::cerr << "Error.";
    return;
  }
  msg.value = val;
  size_t nwrit = jack_ringbuffer_write (global_rb, (char*)(&msg), sizeof(s2ed_msg_t));
}



/** Initializes jack stuff. Exits upon failure. */
void init_jack_or_die(){
  jack_status_t status;

  /* Initial Jack setup. Open (=create?) a client. */
  if ((client = jack_client_open (client_name, 
                                  JackNoStartServer, 
                                  &status)) == 0) {
    std::cerr << "jack server not running?" << std::endl; 
    exit(1);
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

}


Fl_Window *build_main_window(){
  /* Buttons */
  Fl_Window *window = new Fl_Window(600, 280);
  window->resizable(window);
  Fl_Scroll *scroll = new Fl_Scroll(0,0,600,280);
  Fl_Button *box = new Fl_Button(20,20,260,25,"Send al&l");
  box->callback(cb_send); box->labelsize(17); 
  box = new Fl_Button(300,20,260,25,"&Save");
  box->callback(cb_send); box->labelsize(17); 

  /* I3 sliders */
  /* F sliders */

  int px=5, py=100, w=20, h=20, sp=2;
  Fl_Value_Input *c3 = new Fl_Value_Input(px,py+0*(h+sp),w*4,h);
  c3->bounds(0,pt->nPars("I3")-1); c3->precision(0); c3->argument(3);
  c3->callback(cb_change_address);

  Fl_Select_Browser *c7 = new Fl_Select_Browser(px,py+1*(h+sp),w*4,h);
  //c7->bounds(0,pt->nPars("I7")-1); c7->precision(0); c7->argument(7);
  c7->add("hmm1");  c7->add("hmm2");
  c7->callback(cb_change_address);

  Fl_Value_Input *cf = new Fl_Value_Input(px,py+2*(h+sp),w*4,h);
  cf->bounds(0,pt->nPars("F")-1); cf->precision(0); cf->argument(14);
  cf->callback(cb_change_address);


  vs3 = new Fl_Value_Slider(px+4*w+sp,py+0*(h+sp),w*16,h,NULL);
  vs3->bounds(0,7); vs3->precision(0); vs3->type(FL_HOR_NICE_SLIDER);
  vs3->argument(3);
  vs3->callback(cb_new_value);

  vs7= new Fl_Value_Slider(px+4*w+sp,py+1*(h+sp),w*16,h,NULL);
  vs7->bounds(0,127); vs7->precision(0); vs7->type(FL_HOR_NICE_SLIDER);
  vs7->argument(7);
  vs7->callback(cb_new_value);

  vsf= new Fl_Value_Slider(px+4*w+sp,py+2*(h+sp),w*16,h,NULL);
  vsf->bounds(-1.27,1.27); vsf->precision(2); vsf->type(FL_HOR_NICE_SLIDER);
  vsf->argument(14);
  vsf->callback(cb_new_value);

  /*Fl_Dial *dial = new Fl_Dial(320,40,100,100,"Kissa123");
  dial->align(FL_ALIGN_CENTER);
  Fl_Dial *dial2 = new Fl_Dial(420,40,100,100,"@->| ja joo");
  dial2->type(FL_LINE_DIAL);*/

  window->end();
  return window;
}

int main(int argc, char **argv) {
  int retval;

  pt = new synti2::Patchtool("patchdesign.dat");
  pbank = pt->makePatchBank(16);

  //for (int i=0;i<pbank->size();i++) (*pbank)[i].write(std::cout);

  init_jack_or_die(); 

  Fl_Window *window = build_main_window();
  window->show(argc, argv);

  retval = Fl::run();

  jack_ringbuffer_free(global_rb);
  jack_client_close(client);
  
  if (pbank != NULL) free(pbank);
  if (pt != NULL) free(pt);

  return 0;
}
