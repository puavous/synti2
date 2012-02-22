/** Implementation of patchtool and stuff. */
#include <iostream>
#include <fstream>
#include <vector>

#include "patchtool.hpp"


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
  int beg = str.find_first_not_of(" \t\n\r");
  int wbeg = str.find_first_of(" \t\n\r", beg);
  if (wbeg == str.npos) wbeg = str.length();
  std::string res = str.substr(beg,wbeg-beg);
  if (wbeg<str.length()) str.assign(str.substr(wbeg,str.length()-wbeg));
  return std::string(res);
}

/* Actual methods */

synti2::PatchDescr::PatchDescr(std::istream &inputs){
  load_patch_data(inputs);
}

/** Loads the patch format with information */
void 
synti2::PatchDescr::load_patch_data(std::istream &ifs){
  std::string line, curr_section;
  int curr_sectnum = -1;
  std::string pname, pdescr;
  while(std::getline(ifs, line)){
    if (line_is_whitespace(line)) continue;
    if (line[0]=='#') continue;
    if (line[0]=='['){
      /* Begin section */
      line_to_header(line);
      curr_section = line;
      params[line]; /* Makes a new list. */
      curr_sectnum++;
      continue;
    };
    /* Else it is a parameter description. */
    ParamDescr pardsc(line, curr_section);
    params[curr_section].push_back(pardsc); /* add to list */
  }
}

int
synti2::PatchDescr::nPars(std::string type) {
  std::vector<ParamDescr> &v = params[type];
  return v.size();
}

std::string 
synti2::PatchDescr::getDescription(std::string type, int idx)
{
  std::vector<ParamDescr> &v = params[type];
  return v[idx].getDescription();
}

synti2::ParamDescr::ParamDescr(std::string line, std::string sect){
  type = sect;
  name = line_chop(line);
  description = line_chop(line);
  min = line_chop(line);
  max = line_chop(line);
}

synti2::Patchtool::Patchtool(std::string fname){
  std::ifstream ifs(fname.c_str());  
  patch_description = new PatchDescr(ifs);
  /* Output for debug purposes only: */
  patch_description->headerFileForC(std::cout);
}

void
synti2::PatchDescr::headerFileForC(std::ostream &os){
  std::map<std::string, std::vector<ParamDescr> >::iterator it;
  os << "/** Parameter indices as C #defines */" << std::endl;
  os << "#ifndef SYNTI2_PARAMETERS_H" << std::endl;
  os << "#define SYNTI2_PARAMETERS_H" << std::endl;

  for (it = params.begin(); it != params.end(); it++){
    std::string sect = (*it).first;
    std::vector<ParamDescr> &v = (*it).second;

    os << std::endl << std::endl 
       << "/* New section starts: */" << std::endl 
       << "#define SYNTI2_" << sect
       << "_NPARS "<< v.size() << std::endl;

    for (int i=0; i < v.size(); i++){
      os << "/*"<< v[i].getDescription() << "*/" << std::endl;
      os << "#define SYNTI2_" << sect << "_" << v[i].getName()
         << " " << i << std::endl << std::endl;
    }
  }

  os << "#endif" << std::endl;
}

/* Methods of Patch */

synti2::Patch::Patch(synti2::PatchDescr *ipd){
  pd = ipd;
  copy_structure_from_descr();
}

void 
synti2::Patch::write(std::ostream &os){
  os << "# Patch data begins" << std::endl;
  std::map<std::string, std::vector<ParamDescr> >::iterator it;
  for (it = pd->paramBeg(); it != pd->paramEnd(); it++){
    std::string sect = (*it).first;
    os << "[" << sect  << "]" << std::endl;
    std::vector<ParamDescr> &v = (*it).second;
    for (int i=0; i < v.size(); i++){
      os << v[i].getName();
      os << " " << values[sect][i]  << std::endl;
    }
  }
  os << "--- end of patch " << std::endl;
}

void 
synti2::Patch::read(std::istream &is){
  std::cerr << "FIXME: Reading to be implemented" << std::endl;
}


void 
synti2::Patch::setValue(std::string type, int idx, float value){
  (values[type])[idx] = value;
}


/* internal: initializes a patch, matching its description, with
 * zero-values.
 */
void 
synti2::Patch::copy_structure_from_descr(){
  std::map<std::string, std::vector<ParamDescr> >::iterator it;
  for (it = pd->paramBeg(); it != pd->paramEnd(); it++){
    std::string sect = (*it).first;
    std::vector<ParamDescr> &v = (*it).second;
    values[sect]; /* create section and fill with zeros: */
    for (int i=0; i < v.size(); i++) values[sect].push_back(0.0f);
  }
}

