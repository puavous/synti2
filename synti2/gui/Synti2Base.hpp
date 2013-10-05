#ifndef SYNTI2_BASE
#define SYNTI2_BASE

#include <string>
#include <map>
#include <vector>
using std::string;
using std::map;
using std::vector;


namespace synti2base {
  class Description {
  protected:
    string key;
    string cdefine;
    string humanReadable;
  public:
    Description() : key("none"),cdefine("none"),humanReadable("none"){};
    Description(string ikey, string icdefine, string ihumanReadable)
      :key(ikey),cdefine(icdefine),humanReadable(ihumanReadable){};
    string getKey() const {return key;}
    string getCDefine() const {return cdefine;}
    string getHumanReadable() const {return humanReadable;}
  };

  class FeatureDescription : public Description{
  private:
    std::vector<std::string> reqkeys;
  public:
    FeatureDescription(string ik, string icd, string ihr, string ireq)
      : Description(ik,icd,ihr)
    {
      // should split/tokenize, if many will be needed.
      reqkeys.push_back(ireq);
    }
    bool doesRequire(std::string rkey)
    {
      return false;//"reqkeys.contains(rkey);"}
    }
  };

  class CapacityDescription : public Description{
  private:
    int min;
    int max;
    string reqf;
  public:
    CapacityDescription(string ik, string icd, string ihr, int imin, int imax, string ireqf)
      : Description(ik,icd,ihr), min(imin), max(imax), reqf(ireqf)
    {}
  };

}
#endif
