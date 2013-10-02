#ifndef SYNTI2_PATCHBANK2
#define SYNTI2_PATCHBANK2

#include <string>
#include <vector>
#include <istream>
#include <iostream>

using std::string;
using std::vector;
using std::istream;

namespace synti2base {
 
  class Description {
  private:
    string key;
    string cdefine;
    string humanReadable;
  public:
    Description(string ikey, string icdefine, string ihumanReadable)
      :key(ikey),cdefine(icdefine),humanReadable(ihumanReadable){};
    string getKey() const {return key;}
    string getCDefine() const {return cdefine;}
    string getHumanReadable() const {return humanReadable;}
  };

  class FeatureDescription : public Description{
  private:
    std::vector<std::string> reqkeys;
  public:
    FeatureDescription(string ik, string icd, string ihr, string ireq)
      : Description(ik,icd,ihr)
    {
      // should split/tokenize, if many will be needed.
      reqkeys.push_back(ireq);
    }
    bool doesRequire(std::string rkey)
    {
      return false;//"reqkeys.contains(rkey);"}
    }
  };

  class I4ParDescription : public Description {
  private:
    /** Default value */
    int defval; 
    /** Allowed values as a 16-bit pattern */ 
    int allowed; 
    /** Names to be used in GUI; empty->"0","1","2",.."15" */
    std::vector<std::string> names;
    //I4ParDescription(){};/*must give all data on creation*/
  public:
    I4ParDescription(string ik, string icd, string ihr,
                     int idef, int iallow,
                     std::vector<std::string> inames)
      : Description(ik,icd,ihr), defval(idef), allowed(iallow), names(inames)
    {
    }
  };

  class FParDescription : public Description {
  private:
    /** Default value */
    float defval;
    /** Minimum value (mutable) */
    float minval;
    /** Maximum value (mutable) */
    float maxval;
    /** Preferred granularity in nudged changes (mutable) */
    float step;
    //FParDescription(){};/*must give all data on creation*/
  public:
    FParDescription(string ik, string icd, string ihr,
                    float idef, float imin, float imax, float istep)
      : Description(ik,icd,ihr), defval(idef), minval(imin), maxval(imax),
        step(istep)
    {
    }
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


  class Patch{};

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
    std::vector<string> featkeys;
    std::vector<FeatureDescription> feats;
    std::vector<Patch> patches;
    void addFeatureDescription(string key, 
                               string cname,
                               string humanReadable,
                               string requires)
    {
      featkeys.push_back(key);
      feats.push_back(FeatureDescription(key, cname, humanReadable, requires));
    }

    /** Initializes the on/off -toggled feature descriptions */
    void initFeatureDescriptions(){
      /*
        These to be toggled by "COMPOSE_MODE" switch only:
        
      addFeatureDescription("sysex","FEAT_COMPOSE_MODE_SYSEX","Receive SysEx (usable in compose mode only)",""); // Need this at all?
      addFeatureDescription("troubleshooting","FEAT_SAFETY","Safe mode (usable in compose mode only)","");
      */
      addFeatureDescription("fm","FEAT_APPLY_FM","Use frequency modulation","");
      addFeatureDescription("add","FEAT_APPLY_ADD","Use addition matrix","");
      addFeatureDescription("noise","FEAT_NOISE_SOURCE","Use noise source","");
      addFeatureDescription("pdetune","FEAT_PITCH_DETUNE","Enable detuning of operators","");
      /*
      This won't be necessary in the final version. 1/10 cents is
      enough accuracy for everybody. (or is it!? experience will tell.)

      addFeatureDescription("pfine","FEAT_PITCH_FINE_DETUNE","Enable fine detune (convenience only!)","");
      */
      addFeatureDescription("delay","FEAT_DELAY_LINES","Enable delay effects","");
      addFeatureDescription("squash","FEAT_OUTPUT_SQUASH","Squash function at output","");
      addFeatureDescription("phase","FEAT_RESET_PHASE","Enable phase reset on note-on","");
      addFeatureDescription("waves","FEAT_EXTRA_WAVETABLES","Generate extra wavetables with harmonics","");
      addFeatureDescription("cube","FEAT_POWER_WAVES","Enable squared/cubed waveforms","");
      addFeatureDescription("mods","FEAT_MODULATORS","Use modulators / parameter ramps","");
      addFeatureDescription("pbend","FEAT_PITCH_BEND","Enable the 'pitch bend' modulator","mods");
      addFeatureDescription("noff","FEAT_NOTE_OFF","Listen to note off messages","");
      addFeatureDescription("velocity","FEAT_VELOCITY_SENSITIVITY","Respond to velocity","");
      addFeatureDescription("looping","FEAT_LOOPING_ENVELOPES","Enable looping envelopes","");
      addFeatureDescription("legato","FEAT_LEGATO","Enable legato","");
      addFeatureDescription("pscale","FEAT_PITCH_SCALING","Enable pitch scaling","");
      addFeatureDescription("filter","FEAT_FILTER","Enable the lo/band/hi-pass filter","");
      addFeatureDescription("fres","FEAT_FILTER_RESO_ADJUSTABLE","Enable adjustable filter resonance","");
      addFeatureDescription("fresenv","FEAT_FILTER_RESO_ENVELOPE","Enable resonance envelope","");
      addFeatureDescription("fcutenv","FEAT_FILTER_CUTOFF_ENVELOPE","Enable cutoff envelope","");
      addFeatureDescription("fpfollow","FEAT_FILTER_FOLLOW_PITCH","Enable filter frequency to follow pitch","");
      addFeatureDescription("fnotch","FEAT_FILTER_OUTPUT_NOTCH","Enable notch filter","");
      addFeatureDescription("csquash","FEAT_CHANNEL_SQUASH","Enable per-channel squash function","");
      addFeatureDescription("stereo","FEAT_STEREO","Enable stereo output","");
      addFeatureDescription("panenv","FEAT_PAN_ENVELOPE","Enable pan envelope","stereo");
    }
  public:
    PatchBank():patches(14)
    {
      initFeatureDescriptions();
    };

    vector<FeatureDescription>::iterator
    getFeatureBegin()
    {
      return feats.begin();
    }

    vector<FeatureDescription>::iterator
    getFeatureEnd()
    {
      return feats.end();
    }

    vector<I4ParDescription> 
    getI4ParKeys();

    vector<FParDescription> 
    getFParKeys();

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
