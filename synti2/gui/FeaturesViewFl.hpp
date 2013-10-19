#ifndef SYNTI2_GUI_VIEWFEATURES_FL
#define SYNTI2_GUI_VIEWFEATURES_FL
#include <FL/Fl_Group.H>
#include "PatchBank.hpp"
namespace synti2gui {
  class ViewFeatures: public Fl_Group {
  private:
    PatchBank *pb; // we assume the PatchBank outlasts this view.
    void build_feature_selector(int x, int y, int w, int h);
  public:
    ViewFeatures(int,int,int,int,const char*,
                 PatchBank*);
  };
}

#endif
