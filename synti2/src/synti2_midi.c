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
#include "synti2_midi.h"
#include "synti2_misss.h"
#include "synti2_params.h"

#define INSTANT_RAMP_LENGTH 0.005f

/** Map MIDI controller number ccn to a synti2 parameter number. */
static
byte_t
synti2_misss_mapBendDest(){
  return SYNTI2_F_PBVAL;
}

/** Map MIDI controller value ccval to a synti2 parameter value. */
static
float
synti2_misss_mapPitchValue(int bendval){
  float frange = 1.0f; /* Range is now determined by the patch. */
  return (bendval - 0x2000)/(float)0x2000 * frange;
}

/** Map MIDI controller number ccn to a synti2 controller number. */
/* FIXME: Should be based on channel number, too */
static
byte_t
synti2_misss_mapControlDest(byte_t ccn){
  return 0; /* FIXME: Always the first controller. */
}

/** Map Pitch bend of a channel to a synti2 controller. */
static
byte_t
synti2_misss_mapPitchDest(byte_t channel){
  return 3; /* FIXME: Always cc3 for pitch. */
}


/** Map MIDI controller value ccval to a synti2 parameter value. */
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
synti2_misss_note(byte_t *misss_out, 
                  byte_t misss_chn, 
                  byte_t midi_note,
                  byte_t midi_vel){
  *misss_out++ = MISSS_MSG_NOTE;
  *misss_out++ = misss_chn; /* TODO: Channel dispersion logic in caller?*/
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



/**
 * Converts a MIDI message (complete; no running status) into a MISSS
 * message that the synti2 synthesizer can handle; returns the length
 * of the output MISSS message. There must be enough space in the
 * output buffer (TODO: document the maximum message size increase).
 *
 * FIXME: Need a MIDI filter module in C, with parameters, after all?
 * If the whole synti2 interface is intended to be in one programming
 * language(?) probably so.. so do it, pls.
 */
int
synti2_midi_to_misss(byte_t *midi_in, 
                     byte_t *misss_out, 
                     int input_size)
{
  byte_t midi_status;
  byte_t midi_chn;
  byte_t midi_vel = 0;
  byte_t midi_note = 0;
  byte_t midi_ccnum = 0;
  byte_t midi_ccval = 0;
  int midi_bendval = 0x2000;

  midi_status = *midi_in++;
  midi_chn = midi_status & 0x0f;

  switch(midi_status >> 4){
  case MIDI_STATUS_NOTE_OFF:
    return synti2_misss_note(misss_out, midi_chn, *midi_in, 0);
  case MIDI_STATUS_NOTE_ON:
    midi_note = *midi_in++;
    midi_vel = *midi_in++;
    return synti2_misss_note(misss_out, midi_chn, midi_note, midi_vel);
  case MIDI_STATUS_KEY_PRESSURE:
    /* Key pressure becomes channel pressure (no polyphonic pressure). */
    /* return synti2_misss_control(misss_out, midi_chn, 
       synti2_misss_mapPressureDest(), synti2_misss_mapPressureValue());*/
    return 0;
  case MIDI_STATUS_CONTROL:
    /* TODO: So far, we do nothing to Channel Mode Messages. */
    midi_ccnum = *midi_in++;
    midi_ccval = *midi_in++;
    return synti2_misss_ramp(misss_out, midi_chn, 
                             synti2_misss_mapControlDest(midi_ccnum), 
                             INSTANT_RAMP_LENGTH,
                             synti2_misss_mapControlValue(midi_ccval));
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
    return synti2_misss_ramp(misss_out, midi_chn, 
                             synti2_misss_mapBendDest(midi_chn), 
                             INSTANT_RAMP_LENGTH,
                             synti2_misss_mapPitchValue(midi_bendval));
    /* FIXME: delete.. WAS:
    return synti2_misss_ramp(misss_out, midi_chn, 
       synti2_misss_mapPitchDest(), 
       synti2_misss_mapPitchValue(midi_bendval));
    */
  case MIDI_STATUS_SYSTEM:
    return synti2_sysmsg_to_misss(midi_status, midi_in, 
                                  misss_out, input_size-1);
  default:
    return 0;
  }
  return 0;
}
