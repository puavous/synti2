/** @file cbank.cxx Mini tool to convert a sound bank to C code. 
 *
 */
#include<string>
#include<iostream>
#include<fstream>

#include "patchtool.hpp"

int main(int argc, char **argv){
  if (argc<3){
    std::cout << "This program converts a synti2 sound bank to a C code, "
              << "usable in a stand-alone synth build." 
              << std::endl << std::endl;
    std::cout << "Usage: " << std::endl;
    std::cout << argv[0] << " infile outfile" << std::endl;
    return 1;
  }

  synti2::Patchtool *pt = new synti2::Patchtool("src/patchdesign.dat");
  synti2::PatchBank *pbank = pt->makePatchBank(16);

  std::ifstream istr(argv[1]);
  std::ofstream ou(argv[2]);
  pbank->read(istr);
  pbank->exportStandalone(ou);

  return 0;
}
