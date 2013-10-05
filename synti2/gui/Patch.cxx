#include "Patch.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <cstdlib>

using std::string;

const std::string defPatch = "# Patch design.\n"
  "#\n"
  "# Lines beginning with '#' and empty lines are ignored by tools.\n"
  "#\n"
  "# Captions in brackets '[thus]' begin a section of different data\n"
  "# type/precision.\n"
  "#\n"
  "# This defines a Patch along with all necessary details, including GUI \n"
  "# hints for parameters \n"
  "\n"
  "##############  Patch data \n"
  "\n"
  "# Format, one line per parameter: \n"
  "# NAME Description Value ALLOWED/(MIN MAX) GUIPrecision GUIGroup\n"
  "\n"
  "[I4]\n"
  "# 4-bit integer parameters:\n"
  "# These end up as SYNTI2_I4_NAME\n"
  "\n"
  "# TODO: Should perhaps encode the possible values by 16-bit bit-masks,\n"
  "# for better flexibility (mostly the feedback thing would require\n"
  "# this) zero is always allowed, i.e., 0xffff equals range 0-15, 0xfffe\n"
  "# equals range 0,2-15 etc.\n"
  "\n"
  "HARM1 Op1Wav 0 0x00ff   0 5\n"
  "HARM2 Op2Wav 0 0x00ff   0 5\n"
  "HARM3 Op3Wav 0 0x00ff   0 5\n"
  "HARM4 Op4Wav 0 0x00ff   0 5\n"
  "\n"
  "\n"
  "# Indices of the operator amplitude envelopes\n"
  "EAMP1 Op1AmpEnv   0 0x002f 0 0\n"
  "EAMP2 Op2AmpEnv   0 0x002f 0 0\n"
  "EAMP3 Op3AmpEnv   0 0x002f 0 0\n"
  "EAMP4 Op4AmpEnv   0 0x002f 0 0\n"
  "EAMPN NoiseAmpEnv 0 0x002f 0 4\n"
  "\n"
  "# Indices of the pitch envelopes\n"
  "EPIT1 Op1PitchEnv 0 0x002f   0 1\n"
  "EPIT2 Op2PitchEnv 0 0x002f   0 1\n"
  "EPIT3 Op3PitchEnv 0 0x002f   0 1\n"
  "EPIT4 Op4PitchEnv 0 0x002f   0 1\n"
  "\n"
  "EPAN PanEnv       0 0x002f   0 2\n"
  "EFILT FiltCutEnv  0 0x002f   0 3\n"
  "EFILR FiltResEnv  0 0x002f   0 3\n"
  "\n"
  "# Ampl. Velocity sensitivity (0=no effect 1=fully sensitive) \n"
  "\n"
  "VS1 Oper1VelSens   0 0x0001   2 14\n"
  "VS2 Oper2VelSens   0 0x0001   2 14\n"
  "VS3 Oper3VelSens   0 0x0001   2 14\n"
  "VS4 Oper4VelSens   0 0x0001   2 14\n"
  "VSN NoiseVelSens   0 0x0001   2 4\n"
  "VSC FiltCutVelSens 0 0x0001   2 3\n"
  "\n"
  "# Number of envelope knees to jump backwards from last knee\n"
  "# if looping envelope logic is compiled in.\n"
  "# DANGER: all-zero times inside loop lead to infinite iteration!!!\n"
  "ELOOP1 En1LoopLen 0 0x0007   0 2\n"
  "ELOOP2 En2LoopLen 0 0x0007   0 2\n"
  "ELOOP3 En3LoopLen 0 0x0007   0 2\n"
  "ELOOP4 En4LoopLen 0 0x0007   0 2\n"
  "ELOOP5 En5LoopLen 0 0x0007   0 2\n"
  "ELOOP6 En6LoopLen 0 0x0007   0 2\n"
  "\n"
  "# need to express 0..1,4\n"
  "FMTO1 FModTo1 0 0x0008 0 3\n"
  "FMTO2 FModTo2 0 0x0009   0 3\n"
  "FMTO3 FModTo3 0 0x000c   0 3\n"
  "FMTO4 FModTo4 0 0x000f   0 3\n"
  "\n"
  "ADDTO1 MixTo1 0 0x0008   0 4\n"
  "ADDTO2 MixTo2 0 0x0009   0 4\n"
  "ADDTO3 MixTo3 0 0x000c   0 4\n"
  "ADDTO4 MixTo4 0 0x000f   0 4\n"
  "\n"
  "# Filter LP,BP,HP,Notch\n"
  "FILT FilterMode 0 0x000f 0 16\n"
  "\n"
  "# Filter pitch follow source operator\n"
  "FFOLL FiltFollowOsc 0 0x000f 0 16\n"
  "\n"
  "# Need an even number of 3-bit parameters:\n"
  "#xxx UNUSED0 0 0x0000 0 18\n"
  "\n"
  "[F]\n"
  "# 'Floating point' parameters:\n"
  "\n"
  "# This is a 'hack' for null controller target.\n"
  "# (Not used for sound generation.)\n"
  "NULLTGT NOT_Editable 0 0 0 0 1\n"
  "\n"
  "# Global output mix level for convenience\n"
  "MIXLEV MixLev 0 0.0 1.0  2 19\n"
  "\n"
  "# Global pan for some stereophonic aural excitation\n"
  "MIXPAN MixPan 0 -1.0 1.0  2 19\n"
  "\n"
  "# Source Levels (before multiplicative amplitude envelope)\n"
  "# This is for additional 'gain' or phase inversion.\n"
  "\n"
  "LV1 Oper1Lev 0 -8.0 8.0   2 15\n"
  "LV2 Oper2Lev 0 -8.0 8.0   2 15\n"
  "LV3 Oper3Lev 0 -8.0 8.0   2 15\n"
  "LV4 Oper4Lev 0 -8.0 8.0   2 15\n"
  "LVN NoiseLev 0 -8.0 8.0   2 15\n"
  "LVD DlyInLev 0 -8.0 8.0   2 15\n"
  "\n"
  "\n"
  "# After ENVS, envelope params need to be ordered and consistent with the code!\n"
  "ENVS    En1:1T 0 0 0.255   3 6\n"
  "ENV1K1L En1:1L 0 0 1.27   2 6\n"
  "ENV1K2T En1:2T 0 0 1.27   2 6\n"
  "ENV1K2L En1:2L 0 0 1.27   2 6\n"
  "ENV1K3T En1:3T 0 0 1.27   2 6\n"
  "ENV1K3L En1:3L 0 0 1.27   2 6\n"
  "ENV1K4T En1:4T 0 0 1.27   2 6\n"
  "ENV1K4L En1:4L 0 0 1.27   2 6\n"
  "ENV1K5T En1:5T 0 0 1.27   2 6\n"
  "ENV1K5L En1:5L 0 0 1.27   2 6\n"
  "\n"
  "ENV2K1T En2:1T 0 0 0.255   3 7\n"
  "ENV2K1L En2:1L 0 0 5.00   2 7\n"
  "ENV2K2T En2:2T 0 0 1.27   2 7\n"
  "ENV2K2L En2:2L 0 0 5.00   2 7\n"
  "ENV2K3T En2:3T 0 0 2.55   2 7\n"
  "ENV2K3L En2:3L 0 0 5.00   2 7\n"
  "ENV2K4T En2:4T 0 0 2.55   2 7\n"
  "ENV2K4L En2:4L 0 0 5.00   2 7\n"
  "ENV2K5T En2:5T 0 0 2.55   2 7\n"
  "ENV2K5L En2:5L 0 0 5.00   2 7\n"
  "\n"
  "ENV3K1T En3:1T 0 0 0.255   3 8\n"
  "ENV3K1L En3:1L 0 0 12.0   1 8\n"
  "ENV3K2T En3:2T 0 0 1.27   2 8\n"
  "ENV3K2L En3:2L 0 0 12.0   1 8\n"
  "ENV3K3T En3:3T 0 0 2.55   2 8\n"
  "ENV3K3L En3:3L 0 0 12.0   1 8\n"
  "ENV3K4T En3:4T 0 0 2.55   2 8\n"
  "ENV3K4L En3:4L 0 0 12.0   1 8\n"
  "ENV3K5T En3:5T 0 0 2.55   2 8\n"
  "ENV3K5L En3:5L 0 0 12.0   1 8\n"
  "\n"
  "ENV4K1T En4:1T 0  0 0.255   3 9\n"
  "ENV4K1L En4:1L 0 -1.27 1.27   2 9\n"
  "ENV4K2T En4:2T 0  0 1.27   2 9\n"
  "ENV4K2L En4:2L 0 -1.27 1.27   2 9\n"
  "ENV4K3T En4:3T 0  0 2.56   2 9\n"
  "ENV4K3L En4:3L 0 -1.27 1.27   2 9\n"
  "ENV4K4T En4:4T 0  0 5.11   2 9\n"
  "ENV4K4L En4:4L 0 -1.27 1.27   2 9\n"
  "ENV4K5T En4:5T 0  0 2.56   2 9\n"
  "ENV4K5L En4:5L 0 -1.27 1.27   2 9\n"
  "\n"
  "ENV5K1T En5:1T 0  0  1.27   2 10\n"
  "ENV5K1L En5:1L 0 -63 +64   0 10\n"
  "ENV5K2T En5:2T 0  0  1.27   2 10\n"
  "ENV5K2L En5:2L 0 -63 +64   0 10\n"
  "ENV5K3T En5:3T 0  0  1.27   2 10\n"
  "ENV5K3L En5:3L 0 -63 +64   0 10\n"
  "ENV5K4T En5:4T 0  0  1.27   2 10\n"
  "ENV5K4L En5:4L 0 -63 +64   0 10\n"
  "ENV5K5T En5:5T 0  0  1.27   2 10\n"
  "ENV5K5L En5:5L 0 -63 +64   0 10\n"
  "\n"
  "ENV6K1T En6:1T 0  0 1.27   2  11\n"
  "ENV6K1L En6:1L 0 -31.0 32.0   1 11\n"
  "ENV6K2T En6:2T 0  0 1.27   2 11\n"
  "ENV6K2L En6:2L 0 -31.0 32.0   1 11\n"
  "ENV6K3T En6:3T 0  0 1.27   2 11\n"
  "ENV6K3L En6:3L 0 -31.0 32.0   1 11\n"
  "ENV6K4T En6:4T 0  0 1.27   2 11\n"
  "ENV6K4L En6:4L 0 -31.0 32.0   1 11\n"
  "ENV6K5T En6:5T 0  0 1.27   2 11\n"
  "ENV6K5L En6:5L 0 -31.0 32.0   1 11\n"
  "\n"
  "\n"
  "# Detune/Relative fundamental pitch (added to 'note from keyboard')\n"
  "# NOTE: Order of these is hardcoded as of 2012-03-04:\n"
  "\n"
  "DT1  Op1Crs 0  -48.0 48.0   1 12\n"
  "DT2  Op2Crs 0  -48.0 48.0   1 12\n"
  "DT3  Op3Crs 0  -48.0 48.0   1 12\n"
  "DT4  Op4Crs 0  -48.0 48.0   1 12\n"
  "\n"
  "#Not used anymore:\n"
  "#DT1F Op1Fine 0     -.5  .5     3 13\n"
  "#DT2F Op2Fine 0     -.5  .5     3 13\n"
  "#DT3F Op3Fine 0     -.5  .5     3 13\n"
  "#DT4F Op4Fine 0     -.5  .5     3 13\n"
  "\n"
  "# Notes to bend at positive maximum:\n"
  "PBAM   Op1BendAmt 0 0.0 24.0 1 18\n"
  "PBAM2  Op2BendAmt 0 0.0 24.0 1 18\n"
  "PBAM3  Op3BendAmt 0 0.0 24.0 1 18\n"
  "PBAM4  Op4BendAmt 0 0.0 24.0 1 18\n"
  "# Bend value, not for editing but for pitch bend messages.\n"
  "# Bend is used by patching controller #4 to this value.\n"
  "PBVAL NOT_Editable 0 -1.0  1.0  2 18\n"
  "\n"
  "\n"
  "# Filter parameters; the very basic ones\n"
  "FFREQ Cutoff     0 0.0 128.0   1 17\n"
  "FRESO Resonance  0 0.0 1.0     2 17\n"
  "\n"
  "# Delay lines in/out; Number of parameters\n"
  "# must be consistent with code! TODO: Generate parameters\n"
  "# according to code instead of hard-wiring separately here.\n"
  "\n"
  "DINLV1 Dly1InLev 0  -1.0 1.0   2 16\n"
  "DINLV2 Dly2InLev 0  -1.0 1.0   2 16\n"
  "DINLV3 Dly3InLev 0  -1.0 1.0   2 16\n"
  "DINLV4 Dly4InLev 0  -1.0 1.0   2 16\n"
  "DINLV5 Dly5InLev 0  -1.0 1.0   2 16\n"
  "DINLV6 Dly6InLev 0  -1.0 1.0   2 16\n"
  "DINLV7 Dly7InLev 0  -1.0 1.0   2 16\n"
  "DINLV8 Dly8InLev 0  -1.0 1.0   2 16\n"
  "\n"
  "# Delay lengths in milliseconds\n"
  "DLEN1 Dly1Length 0  0.0 2000   0 17\n"
  "DLEN2 Dly2Length 0  0.0 2000   0 17\n"
  "DLEN3 Dly3Length 0  0.0 2000   0 17\n"
  "DLEN4 Dly4Length 0  0.0 2000   0 17\n"
  "DLEN5 Dly5Length 0  0.0 200   1 17\n"
  "DLEN6 Dly6Length 0  0.0 200   1 17\n"
  "DLEN7 Dly7Length 0  0.0 200   2 17\n"
  "DLEN8 Dly8Length 0  0.0 200   2 17\n"
  "\n"
  "\n"
  "# Delay output levels\n"
  "DLEV1 Dly1OutLev 0  0.0 1.0   2 18\n"
  "DLEV2 Dly2OutLev 0  0.0 1.0   2 18\n"
  "DLEV3 Dly3OutLev 0  0.0 1.0   2 18\n"
  "DLEV4 Dly4OutLev 0  0.0 1.0   2 18\n"
  "DLEV5 Dly5OutLev 0  0.0 1.0   2 18\n"
  "DLEV6 Dly6OutLev 0  0.0 1.0   2 18\n"
  "DLEV7 Dly7OutLev 0  0.0 1.0   2 18\n"
  "DLEV8 Dly8OutLev 0  0.0 1.0   2 18\n"
  "\n"
  "\n"
  "# Controller destinations (indices of fpars)\n"
  "# Hmm.. Should these be auxiliary as well?\n"
  "# No.. it is OK to have these per patch.\n"
  "# But the GUI needs some tweaking for these! \n"
  "CDST1 Cont1Dest 0 0.0 127 0 20\n"
  "CDST2 Cont2Dest 0 0.0 127 0 20\n"
  "CDST3 Cont3Dest 0 0.0 127 0 20\n"
  "CDST4 Cont4Dest 0 0.0 127 0 20\n"
  "\n"
  "# Note transition time in seconds\n"
  "LEGLEN LegatoLen 0 0.0 1.0 2 17\n"
  "\n"
  "# Pitch scaling for toms and effects. 'note*(1-pscale) + detune + envs'\n"
  "# Scaling could have been different for each operator, but I don't think\n"
  "# it could have created worthy enough effects. Sure about this?\n"
  "PSCALE PitchScale 0 -1.0 2.0 2 18\n"
  "\n";


/* helper functions */
static
bool line_is_whitespace(std::string &str){
  return (str.find_first_not_of(" \t\n\r") == str.npos);
}

static
void line_to_header(std::string &str){
  int endmark = str.find_first_of(']');
  int len = endmark-1;
  str.assign(str.substr(1,len));
}

static
std::string line_chop(std::string &str){
  unsigned long beg = str.find_first_not_of(" \t\n\r");
  unsigned long wbeg = str.find_first_of(" \t\n\r", beg);
  if (wbeg == str.npos) wbeg = str.length();
  std::string res = str.substr(beg,wbeg-beg);
  if (wbeg<str.length()) str.assign(str.substr(wbeg,str.length()-wbeg));
  return std::string(res);
}


void Patch::addI4Par(std::string s){
  I4Par par(s);
  i4parKeys.push_back(par.getKey());
  i4pars[par.getKey()] = par;
}

void Patch::addFPar(std::string s){
  FPar par(s);
  fparKeys.push_back(par.getKey());
  fpars[par.getKey()] = par;
}

/*
void Patch::initI4Pars(){
  addI4Par("HARM1 Op1Wav 0 0x00ff   0 5");
}
void Patch::initFPars(){
  addFPar("ENV4K3T En4:3T 0  0 2.56   2 9");
}
*/

void FPar::fromLine(string line){
  string skey = line_chop(line);
  string smnemonic = line_chop(line);
  string svalue = line_chop(line);
  string smin = line_chop(line);
  string smax = line_chop(line);
  string sprecision = line_chop(line);
  string sguigroup = line_chop(line);
  key = skey;
  cdefine = skey;
  humanReadable = smnemonic;
  minval = std::strtod(smin.c_str(), 0);
  maxval = std::strtod(smax.c_str(), 0);
  precision = std::strtol(sprecision.c_str(), 0, 0);
  guigroup = std::strtol(sguigroup.c_str(), 0, 0);
}

void FPar::toStream(std::ostream &ost){
  ost << key
      << " " << humanReadable
      << " " << value
      << " " << minval
      << " " << maxval
      << " " << precision
      << " " << guigroup
      << std::endl;
}

FPar::FPar(){fromLine("a b 0 1 2 3 4");}

FPar::FPar(string line){
  fromLine(line);
}

void I4Par::fromLine(string line){
  string skey = line_chop(line);
  string smnemonic = line_chop(line);
  string svalue = line_chop(line);
  string sallowed = line_chop(line);
  string sprecision = line_chop(line);
  string sguigroup = line_chop(line);
  key = skey;
  cdefine = skey;
  humanReadable = smnemonic;
  value = std::strtol(svalue.c_str(), 0, 0);
  allowed = std::strtol(sallowed.c_str(), 0, 0);
  guigroup = std::strtol(sguigroup.c_str(), 0, 0);
}

I4Par::I4Par(string line){
  fromLine(line);
}

void I4Par::toStream(std::ostream &ost){
  ost << key 
      << " " << humanReadable
      << " " << value
      << " " << allowed
      << " " << 0
      << " " << guigroup
      << std::endl;
}

I4Par::I4Par(){fromLine("a b 0 1 2 3");}

Patch::Patch(){
  std::stringstream isst(defPatch);
  fromStream(isst);
  /*
  initI4Pars();
  initFPars();
  */
}

void Patch::toStream(std::ostream & ost){
  std::vector<string>::iterator it;
  ost << "# This output is by Patch::toStream()" << std::endl;
  ost << "UNNAMED" << std::endl;
  ost << "#UNCOMMENTED" << std::endl;
  ost << "# Patch data for 'UNNAMED' begins" << std::endl;
  ost << "UNNAMED" << std::endl;
  ost << "[I4]" << std::endl;
  for(it=i4parKeys.begin();it!=i4parKeys.end();++it){
    i4pars[(*it)].toStream(ost);
  }
  ost << "[F]" << std::endl;
  for(it=fparKeys.begin();it!=fparKeys.end();++it){
    fpars[(*it)].toStream(ost);
  }
  ost << "--- end of patch " << std::endl;
}

/** FIXME: Should not wire key indices from here!? */
void Patch::fromStream(std::istream & ifs){
  std::string line, curr_section("");
  std::string pname, pdescr;
  while(std::getline(ifs, line)){
    if (line_is_whitespace(line)) continue;
    if (line[0]=='#') continue;
    if (line[0]=='['){
      /* Begin section */
      line_to_header(line);
      curr_section = line;
      continue;
    };
    /* Else it is a parameter description. */
    if(curr_section=="I4"){
      I4Par par(line);
      i4parKeys.push_back(par.getKey());
      i4parInd[par.getKey()]=i4pars.size();
      i4pars[par.getKey()]=par;
    } else if (curr_section=="F"){
      FPar par(line);
      fparKeys.push_back(par.getKey());
      fparInd[par.getKey()]=i4pars.size();
      fpars[par.getKey()]=par;
    } else {
      std::cerr << "Unknown section: " << curr_section << std::endl;
    }
  }
}
