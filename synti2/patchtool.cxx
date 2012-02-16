#include <iostream>
#include <fstream>
#include <vector>

#include "patchtool.hpp"

/** Implementation of patchtool and stuff. */
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
  int beg = str.find_first_not_of(" \t\n\r");
  int wbeg = str.find_first_of(" \t\n\r", beg);
  if (wbeg == str.npos) wbeg = str.length();
  std::string res = str.substr(beg,wbeg-beg);
  if (wbeg<str.length()) str.assign(str.substr(wbeg,str.length()-wbeg));
  return std::string(res);
}

/** Loads the patch format with information */
void 
synti2::Patchtool::load_patch_data(const char *fname){
  std::ifstream ifs(fname);
  std::string line;
  std::string curr_section;
  int curr_sectnum = -1;
  std::string pname, pdescr;
  while(std::getline(ifs, line)){
    if (line_is_whitespace(line)) continue;
    if (line[0]=='#') continue;
    if (line[0]=='['){
      /* Begin section */
      line_to_header(line);
      curr_section = line;
      std::cout << "**** New header: ";
      std::cout << line << std::endl;
      sectlist.push_back(line);
      sectsize.push_back(0);
      curr_sectnum++;
      continue;
    };
    /* Else it is a parameter value. */
    pname = line_chop(line);
    pdescr = line_chop(line);
    std::cout << "/*"<< pdescr << "*/" << std::endl;
    std::cout << "#define SYNTI2_" << curr_section 
              << "_" << pname 
              << " " << sectsize[curr_sectnum] << std::endl;
    sectsize[curr_sectnum]++;
  }
  for(int i=0; i<sectlist.size(); i++){
    curr_section = sectlist[i];
    std::cout << "#define SYNTI2_" << curr_section 
              << "_NPARS "<< sectsize[i] << std::endl;
  }
}


synti2::Patchtool::Patchtool(std::string fname){
  std::cout << "Reading from file " << fname << std::endl;
  load_patch_data("patchdesign.dat");
}


int
synti2::Patchtool::nPars(std::string type){
  return -1; /*Not yet implemented. Need to refactor the whole class first..*/
}
