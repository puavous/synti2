#include "featuretool.hpp"

#include <iostream>
#include <string>

std::string tok_upto(const std::string &delims, 
                     std::string &workstring){
  unsigned long spli = workstring.find_first_of(delims);
  std::string res;
  if (spli==workstring.npos){
    res = workstring;
    workstring.clear();
  } else {
    res = workstring.substr(0,spli);
    workstring.erase(0,spli+1);
  }
  return res;
}

void
synti2::Features::fromString(const std::string &s){
  initDefinitions();
  std::string tmp = s;
  std::string mode = tok_upto(":",tmp);
  if (mode == "all"){
    setAll(true);
  } else if (mode == "all-except"){
    setAll(true);
    std::string name;
    while((name = tok_upto(",",tmp)) != ""){
      feature[name] = false;
    }
  } else if (mode == "only"){
    setAll(false);
    std::string name;
    while((name = tok_upto(",",tmp)) != ""){
      feature[name] = true;
    }
  } else {
    std::cerr << "can't do " << s << std::endl;
    std::cerr << "unknown mode " << mode << std::endl;
  }
}


//void
//synti2::Features::writeToStream(std::ostream &ost){
//  ost << "FIXME: Can't do yet" << std::endl;
//}
/* TBD */
