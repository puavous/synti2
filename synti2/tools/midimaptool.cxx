#include "midimaptool.hpp"
#include "midihelper.hpp"
#include "synti2_misss.h"

void
synti2::MidiMap::setNoff(int midichn, bool val){
  mmap.chn[midichn].receive_note_off = val?1:0;
}

bool
synti2::MidiMap::getNoff(int midichn){
  return (mmap.chn[midichn].receive_note_off != 0);
}

std::vector<unsigned char>
synti2::MidiMap::sysexNoff(int midichn){
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(MISSS_SYSEX_MM_NOFF);
  res.push_back(midichn);
  res.push_back(getNoff(midichn)?1:0);
  synti2_sysex_footer(res);
  return res;
}

void
synti2::MidiMap::setMode(int midichn, int val){
  mmap.chn[midichn].mode = val;
}

int
synti2::MidiMap::getMode(int midichn){
  return mmap.chn[midichn].mode;
}

std::vector<unsigned char>
synti2::MidiMap::sysexMode(int midichn){
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(MISSS_SYSEX_MM_MODE);
  res.push_back(midichn);
  res.push_back(getMode(midichn));
  synti2_sysex_footer(res);
  return res;
}

