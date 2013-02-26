#ifndef MISSS_EVENT_H_INCLUDED
#define MISSS_EVENT_H_INCLUDED

#include "synti2_misss.h"

namespace synti2{

  /** Misss event is the message type used internally by Synti2. There
      are two kinds of messages:

      - note on

      - controller ramp. // Think about subclassing here!
  */
  class MisssEvent{
    /* FIXME: Proper interface and so on..  */
  private:
    int type;   /* MisssEvent is either a note or a ramp.. */
    int voice;  /* Always on a voice. */
    int par1;   /* Always one param*/
    int par2;   /* In notes also velocity. */
    float time; /* In ramps time and target value*/
    float target; /* In ramps time and target value*/
  public:
    MisssEvent(const unsigned char *misssbuf);
    bool isNote() {return type == MISSS_MSG_NOTE;}
    bool isRamp() {return type == MISSS_MSG_RAMP;}
    int getVoice() {return voice;}
    int getNote() {return par1;}
    int getVelocity() {return par2;}

    int getMod() {return par1;}
    float getTime() {return time;}
    float getTarget() {return target;}
    //print(std::ostream os);
  };
}
#endif
