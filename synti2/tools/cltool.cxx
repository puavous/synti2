/** Command line tool for configuring synti2 for different kinds of
 *  builds. This functionality used to be in many files, but now it is
 *  centralized to this one tool. One action may be asked at a time,
 *  and the output will be in standard output.
 */

#include<string>
#include<cstring>
#include<iostream>
#include<fstream>

/* Includes for our own application logic*/
#include "captool.hpp"
#include "patchtool.hpp"
#include "miditool.hpp"
#include "midimaptool.hpp"
#include "misss.hpp"


static
bool file_exists(const char* fname){
  std::ifstream checkf(fname);
  return checkf.is_open();
}

static
bool check_file_exists(const char* fname){
  if (!file_exists(fname)){
    std::cerr << "File not found: " << fname << std::endl;
    return false;
  } else return true;
}

/** Input: s2bank with "Capacity definitions". Output: C header file
 *  for synth capacity configuration.
 */
static
void
generateCapHeader(std::istream &s2bank, std::ostream &ou){
  std::stringstream hack("num_voices  16    \n");
  //synti2::Capacities cap(s2bank);
  synti2::Capacities cap(hack);
  cap.writeCapH(ou);
}

static
void
generatePatchDesign(std::istream &s2bank, std::ostream &ou){
  std::stringstream hack("num_voices 11\n num_envs 5 \n num_ops 3\n");
  //synti2::Capacities cap(s2bank);
  synti2::Capacities cap(hack);
  cap.writePatchDesign(ou);
}


/** "Patchdesign" as input; C header file as output. */
void
generateParamHeader(std::istream &istr, std::ostream &ou){
  synti2::PatchDescr pd(istr);
  pd.headerFileForC(ou);
}


/** */
void
generatePatchBank(std::istream &s2bank, std::istream &pdgn, std::ostream &ou, int nvoices){
  synti2::Patchtool *pt = new synti2::Patchtool(pdgn);
  synti2::PatchBank *pbank = pt->makePatchBank(nvoices);
  pbank->read(s2bank);
  pbank->exportStandalone(ou);
}

void
generateMisssSong(const char *smf_fname, const char *s2bank_fname, std::ostream &os, int tpq){
  if (!check_file_exists(smf_fname)) exit(1);
  if (!check_file_exists(s2bank_fname)) exit(1);

  std::ifstream ifs(smf_fname, std::ios::in|std::ios::binary);
  MidiSong ms(ifs);
  ifs.close();

  synti2::MidiMap mapper;

  std::ifstream mapperfs(s2bank_fname);
  mapper.read(mapperfs);
  mapperfs.close();

  /* Spec could come from "exe builder GUI"? Could be part of mapper?
     No?*/
  std::stringstream spec("hm. Should be all like TPQ=24;");
  ms.decimateTime(tpq);

  synti2::MisssSong misss(ms, mapper, spec);

  misss.write_as_c(os);
}


void
usage(std::ostream &os){
  os << "Usage: Instructions to be written... "
     << std::endl;
}

void
die(std::string msg){
  std::cerr << msg << std::endl;
  exit(1);
}

int main(int argc, char **argv){
  if (argc < 2){
    usage(std::cerr);
    exit(1);
  }

  if (strcmp(argv[1],"patchdesign") == 0) {
    std::cerr << "FIXME: To be properly implemented: patchdesign "
              << std::endl;
    if (argc < 2) die("Too few arguments");
    std::ifstream s2bank(argv[2]);
    generatePatchDesign(s2bank, std::cout);
  } else if (strcmp(argv[1],"capheader") == 0){
    std::cerr << "FIXME: To be properly implemented: capheader "
              << std::endl;
    if (argc < 2) die("Too few arguments");
    std::ifstream s2bank(argv[2]);
    generateCapHeader(s2bank, std::cout);

  } else if (strcmp(argv[1],"parheader") == 0){
    std::cerr << "FIXME: To be implemented properly: parheader " 
              << std::endl;
    if (argc < 2) die("Too few arguments");
    std::ifstream pdes(argv[2]);
    generateParamHeader(pdes, std::cout);

  } else if (strcmp(argv[1],"patchdata") == 0){
    std::cerr << "FIXME: To be properly implemented: patchdata " 
              << std::endl;
    if (argc < 3) die("Too few arguments");

    std::ifstream s2bank(argv[2]);
    std::ifstream pdes(argv[3]);
    generatePatchBank(s2bank, pdes, std::cout, 16);

  } else if (strcmp(argv[1],"songdata") == 0){
    std::cerr << "FIXME: To be implemented: songdata " 
              << std::endl;
    if (argc < 3) die("Too few arguments");
    //std::ifstream smf(argv[2]);
    //std::ifstream s2bank(argv[3]);
    generateMisssSong(argv[2], argv[3], std::cout, 4);

  } else {
    std::cerr << "Unknown command: " << argv[1] << std::endl;
    usage(std::cerr);
    exit(1);
  }
}
