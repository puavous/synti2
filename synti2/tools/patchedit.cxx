/**
 * A small program for creating synti2 patches, sending them out via
 * jack MIDI in real time, and exporting them for compiling a
 * stand-alone synti2 player.
 *
 * This is awful throw-away code, just to be able to send messages for
 * the interface which is still under development. Should re-work the
 * GUIs (and other tool programs) when the core interface is stable..
 * (FIXME: yep.. fix me indeed.)
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
#include <FL/Fl_Counter.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Roller.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_File_Chooser.H>

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
#include <sstream>
#include <fstream>

#include "patchtool.hpp"
#include "midihelper.hpp"
//#include "synti2_inter.h"
#include "synti2_misss.h"
#include "midimaptool.hpp"

#define RINGBUFSZ 0x10000
//#define RINGBUFSZ 0x8000

#define MAX_EVENTS_IN_SLICE 80


/* Application logic that needs to be accessed globally */
Fl_Color colortab[] = {
  Fl_Color(246), Fl_Color(254), Fl_Color(241), Fl_Color(254),
  Fl_Color(247), Fl_Color(255), Fl_Color(242), Fl_Color(253),
  Fl_Color(248), Fl_Color(253), Fl_Color(243), Fl_Color(252),
  Fl_Color(249), Fl_Color(252), Fl_Color(244), Fl_Color(251),
  Fl_Color(249), Fl_Color(252), Fl_Color(244), Fl_Color(251),
};
int curr_patch = 0;
Fl_Button* button_send_all = NULL;
Fl_Button* button_send_current = NULL;
Fl_Input* widget_patch_name = NULL;
std::vector<Fl_Valuator*> widgets_i3;
std::vector<Fl_Valuator*> widgets_f;
std::vector<std::string> flbl;
Fl_Window* main_win;
//std::vector<synti2::Patch> patches;

/* Patch data: */
synti2::PatchBank *pbank = NULL;
synti2::Patchtool *pt = NULL;

/* Midi map data: */
synti2::MidiMap *midimap = NULL;

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


/** FIXME: The midi mapper sysexes are different.. */
/*int synti2_mapper_sysex(s2ed_msg_t *sm, jack_midi_data_t * buf){
}
*/


/** send data to synth, if there is new stuff from the GUI. */
static int
process(jack_nframes_t nframes, void *arg)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  jack_midi_data_t *msg;

  jack_midi_data_t *outdata;

  void *midi_in_buffer = (void*)jack_port_get_buffer (inmidi_port, nframes);
  void *midi_out_buffer = (void*)jack_port_get_buffer (outmidi_port, nframes);

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
    if (jack_ringbuffer_read_space (global_rb) < sizeof(size_t)) break;
    jack_ringbuffer_peek (global_rb, (char*)&sz, sizeof(size_t));
    if (jack_ringbuffer_read_space (global_rb) < sz) break;

    /* Our maximum message size so far... FIXME: will change soon.*/
    if (jack_midi_max_event_size(midi_out_buffer) < 11) break;
    /* FIXME: This should be an error with an explosion anyway.. */

    /* Read size and contents. */
    jack_ringbuffer_read (global_rb, (char*)&sz, sizeof(size_t));
    jack_ringbuffer_read (global_rb, (char*)&sysex_buf, sz);

    /*printf("jack event size %d\n", 
      jack_midi_max_event_size(midi_out_buffer));*/

    if (jack_midi_event_write(midi_out_buffer, 0, sysex_buf, sz) != 0){
      printf("Error writing event %d\n", nsent);
      break;
    }

    /* The current synth engine cannot handle very many events in
       real-time. There is a crashing bug (git branch send_all_hang0
       for trying to sort it out): sending few patches at once works
       but with more patches at once the receiving synth crashes. It
       is currently fixed by this kludge that limits the sending more
       than a hard-coded number of events further from the ringbuffer
       on each process() call. The real reason of the crash is not
       known to me as of yet. Should make some tests to see if the
       data is transmitted completely at all or if the problem lies
       deeper in the processing. Anyway it seems to be a synth problem
       that must be considered a limitation as of now. The evil thing
       is that I don't really know why there needs to be this
       limitation.. */
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
  printf("Ringbuffer created with write space %d.\n",jack_ringbuffer_write_space (global_rb));
  

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

    std::ostringstream vs;
    vs << flbl[i] << " = " << val;
    widgets_f[i]->copy_label(vs.str().c_str());
  }
  main_win->redraw();
}

/** Sends a prepared message over to the synth via the jack output port. */
void send_to_jack_process(const std::vector<unsigned char> &bytes){
  char sysex_buf[2000];
  size_t len = bytes.size();
  for (int i = 0; i<bytes.size(); i++){
    sysex_buf[i] = bytes.at(i);
    printf("0x%02x ", (int) ((unsigned char*)sysex_buf)[i]);
  }
  printf("\n"); fflush(stdout);

  size_t nwrit = jack_ringbuffer_write (global_rb, (char*)(&len), sizeof(size_t));
  nwrit = jack_ringbuffer_write (global_rb, sysex_buf, len);
}


/** Sends one complete patch to the synth over jack MIDI. */
void send_patch_to_jack_port(synti2::PatchBank *pbank, int patnum){
  synti2::Patch &patch = pbank->at(patnum);
  for (int i=0; i<patch.getNPars("I3"); i++){
    send_to_jack_process(pbank->getSysex("I3",patnum,i));
  }
  for (int i=0; i<patch.getNPars("F"); i++){
    send_to_jack_process(pbank->getSysex("F",patnum,i));
  }
}

/* User interface callback functions. */

/** Sends all patches of the patch bank to the synth over jack MIDI. */
void cb_send_all(Fl_Widget* w, void* p){
  for(int ip=0; ip < pbank->size(); ip++){
    std::cerr << "Sending " << ip << std::endl;
    send_patch_to_jack_port(pbank, ip);
    printf("Ringbuffer write space now %d.\n",jack_ringbuffer_write_space (global_rb));
  }
}

/** Sends the current patch to the synth over jack MIDI. */
void cb_send_current(Fl_Widget* w, void* p){
  send_patch_to_jack_port(pbank, curr_patch);
}

/** Changes the current patch, and updates other widgets. */
void cb_change_patch(Fl_Widget* w, void* p){
  double val = ((Fl_Valuator*)w)->value();
  if (val > 15){
    val = 15; ((Fl_Valuator*)w)->value(val);
  }
  if (val < 0){
    val = 0; ((Fl_Valuator*)w)->value(val);
  }

  curr_patch = val;
  widgets_to_reflect_reality();
}

/** Sends and stores an "F" value */
void cb_new_f_value(Fl_Widget* w, void* p){
  double val = ((Fl_Valuator*)w)->value();
  int d = (long)p;

  (*pbank)[curr_patch].setValue("F",d,val);
  /** FIXME: Redo (and re-think) the message system. 
   * Should write the messages from the tool classes..*/
  send_to_jack_process(pbank->getSysex("F",curr_patch,d));
  //send_to_jack_port(MISSS_SYSEX_SET_F, d, curr_patch, val);

  std::ostringstream vs;
  vs << flbl[d] << " = " << val << "         "; // hack..
  ((Fl_Valuator*)w)->copy_label(vs.str().c_str());
}

/** Sends and stores an "I3" value */
void cb_new_i3_value(Fl_Widget* w, void* p){
  double val = ((Fl_Valuator*)w)->value();
  int d = (long)p;

  (*pbank)[curr_patch].setValue("I3",d,val);
  send_to_jack_process(pbank->getSysex("I3",curr_patch,d));
  //send_to_jack_port(MISSS_SYSEX_SET_3BIT, d, curr_patch, val);
}

bool file_exists(const char* fname){
  std::ifstream checkf(fname);
  return checkf.is_open();
}

void cb_save_all(Fl_Widget* w, void* p){
  Fl_File_Chooser chooser(".","*.s2bank",Fl_File_Chooser::CREATE,
                          "Save -- select destination file.");
  chooser.show();
  while(chooser.shown()) Fl::wait();
  if ( chooser.value() == NULL ) return;

  if (file_exists(chooser.value())){
    if (2 != fl_choice("File %s exists. \nDo you want to overwrite it?", 
                       "Cancel", "No", "Yes", chooser.value())) return;
  }

  std::ofstream ofs(chooser.value(), std::ios::trunc);
  pbank->write(ofs);
  midimap->write(ofs);
}

/* FIXME: Used here? Even should be? */
void cb_export_c(Fl_Widget* w, void* p){
  Fl_File_Chooser chooser(".","*.c",Fl_File_Chooser::CREATE,
                          "Export to C -- select destination file.");
  chooser.show();
  while(chooser.shown()) Fl::wait();
  if ( chooser.value() == NULL ) return;

  if (file_exists(chooser.value())){
    if (2 != fl_choice("File %s exists. \nDo you want to overwrite it?", 
                       "Cancel", "No", "Yes", chooser.value())) return;
  }

  std::ofstream ofs(chooser.value(), std::ios::trunc);
  pbank->exportStandalone(ofs);
}


void cb_save_current(Fl_Widget* w, void* p){
  Fl_File_Chooser chooser(".","*.s2patch",Fl_File_Chooser::CREATE,
                          "Save Patch -- select destination file.");
  chooser.show();
  while(chooser.shown()) Fl::wait();
  if ( chooser.value() == NULL ) return;

  if (file_exists(chooser.value())){
    if (2 != fl_choice("File %s exists. \nDo you want to overwrite it?", 
                       "Cancel", "No", "Yes", chooser.value())) return;
  }

  std::ofstream ofs(chooser.value(), std::ios::trunc);
  (*pbank)[curr_patch].write(ofs);
}

void cb_load_all(Fl_Widget* w, void* p){
  Fl_File_Chooser chooser(".","*.s2bank",Fl_File_Chooser::SINGLE,
                          "Load all");
  chooser.show(); while(chooser.shown()) Fl::wait();
  if ( chooser.value() == NULL ) return;

  std::ifstream ifs(chooser.value());
  pbank->read(ifs);
  /* FIXME: midimap->read(ifs); */
  button_send_all->do_callback();
  widgets_to_reflect_reality();
}

void cb_load_current(Fl_Widget* w, void* p){
  Fl_File_Chooser chooser(".","*.s2patch",Fl_File_Chooser::SINGLE,
                          "Load a single patch");
  chooser.show(); while(chooser.shown()) Fl::wait();
  if ( chooser.value() == NULL ) return;

  std::ifstream ifs(chooser.value());
  (*pbank)[curr_patch].read(ifs);
  button_send_all->do_callback();  /* sends all even if load just one. */
  widgets_to_reflect_reality();
}

void cb_clear_current(Fl_Widget* w, void* p){
  (*pbank)[curr_patch].clear();
  button_send_current->do_callback();
  widgets_to_reflect_reality();
}


void cb_exit(Fl_Widget* w, void* p){
  ((Fl_Window*)p)->hide();
}

void cb_patch_name(Fl_Widget* w, void* p){
  pbank->at(curr_patch).setName(((Fl_Input*)w)->value());
}

/* Callbacks for midi mapper. */
void cb_mapper_mode(Fl_Widget* w, void* p){
  int chn = (long)p;
  int mode = ((Fl_Choice*)w)->value();
  midimap->setMode(chn, mode);
  send_to_jack_process(midimap->sysexMode(chn));
}

void cb_mapper_fixvelo(Fl_Widget* w, void* p){
  int chn = (long)p;
  int velo = ((Fl_Choice*)w)->value();
  midimap->setFixedVelo(chn, velo);
  send_to_jack_process(midimap->sysexFixedVelo(chn));
}

void cb_mapper_bend(Fl_Widget* w, void* p){
  int chn = (long)p;
  int bdest = ((Fl_Choice*)w)->value();
  midimap->setBendDest(chn, bdest);
  send_to_jack_process(midimap->sysexBendDest(chn));
}

void cb_mapper_pressure(Fl_Widget* w, void* p){
  int chn = (long)p;
  int presdest = ((Fl_Choice*)w)->value();
  midimap->setBendDest(chn, presdest);
  send_to_jack_process(midimap->sysexPressureDest(chn));
}



void cb_mapper_noff(Fl_Widget* w, void* p){
  int chn = (long)p;
  bool newstate  = (((Fl_Check_Button*)w)->value() == 1);
  midimap->setNoff(chn, newstate);
  send_to_jack_process(midimap->sysexNoff(chn));
}

void cb_mapper_voicelist(Fl_Widget* w, void* p){
  int chn = (long)p;
  std::string value = ((Fl_Input*)w)->value();
  midimap->setVoices(chn, value);
  ((Fl_Input*)w)->value(midimap->getVoicesString(chn).c_str());
  send_to_jack_process(midimap->sysexVoices(chn));
}

void cb_mapper_modsrc(Fl_Widget* w, void* p){
  int chn = ((long)p)>>16;
  int imod = ((long)p) & 0x7f;
  int newsrc = ((Fl_Value_Input*)w)->value();
  midimap->setModSource(chn,imod,newsrc);
  send_to_jack_process(midimap->sysexMod(chn,imod));
}

void cb_mapper_modmin(Fl_Widget* w, void* p){
  int chn = ((long)p)>>16;
  int imod = ((long)p) & 0x7f;
  float val = ((Fl_Value_Input*)w)->value();
  midimap->setModMin(chn,imod,val);
  ((Fl_Value_Input*)w)->value(midimap->getModMin(chn,imod));
  send_to_jack_process(midimap->sysexMod(chn,imod));
}

void cb_mapper_modmax(Fl_Widget* w, void* p){
  int chn = ((long)p)>>16;
  int imod = ((long)p) & 0x7f;
  float val = ((Fl_Value_Input*)w)->value();
  midimap->setModMax(chn,imod,val);
  ((Fl_Value_Input*)w)->value(midimap->getModMax(chn,imod));
  send_to_jack_process(midimap->sysexMod(chn,imod));
}









std::string createFvalLabel(int index, std::string label){
  std::stringstream res;
  res << "(" << index << ")";
  res << label;
  return res.str();
  /* FIXME: Remember C++ -- does this leak? No? It's a temporary copy? Make sure though... */
}

/** Builds the patch editor widgets. */
Fl_Group *build_patch_editor(synti2::PatchDescr *pd){
  Fl_Scroll *scroll = new Fl_Scroll(0,25,1200,740);

  Fl_Counter *patch = new Fl_Counter(50,25,50,25,"Patch");
  patch->type(FL_SIMPLE_COUNTER);
  patch->align(FL_ALIGN_LEFT);
  patch->bounds(0,15);
  patch->precision(0);
  patch->callback(cb_change_patch);

  widget_patch_name = new Fl_Input(150,25,90,25,"Name");
  widget_patch_name->callback(cb_patch_name);

  int px=280, py=25, w=75, h=25, sp=2;
  int labsz = 16;
  Fl_Button *box = new Fl_Button(px+ 0*(w+sp),py,w,h,"S&end this");
  box->callback(cb_send_current); box->labelsize(labsz); 
  button_send_current = box;

  box = new Fl_Button(px + 1*(w+sp),py,w,h,"Send al&l");
  box->callback(cb_send_all); box->labelsize(labsz); 
  button_send_all = box;

  px += w/2;
  box = new Fl_Button(px + 2*(w+sp),py,w,h,"Save this");
  box->callback(cb_save_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 3*(w+sp),py,w,h,"&Save all");
  box->callback(cb_save_all); box->labelsize(labsz); 

  box = new Fl_Button(px + 4*(w+sp),py,w,h,"Ex&port C");
  box->callback(cb_export_c); box->labelsize(labsz); 

  box = new Fl_Button(px + 5*(w+sp),py,w,h,"Clear this");
  box->callback(cb_clear_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 6*(w+sp),py,w,h,"Load this");
  box->callback(cb_load_current); box->labelsize(labsz); 

  box = new Fl_Button(px + 7*(w+sp),py,w,h,"Load all");
  box->callback(cb_load_all); box->labelsize(labsz);

  /*
    Quit will go to a menu.
  px += w/2;
  box = new Fl_Button(px + 8*(w+sp),py,w,h,"&Quit");
  box->callback(cb_exit); box->argument((long)window); box->labelsize(17); 
  */

  /* Parameters Valuator Widgets */
  px=5; py=50; w=25; h=20; sp=2;
  for (int i=0; i < pd->nPars("I3"); i++){
    /* Need to store all ptrs and have attach_to_values() */
    Fl_Value_Input *vi = new Fl_Value_Input(px+i*(w+sp),py,w,h);
    widgets_i3.push_back(vi);
    vi->bounds(pd->getMin("I3",i),pd->getMax("I3",i)); 
    vi->precision(0); vi->argument(i);
    vi->color(colortab[pd->getGroup("I3",i)]);
    vi->tooltip(pd->getDescription("I3", i).c_str());
    vi->argument(i);
    vi->callback(cb_new_i3_value);
  }

  py=80; w=85;
  int npars = pd->nPars("F");
  int ncols = 4;
  int nrows = (npars / ncols) + 1;
  int i = 0;
  for (int col=0; col < ncols; col++){
    for (int row=0; row < nrows; row++){
      if (i==npars) break;
      /* Need to store all ptrs and have attach_to_values() */
      Fl_Roller *vsf = 
        new Fl_Roller(px+col*250,py+row*(h+sp),w,h);
      widgets_f.push_back(vsf);
      /* FIXME: think? */
      vsf->bounds(pd->getMin("F",i),pd->getMax("F",i)); 
      vsf->precision(pd->getPrecision("F",i));
      vsf->color(colortab[pd->getGroup("F",i)]);
      vsf->type(FL_HOR_NICE_SLIDER);
      vsf->label(createFvalLabel(i,pd->getDescription("F",i)).c_str());
      flbl.push_back(createFvalLabel(i,pd->getDescription("F",i)));//pd->getDescription("F",i)); // for use in the other part
      vsf->align(FL_ALIGN_RIGHT);
      vsf->callback(cb_new_f_value);
      vsf->argument(i);
      i++;
    }
  }
  return scroll;
}

Fl_Group *build_channel_mapper(int ipx, int ipy, int ipw, int iph, int ic){
  const char *clab[16] = {"1","2","3","4","5","6","7","8","9","10",
                       "11","12","13","14","15","16"};
  int px=ipx, py=ipy;
  int w=100, h=20, sp=1;

  Fl_Group *chn = new Fl_Group(ipx+1,ipy+1,ipw-2,iph-2);
  chn->box(FL_UP_BOX);

  chn->color(ic%2?0x99ee9900:0x66dd6600);

  Fl_Box *lbl = new Fl_Box(ipx+2,ipy+2,30,30,clab[ic]);
  lbl->align(FL_ALIGN_INSIDE);
  lbl->labelsize(20);

  py+=3;
  Fl_Choice *ch = new Fl_Choice(ipx+32,py,w,h,"");
  ch->add("Mono-Dup",0,0,0,0);
  ch->add("Poly Rotate",0,0,0,0);
  ch->add("Key Map",0,0,0,0);
  ch->add("*Mute*",0,0,0,0);
  ch->value(0);  /*hack default..*/
  ch->argument(ic);
  ch->callback(cb_mapper_mode);

  Fl_Input *vl = new Fl_Input(ipx+32+50+w,py,w,h,"-> to: ");
  ch->argument(ic);
  vl->value(clab[ic]); /*hack default..*/
  vl->argument(ic);
  vl->when(FL_WHEN_ENTER_KEY|FL_WHEN_RELEASE);
  vl->callback(cb_mapper_voicelist);

  w=30;

  /*
  Fl_Value_Input *vi = new Fl_Value_Input(px+400,py,w,h,"Mod1");
  vi = new Fl_Value_Input(px+480,py,w,h,"Mod2");
  vi = new Fl_Value_Input(px+560,py,w,h,"Mod3");
  vi = new Fl_Value_Input(px+640,py,w,h,"Mod4");
  */
  vl = new Fl_Input(px+400,py,w,h,"Fix Velo");
  vl->bounds(0,127); vl->precision(0); vl->argument(ic);
  vl->callback(cb_mapper_fixvelo);

  Fl_Check_Button *pb;
  pb = new Fl_Check_Button(px+500,py,w,h,"Sust (NA)");

  vl = new Fl_Value_Input(px+600,py,w*2,h,"Bend->");
  vl->bounds(0,NCONTROLLERS); vl->precision(0); vl-argument(ic);
  vl->callback(cb_mapper_bend);

  vl = new Fl_Value_Input(px+680,py,w*2,h,"Pres->");
  vl->bounds(0,NCONTROLLERS); vl->precision(0); vl-argument(ic);
  vl->callback(cb_mapper_pressure);


  pb = new Fl_Check_Button(px+740,py,w,h,"Rcv noff");
  pb->argument(ic);
  pb->callback(cb_mapper_noff);


  ipx+=31;

  /* Controllers 1-4 */
  Fl_Value_Input *vi;
  const char *contlab[] = {"C1","C2","C3","C4"};
  px=2,py=24,sp=1,w=15;
  for (int i=0;i<4;i++){
    lbl = new Fl_Box(ipx+px+0*(w+sp),ipy+py,w,h,contlab[i]);
    vi = new Fl_Value_Input(ipx+px+1*(w+sp),ipy+py,w*2,h);
    vi->bounds(0,127);
    vi->precision(0);
    vi->argument((ic<<16)+i);
    vi->callback(cb_mapper_modsrc);

    lbl = new Fl_Box(ipx+px+3*(w+sp),ipy+py,w,h,"->");
    vi = new Fl_Value_Input(ipx+px+4*(w+sp),ipy+py,w*3,h);
    vi->argument((ic<<16)+i);
    vi->callback(cb_mapper_modmin);

    lbl = new Fl_Box(ipx+px+7*(w+sp),ipy+py,w,h,"--");
    vi = new Fl_Value_Input(ipx+px+8*(w+sp),ipy+py,w*3,h);
    vi->argument((ic<<16)+i);
    vi->callback(cb_mapper_modmax);
    px += 12*(w+sp);
  }
  
  /* key map */
  px=2;py=48;sp=1;
  w=30;h=20;
  Fl_Scroll *keys = new Fl_Scroll(ipx+px,ipy+py,800,2*h);
  int oct;
  for(int i=0;i<128;i++){
    Fl_Color notecol[] = {
      0xffffff00, 0xcccccc00, 0xffffff00, 0xcccccc00, 0xffffff00,
      0xffffff00, 0xcccccc00, 0xffffff00, 0xcccccc00, 0xffffff00, 0xcccccc00, 0xffffff00};
    
    int note = i % 12;
    int oct = i / 12;
    int row = i / 128;
    int col = i % 128;

    Fl_Value_Input *vi = new Fl_Value_Input(ipx+px+col*(w+sp),ipy+py+row*(h+sp),w,h);
    vi->color(notecol[note]);
  }
  keys->end();
  chn->end();
}

void build_message_mapper(int x, int y, int w, int h){
  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w-2,h-2);

  for (int i=0;i<16;i++){
    build_channel_mapper(x+2,y+2+i*100,900,96,i);
  }
  scroll->end();
}

/** Builds the main window with widgets reflecting a patch description. */
Fl_Window *build_main_window(synti2::PatchDescr *pd){

  /* Overall Operation Buttons */
  Fl_Window *window = new Fl_Window(1200, 740);
  window->resizable(window);
  main_win = window;

  Fl_Tabs *tabs = new Fl_Tabs(0,0,1200,740);

  Fl_Group *gr = new Fl_Group(0,22,1200,720, "Patches");
  Fl_Group *patchedit = build_patch_editor(pd);
  gr->end();
  gr = new Fl_Group(0,22,1200,720, "Control");
  build_message_mapper(1,23,1198,718);
  gr->end();
  gr = new Fl_Group(0,22,1200,720, "Exe Builder");
  gr->end();

  window->end();
  return window;
}

int main(int argc, char **argv) {
  int retval;

  pt = new synti2::Patchtool("src/patchdesign.dat");
  pbank = pt->makePatchBank(16);
  midimap = new synti2::MidiMap();
 
  init_jack_or_die();

  /* Make an edit window for our custom kind of patches. */
  Fl_Window *window = build_main_window(pt->exposePatchDescr());

  widgets_to_reflect_reality();

  window->show(argc, argv);

  retval = Fl::run();

  jack_ringbuffer_free(global_rb);
  jack_client_close(client);
  
  if (pbank != NULL) free(pbank);
  if (pt != NULL) free(pt);

  return 0;
}
