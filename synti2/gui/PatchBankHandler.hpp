#ifndef SYNTI2_GUI_PATCHBANKHANDLER_FL
#define SYNTI2_GUI_PATCHBANKHANDLER_FL

#include "patchtool.hpp" // Maybe re-implement?
using synti2::PatchBank;

namespace synti2gui {
  // FIXME: Move to its own hpp and cxx?
  /** Sends data over to an external sink, for example Jack MIDI. */
  class DataSender {
  };

  // FIXME: Move to its own hpp and cxx?
  /** Updates widgets of a GUI platform, for example fltk. */
  class ViewUpdater {
  };

  /** Handles the logic of synti2 patches and the patch bank. Sends
   *  MIDI data to outputs (delegated to DataSender) when needed, and
   *  requests view updates according to the current state (delegated
   *  to ViewUpdater.)
   */
  class PatchBankHandler{
  private:
    PatchBank *patchBank;      /* The bank that is commanded. */
    ViewUpdater *viewUpdater;  /* Abstracts the GUI library. */
    DataSender *dataSender;    /* Abstracts the MIDI library. */
    size_t activePatch;
    std::string lastErrorMessage;
  private:
    /** */
    bool checkParamValue(std::string key, float v){
      lastErrorMessage = "None shall pass (sanity check unimplemented)";
      return false; /*FIXME: Implement.*/
    }
  public:
    PatchBankHandler(PatchBank *bank, ViewUpdater *vu, DataSender *ds):
      patchBank(bank),viewUpdater(vu),dataSender(ds) {};
    size_t getNPatches(){return patchBank->size();}
    size_t getActivePatch(){return activePatch;}
    bool setActivePatch(int ind){
      if ((ind < 0) || (ind > getNPatches())) {
        lastErrorMessage = "Illegal active patch index request.";
        return false;
      } else {
        activePatch = ind;
        //viewUpdater->rebuildPatchWidgets(); // FIXME: think!
        return true;
      }
    }
    bool setParamValue(std::string key, float v){
      bool can_do = checkParamValue(key, v);
      if (can_do){
        //privSetParam(key,v);
        //privSendParam(key,v);
        //viewUpdater->updateAllConnectedViews(key,v);
      }
      return can_do;
    };
    std::string getLastErrorMessage(){return lastErrorMessage;}
  };
}

#endif
