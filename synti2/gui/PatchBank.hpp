#ifndef SYNTI2_PATCHBANK2
#define SYNTI2_PATCHBANK2

#include <map>
#include <string>
#include <vector>
#include <istream>
#include <iostream>

#include "Synti2Base.hpp"
#include "MidiMap.hpp"
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
    void reloadValuesFromStream(istream &ist);
    void exportHeader(ostream &ost);
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
    /* FIXME: Use these to send actual values when features are changed: */
    /* Should be part of features or not? */
    map<string,bool> i4ParIsDirty;
    map<string,bool> fParIsDirty;
    map<string,bool> parIsEnabled;
    void addFeatureDescription(string key,
                               string cname,
                               string humanReadable,
                               string requires);
    /** Initializes the on/off -toggled feature descriptions */
    void initFeatureDescriptions();
    void resetAllTo(bool state){
        vector<string>::const_iterator k;
        for(k=featkeys.begin();k!=featkeys.end();++k){
            featureEnabled[*k] = state;
        }
    }
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

    /** Stores the feature settings */
    void toStream(ostream &ost);

    void reloadValuesFromStream(istream &ist);

    /** Exports features for a custom exe build. */
    void exportHeader(ostream &ost);
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
    map<string,bool> paramEnabled;

    /** RuleSets and their actions to be performed upon cap/feat change: */
    map<string, vector<RuleSet> > capfeat_rulesets;

    MidiSender *midiSender;

    /** Checks if v is a valid value for a parameter. For floats, this is
     * a range, and for integers, a discrete set of nonzero values and zero;
     * The integers depend on capacities, so cap changes should be handled..
     */
    bool checkParamValue(string key, float v){
      std::cerr << "None shall pass (sanity check unimplemented)" << std::endl;
      return false; /*FIXME: Implement.*/
    }
    bool ruleIsSatisfied(RuleSet const & rs);
    void callRuleActions(string const & key);

  public:
    PatchBank();

    /** Writes a .s2bank to a stream. */
    void toStream(ostream & ost);
    /** Reads a .s2bank from a stream.
    FIXME: Actually should just initialize defaults here; rename accordingly!*/
    void fromStream(istream & ist);

    /** Loads a stored .s2bank from a file. Assume already initialized defaults.*/
    void reloadFromStream(istream & ist);

    void writeOnePatch(size_t ipat, ostream & ost);
    void readOnePatch(size_t ipat, istream & ist);
    void clearOnePatch(size_t ipat){cerr << "not impl." << endl;}
    void pleaseSendPanic(){cerr << "not impl." << endl;}

    /** Writes a C header file for stand-alone synth to a stream. */
    void exportCapFeatHeader(ostream & ost);
    /** Exports only the enabled parameters for a playble exe song. */
    void exportStandalone(ostream &ost);

    void setParEnabled(string const & key, bool status){
        bool old_status = paramEnabled[key];
        std::cout << (status?"Enabling":"Disabling") << " "<< key << endl;
        paramEnabled[key] = status;
        if (status != old_status) {
            sendParamOnAllPatches(key);
        }
    }

    void sendParamOnAllPatches(string const & key){
        // FIXME: All patches, and not only 0.
        sendMidi(getEffectiveParAsSysEx(0,key));
    }
    bool isParEnabled(string const & key){
        return paramEnabled[key];
    }

    /* Hmm.. Couldn't we just give out const references to feats/caps? */
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

    void sendPatch(size_t ipat){
        vector<string>::const_iterator it;
        /* FIXME: Keys in a single key vector!! */
        for(it=getI4Begin(ipat);it!=getI4End(ipat);++it){
            sendMidi(getEffectiveParAsSysEx(ipat,*it));
        }
        for(it=getFBegin(ipat);it!=getFEnd(ipat);++it){
            sendMidi(getEffectiveParAsSysEx(ipat,*it));
        }
    }

    void sendAllPatches(){
        for(int i=0;i<getNumPatches();++i) sendPatch(i);
    }

    int
    getCapacityValue(string key){return caps.value(key);}
    int
    getFeatureValue(string key){return feats.value(key);}

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
    getStoredParAsFloat(size_t ipatch, const string &parkey);

    float
    getEffectiveParAsFloat(int ipatch, const string &parkey);

    vector<unsigned char>
    getEffectiveParAsSysEx(int ipatch, const string &parkey);

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
        for (size_t i=0; i<keys.size(); ++i){
            capfeat_rulesets[keys[i]].push_back(irs);
        }
    }

    void forceAllRuleActions();

    I4Par const& getI4Par (size_t ipat, string const& key) const {
      return patches[ipat].getI4Par(key);
    }

    FPar const& getFPar(size_t ipat, string const& key) const {
      return patches[ipat].getFPar(key);
    }

    const string & getPatchName(size_t ipat) const {
        return patches[ipat].getName();
    }
    void setPatchName(size_t ipat, string const & n) {
        patches[ipat].setName(n);
    }

    /** Sets a midi sender. */
    void setMidiSender(MidiSender *ms){
        midiSender = ms;
    }

    /** Sends a parameter via midi, if a sender is given. */
    void sendMidi(vector<unsigned char> const & bytes){
        if (midiSender != NULL) midiSender->send(bytes);
    };

    void privSetParam(size_t ipat, string key, float v){
        //FIXME: patches[ipat]canAccept(key,v)?
        patches[ipat].setValue(key,v);
    }
    /** Sets a parameter value. */
    bool setParamValue(size_t ipat, string key, float v){

        // We can always set a parameter, regardless of limits/features
        privSetParam(ipat,key,v);
        // But we must send only the "effective" (limited) values.
        sendMidi(getEffectiveParAsSysEx(ipat,key));

        bool can_do = checkParamValue(key, v);
        if (can_do){
            //privSetParam(key,v);
            //privSendParam(key,v);
    /*
  FIXME: This maybe from a callback like registerParamChangeListener();
  send_to_jack_process(pbank->getSysex("I3",curr_patch,d));
  */

        //viewUpdater->updateAllConnectedViews(key,v);
        }
        return can_do;
    };
  };

}

#endif
