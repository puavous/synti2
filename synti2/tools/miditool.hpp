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
  bool isNote(){return ((type == 0x9) || (type == 0xa));}
  int getChannel(){return subtype_or_channel;}
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
  MidiTrack(){current_tick = 0; current_type = 0; current_channel = 0;}
  MidiTrack(std::istream &ins){MidiTrack(); readFrom(ins); rewind();}
  ~MidiTrack(){for(unsigned int i=0; i<vec_evs.size(); i++){delete vec_evs[i];}}

  void rewind(){current_tick = 0; play_ind = 0;}
  /* FIXME: With this design, track cannot really change (very easily
     / efficiently) after it has been rewound once. */
  bool hasMore(){return (play_ind < vec_tks.size());}

  unsigned int peekNextTick(){return vec_tks[play_ind];}
  
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
  MidiSong(std::ifstream &ins);
  ~MidiSong(){
    for(unsigned int i=0; i<tracks.size(); i++) delete tracks[i];}
  void rewind(){
    for(unsigned int i=0; i<tracks.size(); i++) tracks[i]->rewind();
  }

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
  //  int rotate_notes(unsigned char *ev);
  //  void channel(unsigned char *ev);

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
protected:
  void do_write_header_as_c(std::ostream &outs);
  void do_write_data_as_c(std::ostream &outs);
public:
  MisssNoteChunk(int in_c, int out_c, int def_n, int def_v) 
    : MisssChunk(in_c, out_c)
  {    default_note = def_n;    default_velocity = def_v;  }

  /** Must be called in increasing time-order. */
  bool acceptEvent(unsigned int t, MidiEvent &ev);
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

  void build_chunks_from_spec(std::istream &spec);
  void translated_grab_from_midi(MidiSong &midi_song, 
                                 MidiEventTranslator &trans);

public:
  MisssSong(MidiSong &midi_song, 
            MidiEventTranslator &trans, 
            std::istream &spec)
  {
    build_chunks_from_spec(spec);
    translated_grab_from_midi(midi_song, trans);
  }

  void write_as_c(std::ostream &outs);
};


#endif
