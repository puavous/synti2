#ifndef MISSS_WRAPPER_H_INCLUDED
#define MISSS_WRAPPER_H_INCLUDED

#include <iostream>
#include "misssevent.hpp"
#include "miditool.hpp"
#include "midimaptool.hpp"

namespace synti2{

  /** A Misss chunk is a container for "distilled" and "pre-ordered" and
      "packed" "subset" of Misss events. MIDI-like event
      data. Specifically, each chunk deals with exactly one channel and
      exactly one type of messages.
      
      Purpose of the chunks is just to grab the filtered events, and it
      doesn't handle channel mapping. That is the responsibility of
      MidiMap.
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
    virtual bool acceptEvent(unsigned int t, MisssEvent &ev){
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
                                   MidiMap &mapper);
    
  public:
    MisssSong(MidiSong &midi_song, 
              MidiMap &mapper, 
              std::istream &spec);
    
    void write_as_c(std::ostream &outs);
  };
  
}
#endif
