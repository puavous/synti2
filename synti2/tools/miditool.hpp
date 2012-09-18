#ifndef MIDITOOL_H_INCLUDED
#define MIDITOOL_H_INCLUDED

/** Classes for processing MIDI and MISSS sequences and events, either
 * in real-time or off-line.
 *
 */

#include "jack/midiport.h"

#include "midihelper.hpp"

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
 *  subclassing, but I have no time to think right now..f
 */
class MidiEvent{
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
  /* quick hack for tempo setting: */
  int get3byte() const {return (bulk[0] << 16) + (bulk[1] << 8) + bulk[3];}
  int getNote(){return par1;}
  int getCCnum(){return par1;} /*hmm*/
  int getCCval(){return par2;} /*hmm*/
  int getVelocity(){return par2;}
  int getChannel(){return subtype_or_channel;}
  bool isEndOfTrack(){
    return ((type == 0xf) && (subtype_or_channel == 0xf) && (par1 == 0x2f));}

  /** FIXME: Only works for note events, as of now!!! */
  void fromMidiBuffer(const unsigned char *buf){
    type = buf[0] >> 4;
    subtype_or_channel = buf[0] & 0x0f;
    par1 = buf[1];
    par2 = buf[2];
  }

  /** FIXME: Only works for note events, as of now!!! */
  void toMidiBuffer(unsigned char *buf) const {
    buf[0] = (type << 4) + subtype_or_channel;
    buf[1] = par1;
    buf[2] = par2;
  }



  void print(std::ostream &os){
    os << "type " << std::hex <<  this->type 
       << " sub " << std::hex << subtype_or_channel 
       << " par1 " << par1 << " par2 " << par2
       << " size of bulk " << bulk.size() << std::endl;
  }
};


/** MIDI Track as loaded from SMF0 or SMF1. Can read itself from a
 *  binary stream, and iterate ("play back") up to a tick
 *  value. Events are "normalized" upon load: running statuses are
 *  separated into self-contained events.
 */
class MidiTrack {
  /* current tick for create and play.. not nice, but works for now... */
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

  void divideTimesBy(int divisor){
    for(unsigned int i=0; i<vec_tks.size(); i++){
      vec_tks[i] = vec_tks[i] / divisor;
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



/** The translator eats a standard midi message, and pukes an altered
 *  message based on loaded configuration and current state. (TODO: as
 *  midi or as missss?). Doesn't care if it is working in a real-time
 *  or off-line job, it just answers when a question is asked.
 */
class MidiEventTranslator {
private:
  int voice_rotate[MIDI_NCHANNELS];
  int next_rotation[MIDI_NCHANNELS];
  int rotation_of_noteon[MIDI_NCHANNELS][MIDI_NNOTES];
  int note_of_rotation[MIDI_NCHANNELS][MIDI_NCHANNELS];
  int channel_table[MIDI_NCHANNELS][MIDI_NNOTES];

  /** default settings; FIXME: to be removed after proper accessors exist. 
   */
  void hack_defaults();
  /** Reset everything.. FIXME: This should mean a bit different thing.. */
  void reset_state();
public:
  MidiEventTranslator();

  /* we do it on a raw buffer.. */
  int rotate_notes(unsigned char *buffer);
  void channel(unsigned char *buffer);
  void off_to_zero_on(unsigned char *buffer);


  int rotate_notes(jack_midi_event_t *ev);
  void channel(jack_midi_event_t *ev);

  MidiEvent transformOffline(const MidiEvent &evin);
};


/* A Misss chunk is a container for "distilled" and "pre-ordered" and
   "packed" "subset" of MIDI-like event data. Specifically, each chunk
   deals with exactly one channel and exactly one type of messages,
   and the message type must be one of those that the very simple
   event merging device of Synti2 sequence loader can handle, i.e.:

   - note on (possibly with constant velocity and/or constant pitch)

   - controller ramp. // Think about subclassing here!

  Purpose of the chunks is just to grab the filtered events, and it
  doesn't handle channel mapping. That is the responsibility of
  MidiTranslator.

 */


class MisssChunk{
protected:
  int in_channel;  /* accept channel of this chunk */
  int out_channel; /* output channel of this chunk */
  int bytes_per_ev; /* length of one event*/
  std::vector<unsigned int> tick;  /* times of events as ticks. */
  std::vector<unsigned int> dataind;  /* corresp. data locations */
  std::vector<unsigned char> data; /* actual data directly as bytes */
  virtual void do_write_header_as_c(std::ostream &outs);
  virtual void do_write_data_as_c(std::ostream &outs) = 0;
  bool channelMatch(MidiEvent &ev){ return (in_channel == ev.getChannel());}
public:
  /* The only way to create a chunk is by giving a text description(?)*/
  //virtual MisssChunk(std::string desc){bytes_per_ev = 3;}
  MisssChunk(int in_ch, int out_ch){
    in_channel = in_ch; 
    out_channel = out_ch;}
  virtual bool acceptEvent(unsigned int t, MidiEvent &ev){
    return false;
  }
  int size(){return tick.size();}
  void write_as_c(std::ostream &outs){
    if (size() == 0) return; /* zero-length, don't write. */
    do_write_header_as_c(outs);
    do_write_data_as_c(outs);
  }
};

class MisssNoteChunk : public MisssChunk 
{
private:
  /* parameters */
  int default_note;
  int default_velocity;
  int accept_vel_min;
  int accept_vel_max;
protected:
  void do_write_header_as_c(std::ostream &outs);
  void do_write_data_as_c(std::ostream &outs);
public:
  MisssNoteChunk(int in_c, int out_c, int def_n, int def_v, 
                 int avmin, int avmax) 
    : MisssChunk(in_c, out_c)
  {    default_note = def_n;    default_velocity = def_v;  
    accept_vel_min = avmin; accept_vel_max = avmax;}

  /** Must be called in increasing time-order. */
  bool acceptEvent(unsigned int t, MidiEvent &ev);
};


/* FIXME: Attend to this next!!*/
class MisssRampChunk : public MisssChunk {
private:
  /* parameters */
  int control_input; /* midi controller to listen to. Hmm.. Pitch? Pressure? */
  int control_target; /* synti2 controller */
  float range_min;
  float range_max;
protected:
  void do_write_header_as_c(std::ostream &outs);
  void do_write_data_as_c(std::ostream &outs);
public:
  MisssRampChunk(int in_c, int out_c, 
                 int midi_cc, 
                 int s2_controller,
                 float out_range_min,
                 float out_range_max) 
    : MisssChunk(in_c, out_c)
  {
    control_input = midi_cc;
    control_target = s2_controller;
    range_min = out_range_min;
    range_max = out_range_max;
  }
  /** Must be called in increasing time-order. */
  bool acceptEvent(unsigned int t, MidiEvent &ev);

  // mapping_type
};


/** MisssSong can sniff a MidiSong using a MidiTranslator and save a
 * MISSS (midi-like interface for synti2 software synthesizer) data
 * package as C-language source code, directly compilable into a 4k
 * intro, as the main use case of this programming excercise was.
 */
class MisssSong {
private:
  unsigned int ticks_per_quarter;
  unsigned int usec_per_quarter;

  std::vector<MisssChunk*> chunks;

  void figure_out_tempo_from_midi(MidiSong &midi_song);
  void build_chunks_from_spec(std::istream &spec);
  void translated_grab_from_midi(MidiSong &midi_song, 
                                 MidiEventTranslator &trans);

public:
  MisssSong(MidiSong &midi_song, 
            MidiEventTranslator &trans, 
            std::istream &spec);

  void write_as_c(std::ostream &outs);
};


#endif
