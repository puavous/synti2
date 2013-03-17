#ifndef FEATURE_HELPER_HPP
#define FEATURE_HELPER_HPP
/** Management of per-build synth features (part of capacities). Features
 *  can be turned on or off, and only the selected ones will be
 *  compiled in a synth executable.
 */
#include <string>
#include <vector>
#include <iostream>
#include "keyvalhelper.hpp"
namespace synti2 {
  class Features {
  protected:
    std::map<std::string, bool> feature;
    std::map<std::string, std::string> cdefin;
    void initDefinitions(){
      cdefin["delay"]    = "FEAT_DELAY_LINES";
      cdefin["fm"]       = "FEAT_APPLY_FM";
      cdefin["add"]      = "FEAT_APPLY_ADD";
      cdefin["sysex"]    = "FEAT_COMPOSE_MODE_SYSEX";
      cdefin["troubleshooting"] = "FEAT_SAFETY";
      cdefin["noise"]    = "FEAT_NOISE_SOURCE";
      cdefin["squash"]   = "FEAT_OUTPUT_SQUASH";
      cdefin["mods"]     = "FEAT_MODULATORS";
      cdefin["noff"]     = "FEAT_NOTE_OFF";
      cdefin["waves"]    = "FEAT_EXTRA_WAVETABLES";
      cdefin["looping"]  = "FEAT_LOOPING_ENVELOPES";
      cdefin["pscale"]   = "FEAT_PITCH_SCALING";
      cdefin["legato"]   = "FEAT_LEGATO";
      cdefin["pdetune"]  = "FEAT_PITCH_DETUNE";
      cdefin["pfine"]    = "FEAT_PITCH_FINE_DETUNE";
      cdefin["pfine"]    = "FEAT_PITCH_BEND";
      cdefin["velocity"] = "FEAT_VELOCITY_SENSITIVITY";
      cdefin["filter"]   = "FEAT_FILTER";
      cdefin["fres"]     = "FEAT_FILTER_RESO_ADJUSTABLE";
      cdefin["fresenv"]  = "FEAT_FILTER_RESO_ENVELOPE";
      cdefin["fcutenv"]  = "FEAT_FILTER_CUTOFF_ENVELOPE";
      cdefin["fpfollow"] = "FEAT_FILTER_FOLLOW_PITCH";
      cdefin["fnotch"]   = "FEAT_FILTER_OUTPUT_NOTCH";
      cdefin["stereo"]   = "FEAT_STEREO";
      cdefin["panenv"]   = "FEAT_PAN_ENVELOPE";
    }
    void setAll(bool value){
      std::map<std::string,std::string>::const_iterator it;
      for (it=cdefin.begin(); it!=cdefin.end(); ++it){
        feature[it->first] = value;
      }
    }
  public:
    void fromString(const std::string &s);
    Features(std::string s){fromString(s);}
    bool hasFeature(std::string name){
      return feature[name];
    }
    //void readFromStream(std::istream &ist){};
    //void writeToStream(std::ostream &ost);
    void writeAsDefinesForC(std::ostream &ost){
      std::map<std::string,std::string>::const_iterator it;
      for (it=cdefin.begin(); it!=cdefin.end(); ++it){
        if (feature[it->first]){
          ost << "#define " << it->second << std::endl;
        }
      }      
    };
  };
}

#endif
