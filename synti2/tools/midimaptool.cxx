#include "midimaptool.hpp"
#include "midihelper.hpp"
#include "synti2_misss.h"

#include <sstream>
#include <cstring>

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

static
std::vector<unsigned char> 
sysexTwoBytes(int type, int midichn, int firstbyte, int secondbyte){
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(type);
  res.push_back(midichn);
  res.push_back(firstbyte);
  res.push_back(secondbyte);
  synti2_sysex_footer(res);
  return res;
}


synti2::MidiMap::MidiMap(){
  memset (&mmap, 0, sizeof(mmap));
  memset (&state, 0, sizeof(state));
}

void
synti2::MidiMap::write(std::ostream &os)
{ 
  /* FIXME: Re-implement. Needs to be "forall params{write param}".
   * The control section must be just another type of patch. Actually
   * I want a generic synth patch editor, to which I can easily hook
   * also my Roland, Yamaha, Akai, Alesis, and Dave Smith. And which
   * supports "song switching" in a live situation. But that's in the
   * next project... For now, I'll hack onwards.
   */
  os << "--- Mapper section begins ---" << std::endl;
  for(int ic=0;ic<16;ic++){
    os << "#Mapping of Midi channel " << ic << std::endl;
    os << "Mode " << getMode(ic) << std::endl;
    os << "Sust " << getSust(ic) << std::endl;
    os << "Noff " << getNoff(ic) << std::endl;
    os << "FixedVelo " << getFixedVelo(ic) << std::endl;
    os << "Voices " << getVoicesString(ic) << std::endl;
    os << "KeyMap ";
    for (int ik=0;ik<128;ik++){
      os << (ik==0?"":",") << getKeyMap(ic,ik);
    } 
    os << std::endl;
    os << "BendDest " << getBendDest(ic) << std::endl;
    os << "PressureDest " << getPressureDest(ic) << std::endl;
    for (int imod=0;imod<NCONTROLLERS;imod++){
      os << "Mod " << (imod+1);
      os << "," << getModSource(ic,imod);
      os << "," << getModMin(ic,imod);
      os << "," << getModMax(ic,imod);
      os << std::endl;
    }
  }
}


static int readMapParInt(std::istream &ifs, const std::string &name, int defa){
  std::string line;
  if (!std::getline(ifs, line)){
    std::cerr<<"Map data ended prematurely." << std::endl;
    return defa;
  }
  if (line.substr(0,name.length()) != name){
    std::cerr<<"Map data name mismatch." << std::endl;
    return defa;
  }
  std::stringstream ss(line);
  while ((!ss.eof()) && (ss.peek()<'0') || (ss.peek()>'9')) ss.ignore();
  int val;
  ss >> val;
  return val;
}

static std::string readMapParStr(std::istream &ifs, const std::string &name, std::string defa){
  std::string line;
  if (!std::getline(ifs, line)){
    std::cerr<<"Map data ended prematurely." << std::endl;
    return defa;
  }
  if (line.substr(0,name.length()) != name){
    std::cerr<<"Map data name mismatch." << std::endl;
    return defa;
  }
  std::stringstream ss(line);
  while ((!ss.eof()) && (ss.peek()!=' ')) ss.ignore();
  ss.ignore();
  std::string res;
  ss >> res;
  return res;
}

void
synti2::MidiMap::read(std::istream &ifs)
{ 
  std::string line;
  while(std::getline(ifs, line)){
    if (line == "--- Mapper section begins ---") break;
  }
  if (line != "--- Mapper section begins ---"){
    std::cerr << "No map data found in expected location. Skip read." << std::endl; 
    std::cerr << "Read: " << line << std::endl;
    return;
  }
  /*
  if((!std::getline(ifs, line))
     || (line!="--- Mapper section begins ---")){
    std::cerr << "No map data found in expected location. Skip read." << std::endl; 
    return;
    }*/
  for(int ic=0;ic<16;ic++){
    readMapParInt(ifs, "#Mapping of", 0);
    setMode(ic, readMapParInt(ifs, "Mode", 0));
    setSust(ic, readMapParInt(ifs, "Sust", 0));
    setNoff(ic,readMapParInt(ifs, "Noff", 0));
    setFixedVelo(ic, readMapParInt(ifs, "FixedVelo", 0));
    setVoices(ic, readMapParStr(ifs, "Voices", "1"));
    setKeyMap(ic, readMapParStr(ifs, "KeyMap", ""));
    setBendDest(ic, readMapParInt(ifs, "BendDest", 0));
    setPressureDest(ic, readMapParInt(ifs, "PressureDest", 0));
    for (int imod=0;imod<NCONTROLLERS; imod++){
      setMod(ic, imod, readMapParStr(ifs, "Mod", "0,0,0,0"));
    }
  }
  return; 
}

std::vector<synti2::MisssEvent> 
synti2::MidiMap::midiToMisss(const MidiEvent &evin)
{
  unsigned char inbuf[5];
  unsigned char outbuf[NPARTS*(1+3+2*sizeof(float))];
  int msgsizes[NPARTS];
  
  std::vector<synti2::MisssEvent> res;
  evin.toMidiBuffer(inbuf);
  int nmsg;
  nmsg = synti2_midi_to_misss(&mmap, &state,
                              inbuf, outbuf,
                              msgsizes,
                              0 /* FIXME: problem here?*/);
  unsigned char *read;
  read = outbuf;
  for(int i=0;i<nmsg;i++){
    res.push_back(MisssEvent(read));
    read += msgsizes[i];
  }

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

void 
synti2::MidiMap::setKeyMap(int midichn, std::string sval){
  std::stringstream ss(sval);
  int voi;
  for (int ik=0;ik<128;ik++){
    while ((!ss.eof()) && (ss.peek()<'0') || (ss.peek()>'9')) ss.ignore();
    ss >> voi;
    if (ss.fail()){
        std::cerr << "unexpected format of key map. bail out." << std::endl;
        return;
    }
    setKeyMap(midichn, ik, voi);
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

void 
synti2::MidiMap::setKeyMap(int midichn, int key, int val){
  mmap.chn[midichn].note_channel_map[key] = val;}

int 
synti2::MidiMap::getKeyMap(int midichn, int key){
  return mmap.chn[midichn].note_channel_map[key];}

std::vector<unsigned char> 
synti2::MidiMap::sysexKeyMapSingleNote(int midichn, int key){
  return sysexTwoBytes(MISSS_SYSEX_MM_MAPSINGLE,midichn,key,
                       getKeyMap(midichn,key));
}

std::vector<unsigned char> 
synti2::MidiMap::sysexKeyMapAll(int midichn){
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(MISSS_SYSEX_MM_MAPALL);
  res.push_back(midichn);
  for (int i=0;i<128;i++){
    res.push_back(getKeyMap(midichn,i));
  }
  synti2_sysex_footer(res);
  return res;
}

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


void
synti2::MidiMap::setModSource(int midichn, int imod, int val){
  mmap.chn[midichn].mod_src[imod] = val;}

int
synti2::MidiMap::getModSource(int midichn, int imod){
  return mmap.chn[midichn].mod_src[imod];}

void
synti2::MidiMap::setModMin(int midichn, int imod, float val){
  val = synti2::decode_f(synti2::encode_f(val));
  mmap.chn[midichn].mod_min[imod] = val;
}

void
synti2::MidiMap::setMod(int midichn, int imod, std::string sval){
  std::stringstream ss(sval);
  int val; float fval;
  ss >> val;  /*mod num */
  if ((val-1) != imod) return;
  ss.ignore(); ss >> val; setModSource(midichn, imod, val);
  ss.ignore(); ss >> fval; setModMin(midichn, imod, fval);
  ss.ignore(); ss >> fval; setModMax(midichn, imod, fval);
}


float
synti2::MidiMap::getModMin(int midichn, int imod){
  return mmap.chn[midichn].mod_min[imod];}

void
synti2::MidiMap::setModMax(int midichn, int imod, float val){
  val = synti2::decode_f(synti2::encode_f(val));
  mmap.chn[midichn].mod_max[imod] = val;}

float
synti2::MidiMap::getModMax(int midichn, int imod){
  return mmap.chn[midichn].mod_max[imod];}

/* Sends all the parameters of one mod in a package: */
std::vector<unsigned char>
synti2::MidiMap::sysexMod(int midichn, int imod)
{
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(MISSS_SYSEX_MM_MODDATA);
  res.push_back(midichn);
  res.push_back(imod);
  res.push_back(getModSource(midichn,imod));
  push_to_sysex_f(res, getModMin(midichn,imod));
  push_to_sysex_f(res, getModMax(midichn,imod));
  synti2_sysex_footer(res);
  return res;
}


/* Needed? Useful? */

/*
void
synti2::MidiMap::setGlobalInstaRamp(float val)
{
  for (int midichn = 0; midichn < 16; midichn++){
    mmap.chn[midichn].instant_ramp_length = val;
  }
}
*/

/*
float
synti2::MidiMap::getInstaRamp(int midichn){
  return mmap.chn[midichn].instant_ramp_length;}
*/
