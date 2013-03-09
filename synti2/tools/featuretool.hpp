#ifndef FEATURE_HELPER_HPP
#define FEATURE_HELPER_HPP
/** Manment of per-build synth features (part of capacities). Features
 *  can be turned on or off, and only the selected ones will be
 *  compiled in a synth executable.
 */
#include <string>
#include <iostream>
namespace synti2 {
  class Features {
  public:
    Features(std::string s){std::cerr << "can't do " << s << std::endl;}
    bool hasFeature(std::string){
      std::cerr << "FIXME: implement" << std::endl; return true;}
    void readFromStream(std::istream &ist){};
    void writeToStream(std::ostream &ost);
  };
}

#endif
