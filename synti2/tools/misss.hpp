#ifndef MISSS_WRAPPER_H_INCLUDED
#define MISSS_WRAPPER_H_INCLUDED

#include <iostream>
#include "misssevent.hpp"
#include "miditool.hpp"
#include "midimaptool.hpp"

namespace synti2{

  /** 
   * A Misss chunk is a container for "distilled" and "pre-ordered"
   * and "packed" "subset" of Misss events. MIDI-like event
   * data. Specifically, each chunk deals with exactly one channel and
   * exactly one type of messages.
   * 
   * Purpose of the chunks is just to grab the events after they have
   * been filtered, and it doesn't handle channel mapping. That is the
   * sovereign responsibility of MidiMap.
   */
  class MisssChunk{
  protected:
    int voice;  /* accept/output voice of this chunk */
    std::vector<unsigned int> tick;  /* times of events as ticks. */
    std::vector<MisssEvent> evt;     /* corresp. events */

    virtual void do_write_header_as_c(std::ostream &outs);
    virtual void do_write_data_as_c(std::ostream &outs) = 0;
    bool voiceMatch(MisssEvent &ev){return (voice == ev.getVoice());}
  public:
    MisssChunk(int ivoice){ voice = ivoice;}
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
    int accept_vel_min;
    int accept_vel_max;
  protected:
    void do_write_header_as_c(std::ostream &outs);
    void do_write_data_as_c(std::ostream &outs);
  public:
    MisssNoteChunk(int ivoice, int avmin, int avmax) 
      : MisssChunk(ivoice)
    {accept_vel_min = avmin; accept_vel_max = avmax;}
    int computeDefaultNote();
    int computeDefaultVelocity();
    
    /** Must be called in increasing time-order. */
    bool acceptEvent(unsigned int t, MisssEvent &ev);
  };
  
  
  class MisssRampChunk : public MisssChunk {
  private:
    /* parameters */
    int mod; /* synti2 modulator to listen to. */
  protected:
    void do_write_header_as_c(std::ostream &outs);
    void do_write_data_as_c(std::ostream &outs);
  public:
    MisssRampChunk(int ivoice, int imod) 
      : MisssChunk(ivoice)
    { mod = imod; }
    /** Must be called in increasing time-order. */
    bool acceptEvent(unsigned int t, MisssEvent &ev);
    int getModNumber(){return mod;}
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
    ~MisssSong(){for(size_t i=0;i<chunks.size();i++) delete chunks[i];}
    
    void write_as_c(std::ostream &outs);
  };
  
}
#endif
