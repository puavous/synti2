/** Patch tool, class and stuff... */
#ifndef SYNTI2_PATCHTOOL_H
#define SYNTI2_PATCHTOOL_H

#include <string>
#include <vector>
#include <map>
#include <cstdlib>

namespace synti2{

  /* Functions - maybe belonging to some class? */

  float decode_f(const unsigned char *buf);
  float encode_f(float val, unsigned char * buf);

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
    std::string precision;
    std::string group;
  public:
    std::string getDescription(){return description;}
    std::string getName(){return name;}
    float getMin(){return std::strtod(min.c_str(), 0);}
    float getMax(){return std::strtod(max.c_str(), 0);}
    int getPrecision(){return std::strtol(precision.c_str(), 0, 0);}
    int getGroup(){return std::strtol(group.c_str(), 0, 0);}
  };


  /** The description of all parameters in one synti2 sound. Patch
   * description handles the descriptions of its parameters.
   */
  class PatchDescr {
  private:
    /* The main structure is a map of description lists: */
    std::map<std::string, std::vector<ParamDescr> > params;
    std::map<std::string, std::map<std::string, int> > parind;
  public:
    /** Constructs a patch description from a text spec. */
    PatchDescr(std::istream &inputs);

    /** Returns the number of parameters of a certain type. */
    int nPars(std::string type);

    /** Prints out a complete C-header file that contains the descriptions.*/
    void headerFileForC(std::ostream &os);

    /** Returns the almost-human-readable descriptive text. */
    std::string getDescription(std::string type, int idx);
    float getMin(std::string type, int idx){
      return params[type][idx].getMin();}
    float getMax(std::string type, int idx){
      return params[type][idx].getMax();}
    int getPrecision(std::string type, int idx){
      return params[type][idx].getPrecision();}
    int getGroup(std::string type, int idx){
      return params[type][idx].getGroup();}


    /** Returns the index of "name" in map["type"]. */
    int getValueID(std::string type, std::string name){
      if (parind[type].count(name) == 0) {
        return -1;
      }

      return parind[type][name];
    }


    const std::map<std::string, std::vector<ParamDescr> >::iterator
    paramBeg(){return params.begin();}
    const std::map<std::string, std::vector<ParamDescr> >::iterator
    paramEnd(){return params.end();}
  private:
    void load_patch_data(std::istream &ifs);
  };

  /** Patch represents an actual synth patch. Patch uses PatchDescr to
   *  know its structure, and allows holding, manipulating, reading,
   *  and writing of actual sound parameters.
   */
  class Patch {
  private:
    /* The main structure mirrors that of PatchDescr. Values are floats: */
    PatchDescr *pd;
    std::map<std::string, std::vector<float> > values;
    void copy_structure_from_descr();
    std::string name;
  public:
    /** Creates a patch with structure taken from pd; values are
     *  initialized as zeros.
     */
    Patch(PatchDescr *ipd);
    void clear();
    void read(std::istream &is);
    void write(std::ostream &os);
    void setName(std::string newname){name = newname;}
    std::string getName(){return name;}
    void setValueByName(std::string type, std::string name, float value){};
    void setValue(std::string type, int idx, float value);
    float getValueByName(std::string type, std::string name){return 1.23;}
    float getValue(std::string type, int idx){return (values[type])[idx];}
    int getNPars(std::string type){return values[type].size();}
    void exportBytes(std::vector<unsigned char> &bvec);
  };

  class PatchBank : public std::vector<Patch> {
  public:
    void write(std::ostream &os){for(unsigned i=0;i<size();i++) at(i).write(os);}
    void read(std::istream &is){
      /*FIXME: End-of-file logics. */
      for(unsigned i=0;i<size();i++) at(i).read(is);
    }
    void exportStandalone(std::ostream &os);
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
    std::string getDescription(std::string type, int idx){
      return patch_description->getDescription(type, idx);
    };

    PatchDescr *exposePatchDescr(){return patch_description;}

    /** Make an empty patch. */
    Patch makePatch(){return Patch(patch_description);}
    /** Make a patch bank with npatches empty patches */
    PatchBank *makePatchBank(int npatches){
      PatchBank *pb = new PatchBank();
      for (int i=0; i<npatches; i++){
        pb->push_back(makePatch());
      }
      return pb;
    }
  };
}

#endif
