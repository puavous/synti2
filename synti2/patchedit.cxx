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
#include <FL/Fl_Input.H>
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
#include <fstream>

#include "patchtool.hpp"

#define RINGBUFSZ 0x10000

/** Internal format for messages. */
typedef struct {
  int type;     /* see code for meaning 1 = 3 bits, 2 = 7 bits, 3 = float*/
  int location; /* offset into the respective (3/7/float) parameter table */
  float value;  /* value (internal) */
  float actual; /* value (truncated to type and synti2 transmission format.)*/
} s2ed_msg_t;


/* Application logic that needs to be accessed globally */
int curr_patch = 0;
Fl_Button* button_send_all = NULL;
Fl_Input* widget_patch_name = NULL;
std::vector<Fl_Valuator*> widgets_i3;
std::vector<Fl_Valuator*> widgets_f;
//std::vector<synti2::Patch> patches;

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

std::string hack_filename = "hack_patches.attempt";

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
    if (intval > 0x07) jack_error("Too large to be 3bit value! %d", intval);
    sm->actual = (*buf = (intval &= 0x07));
    return 1;
  case 2:
    intval = sm->value;
    if (intval > 0x7f) jack_error("Too large to be 7bit value! %d", intval);
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


/** Updates widgets to current values of the current patch. Works on
 *  global data.
 */
void widgets_to_reflect_reality(){
  widget_patch_name->value((*pbank)[curr_patch].getName().c_str());
  for (int i=0; i<widgets_i3.size(); i++){
    double val = (*pbank)[curr_patch].getValue("I3", i);
    widgets_i3[i]->value(val);
  }
  for (int i=0; i<widgets_f.size(); i++){
    double val = (*pbank)[curr_patch].getValue("F", i);
    widgets_f[i]->value(val);
  }
}

/** Sends a value over to the synth via the jack output port. */
void send_to_jack_port(int type, int idx, int patch, float val){
  /*
  std::cout << "Send " << val 
            << " to " << idx
            << " of " << patch << std::endl;*/
  s2ed_msg_t msg = {0,0,0,0};
  msg.type = type;
  msg.location = idx << 8 + patch;
  msg.value = val;
  size_t nwrit = jack_ringbuffer_write (global_rb, (char*)(&msg), sizeof(s2ed_msg_t));
}

/** Sends one complete patch to the synth over jack MIDI. */
void send_patch_to_jack_port(synti2::Patch &patch, int patnum){
  for (int i=0; i<patch.getNPars("I3"); i++){
    send_to_jack_port(1, i, patnum, patch.getValue("I3",i));
  }
  for (int i=0; i<patch.getNPars("F"); i++){
    send_to_jack_port(3, i, patnum, patch.getValue("F",i));
  }
}

/* User interface callback functions. */

/** Sends all patches of the patch bank to the synth over jack MIDI. */
void cb_send_all(Fl_Widget* w, void* p){
  for(int ip=0; ip<pbank->size(); ip++){
    send_patch_to_jack_port((*pbank)[ip], ip);
  }
}

/** Sends the current patch to the synth over jack MIDI. */
void cb_send_current(Fl_Widget* w, void* p){
  send_patch_to_jack_port((*pbank)[curr_patch], curr_patch);
}

/** Changes the current patch, and updates other widgets. */
void cb_change_patch(Fl_Widget* w, void* p){
  double val = ((Fl_Valuator*)w)->value();
  curr_patch = val;
  widgets_to_reflect_reality();
}

/** Sends and stores an "F" value */
void cb_new_f_value(Fl_Widget* w, void* p){
  double val = ((Fl_Valuator*)w)->value();
  int d = (long)p;

  (*pbank)[curr_patch].setValue("F",d,val);
  send_to_jack_port(3, d, curr_patch, val);
}

/** Sends and stores an "I3" value */
void cb_new_i3_value(Fl_Widget* w, void* p){
  double val = ((Fl_Valuator*)w)->value();
  int d = (long)p;

  (*pbank)[curr_patch].setValue("I3",d,val);
  send_to_jack_port(1, d, curr_patch, val);
}

void cb_save_all(Fl_Widget* w, void* p){
  std::ofstream ofs(hack_filename.c_str(), std::ios::trunc);
  pbank->write(ofs);
}

void cb_save_current(Fl_Widget* w, void* p){
  std::ofstream ofs(hack_filename.c_str(), std::ios::trunc);
  (*pbank)[curr_patch].write(ofs);
}

void cb_load_all(Fl_Widget* w, void* p){
  std::ifstream ifs(hack_filename.c_str());
  pbank->read(ifs);
  button_send_all->do_callback();
  widgets_to_reflect_reality();
}

void cb_load_current(Fl_Widget* w, void* p){
  std::ifstream ifs(hack_filename.c_str());
  (*pbank)[curr_patch].read(ifs);
  button_send_all->do_callback();
  widgets_to_reflect_reality();
}

void cb_exit(Fl_Widget* w, void* p){
  ((Fl_Window*)p)->hide();
}

void cb_patch_name(Fl_Widget* w, void* p){
  pbank->at(curr_patch).setName(((Fl_Input*)w)->value());
}


/** Builds the main window with widgets reflecting a patch description. */
Fl_Window *build_main_window(synti2::PatchDescr *pd){
  /* Overall Operation Buttons */
  Fl_Window *window = new Fl_Window(800, 600);
  window->resizable(window);
  Fl_Scroll *scroll = new Fl_Scroll(0,0,800,600);

  Fl_Value_Input *patch = new Fl_Value_Input(50,20,40,25,"Patch");
  patch->callback(cb_change_patch);

  widget_patch_name = new Fl_Input(150,20,90,25,"Name");
  widget_patch_name->callback(cb_patch_name);

  int px=280, py=20, w=120, h=25, sp=2;
  Fl_Button *box = new Fl_Button(px+ 0*(w+sp),py,w,h,"S&end current");
  box->callback(cb_send_current); box->labelsize(17); 

  box = new Fl_Button(px + 1*(w+sp),py,w,h,"Send al&l");
  box->callback(cb_send_all); box->labelsize(17); 
  button_send_all = box;

  px += w/2;
  box = new Fl_Button(px + 2*(w+sp),py,w,h,"Save current");
  box->callback(cb_save_current); box->labelsize(17); 

  box = new Fl_Button(px + 3*(w+sp),py,w,h,"&Save all");
  box->callback(cb_save_all); box->labelsize(17); 

  box = new Fl_Button(px + 4*(w+sp),py,w,h,"Load current");
  box->callback(cb_load_current); box->labelsize(17); 

  box = new Fl_Button(px + 5*(w+sp),py,w,h,"Load all");
  box->callback(cb_load_all); box->labelsize(17);

  px += w/2;
  box = new Fl_Button(px + 6*(w+sp),py,w,h,"&Quit");
  box->callback(cb_exit); box->argument((long)window); box->labelsize(17); 

  /* Parameters Valuator Widgets */
  px=5; py=50; w=30; h=20; sp=2;
  for (int i=0; i < pd->nPars("I3"); i++){
    /* Need to store all ptrs and have attach_to_values() */
    Fl_Value_Input *vi = new Fl_Value_Input(px+i*(w+sp),py,w,h);
    widgets_i3.push_back(vi);
    vi->bounds(0,7); vi->precision(0); vi->argument(i);
    vi->tooltip(pd->getDescription("I3", i).c_str());
    vi->argument(i);
    vi->callback(cb_new_i3_value);
  }

  py=80; w=30*8;
  int npars = pd->nPars("F");
  int ncols = 3;
  int nrows = (npars / ncols) + 1;
  int i = 0;
  for (int col=0; col < ncols; col++){
    for (int row=0; row < nrows; row++){
      if (i==npars) break;
      /* Need to store all ptrs and have attach_to_values() */
      Fl_Value_Slider *vsf = 
        new Fl_Value_Slider(px+col*400,py+row*(h+sp),w,h);
      widgets_f.push_back(vsf);
      /* FIXME: think? */
      vsf->bounds(pd->getMin("F",i),pd->getMax("F",i)); 
      vsf->precision(2);
      vsf->type(FL_HOR_NICE_SLIDER);
      vsf->label(pd->getDescription("F",i).c_str());
      vsf->align(FL_ALIGN_RIGHT);
      vsf->callback(cb_new_f_value);
      vsf->argument(i);
      i++;
    }
  }

  window->end();
  return window;
}

int main(int argc, char **argv) {
  int retval;

  pt = new synti2::Patchtool("patchdesign.dat");
  pbank = pt->makePatchBank(16);

 
  //for (int i=0;i<pbank->size();i++) (*pbank)[i].write(std::cout);

  init_jack_or_die();

  /* Make an edit window for our custom kind of patches. */
  Fl_Window *window = build_main_window(pt->exposePatchDescr());

  widgets_to_reflect_reality();

  window->show(argc, argv);

  retval = Fl::run();

  std::cout << "Cleaning up." << std::endl;

  jack_ringbuffer_free(global_rb);
  jack_client_close(client);
  
  if (pbank != NULL) free(pbank);
  if (pt != NULL) free(pt);

  return 0;
}
