#ifndef SYNTI2_GUI_VIEWEXEBUILDER_FL
#define SYNTI2_GUI_VIEWEXEBUILDER_FL
#include <FL/Fl_Group.H>
#include "PatchBank.hpp"
namespace synti2gui {
  class ViewExeBuilder: public Fl_Group {
  public:
    ViewExeBuilder(int,int,int,int,const char*,
                   PatchBank*);
  };
}

#endif
