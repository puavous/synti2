/** Patch tool, class and stuff... */
#ifndef SYNTI2_PATCHTOOL_H
#define SYNTI2_PATCHTOOL_H

#include <string>
#include <vector>
#include <map>

namespace synti2{

  /* Classes for: Patch, PatchBank, ParamDescr, ParamValue */

  /** Description for a single parameter; used for GUI. Parameter
      knows about itself, but not about its context (for example, its
      index number must be handled by others). */
  class ParamDescr {
  public:

    /** Construct from a string of following kind:
     *  Mnemonic Description MinVal MaxVal
     * 
     * Each value is a string which must not contain spaces.
     * Type must be one of I3, I7, F as of this version... 
     *
     * Mnemonic: Must be a valid C constant name
     *
     * Description: Almost-human-readable descriptive text.
     *
     * Minimum value. TODO: Specify how to deal with these...
     * 
     *
     * Maximum value. 
     */
    ParamDescr(std::string line, std::string type);
  private:
    std::string type;
    std::string name;
    std::string description;
    std::string min;
    std::string max;
  public:
    std::string getDescription(){return description;}
    std::string getName(){return name;}
  };


  /** The description of all parameters in one synti2 sound. Patch
   * description handles the descriptions of its parameters.
   */
  class PatchDescr {
  public:
    PatchDescr(std::istream &inputs);
    /** Returns the number of parameters of a certain type. */
    int nPars(std::string type);
    /** Prints out a complete C-header file that contains the descriptions */
    void headerFileForC(std::ostream &os);
  private:
    /* The main structure is a map of description lists: */
    std::map<std::string, std::vector<ParamDescr> > params;
    void load_patch_data(std::istream &ifs);
  };

  /** Patch is an actual synth patch. Patch uses PatchDescr to know
   *  its structure, and allows holding, manipulating, reading, and
   *  writing of actual sound parameters.
   */
  class Patch {
    /* FIXME: Implement */
  };

  /** Individual value of a parameter. Hmm.. necessary? Value can be
      stored as a float in patch, and ParamDescr can do all the
      conversions necessary!! Maybe the value is not even worth a
      distinct class? */
  class ParamValue {
  public:
    /** Pointer to the description of this value */
    ParamDescr *desc;
    /** Current value */
    float value;
  };


  /** Class for helping with patch system management. */
  class Patchtool {
  private:
    PatchDescr *patch_description;
    Patchtool(){}; /*Hiding the default constructor.*/

  public:
    Patchtool(std::string fname);
    ~Patchtool(){delete patch_description;}

    /* delegated... TODO: Need these at all? */
    int nPars(std::string type){return patch_description->nPars(type);}

  };
}

#endif
