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
void write(std::ostream &os){ std::cerr << "FIXME: not implemented"; return; }
void read(std::ostream &os){ std::cerr << "FIXME: not implemented"; return; }

void setNoff(int midichn, bool val);
bool getNoff(int midichn);
std::vector<unsigned char> sysexNoff(int midichn);

void setMode(int midichn, int val);
int getMode(int midichn);
std::vector<unsigned char> sysexMode(int midichn);

};

  
}

#endif
