/** Classes for processing MIDI and MISSS sequences and events, either
 * in real-time or off-line.
 *
 */

#include "miditool.hpp"

#include <iostream>
#include <fstream>


/* For simplicity, use the varlength of SMF.*/
static
int
encode_varlength(unsigned int value, unsigned char *dest){
  unsigned char bytes[4];
  int i, vllen;
  /* Chop to 7 bit pieces, MSB in position 0: */
  bytes[0] = (value >> 21) & 0x7f;
  bytes[1] = (value >> 14) & 0x7f;
  bytes[2] = (value >> 7) & 0x7f;
  bytes[3] = (value >> 0) & 0x7f;
  /* Set the continuation bits where needed: */
  vllen = 0;
  for(i=0; i<=3; i++){
    if ((vllen > 0) || (bytes[i] != 0) || (i==3)){
      vllen += 1;
      if (i<3) bytes[i] |= 0x80; /* set cont. bit */ 
    }
  }
  /* Put to output buffer MSB first */
  for(i=4-vllen; i<4; i++){
    *(dest++) = bytes[i];
  }
  return vllen; /* return length of encoded byte stream */
}

/* Formatting tool functions */
static
void fmt_hexbyte(std::ostream &outs, unsigned char b){
  outs << std::setiosflags(std::ios::right)
       << std::resetiosflags(std::ios::left)
       << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)b;
}

static
void fmt_comment(std::ostream &outs, std::string c){
  outs << "/* " 
       << std::setfill(' ') 
       << std::setiosflags(std::ios::left)
       << std::resetiosflags(std::ios::right)
       << std::setw(40) << (c+":") << " */ ";
}

static
void fmt_varlen(std::ostream &outs, unsigned int val){
  unsigned char buf[4];
  int len = encode_varlength(val, buf);
  for (int i=0; i<len; i++){
    fmt_hexbyte(outs, buf[i]);
    if (i<len-1) outs << ", ";
  }
  outs << "   /*" << std::dec << val <<  "*/ ";
}

static
unsigned int bpm_to_usecpq(unsigned int bpm){
  return 60000000 / bpm;
}


static
unsigned int read_4byte(std::istream &ins){
  char buf[4];
  ins.read(buf,4);
  return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static
unsigned int read_2byte(std::istream &ins){
  char buf[2];
  ins.read(buf,2);
  return (buf[0] << 8) + buf[1];
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
  evs[tick].push_back(ev);
}


void
MidiEvent::read_meta_ev(std::istream &ins){
  //int vlenlen;
  par1 = ins.get(); /* par1 will be meta event type. */
  read_varlen(ins, &par2); /* par2 will be meta event length. */
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
  case 0xc: case 0xd:
    /* Only one parameter for these. */
    return;
  case 0x8: case 0x9:  case 0xa:  case 0xb:  case 0xe:
    /* Two parameters for these. */
    this->par2 = ins.get();
    return;
  case 0xf:

    /* No parameters for these, but a bulk of other data may exist: */
    if (subtype_or_channel == 0xf){
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


MidiEvent *MidiTrack::createNormalizedEvent(std::istream &ins){
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

    std::cout << "tick " << cur_tick << " ";
    nev->print(std::cout);
    //std::cout << "nxt " << ins.tellg() << " last " << last << std::endl;
    if (nev->isEndOfTrack()) break;
  }
  //std::cout << "length = " << evs.size() << std::endl;
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



void
MisssChunk::do_write_header_as_c(std::ostream &outs){
  outs << std::endl;
  outs << "/* CHUNK begins ----------------------- */ " << std::endl;
  fmt_comment(outs, "Number of events"); fmt_varlen(outs, tick.size());
  outs << ", " << std::endl;
  fmt_comment(outs, "Channel"); fmt_hexbyte(outs, out_channel);
  outs << ", " << std::endl;
  fmt_comment(outs, "Type"); outs << "MISSS_LAYER_NOTES_CVEL_CPITCH";
  outs << ", " << std::endl;
}

void MisssNoteChunk::do_write_header_as_c(std::ostream &outs){
  MisssChunk::do_write_header_as_c(outs);
  fmt_comment(outs, "Default note"); fmt_hexbyte(outs, default_note);
  outs << ", " << std::endl;
  fmt_comment(outs, "Default velocity"); fmt_hexbyte(outs, default_velocity);
  outs << ", " << std::endl;
}

void
MisssNoteChunk::do_write_data_as_c(std::ostream &outs){
  outs << "/* delta and info : */ " << std::endl;
  unsigned int prev_tick=0;
  for (unsigned int i=0; i<tick.size(); i++){
    unsigned int delta = tick[i] - prev_tick; /* assume order */
    prev_tick = tick[i];
    fmt_varlen(outs, delta);
    outs << ", " << std::endl;
  }
  outs << std::endl;
}

void
MisssSong::write_as_c(std::ostream &outs){
  outs << "/*Song data generated by MisssSong */" << std::endl;
  outs << "#include \"synti2_misss.h\"" << std::endl;

  /*FIXME: Must name it differently soon, when it's not a hack anymore!*/
  outs << "unsigned char hacksong_data[] = {" << std::endl;

  outs << "/* *********** song header *********** */ " << std::endl;
  fmt_comment(outs, "Ticks per quarter");
  fmt_hexbyte(outs, 12);
  outs << ", " << std::endl;
  fmt_comment(outs, "Microseconds per minute"); 
  fmt_varlen(outs, bpm_to_usecpq(220));
  outs << ", " << std::endl;

  outs << "/* *********** chunks *********** */ " << std::endl;

  for (unsigned int i=0; i<chunks.size(); i++){
    chunks.at(i)->write_as_c(outs);
  }

  outs << std::endl;
  outs << "/* CHUNK: hack try -- basedrum on 4beat */ " << std::endl;
  outs << "/* Number of events in this chunk: */ 26," << std::endl;
  outs << "/* Channel of this chunk: */ 0x0a," << std::endl;
  outs << "/* Type    of this chunk: */ MISSS_LAYER_NOTES_CVEL_CPITCH," << std::endl;
  outs << "/* Parameter 1 (Ex. note): */ 39," << std::endl;
  outs << "/* Parameter 2 (Ex. vel): */ 127," << std::endl;
  outs << "/* delta and info : */ " << std::endl;
  outs << "   18,12,12,6,6,12,12,12,6,6,6,18,12,12,12,12,3,3,3,6,6,6,8,8,8,8," << std::endl;



  outs << "/* *********** end *********** */ " << std::endl;
  outs << "/* End of data marker: */ 0x00};" << std::endl;
}



void 
MidiEventTranslator::hack_defaults()
{
  int hack_drummap[12] = {
    /*Chan 9  Oct -5 */ 0,0,1,1,1,3,2,3,2,3,4,3,
  };

  /* For example: 5-poly piano, 2-poly high-string, mono bass, mono lead,
     drums (bd,sd,oh,ch,to), effects? */
  reset_state(); /* Start with nothing, and only set some non-zeros */
  voice_rotate[0]=4; /*Channels 1-4 are 'polyphonic' */
  voice_rotate[5]=2; /*Channels 6-7 are 'biphonic' */

  /* Drum channel diversions as in GM for one octave (bd, sd, sd2 ...): */
  for (int j=0;j<MIDI_NNOTES;j++) {
    channel_table[9][j] = hack_drummap[j % 12];
  }
}

/** Zero everything */
void 
MidiEventTranslator::reset_state(){
  int i,j;
  for(int i=0; i<MIDI_NCHANNELS; i++){
    voice_rotate[i]=1;
    next_rotation[i]=0;
  }
  
  for (i=0;i<MIDI_NCHANNELS;i++){
    /* No notes "playing" when we begin. */
    for (j=0;j<MIDI_NNOTES;j++) rotation_of_noteon[i][j] = -1; 
    for (j=0;j<MIDI_NCHANNELS;j++) note_of_rotation[i][j] = -1;
    /* No notewise diversions ("keyboard splits") */
    for (j=0;j<MIDI_NNOTES;j++) channel_table[i][j] = 0;
  }
}


MidiEventTranslator::MidiEventTranslator(){
  /* FIXME: Zero everything. Maybe make a constructor that can read
   * a file.
   */
  hack_defaults();
}
  

/** Just a dirty hack to rotate channels, if wired to do so.*/
int
MidiEventTranslator::rotate_notes(jack_midi_event_t *ev)
{
  int cmd, chn, note;
  int newrot, oldrot, newchn;

  cmd = ev->buffer[0] >> 4;
  chn = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  if (cmd == 0x09){
    /* Note on goes to the next channel in rotation. */
    newrot = next_rotation[chn] % voice_rotate[chn];
    
    next_rotation[chn]++; /* tentative for next note-on. */
    newchn = chn + newrot;  /* divert to the rotated channel */

    /* Keep a record of where note ons have been put. When there is an
     * overriding note-on, earlier note-on must be erased, so that
     * note-off can be skipped (the note is "lost"/superseded)
     */
    if (note_of_rotation[chn][newrot] >= 0){
      /* Forget the superseded note-on: */
      rotation_of_noteon[chn][note_of_rotation[chn][newrot]] = -1;
    }
    /* remember the new noteon */
    note_of_rotation[chn][newrot] = note;
    rotation_of_noteon[chn][note] = newrot;
  } else if (cmd == 0x08){
    /* Note off goes to the same channel, where the corresponding note
       on was located earlier. FIXME: should swallow if note on is no
       more relevant.  */
    if (rotation_of_noteon[chn][note] < 0) return 0; /* Dismissed already*/

    oldrot = rotation_of_noteon[chn][note];
    note_of_rotation[chn][oldrot] = -1; /* no more note here. */
    newchn = chn+oldrot; /* divert note-off to old target. */
  } else {
    return 1; /* other messages not handled. */
  }

  ev->buffer[0] = (cmd<<4) + newchn; /* mogrify event. */
  return 1;
}


/** Just a dirty hack to spread drums out to different channels. */
void
MidiEventTranslator::channel(jack_midi_event_t *ev){
  int nib1, nib2, note;
  nib1 = ev->buffer[0] >> 4;
  nib2 = ev->buffer[0] & 0x0f;
  note = ev->buffer[1];

  nib2 = nib2 + channel_table[nib2][note];

  ev->buffer[0] = (nib1<<4) + nib2;
}
