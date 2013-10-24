#ifndef SYNTI2_GUI_VIEWPATCHES_FL
#define SYNTI2_GUI_VIEWPATCHES_FL
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Roller.h>
#include <FL/Fl_Slider.h>
#include <FL/Fl_Value_Input.h>
#include "PatchBank.hpp"
#include "Synti2Base.hpp"
using synti2base::PatchBank;


namespace synti2gui {

    class S2Valuator: public Fl_Valuator {
    protected:
        std::string key;
        PatchBank *_pb;
    public:
        S2Valuator(int x, int y, int w, int h,
                   PatchBank *ipb, std::string const & ikey):
                       Fl_Valuator(x,y,w,h,NULL),key(ikey),_pb(ipb){}
        std::string const & getKey() const {return key;}
        PatchBank *pb(){return _pb;}
        virtual void doActivate() = 0;
        virtual void doDeactivate() = 0;
        void activate(){
            cerr << "activating " << key << endl;
            doActivate();}
        void deactivate(){
            cerr << "deactivating " << key << endl;
            doDeactivate();}
    };

    class WidgetEnablerRuleAction: public RuleAction{
    private:
        Fl_Widget *target;
    public:
        WidgetEnablerRuleAction(Fl_Widget *t):target(t){};
        virtual void action (bool ruleOutcome) const {
            S2Valuator *sv = (S2Valuator *)target;
            //std::cerr << "tgt=" << sv->getKey() << "; value: " << ruleOutcome << std::endl;
            if (ruleOutcome){
                sv->activate();
            } else {
                sv->deactivate();
            }
        }
    };


    /** Fixme: Should store a pointer to a description instead?? No...
     * basically not, because fpars are per-patch, not global. Should go
     * through patch bank in any case..
     */
    class S2ValueInput: public S2Valuator{
        Fl_Value_Input *vi;
        static void vicb(Fl_Widget *w, void *p){
            ((S2Valuator*)p)->do_callback();
        }
    public:
        S2ValueInput(int x, int y, int w, int h,
                     PatchBank *ipb, std::string const & ikey)
                     :S2Valuator(x,y,w,h,ipb,ikey)
        {
            vi = new Fl_Value_Input(x,y,w,h);
            vi->callback(vicb,this);
            vi->tooltip(ipb->getI4Par(0,ikey).getHumanReadable().c_str());
            vi->precision(0);
            /* FIXME: Leaks or not? Fl assigns parent automatically? */
        }
        double value(){return vi->value();}
        void precision(int pr){vi->precision(pr);} // FIXME: doPrecision??
        void doActivate(){vi->activate();}
        void doDeactivate(){vi->deactivate();}
    protected:
        void draw(){};
    };

    class S2FValueInput: public S2Valuator{
    private:
        Fl_Roller *vsf;
        static void cbfwd(Fl_Widget *w, void *p){
            ((S2Valuator*)p)->do_callback();
        }
    public:
        S2FValueInput(int x, int y, int w, int h,
                      PatchBank *ipb, std::string const & ikey)
                      :S2Valuator(x,y,w,h,ipb,ikey)
        {
          vsf = new Fl_Roller(x,y,w,h);
          // Gonna ask from active patch.. FIXME: How do I change the limits per-patch?
          vsf->callback(cbfwd,this);
          vsf->tooltip(ipb->getFPar(0,ikey).getHumanReadable().c_str());
          vsf->label(ipb->getFPar(0,ikey).getHumanReadable().c_str());
          vsf->align(FL_ALIGN_RIGHT);
          vsf->type(FL_HOR_NICE_SLIDER);
          //vsf->align(FL_HORIZONTAL);
        }
        double value(){return vsf->value();}
        void precision(int pr){vsf->precision(pr);}
        void doActivate(){vsf->activate();}
        void doDeactivate(){vsf->deactivate();}
    protected:
        void draw(){};
    };


  class ViewPatches: public Fl_Group {
  private:
    PatchBank *pb;
    size_t activePatch;
    std::string lastErrorMessage;

    void build_patch_editor(Fl_Group *gr);

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
