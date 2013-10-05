#ifndef SYNTI2_PATCH2
#define SYNTI2_PATCH2
#include "Synti2Base.hpp"

#include <string>
#include <map>
#include <vector>

using namespace synti2base;

  class Patch{
  private:
    std::vector<string> i4parKeys;
    std::map<string,I4ParDescription> i4pars;
    std::vector<int> i4parValues;

    std::vector<string> fparKeys;
    std::map<string,FParDescription> fpars;
    std::vector<float> fparValues;
    void addI4ParDescription(){
    }
    void addFParDescription(){
    }
    void initI4ParDescriptions(){
      addI4ParDescription();
    }
    void initFParDescriptions(){
      addFParDescription();
    }
  public:
    Patch(){
      initI4ParDescriptions();
      initFParDescriptions();
    };
  };

#endif
