#include "synti2.h"
#include "synti2_midi.h"
#include "synti2_misss.h"

/** @file synti2_midi.c - Conversion from MIDI messages to MISSS
 * (Midi-like Interface for the Synti Software Synthesizer) messages.
 *
 * The same module can be used with whatever MIDI interface (ALSA,
 *  VST..).
 */

/* FIXME: This could be a place also for synti2 channel allocation?
   Maybe? But that needs a bigger revamp of the tool-engine-interface
   responsibility share.
 */


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

/**
 * Converts a MIDI message (complete; no running status) into a MISSS
 * message that the synti2 synthesizer can handle; returns the length
 * of the output MISSS message. There must be enough space in the
 * output buffer (TODO: document the maximum message size increase).
 *
 * FIXME: Move this to a non-jack specific MIDI interpreter module.
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
  case MIDI_STATUS_CONTROL:
    /* return synti2_misss_control(misss_out, midi_chn, 
       synti2_misss_mapControlDest(xx), 
       synti2_misss_mapControlValue(yy));*/
  case MIDI_STATUS_PROGRAM:
    /* Omit program change. Could have some sound bank logic... */
    return 0;
  case MIDI_STATUS_CHANNEL_PRESSURE:
    /* return synti2_misss_control(misss_out, midi_chn, 
       synti2_misss_mapPressureDest(), synti2_misss_mapPressureValue());*/
  case MIDI_STATUS_PITCH_WHEEL:
    /* return synti2_misss_control(misss_out, midi_chn, 
       synti2_misss_mapPitchDest(), synti2_misss_mapPitchValue());*/
  case MIDI_STATUS_SYSTEM:
    /* return synti2_misss_data(xx,yy,zz)*/
  default:
    break;
  }
  return 0;
}

