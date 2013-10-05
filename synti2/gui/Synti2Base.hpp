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
  private:
    string key;
    string cdefine;
    string humanReadable;
  public:
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

  class I4ParDescription : public Description {
  private:
    /** Default value */
    int defval; 
    /** Allowed values [0,15] as a 16-bit pattern */
    int allowed;
    /** Names to be used in GUI; empty->"0","1","2",.."15" */
    std::vector<std::string> names;
    //I4ParDescription(){};/*must give all data on creation*/
  public:
    I4ParDescription(string ik, string icd, string ihr,
                     int idef, int iallow,
                     std::vector<std::string> inames)
      : Description(ik,icd,ihr), defval(idef), allowed(iallow), names(inames)
    {
    }
  };

  class FParDescription : public Description {
  private:
    /** Default value */
    float defval;
    /** Minimum value (mutable) */
    float minval;
    /** Maximum value (mutable) */
    float maxval;
    /** Preferred granularity in nudged changes (mutable) */
    float step;
    //FParDescription(){};/*must give all data on creation*/
  public:
    FParDescription(string ik, string icd, string ihr,
                    float idef, float imin, float imax, float istep)
      : Description(ik,icd,ihr), defval(idef), minval(imin), maxval(imax),
        step(istep)
    {
    }
  };
}
#endif
