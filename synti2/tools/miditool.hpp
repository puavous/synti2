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


/** MidiSong can read a standard midi file, and iterate it (call it
 *  "playback" if you wish) one event at a time.
 */
class MidiSong {
  std::map<miditick_t, evdata_t *> evs;
  MidiSong(const char *fname){;}
};


/* A Misss chunk is a container for "distilled" and "pre-ordered" and
   "packed" "subset" of MIDI-like event data. Specifically, each chunk
   deals with exactly one channel and exactly one type of messages,
   and the message type must be one of those that the very simple
   event merging device of Synti2 sequence loader can handle, i.e.:

   - note on (possibly with constant velocity and/or constant pitch)

   - controller ramp. // Think about subclassing here!
*/
class MisssChunk{
private:
  int channel; /* channel of this chunk */
  int par1; /* parameter 1 of this chunk */
  int par2; /* parameter 2 of this chunk */
  std::vector<unsigned int> tick;  /* time ticks */
  std::vector<unsigned int> dataind;  /* corresp. data locations */
  std::vector<unsigned char> data; /* actual data directly as bytes */
public:
  // Something like this for creation;
  //void from_midi_song_using_translator(MidiSong &s, MidiTranslator &mt);
  // maybe sniff_note_ons_from_channel(MidiSong &s, MidiTranslator &mt, chan);
  /* Or maybe in OOP style just something like:

     setAcceptChn(12);   <- superclass
     Type("Note on"),  <- subclass, actually maybe class MisssNoteChunk
     setNoteAcceptRange("1..32"); <- in MisssNoteChunk only
     setConstantNote(24); <- in MisssNoteChunk!! Wow! THIS determines subtype
     setConstantNote(-1); <- in MisssNoteChunk!! becomes other subtype..
     setConstantVel(100); <- in MisssNoteChunk!! Wow! THIS determines subtype
     setVelAcceptRange(90,110); <- in MisssNoteChunk! THIS is the decimation!
     setVelAcceptRange(1,127); <- and THIS is "no note off" subtype!

     setAcceptController(32); <- in MisssRampChunk only
     setOutputRange(0.0, 1.27); <- in MisssRampChunk only

     Then play back once through translator and each chunk goes
     sniffing for their respective food:

     for each chunk:
       if (chunk.eat_if_tasty(midievent)) break;

     Then MisssRampChunk could have nice algorithms for ramp
     determination, and MisssBulkChunk would be just another, simpler,
     subclass. 

       rampchunk.setDelta() <- THIS is the generative delta which fits
       in the two-byte parameters. OH NO! It doesn't.. need a third
       byte (maybe.. think!) But there could be three for this chunk
       and still only two for the others. It's only an if and add in
       the exe anyway.

       rampchunk.setExaminationTime() <- parameters for the linear
       approximation can be set like this.

     MisssChunk could be a virtual superclass, and Misss file creation
     would become just a matter of building a chunk list from a GUI
     button callback. Oh, but it needs to be serializable. So all
     these will have constructors with a string parameter:

       MisssNoteChunk nc1("Chn: 1 VelRange: 1 127 DefVel: -1 DefNote: -1");
       MisssNoteChunk nc2("Chn: 2 VelRange: 0 127 DefVel: -1 DefNote: -1");
       MisssNoteChunk nc3("Chn: 1 VelRange: 0 0 DefVel: 0 DefNote: -1");

     YES!! Do exactly this. FIXME: asap!!!
  */
  // or something like: from_midi_song_picking_only("chn 1 note on");
  void write_as_c(std::ostream &outs);
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

  std::vector<MisssChunk> chunks;
public:
  MisssSong(){MisssChunk hack; chunks.push_back(hack); chunks.push_back(hack);}
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
