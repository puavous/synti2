#include "keyvalhelper.hpp"

#include <map>
#include <iostream>
#include <string>
#include <sstream>


// copy-pasted helpers..
static
bool line_is_whitespace(std::string &str){
  return (str.find_first_not_of(" \t\n\r") == str.npos);
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

static
std::string line_trim(std::string &str){
  unsigned long beg = str.find_first_not_of(" \t\n\r");
  unsigned long end = str.find_last_not_of(" \t\n\r", beg);
  return str.substr(beg,end+1);
}

static
int 
stoi(const std::string &str){
  std::stringstream ss(str);
  int res = 0;
  ss >> res;
  return res;
}

void KeyValueStorage::readFromStream(std::istream &ist){
  std::string line,k,v;
  while(std::getline(ist, line)){
    if (line_is_whitespace(line)) continue;
    if (line[0]=='#') continue;
    if (line.substr(0,3) == "---") break;
    k = line_chop(line);
    v = line_trim(line);
    m[k] = v;
  }
}

void 
KeyValueStorage::writeToStream(std::ostream &ost) const
{
  std::map<std::string,std::string>::const_iterator it;

  for (it=m.begin(); it!=m.end(); ++it){
    ost << it->first << " " << it->second << std::endl;
  }
  ost << "--- end of capdef" << std::endl;
}

KeyValueStorage::KeyValueStorage(std::istream &ist){
  readFromStream(ist);
}

int KeyValueStorage::asInt(const std::string &key) const{
  return stoi(m.at(key));
}

std::string
KeyValueStorage::asString(const std::string &key) const{
  return m.at(key);
}


std::ostream& operator<< (std::ostream& os, 
                          const KeyValueStorage& kvs)
{ 
  kvs.writeToStream(os);
  return os;
}
