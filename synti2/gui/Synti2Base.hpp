#ifndef SYNTI2_BASE
#define SYNTI2_BASE

#include <string>
#include <map>
#include <vector>

#include <iostream>
#include <sstream>
using std::stringstream;
using std::string;
using std::map;
using std::vector;


namespace synti2base {

  /** Derive this to give a concrete Midi API for sending data. */
  class MidiSender {
  protected:
    /** Override this get bytes moving towards a midi port */
    virtual void doSendBytes(vector<unsigned char> const & bytes) = 0;
    /** Override this to tell if the midi port is ready to send */
    virtual size_t inspectSendCapacity() = 0;
  public:
    /** Send bytes, if all goes well. Success is not guaranteed. Check
     *  queue capacity first with isReadyToSend(). And watch out for
     *  broken physical MIDI cables.
     */
    void send(vector<unsigned char> const & bytes){doSendBytes(bytes);}

    /** See if there is room in the communication pipe */
    bool isReadyToSend(vector<unsigned char> const & bytes){
        return inspectSendCapacity() > (bytes.size() + sizeof(size_t));
    }
  };

  /** Derive this to do something when rules are either met or not. */
  class RuleAction {
  public:
      virtual void action (bool ruleOutcome) //const = 0;
      const {
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
               No.. just need some reference counting here. TODO.
               Only object so far that needs it.. so could count itself(?):
               ra_ptr->nref--;
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

  class Description {
  protected:
    string key;
    string cdefine;
    string humanReadable;
    int guiGroup;
    string guiStyle;
    string ruleString;
  public:
    Description() : key("none"),cdefine("none"),humanReadable("none"),
                    guiStyle("void"),ruleString(";"){};
    Description(string ikey, string icdefine, string ihumanReadable,
                int iguiGroup, string iguiStyle, string irules)
      :key(ikey),cdefine(icdefine),humanReadable(ihumanReadable),
       guiGroup(iguiGroup),guiStyle(iguiStyle),ruleString(irules){};
    string getKey() const {return key;}
    string getCDefine() const {return cdefine;}
    string getHumanReadable() const {return humanReadable;}
    int getGuiGroup() const {return guiGroup;}
    string getGuiStyle() const {return guiStyle;}
    string getRuleString() const {return ruleString;}
    /* Thresholds (key, min value) seem enough for current purposes. */
    RuleSet const getRuleSet() const {
        RuleSet r;
        string rsts = ruleString;
        while(!rsts.empty()){
            size_t isep = rsts.find_first_of(";");
            if (isep==0){rsts.erase(0,1);continue;}
            size_t iop  = rsts.find_first_of(">");
            string key  = rsts.substr(0,iop);
            string sval = rsts.substr(iop+1,isep-iop);
            if (isep==string::npos) isep=rsts.length();
            rsts.erase(0,isep+1);
            int intval;
            stringstream(sval) >> intval;
            std::cerr<<"'" << key << "'>" << intval << "|" << rsts << "|" <<std::endl;
            r.addThreshold(key,intval);
        }
        return r;
    }
  };

  class FeatureDescription : public Description{
  private:
    std::vector<std::string> reqkeys;
  public:
    FeatureDescription(string ik, string icd, string ihr, string ireq)
      : Description(ik,icd,ihr,0,"void",";")
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
      : Description(ik,icd,ihr,0,"void",";"), min(imin), max(imax), reqf(ireqf)
    {}
    int getMin() const {return min;}
    int getMax() const {return max;}
  };

}
#endif
