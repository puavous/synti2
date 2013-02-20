/** @file synti2_midi.c - Conversion from MIDI messages to MISSS
 * (Midi-like Interface for the Synti Software Synthesizer) messages.
 *
 * The same module can be used with whatever MIDI interface (ALSA,
 *  VST..).
 *
 *
 * FIXME: This could be a place also for synti2 channel allocation?
 * Maybe? But that needs a bigger revamp of the tool-engine-interface
 * responsibility share? Yes. See the module diagram. This is to be
 * controlled via SysEx (for which there will be an editor program,
 * similar to the patch editor).
 *
 */

#include "synti2.h"
#include "synti2_guts.h"
#include "synti2_midi.h"
#include "synti2_midi_guts.h"
#include "synti2_misss.h"
#include "synti2_params.h"
#include "midi_spec.h"

#define INSTANT_RAMP_LENGTH 0.005f

/** Map MIDI controller number ccn to a synti2 parameter number. */
static
byte_t
synti2_misss_mapBendDest(){
  return 3; /* FIXME: Always cont. controller #3*/
}

/** Map MIDI controller value ccval to a synti2 parameter value. */
static
float
synti2_misss_mapBendValue(int bendval){
  return (bendval - 0x2000)/(float)0x2000;
}

/** Map MIDI controller number ccn to a synti2 controller number. */
/* FIXME: Should be based on channel number, too */
static
byte_t
synti2_misss_mapControlDest(byte_t ccn){
  return 0; /* FIXME: Always the first controller. */
}

/** Map MIDI controller value ccval to a synti2 parameter value. 

This is a proper place for this old fixme from patchdesign.dat:

# Controller mins and maxes .. NO.... 
#
# FIXME: These should be auxiliary (in the same place with channel
# allocation)
#
# CMIN1 Cont1Min -2.0 +2.0   2 21
# CMIN2 Cont2Min -4.0 +4.0   1 21
# CMIN3 Cont3Min -8.0 +8.0   1 21
# CMIN4 Cont4Min -16.0 +16.0 1 21
#
# CMAX1 Cont1Max -2.0 +2.0   2 22
# CMAX2 Cont2Max -4.0 +4.0   1 22
# CMAX3 Cont3Max -8.0 +8.0   1 22
# CMAX4 Cont4Max -16.0 +16.0 1 22


*/
static
float
synti2_misss_mapControlValue(byte_t ccval){
  float fmin = 0.0f;
  float frange = 1.0f; /*FIXME: Always range [0,1]*/
  return fmin + ((float)ccval / 127.f) * frange;
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

/** Creates a MISSS "F-value" message; returns length of the
 *  message. Output buffer must have enough space. Currently, the
 *  length is 3 + sizeof(float).
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

/** Creates a MISSS "Controller ramp" message; returns length of the 
 * message. Output buffer must have enough space. Currently, the 
 * length is 3 + 2 * sizeof(float).
 */
static
int
synti2_misss_ramp(byte_t *misss_out, 
                  byte_t misss_chn, 
                  byte_t cont_index,
                  float time,
                  float dest_val){
  *misss_out++ = MISSS_MSG_RAMP;
  *misss_out++ = misss_chn; /* TODO: Channel dispersion logic in caller?*/
  *misss_out++ = cont_index;
  *(float*)misss_out = time;
  misss_out += sizeof(float);
  *(float*)misss_out = dest_val;
  return 3+2*sizeof(float); /* Two native floats floats out. */
}


/** As of now, this just forwards the bulk of a MIDI SysEx, replacing
 *  the SysEx header with a MISSS header. Expects input_size to be the
 *  number of bytes to be forwarded.
 */
static
int
synti2_misss_data(byte_t *midi_in, 
                  byte_t *misss_out, 
                  int input_size){
  int i;
  byte_t *p = misss_out;
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
                       byte_t *midi_in, 
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
intercept_noff(synti2_synth *s,
               byte_t *midi_in)
{
  int ic = midi_in[0];
  int value = midi_in[1];
  s->midimap.chn[ic].receive_note_off = value;
}

/* Could be "intercept single byte"!*/
static
void
intercept_mode(synti2_synth *s,
               byte_t *midi_in)
{
  int ic = midi_in[0];
  int value = midi_in[1];
  int ii;
  s->midimap.chn[ic].mode = value;

  /* FIXME: Should be done in intercept_voices, but think about default settings in that case. */
  for(ii=0;(ii<NPARTS) && (s->midimap.chn[ic].voices[ii] > 0);){ii++;}
  s->midimap.chn[ic].nvoices = ii;
}

static
void
intercept_mapsingle(synti2_synth *s,
                    byte_t *midi_in)
{
  int ic = midi_in[0];
  int inote = midi_in[1];
  int value = midi_in[2];
  /* Values need -1 to be actual 0-based indices */
  if (value > NPARTS) value = 0; /* Zero if illegal. */
  s->midimap.chn[ic].note_channel_map[inote] = value;
}

static
void
intercept_voices(synti2_synth *s,
                 byte_t *midi_in)
{
  int ic = *midi_in++;
  int ii, voice;
  s->midimap.chn[ic].nvoices = 0;
  for(ii=0;ii<NPARTS;ii++){
    voice = midi_in[ii];
    if (voice>NPARTS) voice = 0;
    s->midimap.chn[ic].voices[ii] = voice;
    if (voice==0) break;
    else s->midimap.chn[ic].nvoices++;
  }
}




static
int
intercept_mapper_msg(synti2_synth *s,
                     byte_t midi_status, 
                     byte_t *midi_in, 
                     int input_size)
{
  int cmd;
  if (midi_status != 0xf0) return 0;
  
  /* Sysex header: */
  midi_in += 3; /* skip Manufacturer IDs n stuff TODO: think about this */
  cmd = *midi_in++;
  switch (cmd){
  case MISSS_SYSEX_MM_SUST:
    return 1;
  case MISSS_SYSEX_MM_MODE:
    intercept_mode(s, midi_in);
    return 1;
  case MISSS_SYSEX_MM_NOFF:
    intercept_noff(s, midi_in);
    return 1;
  case MISSS_SYSEX_MM_CVEL:
    return 1;
  case MISSS_SYSEX_MM_VOICES:
    intercept_voices(s, midi_in);
    return 1;
  case MISSS_SYSEX_MM_MAPSINGLE:
    intercept_mapsingle(s, midi_in);
    return 1;
  case MISSS_SYSEX_MM_MAPALL:
    return 1;
  case MISSS_SYSEX_MM_BEND:
    return 1;
  case MISSS_SYSEX_MM_PRESSURE:
    return 1;
  case MISSS_SYSEX_MM_MODDATA:
    return 1;
  case MISSS_SYSEX_MM_RAMPLEN:
    return 1;
  }
  return 0;
}


static
int
synti2_map_note_off(synti2_synth *s, 
                    int ic,
                    byte_t *midi_in,
                    byte_t *misss_out,
                    int *msgsizes)
{
  int ii=0;
  int voice, note;
  if (s->midimap.chn[ic].receive_note_off == 0) return 0;

  note = *midi_in;

  /* Solo/unison mode: */
  if (s->midimap.chn[ic].mode == MM_MODE_DUP){
    /* Swallow note-off if it isn't for previous on. */
    if (note != s->midistate.chn[ic].prev_note){
      return 0;
    }
    /* Create unison duplicates on the listed voices: */
    for (ii=0;ii < NPARTS; ii++){
      voice = s->midimap.chn[ic].voices[ii];
      if (voice==0) break;
      voice--; /* adjust index to base 0*/
      
      msgsizes[ii] = synti2_misss_note(misss_out, voice, note, 0);
      misss_out += msgsizes[ii];
    }
    return ii;
  } else if (s->midimap.chn[ic].mode == MM_MODE_MAPPED){
    /* Look-up from the channel map. */
    voice = s->midimap.chn[ic].note_channel_map[note];
    if (voice==0) return 0;
    voice--; /* adjust index to base 0*/
    msgsizes[0] = synti2_misss_note(misss_out, voice, note, 0);
    return 1;
  }
  return 0;
}

static
int
synti2_map_note_on(synti2_synth *s, 
                    int ic,
                    byte_t *midi_in,
                    byte_t *misss_out,
                    int *msgsizes)
{
  int ii=0, midi_note, midi_vel, voice;
  int iv;
  midi_note = *midi_in++;
  midi_vel = *midi_in++;
  
  if (s->midimap.chn[ic].mode == MM_MODE_DUP){
    /* Create unison duplicates on the listed voices: */
    for (ii=0;ii < NPARTS; ii++){
      voice = s->midimap.chn[ic].voices[ii];
      if (voice==0) break;
      msgsizes[ii] = synti2_misss_note(misss_out, voice-1, midi_note, midi_vel);
      misss_out += msgsizes[ii];
    }
    s->midistate.chn[ic].prev_note = midi_note;
    return ii;
  } else if (s->midimap.chn[ic].mode == MM_MODE_POLYROT){
    /* Polyphony by rotating through the voice list. */
    iv = s->midistate.chn[ic].rot.inxt;
    voice = s->midimap.chn[ic].voices[iv];
    inxt = (inxt + 1) % s->midimap.chn[ic].nvoices;
    s->midistate.chn[ic].rot.inxt = inxt;

    msgsizes[0] = synti2_misss_note(misss_out, voice-1, midi_note, midi_vel);
    return 1;
  } else if (s->midimap.chn[ic].mode == MM_MODE_MAPPED){
    /* Look-up from the channel map. */
    voice = s->midimap.chn[ic].note_channel_map[midi_note];
    if (voice==0) return 0;
    msgsizes[0] = synti2_misss_note(misss_out, voice-1, midi_note, midi_vel);
    return 1;
  }

  return 0;
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
synti2_midi_to_misss(synti2_synth *s,
                     const byte_t *midi_in, 
                     byte_t *misss_out, 
                     int *msgsizes,
                     int input_size)
{
  byte_t midi_status;
  byte_t midi_chn;
  byte_t midi_vel = 0;
  byte_t midi_note = 0;
  byte_t midi_ccnum = 0;
  byte_t midi_ccval = 0;
  int midi_bendval = 0x2000;
  int ii, voice;

  midi_status = *midi_in++;
  midi_chn = midi_status & 0x0f;

  switch(midi_status >> 4){
  case MIDI_STATUS_NOTE_OFF:
    return synti2_map_note_off(s, midi_chn, midi_in, misss_out, msgsizes);
  case MIDI_STATUS_NOTE_ON:
    return synti2_map_note_on(s, midi_chn, midi_in, misss_out, msgsizes);
  case MIDI_STATUS_KEY_PRESSURE:
    /* Key pressure becomes channel pressure (no polyphonic pressure). */
    /* return synti2_misss_control(misss_out, midi_chn, 
       synti2_misss_mapPressureDest(), synti2_misss_mapPressureValue());*/
    return 0;
  case MIDI_STATUS_CONTROL:
    /* TODO: So far, we do nothing to Channel Mode Messages. */
    midi_ccnum = *midi_in++;
    midi_ccval = *midi_in++;
    msgsizes[0] = synti2_misss_ramp(misss_out, midi_chn, 
                             synti2_misss_mapControlDest(midi_ccnum), 
                             INSTANT_RAMP_LENGTH,
                             synti2_misss_mapControlValue(midi_ccval));
    return 1;
  case MIDI_STATUS_PROGRAM:
    /* Omit program change. Could have some sound bank logic... */
    return 0;
  case MIDI_STATUS_CHANNEL_PRESSURE:
    /* TODO: Should be mapped to a controller. */
    /* return synti2_misss_control(misss_out, midi_chn, 
       synti2_misss_mapPressureDest(), synti2_misss_mapPressureValue());*/
    return 0;
  case MIDI_STATUS_PITCH_WHEEL:
    /* Pitch wheel is translated into a continuous controller */
    midi_bendval = (midi_in[1] << 7) + midi_in[0];
    msgsizes[0] = synti2_misss_ramp(misss_out, midi_chn, 
                             synti2_misss_mapBendDest(midi_chn), 
                             INSTANT_RAMP_LENGTH,
                             synti2_misss_mapBendValue(midi_bendval));
    return 1;
  case MIDI_STATUS_SYSTEM:
    if (intercept_mapper_msg(s, midi_status, midi_in, input_size-1)){
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
