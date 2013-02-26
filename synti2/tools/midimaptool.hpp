/** Midi mapper tool, class and stuff... 
 * 
 * This is essentially a wrapper for the C-structure used in the
 * realtime/offline midi filter.
*/
#ifndef SYNTI2_MIDIMAPTOOL_H
#define SYNTI2_MIDIMAPTOOL_H

#include "synti2_midi_guts.h"
#include <iostream>
#include <vector>

namespace synti2{
  
  /** FIXME: Put this to its own module, like missstool.{hpp,cxx}
      and maybe the MisssSong should be there too.
   */
  class MisssEvent{
    /* FIXME: Proper interface and so on.. */
    int type; /* MisssEvent is either a note or a ramp.. */
    int voice; /* Always on a voice. */
    int par1; /* Always one param*/
    int par2; /* In notes also velocity. */
    float time; /* In ramps time and target value*/
    float target; /* In ramps time and target value*/
  }

  class MidiMap{
  private:
    /* We wrap the whole thingy here: */
    /* FIXME: instantiation.*/
    synti2_midi_map mmap;
    synti2_midi_state state;
  public:
    MidiMap();
    void write(std::ostream &os);
    void read(std::istream &ins);

    /** Wrapper for midi->misss conversion */
    std::vector<MisssEvent> midiToMisss(const MidiEvent &evin);
    
    void setMode(int midichn, int val);
    int getMode(int midichn);
    std::vector<unsigned char> sysexMode(int midichn);
    
    void setSust(int midichn, bool val);
    bool getSust(int midichn);
    std::vector<unsigned char> sysexSust(int midichn);

    void setNoff(int midichn, bool val);
    bool getNoff(int midichn);
    std::vector<unsigned char> sysexNoff(int midichn);
    
    void setFixedVelo(int midichn, int val);
    int getFixedVelo(int midichn);
    std::vector<unsigned char> sysexFixedVelo(int midichn);

    void setVoices(int midichn, const std::string &val);
    std::string getVoicesString(int midichn);
    std::vector<unsigned char> sysexVoices(int midichn);

    void setKeyMap(int midichn, int key, int val);
    void setKeyMap(int midichn, std::string sval);
    int getKeyMap(int midichn, int key);
    std::vector<unsigned char> sysexKeyMapSingleNote(int midichn, int key);
    std::vector<unsigned char> sysexKeyMapAll(int midichn);

    void setBendDest(int midichn, int val);
    int getBendDest(int midichn);
    std::vector<unsigned char> sysexBendDest(int midichn);

    void setPressureDest(int midichn, int val);
    int getPressureDest(int midichn);
    std::vector<unsigned char> sysexPressureDest(int midichn);

    void setModSource(int midichn, int imod, int val);
    int getModSource(int midichn, int imod);
    std::vector<unsigned char> sysexModSource(int midichn, int imod);

    void setModMin(int midichn, int imod, float val);
    float getModMin(int midichn, int imod);
    std::vector<unsigned char> sysexModMin(int midichn, int imod);

    void setModMax(int midichn, int imod, float val);
    float getModMax(int midichn, int imod);
    std::vector<unsigned char> sysexModMax(int midichn, int imod);

    std::vector<unsigned char> sysexMod(int midichn, int imod);
    void setMod(int midichn, int imod, std::string sval);

    
  };
  
}

#endif
