/** Implementation of C++ wrappers for the internal message data
 *  structures of synti2. Only as much as the tool programs need, so
 *  far...
 */

#include "synti2_misss.h"
#include "misss.hpp"

/* Formatting tool functions for exporting as C source */
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

synti2::MisssEvent::MisssEvent(const unsigned char *misssbuf){
  type = *misssbuf++;
  voice = *misssbuf++;
  par1 = *misssbuf++;
  if (type==MISSS_MSG_NOTE){
    par2 = *misssbuf++;
  } else if (type==MISSS_MSG_RAMP) {
    time = *((float*)misssbuf);
    misssbuf += sizeof(float);
    target = *((float*)misssbuf);
    misssbuf += sizeof(float);
  }
}


void
synti2::MisssChunk::do_write_header_as_c(std::ostream &outs){
  outs << std::endl;
  outs << "/* CHUNK begins ----------------------- */ " << std::endl;
  fmt_comment(outs, "Number of events"); fmt_varlen(outs, tick.size());
  outs << ", " << std::endl;
  fmt_comment(outs, "Channel"); fmt_hexbyte(outs, out_channel);
  outs << ", " << std::endl;
}

void 
synti2::MisssNoteChunk::do_write_header_as_c(std::ostream &outs)
{
  MisssChunk::do_write_header_as_c(outs);
  fmt_comment(outs, "Type"); outs << "MISSS_LAYER_NOTES";
  outs << ", " << std::endl;
  fmt_comment(outs, "Default note"); fmt_hexbyte(outs, default_note);
  outs << ", " << std::endl;
  fmt_comment(outs, "Default velocity"); fmt_hexbyte(outs, default_velocity);
  outs << ", " << std::endl;
}

void
synti2::MisssNoteChunk::do_write_data_as_c(std::ostream &outs)
{
  unsigned int di = 0;
  outs << "/* delta and info : */ " << std::endl;
  unsigned int prev_tick=0;
  for (unsigned int i=0; i<tick.size(); i++){
    unsigned int delta = tick[i] - prev_tick; /* assume order */
    prev_tick = tick[i];
    fmt_varlen(outs, delta);
    outs << ", ";
    if (default_note < 0) {
      fmt_hexbyte(outs, data[di++]); /* omg. hacks start appearing. */
      outs << ", ";
    }
    if (default_velocity < 0) {
      fmt_hexbyte(outs, data[di++]); /* omg. hacks start appearing. */
      outs << ", ";
    }
    outs << std::endl;
  }
  outs << std::endl;
}


bool
synti2::MisssNoteChunk::acceptEvent(unsigned int t, MidiEvent &ev)
{
  if (!ev.isNote()) return false;
  if (!channelMatch(ev)) return false;
  if (! ((accept_vel_min <= ev.getVelocity()) 
         && (ev.getVelocity() <= accept_vel_max))) return false;

  tick.push_back(t);
  dataind.push_back(data.size());
  if (default_note < 0){
    data.push_back(ev.getNote());
  }
  if (default_velocity < 0){
    data.push_back(ev.getVelocity());
  }

  return true;
}

void 
synti2::MisssRampChunk::do_write_header_as_c(std::ostream &outs)
{
  MisssChunk::do_write_header_as_c(outs);
  fmt_comment(outs, "Type"); outs << "MISSS_LAYER_CONTROLLER_RAMPS";
  outs << ", " << std::endl;
  fmt_comment(outs, "Synti2 controller number (zero-based)"); fmt_hexbyte(outs, control_target);
  outs << ", " << std::endl;
  fmt_comment(outs, "Unused parameter (dup)"); fmt_hexbyte(outs, control_target);
  outs << ", " << std::endl;
}

void
synti2::MisssRampChunk::do_write_data_as_c(std::ostream &outs)
{
  //outs << "BROKEN: Not yet implemented: MisssRampChunk::do_write_data_as_c()*/ " << std::endl;

  unsigned int di = 0;
  outs << "/* delta and info : */ " << std::endl;
  unsigned int prev_tick=0;
  for (unsigned int i=0; i<tick.size(); i++){
    unsigned int delta = tick[i] - prev_tick; /* assume order */
    prev_tick = tick[i];
    fmt_varlen(outs, delta);
    outs << ", ";

    /* no earlier than here starts the differences to notes */
    size_t len;
    /* FIXME: this should go into an overloaded function in midihelper.cxx:*/
    unsigned char buf[8];
    for(int i=0;(i<8)&&((di+i) < data.size()); i++){
      buf[i] = data[di+i];
    }

    unsigned int intval;
    float fval;
    len = decode_varlength(buf, &intval);
    fval = synti2::decode_f(intval);
    std::cout << "/* time " << fval << "s */";
    
    for(unsigned int i=0;i<len;i++){
      fmt_hexbyte(outs, data[di++]); /* omg. hacks start appearing. */
      outs << ", ";
    }
    /* and value: */
    len = decode_varlength(buf+len, &intval);
    fval = synti2::decode_f(intval);
    std::cout << "/* value " << fval << " */";

    for(unsigned int i=0;i<len;i++){
      fmt_hexbyte(outs, data[di++]); /* omg. hacks start appearing. */
      outs << ", ";
    }

    outs << std::endl;
  }
  outs << std::endl;
}

bool
synti2::MisssRampChunk::acceptEvent(unsigned int t, MidiEvent &ev)
{
  if (!ev.isCC()) return false;
  if (!channelMatch(ev)) return false;
  if (!(control_input == ev.getCCnum())) return false;

  tick.push_back(t);
  dataind.push_back(data.size());

  /* FIXME: Determine and encode time and tgt value */
  /* OR should it be done by the writer function?? */
  /* Doesn't really matter.. I'm re-doing this thing anyway.*/
  /* ... or am I?*/
  /* Must think this through at some point, yeah..
     time is ticks here, but seconds in the encoding, etc...*/
  /* FIXME: So far, I'll just make an instant change for each midi
     event*/

  float fval = range_min + (ev.getCCval()/127.f) * (range_max - range_min);
  float ftime = 0.004f;

  unsigned int inttime = synti2::encode_f(ftime);
  unsigned int intval = synti2::encode_f(fval);

  unsigned char buf[4];
  size_t len;

  len = encode_varlength(inttime, buf);
  for (size_t i=0;i<len;i++){
    data.push_back(buf[i]);
  }

  len = encode_varlength(intval, buf);
  //std::cout << "/*In goes: "; 
  //std::cout << synti2::decode_f(intval) << " ";
  for (size_t i=0;i<len;i++){
    //fmt_hexbyte(std::cout, buf[i]);
    //std::cout << " ";
    data.push_back(buf[i]);
  }
  //std::cout << "*/" << std::endl;

  return true;
}





synti2::MisssSong::MisssSong(MidiSong &midi_song, 
                     MidiMap &mapper, 
                     std::istream &spec)
{
  figure_out_tempo_from_midi(midi_song);
  build_chunks_from_spec(spec);
  translated_grab_from_midi(midi_song, mapper);
}

void 
synti2::MisssSong::figure_out_tempo_from_midi(MidiSong &midi_song){
  ticks_per_quarter = midi_song.getTPQ();
  usec_per_quarter = midi_song.getMSPQ();
}

void
synti2::MisssSong::build_chunks_from_spec(std::istream &spec)
{
  for (int i=0; i<9; i++){
    /* note ons: */
    chunks.push_back(new MisssNoteChunk(i, i, -1, 123, 1, 127));

    /* note offs: */
    //chunks.push_back(new MisssNoteChunk(i, i, -1, 0, 0, 0));
  }

  chunks.push_back(new MisssNoteChunk(0x9, 0x9, 35, 123, 1, 127));
  chunks.push_back(new MisssNoteChunk(0xa, 0xa, 39, 123, 1, 127));
  chunks.push_back(new MisssNoteChunk(0xb, 0xb, 41, 123, 1, 127));
  chunks.push_back(new MisssNoteChunk(0xc, 0xc, 41, 123, 1, 127));

#if 1
  for (int i=9; i<16; i++){
    /* note ons: */
    chunks.push_back(new MisssNoteChunk(i, i, -1, 123, 1, 127));
    /* no note offs, as can be seen :) */

  /* Continuous controllers: */
  /* FIXME: see that this works... */
  chunks.push_back(new MisssRampChunk(i, i, 0x01, 0x00, 
                                      0.0f, 1.0f));


  }
#endif


  /* FIXME: Implement. */
  std::cout << "/*FIXME: Cannot build from spec yet.*/ " << std::endl;
  std::string hmm;
  std::getline (spec, hmm);
  std::cerr << hmm;
}


void
synti2::MisssSong::translated_grab_from_midi(
      MidiSong &midi_song, 
      MidiMap &mapper)
{
    std::vector<unsigned int> orig_ticks;
    std::vector<MidiEvent> orig_evs;
    std::vector<unsigned int> ticks;
    std::vector<MisssEvent> evs;

    midi_song.linearize(orig_ticks, orig_evs);

    unsigned int i, ie;

    /* Filter: */
    for(i=0; i<orig_ticks.size(); i++){
      int t = orig_ticks[i];
      MidiEvent ev = orig_evs[i];

      // FIXME: This from MidiMap when it is implemented:
      std::vector<MisssEvent> mev = mapper.midiToMisss(ev);
      for(ie=0;ie<mev.size();ie++){
        ticks.push_back(t);
        evs.push_back(mev[i]);
      }
      
      /*std::cout << "  ";
        orig_evs[i].print(std::cout);
        std::cout << "->";
        evs[i].print(std::cout);*/
    }

    /* Collect: */
    for(i=0; i<ticks.size(); i++){
      miditick_t t = ticks[i];
      MisssEvent ev = evs[i];
      
      for(unsigned int c=0; c<chunks.size(); c++){
        if (chunks[c]->acceptEvent(t, ev)) break;
      }
    }
  }


void
synti2::MisssSong::write_as_c(std::ostream &outs){
  outs << "/*Song data generated by MisssSong */" << std::endl;
  outs << "#include \"synti2_misss.h\"" << std::endl;

  /*FIXME: Must name it differently soon, when it's not a hack anymore!*/
  outs << "unsigned char hacksong_data[] = {" << std::endl;

  outs << "/* *********** song header *********** */ " << std::endl;
  fmt_comment(outs, "Ticks per quarter");
  fmt_varlen(outs, ticks_per_quarter);
  outs << ", " << std::endl;
  fmt_comment(outs, "Microseconds per tick"); 
  fmt_varlen(outs, usec_per_quarter);
  outs << ", " << std::endl;

  outs << "/* *********** chunks *********** */ " << std::endl;

  for (unsigned int i=0; i<chunks.size(); i++){
    chunks.at(i)->write_as_c(outs);
  }

  outs << "/* *********** end *********** */ " << std::endl;
  outs << "/* End of data marker: */ 0x00};" << std::endl;
}
