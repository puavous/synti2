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

int main(int argc, char **argv){
  if (argc < 2){
    usage(std::cerr);
    exit(1);
  }

  if (strcmp(argv[1],"patchdesign") == 0) {
    std::cerr << "FIXME: To be implemented: patchdesign "
              << std::endl;
  } else if (strcmp(argv[1],"capheader") == 0){
    std::cerr << "FIXME: To be implemented: capheader "
              << std::endl;
  } else if (strcmp(argv[1],"parheader") == 0){
    std::cerr << "FIXME: To be implemented: parheader " 
              << std::endl;
  } else if (strcmp(argv[1],"patchdata") == 0){
    std::cerr << "FIXME: To be implemented: patchdata " 
              << std::endl;
  } else if (strcmp(argv[1],"songdata") == 0){
    std::cerr << "FIXME: To be implemented: songdata " 
              << std::endl;
  } else {
    std::cerr << "Unknown command: " << argv[1] << std::endl;
    usage(std::cerr);
    exit(1);
  }
}
