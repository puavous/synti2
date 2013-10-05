#ifndef SYNTI2_GUI_VIEWPATCHES_FL
#define SYNTI2_GUI_VIEWPATCHES_FL
#include <FL/Fl_Group.H>
#include "PatchBank.hpp"
using synti2base::PatchBank;

namespace synti2gui {

  class ViewPatches: public Fl_Group {
  private:
    PatchBank *pb;
    size_t activePatch;
    std::string lastErrorMessage;

  public:
    size_t getActivePatch(){return activePatch;}
    bool setActivePatch(int ind){
      if ((ind < 0) || ((size_t)ind > pb->getNumPatches())) {
        lastErrorMessage = "Illegal active patch index request.";
        return false;
      } else {
        activePatch = ind;
        //viewUpdater->rebuildPatchWidgets(); // FIXME: think!
        return true;
      }
    }
    ViewPatches(int,int,int,int,const char*,
                PatchBank*);
  };
}

#endif
