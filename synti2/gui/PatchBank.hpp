#ifndef SYNTI2_PATCHBANK2
#define SYNTI2_PATCHBANK2

#include <map>
#include <string>
#include <vector>
#include <istream>
#include <iostream>

#include "Synti2Base.hpp"
#include "Patch.hpp"

using namespace std;

namespace synti2base {

  /* The following things need quite a close binding, because there
     are rules connecting capacities, features, and parameters that
     need/not to be set in a patch. */

  /** "Capacities" of a synti2 stand-alone compilation - integral
   * numbers of channels and oscillators..
   */
  class Capacities {
  private:
    vector<string> capkeys;
    vector<CapacityDescription> caps;
    map<string,int> capValue;
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
    vector<CapacityDescription>::iterator
    begin(){return caps.begin();}

    vector<CapacityDescription>::iterator
    end(){return caps.end();}

    int value(string key){
      return capValue[key];
    }
    void setValue(string key, int val){
        capValue[key]=val;
    }
    bool hasKey(string key){return capValue.find(key) != capValue.end();}
    void toStream(ostream &ost);
  };

  /** "Features" of a synti2 stand-alone compilation - boolean on/off
   * values of synth modules to be compiled and to take part in
   * compilation.
   */
  class Features {
  private:
    vector<string> featkeys;
    vector<FeatureDescription> feats;
    map<string,bool> featureEnabled;
    void addFeatureDescription(string key,
                               string cname,
                               string humanReadable,
                               string requires);
    /** Initializes the on/off -toggled feature descriptions */
    void initFeatureDescriptions();
  public:
    Features(){initFeatureDescriptions();}

    vector<FeatureDescription>::iterator
    begin(){return feats.begin();}

    vector<FeatureDescription>::iterator
    end(){return feats.end();}

    bool hasKey(string key){
        return featureEnabled.find(key) != featureEnabled.end();
    }

    int value(string key){return featureEnabled[key]?1:0;}

    void setValue(string key, int enabled){
        featureEnabled[key] = (enabled > 0);
    }

    void toStream(ostream &ost);
  };

  class MidiMap{
  private:
    //initMidiMap();
  public:
    MidiMap(){/*initMidiMap();*/}
    void toStream(ostream &ost);
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
    vector<Patch> patches;

    /** RuleSets and their actions to be performed upon cap/feat change: */
    map<string, vector<RuleSet> > capfeat_rulesets;

    string lastErrorMessage;

    /** */
    bool checkParamValue(string key, float v){
      lastErrorMessage = "None shall pass (sanity check unimplemented)";
      return false; /*FIXME: Implement.*/
    }

  public:
    PatchBank();

    /** Writes a .s2bank to a stream. */
    void toStream(ostream & ost);
    /** Reads a .s2bank from a stream. */
    void fromStream(istream & ist);

    vector<FeatureDescription>::iterator
    getFeatureBegin(){return feats.begin();}

    vector<FeatureDescription>::iterator
    getFeatureEnd(){return feats.end();}

    vector<CapacityDescription>::iterator
    getCapacityBegin(){return caps.begin();}

    vector<CapacityDescription>::iterator
    getCapacityEnd(){return caps.end();}

    vector<string>::iterator
    getI4Begin(size_t ipatch){return patches[ipatch].getI4Begin();}

    vector<string>::iterator
    getI4End(size_t ipatch){return patches[ipatch].getI4End();}

    vector<string>::iterator
    getFBegin(size_t ipatch){return patches[ipatch].getFBegin();}

    vector<string>::iterator
    getFEnd(size_t ipatch){return patches[ipatch].getFEnd();}

    int
    getCapacityValue(string key){return caps.value(key);}

    void setNumPatches(int n);
    size_t getNumPatches(){return patches.size();}
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

    //vector<int>getPatchAsMIDISysex(int ipatch);
    /** Enables an on/off feature. Other features may be enabled as a
     *  side-effect. Functions registered by registerFeatureCallback()
     *  will be called.
     */
    void
    setFeature(const string &key, int value){
        feats.setValue(key, value);
        callRuleActions(key);
    }
    void
    setCapacity(const string &key, int value){
        caps.setValue(key, value);
        callRuleActions(key);
    }
    //bool isFeatureEnabled(const string &key);

    int
    getFeatCap(const string &key){
        if(feats.hasKey(key)){
            return feats.value(key);
        } else if (caps.hasKey(key)) {
            return caps.value(key);
        }
        return -1;
    }

    void registerRuleAction(RuleSet irs){
        vector<string> keys = irs.getKeys();
        for (int i=0; i<keys.size(); ++i){
            capfeat_rulesets[keys[i]].push_back(irs);
        }
    }

    void callRuleActions(string const & key){
        vector<RuleSet> const & rss = capfeat_rulesets[key];
        vector<RuleSet>::const_iterator it;
        for(it=rss.begin();it<rss.end();++it){
            vector<string> const & ks = (*it).getKeys();
            vector<int> const & vs = (*it).getThresholds();
            bool cond = true;
            for(int i=0; i<ks.size(); ++i){
                cerr << "Conditional " << ks[i] << " > " << vs[i] << endl;
                cerr << "By values   " << getFeatCap(ks[i])
                  << " > " << vs[i] << endl;
                cond &= (getFeatCap(ks[i]) > vs[i]);
            }
            (*it).getRuleAction()->action(cond);
        }
    }

    I4Par const& getI4Par (size_t ipat, string const& key) const {
      return patches[ipat].getI4Par(key);
    }

    FPar const& getFPar(size_t ipat, string const& key) const {
      return patches[ipat].getFPar(key);
    }

    bool setParamValue(string key, float v){
      bool can_do = checkParamValue(key, v);
      if (can_do){
        //privSetParam(key,v);
        //privSendParam(key,v);
        //viewUpdater->updateAllConnectedViews(key,v);
      }
      return can_do;
    };
    string getLastErrorMessage(){return lastErrorMessage;}

  };

}

#endif
