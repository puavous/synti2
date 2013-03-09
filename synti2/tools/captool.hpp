/** Synth capacities and configuration. */
#ifndef SYNTI2_CAPTOOL_HPP
#define SYNTI2_CAPTOOL_HPP

#include <iostream>

namespace synti2{

  /** This is for reading some kind of a "capacity definition file",
   *   and generating patchdesign.dat and synti2_cap.h based on the
   *   selections made in the definitions.
   */
  class Capacities {
    /* FIXME: To be done. */
  public:
    Capacities(std::istream &ist);
    void writeCapH(std::ostream &ost);
  };
}

#endif
