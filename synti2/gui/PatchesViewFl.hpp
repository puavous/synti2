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
        PatchBank *pb_;

        /* A fancy color scheme. (Non-static initializer requires C++11)*/
        Fl_Color colortab[20] = {
            Fl_Color(246), Fl_Color(254), Fl_Color(241), Fl_Color(254),
            Fl_Color(247), Fl_Color(255), Fl_Color(242), Fl_Color(253),
            Fl_Color(248), Fl_Color(253), Fl_Color(243), Fl_Color(252),
            Fl_Color(249), Fl_Color(252), Fl_Color(244), Fl_Color(251),
            Fl_Color(249), Fl_Color(252), Fl_Color(244), Fl_Color(251),
        };

    public:
        S2Valuator(int x, int y, int w, int h,
                   PatchBank *ipb, std::string const & ikey):
                       Fl_Valuator(x,y,w,h,NULL),key(ikey),pb_(ipb){}
        std::string const & getKey() const {return key;}
        PatchBank *pb(){return pb_;}
        virtual void doActivate() = 0;
        virtual void doDeactivate() = 0;
        virtual void doUpdateLooks() = 0;
        virtual double doReturnValue() = 0;
        virtual void doReloadValue(size_t ipat) = 0;
        void activate(){
            cerr << "activating " << key << endl;
            doActivate();}
        void deactivate(){
            cerr << "deactivating " << key << endl;
            doDeactivate();}
        void updateLooks(){
            doUpdateLooks();
        }
        void reloadValue(size_t ipat){ doReloadValue(ipat); }
        double value(){
            return doReturnValue();
        }

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
            vi->bounds(0,ipb->getI4Par(0,ikey).getMaxValue());
            vi->color(colortab[ipb->getI4Par(0,ikey).getGuiGroup()]);
            /* FIXME: Leaks or not? Fl assigns parent automatically? */
        }
        void precision(int pr){vi->precision(pr);} // FIXME: doPrecision??
        void doActivate(){
            vi->activate();
            vi->color(colortab[pb_->getI4Par(0,key).getGuiGroup()]);
        }
        void doDeactivate(){
            vi->deactivate();
            vi->color(FL_GRAY);
        }
        void doReloadValue(size_t ipat){
            vi->value(pb_->getStoredParAsFloat(ipat, key));
        }
        double doReturnValue(){return vi->value();}
    protected:
        void draw(){}
        void doUpdateLooks(){}

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
          vsf->bounds(ipb->getFPar(0,ikey).getMinValue(),
                      ipb->getFPar(0,ikey).getMaxValue());
          vsf->precision(ipb->getFPar(0,ikey).getPrecision());
          vsf->color(colortab[ipb->getFPar(0,ikey).getGuiGroup()]);

        }
        double doReturnValue(){return vsf->value();}
        void doReloadValue(size_t ipat){
            vsf->value(pb_->getStoredParAsFloat(ipat, key));
            updateLooks();
        }
        void precision(int pr){vsf->precision(pr);}
        void doActivate(){
            vsf->activate();
            vsf->color(colortab[pb_->getFPar(0,key).getGuiGroup()]);
        }
        void doDeactivate(){vsf->deactivate();vsf->color(FL_GRAY);}
        void doUpdateLooks(){
            std::ostringstream vs;
            vs << pb()->getFPar(0,key).getHumanReadable() << " = " << value() << "         "; // hack..
            ((Fl_Valuator*)vsf)->copy_label(vs.str().c_str());
        }

    protected:
        void draw(){};
    };


  class ViewPatches: public Fl_Group {
  private:
    PatchBank *pb;
    size_t activePatch;
    std::string lastErrorMessage;
    std::vector<S2Valuator*> ws;
    Fl_Input* wpatname;

    void build_patch_editor(Fl_Group *gr);

    static std::vector<std::string> keys;
    static void value_callback(Fl_Widget* w, void* p);
    static void butt_send_cb(Fl_Widget* w, void* p);
    static void butt_send_all_cb(Fl_Widget* w, void* p);
    static void butt_save_bank_cb(Fl_Widget* w, void* p);
    static void butt_load_bank_cb(Fl_Widget* w, void* p);
    static void butt_save_current_cb(Fl_Widget* w, void* p);
    static void butt_load_current_cb(Fl_Widget* w, void* p);
    static void butt_clear_cb(Fl_Widget* w, void* p);
    static void butt_panic_cb(Fl_Widget* w, void* p);
    static void inp_name_cb(Fl_Widget* w, void* p);
    static void val_ipat_cb(Fl_Widget* w, void* p);
  public:
    static void refreshViewFromData(void *me){
        ((ViewPatches*)me)->reloadWidgetValues();
    }
    void reloadWidgetValues(){
        wpatname->value(pb->getPatchName(getActivePatch()).c_str());
        std::vector<S2Valuator*>::const_iterator it;
        for (it=ws.begin();it!=ws.end();++it){
            (*it)->reloadValue(activePatch);
        }
    };
    size_t getActivePatch(){return activePatch;}
    void sendCurrentPatch(){pb->sendPatch(getActivePatch());}
    bool setActivePatch(size_t ind){
      if ((ind < 0) || (ind > pb->getNumPatches())) {
        lastErrorMessage = "Illegal active patch index request.";
        return false;
      } else {
        activePatch = ind;
        reloadWidgetValues();
        return true;
      }
    }
    ViewPatches(int,int,int,int,const char*,
                PatchBank*);
  };
}

#endif
