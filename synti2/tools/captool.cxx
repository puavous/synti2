/** Synth capacities and configuration. */
#include "captool.hpp"
#include "keyvalhelper.hpp"
#include "featuretool.hpp"
#include <iostream>
#include <sstream>

using std::endl;

const std::string cap_prologue =
  "#ifndef SYNTI2_CAPACITIES_INCLUDED\n"
  "#define SYNTI2_CAPACITIES_INCLUDED\n";
const std::string cap_epilogue = 
  "#endif\n";
const std::string cap_documentation = 
  "/** @file synti2_cap.h \n"
  " * \n"
  " * OBSERVE: This file is generated by the command line \n"
  " * configurator tool! Editing by hand is not recommended,\n"
  " * and neither should it be necessary. See cltool.cxx for\n"
  " * more details. Some documentation is given below, though.\n"
  " * \n"
  " * This file defines the 'capacities', or limits, of a specific \n"
  " * build of the Synti2 software synthesizer. The idea is that\n"
  " * more restricted custom values can be used in compiling the \n"
  " * final 4k target than those that are used while composing the \n"
  " * music and designing patches. The final 4k executable should\n"
  " * have only the minimum features that its musical score requires.\n"
  " *\n */\n\n"
  " /* Variables defined here (FIXME: refactor names!!) are:\n"
  "  * NPARTS - multitimbrality; number of max simultaneous sounds;\n"
  "  *          also the number of patches in the patch bank\n"
  "  * NENVPERVOICE - envelopes per voice\n"
  "  * NOSCILLATORS - oscillators/operators per voice\n"
  "  * NCONTROLLERS - oscillators/operators per voice\n"
  "  * NENVKNEES    - number of 'knees' in the envelope\n"
  "  * NDELAYS      - number of delay lines\n"
  "  * DELAYSAMPLES - maximum length of each delay line in samples\n"
  "  * SYNTI2_MAX_SONGBYTES  - maximum size of song sequence data\n"
  "  * SYNTI2_MAX_SONGEVENTS - maximum number of sequence events\n"
  "  */\n\n";

const std::string cap_defaults =
  "num_voices 16  \n"
  "num_envs 6\n"
  "num_ops 4\n"
  "num_mods 4\n"
  "num_knees 5\n"
  "num_delays 8\n"
  "features all\n";

synti2::Capacities::Capacities(std::istream &ist) {
  std::stringstream ss(cap_defaults);
  kval.readFromStream(ss);
  kval.readFromStream(ist);
}

void
synti2::Capacities::write(std::ostream &ost) const{
  ost << kval;
}

void
synti2::Capacities::writeCapH(std::ostream &ost) const {
  ost << cap_prologue;
  ost << cap_documentation;
  ost << "#define NPARTS " << kval.asInt("num_voices") << endl;
  ost << "#define NENVPERVOICE " << kval.asInt("num_envs") << endl;

  ost << "#define NOSCILLATORS " << kval.asInt("num_ops") << endl;
  ost << "#define NCONTROLLERS " << kval.asInt("num_mods") << endl;
  ost << "#define NENVKNEES " << kval.asInt("num_knees") << endl;
  ost << "#define NDELAYS " << kval.asInt("num_delays") << endl;

  /* Derived values.. */
  ost << "#define TRIGGERSTAGE "  << "(NENVKNEES+1)" << endl;
  ost << "#define SYNTI2_NENVD (NENVKNEES*2)" << endl;

  /* Some constants cannot change because of implementation. */
  ost << "#define RELEASESTAGE " << "2" << endl;
  ost << "#define LOOPSTAGE " << "1" << endl;

  /* Some remain hard-coded until necessary to modify.*/
  ost << "#define DELAYSAMPLES 0x10000" << endl;
  ost << "#define SYNTI2_MAX_SONGBYTES 30000" << endl;
  ost << "#define SYNTI2_MAX_SONGEVENTS 15000" << endl;
  ost << cap_epilogue;
}

void
synti2::Capacities::writeParamH(std::ostream &ost) const {
  Features f(kval.asString("features"));
}

void
synti2::Capacities::writePatchDesign(std::ostream &ost) const {
  Features f(kval.asString("features"));
  int ip = 0;
  /* FIXME: Should separate the gui part!! */
  ost << "[I3]" << endl;
  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    /* FIXME: Actually unnecessary, if !hasFeature("waves") */
    ost << "HARM" << iop+1 
        << " Op" << iop+1 << "Wav "
        << " 0 7 0 5" << endl;
    ip++;
  }
  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "EAMP" << iop+1
        << " Op" << iop+1 << "AmpEnv "
        << " 0 " << kval.asInt("num_envs") << " 0 0" << endl;
    ip++;
  }
  /* FIXME: Actually unnecessary, if !hasFeature("noise") */
  ost << "EAMP" << "N" 
      << " NoiseAmpEnv "
      << " 0 6 0 4" << endl;
  ip++;

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    /* FIXME: Actually unnecessary, if !hasFeature("pitchenv") */
    ost << "EPIT" << iop+1 
        << " Op" << iop+1 << "PitchEnv "
        << " 0 " << kval.asInt("num_envs") << " 0 1" << endl;
    ip++;
  }
  /* FIXME: Actually unnecessary, if !hasFeature("panenv") */
  ost << "EPAN"
      << " PanEnv "
      << " 0 " << kval.asInt("num_envs") << " 0 2" << endl;
  ip++;
  /* FIXME: And so on... should suppress unnecessary params. */

  ost << "EFILC"
      << " FiltCutEnv "
      << " 0 " << kval.asInt("num_envs") << " 0 3" << endl;
  ip++;

  ost << "EFILR"
      << " FiltResEnv "
      << " 0 " << kval.asInt("num_envs") << " 0 3" << endl;
  ip++;

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "VS" << iop+1 
        << " Op" << iop+1 << "VelSens "
        << " 0 " << " 1 " << " 2 14" << endl;
    ip++;
  }

  ost << "VS" << "N"
      << " Noise" << "VelSens "
      << " 0 " << " 1 " << " 2 4" << endl;
  ip++;

  ost << "VS" << "C"
      << " FiltCut" << "VelSens "
      << " 0 " << " 1 " << " 2 3" << endl;
  ip++;

  for(int ie=0;ie<kval.asInt("num_envs");++ie){
    ost << "ELOOP" << ie+1 
        << " En" << ie+1 << "LoopLen "
        << " 0 " << kval.asInt("num_knees")-2 << " 0 2" << endl;
    ip++;
  }

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "FMTO" << iop+1 
        << " FModTo" << iop+1 
        << " 0 " << iop << " 0 3" << endl;
    ip++;
  }

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "ADDTO" << iop+1 
        << " MixTo" << iop+1 
        << " 0 " << iop << " 0 4" << endl;
    ip++;
  }

  ost << "FILT" 
      << " FilterLowBndHiNtch"
      << " 0 " << " 3 " << " 0 16" << endl;
  ip++;

  ost << "FFOLL" 
      << " FilterFollowOp"
      << " 0 " << kval.asInt("num_ops")-1
      << " 0 16" << endl;
  ip++;

  if (ip%2==1) {
    ost << "xxx" 
        << " UNUSED0"
        << " 0 " << 1
        << " 0 18" << endl;
    ip++;
  }


  ost << "[F]" << endl;

  ost << "NULLTGT NOT_Editable 0 0 0 1" << endl;
  ost << "MIXLEV  MixLev       0.0 1.0 2 19" << endl;
  ost << "MIXPAN  MixPan      -1.0 1.0 2 19" << endl;

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "LV" << iop+1
        << " Op" << iop+1 << "Lev "
        << "-8.0 8.0 " << " 2 15" << endl;
  }
  ost << "LV" << "N"
      << " NoiseLev "
      << "-8.0 8.0 " << " 2 15" << endl;
  ost << "LV" << "D"
      << " DlyInLev "
      << "-8.0 8.0 " << " 2 15" << endl;

  /* FIXME: Really need to separate the GUI preference part .. */
  for(int ie=0;ie<kval.asInt("num_envs");++ie){
    for(int ik=0;ik<kval.asInt("num_knees");++ik){
      ost << "ENV" << ie+1 << "K" << ik+1 << "T"
          << " En" << ie+1 << ":" << ik+1 << "T"
          << " 0 " << 2.00 << " 0 " << 6+ie<< endl;
      ost << "ENV" << ie+1 << "K" << ik+1 << "L"
          << " En" << ie+1 << ":" << ik+1 << "L"
          << " 0 " << 2.00 << " 0 " << 6+ie<< endl;

    }
  }

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "DT" << iop+1
        << " Op" << iop+1 << "Crs "
        << "-48.0 48.0 " << " 1 12" << endl;
  }

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "DT" << iop+1 << "F"
        << " Op" << iop+1 << "Fine "
        << "-.5 .5 " << " 3 13" << endl;
  }

  for(int iop=0;iop<kval.asInt("num_ops");++iop){
    ost << "PBAM" << iop+1 
        << " Op" << iop+1 << "BendAmt "
        << "0.0 24.0" << " 1 18" << endl;
  }

  ost << "PBVAL NOT_Editable -1.0 1.0 2 18" << endl;
  ost << "FFREQ Cutoff 0.0 128.0 1 17" << endl;
  ost << "FRESO Resonance 0.0 1.0 2 17" << endl;

  for(int id=0;id<kval.asInt("num_delays");++id){
    ost << "DINLV" << id+1 
        << " Dly" << id+1 << "InLev "
        << "-1.0 1.0" << " 2 16" << endl;
  }

  for(int id=0;id<kval.asInt("num_delays");++id){
    ost << "DLEN" << id+1 
        << " Dly" << id+1 << "Length "
        << "0 2000" << " 1 17" << endl;
  }

  for(int id=0;id<kval.asInt("num_delays");++id){
    ost << "DLEV" << id+1 
        << " Dly" << id+1 << "OutLev "
        << "0 1.0" << " 1 18" << endl;
  }

  ost << "LEGLEN LegatoLen 0.0 1.0 2 17" << endl;
  ost << "PSCALE PitchScale -1.0 2.0 2 18" << endl;

  /* FIXME: This needs a re-vamp.. but everything does..: */
  for(int im=0;im<kval.asInt("num_mods");++im){
    ost << "CDST" << im+1 
        << " Mod" << im+1 << "Dest "
        << "0 127" << " 0 20" << endl;
  }

}

