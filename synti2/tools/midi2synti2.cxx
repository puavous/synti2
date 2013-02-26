/** @file midi2synti2.cxx
 *
 * A converter program that filters and transforms a midi song (from
 * SMF0 and SMF1 sequence files) to a sequence that synti2 can load in
 * the tiny/4k mode. A non-graphical command-line mode is available
 * for the SMF conversion stage. (FIXME: Not all is implemented as of
 * yet; need a proper setup format)
 *
 * Kludges and awkwardness are likely to persist.
 *
 */

/* Includes for our own application logic*/
#include "miditool.hpp"

/* Standard includes required by this unit */
#include <iostream>
#include <fstream>
#include <sstream>

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

int cmd_line_only(int argc, char **argv){
  if (argc<3) {
    std::cerr << "Usage:" << std::endl;
    std::cerr << "  " << argv[0] << " SMF_filename s2bank_filename " << std::endl;
    return 1;
  }
  if (!check_file_exists(argv[1])) return 1;
  if (!check_file_exists(argv[2])) return 1;

  std::ifstream ifs(argv[1], std::ios::in|std::ios::binary);
  MidiSong ms(ifs);
  ifs.close();

  /* FIXME: This needs to be made into a parameter very soon: */
  /*ms.decimateTime(24);*/

  MidiEventTranslator tr;

  std::stringstream spec("hm. Should be all like TimeDecim=24;");
  MisssSong misss(ms, tr, spec);

  misss.write_as_c(std::cout);

  return 0;
}

int main(int argc, char **argv){

  if (argc > 1){
    /* Command line arguments are given, so run without user
     * interface. TODO: Better handling of arguments! This is barely
     * usable as of now.
     */
    return cmd_line_only(argc, argv);
  }
}
