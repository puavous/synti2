#ifndef SYNTI2_GUI_VIEWMIDIMAPPER_FL
#define SYNTI2_GUI_VIEWMIDIMAPPER_FL
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Check_Button.H>
#include "MidiMap.hpp"
using namespace synti2base;
namespace synti2gui {
  class ViewMidiMapper: public Fl_Group {
  private:
    MidiMap *midimap;

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

    /** Updates widgets to current values of the current patch. Works on
     *  global data.
     */
    void reloadWidgetValues()
    {

            for(int ic=0; ic<16; ic++)
            {
                widg_cmode.at(ic)->value(midimap->getMode(ic));
                widg_cvoices.at(ic)->value(midimap->getVoicesString(ic).c_str());
                widg_cvelo.at(ic)->value(midimap->getFixedVelo(ic));
                widg_hold.at(ic)->value(midimap->getSust(ic));
                widg_bend.at(ic)->value(midimap->getBendDest(ic));
                widg_pres.at(ic)->value(midimap->getPressureDest(ic));
                widg_noff.at(ic)->value(midimap->getNoff(ic));
                for (int imod=0; imod<NUM_MAX_MODULATORS; imod++)
                {
                    widg_modsrc.at(ic*NUM_MAX_MODULATORS + imod)->value(midimap->getModSource(ic,imod));
                    widg_modmin.at(ic*NUM_MAX_MODULATORS + imod)->value(midimap->getModMin(ic,imod));
                    widg_modmax.at(ic*NUM_MAX_MODULATORS + imod)->value(midimap->getModMax(ic,imod));
                }
                for (int ikey=0; ikey<128; ikey++)
                {
                    widg_keysingle.at(ic*128 + ikey)->value(midimap->getKeyMap(ic,ikey));
                }
            }
    }

    public:
    ViewMidiMapper(int,int,int,int,const char*,
                   MidiMap*);
  };
}

#endif
