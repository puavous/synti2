#include "midimaptool.hpp"
#include "midihelper.hpp"
#include "synti2_misss.h"

#include <sstream>

static
std::vector<unsigned char> 
sysexOneByte(int type, int midichn, int thebyte){
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(type);
  res.push_back(midichn);
  res.push_back(thebyte);
  synti2_sysex_footer(res);
  return res;
}

void
synti2::MidiMap::setMode(int midichn, int val){
  mmap.chn[midichn].mode = val;}

int
synti2::MidiMap::getMode(int midichn){
  return mmap.chn[midichn].mode;}

std::vector<unsigned char>
synti2::MidiMap::sysexMode(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_MODE,midichn,getMode(midichn));}


void
synti2::MidiMap::setSust(int midichn, bool val){
  mmap.chn[midichn].use_sustain_pedal = val?1:0;}

bool
synti2::MidiMap::getSust(int midichn){
  return (mmap.chn[midichn].use_sustain_pedal != 0);}

std::vector<unsigned char>
synti2::MidiMap::sysexSust(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_SUST, midichn, 
                      getSust(midichn)?1:0);}

void
synti2::MidiMap::setNoff(int midichn, bool val){
  mmap.chn[midichn].receive_note_off = val?1:0;}

bool
synti2::MidiMap::getNoff(int midichn){
  return (mmap.chn[midichn].receive_note_off != 0);}

std::vector<unsigned char>
synti2::MidiMap::sysexNoff(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_NOFF, midichn, 
                      getNoff(midichn)?1:0);}


void
synti2::MidiMap::setFixedVelo(int midichn, int val){
  mmap.chn[midichn].use_const_velocity = val;}

int
synti2::MidiMap::getFixedVelo(int midichn){
  return mmap.chn[midichn].use_const_velocity;}

std::vector<unsigned char>
synti2::MidiMap::sysexFixedVelo(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_CVEL,midichn,getFixedVelo(midichn));}



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

/* FIXME: Channel map here */


void
synti2::MidiMap::setBendDest(int midichn, int val){
  mmap.chn[midichn].bend_destination = val;}

int
synti2::MidiMap::getBendDest(int midichn){
  return mmap.chn[midichn].bend_destination;}

std::vector<unsigned char>
synti2::MidiMap::sysexBendDest(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_BEND,midichn,getBendDest(midichn));}


void
synti2::MidiMap::setPressureDest(int midichn, int val){
  mmap.chn[midichn].pressure_destination = val;}

int
synti2::MidiMap::getPressureDest(int midichn){
  return mmap.chn[midichn].pressure_destination;}

std::vector<unsigned char>
synti2::MidiMap::sysexPressureDest(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_PRESSURE,midichn,getPressureDest(midichn));}



/* FIXME: Mod sources, mins and maxes here. */

