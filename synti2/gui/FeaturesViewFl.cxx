#include "FeaturesViewFl.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>

#include <iostream>

using synti2gui::PatchBankHandler;
using synti2gui::ViewFeatures;

void build_feature_selector(int x, int y, int w, int h, 
                            PatchBankHandler *pbh = NULL)
{
  if (pbh==NULL) {
    std::cerr << "Error: No PatchBankHandler given. Can't build GUI." << std::endl;
    return;
  }

  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w-2,h-2);

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
