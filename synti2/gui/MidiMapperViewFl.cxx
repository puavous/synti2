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
#if 0
  ch->callback(cb_mapper_mode);
  widg_cmode[ic] = ch;

  Fl_Input *vl = new Fl_Input(ipx+32+50+w,py,w,h,"-> to: ");
  vl->argument(ic);
  vl->value(clab[ic]); /*hack default..*/
  vl->argument(ic);
  vl->when(FL_WHEN_ENTER_KEY|FL_WHEN_RELEASE);
  vl->callback(cb_mapper_voicelist);
  widg_cvoices[ic] = vl;

  w=30;

  Fl_Value_Input *vi;
  vi = new Fl_Value_Input(px+340,py,w,h,"Fix Velo");
  vi->bounds(0,127); vi->precision(0); vi->argument(ic);
  vi->callback(cb_mapper_fixvelo);
  widg_cvelo[ic] = vi;

  Fl_Check_Button *pb;
  pb = new Fl_Check_Button(px+400,py,w,h,"Hold");
  widg_hold[ic] = pb;

  vi = new Fl_Value_Input(px+580,py,w*2,h,"Bend->");
  vi->bounds(0,NCONTROLLERS); vi->precision(0); vi->argument(ic);
  vi->callback(cb_mapper_bend);
  widg_bend[ic] = vi;


  vi = new Fl_Value_Input(px+720,py,w*2,h,"Pres->");
  vi->bounds(0,NCONTROLLERS); vi->precision(0); vi->argument(ic);
  vi->callback(cb_mapper_pressure);
  widg_pres[ic] = vi;

  pb = new Fl_Check_Button(px+780,py,w,h,"Rcv noff");
  pb->argument(ic);
  pb->callback(cb_mapper_noff);
  widg_noff[ic] = pb;


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
    vi->callback(cb_mapper_modsrc);
    widg_modsrc[ic*NCONTROLLERS + i] = vi;

    lbl = new Fl_Box(ipx+px+3*(w+sp),ipy+py,w,h,"->");
    vi = new Fl_Value_Input(ipx+px+4*(w+sp),ipy+py,w*3,h);
    vi->argument((ic<<16)+i);
    vi->callback(cb_mapper_modmin);
    widg_modmin[ic*NCONTROLLERS + i] = vi;

    lbl = new Fl_Box(ipx+px+7*(w+sp),ipy+py,w,h,"--");
    vi = new Fl_Value_Input(ipx+px+8*(w+sp),ipy+py,w*3,h);
    vi->argument((ic<<16)+i);
    vi->callback(cb_mapper_modmax);
    widg_modmax[ic*NCONTROLLERS + i] = vi;

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
    vi->range(0,NPARTS); vi->precision(0);
    vi->callback(cb_mapper_keysingle);
    widg_keysingle[ic*128+i] = vi;
  }
  keys->end();

#endif
  chn->end();
  return chn;
}


void build_message_mapper(int x, int y, int w, int h, 
                          PatchBank *pbh = NULL)
{
  if (pbh==NULL) {
    std::cerr << "Error: No PatchBank given. Can't build GUI." << std::endl;
    return;
  }

  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w-2,h-2);

  for (int i=0;i<16;i++){
    build_channel_mapper(x+2,y+2+i*100,900,96,i);
  }
  scroll->end();
}


ViewMidiMapper::ViewMidiMapper(int x, int y, int w, int h, 
                               const char * name, 
                               PatchBank *pbh)
  : Fl_Group(x, y, w, h, name)
{
  build_message_mapper(x,y,w,h,pbh);
  this->end();
}
