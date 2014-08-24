/** Implementation of C++ wrappers for the internal message data
 *  structures of synti2. Only as much as the tool programs need, so
 *  far...
 */

#include "synti2_misss.h"
#include "synti2_limits.h"
#include "misss.hpp"
#include "midihelper.hpp"

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
void fmt_commented_fval(std::ostream &outs, 
                        std::string comm, float fval){
    unsigned int intval;
    unsigned char buf[8];
    intval = synti2::encode_f(fval);
    int len = encode_varlength(intval, buf);
    outs << "/* " << comm << " " << fval << "s */";
    for(int ib=0;ib<len;ib++){
      fmt_hexbyte(outs, buf[ib]);
      outs << ", ";
    }
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

synti2base::MisssEvent::MisssEvent(const unsigned char *misssbuf){
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

  /*std::cout << "MISSS-" << type << " on voice " << voice
    << " par1=" << par1;
  if (type==MISSS_MSG_NOTE) std::cout << " vel= " << par2 << std::endl;
  else std::cout << "time=" << time << " tgt=" << target<< std::endl;
  */
}

void
synti2base::MisssChunk::do_write_header_as_c(std::ostream &outs){
  outs << std::endl;
  outs << "/* CHUNK begins ----------------------- */ " << std::endl;
  fmt_comment(outs, "Number of events"); fmt_varlen(outs, tick.size());
  outs << ", " << std::endl;
  fmt_comment(outs, "Voice"); fmt_hexbyte(outs, voice);
  outs << ", " << std::endl;
}

void 
synti2base::MisssNoteChunk::do_write_header_as_c(std::ostream &outs)
{
  MisssChunk::do_write_header_as_c(outs);
  fmt_comment(outs, "Type"); outs << "MISSS_LAYER_NOTES";
  outs << ", " << std::endl;
  fmt_comment(outs, "Default note"); fmt_hexbyte(outs, computeDefaultNote());
  outs << ", " << std::endl;
  fmt_comment(outs, "Default velocity"); fmt_hexbyte(outs, computeDefaultVelocity());
  outs << ", " << std::endl;
}

void
synti2base::MisssNoteChunk::do_write_data_as_c(std::ostream &outs)
{
  outs << "/* delta and info : */ " << std::endl;
  unsigned int prev_tick=0;
  unsigned int prev_note=0;
  int defnote = computeDefaultNote();
  int defvel = computeDefaultVelocity();
  for (unsigned int i=0; i<tick.size(); i++){
    unsigned int delta = tick[i] - prev_tick; /* assume order */
    prev_tick = tick[i];
    fmt_varlen(outs, delta);
    outs << ", ";
    if (defnote < 0) {
      int this_note = evt[i].getNote();
      int delta = this_note - prev_note;
      prev_note = this_note;
      fmt_hexbyte(outs, delta);
      outs << ", ";
    }
    if (defvel < 0) {
      fmt_hexbyte(outs, evt[i].getVelocity());
      outs << ", ";
    }
    outs << std::endl;
  }
  outs << std::endl;
}


int
synti2base::MisssNoteChunk::computeDefaultNote()
{
  if (size()==0) return -1;
  int def = evt[0].getNote();
  for (int i=0;i<size();i++){
    if (evt[i].getNote() != def) return -1;
  }
  return def;
}

int
synti2base::MisssNoteChunk::computeDefaultVelocity()
{
  if (size()==0) return -1;
  int def = evt[0].getVelocity();
  for (int i=0;i<size();i++){
    if (evt[i].getVelocity() != def) return -1;
  }
  return def;
}


bool
synti2base::MisssNoteChunk::acceptEvent(unsigned int t, MisssEvent &ev)
{
  if (!ev.isNote()) return false;
  if (!voiceMatch(ev)) return false;
  if (! ((accept_vel_min <= ev.getVelocity()) 
         && (ev.getVelocity() <= accept_vel_max))) return false;

  tick.push_back(t);
  evt.push_back(ev);
  return true;
}

void 
synti2base::MisssRampChunk::do_write_header_as_c(std::ostream &outs)
{
  MisssChunk::do_write_header_as_c(outs);
  fmt_comment(outs, "Type"); outs << "MISSS_LAYER_CONTROLLER_RAMPS";
  outs << ", " << std::endl;
  fmt_comment(outs, "Synti2 controller number (zero-based)"); fmt_hexbyte(outs, getModNumber());
  outs << ", " << std::endl;
  fmt_comment(outs, "Unused parameter (dup)"); fmt_hexbyte(outs, getModNumber());
  outs << ", " << std::endl;
}

void
synti2base::MisssRampChunk::do_write_data_as_c(std::ostream &outs)
{
  outs << "/* delta and info : */ " << std::endl;
  unsigned int prev_tick=0;
  for (unsigned int i=0; i<tick.size(); i++){
    unsigned int delta = tick[i] - prev_tick; /* assume order */
    prev_tick = tick[i];
    fmt_varlen(outs, delta);
    outs << ", ";

    /* no earlier than here starts the differences to notes. Could
       localize. */
    fmt_commented_fval(outs, "time", evt[i].getTime());
    fmt_commented_fval(outs, "value", evt[i].getTarget());

    outs << std::endl;
  }
  outs << std::endl;
}

void 
synti2base::MisssRampChunk::optimize(std::vector<MisssChunk*> &extra, double ticklen)
{
  /* Idea: Ramp ends will be: extreme values where they occur, and
     heuristic "ends of continuous tweaks" elsewhere. Other events
     will be skipped. Naturally also the first touch must be there as
     an instantaneous transition.
  */
  if (size()==0) return;

  double time_gap_setting = 0.333; /* One third of a second,
                                      hattuvakio; should be param.*/

  /* Build a new copies, assign at end.*/
  std::vector<unsigned int> restick; 
  std::vector<MisssEvent> resevt; 

  restick.push_back(tick[0]);
  resevt.push_back(evt[0]);

  int voice = evt[0].getVoice();
  int mod = evt[0].getMod();
  unsigned int tick_start = 0;

  bool building = false;
  for(size_t i=1;i<tick.size();i++){
    /* Start building a new ramp, if previous is finished: */
    if (!building){
      building = true;
      tick_start = tick[i];
      //val_start = evt[i].getTarget();
      continue;
    }

    /* Ok, we're building the next ramp now. */
    bool extreme = false;
    bool rowlast = false;
    if (i == tick.size() - 1){
      extreme = true; /* Last of the whole chunk */
      rowlast = true; /* Last of the whole chunk */
    } else {
      float cval = evt[i].getTarget();
      float pval = evt[i-1].getTarget();
      float nval = evt[i+1].getTarget();
      if (((cval - pval) * (cval-nval)) > 0){
        extreme = true; /* Change of sign */
      }
      if (((tick[i+1] - tick[i]) * ticklen) > time_gap_setting){
        rowlast = true; /* Last in a "row". */
      }
    }

    double timetot = (tick[i] - tick_start) * ticklen;
    if (extreme && (!rowlast)) {
      /* Ramp up to here, but begin a new one straight away. */
      MisssEvent ev(MISSS_MSG_RAMP, 
                    voice, mod, timetot, evt[i].getTarget());
      restick.push_back(tick_start);
      resevt.push_back(ev);
      /* New one starts here: */
      tick_start = tick[i];
      building = true;
    }
    if (rowlast) {
      /* Ramp up to here, and stop building. */
      MisssEvent ev(MISSS_MSG_RAMP, 
                    voice, mod, timetot, evt[i].getTarget());
      restick.push_back(tick_start);
      resevt.push_back(ev);
      /* Stop accumulation until further notice: */
      building = false;
    }
    /* otherwise, we'll just keep on eating new events.. .*/
  }

  this->tick = restick;
  this->evt = resevt;
}

bool
synti2base::MisssRampChunk::acceptEvent(unsigned int t, MisssEvent &ev)
{
  if (!ev.isRamp()) return false;
  if (!voiceMatch(ev)) return false;
  if (!(mod == ev.getMod())) return false;

  tick.push_back(t);
  evt.push_back(ev);
  return true;
/* 
  FIXME: So far, these go unchanged all the way.  .. must re-engineer
     this soon, somehow.  Must think this through at some point,
     yeah..  time is ticks here, but seconds in the encoding, etc...
*/
}



synti2base::MisssSong::MisssSong(MidiSong &midi_song, 
                     MidiMap &mapper, 
                     std::istream &spec)
{
  figure_out_tempo_from_midi(midi_song);
  build_chunks_from_spec(spec);
  translated_grab_from_midi(midi_song, mapper);
}

void 
synti2base::MisssSong::figure_out_tempo_from_midi(MidiSong &midi_song){
  ticks_per_quarter = midi_song.getTPQ();
  usec_per_quarter = midi_song.getMSPQ();
}

void
synti2base::MisssSong::build_chunks_from_spec(std::istream &spec)
{
  for (int i=0; i<NUM_MAX_CHANNELS; i++){
    /* note ons: */
    chunks.push_back(new MisssNoteChunk(i, 1, 127));
    /* note offs: */
    chunks.push_back(new MisssNoteChunk(i, 0, 0));
    /* modulators: */
    for (int im=0;im<NUM_MAX_MODULATORS; im++){
      chunks.push_back(new MisssRampChunk(i, im));
    }
  }

  /* FIXME: Implement. -> What? */
  std::cout << "/*FIXME: Cannot build from spec yet.*/ " 
            << std::endl;
  std::cout << "/* (I wonder what I mean by that, anyway..) */ " 
            << std::endl;
  std::string hmm;
  std::getline (spec, hmm);
  std::cerr << hmm << std::endl;
}


void
synti2base::MisssSong::translated_grab_from_midi(
      MidiSong &midi_song, 
      MidiMap &mapper)
{
    std::vector<unsigned int> orig_ticks;
    std::vector<MidiEvent> orig_evs;
    midi_song.linearize(orig_ticks, orig_evs);

    /* Filter and collect: */
    for(size_t i=0; i<orig_ticks.size(); i++){
      int t = orig_ticks[i];
      MidiEvent ev = orig_evs[i];
      if (!(ev.isNote() || ev.isCC() || ev.isBend())) continue;
      //ev.print(std::cout);
      //std::cout << "Tick " << t << " " << chunks.size() << " ... ";

      std::vector<MisssEvent> mev = mapper.midiToMisss(ev);
      //std::cout << "Translated to " << mev.size() << " MISSS events" << std::endl;

      for(size_t ie=0;ie<mev.size();ie++){
        for(unsigned int c=0; c<chunks.size(); c++){
          if (chunks[c]->acceptEvent(t, mev[ie])) {
            //std::cout << "Accept on chunk " << c << std::endl;
            break;
          }
        }
      }
    }

    /* FIXME: Implement: Post-process chunks (decimate, optimize,
       etc.) */
    double ticklen = (double)usec_per_quarter / ticks_per_quarter 
      / 1000000.0;
    size_t norig = chunks.size();
    for(size_t i = 0; i<norig; i++){
      chunks[i]->optimize(chunks, ticklen); /* chunks may grow. ugly, yeah.*/
    }
  }


void
synti2base::MisssSong::write_as_c(std::ostream &outs){
  outs << "/*Song data generated by MisssSong */" << std::endl;
  outs << "#include \"synti2_misss.h\"" << std::endl;

  /*FIXME: Some configuration for name and static/nonstatic?*/
  outs << "static ";
  /*FIXME: Must name it differently soon, when it's not a hack anymore!*/
  outs << "unsigned char hacksong_data[] = {" << std::endl;

  outs << "/* *********** song header *********** */ " << std::endl;
  fmt_comment(outs, "Ticks per quarter");
  fmt_varlen(outs, ticks_per_quarter);
  outs << ", " << std::endl;
  fmt_comment(outs, "Microseconds per quarter"); 
  fmt_varlen(outs, usec_per_quarter);
  outs << ", " << std::endl;

  outs << "/* *********** chunks *********** */ " << std::endl;

  for (unsigned int i=0; i<chunks.size(); i++){
    chunks[i]->write_as_c(outs);
  }

  outs << "/* *********** end *********** */ " << std::endl;
  outs << "/* End of data marker: */ 0x00};" << std::endl;
}
