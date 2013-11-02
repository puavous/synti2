#ifndef SYNTI2_GUI_VIEWMIDIMAPPER_FL
#define SYNTI2_GUI_VIEWMIDIMAPPER_FL
#include <FL/Fl_Group.H>
#include "PatchBank.hpp"
namespace synti2gui {
  class ViewMidiMapper: public Fl_Group {
  private:
    PatchBank *pb;
    Fl_Group *build_channel_mapper(int ipx, int ipy, int ipw, int iph, int ic);
    void build_message_mapper(int x, int y, int w, int h);
  public:
    ViewMidiMapper(int,int,int,int,const char*,
                   PatchBank*);
  };
}

#endif
