#include "MidiMapperViewFl.hpp"
#include "Action.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Choice.H>

#include <iostream>

using synti2base::PatchBank;
using synti2gui::ViewMidiMapper;

#if 0
FIXME: something like
struct cb_info_t {MidiMap* midimap, void* longpar};
vector<cb_info_t> cbpars;
... widg->callback(fun, &(cbpars[i]));
... or something..


/* Callbacks for midi mapper. */
void cb_mapper_mode(Fl_Widget* w, void* p){
  int chn = (long)p;
  int mode = ((Fl_Choice*)w)->value();
  midimap->setMode(chn, mode);
  send_to_jack_process(midimap->sysexMode(chn));
}

void cb_mapper_fixvelo(Fl_Widget* w, void* p){
  int chn = (long)p;
  int velo = ((Fl_Value_Input*)w)->value();
  midimap->setFixedVelo(chn, velo);
  send_to_jack_process(midimap->sysexFixedVelo(chn));
}

void cb_mapper_bend(Fl_Widget* w, void* p){
  int chn = (long)p;
  int bdest = ((Fl_Value_Input*)w)->value();
  midimap->setBendDest(chn, bdest);
  send_to_jack_process(midimap->sysexBendDest(chn));
}

void cb_mapper_pressure(Fl_Widget* w, void* p){
  int chn = (long)p;
  int presdest = ((Fl_Value_Input*)w)->value();
  midimap->setPressureDest(chn, presdest);
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

void cb_mapper_keysingle(Fl_Widget* w, void* p){
  int chn = ((long)p)>>16;
  int inote = ((long)p) & 0x7f;
  int newvoi = ((Fl_Value_Input*)w)->value();
  midimap->setKeyMap(chn,inote,newvoi);
  send_to_jack_process(midimap->sysexKeyMapSingleNote(chn,inote));
}
#endif // 0


Fl_Group *
ViewMidiMapper::build_channel_mapper(int ipx, int ipy, int ipw, int iph, int ic)
{
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
  //ch->callback(cb_mapper_mode);
  widg_cmode[ic] = ch;

  Fl_Input *vl = new Fl_Input(ipx+32+50+w,py,w,h,"-> to: ");
  vl->argument(ic);
  vl->value(clab[ic]); /*hack default..*/
  vl->argument(ic);
  vl->when(FL_WHEN_ENTER_KEY|FL_WHEN_RELEASE);
  //vl->callback(cb_mapper_voicelist);
  widg_cvoices[ic] = vl;

  w=30;

  Fl_Value_Input *vi;
  vi = new Fl_Value_Input(px+340,py,w,h,"Fix Velo");
  vi->bounds(0,127); vi->precision(0); vi->argument(ic);
  //vi->callback(cb_mapper_fixvelo);
  widg_cvelo[ic] = vi;

//  Fl_Check_Button *pb;
//  pb = new Fl_Check_Button(px+400,py,w,h,"Hold");
//  widg_hold[ic] = pb;

  vi = new Fl_Value_Input(px+580,py,w*2,h,"Bend->");
  vi->bounds(0,NUM_MAX_MODULATORS); vi->precision(0); vi->argument(ic);
  //vi->callback(cb_mapper_bend);
  widg_bend[ic] = vi;


  vi = new Fl_Value_Input(px+720,py,w*2,h,"Pres->");
  vi->bounds(0,NUM_MAX_MODULATORS); vi->precision(0); vi->argument(ic);
  //vi->callback(cb_mapper_pressure);
  widg_pres[ic] = vi;

  Fl_Check_Button *cbutt;
  cbutt = new Fl_Check_Button(px+780,py,w,h,"Rcv noff");
  cbutt->argument(ic);
  //cbutt->callback(cb_mapper_noff);
  widg_noff[ic] = cbutt;


  ipx+=31;

  /* Controllers 1-4 */
  const char *contlab[] = {"C1","C2","C3","C4"};
  px=2,py=24,sp=1,w=15;
  for (int i=0;i<4;i++){
    lbl = new Fl_Box(ipx+px+0*(w+sp),ipy+py,w,h,contlab[i]);
    vi = new Fl_Value_Input(ipx+px+1*(w+sp),ipy+py,w*2,h);
    vi->bounds(0,127);
    vi->precision(0);
    vi->argument((ic<<16)+i);
    //vi->callback(cb_mapper_modsrc);
    widg_modsrc[ic*NUM_MAX_MODULATORS + i] = vi;

    lbl = new Fl_Box(ipx+px+3*(w+sp),ipy+py,w,h,"->");
    vi = new Fl_Value_Input(ipx+px+4*(w+sp),ipy+py,w*3,h);
    vi->argument((ic<<16)+i);
    //vi->callback(cb_mapper_modmin);
    widg_modmin[ic*NUM_MAX_MODULATORS + i] = vi;

    lbl = new Fl_Box(ipx+px+7*(w+sp),ipy+py,w,h,"--");
    vi = new Fl_Value_Input(ipx+px+8*(w+sp),ipy+py,w*3,h);
    vi->argument((ic<<16)+i);
    //vi->callback(cb_mapper_modmax);
    widg_modmax[ic*NUM_MAX_MODULATORS + i] = vi;

    px += 12*(w+sp);
  }

  /* key map */
  px=2;py=48;sp=1;
  w=30;h=20;
  Fl_Scroll *keys = new Fl_Scroll(ipx+px,ipy+py,800,2*h);

  int oct;
  for(int i=0;i<128;i++){
    unsigned int notecol[] = {
      0xffffff00, 0xcccccc00, 0xffffff00, 0xcccccc00, 0xffffff00,
      0xffffff00, 0xcccccc00, 0xffffff00, 0xcccccc00, 0xffffff00, 0xcccccc00, 0xffffff00};

    int note = i % 12;
    int oct = i / 12;
    int row = i / 128;
    int col = i % 128;

    Fl_Value_Input *vi = new Fl_Value_Input(ipx+px+col*(w+sp),ipy+py+row*(h+sp),w,h);
    if (i==0x24) vi->color(0xffcccc00);
    else vi->color(notecol[note]);
    vi->argument((ic<<16)+i);
    vi->range(0,NUM_MAX_CHANNELS); vi->precision(0);
    //vi->callback(cb_mapper_keysingle);
    widg_keysingle[ic*128+i] = vi;
  }

  keys->end();

  chn->end();
  return chn;
}


void
ViewMidiMapper::build_message_mapper(int x, int y, int w, int h)
{
  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w*.8-2,h-2);
  for (int i=0;i<16;i++){
    build_channel_mapper(x+2,y+2+i*100,w,96,i);
  }
  scroll->end();
}


ViewMidiMapper::ViewMidiMapper(int x, int y, int w, int h,
                               const char * name,
                               MidiMap *mm)
  : Fl_Group(x, y, w, h, name), midimap(mm)
{
  widg_cmode.resize(16);
  widg_cvoices.resize(16);
  widg_cvelo.resize(16);
  widg_hold.resize(16);
  widg_bend.resize(16);
  widg_pres.resize(16);
  widg_noff.resize(16);
  widg_modsrc.resize(16*NUM_MAX_MODULATORS);
  widg_modmin.resize(16*NUM_MAX_MODULATORS);
  widg_modmax.resize(16*NUM_MAX_MODULATORS);
  widg_keysingle.resize(16*128);

  build_message_mapper(x,y,w,h);
  this->end();
}
