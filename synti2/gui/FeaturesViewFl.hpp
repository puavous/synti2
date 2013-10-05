#ifndef SYNTI2_GUI_VIEWFEATURES_FL
#define SYNTI2_GUI_VIEWFEATURES_FL
#include <FL/Fl_Group.H>
#include "PatchBank.hpp"
namespace synti2gui {
  class ViewFeatures: public Fl_Group {
  public:
    ViewFeatures(int,int,int,int,const char*,
                 PatchBank*);
  };
}

#endif
