/** Patch tool, class and stuff... */
#ifndef SYNTI2_PATCHTOOL_H
#define SYNTI2_PATCHTOOL_H

#include <string>
#include <vector>

namespace synti2{

  /* Classes for: Patch, PatchBank, ParamDescr, ParamValue */

  /** Parameter description; used for GUI  */
  class ParamDescr {
  public:
    ParamDescr(std::string){};
  private:
    /** Type. Must not contain spaces; must be one of I3, I7, F. */
    std::string type;
    /** Almost-human-readable descriptive text. Must not contain spaces. */
    std::string description;
    /** Minimum value. */
    float min;
    /** Maximum value. */
    float max;
  };

  /** Individual value of a parameter. Hmm.. necessary? */
  class ParamValue {
  public:
    /** Pointer to the description of this value */
    ParamDescr *desc;
    /** Current value */
    float value;
  };

  class PatchDescr {
  public:
    PatchDescr(std::istream);
  private:
    std::vector<ParamDescr> params;
  };

  /** Class for helping with patch system management. */
  class Patchtool {
  private:
    std::vector<std::string> sectlist;
    std::vector<int> sectsize;

    Patchtool(){}; /*Hiding the default constructor.*/

    void load_patch_data(const char *fname);
  public:
    Patchtool(std::string fname);
    //  load_patch_data("patchdesign.dat");

    /** Returns the number of parameters of a certain type. */
    int nPars(std::string type);
    int nPars(int num){return sectsize[num];}
  };
}

#endif
