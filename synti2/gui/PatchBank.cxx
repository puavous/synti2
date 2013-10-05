#include "PatchBank.hpp"

#include <iostream>
PatchBank::PatchBank():patches(14)
{
  initFeatureDescriptions();
  initCapacityDescriptions();
  //toStream(std::cout); /*FIXME: for debug only.*/
};

void PatchBank::toStream(std::ostream & ost){
  ost << "# Output by PatchBank::toStream()" << std::endl;
  // FIXME: capacities.toStream(ostream);
  // FIXME: features.toStream(ostream);
  std::vector<Patch>::iterator pit;
  for(pit=patches.begin(); pit!=patches.end(); ++pit){
    (*pit).toStream(ost);
  }
}

void PatchBank::fromStream(std::istream & ist){
  std::cerr << "PatchBank::fromStream() Can't read yet!" << std::endl;
}
