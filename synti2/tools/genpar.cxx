/** @file genpar.cxx Mini tool to generate a C header file. 
 *
 *  This was used for making the patch design phase a bit easier. Not
 *  very useful after things have stabilized (?). 
 */
#include<string>
#include<iostream>
#include<fstream>

#include "patchtool.hpp"

int main(int argc, char **argv){
  if (argc<3){
    std::cout << "This program reads a plaintext patch spec, "
              << "and outputs a C header file. " 
              << std::endl << std::endl;
    std::cout << "Usage: " << std::endl;
    std::cout << argv[0] << " infile outfile" << std::endl;
    return 1;
  }

  std::ifstream istr(argv[1]);
  std::ofstream ou(argv[2]);

  synti2::PatchDescr pd(istr);
  pd.headerFileForC(ou);

  return 0;
}
