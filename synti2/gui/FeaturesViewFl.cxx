#include "FeaturesViewFl.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Input.H>

#include <iostream>

using synti2gui::PatchBankHandler;
using synti2gui::ViewFeatures;


/** FIXME: Make this a member function; pbh as attribute! And then
 * updateWidgetState() and the dependency logic and
 * updateWidgetState() as a listener to some shoutPatchBankChanged()
 * event.
 */
void build_feature_selector(int x, int y, int w, int h, 
                            PatchBankHandler *pbh = NULL)
{
  if (pbh==NULL) {
    std::cerr << "Error: No PatchBankHandler given. Can't build GUI." << std::endl;
    return;
  }

  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w-2,h-2);
  int px = x+1, py=30, ib=0, width=60, height=20, captwidth=100;

  Fl_Value_Input *vi;
  Fl_Box *lbl;
  lbl = new Fl_Box(px,py,captwidth,height,"Channels:");
  vi = new Fl_Value_Input(px+captwidth,py,width,height);py+=height;
  lbl = new Fl_Box(px,py,captwidth,height,"Operators:");
  vi = new Fl_Value_Input(px+captwidth,py,width,height);py+=height;
  lbl = new Fl_Box(px,py,captwidth,height,"Envelopes:");
  vi = new Fl_Value_Input(px+captwidth,py,width,height);py+=height;
  lbl = new Fl_Box(px,py,captwidth,height,"Env. Knees:");
  vi = new Fl_Value_Input(px+captwidth,py,width,height);py+=height;
  lbl = new Fl_Box(px,py,captwidth,height,"Controllers:");
  vi = new Fl_Value_Input(px+captwidth,py,width,height);py+=height;
  lbl = new Fl_Box(px,py,captwidth,height,"Delays:");
  vi = new Fl_Value_Input(px+captwidth,py,width,height);py+=height;
  lbl = new Fl_Box(px,py,captwidth,height,"Features:");py+=height;

  //std::vector<std::string> fkeys = pbh->getFeatureKeys();
  std::vector<FeatureDescription>::const_iterator it;
  
  for (it=pbh->getFeatureBegin(); it!=pbh->getFeatureEnd(); ++it){
    std::cerr << "should add something" << std::endl;
    Fl_Check_Button *ckb = 
      new Fl_Check_Button(px,py+(ib++)*height,200,20,
                          (*it).getHumanReadable().c_str());
    ckb->argument(ib); // string key as argument? No. Some descriptor object that persists throughout the execution?
    //ckb.callback(ckb_);
  }

  scroll->end();
}


ViewFeatures::ViewFeatures(int x, int y, int w, int h, 
                           const char * name, 
                           PatchBankHandler *pbh)
  : Fl_Group(x, y, w, h, name)
{
  build_feature_selector(x,y,w,h,pbh);
  this->end();
}
