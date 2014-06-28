#ifndef MIDITOOL_H_INCLUDED
#define MIDITOOL_H_INCLUDED

/** 
 * Classes for processing MIDI and MISSS sequences and events, either
 * in real-time or off-line.
 *
 */

#include "jack/midiport.h"

#include "midihelper.hpp"
#include <cmath>

/* We're gonna use the same mapper here as in the synth.. */
#include "synti2_midi.h"
#include "synti2_midi_guts.h"

#define MIDI_NCHANNELS 16
#define MIDI_NNOTES 128

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

typedef unsigned int miditick_t;
typedef unsigned char evdata_t;


/** MIDI Event - something that is supposed to happen. Event does not
 *  know its location in time, but only its effect. One class models
 *  all the different kinds of events. This may not be as flexible as
 *  subclassing, but I have no time to think right now..
 */
class MidiEvent{
  /* FIXME: Why not just store the bytes, and use the accessors to the
     bulk??*/
  int type;
  int subtype_or_channel;
  unsigned int par1;   /* parameters are used with channel events.. */
  unsigned int par2;   /* other events can set these to 0 */
  std::vector<unsigned char> bulk; /* for sysex, lyrics, and friends */
private:
  void read_meta_ev(std::istream &ins);
  void read_system_exclusive(std::istream &ins);
public:
  MidiEvent(int type, unsigned int subtype_or_channel, 
            unsigned int par1, std::istream &ins);
  MidiEvent(int type, unsigned int subtype_or_channel, 
            unsigned int par1, unsigned int par2);
  bool matches(int type, int subtype, unsigned int par1){
    return (this->type==type)
      && (this->subtype_or_channel == subtype)
      && (this->par1 == par1);
  }
  bool isNote() const {return ((type == 0x9) || (type == 0x8));}
  bool isCC() const {return (type == 0xb);}
  bool isPressure() const {return (type == 0xd);}
  bool isBend() const {return (type == 0xe);}

  /* quick hack for tempo setting: */
  int get3byte() const {
    return (bulk[0] << 16) + (bulk[1] << 8) + bulk[2 /*FIXME: error? */];}
  int getNote(){return par1;}
  int getCCnum(){return par1;} /*hmm*/
  int getCCval(){return par2;} /*hmm*/
  int getVelocity(){return par2;}
  int getChannel(){return subtype_or_channel;}
  bool isEndOfTrack(){
    return ((type == 0xf) && (subtype_or_channel == 0xf) 
            && (par1 == 0x2f));}

  void fromMidiBuffer(const unsigned char *buf);
  void toMidiBuffer(unsigned char *buf) const;
  void print(std::ostream &os);
};


/** MIDI Track as loaded from SMF0 or SMF1. Can read itself from a
 *  binary stream, and iterate ("play back") up to a tick
 *  value. Events are "normalized" upon load: running statuses are
 *  separated into self-contained events.
 */
class MidiTrack {
  /* current tick for create and play.. not nice, but works for
     now... */
  unsigned int current_tick;  /* for create */

  unsigned int current_type;   /* for create */
  unsigned int current_channel; /* for create */

  unsigned int play_ind; /* for play */
  //unsigned int current_evind; /* for play */

  typedef unsigned int miditick_t;
  std::vector<miditick_t> vec_tks;
  std::vector<MidiEvent*> vec_evs;

  /** Adds an event (pointer) at a time tick. As of now, must preserve
   * ordering. Assume that we have created the event.
   */
  void addEvent(miditick_t tick, MidiEvent *ev);
protected:
  void readFrom(std::istream &ins);
  /** Creates a normalized midi event. Normalization depends on
   *  current status and side effects that take place while "transport
   *  is running".
   */
  MidiEvent* createNormalizedEvent(std::istream &ins);
public:
  MidiTrack();
  MidiTrack(std::istream &ins);
  ~MidiTrack(){for(unsigned int i=0; i<vec_evs.size(); i++){delete vec_evs[i];}}

  void tpqChange(int from, int to){
    double mul = (double)to / from;
    for(unsigned int i=0; i<vec_tks.size(); i++){
      vec_tks[i] = std::floor((mul * vec_tks[i])+.5);
    }
  }

  void rewind(){current_tick = 0; play_ind = 0;}
  /* FIXME: With this design, track cannot really change (very easily
     / efficiently) after it has been rewound once. */
  bool hasMore(){return (play_ind < vec_tks.size());}

  unsigned int peekNextTick(){return vec_tks[play_ind];}

  MidiEvent * locateEvent(int type, int subtype, int par1);
  
  /* Return a (deep) copy of current event. */
  MidiEvent stepOneEvent(){
    return *(vec_evs[play_ind++]); /* cpy. */
  }
};

/** MidiSong can read a standard midi file, and iterate it (call it
 *  "playback" if you wish) one event at a time.
 */
class MidiSong {
  unsigned int ticks_per_beat;
  std::map<miditick_t, evdata_t *> evs;
  std::vector<MidiTrack*> tracks;

public:
  int getNTracks(){return tracks.size();} /* Necessary? */
  unsigned int getTPQ();
  unsigned int getMSPQ();
  MidiSong(std::ifstream &ins);
  ~MidiSong(){
    for(unsigned int i=0; i<tracks.size(); i++) delete tracks[i];}
  void rewind(){
    for(unsigned int i=0; i<tracks.size(); i++) tracks[i]->rewind();
  }

  void decimateTime(unsigned int new_tpb);
  void linearize(std::vector<unsigned int> &ticks,
                 std::vector<MidiEvent> &evs);
  /** Advance song position to next event. Tick may or may not increase. */
};


#endif
