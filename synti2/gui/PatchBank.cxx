#include "PatchBank.hpp"
#include "Synti2Base.hpp"

#include <iostream>
using namespace std;

namespace synti2base
{

void Capacities::addCapacityDescription(string key,
                                        string cname,
                                        string humanReadable,
                                        int min,
                                        int max,
                                        int initial,
                                        string requiresf
                                       )
{
    capkeys.push_back(key);
    capValue[key] = initial;
    caps.push_back(CapacityDescription(key,cname,humanReadable,min,max,requiresf));
}

/** Initializes integer-type capacities of the synth. */
void Capacities::initCapacityDescriptions()
{
    addCapacityDescription("num_channels","NUM_CHANNELS","Number of channels (patches)",1,256,16,"");
    addCapacityDescription("num_delays","NUM_DELAY_LINES","Number of delay lines",1,8,8,"delay");
    addCapacityDescription("num_ops","NUM_OPERATORS","Number of operators/oscillators per channel", 0,4,4,"fm|add");
    addCapacityDescription("num_envs", "NUM_ENVS", "Number of envelopes per channel", 1,6,6,"");
    addCapacityDescription("num_knees","NUM_ENV_KNEES","Number of knees per envelope", 2,5,5,"");
    addCapacityDescription("num_mods","NUM_MODULATORS","Number of controllable modulators per channel",0,4,4,"mods");
}

void Capacities::toStream(ostream &ost)
{
    ost << "# The following numeric capacities are selected while editing this bank:" << endl;
    vector<CapacityDescription>::const_iterator it;
    for(it=begin(); it!=end(); ++it)
    {
        ost << (*it).getKey() << " " << capValue[(*it).getKey()] << endl;
    }
    ost << "--- End capacity description" << endl;
}

void Capacities::reloadValuesFromStream(istream &ist){
    string line;
    while(getline(ist,line)){
        if (line[0] == '#') continue;
        if (line[0] == '-') break;
        stringstream ss(line);
        string key;
        int value;
        getline(ss, key, ' ');
        if (capValue.find(key) == capValue.end()){
            std::cerr<< "Unknown key: " << key << endl;
            continue; /* FIXME: Unknown key, actually */
        }
        ss >> value;
        capValue[key] = value;
    }
    /* FIXME: Proper parser along the original ideas, instead of the above
    hack-hackedy-hack.
    */
}

void Capacities::exportHeader(ostream &ost)
{
    ost << "/* Capacities (numeric) */ " << endl;
    vector<CapacityDescription>::const_iterator it;
    for(it=begin(); it!=end(); ++it)
    {
        ost << "#define ";
        ost.width(20);
        ost.fill(' ');
        ost << std::left << (*it).getCDefine();
        ost.width(3);
        ost.fill(' ');
        ost << value((*it).getKey());
        ost << " /*" << (*it).getHumanReadable() << "*/";
        ost << endl;
    }
}


void Features::addFeatureDescription(string key,
                                     string cname,
                                     string humanReadable,
                                     string requires)
{
    featkeys.push_back(key);
    featureEnabled[key] = true; // init everything to enabled.
    feats.push_back(FeatureDescription(key, cname, humanReadable, requires));
}

/** Initializes the on/off -toggled feature descriptions */
void Features::initFeatureDescriptions()
{
    /*
      These to be toggled by "COMPOSE_MODE" switch only:
      FIXME: Or should these be always on in COMPOSE_MODE?
      addFeatureDescription("sysex","FEAT_COMPOSE_MODE_SYSEX","Receive SysEx (usable in compose mode only)","");
      addFeatureDescription("troubleshooting","FEAT_SAFETY","Safe mode (usable in compose mode only)","");
    */
    /*
      This won't be necessary in the final version. 1/10 cents is
      enough accuracy for everybody. (or is it!? experience will tell.)
      addFeatureDescription("pfine","FEAT_PITCH_FINE_DETUNE","Enable fine detune (convenience only!)","");
    */
    addFeatureDescription("fm","FEAT_APPLY_FM","Use frequency modulation","");
    addFeatureDescription("add","FEAT_APPLY_ADD","Use addition (source mixing)","");
    addFeatureDescription("noise","FEAT_NOISE_SOURCE","Use noise source","");
    addFeatureDescription("pdetune","FEAT_PITCH_DETUNE","Enable detuning of operators","");
    addFeatureDescription("delay","FEAT_DELAY_LINES","Enable delay lines","");
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
    addFeatureDescription("pitenv","FEAT_PITCH_ENVELOPE","Enable pitch envelopes","");
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

void Features::toStream(ostream &ost)
{
    ost << "# The following on/off features are selected while editing this bank:" << endl;
    vector<FeatureDescription>::const_iterator it;
    for(it=begin(); it!=end(); ++it)
    {
        if (featureEnabled[(*it).getKey()]){
            ost << " " << (*it).getKey();
        }
    }
    ost << endl;
}

void Features::reloadValuesFromStream(istream &ist){
    string line;
    while(getline(ist,line)){
        if (line[0] == '#') continue;
        else break;
    }
    resetAllTo(false);
    stringstream ss(line);
    string key;
    while (getline(ss, key, ' ')) {
        if (featureEnabled.find(key) == featureEnabled.end()){
            std::cerr<< "Unknown key: " << key << endl;
            continue; /* FIXME: Unknown key, actually */
        }
        featureEnabled[key] = true;
    }
    /* FIXME: Proper parser along the original ideas, instead of the above
    hack-hackedy-hack.
    */
}

void Features::exportHeader(ostream &ost)
{
    ost << "/* Enabled (#define) features */ " << endl;
    vector<FeatureDescription>::const_iterator it;
    for(it=begin(); it!=end(); ++it)
    {
        if (!value((*it).getKey())) continue;
        ost << "#define ";
        //ost << (value((*it).getKey())?"#define ":"#undef ");
        ost.width(30);
        ost.fill(' ');
        ost << std::left << (*it).getCDefine();
        ost << " /*" << (*it).getHumanReadable() << " */" << endl;
    }
}


class ParameterEnablerRuleAction : public RuleAction {
private:
    PatchBank *target;
    string key;
public:
    ParameterEnablerRuleAction(PatchBank *t, const string &k):target(t),key(k) {};
    virtual void action (bool ruleOutcome) const {
        target->setParEnabled(key,ruleOutcome);
    }
};

PatchBank::PatchBank():patches(16)
{
    midiSender = NULL;
    //toStream(cout); /*FIXME: for debug only.*/

    /* FIXME: Set up ruleactions for parameter enable/disable.*/
    vector<string>::const_iterator it;
    for (it=getI4Begin(0);it!=getI4End(0); ++it)
    {
        RuleSet rs = getI4Par(0,*it).getRuleSet();
        rs.ownThisAction(new ParameterEnablerRuleAction(this,*it));
        registerRuleAction(rs);
        paramEnabled[*it] = true; // Initialize everything to enabled.
    }
    for (it=getFBegin(0);it!=getFEnd(0); ++it)
    {
        RuleSet rs = getFPar(0,*it).getRuleSet();
        rs.ownThisAction(new ParameterEnablerRuleAction(this,*it));
        registerRuleAction(rs);
        paramEnabled[*it] = true; // Initialize everything to enabled.
    }
}


bool PatchBank::ruleIsSatisfied(RuleSet const & rs)
{
    vector<string> const & ks = rs.getKeys();
    vector<int> const & vs = rs.getThresholds();
    bool cond = true;
    for(size_t i=0; i<ks.size(); ++i)
    {
        bool thiscond = (getFeatCap(ks[i]) > vs[i]);
        cond &= thiscond;
        /*
        cerr << "Conditional " << (i+1) << "/" << ks.size() << ": "
             << ks[i] << " > " << vs[i]
             << "  (" << getFeatCap(ks[i]) << " > " << vs[i] << ") ?"
             << " --> " << thiscond << "  overall: " << cond << endl;
             */
    }
    cerr << "Final decision: " << cond << endl;
    return cond;
}

void PatchBank::callRuleActions(string const & key)
{
    vector<RuleSet> const & rss = capfeat_rulesets[key];
    vector<RuleSet>::const_iterator it;
    for(it=rss.begin(); it<rss.end(); ++it)
    {
        (*it).getRuleAction()->action(ruleIsSatisfied(*it));
    }
}

/** Applies all the rule actions that have been registered. */
void PatchBank::forceAllRuleActions()
{
    map<string, vector<RuleSet> >::const_iterator it;
    for(it=capfeat_rulesets.begin(); it != capfeat_rulesets.end(); ++it)
    {
        callRuleActions((*it).first);
    }
}

/* FIXME: Different output format for "non-defining" parameters.. */
void PatchBank::toStream(ostream & ost)
{
    ost << "# Output by PatchBank::toStream()" << endl;
    caps.toStream(ost);
    feats.toStream(ost);
    midimap.write(ost);
    ost << "--- Patchdata begins" << endl;
    vector<Patch>::iterator pit;
    for(pit=patches.begin(); pit!=patches.end(); ++pit)
    {
        (*pit).valuesToStream(ost);
    }
}

void PatchBank::writeOnePatch(size_t ipat, ostream &ost){
    ost << "# Synti2 single patch." << endl;
    ost << "# Output by " << __FILE__ << endl;
    ost << "# Capacities and features at the time of writing this patch:" << endl;
    caps.toStream(ost);
    feats.toStream(ost);
    ost << "# One patch follows. (may contain values disabled while editing)." << endl;
    patches[ipat].valuesToStream(ost);
    // Midimap plays no role for single patch, so omit that.
}

void PatchBank::readOnePatch(size_t ipat, istream & ist){
    patches[ipat].valuesFromStream(ist);
}


void PatchBank::reloadFromStream(istream & ist)
{
    caps.reloadValuesFromStream(ist);
    feats.reloadValuesFromStream(ist);
    midimap.read(ist);

    string line;
    getline(ist,line);
    if (line != "--- Patchdata begins") return;
    size_t ii=0;
    // FIXME: Implement a proper stop for the following loop:
    while(!ist.eof()){
        readOnePatch(ii,ist);
        if(true /* some check to see if we're at the end.*/) ii++; else break;
    }

    /* Then we need to do some more actions.. maybe here, maybe somewhere else.*/
    forceAllRuleActions();
    sendAllPatches();
    notifyReloadListeners();
}

void PatchBank::exportCapFeatHeader(ostream & ost)
{
  ost << "/** Capacities and features for a customized, unique build of"
      << endl
      << " * the synti2 software synthesizer." << endl << " *" << endl
      << " * Exported by PatchBank.cxx - don't edit manually." << endl
      << " */"<< endl;
  ost << "#ifndef SYNTI2_CAPACITIES_DEFINED" << endl;
  ost << "#define SYNTI2_CAPACITIES_DEFINED" << endl << endl;
  
  caps.exportHeader(ost);
  ost<<endl;

  feats.exportHeader(ost);
  ost<<endl;

  /* FIXME: Rulesets need to be checked somewhere before export!! */
  vector<string>::const_iterator sit;
  int ind;
  
  ost << std::dec;
  ind = 0;
  for (sit=getI4Begin(0); sit!=getI4End(0); ++sit){
    RuleSet rs = getI4Par(0,*sit).getRuleSet();
    if (!ruleIsSatisfied(rs)) continue;
    string const & cdef = getI4Par(0,*sit).getCDefine();
    ost << "#define ";
    ost.width(30);
    ost.fill(' ');
    ost << std::left;
    ost << ("IPAR_" + cdef);
    ost << " " << (ind++) << endl;
  }
  ost << "#define NUM_IPARS " << ind << endl << endl;
  
  int nipar = ind;
  ind = 0;
  for (sit=getFBegin(0); sit!=getFEnd(0); ++sit){
    RuleSet rs = getFPar(0,*sit).getRuleSet();
    if (!ruleIsSatisfied(rs)) continue;
    string const & cdef = getFPar(0,*sit).getCDefine();
    ost << "#define ";
    ost.width(30);
    ost.fill(' ');
    ost << std::left;
    ost << ("FPAR_" + cdef);
    ost << " " << nipar+(ind++) << endl;
  }
  ost << "#define NUM_FPARS " << ind << endl << endl;
  
  ost << "/* Constants that may depend on custom values: */" << endl;
  ost << "#define ENV_TRIGGER_STAGE (NUM_ENV_KNEES + 1)" << endl
      << "#define ENV_RELEASE_STAGE 2" << endl
      << "#define ENV_LOOP_STAGE 1" << endl
      << "#define NUM_ENV_DATA (NUM_ENV_KNEES*2)" << endl;
  
  ost << endl << "#endif" << endl;
}

void
PatchBank::setParEnabled(string const & key, bool status)
{
    bool old_status = paramEnabled[key];
    std::cerr << (status?"Enabling":"Disabling") << " "<< key << endl;
    paramEnabled[key] = status;
    if (status != old_status) {
      sendParamOnAllPatches(key);
    }
}

static
void
encode_f_to_text(float v, ostream &ost){
  unsigned char buf[4];
  unsigned int intval = synti2::encode_f(v);
  size_t len = encode_varlength(intval, buf);
  for (size_t i=0;i<len;i++){
    ost << int(buf[i]) << ", " ;
  }
}

void 
PatchBank::exportStandalone(ostream &ost)
{
  ost << "/* This is automatically generated by PatchBank. */" << std::endl;
  ost << "/* A sound patch bank for synti2. */" << endl;
  
  ost << "unsigned char patch_sysex[] = {" << endl; 
  

  for (unsigned int ipat=0; ipat<getNumPatches(); ipat++){
    vector<string>::const_iterator it;
    std::vector<unsigned char> bytes;

    ost << "    ";
    for(it=getI4Begin(ipat);it!=getI4End(ipat);++it){
      if (isParEnabled(*it)){
        float v = getStoredParAsFloat(ipat, (*it));
        bytes.push_back(v);
        //encode_f_to_text(v, ost);
        // Consecutive bytes:
        ost << (int)v << ", ";
      }
    }
    /*
    // Original version; make byte-size pairs
    if ((bytes.size()%2) == 1) bytes.push_back(0);
    for (size_t i = 0; i<bytes.size(); i += 2){
      int towrite = (bytes[i]<<4) + bytes[i+1];
      ost << towrite << ", ";
    }
    */
    ost << "/* Patch " << ipat << " integer pars.*/" << endl;

        //ost << "/* Patch " << ipat << " " << (*it) << " = " 
        //    << v <<  "*/" << endl;


    for(it=getFBegin(ipat);it!=getFEnd(ipat);++it){
      if (isParEnabled(*it)){
        float v = getStoredParAsFloat(ipat, (*it));
        ost << "    ";
        encode_f_to_text(v, ost);
        ost << "/* Patch " << ipat << " " << (*it) << " = " 
            << v <<  "*/" << endl;
      }
    }
  }
  
  //bytes.push_back(0xf7);


  /*for (unsigned int i=0; i<bytes.size(); i++){
    ost << int(bytes[i]) << ", ";
    if (((i+1)%16) == 0){
      ost << std::endl << "    ";
    }
  }*/

  ost << "  0xf7 /* End-of-data marker */ " << endl;
  ost << "};" << std::endl;
  
  std::cerr << "Wrote it?" << std::endl;
}



float
PatchBank::getStoredParAsFloat(size_t ipatch, const string &parkey){
    return patches[ipatch].getValue(parkey);
}

/** Returns a MIDI SysEx message that would change the value of a
 * parameter in a compose-mode synti2 instance. Actual value is returned
 * when a feature is turned on, and a zero or "no-effect" value is
 * returned when relevant features are off.
 *
 * Note: The features need to be checked on PatchBank side; lower levels
 * will have no knowledge of them.
 */
std::vector<unsigned char>
PatchBank::getEffectiveParAsSysEx(int ipatch, const string &parkey)
{

    vector<unsigned char> res;
    bool value_ok = isParEnabled(parkey);
    patches[ipatch].pushValToSysex(ipatch, parkey, res, value_ok);
    return res;
}


}

