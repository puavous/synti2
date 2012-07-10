/** Implementation of patchtool and stuff. */
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

#include "patchtool.hpp"


/* A hack (?) to have localized code that is useable as a static
   function inside the core synth. Maybe there is a better way for
   this... need to think.. later.*/
#include "synti2_fdec.c"
#include "synti2_fenc.c"


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

/* Actual methods */

/* Class methods */
/** Decode a "floating point" parameter. */
float synti2::decode_f(const unsigned char *buf){
  return synti2_decode_f(buf);
}

/** Encode a "floating point" parameter into 7 bit parts. */
float synti2::encode_f(float val, unsigned char * buf){
  return synti2_encode_f(val, buf);
}


/* Instance methods. */

synti2::PatchDescr::PatchDescr(std::istream &inputs){
  load_patch_data(inputs);
}

/** Loads the patch format with information */
void 
synti2::PatchDescr::load_patch_data(std::istream &ifs){
  std::string line, curr_section;
  std::string pname, pdescr;
  //  int ind=0;
  while(std::getline(ifs, line)){
    if (line_is_whitespace(line)) continue;
    if (line[0]=='#') continue;
    if (line[0]=='['){
      /* Begin section */
      line_to_header(line);
      curr_section = line;
      params[line]; /* Makes a new list. */
      //      ind = params[line].size(); /* Indices are into sections. */
      continue;
    };
    /* Else it is a parameter description. */
    ParamDescr pardsc(line, curr_section);
    parind[curr_section][pardsc.getName()] = params[curr_section].size();
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
  precision = line_chop(line);
  group = line_chop(line);
}

synti2::Patchtool::Patchtool(std::string fname){
  std::ifstream ifs(fname.c_str());  
  patch_description = new PatchDescr(ifs);
}

void
synti2::PatchDescr::headerFileForC(std::ostream &os){
  std::map<std::string, std::vector<ParamDescr> >::iterator it;
  os << "/** Parameter indices as C #defines. " 
     << " *  These have been automatically generated from the patch design"
     << std::endl
     << " *  specification file that includes more documentation."
     << " */" << std::endl;
  os << "#ifndef SYNTI2_PARAMETERS_H" << std::endl;
  os << "#define SYNTI2_PARAMETERS_H" << std::endl;

  for (it = params.begin(); it != params.end(); it++){
    std::string sect = (*it).first;
    std::vector<ParamDescr> &v = (*it).second;

    os << std::endl << std::endl 
       << "/* New section starts: */" << std::endl 
       << "#define SYNTI2_" << sect
       << "_NPARS "<< v.size() << std::endl;

    for (unsigned int i=0; i < v.size(); i++){
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
  os << getName() << std::endl;
  os << "# Patch data for '" << getName() << "'begins" << std::endl;
  std::map<std::string, std::vector<ParamDescr> >::iterator it;
  for (it = pd->paramBeg(); it != pd->paramEnd(); it++){
    std::string sect = (*it).first;
    os << "[" << sect  << "]" << std::endl;
    std::vector<ParamDescr> &v = (*it).second;
    for (unsigned int i=0; i < v.size(); i++){
      os << v[i].getName();
      os << " " << values[sect][i]  << std::endl;
    }
  }
  os << "--- end of patch " << std::endl;
}

void 
synti2::Patch::read(std::istream &ifs){
  /* This patch already has a description. Only read stuff that is
     compatible with current description, and spit information about
     incompatible stuff to stderr. */

  std::string line, curr_section;
  std::string pname, pdescr;
  if(std::getline(ifs, line)){
    setName(line);
  } else {
    std::cerr << "Warning: No patch to read (end of file)" << std::endl;
    return;
  }
  while(std::getline(ifs, line)){
    if (line_is_whitespace(line)) continue;
    if (line[0]=='#') continue;
    if (line[0]=='-') break;
    if (line[0]=='['){
      /* Begin section */
      line_to_header(line);
      curr_section = line;
      continue;
    };

    /* Else it is a parameter value. */
    if (values.count(curr_section) == 0) {
      std::cerr << "Section doesn't exist in current patch format: "
                << curr_section
                << std::endl;
      std::cerr << "Skipping parameter: "
                << curr_section << "_" << line
                << std::endl;
      continue;
    }

    std::string parname = line_chop(line);
    std::string parvals = line_chop(line);
    float val = std::strtod(parvals.c_str(), 0);
    int paraddr = pd->getValueID(curr_section, parname);

    if (paraddr < 0) {
      std::cerr << "Skipping! Not a "
                << curr_section << "-parameter name in current format: " 
                << parname << std::endl;
      continue;
    }
    setValue(curr_section, paraddr, val);
  }

  
}


void 
synti2::Patch::setValue(std::string type, int idx, float value){
  (values[type])[idx] = value;
}

void
synti2::Patch::clear(){
  std::map<std::string, std::vector<ParamDescr> >::iterator it;
  for (it = pd->paramBeg(); it != pd->paramEnd(); it++){
    std::string sect = (*it).first;
    std::vector<ParamDescr> &v = (*it).second;
    for (unsigned int i=0; i < v.size(); i++){
      values[sect][i] = 0.0;
    }
  }
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
    for (unsigned int i=0; i < v.size(); i++) values[sect].push_back(0.0f);
  }
}

/** 
 * Exports the contents of this patch in the sysex binary format that
 * the synti2 engine expects. As of 2012-02-25, the format is I3, I7,
 * and F values, in this order, with their respective encoding.
 */
void
synti2::Patch::exportBytes(std::vector<unsigned char> &bvec){
  /* We seem to know an awful lot about the contents of
   * patchdesign.dat in here... TODO: Can I help it? Well, I
   * could.. but it's not the biggest concern here, so let's hardcode
   * this..
   */
  /*I3 goes by pairing the 3-bit snippets together: */
  for (int i=0; i<getNPars("I3"); i += 2){
    int v1 = getValue("I3",i);
    int v2 = getValue("I3",i+1);
    unsigned char byt = ((v1 & 0x7) << 3) + (v2 & 0x7); 
    bvec.push_back(byt);
  }
  for (int i=0; i<getNPars("I7"); i ++){
    bvec.push_back(getValue("I7",i));
  }

  unsigned char buf[2] = {0,0};
  for (int i=0; i<getNPars("F"); i++){
    synti2::encode_f(getValue("F", i), buf);
    bvec.push_back(buf[0]);
    bvec.push_back(buf[1]);
  }
}


void
synti2::PatchBank::exportStandalone(std::ostream &os){
  os << "/* This is automatically generated by PatchBank. */" << std::endl;
  os << "/* A sound patch bank for synti2. */" << std::endl;

  const unsigned char syx_header[] = {0xf0, 0x00, 0x00, 0x00, /*"manuf. ID"*/
                                      0x00, 0x00, /*opcode 0 == load all*/
                                      0x00, 0x00  /*offset 0 == from 1st.*/};

  std::vector<unsigned char> bytes;
  for (unsigned int i=0; i<sizeof(syx_header); i++){
    bytes.push_back(syx_header[i]);
  }

  for (unsigned int i=0; i<size(); i++){
    at(i).exportBytes(bytes);
  }

  bytes.push_back(0xf7);

  os << "unsigned char patch_sysex[] = {" << std::endl << "    ";

  for (unsigned int i=0; i<bytes.size(); i++){
    os << int(bytes[i]) << ", ";
    if (((i+1)%16) == 0){
      os << std::endl << "    ";
    }
  }
  os << "};" << std::endl;

  std::cerr << "Wrote it?" << std::endl;

}

