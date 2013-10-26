#include "PatchBank.hpp"
#include "Synti2Base.hpp"

#include <iostream>

namespace synti2base {

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
  void Capacities::initCapacityDescriptions(){
    addCapacityDescription("num_channels","NUM_CHANNELS","Number of channels (patches)",1,256,16,"");
    addCapacityDescription("num_delays","NUM_DELAY_LINES","Number of delay lines",1,8,4,"delay");
    addCapacityDescription("num_ops","NUM_OPERATORS","Number of operators/oscillators per channel", 0,4,4,"fm|add");
    addCapacityDescription("num_envs", "NUM_ENVS", "Number of envelopes per channel", 1,6,2,"");
    addCapacityDescription("num_knees","NUM_ENV_KNEES","Number of knees per envelope", 2,5,5,"");
    addCapacityDescription("num_mods","NUM_MODULATORS","Number of controllable modulators per channel",0,4,4,"mods");
  }

  void Capacities::toStream(std::ostream &ost){
    ost << "# Can't really output capacities yet. But the code is here." << std::endl;
  }

  void Capacities::exportHeader(ostream &ost){
      ost << "/* Capacities (numeric) */ " << endl;
      vector<CapacityDescription>::const_iterator it;
      for(it=begin();it!=end();++it){
        ost << "#define ";
        ost.width(20); ost.fill(' ');
        ost << std::left << (*it).getCDefine();
        ost.width(3); ost.fill(' ');
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
    featureEnabled[key] = false; // init everything to disabled.
    feats.push_back(FeatureDescription(key, cname, humanReadable, requires));
  }

  /** Initializes the on/off -toggled feature descriptions */
  void Features::initFeatureDescriptions(){
    /*
      These to be toggled by "COMPOSE_MODE" switch only:

      addFeatureDescription("sysex","FEAT_COMPOSE_MODE_SYSEX","Receive SysEx (usable in compose mode only)",""); // Need this at all?
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

  void Features::toStream(std::ostream &ost){
    ost << "# Can't really output features yet. But the code is here." << std::endl;
  }

  void Features::exportHeader(ostream &ost){
      ost << "/* Enabled features */ " << endl;
      vector<FeatureDescription>::const_iterator it;
      for(it=begin();it!=end();++it){
        ost << "#define " ;
        ost.width(30); ost.fill(' ');
        ost << std::left << (*it).getCDefine();

        ost << " /*" << (*it).getHumanReadable() << " */" << endl;
        //ost.width(3); ost.fill(' ');
        //ost << value((*it).getKey());
        ost << endl;
      }
  }




  void MidiMap::toStream(std::ostream &ost){
    ost << "# Can't really output midimap yet. But the code is here." << std::endl;
  }

  PatchBank::PatchBank():patches(14)
  {
    toStream(std::cout); /*FIXME: for debug only.*/
  };

    void PatchBank::callRuleActions(string const & key)
    {
        vector<RuleSet> const & rss = capfeat_rulesets[key];
        vector<RuleSet>::const_iterator it;
        for(it=rss.begin(); it<rss.end(); ++it)
        {
            vector<string> const & ks = (*it).getKeys();
            vector<int> const & vs = (*it).getThresholds();
            bool cond = true;
            for(int i=0; i<ks.size(); ++i)
            {
                bool thiscond = (getFeatCap(ks[i]) > vs[i]);
                cond &= thiscond;
                cerr << "Conditional " << (i+1) << "/" << ks.size() << ": "
                     << ks[i] << " > " << vs[i]
                     << "  (" << getFeatCap(ks[i]) << " > " << vs[i] << ") ?"
                     << " --> " << thiscond << "  overall: " << cond << endl;
            }
            cerr << "Final decision: " << cond << endl;
            (*it).getRuleAction()->action(cond);
        }
    }

    /** Applies all the rule actions that have been registered. */
    void PatchBank::forceAllRuleActions(){
        map<std::string, vector<RuleSet> >::const_iterator it;
        for(it=capfeat_rulesets.begin();it != capfeat_rulesets.end();++it){
            callRuleActions((*it).first);
        }
    }


  void PatchBank::toStream(std::ostream & ost){
    ost << "# Output by PatchBank::toStream()" << std::endl;
    caps.toStream(ost);
    feats.toStream(ost);
    std::vector<Patch>::iterator pit;
    for(pit=patches.begin(); pit!=patches.end(); ++pit){
      (*pit).toStream(ost);
    }
    midimap.toStream(ost);
  }

  void PatchBank::fromStream(std::istream & ist){
    std::cerr << "PatchBank::fromStream() Can't read yet!" << std::endl;
  }

  void PatchBank::exportCapFeatHeader(std::ostream & ost){
      ost << "/** Capacities and features for a customized, unique build of"
          << endl
          << " * the synti2 software synthesizer." << endl << " *" << endl
          << " * Exported by PatchBank.cxx - don't edit manually." << endl
          << " */"<< endl;

      caps.exportHeader(ost);
      feats.exportHeader(ost);
      // export I4 and F parameter indices.
  }

}
