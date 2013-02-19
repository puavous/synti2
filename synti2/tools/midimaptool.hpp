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
  
  class MidiMap{
  private:
    synti2_midi_map mmap;
  public:
    void write(std::ostream &os){ std::cerr << "FIXME: map write not implemented"; return; }
    void read(std::istream &ins){ std::cerr << "FIXME: map read not implemented"; return; }
    
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

    /* FIXME: Channel map here. */

    void setBendDest(int midichn, int val);
    int getBendDest(int midichn);
    std::vector<unsigned char> sysexBendDest(int midichn);

    void setPressureDest(int midichn, int val);
    int getPressureDest(int midichn);
    std::vector<unsigned char> sysexPressureDest(int midichn);

    void setModSource(int midichn, int val);
    int getModSource(int midichn);
    std::vector<unsigned char> sysexModSource(int midichn);

    void setModMin(int midichn, float val);
    float getModMin(int midichn);
    std::vector<unsigned char> sysexModMin(int midichn);

    void setModMax(int midichn, float val);
    float getModMax(int midichn);
    std::vector<unsigned char> sysexModMax(int midichn);
    
  };
  
}

#endif
