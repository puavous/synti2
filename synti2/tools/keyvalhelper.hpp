/** yet another crappy ini-file kinda thingy */
#ifndef KEYVALHELPER_HPP
#define KEYVALHELPER_HPP

#include <iostream>
#include <map>

class KeyValueStorage {
protected:
  std::map<std::string, std::string> m;
public:
  KeyValueStorage(){};
  KeyValueStorage(std::istream &ist);
  void readFromStream(std::istream &ist);
  void writeToStream(std::ostream &ost) const;
  int asInt(const std::string &key) const;
  std::string asString(const std::string &key) const;

  //friend std::ostream& operator<<(std::ostream& os, const KeyValueStorage& kvs);
};

std::ostream& operator<< (std::ostream& os, 
                          const KeyValueStorage& kvs);

#endif
