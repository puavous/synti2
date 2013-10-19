#include "FeaturesViewFl.hpp"
#include "PatchBank.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Input.H>

#include <iostream>

using synti2base::PatchBank;
using synti2gui::ViewFeatures;


/** FIXME: updateWidgetState() and the dependency logic and
 * updateWidgetState() as a listener to some shoutPatchBankChanged()
 * event.
 */
void
ViewFeatures::build_feature_selector(int x, int y, int w, int h)
{
  if (pb==NULL) {
    std::cerr << "Error: No PatchBank given. Can't build GUI." << std::endl;
    return;
  }

  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w-2,h-2);
  int px = x+1, py=30, ib=0, width=60, height=20, captwidth=350;

  Fl_Value_Input *vi;
  Fl_Box *lbl;
  std::vector<CapacityDescription>::const_iterator cit;
  for (cit=pb->getCapacityBegin(); cit!= pb->getCapacityEnd(); ++cit){
    lbl = new Fl_Box(px,py,captwidth,height,(*cit).getHumanReadable().c_str());
    lbl->align(FL_ALIGN_INSIDE|FL_ALIGN_LEFT);
    vi = new Fl_Value_Input(px+captwidth,py,width,height);py+=height;
    vi->value(pb->getCapacityValue((*cit).getKey()));
    //vi->callback(vi_);
  }

  std::vector<FeatureDescription>::const_iterator it;

  Fl_Check_Button *ckb;
  for (it=pb->getFeatureBegin(); it!=pb->getFeatureEnd(); ++it){
    ckb = new Fl_Check_Button(px,py+(ib++)*height,200,20,
                              (*it).getHumanReadable().c_str());
    ckb->argument(ib); // string key as argument? No. Some descriptor object that persists throughout the execution?
    //ckb.callback(ckb_);
  }

  scroll->end();
}


ViewFeatures::ViewFeatures(int x, int y, int w, int h,
                           const char * name,
                           PatchBank *ipb)
  : Fl_Group(x, y, w, h, name), pb(ipb)
{
  build_feature_selector(x,y,w,h);
  this->end();
}
