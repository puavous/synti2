#include "ExeBuilderViewFl.hpp"
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>

#include <iostream>

using synti2base::PatchBank;
using synti2gui::ViewExeBuilder;

void build_exe_builder(int x, int y, int w, int h,
                         PatchBank *pbh = NULL)
{
  if (pbh==NULL) {
    std::cerr << "Error: No PatchBank given. Can't build GUI." << std::endl;
    return;
  }

  Fl_Scroll *scroll = new Fl_Scroll(x+1,y+1,w-2,h-2);

  scroll->end();
}


ViewExeBuilder::ViewExeBuilder(int x, int y, int w, int h,
                                 const char * name,
                                 PatchBank *pbh)
  : Fl_Group(x, y, w, h, name)
{
  build_exe_builder(x,y,w,h,pbh);
  this->end();
}
