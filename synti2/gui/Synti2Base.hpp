#ifndef SYNTI2_BASE
#define SYNTI2_BASE

#include <string>
#include <map>
#include <vector>

#include <iostream>
using std::string;
using std::map;
using std::vector;


namespace synti2base {

#if 0
  /** Derive this to update GUI when a feature is turned on/off. */
  class FeatCallback {
  public:
    virtual void featureStateWasSet(const std::string &key,bool enabled)
    {
      std::cerr << "Using underived FeatCallback." << std::endl;
    };
  };

  /** Derive this to update GUI when a capacity is changed. */
  class CapacityCallback {
  public:
    virtual void capacityWasSet(const std::string &key,int state)
    {
      std::cerr << "Using underived CapacityCallback." << std::endl;
    };
  };
#endif

  /** Derive this to do something when rules are either met or not. */
  class RuleAction {
  public:
      virtual void action (bool ruleOutcome) const {
          std::cerr << "Using underived RuleAction. " << ruleOutcome << std::endl;
      }
  };

  /** Simple integer threshold rules can be stored here. */
  class RuleSet {
  private:
      std::vector<std::string> keys;
      std::vector<int> thresholds;
      RuleAction *ra_ptr;
  public:
      RuleSet(): ra_ptr(NULL){};
      void ownThisAction(RuleAction *p){ra_ptr = p;}
      ~RuleSet() {
          if (ra_ptr != NULL) {
            /*delete ra_ptr;*/
            /* FIXME: Memory leak. (I suck and need my Java..)
               No.. just need some reference counting. TODO.
            */
            std::cerr << "should be counting refs here... " << std::endl;
            }
      }
      void addThreshold(std::string key, int threshold){
          keys.push_back(key);
          thresholds.push_back(threshold);
      }
      std::vector<std::string> const & getKeys() const {return keys;}
      std::vector<int> const & getThresholds() const {return thresholds;}
      RuleAction const * getRuleAction() const {return ra_ptr;}
  };

#if 0
  /** Derive this to update GUI when a capacity is changed. */
  class CapacityCallback {
  public:
    virtual void changedTo(int val)
    {
        std::cerr << "Using underived CapacityCallback. " << val << std::endl;
    };
  };
#endif

/*
  class RuleCapacityCallback : public CapacityCallback {
  private:
      RuleAction action;
      std::vector<RuleChecker> checks;
  public:
      virtual void changedTo(int val){
      bool status = true;
      std::vector<RuleChecker> checks;
      std::vector<RuleChecker>::const_iterator c;
      for(c=checks.begin();c<checks.end();++c){
          status &= *c.check(val);
      }
      ra.action(status);
      RuleCapacityCallback(RuleAction a): CapacityCallback(), action(a) {};
      void addRuleChecker(RuleChecker){checks.push_back(c);}ĶĶ
  };
*/

  class Description {
  protected:
    string key;
    string cdefine;
    string humanReadable;
    string guiStyle;
    string ruleString;
    //std::vector<CapacityCallback> ccbs;
  public:
    Description() : key("none"),cdefine("none"),humanReadable("none"),
                    guiStyle("void"),ruleString(";"){};
    Description(string ikey, string icdefine, string ihumanReadable,
                string iguiStyle, string irules)
      :key(ikey),cdefine(icdefine),humanReadable(ihumanReadable),
       guiStyle(iguiStyle),ruleString(irules){};
    string getKey() const {return key;}
    string getCDefine() const {return cdefine;}
    string getHumanReadable() const {return humanReadable;}
    string getGuiStyle() const {return guiStyle;}
    string getRuleString() const {return ruleString;}
    /* Thresholds (key, min value) seem enough for current purposes. */
    RuleSet const getRuleSet() const {
        RuleSet r;
        r.addThreshold("add",0); // FIXME: Parse the rulestring here.
        return r;
    }
  };

  class FeatureDescription : public Description{
  private:
    std::vector<std::string> reqkeys;
  public:
    FeatureDescription(string ik, string icd, string ihr, string ireq)
      : Description(ik,icd,ihr,"void",";")
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
      : Description(ik,icd,ihr,"void",";"), min(imin), max(imax), reqf(ireqf)
    {}
    int getMin() const {return min;}
    int getMax() const {return max;}
  };

}
#endif
