#ifndef SYNTI2_GUI_VIEWPATCHES_FL
#define SYNTI2_GUI_VIEWPATCHES_FL
#include <FL/Fl_Group.H>
#include <FL/Fl_Value_Input.h>
#include "PatchBank.hpp"
#include "Synti2Base.hpp"
using synti2base::PatchBank;


namespace synti2gui {

    class WidgetEnablerRuleAction: public RuleAction{
    private:
        Fl_Widget *target;
    public:
        WidgetEnablerRuleAction(Fl_Widget *t):target(t){};
        virtual void action (bool ruleOutcome) const {
            std::cerr << "tgt:" << target->tooltip() << " " << ruleOutcome << std::endl;
            if (ruleOutcome){
                target->activate();
            } else {
                target->deactivate();
            }
        }
    };

    class I4ValueInput: public Fl_Value_Input{
        std::string key;
    public:
        PatchBank *pb;
        I4ValueInput(int x, int y, int w, int h,
                     PatchBank *ipb):
        Fl_Value_Input(x,y,w,h),pb(ipb) {}
        void setKey(std::string ikey){key = ikey;}
        std::string const & getKey() const {return key;}
    };


  class ViewPatches: public Fl_Group {
  private:
    PatchBank *pb;
    size_t activePatch;
    std::string lastErrorMessage;

    void build_patch_editor(Fl_Group *gr, PatchBank *pbh = NULL);

    static std::vector<std::string> keys;
    static void value_callback(Fl_Widget* w, void* p);
  public:
    size_t getActivePatch(){return activePatch;}
    bool setActivePatch(size_t ind){
      if ((ind < 0) || (ind > pb->getNumPatches())) {
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
