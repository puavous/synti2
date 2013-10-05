#ifndef SYNTI2_GUI_VIEWMIDIMAPPER_FL
#define SYNTI2_GUI_VIEWMIDIMAPPER_FL
#include <FL/Fl_Group.H>
#include "PatchBank.hpp"
namespace synti2gui {
  class ViewMidiMapper: public Fl_Group {
  public:
    ViewMidiMapper(int,int,int,int,const char*,
                   PatchBank*);
  };
}

#endif
