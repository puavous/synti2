#ifndef SYNTI2_PATCHBANK2
#define SYNTI2_PATCHBANK2

#include "featuretool.hpp"
using synti2::Features;

#include <string>
#include <vector>
#include <istream>

using std::string;
using std::vector;
using std::istream;

namespace synti2base {

  class I4ParDescription{
    std::string key;
    std::string tooltip;
    int defval;
    int allowed;
    std::vector<std::string> names;
  };

  class FParDescription{
    std::string key;
    std::string tooltip;
    float defval;
    float minval;
    float maxval;
    float step;
  };

  class Patch{};

  /** 
   * PatchBank stores everything related to a synti2 patch bank, i.e.,
   * the enabled features, selected synth capabilities, GUI
   * preferences, MIDI mapping, and the patches themselves.
   *
   * Output (.s2bank, .s2patch, .c, MIDI SysEx) is given separately
   * for compose mode (full index set) and stand-alone mode (index set
   * restricted by feature setup)
   *
   * External communication is via strings, MIDI, and integers
   * only. All the patch logic is encapsulated behind the PatchBank
   * interface.
   */
  class PatchBank {
    Features feats;
    std::vector<Patch> patches;
  public:
    PatchBank():patches(14){};

    vector<string>
    getFeatureKeys()
    {
      return feats.getFeatureKeys();
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
    void enableFeature(const string &key);
    void disableFeature(const string &key);
    void setCapacity(const string &key, int value);
    //void isFeatureEnabled(string key);
  };

}

#endif
