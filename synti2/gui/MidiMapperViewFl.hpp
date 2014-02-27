#ifndef SYNTI2_GUI_VIEWMIDIMAPPER_FL
#define SYNTI2_GUI_VIEWMIDIMAPPER_FL
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Check_Button.H>
#include "MidiMap.hpp"
using namespace synti2base;
namespace synti2gui {

  class MmCbInfo {
  public:
      MidiMap* midimap;
      size_t channel;
      size_t targind;
  };

  class ViewMidiMapper: public Fl_Group {
  private:
    MidiMap *midimap;
    //MidiSender *midiSender;

    /* Widget callback infos. Store for purposes of later destruction */
    std::vector<MmCbInfo*> cbpars;

    /* Mapper pars. */
    std::vector<Fl_Choice*> widg_cmode;
    std::vector<Fl_Input*> widg_cvoices;
    std::vector<Fl_Value_Input*> widg_cvelo;
    std::vector<Fl_Check_Button*> widg_hold;
    std::vector<Fl_Value_Input*> widg_bend;
    std::vector<Fl_Value_Input*> widg_pres;
    std::vector<Fl_Check_Button*> widg_noff;
    std::vector<Fl_Value_Input*> widg_modsrc; //(16*NCONTROLLERS);
    std::vector<Fl_Value_Input*> widg_modmin; //(16*NCONTROLLERS);
    std::vector<Fl_Value_Input*> widg_modmax; //(16*NCONTROLLERS);
    std::vector<Fl_Value_Input*> widg_keysingle; //(16*128);

    Fl_Group *build_channel_mapper(int ipx, int ipy, int ipw, int iph, int ic);
    void build_message_mapper(int x, int y, int w, int h);

    MmCbInfo *newCbInfo(size_t channel, size_t targind){
        MmCbInfo *res = new MmCbInfo();
        res->midimap = midimap;
        res->channel = channel;
        res->targind = targind;
        cbpars.push_back(res);
        return res;
    }
    /** Updates widgets to current values of the current patch. Works on
     *  global data.
     */
    void reloadWidgetValues();

    public:
    static void refreshViewFromData(void *me){
        ((ViewMidiMapper*)me)->reloadWidgetValues();
    }
    //void setMidiSender(MidiSender *ms){
    //    midiSender = ms;
    //}

    ViewMidiMapper(int,int,int,int,const char*,
                   MidiMap*);
  };
}

#endif
