#ifndef SYNTI2_PATCH2
#define SYNTI2_PATCH2
#include "Synti2Base.hpp"

#include <string>
#include <map>
#include <vector>
#include <iostream>

using namespace synti2base;

  /** Parameter has the same base attributes as a feature description */
  typedef Description Par;

  class I4Par : public Par {
  private:
    /** Current value (mutable) */
    int value;
    /** Allowed values [1,16] as a 16-bit pattern; 0 always allowed. */
    int allowed;
    /** Names to be used in GUI; empty->"0","1","2",.."15" */
    std::vector<std::string> names;
    /** GUI grouping hint */
    int guigroup;
  public:
    /*    I4Par(string ik, string icd, string ihr,
                     int idef, int iallow,
                     std::vector<std::string> inames)
      : Par(ik,icd,ihr), value(idef), allowed(iallow), names(inames)
      {}*/
    I4Par();
    I4Par(string line);
    void fromLine(string line);
    void toStream(std::ostream &ost);
  };

  class FPar : public Par {
  private:
    /** Current value (mutable) */
    float value;
    /** Minimum value (mutable) */
    float minval;
    /** Maximum value (mutable) */
    float maxval;
    /** Preferred granularity in nudged changes (mutable) */
    int precision;
    /** GUI grouping hint */
    int guigroup;
    //FParDescription(){};/*must give all data on creation*/
  public:
    /*    FPar(string ik, string icd, string ihr,
                    float idef, float imin, float imax, float istep)
      : Par(ik,icd,ihr), value(idef), minval(imin), maxval(imax),
        step(istep)
        {}*/
    FPar();
    FPar(string line);
    void fromLine(string line);
    void toStream(std::ostream &ost);
  };


/** Patch contains one synti2 patch, and can handle all
 *  input/output/updating of such. Needs a pointer to the Capabilities
 *  and Features handlers in order to know what can be done.
 */
  class Patch{
  private:
    std::vector<string> i4parKeys;
    std::map<string,I4Par> i4pars;
    std::map<string,int> i4parInd;

    std::vector<string> fparKeys;
    std::map<string,FPar> fpars;
    std::map<string,int> fparInd;

    void addI4Par(string);
    void addFPar(string);
    /*
    void initI4Pars();
    void initFPars();
    */
  public:
    Patch();
    void toStream(std::ostream &ost);
    void fromStream(std::istream &ifs);
  };

#endif
