#include "synti2.h"
#include "synti2_jack.h"
#include "synti2_misss.h"
#include "synti2_guts.h"

#include <string.h>

/* The whole thing is relevant only if compiling for Jack. */
#ifdef JACK_MIDI

/* FIXME: Move most of the methods to a MIDI-to-MISSS translator
   module, since jack is necessary only as the outer interface
   layer. The same module can be used with whatever MIDI interface
   (ALSA, VST..). Put the MIDI defines in the corresponding header
   file. That could be a place also for synti2 channel allocation?
   Maybe? But that needs a bigger revamp of the tool-engine
   responsibility share.
 */

/* Some defines to make midi handling more clear in the code part: */
#define MIDI_STATUS_NOTE_OFF 0x8
#define MIDI_STATUS_NOTE_ON 0x9
#define MIDI_STATUS_KEY_PRESSURE 0xa
#define MIDI_STATUS_CONTROL 0xb
#define MIDI_STATUS_PROGRAM 0xc
#define MIDI_STATUS_CHANNEL_PRESSURE 0xd
#define MIDI_STATUS_PITCH_WHEEL 0xe
#define MIDI_STATUS_SYSTEM 0xf



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
static
int
synti2_midi_to_misss(byte_t *midi_in, 
                     byte_t *misss_out, 
                     int input_size)
{
  int midi_status;
  int midi_chn;
  int midi_vel = 0;
  int midi_note = 0;
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


/** Creates a future for the player object that will repeat the next
 *  nframes of midi data from a jack audio connection kit midi port.
 *
 * ... If need be.. this could be made into merging function on top of
 * the sequence being played.. but I won't bother for today's
 * purposes...
 */
static
void
synti2_player_init_from_jack_midi(synti2_player *pl,
                                  jack_port_t *inmidi_port,
                                  jack_nframes_t nframes)
{
  jack_midi_event_t ev;
  jack_nframes_t i, nev;
  byte_t *msg;
  void *midi_in_buffer  = (void *) jack_port_get_buffer (inmidi_port, nframes);
  int out_size;

  /* Re-initialize, and overwrite any former data. */
  pl->playloc = pl->evpool; /* This would "rewind" the song */
  pl->insloc = pl->evpool; /* This would "rewind" the song */
  pl->freeloc = pl->evpool+1;
  pl->idata = 0;
  pl->playloc->next = NULL; /* This effects the deletion of previous msgs.*/
  
  nev = jack_midi_get_event_count(midi_in_buffer);
  for (i=0;i<nev;i++){
    if (jack_midi_event_get (&ev, midi_in_buffer, i) != ENODATA) {

      /* Get next available spot from the data pool */
      msg = pl->data + pl->idata;

      out_size = synti2_midi_to_misss(ev.buffer, msg, ev.size);

      pl->idata += out_size; /*Update the data pool top*/

      synti2_player_event_add(pl, 
                              pl->frames_done + ev.time, 
                              msg, 
                              out_size);
    } else {
      break;
    }
  }
}


/** Interface. */
void
synti2_read_jack_midi(synti2_synth *s,
                      jack_port_t *inmidi_port,
                      jack_nframes_t nframes)
{
  synti2_player_init_from_jack_midi(s->pl, inmidi_port, nframes);
}

#endif
