#include "midimaptool.hpp"
#include "midihelper.hpp"
#include "synti2_misss.h"

#include <sstream>

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

void 
synti2::MidiMap::setVoices(int midichn, const std::string &val){
  /* Input is a list of zero or more non-negative integers, separated
   * by any non-digits; Accept only a subsequence of integers that is
   * increasing and less than the number of channels in the compiled
   * capacities of the synth. Other parts of the input are silently
   * ignored.
   */
  std::stringstream ss(val);
  int voi, prev = 0;
  for (int i=0;i<NPARTS;i++){
    while ((!ss.eof()) && (ss.peek()<'0') || (ss.peek()>'9')) ss.ignore();
    ss >> voi;
    if (ss.fail() || voi == 0){
      mmap.chn[midichn].voices[i] = 0;
      return;
    }
    if ((voi <= prev) || (voi > NPARTS)) continue;
    mmap.chn[midichn].voices[i] = prev = voi;
  }
}

std::string 
synti2::MidiMap::getVoicesString(int midichn){
  std::stringstream res;
  for (int i=0;i<NPARTS;i++){
    int voi = mmap.chn[midichn].voices[i];
    if (voi==0) break;
    res << (i>0?",":"") << voi;
  }
  return res.str();
}

std::vector<unsigned char> 
synti2::MidiMap::sysexVoices(int midichn){
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(MISSS_SYSEX_MM_VOICES);
  res.push_back(midichn);
  for (int i=0;i<NPARTS;i++){
    int voi = mmap.chn[midichn].voices[i];
    res.push_back(voi);
    if (voi==0) break; /* "Null-terminate". */
  }
  synti2_sysex_footer(res);
  return res;
}
