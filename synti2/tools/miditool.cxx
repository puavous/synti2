/** Classes for processing MIDI and MISSS sequences and events, either
 * in real-time or off-line.
 *
 */

#include "miditool.hpp"
#include "midihelper.hpp"
#include "patchtool.hpp"
#include "../include/synti2_midi.h"
#include "../include/midi_spec.h"

#include <iostream>
#include <fstream>

#include <limits.h>


static
unsigned int bpm_to_usecpq(unsigned int bpm){
  return 60000000 / bpm;
}


static
unsigned int read_4byte(std::istream &ins){
  /*  char buf[4];
  ins.read(buf,4);
  return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];*/
  unsigned int res = (ins.get() << 24) 
    + (ins.get() << 16)
    + (ins.get() << 8)
    + (ins.get());
  return res;
}

static
unsigned int read_2byte(std::istream &ins){
  /*  unsigned int res;
  unsigned char buf[2];
  ins.read(buf,2);
  res = (buf[0] << 8) + buf[1];
  */
  unsigned int res = (ins.get() << 8) + ins.get();
  //std::cout << " Read 2byte " << std::dec << res << std::endl;
  return res;
}

static
int read_varlen(std::istream &ins, unsigned int *dest){
  int nread;
  unsigned char byte;
  *dest = 0;
  for (nread=1; nread<=4; nread++){
    byte = ins.get();
    *dest += (byte & 0x7f);
    if ((byte & 0x80) == 0)
      return nread; 
    else *dest <<= 7;
  }
  std::cerr << "Warning: varlength number longer than 4 bytes." << std::endl;
  return 0; 
}

void MidiTrack::addEvent(miditick_t tick, MidiEvent *ev){
  vec_tks.push_back(tick);
  vec_evs.push_back(ev);
}


void
MidiEvent::read_meta_ev(std::istream &ins){
  //int vlenlen;
  par1 = ins.get(); /* par1 will be meta event type. */
  read_varlen(ins, &par2); /* par2 will be meta event length. */
  /* Bulk data will be meta event contents: */
  for(unsigned int i=0; i<par2; i++){bulk.push_back(ins.get());}
}


void
MidiEvent::read_system_exclusive(std::istream &ins){
  unsigned char c = 0;
  /* par1 will be length. */
  for(par1=0; c!=0xf7; par1++){
    bulk.push_back(c = ins.get());
  }
}


MidiEvent::MidiEvent(int type, unsigned int subtype_or_channel, 
                     unsigned int par1, std::istream &ins){
  this->type = type;
  this->subtype_or_channel = subtype_or_channel;
  this->par1 = par1;
  this->par2 = 0; /* Zero for sysex and meta */


  switch(type){
  case MIDI_STATUS_PROGRAM: 
  case MIDI_STATUS_CHANNEL_PRESSURE:
    /* Only one parameter for these. */
    return;
  case MIDI_STATUS_NOTE_OFF: 
  case MIDI_STATUS_NOTE_ON:
  case MIDI_STATUS_KEY_PRESSURE:
  case MIDI_STATUS_CONTROL:  
  case MIDI_STATUS_PITCH_WHEEL:
    /* Two parameters for these. */
    this->par2 = ins.get();
    return;
  case MIDI_STATUS_SYSTEM:

    /* No parameters for these, but a bulk of other data may exist: */
    if (subtype_or_channel == MIDI_STATUS_SYSTEM){
      /* This means FF - meta event. */
      read_meta_ev(ins);
    } else {
      read_system_exclusive(ins);
    }
    return;
  default:
    /* FIXME: Should throw an exception. Invalid input. */
    std::cerr << "Hmm " << subtype_or_channel << " " <<  type;
    std::cerr << "I lost track of MIDI data." << std::endl;
  }
}


MidiEvent::MidiEvent(int type, unsigned int subtype_or_channel, 
                     unsigned int par1, unsigned int par2){
  this->type = type;
  this->subtype_or_channel = subtype_or_channel;
  this->par1 = par1;
  this->par2 = par2;
}

/** FIXME: Only works for note events, as of now!!! */
void 
MidiEvent::fromMidiBuffer(const unsigned char *buf)
{
  type = buf[0] >> 4;
  subtype_or_channel = buf[0] & 0x0f;
  par1 = buf[1];
  par2 = buf[2];
}

/** FIXME: Only works for note events, as of now!!! */
void 
MidiEvent::toMidiBuffer(unsigned char *buf) const {
  buf[0] = (type << 4) + subtype_or_channel;
  buf[1] = par1;
  if (isNote() || isCC() || isBend()){
    buf[2] = par2;
  }
}


void
MidiEvent::print(std::ostream &os)
{
  os << "type " << std::hex <<  this->type 
     << " sub " << std::hex << subtype_or_channel 
     << " par1 " << par1 << " par2 " << par2
     << " size of bulk " << bulk.size() << std::endl;
}



MidiEvent *
MidiTrack::createNormalizedEvent(std::istream &ins){
  int byte = ins.get();
  int type = byte >> 4;

  if (type < 0x8){
    /* "running status" - make a complete stand-alone event based on
     current status (for some events, an additional byte may be read
     from the stream. MidiEvent constructor will handle that for us): */
    return new MidiEvent(current_type, current_channel, byte, ins);
  }

  if ((0x8 <= type) && ( type <= 0xe)) {
    /* reset status - make an event, and update current running status. */
    current_type = type;
    current_channel = byte & 0xf;
    byte = ins.get(); /* read first parameter. */
    return new MidiEvent(current_type, current_channel, byte, ins);
  }

  /* else.. we are dealing with a meta event or sysex (no stat update): */
  return new MidiEvent(type, byte & 0xf, 0, ins);
}

MidiTrack::MidiTrack(){
  current_tick = 0; 
  current_type = 0; 
  current_channel = 0;
}

MidiTrack::MidiTrack(std::istream &ins){
  MidiTrack(); 
  readFrom(ins); 
  rewind();
}

void 
MidiTrack::readFrom(std::istream &ins){
  unsigned int hdr = read_4byte(ins);
  if (hdr != 0x4d54726b){
    std::cerr << "Failed to read MIDI track. Reason:" << std::endl;
    std::cerr << "Doesn't look like a MIDI track (No MTrk)." << std::endl;
    std::cerr << "Instead: " << std::hex << hdr << std::endl;
    return;
  }
  std::streamoff chunk_size = read_4byte(ins);
  unsigned int cur_tick = 0;
  std::streamoff last = ins.tellg() + chunk_size;
  unsigned int delta = 0;
  //while(ins.tellg() < last){  /* Hmm.. my bug why this don't work? */
  for(;;){
    read_varlen(ins, &delta);
    cur_tick += delta;
    MidiEvent* nev = createNormalizedEvent(ins); /* need side effect */
    addEvent(cur_tick, nev); /* Need a factory? */

    //std::cout << "/*SMF tick " << cur_tick << " ( delta " << delta << ")*/" << std::endl;
    //nev->print(std::cout);
    //std::cout << "nxt " << ins.tellg() << " last " << last << std::endl;
    if (nev->isEndOfTrack()) break;
  }
  //std::cout << "length = " << evs.size() << std::endl;
}

MidiEvent * 
MidiTrack::locateEvent(int type, int subtype, int par1){
  for (unsigned int i=0; i<vec_evs.size(); i++){
    if (vec_evs[i]->matches(type, subtype, par1)) return vec_evs[i];
  }
  return NULL;
}



MidiSong::MidiSong(std::ifstream &ins){
  
  /* TODO: would be better to throw an exception from here. */
  if (!ins.is_open()){
    std::cerr << "Failed to read MIDI file. Reason:" << std::endl;
    std::cerr << "File could not be opened." << std::endl;
    return;
  }
  if (read_4byte(ins) != 0x4d546864){
    std::cerr << "Failed to read MIDI file. Reason:" << std::endl;
    std::cerr << "Doesn't look like a MIDI file (No MThd)." << std::endl;
    return;
  }
  if (read_4byte(ins) != 6){
    std::cerr << "Failed to read MIDI file. Reason:" << std::endl;
    std::cerr << "MThd length not 6" << std::endl;
    /* Although the spec says the header 'could' be longer...*/
    return;
  }
  if (read_2byte(ins) >1) {
    std::cerr << "Failed to read MIDI file. Reason:" << std::endl;
    std::cerr << "Only MIDI formats 0 and 1 are supported." << std::endl;
    return;
  }

  unsigned int ntracks = read_2byte(ins);

  unsigned int time_division = read_2byte(ins);
  if (time_division & 0x8000) {
    std::cerr << "Failed to read MIDI file. Reason:" << std::endl;
    std::cerr << "SMPTE timings are not supported." << std::endl;
    return;
  }
  
  ticks_per_beat = time_division;
  
  for (unsigned int i=0; i<ntracks; i++){
    tracks.push_back(new MidiTrack(ins));
  }
}



unsigned int 
MidiSong::getTPQ(){
  return ticks_per_beat;
}

unsigned int 
MidiSong::getMSPQ(){
  MidiEvent * tempoev = tracks[0]->locateEvent(0xf, 0xf, 0x51);
  if (tempoev == NULL){
    return 120; 
  } else {
    return tempoev->get3byte(); /* hacks n hacks. TODO: fix these hacks */
  }
}

void
MidiSong::decimateTime(unsigned int new_tpb){
  for (unsigned int i=0; i<tracks.size(); i++){
    tracks[i]->tpqChange(getTPQ(),new_tpb);
  }
  ticks_per_beat = new_tpb;
}

void
MidiSong::linearize(std::vector<unsigned int> &ticks,
                    std::vector<MidiEvent> &evs)
{
  rewind();  /* rewind tracks.. */
  /* For each track, find next tick. see what comes next. */
  unsigned int ntick = 0;
  unsigned int i;

  bool theyHaveMore = true;
  while(theyHaveMore){
    /* Yield all events at this tick. No problem if there are none..*/
    for(i=0; i<tracks.size(); i++){
      while ( (tracks[i]->hasMore()) 
              && (tracks[i]->peekNextTick() == ntick)){
        ticks.push_back(ntick);
        evs.push_back(tracks[i]->stepOneEvent());
      }
    }

    /* see if our tick should change: */
    theyHaveMore = false;    
    unsigned int mint = UINT_MAX; /*c++ way for this?*/
    for(i=0; i<tracks.size(); i++){
      if (tracks[i]->hasMore()){
        theyHaveMore = true;
        unsigned int t = tracks[i]->peekNextTick();
        if (t<mint) mint = t;
      }
    }
    /* we have minimum. That must be our next tick. */
    ntick = mint;
  }
}
