#include "MidiMap.hpp"
#include "midihelper.hpp"
#include "synti2_misss.h"
#include "synti2_limits.h"
#include "synti2_midi.h"
#include "synti2_midi_guts.h"

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


synti2base::MidiMap::MidiMap(){
  memset (&mmap, 0, sizeof(mmap));
  memset (&state, 0, sizeof(state));
  midiSender = NULL;
}

void synti2base::MidiMap::setMidiSender(MidiSender *ms){
  midiSender = ms;
}



void
synti2base::MidiMap::write(std::ostream &os)
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
    for (int imod=0;imod<NUM_MAX_MODULATORS;imod++){
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
  while ((!ss.eof()) && ((ss.peek()<'0') || (ss.peek()>'9'))) ss.ignore();
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
synti2base::MidiMap::read(std::istream &ifs)
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
    for (int imod=0;imod<NUM_MAX_MODULATORS; imod++){
      setMod(ic, imod, readMapParStr(ifs, "Mod", "0,0,0,0"));
    }
  }

  return;
}

std::vector<synti2base::MisssEvent>
synti2base::MidiMap::midiToMisss(const MidiEvent &evin)
{
  unsigned char inbuf[5];
  unsigned char outbuf[NUM_MAX_CHANNELS*(1+3+2*sizeof(float))];
  int msgsizes[NUM_MAX_CHANNELS];

  std::vector<synti2base::MisssEvent> res;
  evin.toMidiBuffer(inbuf);
  int nmsg;
#if 0
  nmsg = synti2_midi_to_misss(&mmap, &state,
                              inbuf, outbuf,
                              msgsizes,
                              0 /* FIXME: problem here?*/);
#else
  /* FIXME: I have a build problem with codeblocks.. the C part's symbol
     doesn't seem to be found. I suck in finding the problem now.
   */
  nmsg = 0;
#endif
  unsigned char *read;
  read = outbuf;
  for(int i=0;i<nmsg;i++){
    res.push_back(MisssEvent(read));
    read += msgsizes[i];
  }

  return res;
}

void synti2base::MidiMap::sendEverything(){
  if (midiSender == NULL) return;
  for(int ic=0;ic<16;ic++){
    midiSender->send(sysexMode(ic));
    midiSender->send(sysexSust(ic));
    midiSender->send(sysexNoff(ic));
    midiSender->send(sysexFixedVelo(ic));
    midiSender->send(sysexVoices(ic));
    midiSender->send(sysexKeyMapAll(ic));
    midiSender->send(sysexBendDest(ic));
    midiSender->send(sysexPressureDest(ic));
    for (int imod=0;imod<NUM_MAX_MODULATORS;imod++){
      midiSender->send(sysexMod(ic,imod));
    }
  }

}

void
synti2base::MidiMap::setMode(int midichn, int val){
  mmap.chn[midichn].mode = val;
  if (midiSender != NULL) midiSender->send(sysexMode(midichn));
}

int
synti2base::MidiMap::getMode(int midichn){
  return mmap.chn[midichn].mode;
}

std::vector<unsigned char>
synti2base::MidiMap::sysexMode(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_MODE,midichn,getMode(midichn));
}

void
synti2base::MidiMap::setSust(int midichn, bool val){
  mmap.chn[midichn].use_sustain_pedal = val?1:0;
  if (midiSender != NULL) midiSender->send(sysexSust(midichn));
}

bool
synti2base::MidiMap::getSust(int midichn){
  return (mmap.chn[midichn].use_sustain_pedal != 0);
}

std::vector<unsigned char>
synti2base::MidiMap::sysexSust(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_SUST, midichn,
                      getSust(midichn)?1:0);
}

void
synti2base::MidiMap::setNoff(int midichn, bool val){
  mmap.chn[midichn].receive_note_off = val?1:0;
  if (midiSender != NULL) midiSender->send(sysexNoff(midichn));
}

bool
synti2base::MidiMap::getNoff(int midichn){
  return (mmap.chn[midichn].receive_note_off != 0);
}

std::vector<unsigned char>
synti2base::MidiMap::sysexNoff(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_NOFF, midichn,
                      getNoff(midichn)?1:0);
}


void
synti2base::MidiMap::setFixedVelo(int midichn, int val){
  mmap.chn[midichn].use_const_velocity = val;
  if (midiSender != NULL) midiSender->send(sysexFixedVelo(midichn));
}

int
synti2base::MidiMap::getFixedVelo(int midichn){
  return mmap.chn[midichn].use_const_velocity;
}

std::vector<unsigned char>
synti2base::MidiMap::sysexFixedVelo(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_CVEL,midichn,getFixedVelo(midichn));
}



void
synti2base::MidiMap::setVoices(int midichn, const std::string &val){
  /* Input is a list of zero or more non-negative integers, separated
   * by any non-digits; Accept only a subsequence of integers that is
   * increasing and less than the maximum number of channels in any
   * possible compiled synth capacities. Other parts of the input are
   * silently ignored. FIXME: This will allow larger channel indices
   * than those actually available in a custom synth! Some truncation
   * logic needs to be applied at some point!!
   */
  std::stringstream ss(val);
  int voi, prev = 0;
  for (int i=0;i<NUM_MAX_CHANNELS;i++){
    while ((!ss.eof()) && ((ss.peek()<'0') || (ss.peek()>'9'))) ss.ignore();
    ss >> voi;
    if (ss.fail() || voi == 0){
      mmap.chn[midichn].voices[i] = 0;
      break;
    }
    if ((voi <= prev) || (voi > NUM_MAX_CHANNELS)) continue;
    mmap.chn[midichn].voices[i] = prev = voi;
  }
  if (midiSender != NULL) midiSender->send(sysexVoices(midichn));
}

std::string
synti2base::MidiMap::getVoicesString(int midichn){
  std::stringstream res;
  for (int i=0;i<NUM_MAX_CHANNELS;i++){
    int voi = mmap.chn[midichn].voices[i];
    if (voi==0) break;
    res << (i>0?",":"") << voi;
  }
  return res.str();
}

std::vector<unsigned char>
synti2base::MidiMap::sysexVoices(int midichn){
  std::vector<unsigned char> res;
  synti2_sysex_header(res);
  res.push_back(MISSS_SYSEX_MM_VOICES);
  res.push_back(midichn);
  for (int i=0;i<NUM_MAX_CHANNELS;i++){
    int voi = mmap.chn[midichn].voices[i];
    res.push_back(voi);
    if (voi==0) break; /* "Null-terminate". */
  }
  synti2_sysex_footer(res);
  return res;
}

#if 0
/* TODO: */
static void ignore_until_digit(std::istream ss){
  while ((!ss.eof()) && ((ss.peek()<'0') || (ss.peek()>'9'))) ss.ignore();
}
#endif

void
synti2base::MidiMap::setKeyMap(int midichn, std::string sval){
  std::stringstream ss(sval);
  int voi;
  for (int ik=0;ik<128;ik++){
    //ignore_until_digit(ss);
    while ((!ss.eof()) && ((ss.peek()<'0') || (ss.peek()>'9'))) ss.ignore();
    ss >> voi;
    if (ss.fail()){
        std::cerr << "unexpected format of key map. bail out." << std::endl;
        return;
    }
    setKeyMap(midichn, ik, voi);
  }
}

void
synti2base::MidiMap::setKeyMap(int midichn, int key, int val){
  mmap.chn[midichn].note_channel_map[key] = val;
  if (midiSender != NULL) midiSender->send(sysexKeyMapSingleNote(midichn,key));
}

int
synti2base::MidiMap::getKeyMap(int midichn, int key){
  return mmap.chn[midichn].note_channel_map[key];}

std::vector<unsigned char>
synti2base::MidiMap::sysexKeyMapSingleNote(int midichn, int key){
  return sysexTwoBytes(MISSS_SYSEX_MM_MAPSINGLE,midichn,key,
                       getKeyMap(midichn,key));
}

std::vector<unsigned char>
synti2base::MidiMap::sysexKeyMapAll(int midichn){
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
synti2base::MidiMap::setBendDest(int midichn, int val){
  mmap.chn[midichn].bend_destination = val;
  if (midiSender != NULL) midiSender->send(sysexBendDest(midichn));
}

int
synti2base::MidiMap::getBendDest(int midichn){
  return mmap.chn[midichn].bend_destination;
}

std::vector<unsigned char>
synti2base::MidiMap::sysexBendDest(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_BEND,midichn,getBendDest(midichn));
}


void
synti2base::MidiMap::setPressureDest(int midichn, int val){
  mmap.chn[midichn].pressure_destination = val;
  if (midiSender != NULL) midiSender->send(sysexPressureDest(midichn));
}

int
synti2base::MidiMap::getPressureDest(int midichn){
  return mmap.chn[midichn].pressure_destination;
}

std::vector<unsigned char>
synti2base::MidiMap::sysexPressureDest(int midichn){
  return sysexOneByte(MISSS_SYSEX_MM_PRESSURE,midichn,getPressureDest(midichn));
}


void
synti2base::MidiMap::setModSource(int midichn, int imod, int val){
  mmap.chn[midichn].mod_src[imod] = val;
  if (midiSender != NULL) midiSender->send(sysexMod(midichn,imod));
}

int
synti2base::MidiMap::getModSource(int midichn, int imod){
  return mmap.chn[midichn].mod_src[imod];}

void
synti2base::MidiMap::setModMin(int midichn, int imod, float val){
  val = synti2::decode_f(synti2::encode_f(val));
  mmap.chn[midichn].mod_min[imod] = val;
  if (midiSender != NULL) midiSender->send(sysexMod(midichn,imod));
}

void
synti2base::MidiMap::setMod(int midichn, int imod, std::string sval){
  std::stringstream ss(sval);
  int val; float fval;
  ss >> val;  /*mod num */
  if ((val-1) != imod) return;
  ss.ignore(); ss >> val; setModSource(midichn, imod, val);
  ss.ignore(); ss >> fval; setModMin(midichn, imod, fval);
  ss.ignore(); ss >> fval; setModMax(midichn, imod, fval);
  if (midiSender != NULL) midiSender->send(sysexMod(midichn,imod));
}


float
synti2base::MidiMap::getModMin(int midichn, int imod){
  return mmap.chn[midichn].mod_min[imod];}

void
synti2base::MidiMap::setModMax(int midichn, int imod, float val){
  val = synti2::decode_f(synti2::encode_f(val));
  mmap.chn[midichn].mod_max[imod] = val;
  if (midiSender != NULL) midiSender->send(sysexMod(midichn,imod));
}

float
synti2base::MidiMap::getModMax(int midichn, int imod){
  return mmap.chn[midichn].mod_max[imod];}

/* Sends all the parameters of one mod in a package: */
std::vector<unsigned char>
synti2base::MidiMap::sysexMod(int midichn, int imod)
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
