/** @file synti2_midi.c - Conversion from MIDI messages to MISSS
 * (Midi-like Interface for the Synti Software Synthesizer) messages.
 *
 * The same module can be used with whatever MIDI interface (JACK,
 * ALSA, VST..) in real time, or off-line when converting an SMF song
 * to our own, stripped-down format.
 *
 * The conversion is configured using certain SysEx commands that must
 * be created with a suitable editor program. (TODO: Write out a
 * proper specification of the SysEx messages that control the midimap
 * structure).
 *
 */

#include "synti2_midi.h"
#include "synti2_midi_guts.h"
#include "synti2_misss.h"
#include "midi_spec.h"

#include "stdio.h"

#define INSTANT_RAMP_LENGTH 0.005f

/** Map MIDI controller value ccval to a synti2 parameter value. */
static
float
synti2_misss_mapBendValue(int bendval){
  return (bendval - 0x2000)/(float)0x2000;
}

/** Map MIDI controller value ccval to a synti2 parameter value. 
 *
 * {0..127}->[a,b]
 *
 */

static
float
synti2_misss_mapControlValue(synti2_midi_map *map, int ic, int dest, byte_t ccval){
  float fa, fb, frange;
  if (dest==0) return 0.0f;
  fa = map->chn[ic].mod_min[dest-1];
  fb = map->chn[ic].mod_max[dest-1];
  frange = fb - fa;
  return fa + ((float)ccval / 127.f) * frange;
}



/** Creates a MISSS note message; returns length of the
 *  message. Output buffer must have enough space. Currently, the
 *  length is 4.
 */
static
int
synti2_misss_note(
                  byte_t *misss_out, 
                  byte_t misss_chn, 
                  byte_t midi_note,
                  byte_t midi_vel){
  *misss_out++ = MISSS_MSG_NOTE;
  *misss_out++ = misss_chn;
  *misss_out++ = midi_note;
  *misss_out++ = midi_vel;
  return 4;
}

#if 0
/** Creates a MISSS "F-value" message; returns length of the
 *  message. Output buffer must have enough space. Currently, the
 *  length is 3 + sizeof(float). Is this used anywhere anymore,
 *  actually?
 */
static
int
synti2_misss_setf(byte_t *misss_out, 
                  byte_t misss_chn, 
                  byte_t fval_index,
                  float fval){
  *misss_out++ = MISSS_MSG_SETF;
  *misss_out++ = misss_chn; /* TODO: Channel dispersion logic in caller?*/
  *misss_out++ = fval_index;
  *(float*)misss_out = fval;
  return 3+sizeof(float); /* A native float floats out. */
}
#endif

/** Creates a MISSS "Controller ramp" message; returns length of the 
 * message. Output buffer must have enough space. Currently, the 
 * length is 3 + 2 * sizeof(float).
 */
static
int
synti2_misss_ramp(byte_t *misss_out, 
                  byte_t misss_voice, 
                  byte_t cont_index,
                  float time,
                  float dest_val){
  *misss_out++ = MISSS_MSG_RAMP;
  *misss_out++ = misss_voice;
  *misss_out++ = cont_index;
  *(float*)misss_out = time;
  misss_out += sizeof(float);
  *(float*)misss_out = dest_val;
  /*printf("Ramp on %d:%d to %.3f in %.4fs\n",misss_voice,cont_index,dest_val,time);fflush(stdout);*/
  return 3+2*sizeof(float); /* Two native floats floats out. */
}


/** As of now, this just forwards the bulk of a MIDI SysEx, replacing
 *  the SysEx header with a MISSS header. Expects input_size to be the
 *  number of bytes to be forwarded.
 */
static
int
synti2_misss_data(const byte_t *midi_in, 
                  byte_t *misss_out, 
                  int input_size){
  int i;
  /* byte_t *p = misss_out;  */
  for (i=0; i<input_size; i++){
    *misss_out++ = *midi_in++;
  }
  /*printf("Sending %d bytes %02x %02x %02x %02x \n",  input_size, 
    (int)p[0], (int)p[1],(int)p[2],(int)p[3]);*/
  return input_size+1;
}


/**
 * Converts MIDI System messages. Assumes midi_status to be between F0
 * and FF (which means a System Message).
 *
 *
 * FIXME: Manufacturer ID check.. Should also check that there is F7
 *  in the end :) length check; checksums :) could (and should) have
 *  checks now that this code is moved outside the stand-alone synth
 *  and thus is not size critical anymore..
 */
static
int
synti2_sysmsg_to_misss(byte_t midi_status, 
                       const byte_t *midi_in, 
                       byte_t *misss_out, 
                       int input_size)
{
  switch(midi_status){
  case 0xf0:
    /* Sysex header: */
    midi_in += 3; /* skip Manufacturer IDs n stuff TODO: think about this */
    return synti2_misss_data(midi_in, misss_out, input_size-3);
  default:
    /* Other System Messages are swallowed by void, as of now.*/
    /*printf("Unhandled System Message type! \n");*/
    return 0;
  }  
}

static
void
intercept_noff(synti2_midi_map *map,
               const byte_t *midi_in)
{
  int ic = midi_in[0];
  int value = midi_in[1];
  map->chn[ic].receive_note_off = value;
}

/* Could be "intercept single byte"!*/
static
void
intercept_mode(synti2_midi_map *map,
               const byte_t *midi_in)
{
  int ic = midi_in[0];
  int value = midi_in[1];
  int ii;
  map->chn[ic].mode = value;

  /* FIXME: Should be done in intercept_voices, but think about default settings in that case. */
  for(ii=0;(ii<NPARTS) && (map->chn[ic].voices[ii] > 0);){ii++;}
  map->chn[ic].nvoices = ii;
}

static
void
intercept_map_single(synti2_midi_map *map,
                    const byte_t *midi_in)
{
  int ic = midi_in[0];
  int inote = midi_in[1];
  int value = midi_in[2];
  /* Values need -1 to be actual 0-based indices */
  if (value > NPARTS) value = 0; /* Zero if illegal. */
  map->chn[ic].note_channel_map[inote] = value;
}

static
void
intercept_map_all(synti2_midi_map *map,
                  const byte_t *midi_in)
{
  int ic = *midi_in++;
  int inote,value;
  for(inote=0;inote<128;inote++){
    value = midi_in[inote];
    /* Values need -1 to be actual 0-based indices */
    if (value > NPARTS) value = 0; /* Zero if illegal. */
    map->chn[ic].note_channel_map[inote] = value;
  }
}

static
void
intercept_voices(synti2_midi_map *map,
                 const byte_t *midi_in)
{
  int ic, ii, voice;
  ic = *midi_in++;
  map->chn[ic].nvoices = 0;
  for(ii=0;ii<NPARTS;ii++){
    voice = midi_in[ii];
    if (voice>NPARTS) voice = 0;
    map->chn[ic].voices[ii] = voice;
    if (voice==0) break;
    else map->chn[ic].nvoices++;
  }
}

static
void
intercept_bend_dest(synti2_midi_map *map, 
                    const byte_t *midi_in)
{
  int ic, dest;
  ic = *midi_in++;
  dest = *midi_in++;
  map->chn[ic].bend_destination = dest;
}

static
void
intercept_pressure_dest(synti2_midi_map *map, 
                        const byte_t *midi_in)
{
  int ic, dest;
  ic = *midi_in++;
  dest = *midi_in++;
  map->chn[ic].pressure_destination = dest;
}

static
void
intercept_const_velocity(synti2_midi_map *map, 
                         const byte_t *midi_in)
{
  int ic, cvelo;
  ic = *midi_in++;
  cvelo = *midi_in++;
  map->chn[ic].use_const_velocity = cvelo;
}

/* FIXME: Decode is a copy-paste from synti2.c - should refactor maybe*/

/* TODO: This too much of a hack? */
static
#include "synti2_fdec.c"

static
void
decode7b4(const unsigned char * source, unsigned int * dest){
  *dest = 
    ((source[0] & 0x7f) << 21)
    + ((source[1] & 0x7f) << 14)
    + ((source[2] & 0x7f) << 7)
    + ((source[3] & 0x7f) << 0);
}

static
float
float_from_sysex(const byte_t *source){
  unsigned int intval;
  decode7b4(source, &intval);
  return synti2_decode_f(intval);
}

static
void
intercept_mod_params(synti2_midi_map *map, 
                     const byte_t *midi_in)
{
  int ic, imod, modsource;
  float fa, fb;
  ic = *midi_in++;
  imod = *midi_in++;
  modsource = *midi_in++;
  fa = float_from_sysex(midi_in);
  fb = float_from_sysex(midi_in+4);
  map->chn[ic].mod_src[imod] = modsource;
  map->chn[ic].mod_min[imod] = fa;
  map->chn[ic].mod_max[imod] = fb;
}


static
int
intercept_mapper_msg(synti2_midi_map *map,
                     byte_t midi_status, 
                     const byte_t *midi_in)
{
  int cmd;
  if (midi_status != 0xf0) return 0;
  
  /* Sysex header: */
  midi_in += 3; /* skip Manufacturer IDs n stuff TODO: think about this */
  cmd = *midi_in++;
  switch (cmd){
  case MISSS_SYSEX_MM_SUST:
    /*intercept_sust(map, midi_in);*/
    return 1;
  case MISSS_SYSEX_MM_MODE:
    intercept_mode(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_NOFF:
    intercept_noff(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_CVEL:
    intercept_const_velocity(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_VOICES:
    intercept_voices(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_MAPSINGLE:
    intercept_map_single(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_MAPALL:
    intercept_map_all(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_BEND:
    intercept_bend_dest(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_PRESSURE:
    intercept_pressure_dest(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_MODDATA:
    intercept_mod_params(map, midi_in);
    return 1;
  case MISSS_SYSEX_MM_RAMPLEN:
    /*intercept_ramplen(map, midi_in);*/
    return 1;
  default:
    /* Unknown/unimplemented message type. Should be an error, maybe. */
    return 0;
  }
}


static
int
synti2_map_note_off(synti2_midi_map *map, 
                    synti2_midi_state *state, 
                    int ic,
                    const byte_t *midi_in,
                    byte_t *misss_out,
                    int *msgsizes)
{
  int ii=0;
  int voice, note;
  if (map->chn[ic].receive_note_off == 0) return 0;

  note = *midi_in;

  /* Solo/unison mode: */
  if (map->chn[ic].mode == MM_MODE_DUP){
    /* Swallow note-off if it isn't for previous on. */
    if (note != state->chn[ic].prev_note){
      return 0;
    }
    /* Create unison duplicates on the listed voices: */
    for (ii=0;ii < NPARTS; ii++){
      voice = map->chn[ic].voices[ii];
      if (voice==0) break;
      voice--; /* adjust index to base 0*/
      
      msgsizes[ii] = synti2_misss_note(misss_out, voice, note, 0);
      misss_out += msgsizes[ii];
    }
    return ii;
  } else if (map->chn[ic].mode == MM_MODE_POLYROT){
    /* Poly rotate: note off to the corresponding on-channel. */
    voice = state->chn[ic].ons[note];
    if (note != state->chn[ic].notes[voice]){
      /* There has been a new, different, note on for this voice! This
       * out-dated note off must be swallowed. Necessary in live
       * jamming, but basically this is an overload w.r.t. capacities,
       * and probably it should be notified as an unwanted/error
       * situation, and gotten rid of by the composer(?)..
       */
      state->chn[ic].ons[note] = -1;
      return 0;
    }
    state->chn[ic].ons[note] = -1;
    msgsizes[0] = synti2_misss_note(misss_out, voice-1, note, 0);
    return 1;
  } else if (map->chn[ic].mode == MM_MODE_MAPPED){
    /* Look-up from the channel map. */
    voice = map->chn[ic].note_channel_map[note];
    if (voice==0) return 0;
    voice--; /* adjust index to base 0*/
    msgsizes[0] = synti2_misss_note(misss_out, voice, note, 0);
    return 1;
  }
  return 0;
}

static
int
synti2_map_note_on(synti2_midi_map *map, 
                   synti2_midi_state *state, 
                   int ic,
                   const byte_t *midi_in,
                   byte_t *misss_out,
                   int *msgsizes)
{
  int ii=0, midi_note, midi_vel, voice;
  int iv;
  midi_note = *midi_in++;
  midi_vel = *midi_in++;
  
  if (map->chn[ic].mode == MM_MODE_DUP){
    /* Create unison duplicates on the listed voices: */
    for (ii=0;ii < NPARTS; ii++){
      voice = map->chn[ic].voices[ii];
      if (voice==0) break;
      msgsizes[ii] = synti2_misss_note(misss_out, voice-1, midi_note, midi_vel);
      misss_out += msgsizes[ii];
    }
    state->chn[ic].prev_note = midi_note;
    return ii;
  } else if (map->chn[ic].mode == MM_MODE_POLYROT){
    /* Polyphony by rotating through the voice list. */
    iv = state->chn[ic].rot.inxt;
    voice = map->chn[ic].voices[iv];
    /* Remember state, to handle note-offs:*/
    state->chn[ic].ons[midi_note] = voice;
    state->chn[ic].notes[voice] = midi_note;
    /* Update rotation: */
    iv = (iv + 1) % map->chn[ic].nvoices;
    state->chn[ic].rot.inxt = iv;
    /* Send the actual misss msg: */
    msgsizes[0] = synti2_misss_note(misss_out, voice-1, midi_note, midi_vel);
    return 1;
  } else if (map->chn[ic].mode == MM_MODE_MAPPED){
    /* Look-up from the channel map. */
    voice = map->chn[ic].note_channel_map[midi_note];
    if (voice==0) return 0;
    msgsizes[0] = synti2_misss_note(misss_out, voice-1, midi_note, midi_vel);
    return 1;
  }

  return 0;
}

static
int
synti2_yield_mod_ramps(synti2_midi_map *map, 
                       int ic,
                       int dest,
                       float value,
                       byte_t *misss_out,
                       int *msgsizes)
{
  int ivoi, voice;
  if (dest==0) return 0;
  for(ivoi = 0; ivoi < NPARTS; ivoi++){
    voice = map->chn[ic].voices[ivoi];
    if (voice == 0) break;
    msgsizes[ivoi] = synti2_misss_ramp(misss_out, voice-1, dest-1,
                                       INSTANT_RAMP_LENGTH, value);
  }
  return ivoi;
}

static
int
get_cont_dest(synti2_midi_map *map,
              int ic,
              int midi_ccnum)
{
  int dest, ii;
  dest = 0;
  for (ii = 0; ii<NCONTROLLERS; ii++){
    if (map->chn[ic].mod_src[ii] == 0) continue; /* omit Bank. */
    /* First actual possible CC is #1 ("mod wheel") */
    /* +1 because the mapper has "user interface" value range*/
    if ((midi_ccnum) == map->chn[ic].mod_src[ii]){
      dest = ii+1;
      /*printf("to %d \n",dest); fflush(stdout);*/
      break;
    }
  }
  return dest;
}

static
int
synti2_map_cc(synti2_midi_map *map, 
              int ic,
              const byte_t *midi_in,
              byte_t *misss_out,
              int *msgsizes)
{
  int dest, midi_ccnum, midi_ccval;
  float value;
  midi_ccnum = midi_in[0];
  midi_ccval = midi_in[1];
  dest = get_cont_dest(map,ic,midi_ccnum);
  /* TODO: Could use LSB as well? Won't bother for now.. needs GUI, storage, and all that...*/
  value = synti2_misss_mapControlValue(map, ic, dest, midi_ccval);
  return synti2_yield_mod_ramps(map, ic, dest, value, misss_out, msgsizes);
}

static
int
synti2_map_bend(synti2_midi_map *map, 
                int ic,
                const byte_t *midi_in,
                byte_t *misss_out,
                int *msgsizes)
{
  int dest, midi_bendval;
  float value;
  /* Pitch wheel is translated into a continuous controller */
  dest = map->chn[ic].bend_destination;
  midi_bendval = (midi_in[1] << 7) + midi_in[0];
  value = synti2_misss_mapBendValue(midi_bendval);
  return synti2_yield_mod_ramps(map, ic, dest, value, misss_out, msgsizes);
}


static
int
synti2_map_pressure(synti2_midi_map *map, 
                    int ic,
                    const byte_t *midi_in,
                    byte_t *misss_out,
                    int *msgsizes)
{
  int dest, midi_pressure;
  float value;
  /* Pitch wheel is translated into a continuous controller */
  dest = map->chn[ic].pressure_destination;
  midi_pressure = midi_in[0];
  value = synti2_misss_mapControlValue(map, ic, dest, midi_pressure);
  return synti2_yield_mod_ramps(map, ic, dest, value, misss_out, msgsizes);
}

  
/**
 * Converts a MIDI message (complete; no running status) into a set of
 * MISSS messages that the synti2 synthesizer can handle. Applies midi
 * mapping and filtering, as per current settings in the synth
 * object. The state and settings are changed according to the
 * incoming MIDI message stream.
 *
 * Returns the number of output MISSS messages. The message bodies
 * will be in the output byte buffer and the lengths of the messages
 * will be in the integer array. Caller must make sure that there is
 * enough space in the output buffers. (TODO: document the maximum
 * message size increase; this is a function of maximum polyphony, I
 * suppose.)
 *
 */
int
synti2_midi_to_misss(synti2_midi_map *map,
                     synti2_midi_state *state,
                     const byte_t *midi_in,
                     byte_t *misss_out,
                     int *msgsizes,
                     int input_size)
{
  byte_t midi_status, midi_chn;

  midi_status = *midi_in++;
  midi_chn = midi_status & 0x0f;

  switch(midi_status >> 4){
  case MIDI_STATUS_NOTE_OFF:
    return synti2_map_note_off(map, state, midi_chn, midi_in, misss_out, msgsizes);
  case MIDI_STATUS_NOTE_ON:
    return synti2_map_note_on(map, state, midi_chn, midi_in, misss_out, msgsizes);
  case MIDI_STATUS_KEY_PRESSURE:
    return 0; /* Key pressure omitted. TODO: handle somehow? */ 
  case MIDI_STATUS_CONTROL:
    return synti2_map_cc(map, midi_chn, midi_in, misss_out, msgsizes);
  case MIDI_STATUS_PROGRAM:
    return 0; /* Omit program change. TODO: a sound bank system(?) */
  case MIDI_STATUS_CHANNEL_PRESSURE:
    return synti2_map_pressure(map, midi_chn, midi_in, misss_out, msgsizes);
  case MIDI_STATUS_PITCH_WHEEL:
    return synti2_map_bend(map, midi_chn, midi_in, misss_out, msgsizes);
  case MIDI_STATUS_SYSTEM:
    if (intercept_mapper_msg(map, midi_status, midi_in)){
      return 0;
    } else {
      msgsizes[0] = synti2_sysmsg_to_misss(midi_status, midi_in, 
                                           misss_out, input_size-1);
      return 1;
    }
  default:
    return 0;
  }
  return 0;
}
