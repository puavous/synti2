#ifndef SYNTI2_GUI_PATCHBANKHANDLER_FL
#define SYNTI2_GUI_PATCHBANKHANDLER_FL

#include "PatchBank.hpp" // Maybe re-implement?
using synti2base::PatchBank;
using synti2base::FeatureDescription;
using synti2base::CapacityDescription;

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
  public:
    PatchBankHandler(PatchBank *bank, ViewUpdater *vu, DataSender *ds):
      patchBank(bank),viewUpdater(vu),dataSender(ds) {};
    size_t getNPatches(){return patchBank->getNumPatches();}
    std::vector<FeatureDescription>::iterator 
    getFeatureBegin(){
      return patchBank->getFeatureBegin();
    }
    std::vector<FeatureDescription>::iterator 
    getFeatureEnd(){
      return patchBank->getFeatureEnd();
    }
    std::vector<CapacityDescription>::iterator 
    getCapacityBegin(){
      return patchBank->getCapacityBegin();
    }
    std::vector<CapacityDescription>::iterator 
    getCapacityEnd(){
      return patchBank->getCapacityEnd();
    }
    int
    getCapacityValue(std::string key){
      return patchBank->getCapacityValue(key);
    }
  };
}

#endif
