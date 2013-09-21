#ifndef SYNTI2_GUI_ACTION
#define SYNTI2_GUI_ACTION
#include <string>
#include <iostream>
#include "PatchBankHandler.hpp"

//#include "PatchBankHandler.hpp"
namespace synti2gui {
  /** Actions communicate between (any) UI and the patch bank model,
   *  and transfer data to/from (any) external connections
   */
  class Action {
  public:
  };
  
  
  /**
   * Action when a patch parameter has been changed..
   */
  class ValuatorAction : public Action {
  private:
    PatchBankHandler *handler;
    std::string key;
  public:
    ValuatorAction(PatchBankHandler *ihandler, 
                   const std::string& ikey) : handler(ihandler), 
                                              key(ikey){};
    void updateValue(float v){
      if (!handler->setParamValue(key,v)){
        /* Proper error reporting */
        std::cerr << handler->getLastErrorMessage();
      }
    }
  };
}

#endif
