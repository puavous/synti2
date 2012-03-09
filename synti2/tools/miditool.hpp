#ifndef MIDITOOL_H_INCLUDED
#define MIDITOOL_H_INCLUDED

/** Classes for processing MIDI and MISSS sequences and events, either
 * in real-time or off-line.
 *
 */

#include "jack/midiport.h"

#define MIDI_NCHANNELS 16
#define MIDI_NNOTES 128

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

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
  bool isEndOfTrack(){
    return ((type == 0xf) && (subtype_or_channel == 0xf) && (par1 == 0x2f));}
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
  unsigned int current_tick; /* current tick for create and play.. not really nice, but works for now... */
  unsigned int current_type;
  unsigned int current_channel;

  typedef unsigned int miditick_t;
  std::map<miditick_t, std::vector<MidiEvent*> > evs; /* multimap OK? why?wnot?*/

  /** Adds an event (pointer) at a time tick. Who deletes the event?
   *  Should we make a deep copy after all?
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
  MidiTrack(){current_tick = 0; current_type = 0; current_channel = 0;}
  MidiTrack(std::istream &ins){MidiTrack(); readFrom(ins);}
  ~MidiTrack(){/*FIXME: we need to delete our events, because we have allocated them!! Not sure yet, where they'll reside, though... FIXME: asap. */}

  MidiEvent& nextEventUpToTick(unsigned int t){;}
};

/** MidiSong can read a standard midi file, and iterate it (call it
 *  "playback" if you wish) one event at a time.
 */
class MidiSong {
  unsigned int ticks_per_beat;
  std::map<miditick_t, evdata_t *> evs;
  std::vector<MidiTrack*> tracks;
public:
  int getNTracks(){return tracks.size();}
  MidiSong(std::ifstream &ins);
  ~MidiSong(){for(unsigned int i=0; i<tracks.size(); i++){delete tracks[i];}}
  void rewind(){};
  MidiEvent nextEvent(){return MidiEvent(0x9, 0x9, 35, 127);/*FIXME*/}
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
public:
  /* The only way to create a chunk is by giving a text description(?)*/
  //virtual MisssChunk(std::string desc){bytes_per_ev = 3;}
  MisssChunk(){in_channel = 0; out_channel = 0x9;}
  virtual bool acceptEvent(MidiEvent &ev){return false;}
  void write_as_c(std::ostream &outs){
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
protected:
  void do_write_header_as_c(std::ostream &outs);
  void do_write_data_as_c(std::ostream &outs);
public:
  MisssNoteChunk(){
    default_note = -1;
    default_velocity = -1;

    default_note = 35; /* hack bd*/
    default_velocity = 100;
    for (int hack=0; hack<10; hack++){
      tick.push_back(hack*6); /*dataind.push_back(hack); data.push_back();*/
    }
  }
  bool acceptEvent(MidiEvent &ev){return false;}
};

class MisssRampChunk : public MisssChunk {
private:
  /* parameters */
  int control_target;
  int range_min;
  int range_max;
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
public:
  MisssSong(){
    chunks.push_back(new MisssNoteChunk());
    chunks.push_back(new MisssNoteChunk());
    //    chunks.push_back(hack);
  }
  void write_as_c(std::ostream &outs);
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
  int rotate_notes(jack_midi_event_t *ev);
  void channel(jack_midi_event_t *ev);

};

#endif
