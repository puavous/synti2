#ifndef SYNTI2_PATCHBANK2
#define SYNTI2_PATCHBANK2

#include <map>
#include <string>
#include <vector>
#include <istream>
#include <iostream>

#include "Synti2Base.hpp"
#include "Patch.hpp"

using std::string;
using std::vector;
using std::map;
using std::istream;

namespace synti2base {

  /* The following things need quite a close binding, because there
     are rules connecting capacities, features, and parameters that
     need/not to be set in a patch. */

  /** "Capacities" of a synti2 stand-alone compilation - integral
   * numbers of channels and oscillators..
   */
  class Capacities {
  private:
    std::vector<string> capkeys;
    std::vector<CapacityDescription> caps;
    std::map<string,int> capValue;
    void addCapacityDescription(string key, 
                                string cname,
                                string humanReadable,
                                int min,
                                int max,
                                int initial,
                                string requiresf
                                );
    /** Initializes integer-type capacities of the synth. */
    void initCapacityDescriptions();
  public:
    Capacities(){initCapacityDescriptions();}
    std::vector<CapacityDescription>::iterator
    begin(){return caps.begin();}
    std::vector<CapacityDescription>::iterator
    end(){return caps.end();}
    int value(std::string key){
      return capValue[key];
    }
    void toStream(std::ostream &ost);
  };

  /** "Features" of a synti2 stand-alone compilation - boolean on/off
   * values of synth modules to be compiled and to take part in
   * compilation.
   */
  class Features {
  private:
    std::vector<string> featkeys;
    std::vector<FeatureDescription> feats;
    std::map<string,bool> featureEnabled;
    void addFeatureDescription(string key, 
                               string cname,
                               string humanReadable,
                               string requires);
    /** Initializes the on/off -toggled feature descriptions */
    void initFeatureDescriptions();
  public:
    Features(){initFeatureDescriptions();}
    std::vector<FeatureDescription>::iterator
    begin(){return feats.begin();}
    std::vector<FeatureDescription>::iterator
    end(){return feats.end();}
    void toStream(std::ostream &ost);
  };

  class MidiMap{
  public:
    MidiMap(){}
    void toStream(std::ostream &ost);
  };

  /** Derive this to update GUI when a feature is turned on/off. */
  class FeatCallback {
  public:
    virtual void featureStateWasSet(const std::string &key,bool enabled)
    {
      std::cerr << "Using underived FeatCallback." << std::endl;
    };
  };

  /** Derive this to update GUI when a capacity is changed. */
  class CapacityCallback {
  public:
    virtual void capacityWasSet(const std::string &key,int state)
    {
      std::cerr << "Using underived CapacityCallback." << std::endl;
    };
  };



  /** 
   * PatchBank stores everything related to a synti2 patch bank, i.e.,
   * the enabled features, selected synth capabilities, GUI
   * preferences, MIDI mapping, and the patches themselves.
   *
   * Output (.s2bank, .s2patch, .c, MIDI SysEx bytes) is given
   * separately for compose mode (full index set) and stand-alone mode
   * (index set restricted by feature setup)
   *
   * External communication is via strings, MIDI, and integers
   * only. All the patch logic is encapsulated behind the PatchBank
   * interface. Callback function objects can be registered in order
   * to react to side-effects of setters.
   */
  class PatchBank {
  private:
    Features feats;
    Capacities caps;
    MidiMap midimap;
    std::vector<Patch> patches;
  public:
    PatchBank();

    /** Writes a .s2bank to a stream. */
    void toStream(std::ostream & ost);
    /** Reads a .s2bank from a stream. */
    void fromStream(std::istream & ist);

    vector<FeatureDescription>::iterator
    getFeatureBegin(){return feats.begin();}

    vector<FeatureDescription>::iterator
    getFeatureEnd(){return feats.end();}

    vector<CapacityDescription>::iterator
    getCapacityBegin(){return caps.begin();}

    vector<CapacityDescription>::iterator
    getCapacityEnd(){return caps.end();}

    int
    getCapacityValue(string key){return caps.value(key);}

    void setNumPatches(int n);
    int getNumPatches(){return patches.size();}
    int resetPatch(int ipatch);
    int deletePatch(int ipatch);
    void loadPatch(int ipatch, istream iss);

    string 
    getPatchAsString(int ipatch);

    vector<vector<int> > 
    getPatchAsSysExes(int ipatch);

    float 
    getStoredParAsFloat(int ipatch, const string &parkey);

    float 
    getEffectiveParAsFloat(int ipatch, const string &parkey);

    vector<int> 
    getEffectiveParAsSysEx(int ipatch, const string &parkey);

    //std::vector<int>getPatchAsMIDISysex(int ipatch);
    /** Enables an on/off feature. Other features may be enabled as a
     *  side-effect. Functions registered by registerFeatureCallback()
     *  will be called.
     */
    void 
    enableFeature(const string &key);
    /** Disables an on/off feature. Other features may be disabled as
     *  a side-effect. Functions registered by
     *  registerFeatureCallback() will be called.
     */
    void 
    disableFeature(const string &key);
    /** Sets a capacity value. Functions registered by
     *  setCapacityCallback() will be called.
     */
    void 
    setCapacity(const string &key, int value);
    //bool isFeatureEnabled(const string &key);
    void registerFeatureCallback(FeatCallback cb);
    void registerCapacityCallback(CapacityCallback cb);
  };

}

#endif
