#ifndef SYNTI2_GUI_VIEWPATCHES_FL
#define SYNTI2_GUI_VIEWPATCHES_FL
#include <FL/Fl_Group.H>
namespace synti2gui {
  class ViewPatches: public Fl_Group {
  public:
    ViewPatches(int,int,int,int,const char *);
  };
}

#endif
